/* ************************************************************************
*   File: magic.cpp                                     Part of Bylins    *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "magic.h"
#include "magic_internal.h"

#include "engine/core/action_targeting.h"
//#include "gameplay/affects/affect_handler.h"
#include "gameplay/affects/affect_data.h"
#include "engine/db/world_characters.h"
#include "engine/ui/cmd/do_hire.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/fight/pk.h"
#include "engine/core/handler.h"
#include "magic_utils.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_player.h"
#include "utils/random.h"
#include "engine/db/global_objects.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/weather.h"
#include "utils/utils_time.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/tutelar.h"

extern int what_sky;
extern int interpolate(int min_value, int pulse);

byte GetSavingThrows(ECharClass class_id, ESaving type, int level);    // class.cpp
byte GetExtendSavingThrows(ECharClass class_id, ESaving save, int level);
int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id);
void ReactToCast(CharData *victim, CharData *caster, ESpell spell_id);

bool ProcessMatComponents(CharData *caster, int, ESpell spell_id);

bool is_room_forbidden(RoomData *room) {
	for (const auto &af: room->affected) {
		if (af->type == ESpell::kForbidden && (number(1, 100) <= af->modifier)) {
			return true;
		}
	}
	return false;
}

// * Saving throws are now in class.cpp as of bpl13.

/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
int bonus_saving[] ={
			-9,-8,-7,-6,-5,-4,-3,-2,-1, 0,
			1, 2, 3 ,4, 5, 6, 7, 8, 9, 10,
			11,12,13,14,15,16,17,18,19,20,
			21,21,22,22,23,23,24,24,25,25,
			26,26,27,27,28,28,29,29,30,30,
			31,31,32,32,33,33,34,34,35,35,
			36,36,37,37,38,38,39,39,40,40,
			41,41,41,42,42,42,43,43,43,44,
			44,44,45,45,45,46,46,46,47,47,
			47,48,48,48,49,49,49,50,50,50,
};

int bonus_antisaving[] ={
			-9,-8,-7,-6,-5,-4,-3,-2,-1, 0,
			1, 2, 3 ,4, 5, 6, 7, 8, 9, 10,
			11,12,13,14,15,16,17,18,19,20,
			21,21,22,22,23,23,24,24,25,25,
			26,26,27,27,28,28,29,29,30,30,
			31,31,32,32,33,33,34,34,35,35,
			36,36,37,37,38,38,39,39,40,40,
			41,41,41,42,42,42,43,43,43,44,
			44,44,45,45,45,46,46,46,47,47,
			47,48,48,48,49,49,49,50,50,50,
};

int CalcAntiSavings(CharData *ch) {
	int modi = 0;
	if (ch->IsImmortal())
		modi = 1000;
	else if (GET_GOD_FLAG(ch, EGf::kGodsLike))
		modi = 250;
	else if (GET_GOD_FLAG(ch, EGf::kGodscurse))
		modi = -250;
	else
		modi = ch->add_abils.cast_success;
	modi += bonus_antisaving[GetRealWis(ch) - 1];
	if (!ch->IsNpc()) {
		modi *= ch->get_cond_penalty(P_CAST);
	}
//  log("[EXT_APPLY] Name==%s modi==%d",GET_NAME(ch), modi);
	return modi;
}

int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id) {
	auto mod = MUD::Class(ch->GetClass()).spells[spell_id].GetCastMod();
	auto skill = ch->GetSkill(GetMagicSkillId(spell_id));
	return static_cast<int>(mod*skill);
}


int GetBasicSave(CharData *ch, ESaving saving, bool log) {
//	std::stringstream ss;
	int save = (100 - GetExtendSavingThrows(ch->GetClass(), saving, GetRealLevel(ch))) * -1; // Базовые спасброски профессии/уровня

//	ss << "Базовый save персонажа " << GET_NAME(ch) << " (" << saving_name.find(saving)->second << "): " << save;
	switch (saving) {
		case ESaving::kReflex:
			save -= bonus_saving[GetRealDex(ch) - 1];
			if (ch->IsOnHorse())
				save += 20;
			break;
		case ESaving::kStability:
			save -= bonus_saving[GetRealCon(ch) - 1];
			if (ch->IsOnHorse()) {
				save -= 20;
				save -= ch->GetSkill(ESkill::kRiding) / 25;
			}
			break;
		case ESaving::kWill:
			save -= bonus_saving[GetRealWis(ch) - 1];
			break;
		case ESaving::kCritical: 
			save -= bonus_saving[GetRealCon(ch) - 1];
			break;
		default:
		break;
	}
//	ss << " с учетом статов: " << save << "\r\n";
	if (log) {
//		ch->send_to_TC(false, true, true, "%s", ss.str().c_str());
//		mudlog(ss.str(), CMP, kLvlImmortal, SYSLOG, true);
	}
	return save;
}

//killer нужен для того чтоб вывести стату
int CalcSaving(CharData *killer, CharData *victim, ESaving saving, bool need_log) {
	auto class_sav = victim->GetClass();
	int save = GetBasicSave(victim, saving, true); //отрицательное лучше

	if (victim->IsNpc()) {
		class_sav = ECharClass::kMob;
	} else {
		if (class_sav < ECharClass::kFirst || class_sav > ECharClass::kLast) {
			class_sav = ECharClass::kWarrior;
		}
	}

	// Учет осторожного стиля
	if (victim->IsFlagged(EPrf::kAwake)) {
		if (CanUseFeat(victim, EFeat::kImpregnable)) {
			save -= std::max(0, victim->GetSkill(ESkill::kAwake) - 80) / 2;
		// справка танцующая: "Осторожный стиль" добавочно увеличивает спас-броски персонажа. почему-то было давно убрано с комментом "фикс осторожки дружинника"
		} else if (CanUseFeat(victim, EFeat::kShadowStrike)) {
			save -= std::max(0, victim->GetSkill(ESkill::kAwake) - 80) / 2.5;
		}

		save -= victim->GetSkill(ESkill::kAwake) / 5; //CalculateSkillAwakeModifier(killer, victim);
	}
	save += round(GetSave(victim, saving) * abilities::kSaveWeight);    // одежда бафы и слава
	if (need_log) {
		killer->send_to_TC(false, true, true,
				"SAVING (%s): Killer==%s  Target==%s vnum==%d Level==%d base_save==%d save_equip==%d save_awake=-%d result_save=%d\r\n",
				saving_name.find(saving)->second.c_str(),
				GET_NAME(killer),
				GET_NAME(victim),
				GET_MOB_VNUM(victim),
				GetRealLevel(victim),
				GetBasicSave(victim, saving, false),
				GetSave(victim, saving),
				victim->GetSkill(ESkill::kAwake) / 5,
				save);
		if (killer != victim && !victim->IsNpc()) {
			victim->send_to_TC(false, true, true,
					"SAVING (%s): Killer==%s  Target==%s vnum==%d Level==%d base_save==%d save_equip==%d save_awake=-%d result_save==%d\r\n",
					saving_name.find(saving)->second.c_str(),
					GET_NAME(killer),
					GET_NAME(victim),
					GET_MOB_VNUM(killer),
					GetRealLevel(victim),
					GetBasicSave(victim, saving, false),
					GetSave(victim, saving),
					victim->GetSkill(ESkill::kAwake) / 5,
					save);
		}
	}
	// Throwing a 0 is always a failure.
	return save;
}

int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply) {
	// A char never saves against its own (e.g. mirror-reflected) spell -- the effect lands.
	if (killer == victim) {
		return false;
	}
	// No victim or no saving throw specified -> the save is simply not taken: it "fails"
	// (returns false), so the effect lands. This lets callers drop their type==kNone guards.
	if (victim == nullptr || type == ESaving::kNone) {
		return false;
	}
	int save = CalcSaving(killer, victim, type, true);
	int rnd = number(-200, 200);
	char smallbuf[256];
	if (number(1, 100) <= 5 || (AFF_FLAGGED(victim, EAffect::kHold) && type == ESaving::kReflex)) { //абсолютный фейл
		save /= 2;
		sprintf(smallbuf, "Тестовое сообщение: &RПротивник %s (%d), ваш бонус: %d, спас '%s' противника: %d, random -200..200: %d, критудача: ДА, шанс успеха: %2.2f%%.\r\n&n", 
				GET_NAME(victim), GetRealLevel(victim), ext_apply, saving_name.find(type)->second.c_str(), save, rnd, ((std::clamp(save +ext_apply, -200, 200) + 200) / 400.) * 100.);
		if (killer->get_name_str() == "Верий" 
				|| killer->get_name_str() == "Кудояр"
				|| killer->get_name_str() == "Рогоза")
			SendMsgToChar(killer, "%s", smallbuf);
		else
			killer->send_to_TC(false, true, true, smallbuf);
	} else {
		sprintf(smallbuf, "Тестовое сообщение: Противник %s (%d), ваш бонус: %d, спас '%s' противника: %d, random -200..200: %d, критудача: НЕТ, шанс успеха: %2.2f%%.\r\n", 
				GET_NAME(victim), GetRealLevel(victim), ext_apply, saving_name.find(type)->second.c_str(), save, rnd, ((std::clamp(save +ext_apply, -200, 200) + 200) / 400.) * 100.);
		if (killer->get_name_str() == "Верий" 
				|| killer->get_name_str() == "Кудояр"
				|| killer->get_name_str() == "Рогоза")
			SendMsgToChar(killer, "%s", smallbuf);
		else
			killer->send_to_TC(false, true, true, smallbuf);
	}
	save += ext_apply;    // внешний модификатор (обычно +каст)

	if (save <= rnd) {
		// савинги прошли
		return true;
	}
	return false;
}

bool IsAbleToSay(CharData *ch) {
	if (!ch->IsNpc()) {
		return true;
	}
	switch (GET_RACE(ch)) {
		case ENpcRace::kBoggart:
		case ENpcRace::kGhost:
		case ENpcRace::kHuman:
		case ENpcRace::kZombie: [[fallthrough]];
		case ENpcRace::kSpirit: return true;
	}
	return false;
}

void ShowAffExpiredMsg(ESpell aff_type, CharData *ch) {
	if (!ch->IsNpc() && ch->IsFlagged(EPlrFlag::kWriting)) {
		return;
	}

	const std::string &msg = GetAffExpiredText(aff_type);
	if (!msg.empty()) {
		act(msg.c_str(), false, ch, nullptr, nullptr, kToChar | kToSleep);
		SendMsgToChar("\r\n", ch);
	}
}


static bool IsBreath(ESpell spell_id) {
	static const std::set<ESpell> magic_breath {
	 	ESpell::kFireBreath,
	 	ESpell::kGasBreath,
	 	ESpell::kFrostBreath,
	 	ESpell::kAcidBreath,
	 	ESpell::kLightingBreath
	};

	return magic_breath.contains(spell_id);
}

// SetBattleLag(victim, lag) lives inline next to SetWaitState in char_data.h -- it isn't
// magic-specific. This file keeps only the magic-pipeline-specific skill-scaled overload below.

/*
 * This is a temporary function. It's better to tie the lag value to the success rate of the cast
 * rather than directly to the pure skill. But to calculate the success rate, we need a function
 * for calculating the attack rating and a function for calculating the defense rating.
 * These don't exist and can't be written quickly. Replacing them with placeholders defeats the purpose.
 * The placeholders will still return values different from those ultimately used in the system,
 * and after implementing the rating functions, the spells will have to be rebalanced.
 * Therefore, it is easier to use a temporary function with transparent logic.
 */
void SetBattleLag(CharData *victim, double skill_bonus, unsigned base_lag, double bonus_divisor) {
	if (bonus_divisor > 0) {
		const auto lag = base_lag + static_cast<int>(skill_bonus / bonus_divisor);
		SetBattleLag(victim, lag);
	} else {
		SetBattleLag(victim, base_lag);
	}
}

double CalcMagicElementCoeff(CharData *victim, ESpell spell_id) {
	double element_coeff = 1.0;
	if (victim->IsNpc()) {
		if (NPC_FLAGGED(victim, ENpcFlag::kFireCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kFire) {
				element_coeff /= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kWater) {
				element_coeff *= 2.0;
			}
		}
		if (NPC_FLAGGED(victim, ENpcFlag::kAirCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kEarth) {
				element_coeff *= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kAir) {
				element_coeff /= 2.0;
			}
		}
		if (NPC_FLAGGED(victim, ENpcFlag::kWaterCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kFire) {
				element_coeff *= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kWater) {
				element_coeff /= 2.0;
			}
		}
		if (NPC_FLAGGED(victim, ENpcFlag::kEarthCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kEarth) {
				element_coeff /= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kAir) {
				element_coeff *= 2.0;
			}
		}
	}
	return element_coeff;
}

int CalcBaseDmg(CharData *ch, ESpell spell_id) {
	auto base_dmg{0};
	if (IsBreath(spell_id)) {
		if (!ch->IsNpc()) {
			return 0;
		}
		base_dmg = RollDices(ch->mob_specials.damnodice, ch->mob_specials.damsizedice) +
			GetRealDamroll(ch) + str_bonus(GetRealStr(ch), STR_TO_DAM);
	} else {
		base_dmg = MUD::Spell(spell_id).GetPotencyRoll().RollSkillDices();
	}

	if (!ch->IsNpc()) {
		base_dmg *= ch->get_cond_penalty(P_DAMROLL);
	}
	return base_dmg;
}

