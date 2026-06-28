#include "turnundead.h"
#include "gameplay/mechanics/minions.h"
#include "skill_messages.h"
#include "engine/db/global_objects.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"
#include "engine/core/target_resolver.h"
#include "gameplay/abilities/abilities_rollsystem.h"
#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "engine/ui/cmd/do_flee.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/damage.h"

//using namespace abilities;

void do_turn_undead(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {

	if (!GetSkill(ch, ESkill::kTurnUndead)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kTurnUndead, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}
	if (ch->Skills().HasActiveCooldown(ESkill::kTurnUndead)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kTurnUndead, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	};

	int skill = GetSkill(ch, ESkill::kTurnUndead);
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
	target_resolver::FoesRosterType roster{ch, [](CharData *, CharData *target) { return target->IsFlagged(EMobFlag::kUndead); }};
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
		} else if (roll.IsCriticalFail() && !IsCharmice(target)) {
			act("&BВаши жалкие лучи света лишь привели $n3 в ярость!\r\n&n",
				false, target, nullptr, ch, kToVict);
			act("&BЧахлый луч света $N1 лишь привел $n3 в ярость!\r\n&n",
				false, target, nullptr, ch, kToNotVict | kToArenaListen);
			Affect<EApply> af1;
			af1.duration = CalcDuration(target, target, ESkill::kUndefined, 3, 0, 0, 0);
			af1.modifier = std::max(1, roll.GetSuccessDegree() * 2);
			af1.location = EApply::kDamroll;
			af1.affect_type = EAffect::kFrenzy;
			af1.battleflag = 0;
			Affect<EApply> af2;
			af2.duration = CalcDuration(target, target, ESkill::kUndefined, 3, 0, 0, 0);
			af2.modifier = std::max(1, 25 + roll.GetSuccessDegree() * 5);
			af2.location = EApply::kHpRegen;
			af2.affect_type = EAffect::kFrenzy;
			af2.battleflag = 0;
			// issue.affects-improve: the enraged undead cannot flee (EApply::kBind), replacing the kNoFlee affect.
			Affect<EApply> af3;
			af3.duration = CalcDuration(target, target, ESkill::kUndefined, 3, 0, 0, 0);
			af3.modifier = 1;
			af3.location = EApply::kBind;
			af3.affect_type = EAffect::kFrenzy;
			af3.battleflag = 0;
			ImposeAffect(target, af1, true, false, true, false);
			ImposeAffect(target, af2, true, false, true, false);
			ImposeAffect(target, af3, true, false, true, false);
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
