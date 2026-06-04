/**
 \file spell_recall.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellRecall manual-cast handler (extracted from spells.cpp).
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

EStageResult SpellRecall(CastContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	RoomRnum to_room = kNowhere, fnd_room = kNowhere;
	RoomRnum rnum_start, rnum_stop;

	if (!victim || victim->IsNpc() || ch->in_room != victim->in_room || GetRealLevel(victim) >= kLvlImmortal) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	// kNoTeleportOut moved to <blocking><room_flags> in spells.xml; CallMagic
	// fizzles before this function runs.
	if (!ch->IsGod() && AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
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

		if ((ch->IsNpc() && CalcGeneralSaving(ch, victim, ESaving::kWill, GetRealInt(ch))) || victim->IsGod()) {
			return EStageResult::kSuccess;
		}
	}

	if ((to_room = GetRoomRnum(GET_LOADROOM(victim))) == kNowhere)
		to_room = GetRoomRnum(calc_loadroom(victim));

	if (to_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	(void) GetZoneRooms(world[to_room]->zone_rn, &rnum_start, &rnum_stop);
	fnd_room = GetTeleportTargetRoom(victim, rnum_start, rnum_stop);
	if (fnd_room == kNowhere) {
		to_room = Clan::CloseRent(to_room);
		(void) GetZoneRooms(world[to_room]->zone_rn, &rnum_start, &rnum_stop);
		fnd_room = GetTeleportTargetRoom(victim, rnum_start, rnum_stop);
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
	victim->dismount();
	act(MUD::SpellMessages().GetMessage(ESpell::kWorldOfRecall, ESpellMsg::kCastAppearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom);
	look_at_room(victim, 0);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