static int CalcHeal(CharData *ch, CharData *victim, ESpell spell_id, [[maybe_unused]] int level) {
	// Не у каждого спелла из CastToPoints в данных описан heal-экшен
	// (напр. свежедобавленные kPatronage/kWarcryOfPower). Без этой проверки
	// GetHeal() кидает исключение и роняет сервер (#3312). Логируем, какой
	// спелл недонастроен, и лечим на 0 вместо краша.
	if (!MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kHeal)) {
		mudlog(fmt::format("SYSERR: spell {} ({}) has no 'heal' action, heal skipped",
				to_underlying(spell_id), MUD::Spell(spell_id).GetCName()),
			CMP, kLvlImmortal, SYSLOG, true);
		return 0;
	}
	const auto &heal = MUD::Spell(spell_id).actions.GetHeal();
	// prob: percent chance the healing actually happens (default 100). A failed roll heals 0.
	// Skip the RNG when prob is the always-fires default (short-circuit: number() not called).
	const int heal_prob = heal.GetProb();
	if (heal_prob < 100 && number(1, 100) > heal_prob) {
		return 0;
	}
	const auto &potency_roll = MUD::Spell(spell_id).GetPotencyRoll();
	// The heal amount is decoupled from the global potency roll's coefficients: the roll only
	// supplies the raw dice and competencies (skill+stat), while the <amount> weights belong to
	// the heal, so it can be tuned without disturbing the spell's other roll-driven effects.
	const double dice = potency_roll.RollSkillDices();
	const double competencies = potency_roll.CalcSkillCoeff(ch) + potency_roll.CalcBaseStatCoeff(ch);
	int total_heal = static_cast<int>(heal.GetAmountMin()
		+ std::ceil(dice * heal.GetAmountDicesWeight() + competencies * heal.GetAmountCompetenciesWeight()));
	const double bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
	total_heal += static_cast<int>(total_heal * bonus_mod);
	const double npc_heal = heal.GetNpcCoeff();
	if (ch->IsNpc()) {
		total_heal += static_cast<int>(total_heal * npc_heal);
	}

	ch->send_to_TC(false, true, true,
		"&CMag.heal (%s). Dice: %2.2f, Compet.: %2.2f, Min: %2.2f, Bonus: %1.2f, NPC coeff: %f, Total: %d &n\r\n",
		GET_NAME(victim), dice, competencies, heal.GetAmountMin(), 1 + bonus_mod, npc_heal, total_heal);

	victim->send_to_TC(false, true, true,
		"&CMag.heal (%s). Dice: %2.2f, Compet.: %2.2f, Min: %2.2f, Bonus: %1.2f, NPC coeff: %f, Total: %d &n\r\n",
		GET_NAME(ch), dice, competencies, heal.GetAmountMin(), 1 + bonus_mod, npc_heal, total_heal);

	return total_heal;
}

/**
 * Number of *extra* hits a multi-hit damage spell deals beyond its single mandatory hit.
 * The caller adds 1 for the base hit. Three modes:
 *   - no skill / kUndefined: prob% chance of `max`, else 0;
 *   - skill + prob > 0:      prob% chance of `extra` (= min(skill,75)/divisor, capped at max), else 0;
 *   - skill + prob == 0:     random 0..extra (uniform spread of attack count).
 * The kMagicArrows feat handling lives here as a workaround until skill-interference is unified:
 * on kMagicMissile with the feat we halve the divisor and triple the max, which reproduces
 * yesterday's (level+9)/5-vs-(level+9)/10 ratio at the skill cap.
 */
static int CalcExtraHits(CharData *ch, ESpell spell_id, ESkill skill_id,
				  int skill_divisor = 25, int max = 1, int prob = 20) {
	if (spell_id == ESpell::kMagicMissile && ch && CanUseFeat(ch, EFeat::kMagicArrows)) {
		skill_divisor = std::max(1, skill_divisor / 2);
		max = max * 3;
	}
	if (ch == nullptr || skill_id == ESkill::kUndefined) {
		return (number(1, 100) <= prob) ? max : 0;
	}
	int extra = CalcNoviceSkillBonus(ch, skill_id, skill_divisor);
	if (extra > max) extra = max;
	if (prob == 0) return number(0, extra);
	return ((number(1, 100) <= prob) ? extra : 0);
}

/**
 * Forces a character to assume a certain position (knocked down, asleep, stunned, etc.)
 * Knocks off horse, checks for afflictions.
 */
// Knock the victim down to position `pos` (with the spell's kKnockdown* messages) and/or
// force the fight to stop. Passing EPosition::kUndefined as `pos` changes no position at all
// -- only the force_stopfight branch runs (in the engine, the "fighting" state is itself part
// of the position system, so stopping a fight is a position change in that sense).
static void ForceReposition(CharData *victim, ESpell spell_id, EPosition pos, bool force_stopfight = false) {
	if (victim->IsImmortal()) {
		return;
	}
	const bool reposition = (pos != EPosition::kUndefined);
	if (reposition && victim->GetPosition() < pos) {
		return;
	}
	if (((reposition && pos < EPosition::kSit) || force_stopfight) && victim->GetEnemy()) {
		stop_fighting(victim, force_stopfight);
	}
	if (force_stopfight) {
		change_fighting(victim, force_stopfight);
	}
	if (reposition) {
		victim->DropFromHorse();
		if (pos < victim->GetPosition()) {
			act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToRoom).c_str(),
				false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToChar).c_str(),
				false, victim, nullptr, nullptr, kToChar);
			victim->SetPosition(pos);
		}
	}
}

static int CalcTotalSpellDmg(CharData *ch, CharData *victim, ESpell spell_id) {
	const auto &potency_roll = MUD::Spell(spell_id).GetPotencyRoll();
	const bool has_dmg = MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kDamage);
	// prob: a <damage prob=> spell may simply not fire (default 100).
	// A miss returns 0 -- which, like a full magic-resist, is still processed downstream (no aggro
	// change was requested), so behaviour matches today's zero-damage handling. The prob<100 guard
	// short-circuits the RNG when the result is fixed at "always fires".
	if (has_dmg) {
		const int dmg_prob = MUD::Spell(spell_id).actions.GetDmg().GetProb();
		if (dmg_prob < 100 && number(1, 100) > dmg_prob) {
			return 0;
		}
	}
	int total_dmg{0};
	if (number(1, 100) > std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, GET_MR(victim))) {
		const float base_dmg = CalcBaseDmg(ch, spell_id);
		const float skill_coeff = potency_roll.CalcSkillCoeff(ch);
		const float stat_coeff = potency_roll.CalcBaseStatCoeff(ch);
		const float bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
		const float elem_coeff = CalcMagicElementCoeff(victim, spell_id);

		float dmg;
		if (has_dmg) {
			// Additive model, mirroring heal: the <amount> weights scale
			// the roll's dice and competencies (skill+stat); an absent <amount> defaults to min 0
			// and both weights 1.0.
			const auto &dmg_act = MUD::Spell(spell_id).actions.GetDmg();
			dmg = dmg_act.GetAmountMin() + std::ceil(base_dmg * dmg_act.GetAmountDicesWeight()
					+ (skill_coeff + stat_coeff) * dmg_act.GetAmountCompetenciesWeight());
		} else {
			// Legacy multiplicative model dice * (1 + skill + stat), for spells with no <damage>
			// action (e.g. kWarcryOfChallenge).
			dmg = base_dmg * (1.0f + skill_coeff + stat_coeff);
		}

		total_dmg = static_cast<int>(dmg * elem_coeff);
		total_dmg += static_cast<int>(total_dmg * bonus_mod);
		int complex_mod = total_dmg;
		total_dmg = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, total_dmg);
		complex_mod = total_dmg - complex_mod;
		ch->send_to_TC(false, true, true,
				"&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Stat: %2.2f, Amount: %2.2f, Bonus: %1.2f, Cmplx: %d, Elem.coeff: %1.2f, Total: %d &n\r\n",
				GET_NAME(victim), base_dmg, skill_coeff, stat_coeff, dmg, 1 + bonus_mod, complex_mod, elem_coeff, total_dmg);

		victim->send_to_TC(false, true, true,
				"&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Stat: %2.2f, Amount: %2.2f, Bonus: %1.2f, Cmplx: %d, Elem.coeff: %1.2f, Total: %d &n\r\n",
				GET_NAME(ch), base_dmg, skill_coeff, stat_coeff, dmg, bonus_mod, complex_mod, elem_coeff, total_dmg);
	}

	return total_dmg;
}

// Three defensive checks shared between CastDamage and CastAffect. Each returns true (and emits
// the standard 3-line ToChar/ToNotVict/ToVict message trio) when the defense fires; the caller
// decides what to do next (recursive self-cast for reflection, early return for absorption).
//
// Conditions match the stricter set that CastAffect used (IsViolent / !ch->IsGod / same-room
// for the magic mirror; +remort/2 bias on the sonic barrier; IsViolent on the shield block).
// CastDamage previously had slightly looser conditions for some of these; in practice the diffs
// don't bite (damage spells are violent, same-room, etc.), but the unified version is what runs
// for both call sites now.
namespace {

// Build and process one Damage object for a multi-hit damage spell. `count` is the
// remaining-hit counter (1 == last hit) so the no-flee flag is cleared on the final hit only.
// Returns the result of Damage::Process(), reused by the loop's `rand` to decide whether to
// continue (a non-negative value means the victim is still alive).
int LandOneDamageHit(CharData *ch, CharData *victim, ESpell spell_id, int total_dmg,
					 EPosition ch_start_pos, EPosition victim_start_pos, int count) {
	Damage dmg(SpellDmg(spell_id), total_dmg, fight::kMagicDmg);
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;

	if (CanUseFeat(ch, EFeat::kPowerMagic) && victim->IsNpc()) {
		dmg.flags.set(fight::kIgnoreAbsorbe);
	}
	// отражение магии в кастующего
	if (ch == victim) {
		dmg.flags.set(fight::kMagicReflect);
	}
	if (count <= 1) {
		dmg.flags.reset(fight::kNoFleeDmg);
	} else {
		dmg.flags.set(fight::kNoFleeDmg);
	}
	return dmg.Process(ch, victim);
}

bool TryReflectByMagicGlass(CharData *ch, CharData *victim, ESpell spell_id) {
	if (ch == victim) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagWarcry)) return false;
	if (!MUD::Spell(spell_id).IsViolent()) return false;
	if (ch->IsGod()) return false;
	if (ch->in_room != victim->in_room) return false;
	if (!AFF_FLAGGED(victim, EAffect::kMagicGlass)) return false;
	if (number(1, 100) >= (GetRealLevel(victim) / 3)) return false;
	act("Магическое зеркало $N1 отразило вашу магию!", false, ch, nullptr, victim, kToChar);
	act("Магическое зеркало $N1 отразило магию $n1!", false, ch, nullptr, victim, kToNotVict);
	act("Ваше магическое зеркало отразило поражение $n1!", false, ch, nullptr, victim, kToVict);
	return true;
}

bool TryReflectBySonicBarrier(CharData *ch, CharData *victim, ESpell spell_id) {
	if (ch == victim) return false;
	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry)) return false;
	if (!MUD::Spell(spell_id).IsViolent()) return false;
	if (!victim->IsGod()) return false;
	if (!ch->IsNpc() && GetRealLevel(victim) <= (GetRealLevel(ch) + GetRealRemort(ch) / 2)) return false;
	act("Звуковой барьер $N1 отразил ваш крик!", false, ch, nullptr, victim, kToChar);
	act("Звуковой барьер $N1 отразил крик $n1!", false, ch, nullptr, victim, kToNotVict);
	act("Ваш звуковой барьер отразил крик $n1!", false, ch, nullptr, victim, kToVict);
	return true;
}

// The Vityaz magical-shield block: a skill+feat+worn-shield absorption. The chance is
// (kShieldBlock / 20 + shield_weight / 2) percent. Mass/area/warcry casts bypass the shield.
bool TryBlockByMagicalShield(CharData *ch, CharData *victim, ESpell spell_id) {
	if (ch == victim) return false;
	if (!MUD::Spell(spell_id).IsViolent()) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagWarcry)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagMasses)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagAreas)) return false;
	if (victim->GetSkill(ESkill::kShieldBlock) <= 100) return false;
	if (!GET_EQ(victim, EEquipPos::kShield)) return false;
	if (!CanUseFeat(victim, EFeat::kMagicalShield)) return false;
	const int chance = victim->GetSkill(ESkill::kShieldBlock) / 20
		+ GET_EQ(victim, EEquipPos::kShield)->get_weight() / 2;
	if (number(1, 100) >= chance) return false;
	act("Ваши чары повисли на щите $N1, и затем развеялись.", false, ch, nullptr, victim, kToChar);
	act("Щит $N1 поглотил злые чары $n1.", false, ch, nullptr, victim, kToNotVict);
	act("Ваш щит уберег вас от злых чар $n1.", false, ch, nullptr, victim, kToVict);
	return true;
}

}  // namespace

int CastDamage(int level, CharData *ch, CharData *victim, ESpell spell_id) {
	int rand = 0, count = 1, modi = 0;

	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr)
		return (0);

	if (!pk_agro_action(ch, victim))
		return (0);
	log("[MAG DAMAGE] %s damage %s (%d)", GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
	// Breath spells skip the defensive layer (the magic mirror / sonic barrier / shield block
	// were never intended for them); self-casts also bypass it so the caster doesn't deflect
	// their own spell off their own kMagicGlass.
	if (!IsBreath(spell_id) || ch == victim) {
		if (TryReflectByMagicGlass(ch, victim, spell_id)) {
			log("[MAG DAMAGE] Зеркало - полное отражение: %s damage %s (%d)",
				GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
			return CastDamage(level, ch, ch, spell_id);
		}
		if (TryReflectBySonicBarrier(ch, victim, spell_id)) {
			return CastDamage(level, ch, ch, spell_id);
		}
		// kShadowCloak absorption: 21% chance for the victim's cloak to swallow the cast outright.
		// Only damage spells get this defense (no parallel in CastAffect), so it stays inline.
		if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) && AFF_FLAGGED(victim, EAffect::kShadowCloak)
			&& number(1, 100) < 21) {
			act("Густая тень вокруг $N1 жадно поглотила вашу магию.", false, ch, nullptr, victim, kToChar);
			act("Густая тень вокруг $N1 жадно поглотила магию $n1.", false, ch, nullptr, victim, kToNotVict);
			act("Густая тень вокруг вас поглотила магию $n1.", false, ch, nullptr, victim, kToVict);
			log("[MAG DAMAGE] Мантия  - поглощение урона: %s damage %s (%d)",
				GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
			return 0;
		}
		if (TryBlockByMagicalShield(ch, victim, spell_id)) {
			return 0;
		}
	}

	auto ch_start_pos = ch->GetPosition();
	auto victim_start_pos = victim->GetPosition();

	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
		if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc())
			modi = modi - 50;
	}
//	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
//		modi += (GetRealLevel(ch) - 10);

	// Multi-hit count: a damage spell with a <hits> child gets its extra-hit
	// number from CalcExtraHits; the kMagicArrows feat for kMagicMissile is handled inside it.
	// Absent <hits> -> count stays at the file-top default of 1 (single hit), which matches the
	// current behaviour of every non-multi-hit damage spell.
	if (MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kDamage)) {
		const auto &dmg = MUD::Spell(spell_id).actions.GetDmg();
		if (dmg.HasHits()) {
			const ESkill hits_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
			count = 1 + CalcExtraHits(ch, spell_id, hits_skill,
									  dmg.GetHitsSkillDivisor(), dmg.GetHitsMax(), dmg.GetHitsProb());
		}
	}

	int total_dmg{0};

	try {
		total_dmg = CalcTotalSpellDmg(ch, victim, spell_id);
	} catch (std::exception &e) {
		err_log("%s", e.what());
	}
	total_dmg = std::clamp(total_dmg, 0, kMaxHits);

	for (; count > 0 && rand >= 0; count--) {
		if (ch->in_room != kNowhere
			&& victim->in_room != kNowhere
			&& victim->in_room == ch->in_room
			&& ch->GetPosition() > EPosition::kStun
			&& victim->GetPosition() > EPosition::kDead) {
			rand = LandOneDamageHit(ch, victim, spell_id, total_dmg,
									ch_start_pos, victim_start_pos, count);
		}
	}
	return rand;
}

EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id) {
	int vnum = 0;
	const char *missing = nullptr, *use = nullptr, *exhausted = nullptr;
	switch (spell_id) {
		case ESpell::kFascination:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (i->get_type() == EObjType::kMagicIngredient && i->get_val(1) == 3000) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			use = "Вы взяли череп летучей мыши в левую руку.\r\n";
			missing = "Батюшки светы! А помаду-то я дома забыл$g.\r\n";
			exhausted = "$o рассыпался в ваших руках от неловкого движения.\r\n";
			break;
		case ESpell::kHypnoticPattern:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (i->get_type() == EObjType::kMagicIngredient && i->get_val(1) == 3006) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			use = "Вы разожгли палочку заморских благовоний.\r\n";
			missing = "Вы начали суматошно искать свои благовония, но тщетно.\r\n";
			exhausted = "$o дотлели и рассыпались пеплом.\r\n";
			break;
		case ESpell::kEnchantWeapon:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (i->get_type() == EObjType::kMagicIngredient && i->get_val(1) == 1930) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			use = "Вы подготовили дополнительные компоненты для зачарования.\r\n";
			missing = "Вы были уверены что положили его в этот карман.\r\n";
			exhausted = "$o вспыхнул голубоватым светом, когда его вставили в предмет.\r\n";
			break;

		// A spell with no material component: nothing to process, the cast proceeds.
		// (The material component system is to be refined/expanded as a separate task.)
		default: return EStageResult::kSuccess;
	}
	ObjData *tobj = GetObjByVnumInContent(vnum, caster->carrying);
	if (!tobj) {
		act(missing, false, victim, nullptr, caster, kToChar);
		return EStageResult::kBreak;	// required component missing -> stop the cast
	}
	tobj->dec_val(2);
	act(use, false, caster, tobj, nullptr, kToChar);
	if (GET_OBJ_VAL(tobj, 2) < 1) {
		act(exhausted, false, caster, tobj, nullptr, kToChar);
		RemoveObjFromChar(tobj);
		ExtractObjFromWorld(tobj);
	}
	return EStageResult::kSuccess;
}

bool ProcessMatComponents(CharData *caster, int /*vnum*/, ESpell spell_id) {
	const char *missing = nullptr, *use = nullptr, *exhausted = nullptr;
	switch (spell_id) {
		case ESpell::kEnchantWeapon: use = "Вы подготовили дополнительные компоненты для зачарования.\r\n";
			missing = "Вы были уверены что положили его в этот карман.\r\n";
			exhausted = "$o вспыхнул голубоватым светом, когда его вставили в предмет.\r\n";
			break;

		default: log("WARNING: wrong spell_id %d in %s:%d", to_underlying(spell_id), __FILE__, __LINE__);
			return false;
	}
	ObjData *tobj = GET_EQ(caster, EEquipPos::kHold);
	if (!tobj) {
		act(missing, false, caster, nullptr, caster, kToChar);
		return (true);
	}
	tobj->dec_val(2);
	act(use, false, caster, tobj, nullptr, kToChar);
	if (GET_OBJ_VAL(tobj, 2) < 1) {
		act(exhausted, false, caster, tobj, nullptr, kToChar);
		RemoveObjFromChar(tobj);
		ExtractObjFromWorld(tobj);
	}
	return (false);
}

// Applies one affect produced by a TalentAffect apply to the victim, honoring the
// EAffFlag update flags: kAfAccumulateDuration adds to an existing
// affect's duration, kAfUpdateDuration refreshes it to the longer value, and
// kAfUpdateMod replaces the modifier only when the new magnitude is larger. The
// caller runs affect_total() afterwards.
static void ApplyTalentAffect(CharData *victim, Affect<EApply> &af, Bitvector flags, int max_stacks) {
	const bool accum_dur = IS_SET(flags, to_underlying(EAffFlag::kAfAccumulateDuration));
	const bool update_dur = IS_SET(flags, to_underlying(EAffFlag::kAfUpdateDuration));
	const bool update_mod = IS_SET(flags, to_underlying(EAffFlag::kAfUpdateMod));
	for (auto it = victim->affected.begin(); it != victim->affected.end(); ++it) {
		const auto existing = *it;
		if (existing->type == af.type && existing->location == af.location) {
			if (accum_dur) {
				af.duration += existing->duration;
			} else if (update_dur) {
				af.duration = std::max(af.duration, existing->duration);
			}
			if (max_stacks > 1 && existing->stacks < max_stacks) {
				// Add a stack: bump the count and accumulate the modifier.
				// kAfUpdateMod is ignored here; the sum is clamped to int to avoid overflow.
				af.stacks = existing->stacks + 1;
				const int64_t sum = static_cast<int64_t>(existing->modifier) + af.modifier;
				af.modifier = static_cast<int>(std::clamp<int64_t>(sum,
						std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
			} else if (max_stacks > 1) {
				// Already at the cap: keep the accumulated modifier and stack count, only the
				// duration (above) is refreshed.
				af.stacks = existing->stacks;
				af.modifier = existing->modifier;
			} else {
				// Non-stacking affect (max_stacks == 1): legacy kAfUpdateMod behaviour.
				af.stacks = existing->stacks;
				if (update_mod && std::abs(existing->modifier) > std::abs(af.modifier)) {
					af.modifier = existing->modifier;
				}
			}
			victim->AffectRemove(it);
			break;
		}
	}
	affect_to_char(victim, af);
}

// Apply the spell's <affects> talent block to `victim`, returning whether the affect actually
// landed. Three exit paths return false:
//   - the prob roll missed -> silent miss (no message),
//   - a saving throw averted the affect -> "no effect" to the caster,
//   - the affect was already present and the spell doesn't accumulate/update -> "no effect".
// On success, every ordinary apply is imposed; among the random-flagged ones a single uniformly-
// chosen winner (reservoir sampling) is also imposed. The duration is computed once for all
// applies, then each apply's modifier is derived from the cast's potency roll. Every imposed
// affect records the cast's potency and debuff nature so a later dispel can be strength-gated.
static bool TryApplyAffectTalent(CharData *ch, CharData *victim, ESpell spell_id, int modi,
								 const RollResult &potency, float cast_potency, bool cast_debuff) {
	const auto &talent = MUD::Spell(spell_id).actions.GetAffect();
	// prob: percent chance the <affects> block fires at all (default 100, silent miss on fail).
	// Skipping it suppresses the affect, its lag and its reposition (gated by the caller).
	// The prob<100 guard short-circuits the RNG when the spell always fires.
	const int aff_prob = talent.GetProb();
	if (aff_prob < 100 && number(1, 100) > aff_prob) {
		return false;
	}
	// The affect-resist (GET_AR) debuff block is handled up front (see top of CastAffect);
	// here only the saving throw can still avert the affect (kNone saving -> CalcGeneralSaving
	// returns false, so no save is taken).
	if (ch != victim && CalcGeneralSaving(ch, victim, talent.GetSaving(), modi)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		return false;
	}
	const Bitvector flags = talent.GetFlags();
	const bool can_reapply = IS_SET(flags, to_underlying(EAffFlag::kAfAccumulateDuration))
		|| IS_SET(flags, to_underlying(EAffFlag::kAfUpdateDuration));
	if (ch != victim && IsAffectedBySpell(victim, talent.GetSpell()) && !can_reapply) {
		if (ch->in_room == victim->in_room) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
		return false;
	}
	// skill-based duration. The bonus uses the caster's potency-roll base_skill (kUndefined for
	// spells without a <potency_roll> -> flat duration). `victim` decides the unit (PC: hours ->
	// ticks; NPC: raw), preserving today's tick-unit semantics.
	const ESkill duration_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
	int duration = ApplyResist(victim, talent.GetResist(),
		CalcDuration(ch, victim, duration_skill,
					 talent.GetDurationBase(), talent.GetDurationSkillDivisor(),
					 talent.GetDurationMin(), talent.GetDurationMax()));
	duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, duration);
	const double competencies = potency.skill_coeff + potency.stat_coeff;
	auto apply_one = [&](const talents_actions::TalentAffect::Apply &apply) {
		double raw = competencies * apply.competencies_weight + potency.dices * apply.dices_weight;
		double modifier = apply.min + std::ceil(raw);
		Affect<EApply> taf;
		taf.type = talent.GetSpell();
		taf.affect_type = apply.id;
		taf.location = apply.location;
		taf.duration = duration;
		taf.modifier = static_cast<int>(apply.factor * modifier);
		taf.battleflag = flags;
		taf.caster_id = ch->get_uid();
		taf.potency = cast_potency;
		taf.debuff = cast_debuff;
		// apply.stack is the max stack count: re-applying up to the cap adds a stack and
		// accumulates the modifier (see ApplyTalentAffect).
		ApplyTalentAffect(victim, taf, flags, apply.stack);
	};
	// Apply every ordinary apply; among the random-flagged ones (the "random" attribute) impose
	// a single uniformly-chosen winner (reservoir sampling).
	const talents_actions::TalentAffect::Apply *random_choice = nullptr;
	int random_seen = 0;
	for (const auto &apply: talent.GetApplies()) {
		if (apply.random) {
			if (number(1, ++random_seen) == 1) {
				random_choice = &apply;
			}
		} else {
			apply_one(apply);
		}
	}
	if (random_choice) {
		apply_one(*random_choice);
	}
	return true;
}

// On a successfully-landed affect, emit the side effects: battle lag, forced reposition, poison
// owner tag, and the imposition messages. The lag/reposition pair is gated on the spell having an
// <affects> talent (where they live); the poison tag and messages apply to any successful cast.
static void EmitImpositionEffects(CharData *ch, CharData *victim, ESpell spell_id,
								  const RollResult &potency) {
	if (MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kAffect)) {
		const auto &side = MUD::Spell(spell_id).actions.GetAffect();
		// Battle lag: <affects> with <lag> delays the victim once the affect lands. Constant-lag
		// spells use a non-positive bonus_divisor; skill-scaling ones add
		// potency.low_skill_coeff / bonus_divisor.
		if (side.HasLag()) {
			SetBattleLag(victim, potency.low_skill_coeff, side.GetLagBase(), side.GetLagBonusDivisor());
		}
		// Forced reposition / fight-stop: e.g. kSleep knocks to kSleep, kPeaceful stops the fight
		// (pos kUndefined). Runs after the saving/affect-resist gate, so the position only changes
		// when the debuff actually lands.
		if (side.HasReposition()) {
			ForceReposition(victim, spell_id, side.GetRepositionPos(), side.GetRepositionStopFight());
		}
	}
	// вот некрасиво же тут это делать...
	if (spell_id == ESpell::kPoison) {
		victim->poisoner = ch->get_uid();
	}
	// Affect imposition messages: looked up by the cast spell and emitted sheaf-directly, so a
	// spell with no such message shows nothing.
	const auto &imposed = MUD::SpellMessages()[spell_id];
	const auto &to_vict = imposed.GetMessage(ESpellMsg::kAffImposedToChar);
	const auto &to_room = imposed.GetMessage(ESpellMsg::kAffImposedToRoom);
	if (!to_vict.empty())
		act(to_vict.c_str(), false, victim, nullptr, ch, kToChar);
	if (!to_room.empty())
		act(to_room.c_str(), true, victim, nullptr, ch, kToRoom | kToArenaListen);
}

