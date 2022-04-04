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

#include "handler.h"
#include "color.h"
#include "game_economics/auction.h"
#include "entities/char_player.h"
#include "entities/world_characters.h"
#include "house.h"
#include "spam.h"
#include "utils/utils_char_obj.inl"

// extern variables
/*extern DescriptorData *descriptor_list;
extern TimeInfoData time_info;*/

// local functions
void perform_tell(CharData *ch, CharData *vict, char *arg);
int is_tell_ok(CharData *ch, CharData *vict);
bool tell_can_see(CharData *ch, CharData *vict);

// external functions
extern char *diag_timer_to_char(const ObjData *obj);
extern void SetWait(CharData *ch, int waittime, int victim_in_room);

void do_say(CharData *ch, char *argument, int cmd, int subcmd);
void do_gsay(CharData *ch, char *argument, int cmd, int subcmd);
void do_tell(CharData *ch, char *argument, int cmd, int subcmd);
void do_reply(CharData *ch, char *argument, int cmd, int subcmd);
void do_spec_comm(CharData *ch, char *argument, int cmd, int subcmd);
void do_write(CharData *ch, char *argument, int cmd, int subcmd);
void do_page(CharData *ch, char *argument, int cmd, int subcmd);
void do_gen_comm(CharData *ch, char *argument, int cmd, int subcmd);
void do_pray_gods(CharData *ch, char *argument, int cmd, int subcmd);
void do_ignore(CharData *ch, char *argument, int cmd, int subcmd);

#define SIELENCE ("Вы немы, как рыба об лед.\r\n")
#define SOUNDPROOF ("Стены заглушили ваши слова.\r\n")

void do_say(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (!*argument)
		send_to_char("Вы задумались: \"Чего бы такого сказать?\"\r\n", ch);
	else {
		sprintf(buf, "$n сказал$g : '%s'", argument);

// для возможности игнорирования теллов в клетку
// пришлось изменить act в клетку на проход по клетке
		for (const auto to : world[ch->in_room]->people) {
			if (ch == to || ignores(to, ch, EIgnore::kSay)) {
				continue;
			}
			act(buf, false, ch, 0, to, kToVict | DG_NO_TRIG | kToNotDeaf);
		}

		if (!ch->is_npc() && PRF_FLAGGED(ch, EPrf::kNoRepeat)) {
			send_to_char(OK, ch);
		} else {
			delete_doubledollar(argument);
			sprintf(buf, "Вы сказали : '%s'\r\n", argument);
			send_to_char(buf, ch);
		}
		speech_mtrigger(ch, argument);
		speech_wtrigger(ch, argument);
	}
}

void do_gsay(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *k;
	struct Follower *f;

	if (AFF_FLAGGED(ch, EAffect::kSilence)
		|| AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!AFF_FLAGGED(ch, EAffect::kGroup)) {
		send_to_char("Вы не являетесь членом группы!\r\n", ch);
		return;
	}

	if (!*argument) {
		send_to_char("О чем вы хотите сообщить своей группе?\r\n", ch);
	} else {
		if (ch->has_master()) {
			k = ch->get_master();
		} else {
			k = ch;
		}

		sprintf(buf, "$n сообщил$g группе : '%s'", argument);

		if (AFF_FLAGGED(k, EAffect::kGroup)
			&& k != ch
			&& !ignores(k, ch, EIgnore::kGroup)) {
			act(buf, false, ch, 0, k, kToVict | kToSleep | kToNotDeaf);
			// added by WorM  групптелы 2010.10.13
			if (!AFF_FLAGGED(k, EAffect::kDeafness)
				&& GET_POS(k) > EPosition::kDead) {
				sprintf(buf1,
						"%s сообщил%s группе : '%s'\r\n",
						tell_can_see(ch, k) ? GET_NAME(ch) : "Кто-то",
						GET_CH_VIS_SUF_1(ch, k),
						argument);
				k->remember_add(buf1, Remember::ALL);
				k->remember_add(buf1, Remember::GROUP);
			}
			//end by WorM
		}
		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->ch, EAffect::kGroup)
				&& (f->ch != ch)
				&& !ignores(f->ch, ch, EIgnore::kGroup)) {
				act(buf, false, ch, 0, f->ch, kToVict | kToSleep | kToNotDeaf);
				// added by WorM  групптелы 2010.10.13
				if (!AFF_FLAGGED(f->ch, EAffect::kDeafness)
					&& GET_POS(f->ch) > EPosition::kDead) {
					sprintf(buf1,
							"%s сообщил%s группе : '%s'\r\n",
							tell_can_see(ch, f->ch) ? GET_NAME(ch) : "Кто-то",
							GET_CH_VIS_SUF_1(ch, f->ch),
							argument);
					f->ch->remember_add(buf1, Remember::ALL);
					f->ch->remember_add(buf1, Remember::GROUP);
				}
				//end by WorM
			}
		}

		if (PRF_FLAGGED(ch, EPrf::kNoRepeat))
			send_to_char(OK, ch);
		else {
			sprintf(buf, "Вы сообщили группе : '%s'\r\n", argument);
			send_to_char(buf, ch);
			// added by WorM  групптелы 2010.10.13
			ch->remember_add(buf, Remember::ALL);
			ch->remember_add(buf, Remember::GROUP);
			//end by WorM
		}
	}
}

