/**
\file do_offtop.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 23.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/communication/offtop.h"

void do_offtop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || GetRealLevel(ch) >= kLvlImmortal || ch->IsFlagged(EPrf::kStopOfftop)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed) || ch->IsFlagged(EPlrFlag::kMuted)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}
	//if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof)) {
		SendMsgToChar(SOUNDPROOF, ch);
		return;
	}
	if (GetRealLevel(ch) < offtop_system::kMinOfftopLvl && !GetRealRemort(ch)) {
		SendMsgToChar(ch, "Вам стоит достичь хотя бы %d уровня, чтобы вы могли оффтопить.\r\n",
					  offtop_system::kMinOfftopLvl);
		return;
	}
	if (!ch->IsFlagged(EPrf::kOfftopMode)) {
		SendMsgToChar("Вы вне видимости канала.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar(ch, "Раз нечего сказать, то лучше уж помолчать.");
		return;
	}
	utils::ConvertToLow(argument);
	if (!strcmp(ch->get_last_tell().c_str(), argument)) {
		SendMsgToChar("Но вы же недавно говорили тоже самое!?!\r\n", ch);
		return;
	}
	// эта проверка должна идти последней и послее нее мессага полюбому идет в эфир
	if (!antispam::check(ch, antispam::kOfftopMode)) {
		return;
	}
	ch->set_last_tell(argument);
	if (ch->IsFlagged(EPlrFlag::kSpamer)) // а вот фиг, еще проверка :)
		return;
	snprintf(buf, kMaxStringLength, "[оффтоп] %s : '%s'\r\n", GET_NAME(ch), argument);
	snprintf(buf1, kMaxStringLength, "&c%s&n", buf);
	for (DescriptorData *i = descriptor_list; i; i = i->next) {
		// переплут как любитель почитывать логи за ночь очень хотел этот канал...
		// а мы шо, не люди? даешь оффтоп 34-ым! кому не нравится - реж оффтоп...
		if (STATE(i) == CON_PLAYING
			&& i->character
// оффтоп видят все, временно
//			&& (GetRealLevel(i->character) < kLvlImmortal || IS_IMPL(i->character))
			&& i->character->IsFlagged(EPrf::kOfftopMode)
			&& !i->character->IsFlagged(EPrf::kStopOfftop)
			&& !ignores(i->character.get(), ch, EIgnore::kOfftop)) {
			SendMsgToChar(i->character.get(), "%s%s%s", kColorCyn, buf, kColorNrm);
			i->character->remember_add(buf1, Remember::ALL);
		}
	}
	ch->remember_add(buf1, Remember::OFFTOP);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