EStageResult CastAffect(int level, CharData *ch, CharData *victim, const ESpell spell_id, const RollResult &potency) {
	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr) {
		return EStageResult::kSuccess;
	}

	// Calculate PKILL's affects
	if (ch != victim) {
		if (MUD::Spell(spell_id).IsFlagged(kNpcAffectPc)) {
			if (!pk_agro_action(ch, victim)) {
				return EStageResult::kSuccess;
			}
		} else if (MUD::Spell(spell_id).IsFlagged(kNpcAffectNpc) && victim->GetEnemy())	{
			if (!pk_agro_action(ch, victim->GetEnemy()))
				return EStageResult::kSuccess;
		}
	}
	// Shared defensive layer with CastDamage: magic mirror, sonic barrier, magical shield. The
	// kShadowCloak absorption is damage-only and stays in CastDamage.
	if (TryReflectByMagicGlass(ch, victim, spell_id)) {
		CastAffect(level, ch, ch, spell_id);
		return EStageResult::kSuccess;
	}
	if (TryReflectBySonicBarrier(ch, victim, spell_id)) {
		CastAffect(level, ch, ch, spell_id);
		return EStageResult::kSuccess;
	}
	if (TryBlockByMagicalShield(ch, victim, spell_id)) {
		return EStageResult::kSuccess;
	}

	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) && ch != victim && MUD::Spell(spell_id).IsViolent()
		&& number(1, 999) <= GET_AR(victim) * 10) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	// decrease modi for failing, increese fo success
	int modi = 0;
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
	}

	if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc()) {
		modi = modi - 50;
	}


	// A violent spell can never touch an immortal target: there is nothing to build or
	// roll, so stop here. This subsumes the per-case victim->IsImmortal() guards.
	if (victim->IsImmortal() && MUD::Spell(spell_id).IsViolent()) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	// Affect-resist (GET_AR): a blanket block on any debuff (a violent spell with an effect),
	// a historical mechanic -- checked up front, before any saving throw or affect is built,
	// so it stops the debuff regardless of circumstances.
	if (ch != victim && MUD::Spell(spell_id).IsViolent() && number(1, 100) <= GET_AR(victim)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	// The affect's saving throw is read straight from the talent (GetAffect().GetSaving()) in the
	// talent-affect block below; the <blocking>/<required> immunity checks moved up to
	// CastToSingleTarget (action-level, gating the whole cast).
	const bool has_affect_talent = MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kAffect);
	// Material component: consume it if this spell has one (no-op for spells that don't);
	// a missing component stops the cast. (Hook for the material-component system, TBD.)
	if (ProcessMatComponents(ch, victim, spell_id) == EStageResult::kBreak) {
		return EStageResult::kBreak;
	}

	// issue: every affect this cast lands records the cast's potency (strength) and whether it is a
	// debuff, so a later dispel can be gated by strength (see CastUnaffects/DispelSucceeds). The
	// potency is the cast roll (dice + skill + stat coefficients); the nature follows the violent flag.
	const float cast_potency = static_cast<float>(potency.dices + potency.skill_coeff + potency.stat_coeff);
	const bool cast_debuff = MUD::Spell(spell_id).IsViolent();

	// A spell without an <affects> block has no affect to apply -- `success` stays true so the
	// poison/message side-effects still fire for any non-affect-talent path.
	bool success = true;
	if (has_affect_talent) {
		success = TryApplyAffectTalent(ch, victim, spell_id, modi, potency, cast_potency, cast_debuff);
	}

	affect_total(victim);

	if (success) {
		EmitImpositionEffects(ch, victim, spell_id, potency);
	}
	return EStageResult::kSuccess;
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

// * These use act(), don't put the \r\n.
// Сообщения призыва/оживления вынесены в lib/cfg/spell_msg.xml:
// kSummonToRoom (по заклинанию), kSummonFail / kSummonNoCorpse и прочие
// guard-сообщения в ветви kDefault. См. MUD::SpellMessages().

// Per-spell summon parameters produced by PrepareSummonParams. mob_num is the negated VNUM
// passed to ReadMobile; pfail is the failure-chance percent (0 = never fail); keeper marks
// the spawned mob as a charm-keeper helper; handle_corpse means the source object is a corpse
// to be spilled+extracted after a successful spawn.
struct SummonParams {
	MobVnum mob_num = 0;
	int pfail = 0;
	bool keeper = false;
	bool handle_corpse = false;
};

// Pick the necro-mob VNUM to spawn for kAnimateDead, given the source corpse's mob level. The
// upper tier (>34) is a 50/50 between damager/breather. The caster's own (level + remort + 4)
// then caps the result: very low-level necromancers can never spawn higher-tier undead.
static MobVnum PickNecroMobForCorpse(CharData *ch, int corpse_mob_level) {
	MobVnum mob_num;
	if (corpse_mob_level <= 5) {
		mob_num = kMobSkeleton;
	} else if (corpse_mob_level <= 10) {
		mob_num = kMobZombie;
	} else if (corpse_mob_level <= 15) {
		mob_num = kMobBonedog;
	} else if (corpse_mob_level <= 20) {
		mob_num = kMobBonedragon;
	} else if (corpse_mob_level <= 25) {
		mob_num = kMobBonespirit;
	} else if (corpse_mob_level <= 34) {
		mob_num = kMobNecrotank;
	} else {
		mob_num = (number(1, 100) > 50) ? kMobNecrobreather : kMobNecrodamager;
	}
	// kMobNecrocaster disabled, cant cast
	const int cap = GetRealLevel(ch) + GetRealRemort(ch) + 4;
	if (cap < 15 && mob_num > kMobZombie) {
		mob_num = kMobZombie;
	} else if (cap < 25 && mob_num > kMobBonedog) {
		mob_num = kMobBonedog;
	} else if (cap < 32 && mob_num > kMobBonedragon) {
		mob_num = kMobBonedragon;
	}
	return mob_num;
}

// Compute the per-spell summon parameters into `p`. Returns false (no further action by caller)
// when the spell short-circuits with its own diagnostic message (e.g. kAnimateDead/kResurrection
// when obj isn't a corpse, kResurrection on a corpse missing its mob VNUM, or an unsummonable
// spell_id reaching the default case).
static bool PrepareSummonParams(CharData *ch, ObjData *obj, ESpell spell_id, SummonParams &p) {
	switch (spell_id) {
		case ESpell::kClone:
			p.mob_num = kMobDouble;
			// 50% failure, should be based on something later.
			p.pfail = 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) * 5;
			p.keeper = true;
			return true;

		case ESpell::kSummonKeeper:
			p.mob_num = kMobKeeper;
			p.pfail = ch->GetEnemy() ? 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) : 0;
			p.keeper = true;
			return true;

		case ESpell::kSummonFirekeeper:
			p.mob_num = kMobFirekeeper;
			p.pfail = ch->GetEnemy() ? 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) : 0;
			p.keeper = true;
			return true;

		case ESpell::kAnimateDead:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(),
					false, ch, nullptr, nullptr, kToChar);
				return false;
			}
			p.mob_num = GET_OBJ_VAL(obj, 2);
			if (p.mob_num <= 0) {
				p.mob_num = kMobSkeleton;
			} else {
				const int real_mob_num = GetMobRnum(p.mob_num);
				CharData *tmp_mob = mob_proto + real_mob_num;
				p.pfail = 10 + tmp_mob->get_con() * 2
					- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) * 5;
				p.mob_num = PickNecroMobForCorpse(ch, GetRealLevel(tmp_mob));
			}
			p.handle_corpse = true;
			return true;

		case ESpell::kResurrection:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(),
					false, ch, nullptr, nullptr, kToChar);
				return false;
			}
			p.mob_num = GET_OBJ_VAL(obj, 2);
			if (p.mob_num <= 0) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectBadCorpse) + "\r\n", ch);
				return false;
			}
			p.handle_corpse = true;
			{
				CharData *tmp_mob = mob_proto + GetMobRnum(p.mob_num);
				p.pfail = 10 + tmp_mob->get_con() * 2
					- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) * 5;
			}
			return true;

		default:
			return false;
	}
}

// Rename a freshly-spawned resurrection mob as "умертвие <его_имя>" across all six grammatical
// cases. Also clears the long-description, sets neutral gender, marks it kResurrected, and
// optionally boosts dam/hit/HR by 20% for the kFuryOfDarkness feat.
static void RenameAsUndead(CharData *ch, CharData *mob) {
	ClearMinionTalents(mob);
	if (mob->IsFlagged(EMobFlag::kNoGroup)) {
		mob->UnsetFlag(EMobFlag::kNoGroup);
	}

	sprintf(buf2, "умертвие %s %s", GET_PAD(mob, 1), GET_NAME(mob));
	mob->SetCharAliases(buf2);
	sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
	mob->set_npc_name(buf2);
	mob->player_data.long_descr = "";
	sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
	mob->player_data.PNames[ECase::kNom] = std::string(buf2);
	sprintf(buf2, "умертвию %s", GET_PAD(mob, 1));
	mob->player_data.PNames[ECase::kDat] = std::string(buf2);
	sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
	mob->player_data.PNames[ECase::kAcc] = std::string(buf2);
	sprintf(buf2, "умертвием %s", GET_PAD(mob, 1));
	mob->player_data.PNames[ECase::kIns] = std::string(buf2);
	sprintf(buf2, "умертвии %s", GET_PAD(mob, 1));
	mob->player_data.PNames[ECase::kPre] = std::string(buf2);
	sprintf(buf2, "умертвия %s", GET_PAD(mob, 1));
	mob->player_data.PNames[ECase::kGen] = std::string(buf2);
	mob->set_sex(EGender::kNeutral);
	mob->SetFlag(EMobFlag::kResurrected);
	if (CanUseFeat(ch, EFeat::kFuryOfDarkness)) {
		GET_DR(mob) = GET_DR(mob) + GET_DR(mob) * 0.20;
		mob->set_max_hit(mob->get_max_hit() + mob->get_max_hit() * 0.20);
		mob->set_hit(mob->get_max_hit());
		GET_HR(mob) = GET_HR(mob) + GET_HR(mob) * 0.20;
	}
}

// True if the freshly-loaded `mob` can't be summoned by `ch` (sanctuary / mob spec / world flag /
// god's shield / horse). On true, sends the appropriate kResurrect*/kSummonWarhorse message and
// extracts `mob` from the world; caller must `return kSuccess` without further action. Immortals
// bypass every guard except the horse check (which is universal).
static bool IsSummonTargetProtected(CharData *ch, CharData *mob, ESpell spell_id) {
	if (!ch->IsImmortal() && (AFF_FLAGGED(mob, EAffect::kSanctuary) || mob->IsFlagged(EMobFlag::kProtect))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectConsecrated) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	if (!ch->IsImmortal()
		&& (GET_MOB_SPEC(mob) || mob->IsFlagged(EMobFlag::kNoResurrection) || mob->IsFlagged(EMobFlag::kAreaAttack))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectNoPower) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	if (!ch->IsImmortal() && AFF_FLAGGED(mob, EAffect::kGodsShield)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectProtected) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	if (mob->IsFlagged(EMobFlag::kMounting)) {
		mob->UnsetFlag(EMobFlag::kMounting);
	}
	if (IS_HORSE(mob)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonWarhorse) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	return false;
}

// For the top-tier kAnimateDead spawns (kMobNecrodamager..kLastNecroMob): bump max HP by 10% per
// remort, then crank up damnodice until the mob's reformed-charm value matches the caster's
// charm-points budget (or hits the 255 cap, which means damsize is too small for this caster).
static void BoostNecroDamage(CharData *ch, CharData *mob, ESpell spell_id) {
	// add 10% mob health by remort
	mob->set_max_hit(mob->get_max_hit() * (1.0 + GetRealRemort(ch) / 10.0));
	mob->set_hit(mob->get_max_hit());
	int player_charms_value = CalcCharmPoint(ch, spell_id);
	int mob_cahrms_value = GetReformedCharmiceHp(ch, mob, spell_id);
	int damnodice = 1;
	mob->mob_specials.damnodice = damnodice;
	// look for count dice to maximize damage on player_charms_value. max 255.
	while (player_charms_value > mob_cahrms_value && damnodice <= 255) {
		damnodice++;
		mob->mob_specials.damnodice = damnodice;
		mob_cahrms_value = GetReformedCharmiceHp(ch, mob, spell_id);
	}
	damnodice--;

	mob->mob_specials.damnodice = damnodice; // get prew damnodice for match with player_charms_value
	if (damnodice == 255) {
		// if damnodice == 255 mob damage not maximized. damsize too small
		SendMsgToRoom("Темные искры пробежали по земле... И исчезли...", ch->in_room, 0);
	} else {
		// mob damage maximazed.
		SendMsgToRoom("Темные искры пробежали по земле. Кажется сама СМЕРТЬ наполняет это тело силой!",
					  ch->in_room, 0);
	}
}

// Copy caster cosmetics + stats onto the kClone double: PNames in all six cases, every stat, the
// hp/ac/dr/hr/class/build, position, gender, flags.
static void ApplyCloneCosmetics(CharData *ch, CharData *mob) {
	sprintf(buf2, "двойник %s %s", GET_PAD(ch, 1), GET_NAME(ch));
	mob->SetCharAliases(buf2);
	sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
	mob->set_npc_name(buf2);
	mob->player_data.long_descr = "";
	sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kNom] = std::string(buf2);
	sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kGen] = std::string(buf2);
	sprintf(buf2, "двойнику %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kDat] = std::string(buf2);
	sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kAcc] = std::string(buf2);
	sprintf(buf2, "двойником %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kIns] = std::string(buf2);
	sprintf(buf2, "двойнике %s", GET_PAD(ch, 1));
	mob->player_data.PNames[ECase::kPre] = std::string(buf2);

	mob->set_str(ch->get_str());
	mob->set_dex(ch->get_dex());
	mob->set_con(ch->get_con());
	mob->set_wis(ch->get_wis());
	mob->set_int(ch->get_int());
	mob->set_cha(ch->get_cha());

	mob->set_level(GetRealLevel(ch));
	GET_HR(mob) = -20;
	GET_AC(mob) = GET_AC(ch);
	GET_DR(mob) = GET_DR(ch);

	mob->set_max_hit(ch->get_max_hit());
	mob->set_hit(ch->get_max_hit());
	mob->mob_specials.damnodice = 0;
	mob->mob_specials.damsizedice = 0;
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
	mob->set_exp(0);

	mob->SetPosition(EPosition::kStand);
	GET_DEFAULT_POS(mob) = EPosition::kStand;
	mob->set_sex(EGender::kMale);

	mob->set_class(ch->GetClass());
	GET_WEIGHT(mob) = GET_WEIGHT(ch);
	GET_HEIGHT(mob) = GET_HEIGHT(ch);
	GET_SIZE(mob) = GET_SIZE(ch);
	mob->SetFlag(EMobFlag::kClone);
	mob->UnsetFlag(EMobFlag::kMounting);
}

// Recurse into CastSummon to fill the caster's clone quota: cap is max(1, (level+4)/5 - 2), minus
// the clones already in the follower list. Returns false (caller must stop) when the quota is
// already met; the recursive call passes need_fail=0 so subsequent clones don't re-roll failure.
static bool MaybeSpawnAdditionalClones(int level, CharData *ch, ObjData *obj, ESpell spell_id);

// kAnimateDead post-spawn: kResurrected flag + per-tier rescue grants for the kLoyalAssist /
// kHauntingSpirit feats; high-wisdom (75+) casters also gift an ice shield. The wis>=65 magic-
// glass grant is left commented out as a "if we ever want it back" hook.
static void EnhanceAnimateDead(CharData *ch, CharData *mob, MobVnum mob_num,
							   ESpell spell_id, int charm_duration) {
	mob->SetFlag(EMobFlag::kResurrected);
	if (mob_num == kMobSkeleton && CanUseFeat(ch, EFeat::kLoyalAssist)) {
		mob->set_skill(ESkill::kRescue, 100);
	}
	if (mob_num == kMobBonespirit && CanUseFeat(ch, EFeat::kHauntingSpirit)) {
		mob->set_skill(ESkill::kRescue, 120);
	}

	// даем всем поднятым, ну наверное не будет чернок 75+ мудры вызывать зомби в щите.
	const float eff_wis = CalcEffectiveWis(ch, spell_id);
	if (eff_wis >= 65) {
		// пока не даем, если надо включите
		//af.affect_type = to_underlying(EAffectFlag::AFF_MAGICGLASS);
		//affect_to_char(mob, af);
	}
	if (eff_wis >= 75) {
		Affect<EApply> af;
		af.type = ESpell::kUndefined;
		af.duration = charm_duration * (1 + GetRealRemort(ch));
		af.modifier = 0;
		af.location = EApply::kNone;
		af.affect_type = EAffect::kIceShield;
		af.battleflag = 0;
		affect_to_char(mob, af);
	}
}

// kSummonKeeper post-spawn: tie keeper level to caster, then derive a "rating" from
// light-magic + cha and project that onto hit/skills/stats/HR/AC.
// Svent TODO: не забыть перенести это в ability
static void SetupKeeperStats(CharData *ch, CharData *mob) {
	mob->set_level(GetRealLevel(ch));
	int rating = (ch->GetSkill(ESkill::kLightMagic) + GetRealCha(ch)) / 2;
	int v = 50 + RollDices(10, 10) + rating * 6;
	mob->set_hit(v);
	mob->set_max_hit(v);
	mob->set_skill(ESkill::kPunch, 10 + rating * 1.5);
	mob->set_skill(ESkill::kRescue, 50 + rating);
	mob->set_str(3 + rating / 5);
	mob->set_dex(10 + rating / 5);
	mob->set_con(10 + rating / 5);
	GET_HR(mob) = rating / 2 - 4;
	GET_AC(mob) = 100 - rating * 2.65;
}

// kSummonFirekeeper post-spawn: a fire-aura (or fire-shield at 30+ effective cha) charm affect,
// dr/hp/skills scaled by a 0..30 modifier derived from caster cha. Awakens on spawn.
static void SetupFirekeeperStats(CharData *ch, CharData *mob, int charm_duration) {
	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = charm_duration;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;
	af.affect_type = (get_effective_cha(ch) >= 30) ? EAffect::kFireShield : EAffect::kFireAura;
	affect_to_char(mob, af);

	const int modifier = VPOSI((int) get_effective_cha(ch) - 20, 0, 30);

	GET_DR(mob) = 10 + modifier * 3 / 2;
	GET_NDD(mob) = 1;
	GET_SDD(mob) = modifier / 5 + 1;
	mob->mob_specials.extra_attack = 0;

	const int m = 300 + number(modifier * 12, modifier * 16);
	mob->set_hit(m);
	mob->set_max_hit(m);
	mob->set_skill(ESkill::kAwake, 50 + modifier * 2);
	mob->SetFlag(EPrf::kAwake);
}

// Spill the source corpse's contents into the caster's room (decay-checking each item) and
// extract the corpse itself. Used post-spawn whenever SummonParams::handle_corpse is set.
// А надо ли это вообще делать???
static void SpillCorpseContents(CharData *ch, ObjData *obj) {
	for (ObjData *tobj = obj->get_contains(); tobj; ) {
		ObjData *next_obj = tobj->get_next_content();
		RemoveObjFromObj(tobj);
		PlaceObjToRoom(tobj, ch->in_room);
		if (!CheckObjDecay(tobj) && tobj->get_in_room() != kNowhere) {
			act("На земле остал$U лежать $o.", false, ch, tobj, nullptr, kToRoom | kToArenaListen);
		}
		tobj = next_obj;
	}
	ExtractObjFromWorld(obj);
}

EStageResult CastSummon(int level, CharData *ch, ObjData *obj, ESpell spell_id, bool need_fail) {
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	if (spell_id == ESpell::kSumonAngel) {
		SummonTutelar(ch);
		return EStageResult::kSuccess;
	}
	if (spell_id == ESpell::kMentalShadow) {
		SpellMentalShadow(ch);
		return EStageResult::kSuccess;
	}

	SummonParams p;
	if (!PrepareSummonParams(ch, obj, spell_id, p)) {
		return EStageResult::kSuccess;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	// при перке помощь тьмы гораздо меньше шанс фейла -- kFavorOfDarkness rerolls the fail at
	// 1/4 (i.e. only 1 in 4 fails actually sticks); without the feat the fail always sticks.
	if (!ch->IsImmortal() && number(0, 101) < p.pfail && need_fail) {
		const bool fail_sticks = !CanUseFeat(ch, EFeat::kFavorOfDarkness) || number(0, 3) == 0;
		if (fail_sticks) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			if (p.handle_corpse) {
				ExtractObjFromWorld(obj);
			}
			return EStageResult::kSuccess;
		}
	}

	CharData *mob = ReadMobile(-p.mob_num, kVirtual);
	if (!mob) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}

	if (spell_id == ESpell::kResurrection) {
		RenameAsUndead(ch, mob);
	}

	if (IsSummonTargetProtected(ch, mob, spell_id)) {
		return EStageResult::kSuccess;
	}

	if (spell_id == ESpell::kAnimateDead && p.mob_num >= kMobNecrodamager && p.mob_num <= kLastNecroMob) {
		BoostNecroDamage(ch, mob, spell_id);
	}

	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		if (p.handle_corpse) {
			ExtractObjFromWorld(obj);
		}
		return EStageResult::kSuccess;
	}

	mob->set_exp(0);
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
	const auto days_from_full_moon =
		(weather_info.moon_day < 14) ? (14 - weather_info.moon_day) : (weather_info.moon_day - 14);
	// familiar duration is a flat number tied to caster wisdom + the moon phase; no skill scaling,
	// so skill_id=kUndefined. `mob` (the familiar) sets the NPC-raw unit.
	const auto duration = CalcDuration(ch, mob, ESkill::kUndefined,
		GetRealWis(ch) + number(0, days_from_full_moon), 0, 0, 0);
	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = duration;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.affect_type = EAffect::kCharmed;
	af.battleflag = 0;
	affect_to_char(mob, af);
	if (p.keeper) {
		af.affect_type = EAffect::kHelper;
		affect_to_char(mob, af);
		mob->set_skill(ESkill::kRescue, 100);
	}

	mob->SetFlag(EMobFlag::kCorpse);
	if (spell_id == ESpell::kClone) {
		ApplyCloneCosmetics(ch, mob);
	}
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonToRoom).c_str(),
		false, ch, nullptr, mob, kToRoom | kToArenaListen);

	PlaceCharToRoom(mob, ch->in_room);
	ch->add_follower(mob);

	if (spell_id == ESpell::kClone) {
		// клоны теперь кастятся все вместе // ужасно некрасиво сделано
		if (!MaybeSpawnAdditionalClones(level, ch, obj, spell_id)) {
			return EStageResult::kSuccess;
		}
	}
	if (spell_id == ESpell::kAnimateDead) {
		EnhanceAnimateDead(ch, mob, p.mob_num, spell_id, duration);
	}
	if (spell_id == ESpell::kSummonKeeper) {
		SetupKeeperStats(ch, mob);
	}
	if (spell_id == ESpell::kSummonFirekeeper) {
		SetupFirekeeperStats(ch, mob, duration);
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	if (p.handle_corpse) {
		SpillCorpseContents(ch, obj);
	}
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment; //выровняем алигмент чтоб не агрили вдруг
	return EStageResult::kSuccess;
}

// Defined after CastSummon (forward-declared above) because it recurses into it.
static bool MaybeSpawnAdditionalClones(int level, CharData *ch, ObjData *obj, ESpell spell_id) {
	int already = 0;
	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed) && k->get_master() == ch) {
			++already;
		}
	}
	const int remaining = std::max(1, (GetRealLevel(ch) + 4) / 5 - 2) - already;
	if (remaining < 1) {
		return false;
	}
	CastSummon(level, ch, obj, spell_id, 0);
	return true;
}

EStageResult CastToPoints(int level, CharData *ch, CharData *victim, ESpell spell_id) {
	int hit = 0; //если выставить больше нуля, то лечит
	int move = 0; //если выставить больше нуля, то реген хп
	bool extraHealing = false; //если true, то лечит сверх макс.хп

	if (victim == nullptr) {
		log("MAG_POINTS: Ошибка! Не указана цель, spell_id: %d!\r\n", to_underlying(spell_id));
		return EStageResult::kSuccess;
	}

	switch (spell_id) {
		// All hit-point heals share one block: the amount comes from each spell's <heal>/<amount>
		// and overheal-above-max is the data-driven <heal extra=> flag (applied after the switch),
		// so no spell needs its own case anymore. (kEviless is already gated upstream by
		// <required mob_flags="kCorpse"> + kTarMinionsOnly, then just heals its follower.)
		case ESpell::kCureLight:
		case ESpell::kCureSerious:
		case ESpell::kCureCritic:
		case ESpell::kHeal:
		case ESpell::kGroupHeal:
		case ESpell::kGreatHeal:
		case ESpell::kExtraHits:
		case ESpell::kPatronage:
		case ESpell::kWarcryOfPower:
		case ESpell::kEviless:
		case ESpell::kPaladineInspiration:
			hit = CalcHeal(ch, victim, spell_id, level);
			break;
		case ESpell::kResfresh:
		case ESpell::kGroupRefresh: move = victim->get_real_max_move() - victim->get_move();
			break;
		case ESpell::kFullFeed:
		case ESpell::kCommonMeal: {
			if (GET_COND(victim, THIRST) > 0)
				GET_COND(victim, THIRST) = 0;
			if (GET_COND(victim, FULL) > 0)
				GET_COND(victim, FULL) = 0;
		}
			break;
		default: log("MAG_POINTS: Ошибка! Передан неопределенный лечащий спелл spell_id: %d!\r\n",
					 to_underlying(spell_id));
			return EStageResult::kSuccess;
			break;
	}
	// Overheal above the maximum hit points is data-driven: a <heal extra="Y"> lets the heal push
	// hit points beyond the cap (e.g. kExtraHits). Off by default for every other heal.
	if (MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kHeal)) {
		extraHealing = MUD::Spell(spell_id).actions.GetHeal().IsExtra();
	}
	// issue #3304: сообщение цели лечащего заклинания берётся из spell_msg.xml.
	// Не у всех заклинаний есть такое сообщение (напр. kPatronage, kEviless) -
	// выводим только если оно задано именно для данного заклинания. Для лечащих
	// заклинаний (есть heal-экшен) сообщение показываем лишь когда лечение реально
	// произошло (hit != 0), иначе <heal prob=...> "лечил бы" текстом без эффекта.
	const bool is_heal = MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kHeal);
	const auto &points_sheaf = MUD::SpellMessages()[spell_id];
	if (points_sheaf.HasMessage(ESpellMsg::kPointsToVict) && (!is_heal || hit != 0)) {
		SendMsgToChar(points_sheaf.GetMessage(ESpellMsg::kPointsToVict) + "\r\n", victim);
	}
