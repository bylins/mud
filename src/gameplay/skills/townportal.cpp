#include "townportal.h"
#include "administration/privilege.h"
#include "skill_messages.h"

#include "engine/ui/modify.h"
#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/abilities/abilities_constants.h"
#include "gameplay/fight/pk.h"
#include "gameplay/magic/magic_rooms.h"
#include "gameplay/mechanics/portal.h"
#include "engine/db/global_objects.h"
#include "engine/ui/table_wrapper.h"

#include <fmt/format.h>

void GoTownportal(CharData *ch, char *argument);
void TryOpenTownportal(CharData *ch, const Runestone &stone);
void TryOpenLabelPortal(CharData *ch, char *argument);
void OpenTownportal(CharData *ch, const Runestone &stone);
void SetSkillTownportalTimer(CharData *ch);
Runestone GetLabelPortal(CharData *ch);

// "врата": with no argument lists the stones you can open a gate to (your memorised stones);
// with a codeword it opens the gate. Runestone management (список/забыть) lives in DoRunestone.
void DoTownportal(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !GetSkill(ch, ESkill::kTownportal)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kTownportal, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	if (!argument || !*argument) {
		PageRunestonesToChar(ch);
		return;
	}

	one_argument(argument, arg);
	GoTownportal(ch, arg);
}

// "камень"/"stone": runestone management. Bare or "список" -> the memorised-stones list;
// "забыть <слово>" -> forget a stone.
void DoRunestone(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !GetSkill(ch, ESkill::kTownportal)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kTownportal, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	char arg2[kMaxInputLength];
	two_arguments(argument, arg, arg2);
	if (!str_cmp(arg, "забыть")) {
		auto &stone = MUD::Runestones().FindRunestone(arg2);
		RemoveRunestone(ch, stone);
		return;
	}

	PageRunestonesToChar(ch);
}

void GoTownportal(CharData *ch, char *argument) {
	auto &stone = MUD::Runestones().FindRunestone(argument);
	if (stone.IsAllowed() && IsRunestoneKnown(ch, stone)) {
		TryOpenTownportal(ch, stone);
	} else {
		TryOpenLabelPortal(ch, argument);
	}
}

void TryOpenTownportal(CharData *ch, const Runestone &stone) {
	if (IsTimedBySkill(ch, ESkill::kTownportal)) {
		SendMsgToChar("У вас недостаточно сил для постановки врат.\r\n", ch);
		return;
	}

	if (MUD::Runestones().FindRunestone(GET_ROOM_VNUM(ch->in_room)).IsAllowed()) {
		SendMsgToChar("Камень рядом с вами мешает вашей магии.\r\n", ch);
		return;
	}

	if (room_spells::IsRoomAffected(world[ch->in_room], room_spells::ERoomAffect::kRuneLabel)) {
		SendMsgToChar("Начертанные на земле магические руны подавляют вашу магию!\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic) && !privilege::IsGrGod(ch)) {
		SendMsgToChar("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", ch);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.", false, ch, nullptr, nullptr, kToRoom);
		return;
	}

	if (stone.IsDisabled()) {
		act("Лазурная пентаграмма возникла в воздухе... и сразу же растаяла.", false, ch, nullptr, nullptr, kToChar);
		act("$n сложил$g руки в молитвенном жесте, испрашивая у Богов врата...", false, ch, nullptr, nullptr, kToRoom);
		act("Лазурная пентаграмма возникла в воздухе... и сразу же растаяла.", false, ch, nullptr, nullptr, kToRoom);
		SetSkillTownportalTimer(ch);
		return;
	}

	OpenTownportal(ch, stone);
}

void TryOpenLabelPortal(CharData *ch, char *argument) {
	if (name_cmp(ch, argument)) {
		const auto &stone = GetLabelPortal(ch);
		if (stone.IsAllowed()) {
			TryOpenTownportal(ch, stone);
		} else {
			SendMsgToChar("Вы не оставляли рунных меток.\r\n", ch);
		}
		return;
	}
	SendMsgToChar("Вам неизвестны такие руны.\r\n", ch);
}

Runestone GetLabelPortal(CharData *ch) {
	auto label_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), room_spells::ERoomAffect::kRuneLabel);
	if (label_room) {
		return {ch->get_name(), label_room->vnum, ESkill::kTownportal, 1};
	} else {
		return {"undefined", kNowhere, ESkill::kTownportal, 1, "", Runestone::State::kForbidden};
	}
}

void OpenTownportal(CharData *ch, const Runestone &stone) {
	ImproveSkill(ch, ESkill::kTownportal, 1, nullptr);
	auto to_room = GetRoomRnum(stone.GetRoomVnum());
	// (issue.affect-flags: dead write to pkPenterUnique removed -- the field
	// retired, PK-uid lives on the kPortalTimer affect via pk_unique.)
	one_way_portal::ReplacePortalTimer(ch, ch->in_room, to_room, 29);
	act("Лазурная пентаграмма возникла в воздухе.", false, ch, nullptr, nullptr, kToChar);
	act("$n сложил$g руки в молитвенном жесте, испрашивая у Богов врата...", false, ch, nullptr, nullptr, kToRoom);
	act("Лазурная пентаграмма возникла в воздухе.", false, ch, nullptr, nullptr, kToRoom);
	SetSkillTownportalTimer(ch);
}

void SetSkillTownportalTimer(CharData *ch) {
//	if (!privilege::IsImmortal(ch)) {
		TimedSkill timed;
		timed.skill = ESkill::kTownportal;
		// timed.time - это unsigned char, поэтому при уходе в минус будет вынос на 255 и ниже
		int modif = GetSkill(ch, ESkill::kTownportal) / 7 + number(1, 5);
		timed.time = MAX(1, 25 - modif);
		ImposeTimedSkill(ch, &timed);
//	}
}


namespace one_way_portal {

void ReplacePortalTimer(CharData *ch, RoomRnum from_room, RoomRnum to_room, int time) {
	// Two-way endpoint at from_room, plus a one-way (kNoPortalExit) marker at to_room.
	AddPortalTimer(ch, world[from_room], to_room, time);
	AddPortalTimer(ch, world[to_room], from_room, time, 0, room_spells::ERoomAffect::kNoPortalExit);
}

} // namespace OneWayPortal

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
