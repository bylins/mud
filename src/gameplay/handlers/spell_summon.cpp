/**
 \file spell_summon.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellSummon manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "utils/logger.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/minions.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/char_handler.h"

// SummonFollowingCharmices: single-use helper for SpellSummon (kept file-local).
static void SummonFollowingCharmices(CharData *ch, CharData *victim, RoomRnum vic_room, RoomRnum ch_room) {
// призываем чармисов
for (auto *k : victim->followers) {
	if (k->in_room == vic_room) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)) {
			if (!k->GetEnemy()) {
				// Charmice reuses kSummon's disappear/appear keys but with the
				// kCustomMsgThree "arrived following the master" line on arrival.
				act(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCastDisappearToRoom).c_str(),
					true, k, nullptr, nullptr, kToRoom | kToArenaListen);
				RemoveCharFromRoom(k);
				PlaceCharToRoom(k, ch_room);
				act(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgThree).c_str(),
					true, k, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgTwo).c_str(),
					false, ch, nullptr, k, kToVict);
			}
		}
	}
}
}

namespace handlers {

EStageResult SpellSummon(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	RoomRnum ch_room, vic_room;

	if (ch == nullptr || victim == nullptr || ch == victim) {
		return EStageResult::kSuccess;
	}
	if (!victim->desc) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
	}
	ch_room = ch->in_room;
	vic_room = victim->in_room;

	if (ch_room == kNowhere || vic_room == kNowhere) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
		SendToTC(ch, true, true, true, "Цель в Nowhere\r\n");
		return EStageResult::kSuccess;
	}

	if (ch->IsNpc() && victim->IsNpc()) {
		SendToTC(ch, true, true, true, "Да ты МОБ!!!!!\r\n");
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (privilege::IsImmortal(victim)) {
		if (ch->IsNpc() || (!ch->IsNpc() && GetRealLevel(ch) < GetRealLevel(victim))) {
			SendToTC(ch, true, true, true, "Неположено сие деяние!\r\n");
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
	}

	if (!ch->IsNpc() && victim->IsNpc()) {
		if (victim->get_master() != ch) {
			SendToTC(ch, true, true, true, "Чармис не ваш\r\n");
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
	}

	if (!privilege::IsImmortal(ch)) {
		if (!ch->IsNpc() || IsCharmice(ch)) {
			if (AFF_FLAGGED(ch, EAffect::kGodsShield)) {
				SendToTC(ch, true, true, true, "Чармис под зб\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kResurrectProtected) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
			if (!victim->IsFlagged(EPrf::KSummonable) && !group::same_group(ch, victim)) {
				SendToTC(ch, true, true, true, "Чармис не в вашей группе\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kResurrectNoPower) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
			if (NORENTABLE(victim) && !IsCharmice(ch)) {
				SendToTC(ch, true, true, true, "Ваша жертва совсем не рентабельна!\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
			if (victim->GetEnemy()
				|| victim->GetPosition() < EPosition::kRest) {
				SendToTC(ch, true, true, true, "Чармис сражается или дрыхнет\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgFour) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
		}
		if (victim->get_wait() > 0) {
			SendToTC(ch, true, true, true, "Чармис в лаге\r\n");
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}

		if (ROOM_FLAGGED(ch_room, ERoomFlag::kNoSummonOut)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kDeathTrap)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kSlowDeathTrap)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kTunnel)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kNoBattle)
			|| ROOM_FLAGGED(ch_room, ERoomFlag::kGodsRoom)
			|| SECT(ch->in_room) == ESector::kSecret
			|| (!group::same_group(ch, victim)
				&& (ROOM_FLAGGED(ch_room, ERoomFlag::kPeaceful) || ROOM_FLAGGED(ch_room, ERoomFlag::kArena)))) {
			SendToTC(ch, true, true, true, "Чармис в носуммоне\r\n");
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
		// отдельно проверку на клан комнаты, своих чармисов призвать можем ()
		if (!Clan::MayEnter(victim, ch_room, kHousePortal) && !(victim->has_master()) && (victim->get_master() != ch)) {
			SendToTC(ch, true, true, true, "Чармис доступ в замок запрещен\r\n");
			SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}

		if (!ch->IsNpc()) {
			if (ROOM_FLAGGED(vic_room, ERoomFlag::kNoSummonOut)
				|| ROOM_FLAGGED(vic_room, ERoomFlag::kGodsRoom)
				|| !Clan::MayEnter(ch, vic_room, kHousePortal)
				|| AFF_FLAGGED(victim, EAffect::kNoTeleport)
				|| (!group::same_group(ch, victim)
					&& (ROOM_FLAGGED(vic_room, ERoomFlag::kTunnel) || ROOM_FLAGGED(vic_room, ERoomFlag::kArena)))) {
				SendToTC(ch, true, true, true, "Чармис в носуммоне\r\n");
				SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kSummonFail) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
		} else {
			if (ROOM_FLAGGED(vic_room, ERoomFlag::kNoSummonOut) || AFF_FLAGGED(victim, EAffect::kNoTeleport)) {
				// block notice on kSummon's sheaf as kCustomMsgOne.
				SendMsgToChar(MUD::SpellMessages().GetMessage(
						ESpell::kSummon, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
				return EStageResult::kSuccess;
			}
		}

		if (ch->IsNpc() && number(1, 100) < 30) {
			return EStageResult::kSuccess;
		}
	}
	if (!enter_wtrigger(world[ch_room], ch, -1)) {
		SendToTC(ch, true, true, true, "Чармис призыв запрещен триггером\r\n");
		return EStageResult::kSuccess;
	}
	// kSummon overrides kCastDisappearToRoom (vic disappearing
	// from old room) and kCastAppearToRoom (vic arriving in caster's room). kCustomMsgTwo
	// is the to-vict notification.
	act(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kCastDisappearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	RemoveCharFromRoom(victim);
	PlaceCharToRoom(victim, ch_room);
	CheckAutoNosummon(victim);
	victim->SetPosition(EPosition::kStand);
	act(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kCastAppearToRoom).c_str(),
		true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	act(MUD::SpellMessages().GetMessage(ESpell::kSummon, ESpellMsg::kCustomMsgTwo).c_str(),
		false, ch, nullptr, victim, kToVict);
	mount::Dismount(victim);
	sight::look_at_room(victim, 0);
	SummonFollowingCharmices(ch, victim, vic_room, ch_room);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