bool tell_can_see(CharData *ch, CharData *vict) {
	if (CAN_SEE_CHAR(vict, ch) || IS_IMMORTAL(ch) || GET_INVIS_LEV(ch)) {
		return true;
	} else {
		return false;
	}
}

void perform_tell(CharData *ch, CharData *vict, char *arg) {
// shapirus: не позволим телять, если жертва не видит и включила
// соответствующий режим; имморталы могут телять всегда
	if (PRF_FLAGGED(vict, EPrf::kNoInvistell)
		&& !CAN_SEE(vict, ch)
		&& GetRealLevel(ch) < kLvlImmortal
		&& !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		act("$N не любит разговаривать с теми, кого не видит.", false, ch, 0, vict, kToChar | kToSleep);
		return;
	}

	// TODO: если в act() останется показ иммов, то это и эхо ниже переделать на act()
	if (tell_can_see(ch, vict)) {
		snprintf(buf, kMaxStringLength, "%s сказал%s вам : '%s'", GET_NAME(ch), GET_CH_SUF_1(ch), arg);
	} else {
		snprintf(buf, kMaxStringLength, "Кто-то сказал вам : '%s'", arg);
	}
	snprintf(buf1, kMaxStringLength, "%s%s%s\r\n", CCICYN(vict, C_NRM), CAP(buf), CCNRM(vict, C_NRM));
	send_to_char(buf1, vict);
	if (!vict->is_npc()) {
		vict->remember_add(buf1, Remember::ALL);
	}

	if (!vict->is_npc() && !ch->is_npc()) {
		snprintf(buf, kMaxStringLength, "%s%s : '%s'%s\r\n", CCICYN(vict, C_NRM),
				 tell_can_see(ch, vict) ? GET_NAME(ch) : "Кто-то", arg, CCNRM(vict, C_NRM));
		vict->remember_add(buf, Remember::PERSONAL);
	}

	if (!ch->is_npc() && PRF_FLAGGED(ch, EPrf::kNoRepeat)) {
		send_to_char(OK, ch);
	} else {
		snprintf(buf, kMaxStringLength, "%sВы сказали %s : '%s'%s\r\n", CCICYN(ch, C_NRM),
				 tell_can_see(vict, ch) ? vict->player_data.PNames[2].c_str() : "кому-то", arg, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		if (!ch->is_npc()) {
			ch->remember_add(buf, Remember::ALL);
		}
	}

	if (!vict->is_npc() && !ch->is_npc()) {
		vict->set_answer_id(GET_IDNUM(ch));
	}
}

int is_tell_ok(CharData *ch, CharData *vict) {
	if (ch == vict) {
		send_to_char("Вы начали потихоньку разговаривать с самим собой.\r\n", ch);
		return (false);
	} else if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Вам запрещено обращаться к другим игрокам.\r\n", ch);
		return (false);
	} else if (!vict->is_npc() && !vict->desc)    // linkless
	{
		act("$N потерял$G связь в этот момент.", false, ch, 0, vict, kToChar | kToSleep);
		return (false);
	} else if (PLR_FLAGGED(vict, EPlrFlag::kWriting)) {
		act("$N пишет сообщение - повторите попозже.", false, ch, 0, vict, kToChar | kToSleep);
		return (false);
	}

	if (IS_GOD(ch) || PRF_FLAGGED(ch, EPrf::kCoderinfo))
		return (true);

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof))
		send_to_char(SOUNDPROOF, ch);
	else if ((!vict->is_npc() &&
		(PRF_FLAGGED(vict, EPrf::kNoTell) || ignores(vict, ch, EIgnore::kTell))) ||
		ROOM_FLAGGED(vict->in_room, ERoomFlag::kSoundproof))
		act("$N не сможет вас услышать.", false, ch, 0, vict, kToChar | kToSleep);
	else if (GET_POS(vict) < EPosition::kRest || AFF_FLAGGED(vict, EAffect::kDeafness))
		act("$N вас не услышит.", false, ch, 0, vict, kToChar | kToSleep);
	else
		return (true);

	return (false);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
void do_tell(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	/* Непонятно нафига нужно
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
	{
		send_to_char(SOUNDPROOF, ch);
		return;
	}
	*/

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2) {
		send_to_char("Что и кому вы хотите сказать?\r\n", ch);
	} else if (!(vict = get_player_vis(ch, buf, EFind::kCharInWorld))) {
		send_to_char(NOPERSON, ch);
	} else if (vict->is_npc())
		send_to_char(NOPERSON, ch);
	else if (is_tell_ok(ch, vict)) {
		if (PRF_FLAGGED(ch, EPrf::kNoTell))
			send_to_char("Ответить вам не смогут!\r\n", ch);
		perform_tell(ch, vict, buf2);
	}
}

