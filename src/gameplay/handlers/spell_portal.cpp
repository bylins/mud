/**
 \file spell_portal.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellPortal manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/portal.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/groups.h"
#include "administration/privilege.h"
#include "gameplay/magic/spells.h"
#include "gameplay/core/remort.h"

namespace handlers {

EStageResult SpellPortal(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	RoomRnum fnd_room;

	if (victim == nullptr)
		return EStageResult::kSuccess;
	if (GetRealLevel(victim) > GetRealLevel(ch) && !victim->IsFlagged(EPrf::KSummonable) && !group::same_group(ch, victim)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kPortal, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	// пентить чаров <=10 уровня, нельзя так-же нельзя пентать иммов
	if (!privilege::IsGod(ch)) {
		if ((!victim->IsNpc() && GetRealLevel(victim) <= 10 && remort::GetRealRemort(ch) < 9) || privilege::IsImmortal(victim)
			|| AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kPortal, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
	}
	if (victim->IsNpc()) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kPortal, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	fnd_room = victim->in_room;
	if (fnd_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kPortal, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (!privilege::IsGod(ch) && (SECT(fnd_room) == ESector::kSecret || ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) || ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) || ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kPortal, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (ch->in_room == fnd_room) {
		SendMsgToChar("Может, вам лучше просто потоптаться на месте?\r\n", ch);
		return EStageResult::kSuccess;
	}

	bool pkPortal = pk_action_type_summon(ch, victim) == PK_ACTION_REVENGE ||
		pk_action_type_summon(ch, victim) == PK_ACTION_FIGHT;

	if (privilege::IsImmortal(ch) || GET_GOD_FLAG(victim, EGf::kGodscurse)
		// раньше было <= PK_ACTION_REVENGE, что вызывало абьюз при пенте на чара на арене,
		// или пенте кидаемой с арены т.к. в данном случае использовалось PK_ACTION_NO которое меньше PK_ACTION_REVENGE
		|| pkPortal || ((!victim->IsNpc() || IsCharmice(ch)) && victim->IsFlagged(EPrf::KSummonable))
		|| group::same_group(ch, victim)) {
		if (pkPortal) {
			pk_increment_revenge(ch, victim);
		}
		if (room_spells::RoomHasPortal(world[ch->in_room])) {
			bool remove = false;
			for (const auto &aff : world[ch->in_room]->affected) {
				if (room_spells::IsPortalAffect(aff->affect_type)) {
					if (aff->caster_id == ch->get_uid() && aff->modifier == fnd_room) {
						remove = true;
						break;
					}
				}
			}
			if (remove)
				RemovePortalGate(ch->in_room);
		}
		// pk_unique on the affect replaces the old per-room pkPenterUnique field
		// pkPortal => imposing caster's uid; else 0.
		AddPortalTimer(ch, world[fnd_room], ch->in_room, 29, pkPortal ? ch->get_uid() : 0);

		// pentagram-appearance narration lives in kPortal's
		// sheaf -- kCustomMsgOne is the normal line, kCustomMsgTwo is the pk variant
		// (blood-tinged). Each is sent to both kToChar and kToRoom of the destination
		// endpoint's first occupant.
		const auto &dest_pentagram = MUD::SpellMessages().GetMessage(
				ESpell::kPortal, pkPortal ? ESpellMsg::kCustomMsgTwo : ESpellMsg::kCustomMsgOne);
		act(dest_pentagram.c_str(), false, world[fnd_room]->first_character(), nullptr, nullptr, kToChar);
		act(dest_pentagram.c_str(), false, world[fnd_room]->first_character(), nullptr, nullptr, kToRoom);
		CheckAutoNosummon(victim);

		// если пенту ставит имм с привилегией arena (и находясь на арене), то пента получается односторонняя
		if (privilege::CheckFlag(ch, privilege::kArenaMaster) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)) {
			return EStageResult::kSuccess;
		}

		AddPortalTimer(ch, world[ch->in_room], fnd_room, 29, pkPortal ? ch->get_uid() : 0);

		// Caster-side pentagram (same key resolution as the destination side above).
		const auto &caster_pentagram = MUD::SpellMessages().GetMessage(
				ESpell::kPortal, pkPortal ? ESpellMsg::kCustomMsgTwo : ESpellMsg::kCustomMsgOne);
		act(caster_pentagram.c_str(), false, world[ch->in_room]->first_character(), nullptr, nullptr, kToChar);
		act(caster_pentagram.c_str(), false, world[ch->in_room]->first_character(), nullptr, nullptr, kToRoom);
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
