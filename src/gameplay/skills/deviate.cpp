/**
\file deviate.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "deviate.h"

#include "engine/entities/char_data.h"
#include "gameplay/fight/common.h"
#include "engine/db/global_objects.h"

bool CanPerformDeviate(CharData *victim, const HitData &hit_data);

void DoDeviate(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kDodge)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kDodge)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!(ch->GetEnemy())) {
		SendMsgToChar("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}

	if (ch->IsHorsePrevents()) {
		return;
	}

	if (ch->battle_affects.get(kEafDodge)) {
		SendMsgToChar("Вы и так вертитесь, как волчок.\r\n", ch);
		return;
	};
	GoDeviate(ch);
}

void GoDeviate(CharData *ch) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->IsHorsePrevents()) {
		return;
	};
	ch->battle_affects.set(kEafDodge);
	SendMsgToChar("Хорошо, вы попытаетесь уклониться от следующей атаки!\r\n", ch);
}

void ProcessDeviate(CharData *ch, CharData *victim, HitData &hit_data) {
	if (!CanPerformDeviate(victim, hit_data)) {
		return;
	}
	int range = number(1, MUD::Skill(ESkill::kDodge).difficulty);
	int prob = CalcCurrentSkill(victim, ESkill::kDodge, ch);
	if (GET_GOD_FLAG(victim, EGf::kGodscurse)) {
		prob = 0;
	}
	prob = prob * 100 / range;
	if (IsAffectedBySpell(victim, ESpell::kWeb)) {
		prob /= 3;
	}
	TrainSkill(victim, ESkill::kDodge, prob < 100, ch);
	if (hit_data.GetFlags()[fight::kCritLuck]) {
		prob = 0;
	}
	if (prob < 60) {
		act("Вы не смогли уклониться от атаки $N1.", false, victim, nullptr, ch, kToChar);
		act("$N не сумел$G уклониться от вашей атаки.", false, ch, nullptr, victim, kToChar);
		act("$n не сумел$g уклониться от атаки $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
		victim->battle_affects.set(kEafDodge);
	} else if (prob < 100) {
		act("Вы немного уклонились от атаки $N1.", false, victim, nullptr, ch, kToChar);
		act("$N немного уклонил$U от вашей атаки.", false, ch, nullptr, victim, kToChar);
		act("$n немного уклонил$u от атаки $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
		hit_data.dam *= 10/15;
		victim->battle_affects.set(kEafDodge);
	} else if (prob < 200) {
		act("Вы частично уклонились от атаки $N1.", false, victim, nullptr, ch, kToChar);
		act("$N частично уклонил$U от вашей атаки.", false, ch, nullptr, victim, kToChar);
		act("$n частично уклонил$u от атаки $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
		hit_data.dam /= 2;
		victim->battle_affects.set(kEafDodge);
	} else {
		act("Вы уклонились от атаки $N1.", false, victim, nullptr, ch, kToChar);
		act("$N уклонил$U от вашей атаки.", false, ch, nullptr, victim, kToChar);
		act("$n уклонил$u от атаки $N1.", true, victim, nullptr, ch, kToNotVict | kToArenaListen);
		hit_data.dam = -1;
		victim->battle_affects.set(kEafDodge);
	}
	++(victim->battle_counter);
}

bool CanPerformDeviate(CharData *victim, const HitData &hit_data) {
	return (hit_data.dam > 0
		&& !hit_data.hit_no_parry
		&& victim->battle_affects.get(kEafDodge)
		&& victim->get_wait() <= 0
		&& !AFF_FLAGGED(victim, EAffect::kStopFight)
		&& !AFF_FLAGGED(victim, EAffect::kMagicStopFight)
		&& AFF_FLAGGED(victim, EAffect::kHold) == 0
		&& victim->battle_counter < (GetRealLevel(victim) + 7) / 8);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