void do_reply(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc())
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (ch->get_answer_id() == kNobody)
		send_to_char("Вам некому ответить!\r\n", ch);
	else if (!*argument)
		send_to_char("Что вы собираетесь ответить?\r\n", ch);
	else {
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
		for (const auto &i : character_list) {
			if (!i->is_npc()
				&& GET_IDNUM(i) == ch->get_answer_id()) {
				if (is_tell_ok(ch, i.get())) {
					perform_tell(ch, i.get(), argument);
				}

				found = true;

				break;
			}
		}

		if (!found) {
			send_to_char("Этого игрока уже нет в игре.\r\n", ch);
		}
	}
}

void do_spec_comm(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	const char *action_sing, *action_plur, *action_others, *vict1, *vict2;
	char vict3[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_WHISPER) {
		action_sing = "шепнуть";
		vict1 = "кому";
		vict2 = "вам";
		action_plur = "прошептал";
		action_others = "$n что-то прошептал$g $N2.";
	} else {
		action_sing = "спросить";
		vict1 = "у кого";
		vict2 = "у вас";
		action_plur = "спросил";
		action_others = "$n задал$g $N2 вопрос.";
	}

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2) {
		sprintf(buf, "Что вы хотите %s.. и %s?\r\n", action_sing, vict1);
		send_to_char(buf, ch);
	} else if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom)))
		send_to_char(NOPERSON, ch);
	else if (vict == ch)
		send_to_char("От ваших уст до ушей - всего одна ладонь...\r\n", ch);
	else if (ignores(vict, ch, subcmd == SCMD_WHISPER ? EIgnore::kWhisper : EIgnore::kAsk)) {
		sprintf(buf, "%s не желает вас слышать.\r\n", GET_NAME(vict));
		send_to_char(buf, ch);
	} else {
		if (subcmd == SCMD_WHISPER)
			sprintf(vict3, "%s", GET_PAD(vict, 2));
		else
			sprintf(vict3, "у %s", GET_PAD(vict, 1));

		std::stringstream buffer;
		buffer << "$n " << action_plur << "$g " << vict2 << " : " << buf2;
//		sprintf(buf, "$n %s$g %s : '%s'", action_plur, vict2, buf2);
		act(buffer.str().c_str(), false, ch, 0, vict, kToVict | kToNotDeaf);

		if (PRF_FLAGGED(ch, EPrf::kNoRepeat))
			send_to_char(OK, ch);
		else {
			std::stringstream buffer;
			buffer << "Вы " << action_plur << "и " << vict3 << " : '" << buf2 << "'" << std::endl;
//			sprintf(buf, "Вы %sи %s : '%s'\r\n", action_plur, vict3, buf2);
			send_to_char(buffer.str(), ch);
		}

		act(action_others, false, ch, 0, vict, kToNotVict);
	}
}

#define MAX_NOTE_LENGTH 4096    // arbitrary

void do_write(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *paper, *pen = nullptr;
	char *papername, *penname;

	papername = buf1;
	penname = buf2;

	two_arguments(argument, papername, penname);

	if (!ch->desc)
		return;

	if (!*papername)    // nothing was delivered
	{
		send_to_char("Написать?  Чем?  И на чем?\r\n", ch);
		return;
	}
	if (*penname)        // there were two arguments
	{
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			sprintf(buf, "У вас нет %s.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
			sprintf(buf, "У вас нет %s.\r\n", penname);
			send_to_char(buf, ch);
			return;
		}
	} else        // there was one arg.. let's see what we can find
	{
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			sprintf(buf, "Вы не видите %s в инвентаре.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(paper) == EObjType::kPen)    // oops, a pen..
		{
			pen = paper;
			paper = nullptr;
		} else if (GET_OBJ_TYPE(paper) != EObjType::kNote) {
			send_to_char("Вы не можете на ЭТОМ писать.\r\n", ch);
			return;
		}
		// One object was found.. now for the other one.
		if (!GET_EQ(ch, kHold)) {
			sprintf(buf, "Вы нечем писать!\r\n");
			send_to_char(buf, ch);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, kHold))) {
			send_to_char("Вы держите что-то невидимое!  Жаль, но писать этим трудно!!\r\n", ch);
			return;
		}
		if (pen)
			paper = GET_EQ(ch, kHold);
		else
			pen = GET_EQ(ch, kHold);
	}


	// ok.. now let's see what kind of stuff we've found
	if (GET_OBJ_TYPE(pen) != EObjType::kPen) {
		act("Вы не умеете писать $o4.", false, ch, pen, 0, kToChar);
	} else if (GET_OBJ_TYPE(paper) != EObjType::kNote) {
		act("Вы не можете писать на $o5.", false, ch, paper, 0, kToChar);
	} else if (!paper->get_action_description().empty()) {
		send_to_char("Там уже что-то записано.\r\n", ch);
	} else            // we can write - hooray!
	{
		/* this is the PERFECT code example of how to set up:
		 * a) the text editor with a message already loaed
		 * b) the abort buffer if the player aborts the message
		 */
		ch->desc->backstr = nullptr;
		send_to_char("Можете писать.  (/s СОХРАНИТЬ ЗАПИСЬ  /h ПОМОЩЬ)\r\n", ch);
		// ok, here we check for a message ALREADY on the paper
		if (!paper->get_action_description().empty())    // we str_dup the original text to the descriptors->backstr
		{
			ch->desc->backstr = str_dup(paper->get_action_description().c_str());
			// send to the player what was on the paper (cause this is already
			// loaded into the editor)
			send_to_char(paper->get_action_description().c_str(), ch);
		}
		act("$n начал$g писать.", true, ch, 0, 0, kToRoom);
		// assign the descriptor's->str the value of the pointer to the text
		// pointer so that we can reallocate as needed (hopefully that made
		// sense :>)
		const utils::AbstractStringWriter::shared_ptr writer(new CActionDescriptionWriter(*paper));
		string_write(ch->desc, writer, MAX_NOTE_LENGTH, 0, nullptr);
	}
}

void do_page(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	CharData *vict;

	half_chop(argument, arg, buf2);

	if (ch->is_npc())
		send_to_char("Создания-не-персонажи этого не могут.. ступайте.\r\n", ch);
	else if (!*arg)
		send_to_char("Whom do you wish to page?\r\n", ch);
	else {
		std::stringstream buffer;
		buffer << "\007\007*$n*" << buf2;
//		sprintf(buf, "\007\007*$n* %s", buf2);
		if (!str_cmp(arg, "all") || !str_cmp(arg, "все")) {
			if (IS_GRGOD(ch)) {
				for (d = descriptor_list; d; d = d->next) {
					if (STATE(d) == CON_PLAYING && d->character) {
						act(buf, false, ch, 0, d->character.get(), kToVict);
					}
				}
			} else {
				send_to_char("Это доступно только БОГАМ!\r\n", ch);
			}
			return;
		}
		if ((vict = get_char_vis(ch, arg, EFind::kCharInWorld)) != nullptr) {
			act(buffer.str().c_str(), false, ch, 0, vict, kToVict);
			if (PRF_FLAGGED(ch, EPrf::kNoRepeat))
				send_to_char(OK, ch);
			else
				act(buffer.str().c_str(), false, ch, 0, vict, kToChar);
		} else
			send_to_char("Такой игрок отсутствует!\r\n", ch);
	}
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

struct communication_type {
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

void do_gen_comm(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	DescriptorData *i;
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
			{"Вы не можете орать.\r\n",    // holler
			 "орать",
			 "Вы вне видимости канала.",
			 KIYEL,
			 "заорали",
			 "заорал$g",
			 4,
			 25,
			 EPrf::kNoHoller},

			{"Вам запрещено кричать.\r\n",    // shout
			 "кричать",
			 "Вы вне видимости канала.\r\n",
			 KIYEL,
			 "закричали",
			 "закричал$g",
			 2,
			 10,
			 EPrf::kNoShout},

			{"Вам недозволено болтать.\r\n",    // gossip
			 "болтать",
			 "Вы вне видимости канала.\r\n",
			 KYEL,
			 "заметили",
			 "заметил$g",
			 3,
			 15,
			 EPrf::kNoGossip},

			{"Вам не к лицу торговаться.\r\n",    // auction
			 "торговать",
			 "Вы вне видимости канала.\r\n",
			 KIYEL,
			 "попробовали поторговаться",
			 "вступил$g в торг",
			 2,
			 0,
			 EPrf::kNoAuction},
		};

	// to keep pets, etc from being ordered to shout
//  if (!ch->desc)
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		send_to_char(SIELENCE, ch);
		return;
	}

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (PLR_FLAGGED(ch, EPlrFlag::kMuted) && subcmd != SCMD_AUCTION) {
		send_to_char(com_msgs[subcmd].muted_msg, ch);
		return;
	}
	//if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof)) {
		send_to_char(SOUNDPROOF, ch);
		return;
	}

	if (GetRealLevel(ch) < com_msgs[subcmd].min_lev && !GET_REAL_REMORT(ch)) {
		sprintf(buf1,
				"Вам стоит достичь хотя бы %d уровня, чтобы вы могли %s.\r\n",
				com_msgs[subcmd].min_lev, com_msgs[subcmd].action);
		send_to_char(buf1, ch);
		return;
	}

	// make sure the char is on the channel
	if (PRF_FLAGS(ch).get(com_msgs[subcmd].noflag)) {
		send_to_char(com_msgs[subcmd].no_channel, ch);
		return;
	}

	// skip leading spaces
	skip_spaces(&argument);

	// make sure that there is something there to say!
	if (!*argument && subcmd != SCMD_AUCTION) {
		sprintf(buf1, "ЛЕГКО! Но, Ярило вас побери, ЧТО %s???\r\n", com_msgs[subcmd].action);
		send_to_char(buf1, ch);
		return;
	}

	// set up the color on code
	strcpy(color_on, com_msgs[subcmd].color);

	// Анти-БУКОВКИ :coded by Делан

