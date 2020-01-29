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

#include "logger.hpp"
#include "char.hpp"
#include "utils.h"

void write_aliases(CHAR_DATA * ch);
void read_aliases(CHAR_DATA * ch);

void write_aliases(CHAR_DATA * ch)

{
	FILE *file;
	char fn[MAX_STRING_LENGTH];
	struct alias_data *temp;

	log("Write alias %s", GET_NAME(ch));
	get_filename(GET_NAME(ch), fn, ALIAS_FILE);
	remove(fn);

	if (GET_ALIASES(ch) == NULL)
		return;

	if ((file = fopen(fn, "w")) == NULL)
	{
		log("SYSERR: Couldn't save aliases for %s in '%s'.", GET_NAME(ch), fn);
		perror("SYSERR: write_aliases");
		return;
	}

	for (temp = GET_ALIASES(ch); temp; temp = temp->next)
	{
		size_t aliaslen = strlen(temp->alias);
		size_t repllen = strlen(temp->replacement) - 1;

		fprintf(file, "%d\n%s\n"	// Alias
				"%d\n%s\n"	// Replacement
				"%d\n",	// Type
			static_cast<int>(aliaslen),
			temp->alias,
			static_cast<int>(repllen),
			temp->replacement + 1,
			temp->type);
	}

	fclose(file);
}

void read_aliases(CHAR_DATA * ch)
{
	FILE *file;
	char xbuf[MAX_STRING_LENGTH];
	struct alias_data *t2;
	int length;

	log("Read alias %s", GET_NAME(ch));
	get_filename(GET_NAME(ch), xbuf, ALIAS_FILE);

	if ((file = fopen(xbuf, "r")) == NULL)
	{
		if (errno != ENOENT)
		{
			log("SYSERR: Couldn't open alias file '%s' for %s.", xbuf, GET_NAME(ch));
			perror("SYSERR: read_aliases");
		}
		return;
	}

	CREATE(GET_ALIASES(ch), 1);
	t2 = GET_ALIASES(ch);

	const char* dummyc;
	int dummyi;
	for (;;)  		// Read the aliased command.
	{
		dummyi = fscanf(file, "%d\n", &length);
		dummyc = fgets(xbuf, length + 1, file);
		t2->alias = str_dup(xbuf);

		// Build the replacement.
		dummyi = fscanf(file, "%d\n", &length);
		*xbuf = ' ';	// Doesn't need terminated, fgets() will.
		dummyc = fgets(xbuf + 1, length + 1, file);
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
