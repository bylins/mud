/**
\file multyparry.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "multyparry.h"

#include "engine/entities/char_data.h"
#include "utils/random.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/affects/affect_contants.h"
#include "engine/db/global_objects.h"
#include "gameplay/fight/common.h"
#include "gameplay/mechanics/equipment.h"

bool CanPerformMultyparry(CharData *victim, const HitData &hit_data);

void go_multyparry(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || AFF_FLAGGED(ch, EAffect::kStopLeft) || IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	ch->battle_affects.set(kEafMultyparry);
	SendMsgToChar("Вы попробуете использовать веерную защиту.\r\n", ch);
}

void do_multyparry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kMultiparry)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kMultiparry)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	ObjData *primary = GET_EQ(ch, EEquipPos::kWield), *offhand = GET_EQ(ch, EEquipPos::kHold);
	if (!(ch->IsNpc()
		|| (primary
			&& primary->get_type() == EObjType::kWeapon
			&& offhand
			&& offhand->get_type() == EObjType::kWeapon)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("Вы не можете отражать атаки безоружным.\r\n", ch);
		return;
	}
	if (ch->battle_affects.get(kEafOverwhelm)) {
		SendMsgToChar("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_multyparry(ch);
}

void ProcessMultyparry(CharData *ch, CharData *victim, HitData &hit_data) {
	if (!CanPerformMultyparry(victim, hit_data)) {
		return;
	}
	if (!((GET_EQ(victim, EEquipPos::kWield)
		&& GET_EQ(victim, EEquipPos::kWield)->get_type() == EObjType::kWeapon
		&& GET_EQ(victim, EEquipPos::kHold)
		&& GET_EQ(victim, EEquipPos::kHold)->get_type() == EObjType::kWeapon)
		|| victim->IsNpc()
		|| IS_IMMORTAL(victim))) {
		SendMsgToChar("У вас нечем отклонять атаки противников.\r\n", victim);
	} else {
		int range = number(1, MUD::Skill(ESkill::kMultiparry).difficulty) + 15*victim->battle_counter;
		int prob = CalcCurrentSkill(victim, ESkill::kMultiparry, ch);
		prob = prob * 100 / range;

		if ((hit_data.weap_skill == ESkill::kBows || hit_data.hit_type == fight::type_maul)
			&& !IS_IMMORTAL(victim)
			&& (!CanUseFeat(victim, EFeat::kParryArrow)
				|| number(1, 1000) >= 20 * std::min(GetRealDex(victim), 35))) {
			prob = 0;
		} else {
			++(victim->battle_counter);
		}

		TrainSkill(victim, ESkill::kMultiparry, prob >= 50, ch);
		SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kMultiparry).name, range, prob, prob >= 50);
		if (hit_data.GetFlags()[fight::kCritLuck]) {
			prob = 0;
		}
		if (prob < 50) {
			act("Вы не смогли отбить атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N не сумел$G отбить вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n не сумел$g отбить атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
		} else if (prob < 90) {
			act("Вы немного отклонили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N немного отклонил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n немного отклонил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, number(0, 2) ? EEquipPos::kWield : EEquipPos::kHold, hit_data.dam, 10);
			hit_data.dam *= 10 / 15;
		} else if (prob < 180) {
			act("Вы частично отклонили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N частично отклонил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n частично отклонил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, number(0, 2) ? EEquipPos::kWield : EEquipPos::kHold, hit_data.dam, 15);
			hit_data.dam /= 2;
		} else {
			act("Вы полностью отклонили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N полностью отклонил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n полностью отклонил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, number(0, 2) ? EEquipPos::kWield : EEquipPos::kHold, hit_data.dam, 25);
			hit_data.dam = -1;
		}
	}
}

bool CanPerformMultyparry(CharData *victim, const HitData &hit_data) {
	return (hit_data.dam > 0
		&& !hit_data.hit_no_parry
		&& victim->battle_affects.get(kEafMultyparry)
		&& !AFF_FLAGGED(victim, EAffect::kStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kStopRight)
		&& !AFF_FLAGGED(victim, EAffect::kStopLeft)
		&& victim->battle_counter < (GetRealLevel(victim) + 4) / 5
		&& victim->get_wait() <= 0
		&& AFF_FLAGGED(victim, EAffect::kHold) == 0);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