#define MAX_UPPERS_CHAR_PRC 30
#define MAX_UPPERS_SEQ_CHAR 3

	if ((subcmd != SCMD_AUCTION) && (!IS_IMMORTAL(ch)) && (!ch->is_npc())) {
		const unsigned int bad_smb_procent = MAX_UPPERS_CHAR_PRC;
		int bad_simb_cnt = 0, bad_seq_cnt = 0;

		// фильтруем верхний регистр
		for (int k = 0; argument[k] != '\0'; k++) {
			if (a_isupper(argument[k])) {
				bad_simb_cnt++;
				bad_seq_cnt++;
			} else
				bad_seq_cnt = 0;

			if ((bad_seq_cnt > 1) &&
				(((bad_simb_cnt * 100 / strlen(argument)) > bad_smb_procent) ||
					(bad_seq_cnt > MAX_UPPERS_SEQ_CHAR)))
				argument[k] = a_lcc(argument[k]);
		}
		// фильтруем одинаковые сообщения в эфире
		if (!str_cmp(ch->get_last_tell().c_str(), argument)) {
			send_to_char("Но вы же недавно говорили тоже самое?!\r\n", ch);
			return;
		}
		ch->set_last_tell(argument);
	}

	// в этой проверке заодно списываются мувы за крики, поэтому она должна идти последней
	if (!check_moves(ch, com_msgs[subcmd].move_cost))
		return;

	char out_str[kMaxStringLength];

	// first, set up strings to be given to the communicator
	if (subcmd == SCMD_AUCTION) {
		*buf = '\0';
		auction_drive(ch, argument);
		return;
	} else {
		/* Непонятный запрет
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
		{
			send_to_char(SOUNDPROOF, ch);
			return;
		}
		*/
		if (PRF_FLAGS(ch).get(EPrf::kNoRepeat))
			send_to_char(OK, ch);
		else {
			if (COLOR_LEV(ch) >= C_CMP) {
				snprintf(buf1, kMaxStringLength, "%sВы %s : '%s'%s", color_on,
						 com_msgs[subcmd].you_action, argument, KNRM);
			} else {
				snprintf(buf1, kMaxStringLength, "Вы %s : '%s'",
						 com_msgs[subcmd].you_action, argument);
			}
			act(buf1, false, ch, 0, 0, kToChar | kToSleep);

			if (!ch->is_npc()) {
				snprintf(buf1 + strlen(buf1), kMaxStringLength, "\r\n");
				ch->remember_add(buf1, Remember::ALL);
			}
		}
		switch (subcmd) {
			case SCMD_SHOUT: ign_flag = EIgnore::kShout;
				break;
			case SCMD_GOSSIP:
				if (PLR_FLAGGED(ch, EPlrFlag::kSpamer))
					return;
				ign_flag = EIgnore::kGossip;
				break;
			case SCMD_HOLLER:
				if (PLR_FLAGGED(ch, EPlrFlag::kSpamer))
					return;
				ign_flag = EIgnore::kHoller;
				break;
			default: ign_flag = 0;
		}
		snprintf(out_str, kMaxStringLength, "$n %s : '%s'", com_msgs[subcmd].hi_action, argument);
		if (IS_FEMALE(ch)) {
			if (!ch->is_npc() && (subcmd == SCMD_GOSSIP)) {
				snprintf(buf1, kMaxStringLength, "%s%s заметила :'%s'%s\r\n", color_on, GET_NAME(ch), argument, KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!ch->is_npc() && (subcmd == SCMD_HOLLER)) {
				snprintf(buf1, kMaxStringLength, "%s%s заорала :'%s'%s\r\n", color_on, GET_NAME(ch), argument, KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		} else {
			if (!ch->is_npc() && (subcmd == SCMD_GOSSIP)) {
				snprintf(buf1, kMaxStringLength, "%s%s заметил :'%s'%s\r\n", color_on, GET_NAME(ch), argument, KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!ch->is_npc() && (subcmd == SCMD_HOLLER)) {
				snprintf(buf1, kMaxStringLength, "%s%s заорал :'%s'%s\r\n", color_on, GET_NAME(ch), argument, KNRM);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		}
	}
	// now send all the strings out
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
			!PRF_FLAGS(i->character).get(com_msgs[subcmd].noflag) &&
			!PLR_FLAGGED(i->character, EPlrFlag::kWriting) &&
			!ROOM_FLAGGED(i->character->in_room, ERoomFlag::kSoundproof) && GET_POS(i->character) > EPosition::kSleep) {
			if (ignores(i->character.get(), ch, ign_flag)) {
				continue;
			}

			if (subcmd == SCMD_SHOUT
				&& ((world[ch->in_room]->zone_rn != world[i->character->in_room]->zone_rn)
					|| !AWAKE(i->character))) {
				continue;
			}

			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char(color_on, i->character.get());
			}

			act(out_str, false, ch, 0, i->character.get(), kToVict | kToSleep | kToNotDeaf);
			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char(KNRM, i->character.get());
			}

			const std::string text = Remember::format_gossip(ch, i->character.get(), subcmd, argument);

			i->character->remember_add(text, Remember::ALL);
		}
	}
}

