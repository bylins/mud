#include "turnundead.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/common.h"
#include "action_targeting.h"
#include "abilities/abilities_rollsystem.h"
#include "handler.h"
#include "cmd/flee.h"

using namespace AbilitySystem;

void do_turn_undead(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {

	if (!ch->get_skill(ESkill::kTurnUndead)) {
		send_to_char("Вам это не по силам.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kTurnUndead)) {
		send_to_char("Вам нужно набраться сил для применения этого навыка.\r\n", ch);
		return;
	};

	int skill = ch->get_skill(ESkill::kTurnUndead);
	TimedSkill timed;
	timed.skill = ESkill::kTurnUndead;
	if (can_use_feat(ch, EXORCIST_FEAT)) {
		timed.time = IsTimedBySkill(ch, ESkill::kTurnUndead) + kHoursPerTurnUndead - 2;
		skill += 10;
	} else {
		timed.time = IsTimedBySkill(ch, ESkill::kTurnUndead) + kHoursPerTurnUndead;
	}
	if (timed.time > kHoursPerDay) {
		send_to_char("Вам пока не по силам изгонять нежить, нужно отдохнуть.\r\n", ch);
		return;
	}
	timed_to_char(ch, &timed);

	send_to_char(ch, "Вы свели руки в магическом жесте и отовсюду хлынули яркие лучи света.\r\n");
	act("$n свел$g руки в магическом жесте и отовсюду хлынули яркие лучи света.\r\n",
		false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

// костылиии... и магик намберы
	int victims_amount = 20;
	int victims_hp_amount = skill * 25 + std::max(0, skill - 80) * 50;
	Damage damage(SkillDmg(ESkill::kTurnUndead), fight::kZeroDmg, fight::kMagicDmg, nullptr);
	damage.magic_type = kTypeLight;
	damage.flags.set(fight::kIgnoreFireShield);
	TechniqueRoll roll;
	ActionTargeting::FoesRosterType roster{ch, [](CharData *, CharData *target) { return IS_UNDEAD(target); }};
	for (const auto target : roster) {
		damage.dam = fight::kZeroDmg;
		roll.Init(ch, TURN_UNDEAD_FEAT, target);
		if (roll.IsSuccess()) {
			if (roll.IsCriticalSuccess() && GetRealLevel(ch) > target->get_level() + RollDices(1, 5)) {
				send_to_char(ch, "&GВы окончательно изгнали %s из мира!&n\r\n", GET_PAD(target, 3));
				damage.dam = std::max(1, GET_HIT(target) + 11);
			} else {
				damage.dam = roll.CalcDamage();
				victims_hp_amount -= damage.dam;
			};
		} else if (roll.IsCriticalFail() && !IS_CHARMICE(target)) {
			act("&BВаши жалкие лучи света лишь привели $n3 в ярость!\r\n&n",
				false, target, nullptr, ch, kToVict);
			act("&BЧахлый луч света $N1 лишь привел $n3 в ярость!\r\n&n",
				false, target, nullptr, ch, kToNotVict | kToArenaListen);
			Affect<EApplyLocation> af1;
			af1.type = kSpellCourage;
			af1.duration = CalcDuration(target, 3, 0, 0, 0, 0);
			af1.modifier = MAX(1, roll.GetSuccessDegree() * 2);
			af1.location = APPLY_DAMROLL;
			af1.bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			af1.battleflag = 0;
			Affect<EApplyLocation> af2;
			af2.type = kSpellCourage;
			af2.duration = CalcDuration(target, 3, 0, 0, 0, 0);
			af2.modifier = MAX(1, 25 + roll.GetSuccessDegree() * 5);
			af2.location = APPLY_HITREG;
			af2.bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			af2.battleflag = 0;
			affect_join(target, af1, true, false, true, false);
			affect_join(target, af2, true, false, true, false);
		};
		damage.Process(ch, target);
		if (!target->purged() && roll.IsSuccess() && !MOB_FLAGGED(target, MOB_NOFEAR)
			&& !CalcGeneralSaving(ch, target, ESaving::kWill, GET_REAL_WIS(ch) + GET_REAL_INT(ch))) {
			go_flee(target);
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
