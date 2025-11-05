#include "parry.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/equipment.h"

bool CanPerformParry(CharData *victim, const HitData &hit_data);

void GoParry(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || AFF_FLAGGED(ch, EAffect::kStopLeft) || IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	ch->battle_affects.set(kEafParry);
	SendMsgToChar("Вы попробуете отклонить следующую атаку.\r\n", ch);
}

void DoParry(CharData *ch, char *argument /*argument*/, int/* cmd*/cmd, int/* subcmd*/subcmd) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kParry)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kParry)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		if (GET_EQ(ch, EEquipPos::kBoths)) {
			SendMsgToChar("Вы не можете отклонить атаку двуручным оружием.\r\n", ch);
			return;
		}

		bool prim = 0, offh = 0;
		if (GET_EQ(ch, EEquipPos::kWield) && GET_EQ(ch, EEquipPos::kWield)->get_type() == EObjType::kWeapon) {
			prim = 1;
		}
		if (GET_EQ(ch, EEquipPos::kHold) && GET_EQ(ch, EEquipPos::kHold)->get_type() == EObjType::kWeapon) {
			offh = 1;
		}

		if (!prim && !offh) {
			SendMsgToChar("Вы не можете отклонить атаку безоружным.\r\n", ch);
			return;
		} else if (!prim || !offh) {
			SendMsgToChar("Вы можете отклонить атаку только с двумя оружиями в руках.\r\n", ch);
			return;
		}
	}

	if (ch->battle_affects.get(kEafOverwhelm)) {
		SendMsgToChar("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	GoParry(ch);
}

void CheckParryOverride(CharData *ch) {
	std::string message;
	if (ch->battle_affects.get(kEafBlock)) {
		message = "Вы прекратили прятаться за щит и бросились в бой.";
		ch->battle_affects.unset(kEafBlock);
	}
	if (ch->battle_affects.get(kEafParry)) {
		message = "Вы прекратили парировать атаки и бросились в бой.";
		ch->battle_affects.unset(kEafParry);
	}
	if (ch->battle_affects.get(kEafMultyparry)) {
		message = "Вы забыли о защите и бросились в бой.";
		ch->battle_affects.unset(kEafMultyparry);
	}
	act(message.c_str(), false, ch, 0, 0, kToChar);
}

void ProcessParry(CharData *ch, CharData *victim, HitData &hit_data) {
	if (!CanPerformParry(victim, hit_data)) {
		return;
	}
	if (!((GET_EQ(victim, EEquipPos::kWield)
		&& GET_EQ(victim, EEquipPos::kWield)->get_type() == EObjType::kWeapon
		&& GET_EQ(victim, EEquipPos::kHold)
		&& GET_EQ(victim, EEquipPos::kHold)->get_type() == EObjType::kWeapon)
		|| victim->IsNpc()
		|| IS_IMMORTAL(victim))) {
		SendMsgToChar("У вас нечем отклонить атаку противника.\r\n", victim);
		victim->battle_affects.unset(kEafParry);
	} else {
		int range = number(1, MUD::Skill(ESkill::kParry).difficulty);
		int prob = CalcCurrentSkill(victim, ESkill::kParry, ch);
		prob = prob * 100 / range;
		TrainSkill(victim, ESkill::kParry, prob < 100, ch);
		SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kParry).name, range, prob, prob >= 70);
		if (hit_data.GetFlags()[fight::kCritLuck]) {
			prob = 0;
		}
		if (prob < 70
			|| ((hit_data.weap_skill == ESkill::kBows || hit_data.hit_type == fight::type_maul)
				&& !IS_IMMORTAL(victim)
				&& (!CanUseFeat(victim, EFeat::kParryArrow)
					|| number(1, 1000) >= 20 * std::min(GetRealDex(victim), 35)))) {
			act("Вы не смогли отбить атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N не сумел$G отбить вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n не сумел$g отбить атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			prob = 2;
			victim->battle_affects.set(kEafUsedleft);
		} else if (prob < 100) {
			act("Вы немного отклонили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N немного отклонил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n немного отклонил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, number(0, 2) ? EEquipPos::kWield : EEquipPos::kHold, hit_data.dam, 10);
			prob = 1;
			hit_data.dam *= 10 / 15;
			victim->battle_affects.set(kEafUsedleft);
		} else if (prob < 170) {
			act("Вы частично отклонили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N частично отклонил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n частично отклонил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, number(0, 2) ? EEquipPos::kWield : EEquipPos::kHold, hit_data.dam, 15);
			prob = 0;
			hit_data.dam /= 2;
			victim->battle_affects.set(kEafUsedleft);
		} else {
			act("Вы полностью отклонили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N полностью отклонил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n полностью отклонил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, number(0, 2) ? EEquipPos::kWield : EEquipPos::kHold, hit_data.dam, 25);
			prob = 0;
			hit_data.dam = -1;
		}
		if (prob > 0) {
			SetSkillCooldownInFight(victim, ESkill::kGlobalCooldown, 1);
		}
		SetSkillCooldownInFight(victim, ESkill::kParry, prob);
		victim->battle_affects.unset(kEafParry);
	}
}

bool CanPerformParry(CharData *victim, const HitData &hit_data) {
	return (hit_data.dam > 0
		&& !hit_data.hit_no_parry
		&& victim->battle_affects.get(kEafParry)
		&& !AFF_FLAGGED(victim, EAffect::kStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kStopRight)
		&& !AFF_FLAGGED(victim, EAffect::kStopLeft)
		&& victim->get_wait() <= 0
		&& AFF_FLAGGED(victim, EAffect::kHold) == 0);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :