#include "expendientcut.h"

#include "abilities/abilities_rollsystem.h"
#include "color.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/pk.h"
#include "game_skills/protect.h"

void ApplyNoFleeAffect(CharData *ch, int duration) {
	Affect<EApply> noflee;
	noflee.type = kSpellExpedientFail;
	noflee.bitvector = to_underlying(EAffect::kNoFlee);
	noflee.location = EApply::kNone;
	noflee.modifier = 0;
	noflee.duration = CalcDuration(ch, duration, 0, 0, 0, 0);;
	noflee.battleflag = kAfBattledec | kAfPulsedec;
	ImposeAffect(ch, noflee, true, false, true, false);
	send_to_char("Вы выпали из ритма боя.\r\n", ch);
}

void PerformCutSuccess(AbilitySystem::TechniqueRoll &roll) {
	act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.",
		false, roll.GetActor(), nullptr, roll.GetRival(), kToVict);
	act("$n сделал$g неуловимое движение, сместившись за спину $N1.",
		true, roll.GetActor(), nullptr, roll.GetRival(), kToNotVict | kToArenaListen);
	Affect<EApply> cut;
	cut.type = kSpellBattle;
	cut.bitvector = to_underlying(EAffect::kHaemorrhage);
	cut.location = EApply::kResistVitality;
	cut.modifier = -std::min(25, number(1, roll.GetActorRating())/12) - (roll.IsCriticalSuccess() ? 10 : 0);
	cut.duration = CalcDuration(roll.GetActor(), 3*number(2, 4), 0, 0, 0, 0);;
	cut.battleflag = kAfBattledec | kAfPulsedec;
	ImposeAffect(roll.GetRival(), cut, false, true, false, true);
}

void PerformCutFail(AbilitySystem::TechniqueRoll &roll) {
	act("Ваши свистящие удары пропали втуне, не задев $N3.",
		false, roll.GetActor(), nullptr, roll.GetRival(), kToChar);
	if (roll.IsCriticalFail()) {
		send_to_char(roll.GetActor(), "%sВы поскользнулись и потеряли равновесие.%s", BWHT, KNRM);
		act("$n поскользнул$u и потерял$g равновесие.",
			false, roll.GetActor(), nullptr, roll.GetRival(), kToVict);
		act("$n поскользнул$u и потерял$g равновесие.",
			true, roll.GetActor(), nullptr, roll.GetRival(), kToNotVict | kToArenaListen);
		SetWait(roll.GetActor(), 2, false);
	};
}

void GoExpedientCut(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	}

	vict = TryToFindProtector(vict, ch);

	AbilitySystem::TechniqueRoll roll;
	roll.Init(ch, EFeat::kCutting, vict);

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
		damage.flags.set(fight::kIgnoreFireShield);
		if (roll.IsCriticalSuccess()) {
			send_to_char("&GТочно в становую жилу!&n\r\n", roll.GetActor());
			damage.flags.set(fight::kCritHit);
		};
		no_flee_duration = 2;
	} else {
		PerformCutFail(roll);
		dmg = fight::kZeroDmg;
		no_flee_duration = 3;
	};
	damage.dam = dmg;
	damage.wielded = GET_EQ(ch, EEquipPos::kWield);
	damage.Process(roll.GetActor(), roll.GetRival());
	damage.dam = dmg;
	damage.wielded = GET_EQ(ch, EEquipPos::kHold);
	damage.Process(roll.GetActor(), roll.GetRival());
	ApplyNoFleeAffect(ch, no_flee_duration);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
}

void SetExtraAttackCut(CharData *ch, CharData *victim) {
	if (!pk_agro_action(ch, victim)) {
		return;
	}
	if (!ch->GetEnemy()) {
		act("Ваше оружие свистнуло, когда вы бросились на $N3, применив \"порез\".",
			false, ch, nullptr, victim, kToChar);
		SetFighting(ch, victim);
		ch->SetExtraAttack(kExtraAttackCut, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.", false, ch, nullptr, victim, kToChar);
		ch->SetExtraAttack(kExtraAttackCut, victim);
	}
}

void DoExpedientCut(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {

	if (ch->IsNpc() || (!IsAbleToUseFeat(ch, EFeat::kCutting) && !IS_IMPL(ch))) {
		send_to_char("Вы не владеете таким приемом.\r\n", ch);
		return;
	}

	if (ch->IsHorsePrevents()) {
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kStopRight) || IsUnableToAct(ch)) {
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

	if (ch->GetEnemy() && vict->GetEnemy() != ch) {
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
