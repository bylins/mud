#include "expendientcut.h"

#include "abilities/abilities_rollsystem.h"
#include "color.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/pk.h"
#include "skills/protect.h"

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

void PerformCutSuccess(AbilitySystem::TechniqueRoll &roll) {
	act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.",
		false, roll.GetActor(), nullptr, roll.GetRival(), kToVict);
	act("$n сделал$g неуловимое движение, сместившись за спину $N1.",
		true, roll.GetActor(), nullptr, roll.GetRival(), kToNotVict | kToArenaListen);
	Affect<EApplyLocation> immun_physic;
	immun_physic.type = kSpellExpedient;
	immun_physic.location = EApplyLocation::APPLY_PR;
	immun_physic.modifier = 100;
	immun_physic.duration = CalcDuration(roll.GetActor(), 3, 0, 0, 0, 0);
	immun_physic.battleflag = kAfBattledec | kAfPulsedec;
	affect_join(roll.GetActor(), immun_physic, false, false, false, false);
	Affect<EApplyLocation> immun_magic;
	immun_magic.type = kSpellExpedient;
	immun_magic.location = EApplyLocation::APPLY_MR;
	immun_magic.modifier = 100;
	immun_magic.duration = CalcDuration(roll.GetActor(), 3, 0, 0, 0, 0);
	immun_magic.battleflag = kAfBattledec | kAfPulsedec;
	affect_join(roll.GetActor(), immun_magic, false, false, false, false);
}

void PerformCutFail(AbilitySystem::TechniqueRoll &roll) {
	act("Ваши свистящие удары пропали втуне, не задев $N3.",
		false, roll.GetActor(), nullptr, roll.GetRival(), kToChar);
	// Уберем пока критфейл
/*	if (roll.IsCriticalFail()) {
		send_to_char(roll.GetActor(), "%sВы поскользнулись и упали упали!%s", BWHT, KNRM);
		act(buf, false, roll.GetActor(), nullptr, roll.GetRival(), kToChar);
		act("$n поскользнул$u и упал$g.", false, roll.GetActor(), nullptr, roll.GetRival(), kToVict);
		act("$n поскользнул$u и упал$g.", true, roll.GetActor(), nullptr, roll.GetRival(), kToNotVict | kToArenaListen);
		GET_POS(roll.GetActor()) = EPosition::kSit;
	};*/
}

void GoExpedientCut(CharData *ch, CharData *vict) {

	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_EXPEDIENT)) {
		send_to_char("Вы еще не восстановили силы после предыдущего приема.\r\n", ch);
		return;
	}

	vict = TryToFindProtector(vict, ch);

	AbilitySystem::TechniqueRoll roll;
	roll.Init(ch, EXPEDIENT_CUT_FEAT, vict);

	if (roll.IsWrongConditions()) {
		roll.SendDenyMsgToActor();
		return;
	};

	Damage damage(SkillDmg(roll.GetBaseSkill()), fight::kZeroDmg, fight::kPhysDmg, nullptr);
	int no_flee_duration;
	int dmg;
	if (roll.IsSuccess()) {
		PerformCutSuccess(roll);
		dmg = roll.CalcDamage();
		if (roll.IsCriticalSuccess()) {
			send_to_char("&GТочно в становую жилу!&n\r\n", roll.GetActor());
			damage.flags.set(fight::IGNORE_ABSORBE);
			damage.flags.set(fight::CRIT_HIT);
		};
		no_flee_duration = 3;
	} else {
		PerformCutFail(roll);
		dmg = fight::kZeroDmg;
		no_flee_duration = 2;
	};

	damage.dam = dmg;
	damage.wielded = GET_EQ(ch, WEAR_WIELD);
	damage.Process(roll.GetActor(), roll.GetRival());
	damage.dam = dmg;
	damage.wielded = GET_EQ(ch, WEAR_HOLD);
	damage.Process(roll.GetActor(), roll.GetRival());
	ApplyNoFleeAffect(ch, no_flee_duration);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);

/*
	hit(ch, vict, ESkill::kUndefined, fight::kMainHand);
	hit(ch, vict, ESkill::kUndefined, fight::kOffHand);
*/
}

void SetExtraAttackCut(CharData *ch, CharData *victim) {
	if (!pk_agro_action(ch, victim)) {
		return;
	}
	if (!ch->get_fighting()) {
		act("Ваше оружие свистнуло, когда вы бросились на $N3, применив \"порез\".",
			false, ch, nullptr, victim, kToChar);
		set_fighting(ch, victim);
		ch->set_extra_attack(kExtraAttackCut, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.", false, ch, nullptr, victim, kToChar);
		ch->set_extra_attack(kExtraAttackCut, victim);
	}
}

void DoExpedientCut(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {

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

	if (!may_kill_here(ch, vict, argument) || !check_pkill(ch, vict, arg)) {
		return;
	}

	if (!IsHaveNoExtraAttack(ch)) {
		return;
	}

	SetExtraAttackCut(ch, vict);
}
