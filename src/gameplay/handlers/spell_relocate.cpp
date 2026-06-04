/**
 \file spell_relocate.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellRelocate manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/magic/spells.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_internal.h"
#include "engine/db/global_objects.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/cmd/do_hire.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/skills/animal_master.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "engine/ui/color.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/minions.h"
#include <cmath>

namespace handlers {

EStageResult SpellRelocate(CastContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	RoomRnum to_room, fnd_room;

	if (victim == nullptr)
		return EStageResult::kSuccess;

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!ch->IsGod() && AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
		SendSummonFail(ch, ESpell::kRelocate);
		return EStageResult::kSuccess;
	}

	to_room = victim->in_room;

	if (to_room == kNowhere) {
		SendSummonFail(ch, ESpell::kRelocate);
		return EStageResult::kSuccess;
	}

	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		fnd_room = Clan::CloseRent(to_room);
	} else {
		fnd_room = to_room;
	}

	if (fnd_room != to_room && !ch->IsGod()) {
		SendSummonFail(ch, ESpell::kRelocate);
		return EStageResult::kSuccess;
	}

	if (!ch->IsGod() &&
		(SECT(fnd_room) == ESector::kSecret ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kNoRelocateIn) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) || (ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) && !ch->IsImmortal()))) {
		SendSummonFail(ch, ESpell::kRelocate);
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
	ch->dismount();
	look_at_room(ch, 0);
	act(MUD::SpellMessages().GetMessage(ESpell::kRelocate, ESpellMsg::kCastAppearToRoom).c_str(),
		true, ch, nullptr, nullptr, kToRoom);
	SetBattleLag(ch, 2);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
