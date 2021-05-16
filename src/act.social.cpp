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

#include "logger.hpp"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "chars/character.h"
#include "room.hpp"

// extern variables
extern DESCRIPTOR_DATA *descriptor_list;

// local globals
int number_of_social_messages = -1;
int number_of_social_commands = -1;
struct social_messg *soc_mess_list = NULL;
struct social_keyword *soc_keys_list = NULL;

// local functions
int find_action(char *cmd);
int do_social(CHAR_DATA * ch, char *argument);
void do_insult(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

int find_action(char *cmd)
{
	int bot, top, mid, chk;

	bot = 0;
	top = number_of_social_commands - 1;
	size_t len = strlen(cmd);

	if (top < 0 || !len)
	{
		return -1;
	}

	for (;;)
	{
		mid = (bot + top) / 2;

		if (bot > top)
			return (-1);
		if (!(chk = strn_cmp(cmd, soc_keys_list[mid].keyword, len)))
		{
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

int do_social(CHAR_DATA * ch, char *argument)
{
	int act_nr;
	char social[MAX_INPUT_LENGTH];
	struct social_messg *action;
	CHAR_DATA *vict;

	if (!argument || !*argument)
		return (FALSE);

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Боги наказали вас и вы не можете выражать эмоции!\r\n", ch);
		return (FALSE);
	}

	argument = one_argument(argument, social);

	if ((act_nr = find_action(social)) < 0)
		return (FALSE);

	action = &soc_mess_list[act_nr];
	if (GET_POS(ch) < action->ch_min_pos || GET_POS(ch) > action->ch_max_pos)
	{
		send_to_char("Вам крайне неудобно это сделать.\r\n", ch);
		return (TRUE);
	}

	if (action->char_found && argument)
		one_argument(argument, buf);
	else
		*buf = '\0';

	if (!*buf)
	{
		send_to_char(action->char_no_arg, ch);
		send_to_char("\r\n", ch);
		for (const auto to : world[ch->in_room]->people)
		{
			if (to == ch
				|| ignores(to, ch, IGNORE_EMOTE))
			{
				continue;
			}

			act(action->others_no_arg, FALSE, ch, 0, to, TO_VICT | CHECK_DEAF);
			act(deaf_social, FALSE, ch, 0, to, TO_VICT | CHECK_NODEAF);
		}
		return (TRUE);
	}
	if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
	{
		const auto message = action->not_found
			? action->not_found
			: "Поищите кого-нибудь более доступного для этих целей.\r\n";
		send_to_char(message, ch);
		send_to_char("\r\n", ch);
	}
	else if (vict == ch)
	{
		send_to_char(action->char_no_arg, ch);
		send_to_char("\r\n", ch);
		for (const auto to : world[ch->in_room]->people)
		{
			if (to == ch
				|| ignores(to, ch, IGNORE_EMOTE))
			{
				continue;
			}

			act(action->others_no_arg, FALSE, ch, 0, to, TO_VICT | CHECK_DEAF);
			act(deaf_social, FALSE, ch, 0, to, TO_VICT | CHECK_NODEAF);
		}
	}
	else
	{
		if (GET_POS(vict) < action->vict_min_pos || GET_POS(vict) > action->vict_max_pos)
			act("$N2 сейчас, похоже, не до вас.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		else
		{
			act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
// здесь зарылся баг, связанный с тем, что я не знаю,
// как без грязных хаков сделать так, чтобы
// можно было этот вызов делать в цикле для каждого чара в клетке и при этом
// строка посылалась произвольному чару, но действие осуществлялось над vict.
// в итоге даже если чар в клетке игнорирует чара ch, то все равно он
// будет видеть его действия, имеющие цель, если при этом цель -- не он сам.
// для глухих -- то же самое.
			act(action->others_found, FALSE, ch, 0, vict, TO_NOTVICT | CHECK_DEAF);
			act(deaf_social, FALSE, ch, 0, 0, TO_ROOM | CHECK_NODEAF);
			if (!ignores(vict, ch, IGNORE_EMOTE))
				act(action->vict_found, FALSE, ch, 0, vict, TO_VICT | CHECK_DEAF);
		}
	}
	return (TRUE);
}

void do_insult(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Боги наказали вас и вы не можете выражать эмоции!\r\n", ch);
		return;
	}
	if (*arg)
	{
		if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			send_to_char("&KА он вас и не услышит :(&n\r\n", ch);
		else
		{
			if (victim != ch)
			{
				sprintf(buf, "&KВы оскорбили %s.&n\r\n", GET_PAD(victim, 3));
				send_to_char(buf, ch);

				switch (number(0, 2))
				{
				case 0:
					if (IS_MALE(ch))
					{
						if (IS_MALE(victim))
							act("&K$n высмеял$g вашу манеру держать меч !&n",
								FALSE, ch, 0, victim, TO_VICT);
						else
							act("&K$n заявил$g, что удел любой женщины - дети, кухня и церковь.&n", FALSE, ch, 0, victim, TO_VICT);
					}
					else  	// Ch == Woman
					{
						if (IS_MALE(victim))
							act("&K$n заявил$g вам, что у н$s больше... (что $e имел$g в виду?)&n", FALSE, ch, 0, victim, TO_VICT);
						else
							act("&K$n обьявил$g всем о вашем близком родстве с Бабой-Ягой.&n", FALSE, ch, 0, victim, TO_VICT);
					}
					break;
				case 1:
					act("&K$n1 чем-то не удовлетворила ваша мама!&n", FALSE,
						ch, 0, victim, TO_VICT);
					break;
				default:
					act("&K$n предложил$g вам посетить ближайший хутор!\r\n"
						"$e заявил$g, что там обитают на редкость крупные бабочки.&n",
						FALSE, ch, 0, victim, TO_VICT);
					break;
				}	// end switch

				act("&K$n оскорбил$g $N1. СМЕРТЕЛЬНО.&n", TRUE, ch, 0, victim, TO_NOTVICT);
			}
			else  	// ch == victim
			{
				send_to_char("&KВы почувствовали себя оскорбленным.&n\r\n", ch);
			}
		}
	}
	else
		send_to_char("&KВы уверены, что стоит оскорблять такими словами всех?&n\r\n", ch);
}

char *fread_action(FILE * fl, int nr)
{
	char buf[MAX_STRING_LENGTH];

	const char* result = fgets(buf, MAX_STRING_LENGTH, fl);
	UNUSED_ARG(result);

	if (feof(fl))
	{
		log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
		exit(1);
	}
	if (*buf == '#')
		return (NULL);

	buf[strlen(buf) - 1] = '\0';
	return (str_dup(buf));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
