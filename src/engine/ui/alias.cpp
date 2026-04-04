/* ************************************************************************
*  File: alias.cpp				A utility to Bylins 	  *
* Usage: writing/reading player's aliases.				  *
*									  *
* Code done by Jeremy Hess and Chad Thompson				  *
* Modifed by George Greer for inclusion into CircleMUD bpl15.		  *
*									  *
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		  *
*									  *
*  $Author$                                                       *
*  $Date$                                           *
*  $Revision$                                                      *
*********************************************************************** */

#include <sstream>
#include <vector>

#include "engine/entities/char_data.h"
#include "engine/ui/alias.h"

void WriteAliases(CharData *ch) {
	FILE *file;
	char fn[kMaxStringLength];
	struct alias_data *temp;

	log("Write alias %s", GET_NAME(ch));
	get_filename(GET_NAME(ch), fn, kAliasFile);
	remove(fn);

	if (GET_ALIASES(ch) == nullptr)
		return;

	if ((file = fopen(fn, "w")) == nullptr) {
		log("SYSERR: Couldn't save aliases for %s in '%s'.", GET_NAME(ch), fn);
		perror("SYSERR: WriteAliases");
		return;
	}

	for (temp = GET_ALIASES(ch); temp; temp = temp->next) {
		size_t aliaslen = strlen(temp->alias);
		size_t repllen = strlen(temp->replacement);

		fprintf(file, "%d\n%s\n"    // Alias
					  "%d\n%s\n"    // Replacement
					  "%d\n",    // Type
				static_cast<int>(aliaslen),
				temp->alias,
				static_cast<int>(repllen),
				temp->replacement,
				temp->type);
	}

	fclose(file);
}

void ReadAliases(CharData *ch) {
	FILE *file;
	char xbuf[kMaxStringLength];
	struct alias_data *t2;
	int length;

	log("Read alias %s", GET_NAME(ch));
	get_filename(GET_NAME(ch), xbuf, kAliasFile);

	if ((file = fopen(xbuf, "r")) == nullptr) {
		if (errno != ENOENT) {
			log("SYSERR: Couldn't open alias file '%s' for %s.", xbuf, GET_NAME(ch));
			perror("SYSERR: ReadAliases");
		}
		return;
	}

	CREATE(GET_ALIASES(ch), 1);
	t2 = GET_ALIASES(ch);

	const char *dummyc;
	int dummyi;
	for (;;)        // Read the aliased command.
	{
		dummyi = fscanf(file, "%d\n", &length);
		dummyc = fgets(xbuf, length + 1, file);
		t2->alias = str_dup(xbuf);
		// Build the replacement.
		dummyi = fscanf(file, "%d\n", &length);
		dummyc = fgets(xbuf, length + 1, file);
		t2->replacement = str_dup(xbuf);
		// Figure out the alias type.
		dummyi = fscanf(file, "%d\n", &length);
		t2->type = length;
		if (feof(file))
			break;

		CREATE(t2->next, 1);
		t2 = t2->next;
	};
	UNUSED_ARG(dummyc);
	UNUSED_ARG(dummyi);

	fclose(file);
}

struct alias_data *FindAlias(struct alias_data *alias_list, char *str) {
	while (alias_list != nullptr) {
		if (*str == *alias_list->alias)    // hey, every little bit counts :-)
			if (!strcmp(str, alias_list->alias))
				return (alias_list);

		alias_list = alias_list->next;
	}

	return (nullptr);
}

void FreeAlias(struct alias_data *a) {
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}

/*
 * Valid numeric replacements are only $1 ... $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.) Also, valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
constexpr int kNumTokens{9};

void perform_complex_alias(struct iosystem::TextBlocksQueue *input_q, char *orig, struct alias_data *a) {
	struct iosystem::TextBlocksQueue temp_queue;
	temp_queue.head = temp_queue.tail = nullptr;

	// разбиваем аргументы (пропуская первое слово если это имя алиаса)
	std::vector<std::string> tokens;
	std::string orig_str(orig);
	std::istringstream stream(orig_str);
	std::string word;
	if (stream >> word) {
		// первое слово может быть именем алиаса - пропускаем его
		if (str_cmp(word.c_str(), a->alias) != 0) {
			tokens.push_back(word);
		}
		while (stream >> word && tokens.size() < static_cast<size_t>(kNumTokens)) {
			tokens.push_back(word);
		}
	}

	// подставляем переменные в шаблон замены
	std::string result;
	const char *repl = a->replacement;
	while (*repl) {
		if (*repl == kAliasSepChar) {
			write_to_q(result.c_str(), &temp_queue, 1);
			result.clear();
		} else if (*repl == kAliasVarChar) {
			repl++;
			int num = *repl - '1';
			if (num >= 0 && num < static_cast<int>(tokens.size())) {
				result += tokens[num];
			} else if (*repl == kAliasGlobChar) {
				result += orig_str;
			} else {
				result += *repl;
				if (*repl == '$') {
					result += '$';
				}
			}
		} else {
			result += *repl;
		}
		repl++;
	}

	if (!result.empty()) {
		write_to_q(result.c_str(), &temp_queue, 1);
	}

	// push our temp_queue on to the _front_ of the input queue
	if (input_q->head == nullptr) {
		*input_q = temp_queue;
	} else {
		temp_queue.tail->next = input_q->head;
		input_q->head = temp_queue.head;
	}
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int PerformAlias(DescriptorData *d, char *orig) {
	char first_arg[kMaxInputLength], *ptr;
	struct alias_data *a, *tmp;

	// Mobs don't have alaises. //
	if (d->character->IsNpc())
		return (0);

	// bail out immediately if the guy doesn't have any aliases //
	if ((tmp = GET_ALIASES(d->character)) == nullptr)
		return (0);

	// find the alias we're supposed to match //
	ptr = any_one_arg(orig, first_arg);

	// bail out if it's null //
	if (!*first_arg)
		return (0);

	// if the first arg is not an alias, return without doing anything //
	if ((a = FindAlias(tmp, first_arg)) == nullptr)
		return (0);

	if (a->type == kAliasSimple) {
		strcpy(orig, a->replacement);
		return (0);
	} else {
		perform_complex_alias(&d->input, ptr, a);
		return (1);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
