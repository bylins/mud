#include "shield_block.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/equipment.h"

bool CanPerformShieldBlock(CharData *victim, const HitData &hit_data);

void go_block(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopLeft)) {
		SendMsgToChar("Ваша рука парализована.\r\n", ch);
		return;
	}
	ch->battle_affects.set(kEafBlock);
	SendMsgToChar("Хорошо, вы попробуете отразить щитом следующую атаку.\r\n", ch);
}

void do_block(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kShieldBlock)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kShieldBlock)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь!\r\n", ch);
		return;
	};
	if (!(ch->IsNpc()
		|| GET_EQ(ch, kShield)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("Вы не можете сделать это без щита.\r\n", ch);
		return;
	}
	if (ch->battle_affects.get(kEafBlock)) {
		SendMsgToChar("Вы уже прикрываетесь щитом!\r\n", ch);
		return;
	}
	go_block(ch);
}

void ProcessShieldBlock(CharData *ch, CharData *victim, HitData &hit_data) {
	if (!CanPerformShieldBlock(victim, hit_data)) {
		return;
	}
	if (!(GET_EQ(victim, EEquipPos::kShield) || victim->IsNpc() || IS_IMMORTAL(victim))) {
		SendMsgToChar("У вас нечем отразить атаку противника.\r\n", victim);
	} else {
		int range = number(1, MUD::Skill(ESkill::kShieldBlock).difficulty);
		int prob = CalcCurrentSkill(victim, ESkill::kShieldBlock, ch);
		++(victim->battle_counter);
		TrainSkill(victim, ESkill::kShieldBlock, prob > 99, ch);
		SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kShieldBlock).name, range, prob, prob > 99);
		prob = prob * 100 / range;
		if (hit_data.GetFlags()[fight::kCritLuck]) {
			prob = 0;
		}
		if (prob < 100) {
			act("Вы не смогли отразить атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N не сумел$G отразить вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n не сумел$g отразить атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
		} else if (prob < 150) {
			act("Вы немного отразили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N немного отразил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n немного отразил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, EEquipPos::kShield, hit_data.dam, 10);
			hit_data.dam *= 10 / 15;
		} else if (prob < 250) {
			act("Вы частично отразили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N частично отразил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n частично отразил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, EEquipPos::kShield, hit_data.dam, 15);
			hit_data.dam /= 2;
		} else {
			act("Вы полностью отразили атаку $N1.", false, victim, nullptr, ch, kToChar);
			act("$N полностью отразил$G вашу атаку.", false, ch, nullptr, victim, kToChar);
			act("$n полностью отразил$g атаку $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
			DamageEquipment(victim, EEquipPos::kShield, hit_data.dam, 25);
			hit_data.dam = -1;
		}
	}
}

bool CanPerformShieldBlock(CharData *victim, const HitData &hit_data) {
	return (hit_data.dam > 0
		&& !hit_data.hit_no_parry
		&& ((victim->battle_affects.get(kEafBlock) || CanPerformAutoblock(victim)) && victim->GetPosition() > EPosition::kSit)
		&& !AFF_FLAGGED(victim, EAffect::kStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kStopLeft)
		&& victim->get_wait() <= 0
		&& AFF_FLAGGED(victim, EAffect::kHold) == 0
		&& victim->battle_counter < (GetRealLevel(victim) + 8) / 9);
}

bool CanPerformAutoblock(CharData *ch) {
	return (GET_EQ(ch, EEquipPos::kShield)
		&& ch->battle_affects.get(kEafAwake)
		&& ch->battle_affects.get(kEafAutoblock));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
