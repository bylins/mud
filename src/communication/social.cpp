/* ************************************************************************
*   File: act.social.cpp                                Part of Bylins    *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                       *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "handler.h"
#include "social.h"

// local globals
int number_of_social_messages = -1;
int number_of_social_commands = -1;
struct SocialMessages *soc_mess_list = nullptr;
struct SocialKeyword *soc_keys_list = nullptr;

int find_action(char *cmd) {
	int bot, top, mid, chk;

	bot = 0;
	top = number_of_social_commands - 1;
	size_t len = strlen(cmd);

	if (top < 0 || !len) {
		return -1;
	}

	for (;;) {
		mid = (bot + top) / 2;

		if (bot > top)
			return (-1);
		if (!(chk = strn_cmp(cmd, soc_keys_list[mid].keyword, len))) {
			while (mid > 0 && !strn_cmp(cmd, soc_keys_list[mid - 1].keyword, len))
				mid--;
			return (soc_keys_list[mid].social_message);
		}

		if (chk > 0)
			bot = mid + 1;
		else
			top = mid - 1;
	}
}

const char *deaf_social = "&K$n попытал$u очень эмоционально выразить свою мысль.&n";

int do_social(CharData *ch, char *argument) {
	int act_nr;
	char social[kMaxInputLength];
	struct SocialMessages *action;
	CharData *vict;

	if (!argument || !*argument)
		return (false);

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Боги наказали вас и вы не можете выражать эмоции!\r\n", ch);
		return (false);
	}

	argument = one_argument(argument, social);

	if ((act_nr = find_action(social)) < 0)
		return (false);

	action = &soc_mess_list[act_nr];
	if (GET_POS(ch) < action->ch_min_pos || GET_POS(ch) > action->ch_max_pos) {
		send_to_char("Вам крайне неудобно это сделать.\r\n", ch);
		return (true);
	}

	if (action->char_found && argument)
		one_argument(argument, buf);
	else
		*buf = '\0';

	if (!*buf) {
		send_to_char(action->char_no_arg, ch);
		send_to_char("\r\n", ch);
		for (const auto to : world[ch->in_room]->people) {
			if (to == ch
				|| ignores(to, ch, EIgnore::kEmote)) {
				continue;
			}

			act(action->others_no_arg, false, ch, nullptr, to, kToVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, to, kToVict | kToDeaf);
		}
		return (true);
	}
	if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		const auto message = action->not_found
							 ? action->not_found
							 : "Поищите кого-нибудь более доступного для этих целей.\r\n";
		send_to_char(message, ch);
		send_to_char("\r\n", ch);
	} else if (vict == ch) {
		send_to_char(action->char_no_arg, ch);
		send_to_char("\r\n", ch);
		for (const auto to : world[ch->in_room]->people) {
			if (to == ch
				|| ignores(to, ch, EIgnore::kEmote)) {
				continue;
			}

			act(action->others_no_arg, false, ch, nullptr, to, kToVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, to, kToVict | kToDeaf);
		}
	} else {
		if (GET_POS(vict) < action->vict_min_pos || GET_POS(vict) > action->vict_max_pos)
			act("$N2 сейчас, похоже, не до вас.", false, ch, nullptr, vict, kToChar | kToSleep);
		else {
			act(action->char_found, 0, ch, nullptr, vict, kToChar | kToSleep);
// здесь зарылся баг, связанный с тем, что я не знаю,
// как без грязных хаков сделать так, чтобы
// можно было этот вызов делать в цикле для каждого чара в клетке и при этом
// строка посылалась произвольному чару, но действие осуществлялось над vict.
// в итоге даже если чар в клетке игнорирует чара ch, то все равно он
// будет видеть его действия, имеющие цель, если при этом цель -- не он сам.
// для глухих -- то же самое.
			act(action->others_found, false, ch, nullptr, vict, kToNotVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, nullptr, kToRoom | kToDeaf);
			if (!ignores(vict, ch, EIgnore::kEmote))
				act(action->vict_found, false, ch, nullptr, vict, kToVict | kToNotDeaf);
		}
	}
	return (true);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
