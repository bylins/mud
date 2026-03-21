/**
\file do_alias.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/alias.h"

void do_alias(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char *repl;
	struct alias_data *a;

	if (ch->IsNpc())
		return;

	repl = any_one_arg(argument, arg);

	if (!*arg) {
		SendMsgToChar("Определены следующие алиасы:\r\n", ch);
		if ((a = ch->player_specials->aliases) == nullptr)
			SendMsgToChar(" Нет алиасов.\r\n", ch);
		else {
			while (a != nullptr) {
				sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
				SendMsgToChar(buf, ch);
				a = a->next;
			}
		}
	} else {
		if ((a = FindAlias(ch->player_specials->aliases, arg)) != nullptr) {
			REMOVE_FROM_LIST(a, ch->player_specials->aliases);
			FreeAlias(a);
		}

		if (!*repl) {
			if (a == nullptr)
				SendMsgToChar("Такой алиас не определен.\r\n", ch);
			else
				SendMsgToChar("Алиас успешно удален.\r\n", ch);
		} else {
			if (!str_cmp(arg, "alias")) {
				SendMsgToChar("Вы не можете определить алиас 'alias'.\r\n", ch);
				return;
			}
			CREATE(a, 1);
			a->alias = str_dup(arg);
			delete_doubledollar(repl);
			a->replacement = str_dup(repl);
			if (strchr(repl, kAliasSepChar) || strchr(repl, kAliasVarChar))
				a->type = kAliasComplex;
			else
				a->type = kAliasSimple;
			a->next = ch->player_specials->aliases;
			ch->player_specials->aliases = a;
			SendMsgToChar("Алиас успешно добавлен.\r\n", ch);
		}
		SetWaitState(ch, 1 * kBattleRound);
		WriteAliases(ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