//	log("HEAL: до модификатора  Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
	hit = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, hit);

	if (hit && victim->GetEnemy() && ch != victim) {
		if (!pk_agro_action(ch, victim->GetEnemy()))
			return EStageResult::kSuccess;
	}
	// лечение
	if (victim->get_hit() < kMaxHits && hit != 0) {
		if (!extraHealing && victim->get_hit() < victim->get_real_max_hit()) {
			if (AFF_FLAGGED(victim, EAffect::kLacerations)) {
//				log("HEAL: порез Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
				victim->set_hit(std::min(victim->get_hit() + hit / 2, victim->get_real_max_hit()));
			} else {
//				log("HEAL: Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
				victim->set_hit(std::min(victim->get_hit() + hit, victim->get_real_max_hit()));
			}
		}
		if (extraHealing) {
//			log("HEAL: наддув Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
			if (victim->get_real_max_hit() <= 0) {
				victim->set_hit(std::max(victim->get_hit(), std::min(victim->get_hit() + hit, 1)));
			} else {
				victim->set_hit(std::clamp(victim->get_hit() + hit, victim->get_hit(),
						victim->get_real_max_hit() + victim->get_real_max_hit() * 33 / 100));
			}
		}
	}
	if (move != 0 && victim->get_move() < victim->get_real_max_move()) {
		victim->set_move(std::min(victim->get_move() + move, victim->get_real_max_move()));
	}
	update_pos(victim);

	return EStageResult::kSuccess;
}

namespace {
// Helpers for the data-driven CastUnaffects.

// True if `affect` carries at least one of `flags`. The <unaffect affect_flags=> set lists the
// EAffFlag bits (kAfCurable / kAfDispellable) an affect must have to be eligible for removal by
// that unaffect; this is the single source of truth for "can this be removed" (it replaced the old
// CheckNodispel blacklist). An affect with no matching flag -- charm/quest effects, or anything
// applied outside <affects> in code -- is irremovable.
bool AffectMatchesFlags(const Affect<EApply>::shared_ptr &affect, Bitvector flags) {
	return affect && IS_SET(affect->battleflag, flags);
}

// True if the victim carries a removable affect of the given spell type (one matching `flags`).
bool HasDispellableAffect(CharData *victim, ESpell spell, Bitvector flags) {
	for (const auto &aff : victim->affected) {
		if (aff && aff->type == spell && AffectMatchesFlags(aff, flags)) {
			return true;
		}
	}
	return false;
}

// Evaluate a <blocking>/<breaking> condition set (IsAffectedBySpell only, issue #3342):
// true if any any_of affect is present, or every all_of affect is present.
bool UnaffectConditionMet(CharData *victim, const talents_actions::TalentUnaffect::Set &set) {
	for (const auto spell : set.any_of) {
		if (IsAffectedBySpell(victim, spell)) {
			return true;
		}
	}
	if (!set.all_of.empty()) {
		bool all = true;
		for (const auto spell : set.all_of) {
			if (!IsAffectedBySpell(victim, spell)) {
				all = false;
				break;
			}
		}
		if (all) {
			return true;
		}
	}
	return false;
}

// One affect queued for dispel, tagged with its source block's breaking_by_failure flag: if the
// dispel of a candidate with break_on_fail set is resisted, the whole cast chain breaks.
struct RemovalCandidate {
	ESpell spell;
	bool break_on_fail;
};

// Build the list of affects to dispel for a <remove>/<remove_anyway> set:
//   - any_of (explicit list)   -> the first listed affect that is present and matches `flags`
//   - all_of (explicit list)   -> every listed affect that is present and matches `flags`
//   - wildcard_any (any_of="*")-> ONE eligible affect picked uniformly at random
//   - wildcard_all (all_of="*")-> EVERY eligible affect on the victim
// Each candidate carries the set's breaking_by_failure. The wildcards replaced the dedicated
// kDispellMagic code path (issue.dispell) and enable generic "strip-by-flag" dispels (e.g.
// future sphere-specific dispels added by tagging affects with kAfXSphere flags).
void CollectRemovals(CharData *victim, const talents_actions::TalentUnaffect::Set &set,
					 std::vector<RemovalCandidate> &out, Bitvector flags) {
	if (set.wildcard_any) {
		// Reservoir sample one eligible affect uniformly.
		ESpell pick = ESpell::kUndefined;
		int seen = 0;
		for (const auto &aff : victim->affected) {
			if (AffectMatchesFlags(aff, flags) && number(1, ++seen) == 1) {
				pick = aff->type;
			}
		}
		if (pick != ESpell::kUndefined) {
			out.push_back({pick, set.breaking_by_failure});
		}
	} else {
		for (const auto spell : set.any_of) {
			if (HasDispellableAffect(victim, spell, flags)) {
				out.push_back({spell, set.breaking_by_failure});
				break;
			}
		}
	}
	if (set.wildcard_all) {
		// Sweep every eligible affect once. Note: a victim may carry multiple affects of the
		// same spell type (different locations); each is queued so the per-candidate dispel
		// pipeline removes them all -- consistent with explicit all_of repeating spell names.
		for (const auto &aff : victim->affected) {
			if (AffectMatchesFlags(aff, flags)) {
				out.push_back({aff->type, set.breaking_by_failure});
			}
		}
	} else {
		for (const auto spell : set.all_of) {
			if (HasDispellableAffect(victim, spell, flags)) {
				out.push_back({spell, set.breaking_by_failure});
			}
		}
	}
}

// A successful dispel of a stacked affect peels one stack instead of removing it
//: for every affect of `spell` with stacks > 1, reduce the stack count by 1
// and the accumulated modifier proportionally (~modifier/stacks), re-applying so the character's
// stats update. If no affect of the spell has more than one stack, remove it outright.
void ReduceStackOrRemove(CharData *victim, ESpell spell) {
	bool any_multi = false;
	for (const auto &aff : victim->affected) {
		if (aff && aff->type == spell && aff->stacks > 1) {
			any_multi = true;
			break;
		}
	}
	if (!any_multi) {
		RemoveAffectFromCharAndRecalculate(victim, spell);
		return;
	}
	std::vector<Affect<EApply>> rebuilt;
	for (const auto &aff : victim->affected) {
		if (aff && aff->type == spell) {
			Affect<EApply> peeled = *aff;
			if (peeled.stacks > 1) {
				peeled.modifier = static_cast<int>(
						static_cast<int64_t>(peeled.modifier) * (peeled.stacks - 1) / peeled.stacks);
				peeled.stacks -= 1;
			}
			rebuilt.push_back(peeled);
		}
	}
	RemoveAffectFromChar(victim, spell);          // strip all of the spell's affects (deltas undone)
	for (auto &peeled : rebuilt) {
		affect_to_char(victim, peeled);           // re-add with the reduced modifier (stats recalced)
	}
}

// Remove one affect (or peel a stack of it) and show its own dispel message (keyed by the removed
// affect's spell, issues #3335/#3342); an affect whose sheaf has no such message shows nothing.
void RemoveAffectAndAnnounce(CharData *ch, CharData *victim, ESpell removed) {
	ReduceStackOrRemove(victim, removed);
	const auto &sheaf = MUD::SpellMessages()[removed];
	const auto &to_vict = sheaf.GetMessage(ESpellMsg::kAffDispelledToChar);
	if (!to_vict.empty()) {
		act(to_vict.c_str(), false, victim, nullptr, ch, kToChar);
	}
	const auto &to_room = sheaf.GetMessage(ESpellMsg::kAffDispelledToRoom);
	if (!to_room.empty()) {
		act(to_room.c_str(), true, victim, nullptr, ch, kToRoom | kToArenaListen);
	}
}

// issue: potency-gated dispel. True if `dispel_spell` should remove `affect_spell` from victim.
// Three cases drive whether a strength check is even rolled:
//   1. a violent dispel -> always check (any affect);
//   2. a non-violent dispel of a debuff -> check;
//   3. a non-violent dispel of a buff -> no check, always removed.
// The check itself: a flat 5% chance to remove regardless of strength, otherwise the dispel's
// potency -- (RollSkillDices + skill_coeff + stat_coeff) * potency_weight -- must exceed the
// affect's recorded potency (the strength of the cast that imposed it).
bool DispelSucceeds(CharData *ch, CharData *victim, ESpell dispel_spell, ESpell affect_spell,
					float potency_weight) {
	float affect_potency = 0.0f;
	bool affect_is_debuff = false;
	for (const auto &aff : victim->affected) {
		if (aff && aff->type == affect_spell) {
			affect_potency = aff->potency;
			affect_is_debuff = aff->debuff;
			break;
		}
	}
	// Case 3: a non-violent spell removing a buff needs no check.
	if (!MUD::Spell(dispel_spell).IsViolent() && !affect_is_debuff) {
		return true;
	}
	// Always a 5% chance to remove regardless of potency.
	if (number(1, 100) <= 5) {
		return true;
	}
	const auto &roll = MUD::Spell(dispel_spell).GetPotencyRoll();
	const float spell_potency = static_cast<float>(
			roll.RollSkillDices() + roll.CalcSkillCoeff(ch) + roll.CalcBaseStatCoeff(ch)) * potency_weight;
	return spell_potency > affect_potency;
}

}  // namespace

EStageResult CastUnaffects(int/* level*/, CharData *ch, CharData *victim, ESpell spell_id) {
	if (victim == nullptr) {
		return EStageResult::kSuccess;
	}

	// prob: percent chance the <unaffect> block fires at all (default 100). The prob<100 guard
	// short-circuits the RNG when the unaffect always fires.
	if (MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kUnaffect)) {
		const int unaff_prob = MUD::Spell(spell_id).actions.GetUnaffect().GetProb();
		if (unaff_prob < 100 && number(1, 100) > unaff_prob) {
			return EStageResult::kSuccess;
		}
	}

	// Fully data-driven path: the <unaffect> block says which affects block/break the cast and
	// which it removes. The any_of="*"/all_of="*" wildcards cover the kDispellMagic "pick one
	// random eligible affect" case (issue.dispell) and any future generic flag-filtered dispels.
	if (!MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kUnaffect)) {
		return EStageResult::kSuccess;
	}
	const auto &unaffect = MUD::Spell(spell_id).actions.GetUnaffect();

	// TODO(#3342): CastUnaffect has no saving (success) check yet. kEnergyDrain/kWeaknes
	// used to gate their kStrength/kDexterity removal behind a save in CastAffect; until
	// that check is added here the buff is stripped regardless of the save. See their
	// commented stub case in CastAffect.
	const bool blocking = UnaffectConditionMet(victim, unaffect.GetBlocking());
	const bool breaking = UnaffectConditionMet(victim, unaffect.GetBreaking());

	// breaking_by_failure (issue): a resisted dispel of a candidate from a remove/remove_anyway
	// block so flagged breaks the cast chain, in addition to the present-affect <breaking> set.
	bool break_chain = breaking;

	const Bitvector flags = unaffect.GetAffectFlags();
	std::vector<RemovalCandidate> to_remove;
	CollectRemovals(victim, unaffect.GetRemoveAnyway(), to_remove, flags);  // dispelled even when blocked
	if (!blocking) {
		CollectRemovals(victim, unaffect.GetRemove(), to_remove, flags);   // dispelled only when not blocked
	}

	if (!to_remove.empty()) {
		// PK-action check (preserved from the old switch): keyed on the first dispelled
		// affect's spell flags; a disallowed action aborts the removal entirely.
		const auto primary = to_remove.front().spell;
		if (ch != victim) {
			if (MUD::Spell(primary).IsFlagged(kNpcAffectNpc)) {
				if (!pk_agro_action(ch, victim)) {
					return EStageResult::kSuccess;
				}
			} else if (MUD::Spell(primary).IsFlagged(kNpcAffectPc) && victim->GetEnemy()) {
				if (!pk_agro_action(ch, victim->GetEnemy())) {
					return EStageResult::kSuccess;
				}
			}
		}
		bool removed_any = false;
		bool resisted_any = false;
		for (const auto &cand : to_remove) {
			if (DispelSucceeds(ch, victim, spell_id, cand.spell, unaffect.GetPotencyWeight())) {
				RemoveAffectAndAnnounce(ch, victim, cand.spell);
				removed_any = true;
			} else {
				resisted_any = true;
				if (cand.break_on_fail) {
					break_chain = true;
				}
			}
		}
		// Everything that could be removed resisted the potency check -> "no effect" to the caster.
		// Only for a pure dispel: a kMagAffects spell stays silent and still applies its own affect.
		if (!removed_any && resisted_any && !MUD::Spell(spell_id).IsFlagged(kMagAffects)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
	} else if (breaking
			|| (!MUD::Spell(spell_id).IsFlagged(kMagAffects)
				&& (!unaffect.GetRemove().empty() || !unaffect.GetRemoveAnyway().empty()))) {
		// "No effect" when a break stopped the cast, or when a *pure* dispel spell found
		// nothing to remove. An affect-applying spell (kMagAffects) whose removal simply
		// found nothing stays silent and goes on to apply its own affect.
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	}

	return break_chain ? EStageResult::kBreak : EStageResult::kSuccess;
}

