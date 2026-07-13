#include "expendientcut.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "skill_messages.h"
#include "engine/db/global_objects.h"

#include "gameplay/abilities/abilities_rollsystem.h"
#include "engine/ui/color.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/pk.h"
#include "protect.h"
#include "gameplay/mechanics/damage.h"

void ApplyNoFleeAffect(CharData *ch, int duration) {
	Affect<EApply> noflee;
	noflee.affect_type = EAffect::kNoFlee;
	noflee.location = EApply::kNone;
	noflee.modifier = 0;
	noflee.duration = CalcDuration(ch, ch, ESkill::kUndefined, duration, 0, 0, 0);;
	noflee.battleflag = kAfBattledec | kAfPulsedec;
	ImposeAffect(ch, noflee, true, false, true, false);
	SendMsgToChar("Вы выпали из ритма боя.\r\n", ch);
}

void ApplyDebuffs(abilities_roll::TechniqueRoll &roll) {
	Affect<EApply> cut;
	cut.duration = CalcDuration(roll.GetActor(), roll.GetActor(), ESkill::kUndefined, 3 * number(2, 4), 0, 0, 0);;
	cut.battleflag = kAfBattledec;
	if (roll.GetActor()->IsFlagged(EPrf::kPerformSerratedBlade)) {
		cut.modifier = 1;
		cut.affect_type = EAffect::kLacerations;
		cut.location = EApply::kNone;
	} else {
		cut.modifier = -std::min(25, number(1, roll.GetActorRating()) / 10) - (roll.IsCriticalSuccess() ? 10 : 0);
		cut.affect_type = EAffect::kHaemorrhage;
		cut.location = EApply::kResistVitality;
	}
	ImposeAffect(roll.GetRival(), cut, false, true, false, true);
}

void PerformCutSuccess(abilities_roll::TechniqueRoll &roll) {
	act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.",
		false, roll.GetActor(), nullptr, roll.GetRival(), kToVict);
	act("$n сделал$g неуловимое движение, сместившись за спину $N1.",
		true, roll.GetActor(), nullptr, roll.GetRival(), kToNotVict | kToArenaListen);
	if (!roll.GetRival()->IsFlagged(EMobFlag::kUndead) && GET_RACE(roll.GetRival()) != ENpcRace::kConstruct) {
		ApplyDebuffs(roll);
	}
}

void PerformCutFail(abilities_roll::TechniqueRoll &roll) {
	act("Ваши свистящие удары пропали втуне, не задев $N3.",
		false, roll.GetActor(), nullptr, roll.GetRival(), kToChar);
	if (roll.IsCriticalFail()) {
		SendMsgToChar(roll.GetActor(), "%sВы поскользнулись и потеряли равновесие.%s", kColorGry, kColorNrm);
		act("$n поскользнул$u и потерял$g равновесие.",
			false, roll.GetActor(), nullptr, roll.GetRival(), kToVict);
		act("$n поскользнул$u и потерял$g равновесие.",
			true, roll.GetActor(), nullptr, roll.GetRival(), kToNotVict | kToArenaListen);
		SetWait(roll.GetActor(), 2, false);
	};
}

void GoExpedientCut(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}

	if (ch->Skills().HasActiveCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	}
//	if (vict->purged()) {
//		return;
//	}

	vict = TryToFindProtector(vict, ch);

	abilities_roll::TechniqueRoll roll;
	roll.Init(ch, abilities::EAbility::kCutting, vict);

	if (roll.IsWrongConditions()) {
		roll.SendDenyMsgToActor();
		return;
	};

	Damage damage(SkillDmg(ESkill::kCutting), fight::kZeroDmg, fight::kPhysDmg, nullptr);
	int no_flee_duration;
	int dmg;
	if (roll.IsSuccess()) {
		PerformCutSuccess(roll);
		dmg = roll.CalcDamage();
		damage.flags.set(fight::kIgnoreFireShield);
		if (roll.IsCriticalSuccess()) {
			SendMsgToChar("&GТочно в становую жилу!&n\r\n", roll.GetActor());
			damage.flags.set(fight::kCritHit);
		};
		no_flee_duration = 2;
	} else {
		PerformCutFail(roll);
		dmg = fight::kZeroDmg;
		no_flee_duration = 3;
	};
	damage.dam = dmg;
	damage.flags.set(fight::kIgnoreBlink);
	damage.wielded = GET_EQ(ch, EEquipPos::kWield);
	damage.Process(roll.GetActor(), roll.GetRival());
	if (roll.GetActor()->in_room == roll.GetRival()->in_room) {
		damage.dam = dmg;
		damage.wielded = GET_EQ(ch, EEquipPos::kHold);
		damage.Process(roll.GetActor(), roll.GetRival());
		ApplyNoFleeAffect(ch, no_flee_duration);
	}
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
}

void SetExtraAttackCut(CharData *ch, CharData *victim) {
	if (!pk_agro_action(ch, victim)) {
		return;
	}
	if (!ch->GetEnemy()) {
		act("Вы решили порезать $N3 ровными кусочками.",
			false, ch, nullptr, victim, kToChar);
		SetFighting(ch, victim);
		ch->SetExtraAttack(kExtraAttackCut, victim);
	} else {
		act("Хорошо. Вы попытаетесь порезать $N3.", false, ch, nullptr, victim, kToChar);
		ch->SetExtraAttack(kExtraAttackCut, victim);
	}
}

void DoExpedientCut(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	if (ch->IsNpc() || (!CanUseFeat(ch, EFeat::kCutting) && !privilege::IsImpl(ch))) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}
	if (mount::IsBlockedByHorse(ch)) {
		return;
	}
	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kGetOnFeet) + "\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}
	if (vict == ch) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCutting, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}
        if (ch->GetEnemy()) {
        if (ch->GetEnemy() != vict && vict->GetEnemy() != ch) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, nullptr, vict, kToChar);
		return;
}
	}
	if (!may_kill_here(ch, vict, argument) || !check_pkill(ch, vict, arg)) {
		return;
	}
	if (!IsHaveNoExtraAttack(ch)) {
		return;
	}
	SetExtraAttackCut(ch, vict);
}
