/**
 \file spell_recall.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellRecall manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/char_handler.h"

namespace handlers {

EStageResult SpellRecall(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	RoomRnum to_room = kNowhere, fnd_room = kNowhere;

	if (!victim || victim->IsNpc() || ch->in_room != victim->in_room || GetRealLevel(victim) >= kLvlImmortal) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!privilege::IsGod(ch) && AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (victim != ch) {
		if (group::same_group(ch, victim)) {
			if (number(1, 100) <= 5) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
		} else if (!ch->IsNpc() || (ch->has_master()
			&& !ch->get_master()->IsNpc())) // игроки не в группе и  чармисы по приказу не могут реколить свитком
		{
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}

		if ((ch->IsNpc() && CalcGeneralSaving(ch, victim, ESaving::kWill, GetRealInt(ch))) || privilege::IsGod(victim)) {
			return EStageResult::kSuccess;
		}
	}

	if ((to_room = GetRoomRnum(GET_LOADROOM(victim))) == kNowhere)
		to_room = GetRoomRnum(calc_loadroom(victim));

	if (to_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	fnd_room = target_resolver::GetRandomTeleportTargetInZone(victim, to_room);
	if (fnd_room == kNowhere) {
		to_room = Clan::CloseRent(to_room);
		fnd_room = target_resolver::GetRandomTeleportTargetInZone(victim, to_room);
	}

	if (fnd_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (victim->GetEnemy() && (victim != ch)) {
		if (!pk_agro_action(ch, victim->GetEnemy()))
			return EStageResult::kSuccess;
	}
	if (!enter_wtrigger(world[fnd_room], ch, -1))
		return EStageResult::kSuccess;
	// kWorldOfRecall overrides the kCastDisappearToRoom /
	// kCastAppearToRoom defaults with its specific "centre of room" wording.
	act(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kCastDisappearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	RemoveCharFromRoom(victim);
	PlaceCharToRoom(victim, fnd_room);
	mount::Dismount(victim);
	act(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kCastAppearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom);
	sight::look_at_room(victim, 0);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