void do_mobshout(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *i;

	// to keep pets, etc from being ordered to shout
	if (!(ch->is_npc() || IS_IMMORTAL(ch)))
		return;
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	skip_spaces(&argument); //убираем пробел в начале сообщения
	sprintf(buf, "$n заорал$g : '%s'", argument);

	// now send all the strings out
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING
			&& i->character
			&& !PLR_FLAGGED(i->character, EPlrFlag::kWriting)
			&& GET_POS(i->character) > EPosition::kSleep) {
			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char(KIYEL, i->character.get());
			}

			act(buf, false, ch, 0, i->character.get(), kToVict | kToSleep | kToNotDeaf);

			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char(KNRM, i->character.get());
			}
		}
	}
}

void do_pray_gods(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	DescriptorData *i;
	CharData *victim = nullptr;

	skip_spaces(&argument);

	if (!ch->is_npc() && (PLR_FLAGGED(ch, EPlrFlag::kDumbed) || PLR_FLAGGED(ch, EPlrFlag::kMuted))) {
		send_to_char("Вам запрещено обращаться к Богам, вероятно, вы их замучили...\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch)) {
		// Выделяем чара кому отвечают иммы
		argument = one_argument(argument, arg1);
		skip_spaces(&argument);
		if (!*arg1) {
			send_to_char("Какому смертному вы собираетесь ответить?\r\n", ch);
			return;
		}
		victim = get_player_vis(ch, arg1, EFind::kCharInWorld);
		if (victim == nullptr) {
			send_to_char("Такого нет в игре!\r\n", ch);
			return;
		}
	}

	if (!*argument) {
		sprintf(buf, "С чем вы хотите обратиться к Богам?\r\n");
		send_to_char(buf, ch);
		return;
	}
	if (PRF_FLAGGED(ch, EPrf::kNoRepeat))
		send_to_char(OK, ch);
	else {
		if (ch->is_npc())
			return;
		if (IS_IMMORTAL(ch)) {
			sprintf(buf, "&RВы одарили СЛОВОМ %s : '%s'&n\r\n", GET_PAD(victim, 3), argument);
		} else {
			sprintf(buf, "&RВы воззвали к Богам с сообщением : '%s'&n\r\n", argument);
			SetWait(ch, 3, false);
		}
		send_to_char(buf, ch);
		ch->remember_add(buf, Remember::PRAY_PERSONAL);
	}

	if (IS_IMMORTAL(ch)) {
		sprintf(buf, "&R%s ответил%s вам : '%s'&n\r\n", GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		send_to_char(buf, victim);
		victim->remember_add(buf, Remember::PRAY_PERSONAL);

		snprintf(buf1, kMaxStringLength, "&R%s ответил%s %s : '%s&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), GET_PAD(victim, 2), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, kMaxStringLength, "&R%s ответил%s на воззвание %s : '%s'&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), GET_PAD(victim, 1), argument);
	} else {
		snprintf(buf1, kMaxStringLength, "&R%s воззвал%s к богам : '%s&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, kMaxStringLength, "&R%s воззвал%s к богам с сообщением : '%s'&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), argument);
	}

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING) {
			if ((IS_IMMORTAL(i->character.get())
				|| (GET_GOD_FLAG(i->character.get(), EGf::kDemigod)
					&& (GetRealLevel(ch) < 6)))
				&& (i->character.get() != ch)) {
				send_to_char(buf, i->character.get());
				i->character->remember_add(buf, Remember::ALL);
			}
		}
	}
}

