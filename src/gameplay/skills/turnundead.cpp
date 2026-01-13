#include "turnundead.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"
#include "engine/core/action_targeting.h"
#include "gameplay/abilities/abilities_rollsystem.h"
#include "engine/core/handler.h"
#include "engine/ui/cmd/do_flee.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/damage.h"

//using namespace abilities;

void do_turn_undead(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {

	if (!ch->GetSkill(ESkill::kTurnUndead)) {
		SendMsgToChar("Вам это не по силам.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kTurnUndead)) {
		SendMsgToChar("Вам нужно набраться сил для применения этого навыка.\r\n", ch);
		return;
	};

	int skill = ch->GetSkill(ESkill::kTurnUndead);
	TimedSkill timed;
	timed.skill = ESkill::kTurnUndead;
	if (CanUseFeat(ch, EFeat::kExorcist)) {
		timed.time = IsTimedBySkill(ch, ESkill::kTurnUndead) + kHoursPerTurnUndead - 2;
	} else {
		timed.time = IsTimedBySkill(ch, ESkill::kTurnUndead) + kHoursPerTurnUndead;
	}
	if (timed.time > kHoursPerDay) {
		SendMsgToChar("Вам пока не по силам изгонять нежить, нужно отдохнуть.\r\n", ch);
		return;
	}
	ImposeTimedSkill(ch, &timed);

	SendMsgToChar(ch, "Вы свели руки в магическом жесте и отовсюду хлынули яркие лучи света.\r\n");
	act("$n свел$g руки в магическом жесте и отовсюду хлынули яркие лучи света.\r\n",
		false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

// костылиии... и магик намберы
	int victims_amount = 20;
	int victims_hp_amount = skill * 25 + std::max(0, skill - 80) * 50;
	Damage damage(SkillDmg(ESkill::kTurnUndead), fight::kZeroDmg, fight::kMagicDmg, nullptr);
	damage.element = EElement::kLight;
	damage.flags.set(fight::kIgnoreFireShield);
	abilities_roll::TechniqueRoll roll;
	ActionTargeting::FoesRosterType roster{ch, [](CharData *, CharData *target) { return IS_UNDEAD(target); }};
	for (const auto target: roster) {
//		if (target->purged() || target->in_room == kNowhere)
//			continue;
		damage.dam = fight::kZeroDmg;
		roll.Init(ch, abilities::EAbility::kTurnUndead, target);
		if (roll.IsSuccess()) {
			if (roll.IsCriticalSuccess() && GetRealLevel(ch) > target->GetLevel() + RollDices(1, 5)) {
				SendMsgToChar(ch, "&GВы окончательно изгнали %s из мира!&n\r\n", GET_PAD(target, 3));
				damage.dam = std::max(1, target->get_hit() + 11);
			} else {
				damage.dam = roll.CalcDamage();
				victims_hp_amount -= damage.dam;
			};
		} else if (roll.IsCriticalFail() && !IS_CHARMICE(target)) {
			act("&BВаши жалкие лучи света лишь привели $n3 в ярость!\r\n&n",
				false, target, nullptr, ch, kToVict);
			act("&BЧахлый луч света $N1 лишь привел $n3 в ярость!\r\n&n",
				false, target, nullptr, ch, kToNotVict | kToArenaListen);
			Affect<EApply> af1;
			af1.type = ESpell::kCourage;
			af1.duration = CalcDuration(target, 3, 0, 0, 0, 0);
			af1.modifier = std::max(1, roll.GetSuccessDegree() * 2);
			af1.location = EApply::kDamroll;
			af1.bitvector = to_underlying(EAffect::kNoFlee);
			af1.battleflag = 0;
			Affect<EApply> af2;
			af2.type = ESpell::kCourage;
			af2.duration = CalcDuration(target, 3, 0, 0, 0, 0);
			af2.modifier = std::max(1, 25 + roll.GetSuccessDegree() * 5);
			af2.location = EApply::kHpRegen;
			af2.bitvector = to_underlying(EAffect::kNoFlee);
			af2.battleflag = 0;
			ImposeAffect(target, af1, true, false, true, false);
			ImposeAffect(target, af2, true, false, true, false);
		};
		damage.flags.set(fight::kIgnoreBlink);
		damage.Process(ch, target);
		if (!target->purged() && roll.IsSuccess() && !target->IsFlagged(EMobFlag::kNoFear)
			&& !CalcGeneralSaving(ch, target, ESaving::kWill, GetRealWis(ch) + GetRealInt(ch))) {
			GoFlee(target);
		};
		--victims_amount;
		if (victims_amount == 0 || victims_hp_amount <= 0) {
			break;
		};
	};
	//set_wait(ch, 1, true);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	SetSkillCooldownInFight(ch, ESkill::kTurnUndead, 2);
}
