/*************************************************************************
*   File: act.comm.cpp                                  Part of Bylins    *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "dg_script/dg_scripts.h"
#include "auction.h"
#include "privilege.hpp"
#include "chars/character.h"
#include "chars/char_player.hpp"
#include "remember.hpp"
#include "house.h"
#include "obj.hpp"
#include "room.hpp"
#include "spam.hpp"
#include "char_obj_utils.inl"
#include "chars/world.characters.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <sstream>
#include <list>
#include <string>

// extern variables
extern DESCRIPTOR_DATA *descriptor_list;
extern TIME_INFO_DATA time_info;

// local functions
void perform_tell(CHAR_DATA * ch, CHAR_DATA * vict, char *arg);
int is_tell_ok(CHAR_DATA * ch, CHAR_DATA * vict);
bool tell_can_see(CHAR_DATA *ch, CHAR_DATA *vict);

// external functions
extern char *diag_timer_to_char(const OBJ_DATA* obj);
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);

void do_say(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gsay(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_tell(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_reply(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_spec_comm(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_write(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_page(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gen_comm(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_pray_gods(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_ignore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

#define SIELENCE ("Вы немы, как рыба об лед.\r\n")
#define SOUNDPROOF ("Стены заглушили ваши слова.\r\n")

void do_say(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/){
	skip_spaces(&argument);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (!*argument)
		send_to_char("Вы задумались: \"Чего бы такого сказать?\"\r\n", ch);
	else {
		sprintf(buf, "$n сказал$g : '%s'", argument);

//      act (buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG | CHECK_DEAF);
// shapirus; для возможности игнорирования теллов в клетку
// пришлось изменить act в клетку на проход по клетке
		for (const auto to : world[ch->in_room]->people)
		{
			if (ch == to || ignores(to, ch, IGNORE_SAY))
			{
				continue;
			}

			act(buf, FALSE, ch, 0, to, TO_VICT | DG_NO_TRIG | CHECK_DEAF);
		}

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		{
			send_to_char(OK, ch);
		}
		else
		{
			delete_doubledollar(argument);
			sprintf(buf, "Вы сказали : '%s'\r\n", argument);
			send_to_char(buf, ch);
		}
		speech_mtrigger(ch, argument);
		speech_wtrigger(ch, argument);
	}
}

void do_gsay(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *k;
	struct follow_type *f;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
	{
		send_to_char("Вы не являетесь членом группы!\r\n", ch);
		return;
	}

	if (!*argument)
	{
		send_to_char("О чем вы хотите сообщить своей группе?\r\n", ch);
	}
	else
	{
		if (ch->has_master())
		{
			k = ch->get_master();
		}
		else
		{
			k = ch;
		}

		sprintf(buf, "$n сообщил$g группе : '%s'", argument);

		if (AFF_FLAGGED(k, EAffectFlag::AFF_GROUP)
			&& k != ch
			&& !ignores(k, ch, IGNORE_GROUP))
		{
			act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP | CHECK_DEAF);
			// added by WorM  групптелы 2010.10.13
			if(!AFF_FLAGGED(k, EAffectFlag::AFF_DEAFNESS)
				&& GET_POS(k) > POS_DEAD)
			{
				sprintf(buf1, "%s сообщил%s группе : '%s'\r\n", tell_can_see(ch, k) ? GET_NAME(ch) : "Кто-то", GET_CH_VIS_SUF_1(ch, k), argument);
				k->remember_add(buf1, Remember::ALL);
				k->remember_add(buf1, Remember::GROUP);
			}
			//end by WorM
		}
		for (f = k->followers; f; f = f->next)
		{
			if (AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
				&& (f->follower != ch)
				&& !ignores(f->follower, ch, IGNORE_GROUP))
			{
				act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP | CHECK_DEAF);
				// added by WorM  групптелы 2010.10.13
				if (!AFF_FLAGGED(f->follower, EAffectFlag::AFF_DEAFNESS)
					&& GET_POS(f->follower) > POS_DEAD)
				{
					sprintf(buf1, "%s сообщил%s группе : '%s'\r\n", tell_can_see(ch, f->follower) ? GET_NAME(ch) : "Кто-то", GET_CH_VIS_SUF_1(ch, f->follower), argument);
					f->follower->remember_add(buf1, Remember::ALL);
					f->follower->remember_add(buf1, Remember::GROUP);
				}
				//end by WorM
			}
		}

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
		{
			sprintf(buf, "Вы сообщили группе : '%s'\r\n", argument);
			send_to_char(buf, ch);
			// added by WorM  групптелы 2010.10.13
			ch->remember_add(buf, Remember::ALL);
			ch->remember_add(buf, Remember::GROUP);
			//end by WorM
		}
	}
}

bool tell_can_see(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (CAN_SEE_CHAR(vict, ch) || IS_IMMORTAL(ch) || GET_INVIS_LEV(ch))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void perform_tell(CHAR_DATA * ch, CHAR_DATA * vict, char *arg)
{
// shapirus: не позволим телять, если жертва не видит и включила
// соответствующий режим; имморталы могут телять всегда
	if (PRF_FLAGGED(vict, PRF_NOINVISTELL)
			&& !CAN_SEE(vict, ch)
			&& GET_LEVEL(ch) < LVL_IMMORT
			&& !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		act("$N не любит разговаривать с теми, кого не видит.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		return;
	}

	// TODO: если в act() останется показ иммов, то это и эхо ниже переделать на act()
	if (tell_can_see(ch, vict))
	{
		snprintf(buf, MAX_STRING_LENGTH, "%s сказал%s вам : '%s'", GET_NAME(ch), GET_CH_SUF_1(ch), arg);
	}
	else
	{
		snprintf(buf, MAX_STRING_LENGTH, "Кто-то сказал вам : '%s'", arg);
	}
	snprintf(buf1, MAX_STRING_LENGTH, "%s%s%s\r\n", CCICYN(vict, C_NRM), CAP(buf), CCNRM(vict, C_NRM));
	send_to_char(buf1, vict);
	if (!IS_NPC(vict))
	{
		vict->remember_add(buf1, Remember::ALL);
	}

	if (!IS_NPC(vict) && !IS_NPC(ch))
	{
		snprintf(buf, MAX_STRING_LENGTH, "%s%s : '%s'%s\r\n", CCICYN(vict, C_NRM),
				tell_can_see(ch, vict) ? GET_NAME(ch) : "Кто-то", arg, CCNRM(vict, C_NRM));
		vict->remember_add(buf, Remember::PERSONAL);
	}

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
	{
		send_to_char(OK, ch);
	}
	else
	{
		snprintf(buf, MAX_STRING_LENGTH, "%sВы сказали %s : '%s'%s\r\n", CCICYN(ch, C_NRM),
				tell_can_see(vict, ch) ? vict->player_data.PNames[2].c_str() : "кому-то", arg, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		if (!IS_NPC(ch))
		{
			ch->remember_add(buf, Remember::ALL);
		}
	}

	if (!IS_NPC(vict) && !IS_NPC(ch))
	{
		vict->set_answer_id(GET_IDNUM(ch));
	}
}

int is_tell_ok(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (ch == vict)
	{
		send_to_char("Вы начали потихоньку разговаривать с самим собой.\r\n", ch);
		return (FALSE);
	}
	else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вам запрещено обращаться к другим игрокам.\r\n", ch);
		return (FALSE);
	}
	else if (!IS_NPC(vict) && !vict->desc)  	// linkless
	{
		act("$N потерял$G связь в этот момент.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		return (FALSE);
	}
	else if (PLR_FLAGGED(vict, PLR_WRITING))
	{
		act("$N пишет сообщение - повторите попозже.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		return (FALSE);
	}

	if (IS_GOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		return (TRUE);

	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
		send_to_char(SOUNDPROOF, ch);
	else if ((!IS_NPC(vict) &&
			  (PRF_FLAGGED(vict, PRF_NOTELL) || ignores(vict, ch, IGNORE_TELL))) ||
			 ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
		act("$N не сможет вас услышать.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (GET_POS(vict) < POS_RESTING || AFF_FLAGGED(vict, EAffectFlag::AFF_DEAFNESS))
		act("$N вас не услышит.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return (TRUE);

	return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
void do_tell(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict = NULL;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char(SIELENCE, ch);
		return;
	}

	/* Непонятно нафига нужно
	if (ROOM_FLAGGED(ch->in_room, ROOM_ARENARECV))
	{
		send_to_char(SOUNDPROOF, ch);
		return;
	}
	*/

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
	{
		send_to_char("Что и кому вы хотите сказать?\r\n", ch);
	}
	else if (!(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
	{
		send_to_char(NOPERSON, ch);
	}
	else if (IS_NPC(vict))
		send_to_char(NOPERSON, ch);
	else if (is_tell_ok(ch, vict))
	{
		if (PRF_FLAGGED(ch, PRF_NOTELL))
			send_to_char("Ответить вам не смогут!\r\n", ch);
		perform_tell(ch, vict, buf2);
	}
}

void do_reply(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (ch->get_answer_id() == NOBODY)
		send_to_char("Вам некому ответить!\r\n", ch);
	else if (!*argument)
		send_to_char("Что вы собираетесь ответить?\r\n", ch);
	else
	{
		/*
		 * Make sure the person you're replying to is still playing by searching
		 * for them.  Note, now last tell is stored as player IDnum instead of
		 * a pointer, which is much better because it's safer, plus will still
		 * work if someone logs out and back in again.
		 */

		/*
		 * XXX: A descriptor list based search would be faster although
		 *      we could not find link dead people.  Not that they can
		 *      hear tells anyway. :) -gg 2/24/98
		 */
		bool found = false;
		for (const auto i : character_list)
		{
			if (!IS_NPC(i)
				&& GET_IDNUM(i) == ch->get_answer_id())
			{
				if (is_tell_ok(ch, i.get()))
				{
					perform_tell(ch, i.get(), argument);
				}

				found = true;

				break;
			}
		}

		if (!found)
		{
			send_to_char("Этого игрока уже нет в игре.\r\n", ch);
		}
	}
}

void do_spec_comm(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *vict;
	const char *action_sing, *action_plur, *action_others, *vict1, *vict2;
	char vict3[MAX_INPUT_LENGTH];

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_WHISPER)
	{
		action_sing = "шепнуть";
		vict1 = "кому";
		vict2 = "вам";
		action_plur = "прошептал";
		action_others = "$n что-то прошептал$g $N2.";
	}
	else
	{
		action_sing = "спросить";
		vict1 = "у кого";
		vict2 = "у вас";
		action_plur = "спросил";
		action_others = "$n задал$g $N2 вопрос.";
	}

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
	{
		sprintf(buf, "Что вы хотите %s.. и %s?\r\n", action_sing, vict1);
		send_to_char(buf, ch);
	}
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if (vict == ch)
		send_to_char("От ваших уст до ушей - всего одна ладонь...\r\n", ch);
	else if (ignores(vict, ch, subcmd == SCMD_WHISPER ? IGNORE_WHISPER : IGNORE_ASK))
	{
		sprintf(buf, "%s не желает вас слышать.\r\n", GET_NAME(vict));
		send_to_char(buf, ch);
	}
	else
	{
		if (subcmd == SCMD_WHISPER)
			sprintf(vict3, "%s", GET_PAD(vict, 2));
		else
			sprintf(vict3, "у %s", GET_PAD(vict, 1));

		std::stringstream buffer;
		buffer << "$n " << action_plur << "$g " << vict2 << " : " << buf2;
//		sprintf(buf, "$n %s$g %s : '%s'", action_plur, vict2, buf2);
		act(buffer.str().c_str(), FALSE, ch, 0, vict, TO_VICT | CHECK_DEAF);

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
		{
			std::stringstream buffer;
			buffer << "Вы " << action_plur << "и " << vict3 << " : '"<< buf2 << "'" << std::endl;
//			sprintf(buf, "Вы %sи %s : '%s'\r\n", action_plur, vict3, buf2);
			send_to_char(buffer.str(), ch);
		}

		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
	}
}

