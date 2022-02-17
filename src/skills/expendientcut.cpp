//
// Created by ubuntu on 03/09/20.
//

#include "expendientcut.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/pk.h"
#include "skills/protect.h"
//#include <cmath>

ESkill GetExpedientWeaponSkill(CharData *ch) {
	ESkill skill = ESkill::kPunch;

	if (GET_EQ(ch, WEAR_WIELD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == CObjectPrototype::ITEM_WEAPON)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
	} else if (GET_EQ(ch, WEAR_BOTHS) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == CObjectPrototype::ITEM_WEAPON)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
	} else if (GET_EQ(ch, WEAR_HOLD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == CObjectPrototype::ITEM_WEAPON)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD));
	};

	return skill;
}

int GetExpedientKeyParameter(CharData *ch, ESkill skill) {
	switch (skill) {
		case ESkill::kPunch:
		case ESkill::kClubs:
		case ESkill::kAxes:
		case ESkill::kTwohands:
			[[fallthrough]];
		case ESkill::kSpades: return ch->get_str();
		case ESkill::kLongBlades:
		case ESkill::kShortBlades:
		case ESkill::kNonstandart:
		case ESkill::kBows:
			[[fallthrough]];
		case ESkill::kPicks: return ch->get_dex();
		default: return ch->get_str();
	}
}

int CalcParameterBonus(int parameter) {
	return ((parameter - 20) / 4);
}

int CalcExpedientRating(CharData *ch, ESkill skill) {
	return floor(ch->get_skill(skill) / 2.00 + CalcParameterBonus(GetExpedientKeyParameter(ch, skill)));
}

int CalcExpedientCap(CharData *ch, ESkill skill) {
	if (!IS_NPC(ch)) {
		return floor(1.33 * (CalcSkillRemortCap(ch) / 2.00 + CalcParameterBonus(GetExpedientKeyParameter(ch, skill))));
	} else {
		return floor(1.33 * ((kSkillCapOnZeroRemort + 5 * MAX(0, GET_REAL_LEVEL(ch) - 30) / 2.00
			+ CalcParameterBonus(GetExpedientKeyParameter(ch, skill)))));
	}
}

int CalcSuccessDegree(int roll, int rating) {
	return ((rating - roll) / 5);
}

bool CheckExpedientSuccess(CharData *ch, CharData *victim) {
	ESkill actor_skill = GetExpedientWeaponSkill(ch);
	int actor_rating = CalcExpedientRating(ch, actor_skill);
	int actor_cap = CalcExpedientCap(ch, actor_skill);
	int actor_roll = RollDices(1, actor_cap);
	int actor_success_degree = CalcSuccessDegree(actor_roll, actor_rating);

	ESkill victim_skill = GetExpedientWeaponSkill(victim);
	int victim_rating = CalcExpedientRating(victim, victim_skill);
	int victim_cap = CalcExpedientCap(victim, victim_skill);
	int victim_roll = RollDices(1, victim_cap);
	int victim_success_degree = CalcSuccessDegree(victim_roll, victim_rating);

	if ((actor_roll <= actor_rating) && (victim_roll > victim_rating))
		return true;
	if ((actor_roll > actor_rating) && (victim_roll <= victim_rating))
		return false;
	if ((actor_roll > actor_rating) && (victim_roll > victim_rating))
		return CheckExpedientSuccess(ch, victim);

	if (actor_success_degree > victim_success_degree)
		return true;
	if (actor_success_degree < victim_success_degree)
		return false;

	if (CalcParameterBonus(GetExpedientKeyParameter(ch, actor_skill))
		> CalcParameterBonus(GetExpedientKeyParameter(victim, victim_skill)))
		return true;
	if (CalcParameterBonus(GetExpedientKeyParameter(ch, actor_skill))
		< CalcParameterBonus(GetExpedientKeyParameter(victim, victim_skill)))
		return false;

	if (actor_roll < victim_roll)
		return true;
	if (actor_roll > victim_roll)
		return true;

	return CheckExpedientSuccess(ch, victim);
}

void ApplyNoFleeAffect(CharData *ch, int duration) {
	//При простановке сразу 2 флагов битвектора начинаются глюки
	//По-видимому, где-то не учтено, что ненулевых битов может быть более одного
	Affect<EApplyLocation> noflee;
	noflee.type = kSpellBattle;
	noflee.bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
	noflee.location = EApplyLocation::APPLY_NONE;
	noflee.modifier = 0;
	noflee.duration = CalcDuration(ch, duration, 0, 0, 0, 0);;
	noflee.battleflag = kAfBattledec | kAfPulsedec;
	affect_join(ch, noflee, true, false, true, false);

	Affect<EApplyLocation> battle;
	battle.type = kSpellBattle;
	battle.bitvector = to_underlying(EAffectFlag::AFF_EXPEDIENT);
	battle.location = EApplyLocation::APPLY_NONE;
	battle.modifier = 0;
	battle.duration = CalcDuration(ch, duration, 0, 0, 0, 0);;
	battle.battleflag = kAfBattledec | kAfPulsedec;
	affect_join(ch, battle, true, false, true, false);

	send_to_char("Вы выпали из ритма боя.\r\n", ch);
}