/**
* Канал оффтоп. Не виден иммам, всегда видно кто говорит, вкл/выкл режим оффтоп.
*/
void do_offtop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc() || GetRealLevel(ch) >= kLvlImmortal || PRF_FLAGGED(ch, EPrf::kStopOfftop)) {
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (PLR_FLAGGED(ch, EPlrFlag::kDumbed) || PLR_FLAGGED(ch, EPlrFlag::kMuted)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}
	//if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof)) {
		send_to_char(SOUNDPROOF, ch);
		return;
	}
	if (GetRealLevel(ch) < antispam::kMinOfftopLvl && !GET_REAL_REMORT(ch)) {
		send_to_char(ch, "Вам стоит достичь хотя бы %d уровня, чтобы вы могли оффтопить.\r\n",
					 antispam::kMinOfftopLvl);
		return;
	}
	if (!PRF_FLAGGED(ch, EPrf::kOfftopMode)) {
		send_to_char("Вы вне видимости канала.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	if (!*argument) {
		send_to_char(ch, "Раз нечего сказать, то лучше уж помолчать.");
		return;
	}
	utils::ConvertToLow(argument);
	if (!strcmp(ch->get_last_tell().c_str(), argument)) {
		send_to_char("Но вы же недавно говорили тоже самое!?!\r\n", ch);
		return;
	}
	// эта проверка должна идти последней и послее нее мессага полюбому идет в эфир
	if (!antispam::check(ch, antispam::kOfftopMode)) {
		return;
	}
	ch->set_last_tell(argument);
	if (PLR_FLAGGED(ch, EPlrFlag::kSpamer)) // а вот фиг, еще проверка :)
		return;
	snprintf(buf, kMaxStringLength, "[оффтоп] %s : '%s'\r\n", GET_NAME(ch), argument);
	snprintf(buf1, kMaxStringLength, "&c%s&n", buf);
	for (DescriptorData *i = descriptor_list; i; i = i->next) {
		// переплут как любитель почитывать логи за ночь очень хотел этот канал...
		// а мы шо, не люди? даешь оффтоп 34-ым! кому не нравится - реж оффтоп...
		if (STATE(i) == CON_PLAYING
			&& i->character
			&& (GetRealLevel(i->character) < kLvlImmortal || IS_IMPL(i->character))
			&& PRF_FLAGGED(i->character, EPrf::kOfftopMode)
			&& !PRF_FLAGGED(i->character, EPrf::kStopOfftop)
			&& !ignores(i->character.get(), ch, EIgnore::kOfftop)) {
			send_to_char(i->character.get(), "%s%s%s", KCYN, buf, KNRM);
			i->character->remember_add(buf1, Remember::ALL);
		}
	}
	ch->remember_add(buf1, Remember::OFFTOP);
}

// shapirus
void ignore_usage(CharData *ch) {
	send_to_char("Формат команды: игнорировать <имя|все> <режим|все> <добавить|убрать>\r\n"
				 "Доступные режимы:\r\n"
				 "  сказать говорить шептать спросить эмоция кричать\r\n"
				 "  болтать орать группа дружина союзники\r\n", ch);
}

int ign_find_id(char *name, long *id) {
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (!str_cmp(name, player_table[i].name())) {
			if (player_table[i].level >= kLvlImmortal) {
				return 0;
			}

			*id = player_table[i].id();
			return 1;
		}
	}
	return -1;
}

const char *ign_find_name(long id) {
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (id == player_table[i].id()) {
			return player_table[i].name();
		}
	}

	return "кто-то";
}

char *text_ignore_modes(unsigned long mode, char *buf) {
	buf[0] = 0;

	if (IS_SET(mode, EIgnore::kTell))
		strcat(buf, " сказать");
	if (IS_SET(mode, EIgnore::kSay))
		strcat(buf, " говорить");
	if (IS_SET(mode, EIgnore::kWhisper))
		strcat(buf, " шептать");
	if (IS_SET(mode, EIgnore::kAsk))
		strcat(buf, " спросить");
	if (IS_SET(mode, EIgnore::kEmote))
		strcat(buf, " эмоция");
	if (IS_SET(mode, EIgnore::kShout))
		strcat(buf, " кричать");
	if (IS_SET(mode, EIgnore::kGossip))
		strcat(buf, " болтать");
	if (IS_SET(mode, EIgnore::kHoller))
		strcat(buf, " орать");
	if (IS_SET(mode, EIgnore::kGroup))
		strcat(buf, " группа");
	if (IS_SET(mode, EIgnore::kClan))
		strcat(buf, " дружина");
	if (IS_SET(mode, EIgnore::kAlliance))
		strcat(buf, " союзники");
	if (IS_SET(mode, EIgnore::kOfftop))
		strcat(buf, " оффтопик");
	return buf;
}

