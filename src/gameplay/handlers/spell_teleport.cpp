/**
 \file spell_teleport.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellTeleport manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/char_handler.h"

namespace handlers {

EStageResult SpellTeleport(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	RoomRnum in_room = ch->in_room, fnd_room = kNowhere;

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!privilege::IsGod(ch) && AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kTeleport, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	fnd_room = target_resolver::GetRandomTeleportTargetInZone(ch, in_room);
	if (fnd_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kTeleport, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return EStageResult::kSuccess;
	// kTeleport overrides kCastDisappearToRoom / kCastAppearToRoom.
	act(MUD::SpellMessages().GetMessage(ESpell::kTeleport, ESpellMsg::kCastDisappearToRoom).c_str(),
		false, ch, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	mount::Dismount(ch);
	act(MUD::SpellMessages().GetMessage(ESpell::kTeleport, ESpellMsg::kCastAppearToRoom).c_str(),
		false, ch, nullptr, nullptr, kToRoom);
	sight::look_at_room(ch, 0);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
