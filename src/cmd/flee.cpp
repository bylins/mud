#include "flee.h"

#include "act_movement.h"
#include "entities/char_data.h"

EDirection SelectRndDirection(CharData *ch, int fail_chance);

void ReduceExpAfterFlee(CharData *ch, CharData *victim, RoomRnum room) {
	if (CanUseFeat(ch, EFeat::kRetreat) || ROOM_FLAGGED(room, ERoomFlag::kArena)) {
		return;
	}

	const auto loss = std::max(1, GET_REAL_MAX_HIT(victim) - GET_HIT(victim)) * GetRealLevel(victim);
	EndowExpToChar(ch, -loss);
}

// ********************* FLEE PROCEDURE
void GoFlee(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kHold) || ch->get_wait() > 0) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kNoFlee) ||
		AFF_FLAGGED(ch, EAffect::kCombatLuck) ||
		ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Невидимые оковы мешают вам сбежать.\r\n", ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вы не можете сбежать из этого положения.\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch)) {
		SetWaitState(ch, kBattleRound);
	}

	if (ch->IsOnHorse() && (ch->get_horse()->GetPosition() < EPosition::kFight ||
		AFF_FLAGGED(ch->get_horse(), EAffect::kHold))) {
		SendMsgToChar("Ваш скакун не в состоянии вынести вас из боя!\r\n", ch);
		return;
	}

	auto direction = SelectRndDirection(ch, CanUseFeat(ch, EFeat::kRetreat) ? 0 : 50);
	if (direction != EDirection::kUndefinedDir) {
		const auto was_fighting = ch->GetEnemy();
		const auto was_in = ch->in_room;

		if (DoSimpleMove(ch, direction, true, nullptr, true)) {
			act("$n запаниковал$g и пытал$u сбежать!",
				true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			if (ch->IsOnHorse()) {
				act("Верн$W $N вынес$Q вас из боя.", false, ch, nullptr, ch->get_horse(), kToChar);
			} else {
				SendMsgToChar("Вы быстро убежали с поля битвы.\r\n", ch);
			}

			if (was_fighting && !ch->IsNpc()) {
				ReduceExpAfterFlee(ch, was_fighting, was_in);
			}
		} else {
			act("$n запаниковал$g и попытал$u убежать, но не смог$q!",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
		}
	} else {
		act("$n запаниковал$g и попытал$u убежать, но не смог$q!",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		SendMsgToChar("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
	}
}

void GoDirectFlee(CharData *ch, int direction) {
	if (AFF_FLAGGED(ch, EAffect::kHold) || ch->get_wait() > 0) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kNoFlee) ||
		AFF_FLAGGED(ch, EAffect::kCombatLuck) ||
		ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Невидимые оковы мешают вам сбежать.\r\n", ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вы не сможете сбежать из этого положения.\r\n", ch);
		return;
	}

	if (IsCorrectDirection(ch, direction, true, false)
		&& !ROOM_FLAGGED(EXIT(ch, direction)->to_room(), ERoomFlag::kDeathTrap)) {
		if (DoSimpleMove(ch, direction, true, nullptr, true)) {
			const auto was_in = ch->in_room;
			const auto was_fighting = ch->GetEnemy();

			act("$n запаниковал$g и попытал$u убежать.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Вы быстро убежали с поля битвы.\r\n", ch);
			if (was_fighting && !ch->IsNpc()) {
				ReduceExpAfterFlee(ch, was_fighting, was_in);
			}

			if (!IS_IMMORTAL(ch)) {
				SetWaitState(ch, 1 * kBattleRound);
			}
			return;
		}
	}
	GoFlee(ch);
}

void DoFlee(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int direction = -1;
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}
	if (CanUseFeat(ch, EFeat::kCalmness) || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		one_argument(argument, arg);
		if ((direction = search_block(arg, dirs, false)) >= 0 ||
			(direction = search_block(arg, dirs_rus, false)) >= 0) {
			GoDirectFlee(ch, direction);
			return;
		}
	}
	GoFlee(ch);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