void do_ignore(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	unsigned int mode = 0, list_empty = 1, all = 0, flag = 0;
	long vict_id;
	char buf[256], buf1[256], name[50];

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

// криво попросили -- выведем справку
	if (arg1[0] && (!arg2[0] || !arg3[0])) {
		ignore_usage(ch);
		return;
	}
// при вызове без параметров выведем весь список
	if (!arg1[0] && !arg2[0] && !arg3[0]) {
		sprintf(buf, "%sВы игнорируете следующих персонажей:%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		for (const auto &ignore : ch->get_ignores()) {
			if (!ignore->id)
				continue;
			if (ignore->id == -1) {
				strcpy(name, "Все");
			} else {
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

		if (list_empty) {
			send_to_char("  список пуст.\r\n", ch);
		}
		return;
	}

	if (utils::IsAbbrev(arg2, "все"))
		all = 1;
	else if (utils::IsAbbrev(arg2, "сказать"))
		flag = EIgnore::kTell;
	else if (utils::IsAbbrev(arg2, "говорить"))
		flag = EIgnore::kSay;
	else if (utils::IsAbbrev(arg2, "шептать"))
		flag = EIgnore::kWhisper;
	else if (utils::IsAbbrev(arg2, "спросить"))
		flag = EIgnore::kAsk;
	else if (utils::IsAbbrev(arg2, "эмоция"))
		flag = EIgnore::kEmote;
	else if (utils::IsAbbrev(arg2, "кричать"))
		flag = EIgnore::kShout;
	else if (utils::IsAbbrev(arg2, "болтать"))
		flag = EIgnore::kGossip;
	else if (utils::IsAbbrev(arg2, "орать"))
		flag = EIgnore::kHoller;
	else if (utils::IsAbbrev(arg2, "группа"))
		flag = EIgnore::kGroup;
	else if (utils::IsAbbrev(arg2, "дружина"))
		flag = EIgnore::kClan;
	else if (utils::IsAbbrev(arg2, "союзники"))
		flag = EIgnore::kAlliance;
	else if (utils::IsAbbrev(arg2, "оффтоп"))
		flag = EIgnore::kOfftop;
	else {
		ignore_usage(ch);
		return;
	}

// имени "все" соответствует id -1
	if (utils::IsAbbrev(arg1, "все")) {
		vict_id = -1;
	} else {
		// убедимся, что добавляемый чар на данный момент существует
		// и он не имм, а заодно получим его id
		switch (ign_find_id(arg1, &vict_id)) {
			case 0: send_to_char("Нельзя игнорировать Богов, это плохо кончится.\r\n", ch);
				return;
			case -1: send_to_char("Нет такого персонажа, уточните имя.\r\n", ch);
				return;
		}
	}

// ищем жертву в списке
	ignore_data::shared_ptr ignore = nullptr;
	for (const auto &ignore_i : ch->get_ignores()) {
		if (ignore_i->id == vict_id) {
			ignore = ignore_i;
			break;
		}
	}

	if (utils::IsAbbrev(arg3, "добавить")) {
// создаем новый элемент списка в хвосте, если не нашли
		if (!ignore) {
			const auto cur = std::make_shared<ignore_data>();
			cur->id = vict_id;
			cur->mode = 0;
			ch->add_ignore(cur);
			ignore = cur;
		}
		mode = ignore->mode;
		if (all) {
			SET_BIT(mode, EIgnore::kTell);
			SET_BIT(mode, EIgnore::kSay);
			SET_BIT(mode, EIgnore::kWhisper);
			SET_BIT(mode, EIgnore::kAsk);
			SET_BIT(mode, EIgnore::kEmote);
			SET_BIT(mode, EIgnore::kShout);
			SET_BIT(mode, EIgnore::kGossip);
			SET_BIT(mode, EIgnore::kHoller);
			SET_BIT(mode, EIgnore::kGroup);
			SET_BIT(mode, EIgnore::kClan);
			SET_BIT(mode, EIgnore::kAlliance);
			SET_BIT(mode, EIgnore::kOfftop);
		} else {
			SET_BIT(mode, flag);
		}
		ignore->mode = mode;
	} else if (utils::IsAbbrev(arg3, "убрать")) {
		if (!ignore || ignore->id != vict_id) {
			if (vict_id == -1) {
				send_to_char("Вы и так не игнорируете всех сразу.\r\n", ch);
			} else {
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
		if (all) {
			mode = 0;
		} else {
			REMOVE_BIT(mode, flag);
		}
		ignore->mode = mode;
		if (!mode)
			ignore->id = 0;
	} else {
		ignore_usage(ch);
		return;
	}
	if (mode) {
		if (ignore->id == -1) {
			sprintf(buf, "Для всех сразу вы игнорируете:%s.\r\n", text_ignore_modes(ignore->mode, buf1));
			send_to_char(buf, ch);
		} else {
			strcpy(name, ign_find_name(ignore->id));
			name[0] = UPPER(name[0]);
			sprintf(buf, "Для персонажа %s%s%s вы игнорируете:%s.\r\n",
					CCWHT(ch, C_NRM), name, CCNRM(ch, C_NRM), text_ignore_modes(ignore->mode, buf1));
			send_to_char(buf, ch);
		}
	} else {
		if (vict_id == -1) {
			send_to_char("Вы больше не игнорируете всех сразу.\r\n", ch);
		} else {
			strcpy(name, ign_find_name(vict_id));
			name[0] = UPPER(name[0]);
			sprintf(buf, "Вы больше не игнорируете персонажа %s%s%s.\r\n",
					CCWHT(ch, C_NRM), name, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}
}
