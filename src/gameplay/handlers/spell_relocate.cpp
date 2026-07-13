/**
 \file spell_relocate.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellRelocate manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/char_handler.h"

namespace handlers {

EStageResult SpellRelocate(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	RoomRnum to_room, fnd_room;

	if (victim == nullptr)
		return EStageResult::kSuccess;

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!privilege::IsGod(ch) && AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	to_room = victim->in_room;

	if (to_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		fnd_room = Clan::CloseRent(to_room);
	} else {
		fnd_room = to_room;
	}

	if (fnd_room != to_room && !privilege::IsGod(ch)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (!privilege::IsGod(ch) &&
		(SECT(fnd_room) == ESector::kSecret ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kNoRelocateIn) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) || (ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) && !privilege::IsImmortal(ch)))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return EStageResult::kSuccess;
	// kRelocate shares the kTeleport disappear/appear wording
	// and adds its own kCustomMsgOne caster-side "azure flash" banner.
	act(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kCastDisappearToRoom).c_str(),
		true, ch, nullptr, nullptr, kToRoom);
	SendMsgToChar(MUD::SpellMessages().GetMessage(
			ESpell::kRelocate, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	mount::Dismount(ch);
	sight::look_at_room(ch, 0);
	act(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kCastAppearToRoom).c_str(),
		true, ch, nullptr, nullptr, kToRoom);
	SetBattleLag(ch, 2);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
