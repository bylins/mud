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

#include "engine/entities/char_player.h"
#include "engine/core/handler.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/db/world_characters.h"
#include "engine/ui/color.h"
#include "gameplay/clans/house.h"
#include "gameplay/communication/offtop.h"
#include "gameplay/communication/spam.h"
#include "gameplay/economics/auction.h"
#include "gameplay/communication/ignores.h"
#include "engine/ui/modify.h"

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


void do_spec_comm(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	const char *action_sing, *action_plur, *action_others, *vict1, *vict2;
	char vict3[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
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
		SendMsgToChar(buf, ch);
	} else if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom)))
		SendMsgToChar(NOPERSON, ch);
	else if (vict == ch)
		SendMsgToChar("От ваших уст до ушей - всего одна ладонь...\r\n", ch);
	else if (ignores(vict, ch, subcmd == SCMD_WHISPER ? EIgnore::kWhisper : EIgnore::kAsk)) {
		sprintf(buf, "%s не желает вас слышать.\r\n", GET_NAME(vict));
		SendMsgToChar(buf, ch);
	} else {
		if (subcmd == SCMD_WHISPER)
			sprintf(vict3, "%s", GET_PAD(vict, 2));
		else
			sprintf(vict3, "у %s", GET_PAD(vict, 1));

		std::stringstream buffer;
		buffer << "$n " << action_plur << "$g " << vict2 << " : " << buf2;
//		sprintf(buf, "$n %s$g %s : '%s'", action_plur, vict2, buf2);
		act(buffer.str().c_str(), false, ch, nullptr, vict, kToVict | kToNotDeaf);

		if (ch->IsFlagged(EPrf::kNoRepeat))
			SendMsgToChar(OK, ch);
		else {
			std::stringstream buffer;
			buffer << "Вы " << action_plur << "и " << vict3 << " : '" << buf2 << "'" << "\r\n";
//			sprintf(buf, "Вы %sи %s : '%s'\r\n", action_plur, vict3, buf2);
			SendMsgToChar(buffer.str(), ch);
		}

		act(action_others, false, ch, nullptr, vict, kToNotVict);
	}
}

void do_page(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	CharData *vict;

	half_chop(argument, arg, buf2);

	if (ch->IsNpc())
		SendMsgToChar("Создания-не-персонажи этого не могут.. ступайте.\r\n", ch);
	else if (!*arg)
		SendMsgToChar("Whom do you wish to page?\r\n", ch);
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
				SendMsgToChar("Это доступно только БОГАМ!\r\n", ch);
			}
			return;
		}
		if ((vict = get_char_vis(ch, arg, EFind::kCharInWorld)) != nullptr) {
			act(buffer.str().c_str(), false, ch, 0, vict, kToVict);
			if (ch->IsFlagged(EPrf::kNoRepeat))
				SendMsgToChar(OK, ch);
			else
				act(buffer.str().c_str(), false, ch, 0, vict, kToChar);
		} else
			SendMsgToChar("Такой игрок отсутствует!\r\n", ch);
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
  EPrf noflag;
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
			 kColorBoldYel,
			 "заорали",
			 "заорал$g",
			 4,
			 25,
			 EPrf::kNoHoller},

			{"Вам запрещено кричать.\r\n",    // shout
			 "кричать",
			 "Вы вне видимости канала.\r\n",
			 kColorBoldYel,
			 "закричали",
			 "закричал$g",
			 2,
			 10,
			 EPrf::kNoShout},

			{"Вам недозволено болтать.\r\n",    // gossip
			 "болтать",
			 "Вы вне видимости канала.\r\n",
			 kColorYel,
			 "заметили",
			 "заметил$g",
			 3,
			 15,
			 EPrf::kNoGossip},

			{"Вам не к лицу торговаться.\r\n",    // auction
			 "торговать",
			 "Вы вне видимости канала.\r\n",
			 kColorBoldYel,
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

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kMuted) && subcmd != SCMD_AUCTION) {
		SendMsgToChar(com_msgs[subcmd].muted_msg, ch);
		return;
	}
	//if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof)) {
		SendMsgToChar(SOUNDPROOF, ch);
		return;
	}

	if (GetRealLevel(ch) < com_msgs[subcmd].min_lev && !GetRealRemort(ch)) {
		sprintf(buf1,
				"Вам стоит достичь хотя бы %d уровня, чтобы вы могли %s.\r\n",
				com_msgs[subcmd].min_lev, com_msgs[subcmd].action);
		SendMsgToChar(buf1, ch);
		return;
	}

	// make sure the char is on the channel
	if (ch->IsFlagged(com_msgs[subcmd].noflag)) {
		SendMsgToChar(com_msgs[subcmd].no_channel, ch);
		return;
	}

	// skip leading spaces
	skip_spaces(&argument);

	// make sure that there is something there to say!
	if (!*argument && subcmd != SCMD_AUCTION) {
		sprintf(buf1, "ЛЕГКО! Но, Ярило вас побери, ЧТО %s???\r\n", com_msgs[subcmd].action);
		SendMsgToChar(buf1, ch);
		return;
	}

	// set up the color on code
	strcpy(color_on, com_msgs[subcmd].color);

	// Анти-БУКОВКИ :coded by Делан