void GoCutShorts(CharData *ch, CharData *vict) {

	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_EXPEDIENT)) {
		send_to_char("Вы еще не восстановили силы после предыдущего приема.\r\n", ch);
		return;
	}

	vict = TryToFindProtector(vict, ch);

	if (!CheckExpedientSuccess(ch, vict)) {
		act("Ваши свистящие удары пропали втуне, не задев $N3.",
			false, ch, nullptr, vict, kToChar);
		Damage dmg(SkillDmg(ESkill::kShortBlades), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.skill_id = ESkill::kUndefined;
		dmg.process(ch, vict);
		ApplyNoFleeAffect(ch, 2);
		return;
	}

	act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.",
		false, ch, nullptr, vict, kToVict);
	act("$n сделал$g неуловимое движение, сместившись за спину $N1.",
		true, ch, nullptr, vict, kToNotVict | kToArenaListen);
	hit(ch, vict, ESkill::kUndefined, fight::kMainHand);
	hit(ch, vict, ESkill::kUndefined, fight::kOffHand);

	Affect<EApplyLocation> immun_physic;
	immun_physic.type = kSpellExpedient;
	immun_physic.location = EApplyLocation::APPLY_PR;
	immun_physic.modifier = 100;
	immun_physic.duration = CalcDuration(ch, 3, 0, 0, 0, 0);
	immun_physic.battleflag = kAfBattledec | kAfPulsedec;
	affect_join(ch, immun_physic, false, false, false, false);

	Affect<EApplyLocation> immun_magic;
	immun_magic.type = kSpellExpedient;
	immun_magic.location = EApplyLocation::APPLY_MR;
	immun_magic.modifier = 100;
	immun_magic.duration = CalcDuration(ch, 3, 0, 0, 0, 0);
	immun_magic.battleflag = kAfBattledec | kAfPulsedec;
	affect_join(ch, immun_magic, false, false, false, false);

	ApplyNoFleeAffect(ch, 3);
}

void SetExtraAttackCutShorts(CharData *ch, CharData *victim) {
	if (!IsHaveNoExtraAttack(ch)) {
		return;
	}

	if (!pk_agro_action(ch, victim)) {
		return;
	}

	if (!ch->get_fighting()) {
		act("Ваше оружие свистнуло, когда вы бросились на $N3, применив \"порез\".",
			false, ch, nullptr, victim, kToChar);
		set_fighting(ch, victim);
		ch->set_extra_attack(kExtraAttackCutShorts, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.", false, ch, nullptr, victim, kToChar);
		ch->set_extra_attack(kExtraAttackCutShorts, victim);
	}
}

void SetExtraAttackCutPick(CharData *ch, CharData *victim) {
	if (!IsHaveNoExtraAttack(ch)) {
		return;
	}
	if (!pk_agro_action(ch, victim)) {
		return;
	}

	if (!ch->get_fighting()) {
		act("Вы перехватили оружие обратным хватом и проскользнули за спину $N1.",
			false, ch, nullptr, victim, kToChar);
		set_fighting(ch, victim);
		ch->set_extra_attack(kExtraAttackPick, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.",
			false, ch, nullptr, victim, kToChar);
		ch->set_extra_attack(kExtraAttackPick, victim);
	}
}

ESkill GetExpedientCutSkill(CharData *ch) {
	ESkill skill{ESkill::kIncorrect};

	if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
		if (skill != static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD))) {
			send_to_char("Для этого приема в обеих руках нужно держать оружие одого типа!\r\n", ch);
			return ESkill::kIncorrect;
		}
	} else if (GET_EQ(ch, WEAR_BOTHS)) {
		skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
	} else {
		send_to_char("Для этого приема вам надо использовать одинаковое оружие в обеих руках либо двуручное.\r\n", ch);
		return ESkill::kIncorrect;
	}

	if (!can_use_feat(ch, FindWeaponMasterFeat(skill)) && !IS_IMPL(ch)) {
		send_to_char("Вы недостаточно искусны в обращении с этим видом оружия.\r\n", ch);
		return ESkill::kIncorrect;
	}

	return skill;
}

void DoExpedientCut(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {

	ESkill skill;

	if (IS_NPC(ch) || (!can_use_feat(ch, EXPEDIENT_CUT_FEAT) && !IS_IMPL(ch))) {
		send_to_char("Вы не владеете таким приемом.\r\n", ch);
		return;
	}

	if (ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	if (!IsHaveNoExtraAttack(ch)) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите порезать?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы таки да? Ой-вей, но тут Древняя Русь, а не Палестина!\r\n", ch);
		return;
	}
	if (ch->get_fighting() && vict->get_fighting() != ch) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, nullptr, vict, kToChar);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	skill = GetExpedientCutSkill(ch);
	if (skill == ESkill::kIncorrect)
		return;

	switch (skill) {
		case ESkill::kShortBlades:
			[[fallthrough]];
		case ESkill::kSpades:
			SetExtraAttackCutShorts(ch, vict);
			break;
		case ESkill::kPicks:
			SetExtraAttackCutPick(ch, vict);
			break;
		case ESkill::kLongBlades:
			[[fallthrough]];
		case ESkill::kTwohands:
			send_to_char("Порез мечом (а тем паче - двуручником или копьем) - это сурьезно. Но пока невозможно.\r\n",
						 ch);
			break;
		default:send_to_char("Ваше оружие не позволяет провести такой прием.\r\n", ch);
	}

}