// Try to enchant a weapon. Returns the to_char message to relay to the caster, or nullptr when
// the caller should fall through to the kNoeffect fallback. Side effects: may consume a reagent
// (a held magical symbol in MAGIC1/2/3_ENCHANT_VNUM containers), set the obj's enchant, and
// silently emit kEnchantSetItem when the item is part of a set. Caller is responsible for the
// (ch, obj) null guard.
static const char *EnchantWeapon(CharData *ch, ObjData *obj, ESpell spell_id) {
	// Either already enchanted or not a weapon.
	if (obj->get_type() != EObjType::kWeapon) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantNotWeapon).c_str();
	}
	if (obj->has_flag(EObjFlag::kMagic)) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantMagic).c_str();
	}
	if (obj->has_flag(EObjFlag::kSetItem)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantSetItem) + "\r\n", ch);
		return nullptr;
	}

	auto reagobj = GET_EQ(ch, EEquipPos::kHold);
	if (reagobj
		&& (GetObjByVnumInContent(GlobalDrop::MAGIC1_ENCHANT_VNUM, reagobj)
			|| GetObjByVnumInContent(GlobalDrop::MAGIC2_ENCHANT_VNUM, reagobj)
			|| GetObjByVnumInContent(GlobalDrop::MAGIC3_ENCHANT_VNUM, reagobj))) {
		// у нас имеется доп символ для зачарования
		obj->set_enchant(ch->GetSkill(ESkill::kLightMagic), reagobj);
		ProcessMatComponents(ch, reagobj->get_rnum(), spell_id); //может неправильный вызов
	} else {
		obj->set_enchant(ch->GetSkill(ESkill::kLightMagic));
	}
	if (GET_RELIGION(ch) == kReligionMono) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantMono).c_str();
	}
	if (GET_RELIGION(ch) == kReligionPoly) {
		return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantPoly).c_str();
	}
	return MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantOther).c_str();
}

// When `obj` is null but `victim` isn't, pick a random item from the victim's equipment/
// inventory. If neither obj nor victim is given there is nothing to act on -- the function
// exits without effect.
EStageResult CastToAlterObjs(CharData *ch, CharData *victim, ObjData *obj, ESpell spell_id) {
	const char *to_char = nullptr;

	if (obj == nullptr && victim != nullptr) {
		int rand = number(1, 50);
		if (rand <= EEquipPos::kBoths) {
			obj = GET_EQ(victim, rand);
		} else {
			for (rand -= EEquipPos::kBoths, obj = victim->carrying; rand && obj;
				 rand--, obj = obj->get_next_content());
		}
	}

	if (obj == nullptr) {
		return EStageResult::kSuccess;
	}

	if (obj->has_flag(EObjFlag::kNoalter)) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kObjResist).c_str(), true, ch, obj, nullptr, kToChar);
		return EStageResult::kSuccess;
	}

	switch (spell_id) {
		case ESpell::kBless:
			if (!obj->has_flag(EObjFlag::kBless)
				&& (obj->get_weight() <= 5 * GetRealLevel(ch))) {
				obj->set_extra_flag(EObjFlag::kBless);
				if (obj->has_flag(EObjFlag::kNodrop)) {
					obj->unset_extraflag(EObjFlag::kNodrop);
					if (obj->get_type() == EObjType::kWeapon) {
						obj->inc_val(2);
					}
				}
				obj->add_maximum(std::max(obj->get_maximum_durability() >> 2, 1));
				obj->set_current_durability(obj->get_maximum_durability());
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
				obj->add_timed_spell(ESpell::kBless, -1);
			}
			break;

		case ESpell::kCurse:
			if (!obj->has_flag(EObjFlag::kNodrop)) {
				obj->set_extra_flag(EObjFlag::kNodrop);
				if (obj->get_type() == EObjType::kWeapon) {
					if (GET_OBJ_VAL(obj, 2) > 0) {
						obj->dec_val(2);
					}
				} else if (ObjSystem::is_armor_type(obj)) {
					if (GET_OBJ_VAL(obj, 0) > 0) {
						obj->dec_val(0);
					}
					if (GET_OBJ_VAL(obj, 1) > 0) {
						obj->dec_val(1);
					}
				}
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			}
			break;

		case ESpell::kInvisible:
			if (!obj->has_flag(EObjFlag::kNoinvis)
				&& !obj->has_flag(EObjFlag::kInvisible)) {
				obj->set_extra_flag(EObjFlag::kInvisible);
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			}
			break;

		case ESpell::kPoison:
			if (!GET_OBJ_VAL(obj, 3)
				&& (obj->get_type() == EObjType::kLiquidContainer
					|| obj->get_type() == EObjType::kFountain
					|| obj->get_type() == EObjType::kFood)) {
				obj->set_val(3, 1);
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			}
			break;

		case ESpell::kRemoveCurse:
			if (obj->has_flag(EObjFlag::kNodrop)) {
				obj->unset_extraflag(EObjFlag::kNodrop);
				if (obj->get_type() == EObjType::kWeapon) {
					obj->inc_val(2);
				}
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			}
			break;

		case ESpell::kEnchantWeapon:
			// obj is already non-null (guarded above); ch is hypothetically nullable.
			if (ch == nullptr) {
				return EStageResult::kSuccess;
			}
			to_char = EnchantWeapon(ch, obj, spell_id);
			break;
		case ESpell::kRemovePoison:
			if (obj->get_rnum() < 0) {
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kRemovePoisonUnknown).c_str();
				char message[100];
				sprintf(message,
						"неизвестный прототип объекта : %s (VNUM=%d)",
						obj->get_PName(ECase::kNom).c_str(),
						obj->get_vnum());
				mudlog(message, BRF, kLvlBuilder, SYSLOG, 1);
				break;
			}
			if (obj_proto[obj->get_rnum()]->get_val(3) > 1 && GET_OBJ_VAL(obj, 3) == 1) {
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kRemovePoisonRotten).c_str();
				break;
			}
			if ((GET_OBJ_VAL(obj, 3) == 1)
				&& ((obj->get_type() == EObjType::kLiquidContainer)
					|| obj->get_type() == EObjType::kFountain
					|| obj->get_type() == EObjType::kFood)) {
				obj->set_val(3, 0);
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			}
			break;

		case ESpell::kFly: obj->add_timed_spell(ESpell::kFly, -1);
			obj->set_extra_flag(EObjFlag::kFlying);
			to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			break;

		case ESpell::kAcid:
		case ESpell::kAcidArrow: {
			// The corrode message is keyed on the cast spell and shown to the victim whose item is
			// being damaged (or the caster if there's no separate victim). Returning here skips the
			// standard caster-side to_char fallback (kNoeffect / kAlterObj).
			CharData *recipient = victim ? victim : ch;
			act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAcidCorrodeObj).c_str(),
				false, recipient, obj, nullptr, kToChar);
			DamageObj(obj, number(GetRealLevel(ch) * 2, GetRealLevel(ch) * 4), 100);
			return EStageResult::kSuccess;
		}

		case ESpell::kRepair: obj->set_current_durability(obj->get_maximum_durability());
			to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			break;

		case ESpell::kTimerRestore:
			if (obj->get_rnum() != kNothing) {
				obj->set_current_durability(obj->get_maximum_durability());
				obj->set_timer(obj_proto.at(obj->get_rnum())->get_timer());
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
				log("%s used magic repair", GET_NAME(ch));
			} else {
				return EStageResult::kSuccess;
			}
			break;

		case ESpell::kRestoration: {
			if (obj->has_flag(EObjFlag::kMagic)
				&& (obj->get_rnum() != kNothing)) {
				if (obj_proto.at(obj->get_rnum())->has_flag(EObjFlag::kMagic)) {
					return EStageResult::kSuccess;
				}
				obj->unset_enchant();
			} else {
				return EStageResult::kSuccess;
			}
			to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
		}
			break;

		case ESpell::kLight: obj->add_timed_spell(ESpell::kLight, -1);
			obj->set_extra_flag(EObjFlag::kGlow);
			to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAlterObjToChar).c_str();
			break;

		case ESpell::kDarkness:
			if (obj->timed_spell().check_spell(ESpell::kLight)) {
				obj->del_timed_spell(ESpell::kLight, true);
				return EStageResult::kSuccess;
			}
			break;
		default: break;
	} // switch

	if (to_char == nullptr) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	} else {
		act(to_char, true, ch, obj, nullptr, kToChar);
	}

	return EStageResult::kSuccess;
}

EStageResult CastCreation(int/* level*/, CharData *ch, ESpell spell_id) {
	ObjVnum obj_vnum;

	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}

	switch (spell_id) {
		case ESpell::kCreateFood: obj_vnum = kStartBread;
			break;

		case ESpell::kCreateLight: obj_vnum = kCreateLight;
			break;

		default: SendMsgToChar("Spell unimplemented, it would seem.\r\n", ch);
			return EStageResult::kSuccess;
			break;
	}

	const auto tobj = world_objects.create_from_prototype_by_vnum(obj_vnum);
	if (!tobj) {
		SendMsgToChar("Что-то не видно образа для создания.\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found", to_underlying(spell_id), obj_vnum);
		return EStageResult::kSuccess;
	}

	act("$n создал$g $o3.", false, ch, tobj.get(), nullptr, kToRoom | kToArenaListen);
	act("Вы создали $o3.", false, ch, tobj.get(), nullptr, kToChar);
	load_otrigger(tobj.get());

	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else if (ch->GetCarryingWeight() + tobj->get_weight() > CAN_CARRY_W(ch)) {
		SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else {
		PlaceObjToInventory(tobj.get(), ch);
	}

	return EStageResult::kSuccess;
}

// Dispatch for spells whose effect is a hand-coded handler in spells.cpp (the kMagManual flag).
// Some handlers take only (caster, cvict) and ignore the unused `level` / `ovict` arguments.
EStageResult CastManual(int level, CharData *caster, CharData *cvict, ObjData *ovict, ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kControlWeather: SpellControlWeather(level, caster, cvict, ovict);
			break;
		case ESpell::kCreateWater: SpellCreateWater(level, caster, cvict, ovict);
			break;
		case ESpell::kLocateObject: SpellLocateObject(level, caster, cvict, ovict);
			break;
		case ESpell::kCreateWeapon: SpellCreateWeapon(level, caster, cvict, ovict);
			break;
		case ESpell::kCharm: SpellCharm(level, caster, cvict, ovict);
			break;
		case ESpell::kEnergyDrain: SpellEnergydrain(level, caster, cvict, ovict);
			break;
		case ESpell::kMassFear:
		case ESpell::kFear: SpellFear(level, caster, cvict, ovict);
			break;
		case ESpell::kSacrifice: SpellSacrifice(level, caster, cvict, ovict);
			break;
		case ESpell::kIdentify: SpellIdentify(level, caster, cvict, ovict);
			break;
		case ESpell::kFullIdentify: SpellFullIdentify(level, caster, cvict, ovict);
			break;
		case ESpell::kHolystrike: SpellHolystrike(level, caster, cvict, ovict);
			break;
		case ESpell::kVampirism: SpellVampirism(level, caster, cvict, ovict);
			break;
		// Movement-style spells whose handlers take only (caster, cvict).
		case ESpell::kGroupRecall:
		case ESpell::kWorldOfRecall: SpellRecall(caster, cvict);
			break;
		case ESpell::kTeleport: SpellTeleport(caster, cvict);
			break;
		case ESpell::kSummon: SpellSummon(caster, cvict);
			break;
		case ESpell::kPortal: SpellPortal(caster, cvict);
			break;
		case ESpell::kRelocate: SpellRelocate(caster, cvict);
			break;
		default: return EStageResult::kSuccess;
	}
	return EStageResult::kSuccess;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

int CheckMobList(CharData *ch) {
	for (const auto &vict: character_list) {
		if (vict.get() == ch) {
			return (true);
		}
	}

	return (false);
}

// A reasonably-bright victim who can't see its invisible caster spends one of its own
// prepared spells trying to remedy that (detect invis -> sense life -> light, first
// available wins). The intellect check is "above 25, or a soft number(10,25) roll".
void MaybeAutoCastDetection(CharData *victim, CharData *caster) {
	if (CAN_SEE(victim, caster)) {
		return;
	}
	if (GetRealInt(victim) <= 25 && GetRealInt(victim) <= number(10, 25)) {
		return;
	}
	if (!AFF_FLAGGED(victim, EAffect::kDetectInvisible)
		&& GET_SPELL_MEM(victim, ESpell::kDetectInvis) > 0) {
		CastSpell(victim, victim, nullptr, nullptr, ESpell::kDetectInvis, ESpell::kDetectInvis);
	} else if (!AFF_FLAGGED(victim, EAffect::kDetectLife)
		&& GET_SPELL_MEM(victim, ESpell::kSenseLife) > 0) {
		CastSpell(victim, victim, nullptr, nullptr, ESpell::kSenseLife, ESpell::kSenseLife);
	} else if (!AFF_FLAGGED(victim, EAffect::kInfravision)
		&& GET_SPELL_MEM(victim, ESpell::kLight) > 0) {
		CastSpell(victim, victim, nullptr, nullptr, ESpell::kLight, ESpell::kLight);
	}
}

