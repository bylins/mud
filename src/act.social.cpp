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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "char.hpp"
#include "room.hpp"

// extern variables
extern DESCRIPTOR_DATA *descriptor_list;

void get_one_line(FILE * fl, char *buf);

// local globals
int top_of_socialm = -1;
int top_of_socialk = -1;
struct social_messg *soc_mess_list = NULL;
struct social_keyword *soc_keys_list = NULL;

// local functions
int find_action(char *cmd);
int do_social(CHAR_DATA * ch, char *argument);
ACMD(do_insult);

int find_action(char *cmd)
{
	int bot, top, mid, len, chk;

	bot = 0;
	top = top_of_socialk;
	len = strlen(cmd);

	if (top < 0 || !len)
		return (-1);

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
	CHAR_DATA *vict, *to;

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
		for (to = world[ch->in_room]->people; to; to = to->next_in_room)
		{
			if (to == ch || ignores(to, ch, IGNORE_EMOTE))
				continue;
			act(action->others_no_arg, FALSE, ch, 0, to, TO_VICT | CHECK_DEAF);
			act(deaf_social, FALSE, ch, 0, to, TO_VICT | CHECK_NODEAF);
		}
		return (TRUE);
	}
	if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
	{
		send_to_char(action->not_found ? action->not_found :
					 "Поищите кого-нибудь более доступного для этих целей.\r\n", ch);
		send_to_char("\r\n", ch);
	}
	else if (vict == ch)
	{
		send_to_char(action->char_no_arg, ch);
		send_to_char("\r\n", ch);
		for (to = world[ch->in_room]->people; to; to = to->next_in_room)
		{
			if (to == ch || ignores(to, ch, IGNORE_EMOTE))
				continue;
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



ACMD(do_insult)
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
				sprintf(buf, "&KВы оскорбили %s.&n\r\n", GET_PAD(victim, 1));
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

char *str_dup_bl(const char *source)
{
	char line[MAX_INPUT_LENGTH];

	line[0] = 0;
	if (source[0])
	{
		strcat(line, "&K");
		strcat(line, source);
		strcat(line, "&n");
	}

	return (str_dup(line));
}

void load_socials(FILE * fl)
{
	char line[MAX_INPUT_LENGTH], *scan, next_key[MAX_INPUT_LENGTH];
	int key = -1, message = -1, c_min_pos, c_max_pos, v_min_pos, v_max_pos, what;

	// get the first keyword line
	get_one_line(fl, line);
	while (*line != '$')
	{
		message++;
		scan = one_word(line, next_key);
		while (*next_key)
		{
			key++;
			log("Social %d '%s' - message %d", key, next_key, message);
			soc_keys_list[key].keyword = str_dup(next_key);
			soc_keys_list[key].social_message = message;
			scan = one_word(scan, next_key);
		}

		what = 0;
		get_one_line(fl, line);
		while (*line != '#')
		{
			scan = line;
			skip_spaces(&scan);
			if (scan && *scan && *scan != ';')
			{
				switch (what)
				{
				case 0:
					if (sscanf
							(scan, " %d %d %d %d \n", &c_min_pos, &c_max_pos,
							 &v_min_pos, &v_max_pos) < 4)
					{
						log("SYSERR: format error in %d social file near social '%s' #d #d #d #d\n", message, line);
						exit(1);
					}
					soc_mess_list[message].ch_min_pos = c_min_pos;
					soc_mess_list[message].ch_max_pos = c_max_pos;
					soc_mess_list[message].vict_min_pos = v_min_pos;
					soc_mess_list[message].vict_max_pos = v_max_pos;
					break;
				case 1:
					soc_mess_list[message].char_no_arg = str_dup_bl(scan);
					break;
				case 2:
					soc_mess_list[message].others_no_arg = str_dup_bl(scan);
					break;
				case 3:
					soc_mess_list[message].char_found = str_dup_bl(scan);
					break;
				case 4:
					soc_mess_list[message].others_found = str_dup_bl(scan);
					break;
				case 5:
					soc_mess_list[message].vict_found = str_dup_bl(scan);
					break;
				case 6:
					soc_mess_list[message].not_found = str_dup_bl(scan);
					break;
				}
			}
			if (!scan || *scan != ';')
				what++;
			get_one_line(fl, line);
		}
		// get next keyword line (or $)
		get_one_line(fl, line);
	}
}

char *fread_action(FILE * fl, int nr)
{
	char buf[MAX_STRING_LENGTH];

	fgets(buf, MAX_STRING_LENGTH, fl);
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