#define MAX_NOTE_LENGTH 4096	// arbitrary

void do_write(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	OBJ_DATA *paper, *pen = NULL;
	char *papername, *penname;

	papername = buf1;
	penname = buf2;

	two_arguments(argument, papername, penname);

	if (!ch->desc)
		return;

	if (!*papername)  	// nothing was delivered
	{
		send_to_char("Написать?  Чем?  И на чем?\r\n", ch);
		return;
	}
	if (*penname)  		// there were two arguments
	{
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
		{
			sprintf(buf, "У вас нет %s.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
		{
			sprintf(buf, "У вас нет %s.\r\n", penname);
			send_to_char(buf, ch);
			return;
		}
	}
	else  		// there was one arg.. let's see what we can find
	{
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
		{
			sprintf(buf, "Вы не видите %s в инвентаре.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(paper) == OBJ_DATA::ITEM_PEN)  	// oops, a pen..
		{
			pen = paper;
			paper = NULL;
		}
		else if (GET_OBJ_TYPE(paper) != OBJ_DATA::ITEM_NOTE)
		{
			send_to_char("Вы не можете на ЭТОМ писать.\r\n", ch);
			return;
		}
		// One object was found.. now for the other one.
		if (!GET_EQ(ch, WEAR_HOLD))
		{
			sprintf(buf, "Вы нечем писать!\r\n");
			send_to_char(buf, ch);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD)))
		{
			send_to_char("Вы держите что-то невидимое!  Жаль, но писать этим трудно!!\r\n", ch);
			return;
		}
		if (pen)
			paper = GET_EQ(ch, WEAR_HOLD);
		else
			pen = GET_EQ(ch, WEAR_HOLD);
	}


	// ok.. now let's see what kind of stuff we've found
	if (GET_OBJ_TYPE(pen) != OBJ_DATA::ITEM_PEN)
	{
		act("Вы не умеете писать $o4.", FALSE, ch, pen, 0, TO_CHAR);
	}
	else if (GET_OBJ_TYPE(paper) != OBJ_DATA::ITEM_NOTE)
	{
		act("Вы не можете писать на $o5.", FALSE, ch, paper, 0, TO_CHAR);
	}
	else if (!paper->get_action_description().empty())
	{
		send_to_char("Там уже что-то записано.\r\n", ch);
	}
	else  			// we can write - hooray!
	{
		/* this is the PERFECT code example of how to set up:
		 * a) the text editor with a message already loaed
		 * b) the abort buffer if the player aborts the message
		 */
		ch->desc->backstr = NULL;
		send_to_char("Можете писать.  (/s СОХРАНИТЬ ЗАПИСЬ  /h ПОМОЩЬ)\r\n", ch);
		// ok, here we check for a message ALREADY on the paper
		if (!paper->get_action_description().empty())  	// we str_dup the original text to the descriptors->backstr
		{
			ch->desc->backstr = str_dup(paper->get_action_description().c_str());
			// send to the player what was on the paper (cause this is already
			// loaded into the editor)
			send_to_char(paper->get_action_description().c_str(), ch);
		}
		act("$n начал$g писать.", TRUE, ch, 0, 0, TO_ROOM);
		// assign the descriptor's->str the value of the pointer to the text
		// pointer so that we can reallocate as needed (hopefully that made
		// sense :>)
		const AbstractStringWriter::shared_ptr writer(new CActionDescriptionWriter(*paper));
		string_write(ch->desc, writer, MAX_NOTE_LENGTH, 0, NULL);
	}
}

void do_page(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *vict;

	half_chop(argument, arg, buf2);

	if (IS_NPC(ch))
		send_to_char("Создания-не-персонажи этого не могут.. ступайте.\r\n", ch);
	else if (!*arg)
		send_to_char("Whom do you wish to page?\r\n", ch);
	else {
		std::stringstream buffer;
		buffer << "\007\007*$n*" << buf2;
//		sprintf(buf, "\007\007*$n* %s", buf2);
		if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
		{
			if (IS_GRGOD(ch))
			{
				for (d = descriptor_list; d; d = d->next)
				{
					if (STATE(d) == CON_PLAYING && d->character)
					{
						act(buf, FALSE, ch, 0, d->character.get(), TO_VICT);
					}
				}
			}
			else
			{
				send_to_char("Это доступно только БОГАМ!\r\n", ch);
			}
			return;
		}
		if ((vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) != NULL)
		{
			act(buffer.str().c_str(), FALSE, ch, 0, vict, TO_VICT);
			if (PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(OK, ch);
			else
				act(buffer.str().c_str(), FALSE, ch, 0, vict, TO_CHAR);
		}
		else
			send_to_char("Такой игрок отсутствует!\r\n", ch);
	}
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

struct communication_type
{
	const char *muted_msg;
	const char *action;
	const char *no_channel;
	const char *color;
	const char *you_action;
	const char *hi_action;
	int min_lev;
	int move_cost;
	int noflag;
};

void do_gen_comm(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	DESCRIPTOR_DATA *i;
	char color_on[24];
	int ign_flag;
	/*
	 * com_msgs: Message if you can't perform the action because of mute
	 *           name of the action
	 *           message if you're not on the channel
	 *           a color string.
	 *           Вы ....
	 *           Он(а) ....
	 *           min access level.
	 *           mov cost.
	 */

	struct communication_type com_msgs[] =
	{
		{"Вы не можете орать.\r\n",	// holler
		 "орать",
		 "Вы вне видимости канала.",
		 KIYEL,
		 "заорали",
		 "заорал$g",
		 4,
		 25,
		 PRF_NOHOLLER},

		{"Вам запрещено кричать.\r\n",	// shout
		 "кричать",
		 "Вы вне видимости канала.\r\n",
		 KIYEL,
		 "закричали",
		 "закричал$g",
		 2,
		 10,
		 PRF_NOSHOUT},

		{"Вам недозволено болтать.\r\n",	// gossip
		 "болтать",
		 "Вы вне видимости канала.\r\n",
		 KYEL,
		 "заметили",
		 "заметил$g",
		 3,
		 15,
		 PRF_NOGOSS},

		{"Вам не к лицу торговаться.\r\n",	// auction
		 "торговать",
		 "Вы вне видимости канала.\r\n",
		 KIYEL,
		 "попробовали поторговаться",
		 "вступил$g в торг",
		 2,
		 0,
		 PRF_NOAUCT},
	};

	// to keep pets, etc from being ordered to shout
//  if (!ch->desc)
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
	{
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (PLR_FLAGGED(ch, PLR_MUTE) && subcmd != SCMD_AUCTION)
	{
		send_to_char(com_msgs[subcmd].muted_msg, ch);
		return;
	}
	//if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) || ROOM_FLAGGED(ch->in_room, ROOM_ARENARECV))
	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
	{
		send_to_char(SOUNDPROOF, ch);
		return;
	}

	if (GET_LEVEL(ch) < com_msgs[subcmd].min_lev && !GET_REMORT(ch))
	{
		sprintf(buf1,
				"Вам стоит достичь хотя бы %d уровня, чтобы вы могли %s.\r\n",
				com_msgs[subcmd].min_lev, com_msgs[subcmd].action);
		send_to_char(buf1, ch);
		return;
	}

	// make sure the char is on the channel
	if (PRF_FLAGGED(ch, com_msgs[subcmd].noflag))
	{
		send_to_char(com_msgs[subcmd].no_channel, ch);
		return;
	}

	// skip leading spaces
	skip_spaces(&argument);

	// make sure that there is something there to say!
	if (!*argument && subcmd != SCMD_AUCTION)
	{
		sprintf(buf1, "ЛЕГКО! Но, Ярило вас побери, ЧТО %s???\r\n", com_msgs[subcmd].action);
		send_to_char(buf1, ch);
		return;
	}

	// set up the color on code
	strcpy(color_on, com_msgs[subcmd].color);

	// Анти-БУКОВКИ :coded by Делан

#define MAX_UPPERS_CHAR_PRC 30
#define MAX_UPPERS_SEQ_CHAR 3

	if ((subcmd != SCMD_AUCTION) && (!IS_IMMORTAL(ch)) && (!IS_NPC(ch)))
	{
		const unsigned int bad_smb_procent = MAX_UPPERS_CHAR_PRC;
		int bad_simb_cnt = 0, bad_seq_cnt = 0;

		// фильтруем верхний регистр
		for (int k = 0; argument[k] != '\0'; k++)
		{
			if (a_isupper(argument[k]))
			{
				bad_simb_cnt++;
				bad_seq_cnt++;
			}
			else
				bad_seq_cnt = 0;

			if ((bad_seq_cnt > 1) &&
					(((bad_simb_cnt * 100 / strlen(argument)) > bad_smb_procent) ||
					 (bad_seq_cnt > MAX_UPPERS_SEQ_CHAR)))
				argument[k] = a_lcc(argument[k]);
		}
		// фильтруем одинаковые сообщения в эфире
		if (!str_cmp(ch->get_last_tell().c_str(), argument))
		{
			send_to_char("Но вы же недавно говорили тоже самое?!\r\n", ch);
			return;
		}
		ch->set_last_tell(argument);
	}

	// в этой проверке заодно списываются мувы за крики, поэтому она должна идти последней
	if (!check_moves(ch, com_msgs[subcmd].move_cost))
		return;

	char out_str[MAX_STRING_LENGTH];

	// first, set up strings to be given to the communicator
	if (subcmd == SCMD_AUCTION)
	{
		*buf = '\0';
		auction_drive(ch, argument);
		return;
	}
	else
	{
		/* Непонятный запрет
		if (ROOM_FLAGGED(ch->in_room, ROOM_ARENARECV))
		{
			send_to_char(SOUNDPROOF, ch);
			return;
		}
		*/
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
		{
			if (COLOR_LEV(ch) >= C_CMP)
			{
				snprintf(buf1, MAX_STRING_LENGTH, "%sВы %s : '%s'%s", color_on,
						com_msgs[subcmd].you_action, argument, KNRM);
			}
			else
			{
				snprintf(buf1, MAX_STRING_LENGTH, "Вы %s : '%s'",
						com_msgs[subcmd].you_action, argument);
			}
			act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);

			if (!IS_NPC(ch))
			{
				snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH, "\r\n");
				ch->remember_add(buf1, Remember::ALL);
			}
		}
		switch (subcmd) {
		case SCMD_SHOUT:
			ign_flag = IGNORE_SHOUT;
			break;
		case SCMD_GOSSIP:
			if (PLR_FLAGGED(ch, PLR_SPAMMER))
				return;
			ign_flag = IGNORE_GOSSIP;
			break;
		case SCMD_HOLLER:
			if (PLR_FLAGGED(ch, PLR_SPAMMER))
				return;
			ign_flag = IGNORE_HOLLER;
			break;
		default:
			ign_flag = 0;
		}
		snprintf(out_str, MAX_STRING_LENGTH, "$n %s : '%s'", com_msgs[subcmd].hi_action, argument);
		if (IS_FEMALE(ch)) {
			if (!IS_NPC(ch) && (subcmd == SCMD_GOSSIP)) {
				snprintf(buf1, MAX_STRING_LENGTH, "%s%s заметила :'%s'%s\r\n",color_on, GET_NAME(ch), argument,KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!IS_NPC(ch) && (subcmd == SCMD_HOLLER)) {
				snprintf(buf1, MAX_STRING_LENGTH, "%s%s заорала :'%s'%s\r\n",color_on, GET_NAME(ch), argument,KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		}
		else {
			if (!IS_NPC(ch) && (subcmd == SCMD_GOSSIP)) {
				snprintf(buf1, MAX_STRING_LENGTH, "%s%s заметил :'%s'%s\r\n",color_on, GET_NAME(ch), argument,KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!IS_NPC(ch) && (subcmd == SCMD_HOLLER)) {
				snprintf(buf1, MAX_STRING_LENGTH, "%s%s заорал :'%s'%s\r\n",color_on, GET_NAME(ch), argument,KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		}
	}
	// now send all the strings out
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
				!PRF_FLAGGED(i->character, com_msgs[subcmd].noflag) &&
				!PLR_FLAGGED(i->character, PLR_WRITING) &&
				!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) && GET_POS(i->character) > POS_SLEEPING)
		{
			if (ignores(i->character.get(), ch, ign_flag))
			{
				continue;
			}

			if (subcmd == SCMD_SHOUT
				&& ((world[ch->in_room]->zone != world[i->character->in_room]->zone)
					|| !AWAKE(i->character)))
			{
				continue;
			}

			if (COLOR_LEV(i->character) >= C_NRM)
			{
				send_to_char(color_on, i->character.get());
			}

			act(out_str, FALSE, ch, 0, i->character.get(), TO_VICT | TO_SLEEP | CHECK_DEAF);
			if (COLOR_LEV(i->character) >= C_NRM)
			{
				send_to_char(KNRM, i->character.get());
			}

			const std::string text = Remember::format_gossip(ch, i->character.get(), subcmd, argument);

			i->character->remember_add(text, Remember::ALL);
		}
	}
}

void do_mobshout(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *i;

	// to keep pets, etc from being ordered to shout
	if (!(IS_NPC(ch) || WAITLESS(ch)))
		return;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	skip_spaces(&argument); //убираем пробел в начале сообщения
	sprintf(buf, "$n заорал$g : '%s'", argument);

	// now send all the strings out
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING
			&& i->character
			&& !PLR_FLAGGED(i->character, PLR_WRITING)
			&& GET_POS(i->character) > POS_SLEEPING)
		{
			if (COLOR_LEV(i->character) >= C_NRM)
			{
				send_to_char(KIYEL, i->character.get());
			}

			act(buf, FALSE, ch, 0, i->character.get(), TO_VICT | TO_SLEEP | CHECK_DEAF);

			if (COLOR_LEV(i->character) >= C_NRM)
			{
				send_to_char(KNRM, i->character.get());
			}
		}
	}
}

void do_pray_gods(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *i;
	CHAR_DATA *victim = NULL;

	skip_spaces(&argument);

	if (!IS_NPC(ch) && (PLR_FLAGGED(ch, PLR_DUMB) || PLR_FLAGGED(ch, PLR_MUTE)))
	{
		send_to_char("Вам запрещено обращаться к Богам, вероятно, вы их замучили...\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch))
	{
		// Выделяем чара кому отвечают иммы
		argument = one_argument(argument, arg1);
		skip_spaces(&argument);
		if (!*arg1)
		{
			send_to_char("Какому смертному вы собираетесь ответить?\r\n", ch);
			return;
		}
		victim = get_player_vis(ch, arg1, FIND_CHAR_WORLD);
		if (victim == NULL)
		{
			send_to_char("Такого нет в игре!\r\n", ch);
			return;
		}
	}

	if (!*argument)
	{
		sprintf(buf, "С чем вы хотите обратиться к Богам?\r\n");
		send_to_char(buf, ch);
		return;
	}
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else
	{
		if (IS_NPC(ch))
			return;
		if (IS_IMMORTAL(ch))
		{
			sprintf(buf, "&RВы одарили СЛОВОМ %s : '%s'&n\r\n", GET_PAD(victim, 3), argument);
		}
		else
		{
			sprintf(buf, "&RВы воззвали к Богам с сообщением : '%s'&n\r\n", argument);
			set_wait(ch, 3, FALSE);
		}
		send_to_char(buf, ch);
		ch->remember_add(buf, Remember::PRAY_PERSONAL);
	}

	if (IS_IMMORTAL(ch))
	{
		sprintf(buf, "&R%s ответил%s вам : '%s'&n\r\n", GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		send_to_char(buf, victim);
		victim->remember_add(buf, Remember::PRAY_PERSONAL);

		snprintf(buf1, MAX_STRING_LENGTH, "&R%s ответил%s %s : '%s&n\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch), GET_PAD(victim, 2), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, MAX_STRING_LENGTH, "&R%s ответил%s на воззвание %s : '%s'&n\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch), GET_PAD(victim, 1), argument);
	}
	else
	{
		snprintf(buf1, MAX_STRING_LENGTH, "&R%s воззвал%s к богам : '%s&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, MAX_STRING_LENGTH, "&R%s воззвал%s к богам с сообщением : '%s'&n\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch), argument);
	}

	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING)
		{
			if ((IS_IMMORTAL(i->character.get())
					|| (GET_GOD_FLAG(i->character.get(), GF_DEMIGOD)
						&& (GET_LEVEL(ch) < 6)))
				&& (i->character.get() != ch))
			{
				send_to_char(buf, i->character.get());
				i->character->remember_add(buf, Remember::ALL);
			}
		}
	}
}

/**
* Канал оффтоп. Не виден иммам, всегда видно кто говорит, вкл/выкл режим оффтоп.
*/
void do_offtop(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_IGVA_PRONA))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (PLR_FLAGGED(ch, PLR_DUMB) || PLR_FLAGGED(ch, PLR_MUTE))
	{
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}
	//if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) || ROOM_FLAGGED(ch->in_room, ROOM_ARENARECV))
	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
	{
		send_to_char(SOUNDPROOF, ch);
		return;
	}
	if (GET_LEVEL(ch) < SpamSystem::MIN_OFFTOP_LVL && !GET_REMORT(ch))
	{
		send_to_char(ch, "Вам стоит достичь хотя бы %d уровня, чтобы вы могли оффтопить.\r\n",
				SpamSystem::MIN_OFFTOP_LVL);
		return;
	}
	if (!PRF_FLAGGED(ch, PRF_OFFTOP_MODE))
	{
		send_to_char("Вы вне видимости канала.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	if (!*argument)
	{
		send_to_char(ch, "Раз нечего сказать, то лучше уж помолчать.");
		return;
	}
	lower_convert(argument);
	if (!strcmp(ch->get_last_tell().c_str(), argument))
	{
		send_to_char("Но вы же недавно говорили тоже самое!?!\r\n", ch);
		return;
	}
	// эта проверка должна идти последней и послее нее мессага полюбому идет в эфир
	if (!SpamSystem::check(ch, SpamSystem::OFFTOP_MODE))
	{
		return;
	}
	ch->set_last_tell(argument);
	if (PLR_FLAGGED(ch, PLR_SPAMMER)) // а вот фиг, еще проверка :)
		return;
	snprintf(buf, MAX_STRING_LENGTH, "[оффтоп] %s : '%s'\r\n", GET_NAME(ch), argument);
	snprintf(buf1, MAX_STRING_LENGTH, "&c%s&n", buf);
	for (DESCRIPTOR_DATA *i = descriptor_list; i; i = i->next)
	{
		// переплут как любитель почитывать логи за ночь очень хотел этот канал...
		// а мы шо, не люди? даешь оффтоп 34-ым! кому не нравится - реж оффтоп...
		if (STATE(i) == CON_PLAYING
			&& i->character
			&& (GET_LEVEL(i->character) < LVL_IMMORT || IS_IMPL(i->character))
			&& PRF_FLAGGED(i->character, PRF_OFFTOP_MODE)
			&& !PRF_FLAGGED(i->character, PRF_IGVA_PRONA)
			&& !ignores(i->character.get(), ch, IGNORE_OFFTOP))
		{
			send_to_char(i->character.get(), "%s%s%s", CCCYN(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
			i->character->remember_add(buf1, Remember::ALL);
		}
	}
	ch->remember_add(buf1, Remember::OFFTOP);
}

// shapirus
void ignore_usage(CHAR_DATA * ch)
{
	send_to_char("Формат команды: игнорировать <имя|все> <режим|все> <добавить|убрать>\r\n"
				 "Доступные режимы:\r\n"
				 "  сказать говорить шептать спросить эмоция кричать\r\n"
				 "  болтать орать группа дружина союзники\r\n", ch);
}

int ign_find_id(char *name, long *id)
{
	for (std::size_t i = 0; i < player_table.size(); i++)
	{
		if (!str_cmp(name, player_table[i].name()))
		{
			if (player_table[i].level >= LVL_IMMORT)
			{
				return 0;
			}

			*id = player_table[i].id();
			return 1;
		}
	}
	return -1;
}

const char * ign_find_name(long id)
{
	for (std::size_t i = 0; i < player_table.size(); i++)
	{
		if (id == player_table[i].id())
		{
			return player_table[i].name();
		}
	}

	return "кто-то";
}

char *text_ignore_modes(unsigned long mode, char *buf)
{
	buf[0] = 0;

	if (IS_SET(mode, IGNORE_TELL))
		strcat(buf, " сказать");
	if (IS_SET(mode, IGNORE_SAY))
		strcat(buf, " говорить");
	if (IS_SET(mode, IGNORE_WHISPER))
		strcat(buf, " шептать");
	if (IS_SET(mode, IGNORE_ASK))
		strcat(buf, " спросить");
	if (IS_SET(mode, IGNORE_EMOTE))
		strcat(buf, " эмоция");
	if (IS_SET(mode, IGNORE_SHOUT))
		strcat(buf, " кричать");
	if (IS_SET(mode, IGNORE_GOSSIP))
		strcat(buf, " болтать");
	if (IS_SET(mode, IGNORE_HOLLER))
		strcat(buf, " орать");
	if (IS_SET(mode, IGNORE_GROUP))
		strcat(buf, " группа");
	if (IS_SET(mode, IGNORE_CLAN))
		strcat(buf, " дружина");
	if (IS_SET(mode, IGNORE_ALLIANCE))
		strcat(buf, " союзники");
	if (IS_SET(mode, IGNORE_OFFTOP))
		strcat(buf, " оффтопик");
	return buf;
}

void do_ignore(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	unsigned int mode = 0, list_empty = 1, all = 0, flag = 0;
	long vict_id;
	char buf[256], buf1[256], name[50];

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

// криво попросили -- выведем справку
	if (arg1[0] && (!arg2[0] || !arg3[0]))
	{
		ignore_usage(ch);
		return;
	}
// при вызове без параметров выведем весь список
	if (!arg1[0] && !arg2[0] && !arg3[0])
	{
		sprintf(buf, "%sВы игнорируете следующих персонажей:%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		for (const auto ignore : ch->get_ignores())
		{
			if (!ignore->id)
				continue;
			if (ignore->id == -1)
			{
				strcpy(name, "Все");
			}
			else
			{
				strcpy(name, ign_find_name(ignore->id));
				name[0] = UPPER(name[0]);
			}
			sprintf(buf, "  %s: ", name);
			send_to_char(buf, ch);
			mode = ignore->mode;
			send_to_char(text_ignore_modes(mode, buf), ch);
			send_to_char("\r\n", ch);
			list_empty = 0;
		}

		if (list_empty)
		{
			send_to_char("  список пуст.\r\n", ch);
		}
		return;
	}

	if (is_abbrev(arg2, "все"))
		all = 1;
	else if (is_abbrev(arg2, "сказать"))
		flag = IGNORE_TELL;
	else if (is_abbrev(arg2, "говорить"))
		flag = IGNORE_SAY;
	else if (is_abbrev(arg2, "шептать"))
		flag = IGNORE_WHISPER;
	else if (is_abbrev(arg2, "спросить"))
		flag = IGNORE_ASK;
	else if (is_abbrev(arg2, "эмоция"))
		flag = IGNORE_EMOTE;
	else if (is_abbrev(arg2, "кричать"))
		flag = IGNORE_SHOUT;
	else if (is_abbrev(arg2, "болтать"))
		flag = IGNORE_GOSSIP;
	else if (is_abbrev(arg2, "орать"))
		flag = IGNORE_HOLLER;
	else if (is_abbrev(arg2, "группа"))
		flag = IGNORE_GROUP;
	else if (is_abbrev(arg2, "дружина"))
		flag = IGNORE_CLAN;
	else if (is_abbrev(arg2, "союзники"))
		flag = IGNORE_ALLIANCE;
	else if (is_abbrev(arg2, "оффтоп"))
		flag = IGNORE_OFFTOP;
	else
	{
		ignore_usage(ch);
		return;
	}

// имени "все" соответствует id -1
	if (is_abbrev(arg1, "все"))
	{
		vict_id = -1;
	}
	else
	{
		// убедимся, что добавляемый чар на данный момент существует
		// и он не имм, а заодно получим его id
		switch (ign_find_id(arg1, &vict_id))
		{
		case 0:
			send_to_char("Нельзя игнорировать Богов, это плохо кончится.\r\n", ch);
			return;
		case -1:
			send_to_char("Нет такого персонажа, уточните имя.\r\n", ch);
			return;
		}
	}

// ищем жертву в списке
	ignore_data::shared_ptr ignore = nullptr;
	for (const auto ignore_i : ch->get_ignores())
	{
		if (ignore_i->id == vict_id)
		{
			ignore = ignore_i;
			break;
		}
	}

	if (is_abbrev(arg3, "добавить"))
	{
// создаем новый элемент списка в хвосте, если не нашли
		if (!ignore)
		{
			const auto cur = std::make_shared<ignore_data>();
			cur->id = vict_id;
			cur->mode = 0;
			ch->add_ignore(cur);
			ignore = cur;
		}
		mode = ignore->mode;
		if (all)
		{
			SET_BIT(mode, IGNORE_TELL);
			SET_BIT(mode, IGNORE_SAY);
			SET_BIT(mode, IGNORE_WHISPER);
			SET_BIT(mode, IGNORE_ASK);
			SET_BIT(mode, IGNORE_EMOTE);
			SET_BIT(mode, IGNORE_SHOUT);
			SET_BIT(mode, IGNORE_GOSSIP);
			SET_BIT(mode, IGNORE_HOLLER);
			SET_BIT(mode, IGNORE_GROUP);
			SET_BIT(mode, IGNORE_CLAN);
			SET_BIT(mode, IGNORE_ALLIANCE);
			SET_BIT(mode, IGNORE_OFFTOP);
		}
		else
		{
			SET_BIT(mode, flag);
		}
		ignore->mode = mode;
	}
	else if (is_abbrev(arg3, "убрать"))
	{
		if (!ignore || ignore->id != vict_id)
		{
			if (vict_id == -1)
			{
				send_to_char("Вы и так не игнорируете всех сразу.\r\n", ch);
			}
			else
			{
				strcpy(name, ign_find_name(vict_id));
				name[0] = UPPER(name[0]);
				sprintf(buf,
						"Вы и так не игнорируете "
						"персонажа %s%s%s.\r\n", CCWHT(ch, C_NRM), name, CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
			}
			return;
		}
		mode = ignore->mode;
		if (all)
		{
			mode = 0;
		}
		else
		{
			REMOVE_BIT(mode, flag);
		}
		ignore->mode = mode;
		if (!mode)
			ignore->id = 0;
	}
	else
	{
		ignore_usage(ch);
		return;
	}
	if (mode)
	{
		if (ignore->id == -1)
		{
			sprintf(buf, "Для всех сразу вы игнорируете:%s.\r\n", text_ignore_modes(ignore->mode, buf1));
			send_to_char(buf, ch);
		}
		else
		{
			strcpy(name, ign_find_name(ignore->id));
			name[0] = UPPER(name[0]);
			sprintf(buf, "Для персонажа %s%s%s вы игнорируете:%s.\r\n",
					CCWHT(ch, C_NRM), name, CCNRM(ch, C_NRM), text_ignore_modes(ignore->mode, buf1));
			send_to_char(buf, ch);
		}
	}
	else
	{
		if (vict_id == -1)
		{
			send_to_char("Вы больше не игнорируете всех сразу.\r\n", ch);
		}
		else
		{
			strcpy(name, ign_find_name(vict_id));
			name[0] = UPPER(name[0]);
			sprintf(buf, "Вы больше не игнорируете персонажа %s%s%s.\r\n",
					CCWHT(ch, C_NRM), name, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}
}