void ReactToCast(CharData *victim, CharData *caster, ESpell spell_id) {
	if (caster == victim)
		return;

	if (!CheckMobList(victim) || !MUD::Spell(spell_id).IsViolent())
		return;

	if (AFF_FLAGGED(victim, EAffect::kCharmed) ||
		AFF_FLAGGED(victim, EAffect::kSleep) ||
		AFF_FLAGGED(victim, EAffect::kBlind) ||
		AFF_FLAGGED(victim, EAffect::kStopFight) ||
		AFF_FLAGGED(victim, EAffect::kMagicStopFight) || AFF_FLAGGED(victim, EAffect::kHold)
		|| IS_HORSE(victim))
		return;

	if (caster->IsNpc() && caster->get_rnum() == GetMobRnum(kDgCasterProxy))
		return;

	if (CAN_SEE(victim, caster) && MAY_ATTACK(victim) && victim->in_room == caster->in_room) {
		if (victim->IsNpc())
			mob_ai::attack_best(victim, caster, false);
		else
			hit(victim, caster, ESkill::kUndefined, fight::kMainHand);
	} else if (CAN_SEE(victim, caster) && !caster->IsNpc() && victim->IsNpc()
		&& victim->IsFlagged(EMobFlag::kMemory)) {
		mob_ai::mobRemember(victim, caster);
	}

	if (caster->purged()) {
		return;
	}
	MaybeAutoCastDetection(victim, caster);
}

// <blocking>: true if the target carries ANY of the listed flags/affects
// (NPC mob flags matter only for NPCs; affect flags via AFF_FLAGGED for any target).
static bool TargetIsBlocked(CharData *victim, const talents_actions::FlagCondition &cond) {
	if (victim->IsNpc()) {
		for (const auto flag : cond.mob_flags) {
			if (victim->IsFlagged(flag)) {
				return true;
			}
		}
	}
	for (const auto aff : cond.affect_flags) {
		if (AFF_FLAGGED(victim, aff)) {
			return true;
		}
	}
	// align: blocks the cast when the target carries the matching
	// alignment (IsGood / IsEvil / IsNeutral). kAny means no alignment block.
	if (cond.align == EAlign::kGood && IsGood(victim)) {
		return true;
	}
	if (cond.align == EAlign::kEvil && IsEvil(victim)) {
		return true;
	}
	if (cond.align == EAlign::kNeutral && IsNeutral(victim)) {
		return true;
	}
	return false;
}

// <required>: true only if the target has ALL the listed flags/affects
// (a required mob flag implies the target is an NPC carrying it).
static bool TargetMeetsRequired(CharData *victim, const talents_actions::FlagCondition &cond) {
	for (const auto flag : cond.mob_flags) {
		if (!victim->IsNpc() || !victim->IsFlagged(flag)) {
			return false;
		}
	}
	for (const auto aff : cond.affect_flags) {
		if (!AFF_FLAGGED(victim, aff)) {
			return false;
		}
	}
	// align: require the target to carry the matching alignment
	// (IsGood / IsEvil / IsNeutral). kAny means no alignment requirement.
	if (cond.align == EAlign::kGood && !IsGood(victim)) {
		return false;
	}
	if (cond.align == EAlign::kEvil && !IsEvil(victim)) {
		return false;
	}
	if (cond.align == EAlign::kNeutral && !IsNeutral(victim)) {
		return false;
	}
	return true;
}

// True if `victim` matches any of the reflection's affect flags or its alignment.
// Bare flag/alignment match -- potency-gated reflection isn't possible today (mob/object
// affects carry no potency value), so flag presence + prob is the best we can do.
static bool VictimMatchesReflection(CharData *victim, const talents_actions::Reflection &refl) {
	for (const auto aff : refl.affect_flags) {
		if (AFF_FLAGGED(victim, aff)) return true;
	}
	if (refl.align == EAlign::kGood && IsGood(victim)) return true;
	if (refl.align == EAlign::kEvil && IsEvil(victim)) return true;
	if (refl.align == EAlign::kNeutral && IsNeutral(victim)) return true;
	return false;
}

// If the spell's <reflection> matches `cvict` and the prob roll succeeds, emit the three
// reflection messages and return `caster` -- the spell will now hit the caster instead.
// Otherwise return `cvict` unchanged. The redirection happens once for the whole cast (damage +
// affects + ...), not per stage. Self-casts never bounce.
static CharData *MaybeReflectToCaster(CharData *caster, CharData *cvict, ESpell spell_id) {
	if (!cvict || cvict == caster) {
		return cvict;
	}
	const auto &refl = MUD::Spell(spell_id).actions.GetReflection();
	if (refl.empty() || !VictimMatchesReflection(cvict, refl)) {
		return cvict;
	}
	if (number(1, 100) > refl.prob) {
		return cvict;
	}
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kReflectedToChar).c_str(),
		false, caster, nullptr, cvict, kToChar);
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kReflectedToVict).c_str(),
		false, caster, nullptr, cvict, kToVict);
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kReflectedToRoom).c_str(),
		false, caster, nullptr, cvict, kToNotVict | kToArenaListen);
	return caster;
}

int CastToSingleTarget(CharData *caster, CharData *cvict, ObjData *ovict, CastRollResult roll) {
	const ESpell spell_id = roll.spell_id;
	const int level = roll.level;
	// kTarMinionsOnly: castable only on one of the caster's own NPC followers (master == caster).
	// Checked per target so it covers group/mass casts too. A single-target cast on the wrong
	// target is refused with a message; group/mass casts just skip non-followers silently.
	if (MUD::Spell(spell_id).AllowTarget(kTarMinionsOnly)
			&& !(cvict && cvict->IsNpc() && cvict->get_master() == caster)) {
		if (!MUD::Spell(spell_id).IsFlagged(kMagGroups | kMagMasses | kMagAreas)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastNotMinion) + "\r\n", caster);
		}
		return 0;
	}
	// Action-level <blocking>/<required> gates: the cast is refused on a target
	// that carries a blocking flag/affect or lacks a required one. This sits here (not inside a
	// stage) so it gates the whole cast -- damage, affects, etc. Per target, so it covers group/
	// mass casts; for those a refusal stays silent (no per-target spam).
	if (cvict && (TargetIsBlocked(cvict, MUD::Spell(spell_id).actions.GetBlocking())
			|| !TargetMeetsRequired(cvict, MUD::Spell(spell_id).actions.GetRequired()))) {
		if (!MUD::Spell(spell_id).IsFlagged(kMagGroups | kMagMasses | kMagAreas)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
		}
		return 0;
	}
	// Action-level caster gate: mirrors the victim-side <blocking>,
	// but examines the CASTER. Used to refuse casts the caster cannot wield -- e.g. kDispelEvil
	// gets <caster_blocking align="kEvil"/> so an evil caster simply can't fire it. Always emits
	// kNoeffect (no group/mass silent skip: a caster-side block concerns the one caster, not the
	// per-target loop).
	if (TargetIsBlocked(caster, MUD::Spell(spell_id).actions.GetCasterBlocking())) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
		return 0;
	}
	cvict = MaybeReflectToCaster(caster, cvict, spell_id);
	if (cvict && (caster != cvict))
		// The level-difference half of this guard is commented out: after
		// proper balancing it should be moot -- a low-level mage can't land a strong buff,
		// can't penetrate a debuff's saving throw, and damage now scales with (low) skill.
		// Kept for quick reactivation if some unforeseen case needs it:
		//     || (((GetRealLevel(cvict) / 2) > (GetRealLevel(caster) + (GetRealRemort(caster) / 2))) && !caster->IsNpc())
		if (cvict->IsGod() /* level-diff condition disabled, see above */) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
			return (-1);
		}

	if (!cast_mtrigger(cvict, caster, spell_id)) {
		return -1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagWarcry) && cvict && IS_UNDEAD(cvict))
		return 1;

	if (MUD::Spell(spell_id).IsFlagged(kMagDamage))
		if (CastDamage(level, caster, cvict, spell_id) == -1)
			return (-1);    // Successful and target died, don't cast again.

	// Unaffect runs before affect: a spell strips/blocks existing affects
	// first and may break the chain, before applying any new affect of its own.
	if (MUD::Spell(spell_id).IsFlagged(kMagUnaffects)
			&& CastUnaffects(abs(level), caster, cvict, spell_id) == EStageResult::kBreak) {
		return 1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagAffects)
			&& CastAffect(abs(level), caster, cvict, spell_id, roll.potency) == EStageResult::kBreak) {
		return 1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagPoints)
			&& CastToPoints(level, caster, cvict, spell_id) == EStageResult::kBreak) {
		return 1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagAlterObjs)
			&& CastToAlterObjs(caster, cvict, ovict, spell_id) == EStageResult::kBreak) {
		return 1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagSummons)
			&& CastSummon(abs(level), caster, ovict, spell_id, true) == EStageResult::kBreak) {
		return 1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagCreations)
			&& CastCreation(abs(level), caster, spell_id) == EStageResult::kBreak) {
		return 1;
	}

	if (MUD::Spell(spell_id).IsFlagged(kMagManual)
			&& CastManual(abs(level), caster, cvict, ovict, spell_id) == EStageResult::kBreak) {
		return 1;
	}

	ReactToCast(cvict, caster, spell_id);
	return 1;
}

// Сообщения массовых/площадных заклинаний вынесены в lib/cfg/spell_msg.xml
//: kAreaToChar / kAreaToRoom / kAreaToVict, доступны через
// MUD::SpellMessages(). См. CallMagicToArea / CallMagicToGroup.

void TrySendCastMessages(CharData *ch, CharData *victim, RoomData *room, ESpell spell_id) {
	if (room && world[ch->in_room] == room && IsAbleToSay(ch)) {
		const auto &sheaf = MUD::SpellMessages()[spell_id];
		if (sheaf.HasMessage(ESpellMsg::kAreaToChar)) {
			// вот тут надо воткнуть проверку на группу.
			act(sheaf.GetMessage(ESpellMsg::kAreaToChar).c_str(), false, ch, nullptr, victim, kToChar);
		}
		if (sheaf.HasMessage(ESpellMsg::kAreaToRoom)) {
			act(sheaf.GetMessage(ESpellMsg::kAreaToRoom).c_str(), false, ch, nullptr, victim, kToRoom | kToArenaListen);
		}
	}
};

int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, CastRollResult roll) {
	const ESpell spell_id = roll.spell_id;
	int level = roll.level;     // mutated by the per-target level decay below
	if (ch == nullptr || ch->in_room == kNowhere) {
		return 0;
	}
	try {
		const auto params = MUD::Spell(spell_id).actions.GetArea();
		ActionTargeting::FoesRosterType roster{ch, victim,
											   [](CharData *, CharData *target) {
												   return !IS_HORSE(target);
											   }};
		TrySendCastMessages(ch, victim, room, spell_id);
		int targets_num = params.CalcTargetsQuantity(ch->GetSkill(GetMagicSkillId(spell_id)));
		int targets_counter = 1;
		int lvl_decay = 0;
		double cast_decay = 0.0;
		if (CanUseFeat(ch, EFeat::kMultipleCast)) {
			cast_decay = params.cast_decay * 0.6;
			lvl_decay = std::max(0, params.level_decay - 1);
		} else {
			cast_decay = params.cast_decay;
			lvl_decay = params.level_decay;
		}
		const int kCasterCastSuccess = GET_CAST_SUCCESS(ch);

		const auto &area_sheaf = MUD::SpellMessages()[spell_id];
		const bool has_vict_msg = area_sheaf.HasMessage(ESpellMsg::kAreaToVict);
		const std::string vict_msg = has_vict_msg ? area_sheaf.GetMessage(ESpellMsg::kAreaToVict) : std::string{};
		for (const auto &target: roster) {
			if (has_vict_msg && target->desc) {
				act(vict_msg.c_str(), false, ch, nullptr, target, kToVict);
			}
			roll.level = level;
			CastToSingleTarget(ch, target, nullptr, roll);
			if (ch->purged()) {
				return 1;
			}
			if (!ch->IsNpc()) {
				++targets_counter;
				if (targets_counter > params.free_targets) {
					int tax = kCasterCastSuccess * cast_decay * (targets_counter - params.free_targets);
					GET_CAST_SUCCESS(ch) = std::max(-200, kCasterCastSuccess - tax);
					level = std::max(1, level - lvl_decay);
					if (ch->IsFlagged(EPrf::kTester)) {
						SendMsgToChar(ch,
									  "&GМакс. целей: %d, Каст: %d, Уровень заклинания: %d.&n\r\n",
									  targets_num,
									  GET_CAST_SUCCESS(ch),
									  level);
					}
				};
			};
			if (targets_counter >= targets_num) {
				break;
			}
		}

		GET_CAST_SUCCESS(ch) = kCasterCastSuccess;
	} catch (std::runtime_error &e) {
		err_log("%s", e.what());
	}
	return 1;
}

// Применение заклинания к группе в комнате
//---------------------------------------------------------
int CallMagicToGroup(CharData *ch, CastRollResult roll) {
	const ESpell spell_id = roll.spell_id;
	if (ch == nullptr) {
		return 0;
	}

	TrySendCastMessages(ch, nullptr, world[ch->in_room], spell_id);

	ActionTargeting::FriendsRosterType roster{ch, ch};
	roster.flip();
	for (const auto target: roster) {
		CastToSingleTarget(ch, target, nullptr, roll);
	}
	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