#define MAX_UPPERS_CHAR_PRC 30
#define MAX_UPPERS_SEQ_CHAR 3

	if ((subcmd != SCMD_AUCTION) && (!IS_IMMORTAL(ch)) && (!ch->IsNpc())) {
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
			SendMsgToChar("Но вы же недавно говорили тоже самое?!\r\n", ch);
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
			SendMsgToChar(SOUNDPROOF, ch);
			return;
		}
		*/
		if (ch->IsFlagged(EPrf::kNoRepeat))
			SendMsgToChar(OK, ch);
		else {
			snprintf(buf1, kMaxStringLength, "%sВы %s : '%s'%s", color_on,
					 com_msgs[subcmd].you_action, argument, kColorNrm);
			act(buf1, false, ch, 0, 0, kToChar | kToSleep);

			if (!ch->IsNpc()) {
				strcat(buf1, "\r\n");
				ch->remember_add(buf1, Remember::ALL);
			}
		}
		switch (subcmd) {
			case SCMD_SHOUT: ign_flag = EIgnore::kShout;
				break;
			case SCMD_GOSSIP:
				if (ch->IsFlagged(EPlrFlag::kSpamer))
					return;
				ign_flag = EIgnore::kGossip;
				break;
			case SCMD_HOLLER:
				if (ch->IsFlagged(EPlrFlag::kSpamer))
					return;
				ign_flag = EIgnore::kHoller;
				break;
			default: ign_flag = 0;
		}
		snprintf(out_str, kMaxStringLength, "$n %s : '%s'", com_msgs[subcmd].hi_action, argument);
		if (IS_FEMALE(ch)) {
			if (!ch->IsNpc() && (subcmd == SCMD_GOSSIP)) {
				snprintf(buf1, kMaxStringLength, "%s%s заметила :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!ch->IsNpc() && (subcmd == SCMD_HOLLER)) {
				snprintf(buf1, kMaxStringLength, "%s%s заорала :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		} else {
			if (!ch->IsNpc() && (subcmd == SCMD_GOSSIP)) {
				snprintf(buf1, kMaxStringLength, "%s%s заметил :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!ch->IsNpc() && (subcmd == SCMD_HOLLER)) {
				snprintf(buf1, kMaxStringLength, "%s%s заорал :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		}
	}
	// now send all the strings out
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
			!i->character->IsFlagged(com_msgs[subcmd].noflag) &&
			!i->character->IsFlagged(EPlrFlag::kWriting) &&
			!ROOM_FLAGGED(i->character->in_room, ERoomFlag::kSoundproof)
			&& i->character->GetPosition() > EPosition::kSleep) {
			if (ignores(i->character.get(), ch, ign_flag)) {
				continue;
			}

			if (subcmd == SCMD_SHOUT
				&& ((world[ch->in_room]->zone_rn != world[i->character->in_room]->zone_rn)
					|| !AWAKE(i->character))) {
				continue;
			}

			SendMsgToChar(color_on, i->character.get());
			act(out_str, false, ch, 0, i->character.get(), kToVict | kToSleep | kToNotDeaf);
			SendMsgToChar(kColorNrm, i->character.get());
			const std::string text = Remember::format_gossip(ch, i->character.get(), subcmd, argument);
			i->character->remember_add(text, Remember::ALL);
		}
	}
}

void do_mobshout(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *i;

	// to keep pets, etc from being ordered to shout
	if (!(ch->IsNpc() || IS_IMMORTAL(ch)))
		return;
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	skip_spaces(&argument); //убираем пробел в начале сообщения
	sprintf(buf, "$n заорал$g : '%s'", argument);

	// now send all the strings out
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING
			&& i->character
			&& !i->character->IsFlagged(EPlrFlag::kWriting)
			&& i->character->GetPosition() > EPosition::kSleep) {
			SendMsgToChar(kColorBoldYel, i->character.get());
			act(buf, false, ch, 0, i->character.get(), kToVict | kToSleep | kToNotDeaf);
			SendMsgToChar(kColorNrm, i->character.get());
		}
	}
}

void do_pray_gods(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	DescriptorData *i;
	CharData *victim = nullptr;

	skip_spaces(&argument);

	if (!ch->IsNpc() && (ch->IsFlagged(EPlrFlag::kDumbed) || ch->IsFlagged(EPlrFlag::kMuted))) {
		SendMsgToChar("Вам запрещено обращаться к Богам, вероятно, вы их замучили...\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch)) {
		// Выделяем чара кому отвечают иммы
		argument = one_argument(argument, arg1);
		skip_spaces(&argument);
		if (!*arg1) {
			SendMsgToChar("Какому смертному вы собираетесь ответить?\r\n", ch);
			return;
		}
		victim = get_player_vis(ch, arg1, EFind::kCharInWorld);
		if (victim == nullptr) {
			SendMsgToChar("Такого нет в игре!\r\n", ch);
			return;
		}
	}

	if (!*argument) {
		sprintf(buf, "С чем вы хотите обратиться к Богам?\r\n");
		SendMsgToChar(buf, ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar(OK, ch);
	else {
		if (ch->IsNpc())
			return;
		if (IS_IMMORTAL(ch)) {
			sprintf(buf, "&RВы одарили СЛОВОМ %s : '%s'&n\r\n", GET_PAD(victim, 3), argument);
		} else {
			sprintf(buf, "&RВы воззвали к Богам с сообщением : '%s'&n\r\n", argument);
			SetWait(ch, 3, false);
		}
		SendMsgToChar(buf, ch);
		ch->remember_add(buf, Remember::PRAY_PERSONAL);
	}

	if (IS_IMMORTAL(ch)) {
		sprintf(buf, "&R%s ответил%s вам : '%s'&n\r\n", GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		SendMsgToChar(buf, victim);
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

		snprintf(buf, kMaxStringLength, "&R[%5d] %s воззвал%s к богам с сообщением : '%s'&n\r\n",
				world[ch->in_room]->vnum, GET_NAME(ch), GET_CH_SUF_1(ch), argument);
	}

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING) {
			if ((IS_IMMORTAL(i->character.get())
				|| (GET_GOD_FLAG(i->character.get(), EGf::kDemigod)
					&& (GetRealLevel(ch) < 6)))
				&& (i->character.get() != ch)) {
				SendMsgToChar(buf, i->character.get());
				i->character->remember_add(buf, Remember::ALL);
			}
		}
	}
}


