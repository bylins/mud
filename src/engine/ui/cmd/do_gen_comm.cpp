/**
\file do_gen_comm.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "do_gen_comm.h"

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/economics/auction.h"
#include "gameplay/communication/remember.h"

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

std::string format_gossip_name(CharData *ch, CharData *vict);
std::string format_gossip(CharData *ch, CharData *vict, int cmd, const char *argument);

// \todo Тут в одну команду свалено вообще все крики и до кучи аукцион.
// По-хорошему, надо распилить на несколько команд, а саму посылку сообщения всем вытащить в communication
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

	// to keep pets, etc. from being ordered to shout
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

	if (ch->IsFlagged(EPlrFlag::kMuted) && subcmd != kScmdAuction) {
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
	if (!*argument && subcmd != kScmdAuction) {
		sprintf(buf1, "ЛЕГКО! Но, Ярило вас побери, ЧТО %s???\r\n", com_msgs[subcmd].action);
		SendMsgToChar(buf1, ch);
		return;
	}

	// set up the color on code
	strcpy(color_on, com_msgs[subcmd].color);

	// Анти-БУКОВКИ :coded by Делан

#define MAX_UPPERS_CHAR_PRC 30
#define MAX_UPPERS_SEQ_CHAR 3

	if ((subcmd != kScmdAuction) && (!IS_IMMORTAL(ch)) && (!ch->IsNpc())) {
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
	if (subcmd == kScmdAuction) {
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
			act(buf1, false, ch, nullptr, nullptr, kToChar | kToSleep);

			if (!ch->IsNpc()) {
				strcat(buf1, "\r\n");
				ch->remember_add(buf1, Remember::ALL);
			}
		}
		switch (subcmd) {
			case kScmdShout: ign_flag = EIgnore::kShout;
				break;
			case kScmdGossip:
				if (ch->IsFlagged(EPlrFlag::kSpamer))
					return;
				ign_flag = EIgnore::kGossip;
				break;
			case kScmdHoller:
				if (ch->IsFlagged(EPlrFlag::kSpamer))
					return;
				ign_flag = EIgnore::kHoller;
				break;
			default: ign_flag = 0;
		}
		snprintf(out_str, kMaxStringLength, "$n %s : '%s'", com_msgs[subcmd].hi_action, argument);
		if (IS_FEMALE(ch)) {
			if (!ch->IsNpc() && (subcmd == kScmdGossip)) {
				snprintf(buf1, kMaxStringLength, "%s%s заметила :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!ch->IsNpc() && (subcmd == kScmdHoller)) {
				snprintf(buf1, kMaxStringLength, "%s%s заорала :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
		} else {
			if (!ch->IsNpc() && (subcmd == kScmdGossip)) {
				snprintf(buf1, kMaxStringLength, "%s%s заметил :'%s'%s\r\n", color_on, GET_NAME(ch), argument, kColorNrm);
				ch->remember_add(buf1, Remember::GOSSIP);
			}
			if (!ch->IsNpc() && (subcmd == kScmdHoller)) {
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

			if (subcmd == kScmdShout
				&& ((world[ch->in_room]->zone_rn != world[i->character->in_room]->zone_rn)
					|| !AWAKE(i->character))) {
				continue;
			}

			SendMsgToChar(color_on, i->character.get());
			act(out_str, false, ch, nullptr, i->character.get(), kToVict | kToSleep | kToNotDeaf);
			SendMsgToChar(kColorNrm, i->character.get());
			const std::string text = format_gossip(ch, i->character.get(), subcmd, argument);
			i->character->remember_add(text, Remember::ALL);
		}
	}
}

/**
* Болтовня ch, пишущаяся во вспом все к vict'иму. Изврат конечно, но переделывать
* систему в do_gen_comm чет облом пока, а возвращать сформированную строку из act() не хочется.
*/
std::string format_gossip(CharData *ch, CharData *vict, int cmd, const char *argument) {
	return fmt::format("{}{} {}{} : '{}'{}\r\n",
					   (cmd == kScmdGossip ? kColorYel : kColorBoldYel),
					   format_gossip_name(ch, vict).c_str(),
					   (cmd == kScmdGossip ? "заметил" : "заорал"),
					   GET_CH_VIS_SUF_1(ch, vict),
					   argument,
					   kColorNrm);
}

/**
* Формирование имени болтающего/орущего при записе во 'вспом все' в обход act().
* Иммы видны всегда, кто-ты с большой буквы.
*/
std::string format_gossip_name(CharData *ch, CharData *vict) {
	if (ch->get_name().empty()) {
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return "";
	}
	std::string name = IS_IMMORTAL(ch) ? GET_NAME(ch) : PERS(ch, vict, 0);
	name[0] = UPPER(name[0]);
	return name;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
