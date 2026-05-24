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

#include "engine/core/action_targeting.h"
#include "gameplay/affects/affect_handler.h"
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
#include "gameplay/mechanics/groups.h"
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
			if (ch->IsOnHorse())
				save -= 20;
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
	int save = CalcSaving(killer, victim, type, true);
	int rnd = number(-200, 200);
	char smallbuf[256];
	if (number(1, 100) <=5) { //абсолютный фейл
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


bool IsBreath(ESpell spell_id) {
	static const std::set<ESpell> magic_breath {
	 	ESpell::kFireBreath,
	 	ESpell::kGasBreath,
	 	ESpell::kFrostBreath,
	 	ESpell::kAcidBreath,
	 	ESpell::kLightingBreath
	};

	return magic_breath.contains(spell_id);
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

int CalcHeal(CharData *ch, CharData *victim, ESpell spell_id, int level) {
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
	auto spell_heal = MUD::Spell(spell_id).actions.GetHeal();
	const auto &potency_roll = MUD::Spell(spell_id).GetPotencyRoll();
	int total_heal{0};
	double skill_mod;

	double base_heal = potency_roll.RollSkillDices();
	if (level >= 0) {
		skill_mod = base_heal * potency_roll.CalcSkillCoeff(ch);
	} else {
		skill_mod = base_heal * abs(level) / 0.25;
	}
//	mudlog(fmt::format("Хиляем level = {}, skill_mod = {}", level, skill_mod));
	double stat_mod = base_heal * potency_roll.CalcBaseStatCoeff(ch);
	double bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
	total_heal = static_cast<int>(base_heal + skill_mod + stat_mod);
	total_heal += static_cast<int>(total_heal * bonus_mod);
	double npc_heal = spell_heal.GetNpcCoeff();
	if (ch->IsNpc()) {
		total_heal += static_cast<int>(total_heal * npc_heal);
	}

	ch->send_to_TC(false, true, true,
		"&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Wis: %2.2f, Bonus: %1.2f, NPC coeff: %f, Total: %d &n\r\n",
		GET_NAME(victim),
		base_heal,
		skill_mod,
		stat_mod,
		1 + bonus_mod,
		npc_heal,
		total_heal);

	victim->send_to_TC(false, true, true,
			"&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Wis: %2.2f, Bonus: %1.2f, NPC coeff: %f, Total: %d &n\r\n",
			GET_NAME(ch),
			base_heal,
			skill_mod,
			stat_mod,
			bonus_mod,
			npc_heal,
			total_heal);

	return total_heal;
}

int CalcTotalSpellDmg(CharData *ch, CharData *victim, ESpell spell_id) {
	const auto &potency_roll = MUD::Spell(spell_id).GetPotencyRoll();
	int total_dmg{0};
	if (number(1, 100) > std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, GET_MR(victim))) {
		float base_dmg = CalcBaseDmg(ch, spell_id);
		float skill_mod = base_dmg * potency_roll.CalcSkillCoeff(ch);
		float wis_mod = base_dmg * potency_roll.CalcBaseStatCoeff(ch);
		float bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
//		auto complex_mod = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, base_dmg) - base_dmg;
		float elem_coeff = CalcMagicElementCoeff(victim, spell_id);

		total_dmg = static_cast<int>((base_dmg + skill_mod + wis_mod) * elem_coeff);
		total_dmg += static_cast<int>(total_dmg * bonus_mod);
		int complex_mod = total_dmg;
		total_dmg = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, total_dmg);
		complex_mod = total_dmg - complex_mod;
		ch->send_to_TC(false, true, true,
				"&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Wis: %2.2f, Bonus: %1.2f, Cmplx: %d, Poison: %1.2f, Elem.coeff: %1.2f, Total: %d &n\r\n",
				GET_NAME(victim),
				base_dmg,
				skill_mod,
				wis_mod,
				1 + bonus_mod,
				complex_mod,
				elem_coeff,
				total_dmg);

		victim->send_to_TC(false, true, true,
				"&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Wis: %2.2f, Bonus: %1.2f, Cmplx: %d, Poison: %1.2f, Elem.coeff: %1.2f, Total: %d &n\r\n",
				GET_NAME(ch),
				base_dmg,
				skill_mod,
				wis_mod,
				bonus_mod,
				complex_mod,
				elem_coeff,
				total_dmg);
	}

	return total_dmg;
}

int CastDamage(int level, CharData *ch, CharData *victim, ESpell spell_id) {
	int rand = 0, count = 1, modi = 0;
	ObjData *obj = nullptr;

	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr)
		return (0);

	if (!pk_agro_action(ch, victim))
		return (0);
	log("[MAG DAMAGE] %s damage %s (%d)", GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
	// Magic glass
	if (!IsBreath(spell_id) || ch == victim) {
		if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry)) {
			if (ch != victim && spell_id <= ESpell::kLast &&
				((AFF_FLAGGED(victim, EAffect::kMagicGlass) && number(1, 100) < (GetRealLevel(victim) / 3)))) {
				act("Магическое зеркало $N1 отразило вашу магию!", false, ch, nullptr, victim, kToChar);
				act("Магическое зеркало $N1 отразило магию $n1!", false, ch, nullptr, victim, kToNotVict);
				act("Ваше магическое зеркало отразило поражение $n1!", false, ch, nullptr, victim, kToVict);
				log("[MAG DAMAGE] Зеркало - полное отражение: %s damage %s (%d)",
					GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
				return (CastDamage(level, ch, ch, spell_id));
			}
		} else {
			if (ch != victim && spell_id <= ESpell::kLast && victim->IsGod()
				&& (ch->IsNpc() || GetRealLevel(victim) > GetRealLevel(ch))) {
				act("Звуковой барьер $N1 отразил ваш крик!", false, ch, nullptr, victim, kToChar);
				act("Звуковой барьер $N1 отразил крик $n1!", false, ch, nullptr, victim, kToNotVict);
				act("Ваш звуковой барьер отразил крик $n1!", false, ch, nullptr, victim, kToVict);
				return (CastDamage(level, ch, ch, spell_id));
			}
		}

		if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) && AFF_FLAGGED(victim, EAffect::kShadowCloak)
			&& spell_id <= ESpell::kLast && number(1, 100) < 21) {
			act("Густая тень вокруг $N1 жадно поглотила вашу магию.", false, ch, nullptr, victim, kToChar);
			act("Густая тень вокруг $N1 жадно поглотила магию $n1.", false, ch, nullptr, victim, kToNotVict);
			act("Густая тень вокруг вас поглотила магию $n1.", false, ch, nullptr, victim, kToVict);
			log("[MAG DAMAGE] Мантия  - поглощение урона: %s damage %s (%d)",
				GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
			return 0;
		}
		// Блочим маг дамагу от директ спелов для Витязей : шанс (скил/20 + вес.щита/2) ~ 30% при 200 скила и 40вес щита
		if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) &&
			!MUD::Spell(spell_id).IsFlagged(kMagMasses) &&
			!MUD::Spell(spell_id).IsFlagged(kMagAreas) &&
			(victim->GetSkill(ESkill::kShieldBlock) > 100) && GET_EQ(victim, kShield) &&
			CanUseFeat(victim, EFeat::kMagicalShield) &&
			(number(1, 100)	<
			((victim->GetSkill(ESkill::kShieldBlock)) / 20 + GET_EQ(victim, kShield)->get_weight() / 2))) {
			act("Ловким движением щита $N отразил вашу магию.", false, ch, nullptr, victim, kToChar);
			act("Ловким движением щита $N отразил магию $n1.", false, ch, nullptr, victim, kToNotVict);
			act("Вы отразили своим щитом магию $n1.", false, ch, nullptr, victim, kToVict);
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

	auto instant_death{false};
	// issue #3304: часть сообщений вынесена в lib/cfg/spell_msg.xml
	// (kAcid/kClod/kStunning/kVacuum/kArrows*; общий kKnockdown* в kDefault).
	// TODO: kAcidArrow (случайные эффекты), kDispelEvil/Good и kHolystrike
	// остаются в коде - они завязаны на игровую логику; защитные реакции
	// (зеркало/барьер/мантия/щит) тоже намеренно оставлены в коде.
	switch (spell_id) {
		case ESpell::kMagicMissile: {
			if (CanUseFeat(ch, EFeat::kMagicArrows))
				count = (level + 9) / 5;
			else
				count = (level + 9) / 10;
			break;
		}
		case ESpell::kLightingBolt: {
				count += ((number(1, 5) == 1) ? 1 : 0);
				count = std::min(count, 4);
			break;
		}
		case ESpell::kWhirlwind: {
				count += ((number(1, 7) == 1) ? 1 : 0);
				count = std::min(count, 4);
			break;
		}
		case ESpell::kAcid: {
			obj = nullptr;
			if (victim->IsNpc()) {
				rand = number(1, 50);
				if (rand <= EEquipPos::kBoths) {
					obj = GET_EQ(victim, rand);
				} else {
					for (rand -= EEquipPos::kBoths, obj = victim->carrying; rand && obj;
						 rand--, obj = obj->get_next_content());
				}
			}
			if (obj) {
				// уберем костыльное снижение урона за бессмысленностью - все ранво мало кто пользуктся
				// лучше потом реалиховать нормальную механику модификацию урона от обстоятельств
				// spell_dmg.value().dice_size = spell_dmg.value().dice_size * 2 / 3;
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAcidCorrodeObj).c_str(), false, victim, obj, nullptr, kToChar);
				DamageObj(obj, number(level * 2, level * 4), 100);
			}
			break;
		}
		case ESpell::kClod: {
				if (victim->GetPosition() > EPosition::kSit && !victim->IsImmortal() && (number(1, 100) > GET_AR(victim)) &&
					(AFF_FLAGGED(victim, EAffect::kHold) || !CalcGeneralSaving(ch, victim, ESaving::kReflex, modi))) {
				if (IS_HORSE(victim))
					victim->DropFromHorse();
				victim->SetPosition(EPosition::kSit);
				victim->DropFromHorse();
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kLaggedToRoom).c_str(), false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kLaggedToChar).c_str(), false, victim, nullptr, nullptr, kToChar);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kAcidArrow: {
	      // шанс доп аффекта 25% при 100% магии, 45 - 200, 85 -400..
			if (number(1, 100)>(5 + (ch->GetSkill(GetMagicSkillId(spell_id))/5))) { 
			 	break;
			 }
			int rnd = number(1, 5);
			switch (rnd) {
				case 1: // обожгло глотку - молча
				act("Кислота плеснула на горло $n1", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Жуткая кислота опалила ваше горло!", false, victim, nullptr, nullptr, kToChar);
				CastAffect(level, ch, victim, ESpell::kSilence); 
					break;
				case 2: // телесный ожог - лихорадка
				act("$n покрылся язвами по всему телу.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Кислота причинила вам жуткие ожоги кожи!", false, victim, nullptr, nullptr, kToChar);
				CastAffect(level, ch, victim, ESpell::kFever); 
					break;
				case 3: // ядовитые испарения - яд
				act("$n позеленел от действия кислотной стрелы.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Кислота обожгла вам все тело!", false, victim, nullptr, nullptr, kToChar);
				CastAffect(level, ch, victim, ESpell::kPoison); 
					break;
				case 4: // обожгло глаза - слепота
				act("Часть кислоты попала в глаза $n3.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Кислотные испарения выедают вам глаза!", false, victim, nullptr, nullptr, kToChar);
				CastAffect(level, ch, victim, ESpell::kBlindness); 
					break;
				case 5: // обожгот экип - кислотой
				act("Кислота покрыла доспехи $n3.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Кислота покрыла ваши доспехи.", false, victim, nullptr, nullptr, kToChar);
				CastDamage(level, ch, victim, ESpell::kAcid); 
					break;
				default:break;
			}	
			break;
		}
		case ESpell::kEarthquake: {
			if (ch->IsOnHorse()) {
				rand = number(1, 100);
				if (rand > 95)
					break;
				if (rand < 5 || (CalcCurrentSkill(victim, ESkill::kRiding, nullptr) * number(1, 6))
					< ch->GetSkill(ESkill::kEarthMagic) * number(1, 6)) {//фейл
					ch->DropFromHorse();
					break;
				}
			}
			if (victim->GetPosition() > EPosition::kSit && !victim->IsImmortal() && (number(1, 100) > GET_AR(victim)) &&
					(AFF_FLAGGED(victim, EAffect::kHold) || !CalcGeneralSaving(ch, victim, ESaving::kReflex, modi))) {
				victim->SetPosition(EPosition::kSit);
				victim->DropFromHorse();
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToRoom).c_str(), false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToChar).c_str(), false, victim, nullptr, nullptr, kToChar);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kSonicWave: {
			if (victim->GetPosition() > EPosition::kSit &&
				!victim->IsImmortal() && (number(1, 100) > GET_AR(victim)) 
						&& (AFF_FLAGGED(victim, EAffect::kHold) || !CalcGeneralSaving(ch, victim, ESaving::kStability, modi))) {
				victim->SetPosition(EPosition::kSit);
				victim->DropFromHorse();
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToRoom).c_str(), false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToChar).c_str(), false, victim, nullptr, nullptr, kToChar);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kStunning: {
			if (ch == victim ||
					((number(1, 100) > GET_AR(victim)) && !CalcGeneralSaving(ch, victim, ESaving::kCritical, modi))) {
				int choice_stunning = 750;
				if (CanUseFeat(ch, EFeat::kDarkPact))
					choice_stunning -= GetRealRemort(ch) * 15;
				if (number(1, 999) > choice_stunning) {
					act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToChar).c_str(), false, ch, nullptr, victim, kToChar);
					act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToNotVict).c_str(), false, ch, nullptr, victim, kToNotVict);
					act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToVict).c_str(), false, ch, nullptr, victim, kToVict);
				if (victim->GetPosition() > EPosition::kStun) {
					victim->SetPosition(EPosition::kStun);
					victim->DropFromHorse();
				}
					SetWaitState(victim, (5 + (GetRealWis(ch) - 20) / 6) * kBattleRound);
				}
			}
			break;
		}
		case ESpell::kVacuum: {
			if (!IsAffectedBySpell(victim, spell_id)) {
				if (ch == victim ||
						((number(0, 100) <= 20) && (number(1, 100) > GET_AR(victim))
								&& !CalcGeneralSaving(ch, victim, ESaving::kCritical, modi))) {
					act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToChar).c_str(), false, ch, nullptr, victim, kToChar);
//					act("$n0 упал$q без сознания!", true, victim, nullptr, ch, kToRoom | kToArenaListen);
					act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToNotVict).c_str(), false, ch, nullptr, victim, kToNotVict);
					act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToVict).c_str(), false, ch, nullptr, victim, kToVict);
					auto wait = 4 + std::max(1, GetRealLevel(ch) + (GetRealWis(ch) - 29)) / 7;
					Affect<EApply> af;
					af.type = spell_id;
					af.affect_type = EAffect::kMagicStopFight;
					af.modifier = 0;
					af.duration = wait * kBattleRound;
					ch->send_to_TC(false, true, true, "Круг пустоты длительность = %d пульсов.\r\n", af.duration);
					af.battleflag = kAfPulsedec;
					af.location = EApply::kNone;
					ImposeAffect(victim, af);
					if (victim->GetPosition() > EPosition::kStun) {
						victim->SetPosition(EPosition::kStun);
						victim->DropFromHorse();
					}
//			SetWaitState(victim, wait * kBattleRound);
				}
			}
			break;
		}
		case ESpell::kDispelEvil: {
			if (ch != victim && IS_EVIL(ch) && !ch->IsImmortal() && ch->get_hit() > 1) {
				SendMsgToChar("Ваша магия обратилась против вас.", ch);
				ch->set_hit(1);
			}
			if (!IS_EVIL(victim)) {
				if (victim != ch)
					act("Боги защитили $N3 от вашей магии.", false, ch, nullptr, victim, kToChar);
				return 0;
			};
			break;
		}
		case ESpell::kDispelGood: {
			if (ch != victim && IS_GOOD(ch) && !ch->IsImmortal() && ch->get_hit() > 1) {
				SendMsgToChar("Ваша магия обратилась против вас.", ch);
				ch->set_hit(1);
			}
			if (!IS_GOOD(victim)) {
				if (victim != ch)
					act("Боги защитили $N3 от вашей магии.", false, ch, nullptr, victim, kToChar);
				return 0;
			};
			break;
		}
		case ESpell::kSacrifice: {
			if (victim->IsImmortal())
				break;
			break;
		}
		case ESpell::kDustStorm: {
			if (victim->GetPosition() > EPosition::kSit &&
				!victim->IsImmortal() && (number(1, 100) > GET_AR(victim)) &&
				(!CalcGeneralSaving(ch, victim, ESaving::kReflex, modi))) {
				victim->DropFromHorse();
				victim->SetPosition(EPosition::kSit);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToRoom).c_str(), false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToChar).c_str(), false, victim, nullptr, nullptr, kToChar);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kHolystrike: {
			if (AFF_FLAGGED(victim, EAffect::kForcesOfEvil)) {
				if (CalcGeneralSaving(ch, victim, ESaving::kWill, modi)) {
					act("Черное облако вокруг вас нейтрализовало действие тумана, растворившись в нем.",
						false, victim, nullptr, nullptr, kToChar);
					act("Черное облако вокруг $n1 нейтрализовало действие тумана.",
						false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
					RemoveAffectFromChar(victim, ESpell::kEviless);
				} else {
					instant_death = true;
				}
			}
			break;
		}
		case ESpell::kWarcryOfThunder: {
			if (victim->GetPosition() > EPosition::kSit && !victim->IsImmortal() && (AFF_FLAGGED(victim, EAffect::kHold) ||
				!CalcGeneralSaving(ch, victim, ESaving::kStability, modi))) {
				victim->DropFromHorse();
				victim->SetPosition(EPosition::kSit);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToRoom).c_str(), false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToChar).c_str(), false, victim, nullptr, nullptr, kToChar);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kArrowsFire:
		case ESpell::kArrowsWater:
		case ESpell::kArrowsEarth:
		case ESpell::kArrowsAir:
		case ESpell::kArrowsDeath: {
			if (!ch->IsNpc()) {
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToChar).c_str(), false, ch, nullptr, victim, kToChar);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToNotVict).c_str(), false, ch, nullptr, victim, kToNotVict);
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kDamageToVict).c_str(), false, ch, nullptr, victim, kToVict);
			}
			break;
		}
		default: break;
	}
	int total_dmg{0};

	if (instant_death) {
		total_dmg = std::max(1, victim->get_hit() + 1);
		if (victim->IsNpc()) {
			total_dmg += 99;
		}
	} else {
		try {
			total_dmg = CalcTotalSpellDmg(ch, victim, spell_id);
		} catch (std::exception &e) {
			err_log("%s", e.what());
		}
	}
	total_dmg = std::clamp(total_dmg, 0, kMaxHits);

	for (; count > 0 && rand >= 0; count--) {
		if (ch->in_room != kNowhere
			&& victim->in_room != kNowhere
			&& victim->in_room == ch->in_room
			&& ch->GetPosition() > EPosition::kStun
			&& victim->GetPosition() > EPosition::kDead) {
			// инит полей для дамага
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
			rand = dmg.Process(ch, victim);
		}
	}
	return rand;
}

bool ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id) {
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

		default: log("WARNING: wrong spell_id %d in %s:%d", to_underlying(spell_id), __FILE__, __LINE__);
			return false;
	}
	ObjData *tobj = GetObjByVnumInContent(vnum, caster->carrying);
	if (!tobj) {
		act(missing, false, victim, nullptr, caster, kToChar);
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
// EAffFlag update flags (issue #3334): kAfAccumulateDuration adds to an existing
// affect's duration, kAfUpdateDuration refreshes it to the longer value, and
// kAfUpdateMod replaces the modifier only when the new magnitude is larger. The
// caller runs affect_total() afterwards.
static void ApplyTalentAffect(CharData *victim, Affect<EApply> &af, Bitvector flags) {
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
			if (update_mod && std::abs(existing->modifier) > std::abs(af.modifier)) {
				af.modifier = existing->modifier;
			}
			victim->AffectRemove(it);
			break;
		}
	}
	affect_to_char(victim, af);
}

int CastAffect(int level, CharData *ch, CharData *victim, ESpell spell_id, const RollResult &potency) {
	bool accum_affect = false, accum_duration = false, success = true;
	bool update_spell = false;
	const char *to_vict = nullptr, *to_room = nullptr;
	int i, modi = 0;
	int rnd = 0;
	int decline_mod = 0;
	if (victim == nullptr
		|| victim->in_room == kNowhere
		|| ch == nullptr) {
		return 0;
	}

	// Calculate PKILL's affects
	if (ch != victim) {
		if (MUD::Spell(spell_id).IsFlagged(kNpcAffectPc)) {
			if (!pk_agro_action(ch, victim)) {
				return 0;
			}
		} else if (MUD::Spell(spell_id).IsFlagged(kNpcAffectNpc) && victim->GetEnemy())	{
			if (!pk_agro_action(ch, victim->GetEnemy()))
				return 0;
		}
	}
	// Magic glass
	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry)) {
		if (ch != victim
			&& MUD::Spell(spell_id).IsViolent()
			&& ((!ch->IsGod()
				&& AFF_FLAGGED(victim, EAffect::kMagicGlass)
				&& (ch->in_room == victim->in_room)
				&& number(1, 100) < (GetRealLevel(victim) / 3))
				|| (victim->IsGod()
					&& (ch->IsNpc()
						|| GetRealLevel(victim) > (GetRealLevel(ch)))))) {
			act("Магическое зеркало $N1 отразило вашу магию!", false, ch, nullptr, victim, kToChar);
			act("Магическое зеркало $N1 отразило магию $n1!", false, ch, nullptr, victim, kToNotVict);
			act("Ваше магическое зеркало отразило поражение $n1!", false, ch, nullptr, victim, kToVict);
			CastAffect(level, ch, ch, spell_id);
			return 0;
		}
	} else {
		if (ch != victim && MUD::Spell(spell_id).IsViolent() && victim->IsGod()
			&& (ch->IsNpc() || GetRealLevel(victim) > (GetRealLevel(ch) + GetRealRemort(ch) / 2))) {
			act("Звуковой барьер $N1 отразил ваш крик!", false, ch, nullptr, victim, kToChar);
			act("Звуковой барьер $N1 отразил крик $n1!", false, ch, nullptr, victim, kToNotVict);
			act("Ваш звуковой барьер отразил крик $n1!", false, ch, nullptr, victim, kToVict);
			CastAffect(level, ch, ch, spell_id);
			return 0;
		}
	}
	//  блочим директ аффекты вредных спелов для Витязей  шанс = (скил/20 + вес.щита/2)  ()
	if (ch != victim && MUD::Spell(spell_id).IsViolent() && !MUD::Spell(spell_id).IsFlagged(kMagWarcry)
		&& !MUD::Spell(spell_id).IsFlagged(kMagMasses) && !MUD::Spell(spell_id).IsFlagged(kMagAreas)
		&& (victim->GetSkill(ESkill::kShieldBlock) > 100) && GET_EQ(victim, EEquipPos::kShield)
		&& CanUseFeat(victim, EFeat::kMagicalShield)
		&& (number(1, 100) < ((victim->GetSkill(ESkill::kShieldBlock)) / 20
			+ GET_EQ(victim, EEquipPos::kShield)->get_weight() / 2))) {
		act("Ваши чары повисли на щите $N1, и затем развеялись.", false, ch, nullptr, victim, kToChar);
		act("Щит $N1 поглотил злые чары $n1.", false, ch, nullptr, victim, kToNotVict);
		act("Ваш щит уберег вас от злых чар $n1.", false, ch, nullptr, victim, kToVict);
		return (0);
	}

	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) && ch != victim && MUD::Spell(spell_id).IsViolent()
		&& number(1, 999) <= GET_AR(victim) * 10) {
		SendMsgToChar(NOEFFECT, ch);
		return 0;
	}

	Affect<EApply> af[kMaxSpellAffects];
	for (i = 0; i < kMaxSpellAffects; i++) {
		af[i].type = spell_id;
		af[i].affect_type = EAffect::kUndefinded;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = EApply::kNone;
	}

	// decrease modi for failing, increese fo success
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
	}

	if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc()) {
		modi = modi - 50;
	}


	auto savetype{ESaving::kStability};
	switch (spell_id) {
		// The affect itself now comes from the <affects> talent action (issue #3334);
		// only the cast messages remain here. The saving throw and application are
		// done by the talent-affect block at the end of this function.
		case ESpell::kChillTouch:
			to_room = "Боевой пыл $n1 несколько остыл.";
			to_vict = "Вы почувствовали себя слабее!";
			break;

		case ESpell::kEnergyDrain:
		case ESpell::kWeaknes: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			if (IsAffectedBySpell(victim, ESpell::kStrength)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kStrength);
				success = false;
				break;
			}
			if (IsAffectedBySpell(victim, ESpell::kDexterity)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kDexterity);
				success = false;
				break;
			}
			to_room = "$n стал$g немного слабее.";
			to_vict = "Вы почувствовали себя слабее!";
			break;
		case ESpell::kStoneWall:
		case ESpell::kStoneSkin:
			to_room = "Кожа $n1 покрылась каменными пластинами.";
			to_vict = "Вы стали менее чувствительны к ударам.";
			break;

		case ESpell::kGeneralRecovery:
		case ESpell::kFastRegeneration:
			to_room = "$n расцвел$g на ваших глазах.";
			to_vict = "Вас наполнила живительная сила.";
			break;

		case ESpell::kAirShield:
			if (IsAffectedBySpell(victim, ESpell::kIceShield)) {
				RemoveAffectFromChar(victim, ESpell::kIceShield);
			}
			if (IsAffectedBySpell(victim, ESpell::kFireShield)) {
				RemoveAffectFromChar(victim, ESpell::kFireShield);
			}
			to_room = "$n3 окутал воздушный щит.";
			to_vict = "Вас окутал воздушный щит.";
			break;

		case ESpell::kFireShield:
			if (IsAffectedBySpell(victim, ESpell::kIceShield))
				RemoveAffectFromChar(victim, ESpell::kIceShield);
			if (IsAffectedBySpell(victim, ESpell::kAirShield))
				RemoveAffectFromChar(victim, ESpell::kAirShield);
			to_room = "$n3 окутал огненный щит.";
			to_vict = "Вас окутал огненный щит.";
			break;

		case ESpell::kIceShield:
			if (IsAffectedBySpell(victim, ESpell::kFireShield))
				RemoveAffectFromChar(victim, ESpell::kFireShield);
			if (IsAffectedBySpell(victim, ESpell::kAirShield))
				RemoveAffectFromChar(victim, ESpell::kAirShield);
			to_room = "$n3 окутал ледяной щит.";
			to_vict = "Вас окутал ледяной щит.";
			break;

		case ESpell::kAirAura:
			to_room = "$n3 окружила воздушная аура.";
			to_vict = "Вас окружила воздушная аура.";
			break;

		case ESpell::kEarthAura:
			to_room = "$n глубоко поклонил$u земле.";
			to_vict = "Глубокий поклон тебе, матушка земля.";
			break;

		case ESpell::kFireAura:
			to_room = "$n3 окружила огненная аура.";
			to_vict = "Вас окружила огненная аура.";
			break;

		case ESpell::kIceAura:
			to_room = "$n3 окружила ледяная аура.";
			to_vict = "Вас окружила ледяная аура.";
			break;

		case ESpell::kGroupCloudly:
		case ESpell::kCloudly:
			to_room = "Очертания $n1 расплылись и стали менее отчетливыми.";
			to_vict = "Ваше тело стало прозрачным, как туман.";
			break;
		case ESpell::kGroupArmor:
		case ESpell::kArmor:
			to_room = "Вокруг $n1 вспыхнул белый щит и тут же погас.";
			to_vict = "Вы почувствовали вокруг себя невидимую защиту.";
			break;

		case ESpell::kFascination:
			if (ProcessMatComponents(ch, victim, spell_id)) {
				success = false;
				break;
			}
			to_room =
				"$n0 достал$g из маленькой сумочки какие-то вонючие порошки и отвернул$u, бормоча под нос \r\n\"..так это на ресницы надо, кажется... Эх, только бы не перепутать...\" \r\n";
			to_vict =
				"Вы попытались вспомнить уроки старой цыганки, что учила вас людям головы морочить.\r\nХотя вы ее не очень то слушали.\r\n";
			spell_id = ESpell::kFascination;
			break;

		case ESpell::kGroupBless:
		case ESpell::kBless:
			to_room = "$n осветил$u на миг неземным светом.";
			to_vict = "Боги одарили вас своей улыбкой.";
			break;

		case ESpell::kCallLighting:
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
			}
			spell_id = ESpell::kMagicBattle;
			to_room = "$n зашатал$u, пытаясь прийти в себя от взрыва шаровой молнии.";
			to_vict = "Взрыв шаровой молнии $N1 отдался в вашей голове громким звоном.";
			break;

		case ESpell::kColdWind:
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
			}
			to_vict = "Вы покрылись серебристым инеем.";
			to_room = "$n покрыл$u красивым серебристым инеем.";
			break;
		case ESpell::kGroupAwareness:
		case ESpell::kAwareness:
			to_room = "$n начал$g внимательно осматриваться по сторонам.";
			to_vict = "Вы стали более внимательны к окружающему.";
			break;

		case ESpell::kGodsShield:
			to_room = "$n покрыл$u сверкающим коконом.";
			to_vict = "Вас покрыл голубой кокон.";
			break;

		case ESpell::kGroupHaste:
		case ESpell::kHaste:
			if (IsAffectedBySpell(victim, ESpell::kSlowdown)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kSlowdown);
				success = false;
				break;
			}
			to_vict = "Вы начали двигаться быстрее.";
			to_room = "$n начал$g двигаться заметно быстрее.";
			break;

		case ESpell::kShadowCloak:
			to_room = "$n скрыл$u в густой тени.";
			to_vict = "Густые тени окутали вас.";
			break;

		case ESpell::kEnlarge:
			if (IsAffectedBySpell(victim, ESpell::kLessening)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kLessening);
				success = false;
				break;
			}
			to_room = "$n начал$g расти, как на дрожжах.";
			to_vict = "Вы стали крупнее.";
			break;

		case ESpell::kLessening:
			if (IsAffectedBySpell(victim, ESpell::kEnlarge)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kEnlarge);
				success = false;
				break;
			}
			af[0].location = EApply::kSize;
			af[0].modifier = -(5 + level / 3);
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			accum_duration = true;
			to_room = "$n скукожил$u.";
			to_vict = "Вы стали мельче.";
			break;

		case ESpell::kMagicGlass:
		case ESpell::kGroupMagicGlass:
			to_room = "$n3 покрыла зеркальная пелена.";
			to_vict = "Вас покрыло зеркало магии.";
			break;

		case ESpell::kCloudOfArrows:
			to_room = "$n3 окружило облако летающих огненных стрел.";
			to_vict = "Вас окружило облако летающих огненных стрел.";
			break;

		case ESpell::kStoneHands:
			to_room = "Руки $n1 задубели.";
			to_vict = "Ваши руки задубели.";
			break;

		case ESpell::kGroupPrismaticAura:
		case ESpell::kPrismaticAura:
			if (!ch->IsNpc() && !group::same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			if (IsAffectedBySpell(victim, ESpell::kSanctuary)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kSanctuary);
				success = false;
				break;
			}
			if (AFF_FLAGGED(victim, EAffect::kSanctuary)) {
				success = false;
				break;
			}
			af[0].affect_type = EAffect::kPrismaticAura;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			accum_duration = true;
			to_room = "$n3 покрыла призматическая аура.";
			to_vict = "Вас покрыла призматическая аура.";
			spell_id = ESpell::kPrismaticAura;
			break;

		case ESpell::kMindless:
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = EApply::kManaRegen;
			af[0].modifier = -50;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim,
													  0,
													  GetRealWis(ch) + GetRealInt(ch),
													  10,
													  0,
													  0));
			af[1].location = EApply::kCastSuccess;
			af[1].modifier = -50;
			af[1].duration = af[0].duration;
			af[2].location = EApply::kHitroll;
			af[2].modifier = -5;
			af[2].duration = af[0].duration;

			to_room = "$n0 стал$g слаб$g на голову!";
			to_vict = "Ваш разум помутился!";
			break;

		case ESpell::kDustStorm:
		case ESpell::kShineFlash:
		case ESpell::kMassBlindness:
		case ESpell::kPowerBlindness:
		case ESpell::kBlindness: savetype = ESaving::kStability;
			if (victim->IsFlagged(EMobFlag::kNoBlind) ||
				victim->IsImmortal() ||
				((ch != victim) &&
					!GET_GOD_FLAG(victim, EGf::kGodscurse) && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			switch (spell_id) {
				case ESpell::kDustStorm:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 3, level, 6, 0, 0));
					break;
				case ESpell::kShineFlash:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level + 7, 8, 0, 0));
					break;
				case ESpell::kMassBlindness:
				case ESpell::kBlindness:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level, 8, 0, 0));
					break;
				case ESpell::kPowerBlindness:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 3, level, 6, 0, 0));
					break;
				default: break;
			}
			af[0].affect_type = EAffect::kBlind;
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 ослеп$q!";
			to_vict = "Вы ослепли!";
			spell_id = ESpell::kBlindness;
			break;

		case ESpell::kMadness: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 3, 0, 0, 0, 0));
			af[0].affect_type = EAffect::kNoFlee;
			af[1].location = EApply::kMadness;
			af[1].duration = af[0].duration;
			af[1].modifier = level;
			to_room = "Теперь $n не сможет сбежать из боя!";
			to_vict = "Вас обуяло безумие!";
			break;

		case ESpell::kWeb: savetype = ESaving::kReflex;
			if (AFF_FLAGGED(victim, EAffect::kBrokenChains)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = EApply::kHitroll;
			af[0].modifier = -2 - GetRealRemort(ch) / 5;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 3, level, 6, 0, 0));
			af[0].battleflag = kAfBattledec;
			af[0].affect_type = EAffect::kNoFlee;
			af[1].location = EApply::kAc;
			af[1].modifier = 20;
			af[1].duration = af[0].duration;
			af[1].battleflag = kAfBattledec;
			af[1].affect_type = EAffect::kNoFlee;
			to_room = "$n3 покрыла невидимая паутина, сковывая $s движения!";
			to_vict = "Вас покрыла невидимая паутина!";
			break;

		case ESpell::kMassCurse:
		case ESpell::kCurse: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			// если есть фит порча
			if (CanUseFeat(ch, EFeat::kCorruption))
				decline_mod += GetRealRemort(ch);
			af[0].location = EApply::kInitiative;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 1, level, 2, 0, 0));
			af[0].modifier = -(5 + decline_mod);
			af[0].affect_type = EAffect::kCurse;

			af[1].location = EApply::kHitroll;
			af[1].duration = af[0].duration;
			af[1].modifier = -(level / 6 + decline_mod + GetRealRemort(ch) / 5);
			af[1].affect_type = EAffect::kCurse;

			if (level >= 20) {
				af[2].location = EApply::kCastSuccess;
				af[2].duration = af[0].duration;
				af[2].modifier = -(level / 3 + GetRealRemort(ch));
				if (ch->IsNpc() && level >= (kLvlImmortal))
					af[2].modifier += (kLvlImmortal - level - 1);    //1 cast per mob level above 30
				af[2].affect_type = EAffect::kCurse;
			}
			accum_duration = true;
			accum_affect = true;
			to_room = "Красное сияние вспыхнуло над $n4 и тут же погасло!";
			to_vict = "Боги сурово поглядели на вас.";
			spell_id = ESpell::kCurse;
			break;

		case ESpell::kMassSlow:
		case ESpell::kSlowdown: savetype = ESaving::kStability;
			if (AFF_FLAGGED(victim, EAffect::kBrokenChains)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			if (IsAffectedBySpell(victim, ESpell::kHaste)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kHaste);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 9, 0, 0, 0, 0));
			af[0].affect_type = EAffect::kSlow;
			af[1].duration =
				ApplyResist(victim, GetResistType(spell_id), CalcDuration(victim, 9, 0, 0, 0, 0));
			af[1].location = EApply::kDex;
			af[1].modifier = -1 - GetRealRemort(ch) / 5;
			to_room = "Движения $n1 заметно замедлились.";
			to_vict = "Ваши движения заметно замедлились.";
			spell_id = ESpell::kSlowdown;
			break;

		case ESpell::kGroupSincerity:
		case ESpell::kDetectAlign:
			to_vict = "Ваши глаза приобрели зеленый оттенок.";
			to_room = "Глаза $n1 приобрели зеленый оттенок.";
			break;

		case ESpell::kAllSeeingEye:
		case ESpell::kDetectInvis:
			to_vict = "Ваши глаза приобрели золотистый оттенок.";
			to_room = "Глаза $n1 приобрели золотистый оттенок.";
			break;

		case ESpell::kMagicalGaze:
		case ESpell::kDetectMagic:
			to_vict = "Ваши глаза приобрели желтый оттенок.";
			to_room = "Глаза $n1 приобрели желтый оттенок.";
			break;

		case ESpell::kSightOfDarkness:
		case ESpell::kInfravision:
			to_vict = "Ваши глаза приобрели красный оттенок.";
			to_room = "Глаза $n1 приобрели красный оттенок.";
			break;

		case ESpell::kSnakeEyes:
		case ESpell::kDetectPoison:
			to_vict = "Ваши глаза приобрели карий оттенок.";
			to_room = "Глаза $n1 приобрели карий оттенок.";
			break;

		case ESpell::kGroupInvisible:
		case ESpell::kInvisible:
			if (!victim)
				victim = ch;
			if (IsAffectedBySpell(victim, ESpell::kGlitterDust)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			af[0].modifier = -40;
			af[0].location = EApply::kAc;
			af[0].affect_type = EAffect::kInvisible;
			accum_duration = true;
			to_vict = "Вы стали невидимы для окружающих.";
			to_room = "$n медленно растворил$u в пустоте.";
			spell_id = ESpell::kInvisible;
			break;

		case ESpell::kFever: savetype = ESaving::kStability;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].location = EApply::kHpRegen;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 0, level, 2, 0, 0));
			af[0].modifier = -95;
			af[1].location = EApply::kManaRegen;
			af[1].duration = af[0].duration;
			af[1].modifier = -95;
			af[2].location = EApply::kMoveRegen;
			af[2].duration = af[0].duration;
			af[2].modifier = -95;
			af[3].location = EApply::kPlague;
			af[3].duration = af[0].duration;
			af[3].modifier = level;
			af[4].location = EApply::kWis;
			af[4].duration = af[0].duration;
			af[4].modifier = -GetRealRemort(ch) / 5;
			af[5].location = EApply::kInt;
			af[5].duration = af[0].duration;
			af[5].modifier = -GetRealRemort(ch) / 5;
			af[6].location = EApply::kDex;
			af[6].duration = af[0].duration;
			af[6].modifier = -GetRealRemort(ch) / 5;
			af[7].location = EApply::kStr;
			af[7].duration = af[0].duration;
			af[7].modifier = -GetRealRemort(ch) / 5;
			to_vict = "Вас скрутило в жестокой лихорадке.";
			to_room = "$n3 скрутило в жестокой лихорадке.";
			break;

		case ESpell::kPoison: savetype = ESaving::kCritical;
			if (ch != victim && (AFF_FLAGGED(victim, EAffect::kGodsShield) ||
				CalcGeneralSaving(ch, victim, savetype, modi))) {
				if (ch->in_room == victim->in_room) {
					SendMsgToChar(NOEFFECT, ch);
				}
				success = false;
				break;
			}
			af[0].location = EApply::kStr;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id), CalcDuration(victim, 0, level, 1, 0, 0));
			af[0].modifier = -2;
			af[0].affect_type = EAffect::kPoisoned;
			af[0].battleflag = kAfSameTime;

			af[1].location = EApply::kPoison;
			af[1].duration = af[0].duration;
			af[1].modifier = level + GetRealRemort(ch) / 2;
			af[1].affect_type = EAffect::kPoisoned;
			af[1].battleflag = kAfSameTime;

			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";

			break;

		case ESpell::kProtectFromEvil:
		case ESpell::kGroupProtectFromEvil:
			if (!ch->IsNpc() && !group::same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			af[0].location = EApply::kResistDark;
			if (spell_id == ESpell::kProtectFromEvil) {
				RemoveAffectFromChar(ch, ESpell::kGroupProtectFromEvil);
				af[0].modifier = 5;
			} else {
				RemoveAffectFromChar(ch, ESpell::kProtectFromEvil);
				af[0].modifier = level;
			}
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			af[0].affect_type = EAffect::kProtectFromDark;
			accum_duration = true;
			to_vict = "Вы подавили в себе страх к тьме.";
			to_room = "$n подавил$g в себе страх к тьме.";
			break;

		case ESpell::kGroupSanctuary:
		case ESpell::kSanctuary:
			if (!ch->IsNpc() && !group::same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			if (IsAffectedBySpell(victim, ESpell::kPrismaticAura)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kPrismaticAura);
				success = false;
				break;
			}
			if (AFF_FLAGGED(victim, EAffect::kPrismaticAura)) {
				success = false;
				break;
			}

			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			af[0].affect_type = EAffect::kSanctuary;
			to_vict = "Белая аура мгновенно окружила вас.";
			to_room = "Белая аура покрыла $n3 с головы до пят.";
			spell_id = ESpell::kSanctuary;
			break;

		case ESpell::kSleep: savetype = ESaving::kWill;
			if (AFF_FLAGGED(victim, EAffect::kHold) || victim->IsFlagged(EMobFlag::kNoSleep)
				|| (ch != victim && CalcGeneralSaving(ch, victim, ESaving::kWill, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			};

			if (victim->GetEnemy())
				stop_fighting(victim, false);
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 1, level, 6, 1, 6));
			af[0].affect_type = EAffect::kSleep;
			af[0].battleflag = kAfBattledec;
			if (victim->GetPosition() > EPosition::kSleep && success) {
				if (victim->IsOnHorse()) {
					victim->DropFromHorse();
				}
				SendMsgToChar("Вы слишком устали... Спать... Спа...\r\n", victim);
				act("$n прилег$q подремать.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);

				victim->SetPosition(EPosition::kSleep);
			}
			break;

		case ESpell::kGroupStrength:
		case ESpell::kStrength:
			if (IsAffectedBySpell(victim, ESpell::kWeaknes)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kWeaknes);
				success = false;
				break;
			}
			af[0].location = EApply::kStr;
			af[0].duration = CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + GetRealRemort(ch) / 5;
			else
				af[0].modifier = (level + 14) / 15 + GetRealRemort(ch) / 5;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Вы почувствовали себя сильнее.";
			to_room = "Мышцы $n1 налились силой.";
			spell_id = ESpell::kStrength;
			break;

		case ESpell::kDexterity:
			if (IsAffectedBySpell(victim, ESpell::kWeaknes)) {
				RemoveAffectFromCharAndRecalculate(victim, ESpell::kWeaknes);
				success = false;
				break;
			}
			af[0].location = EApply::kDex;
			af[0].duration =
					CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + GetRealRemort(ch) / 5;
			else
				af[0].modifier = (level + 14) / 15 + GetRealRemort(ch) / 5;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Вы почувствовали себя более шустрым.";
			to_room = "$n0 будет двигаться более шустро.";
			spell_id = ESpell::kDexterity;
			break;

		case ESpell::kPatronage: af[0].location = EApply::kHp;
			af[0].duration = CalcDuration(victim, 3, level, 10, 0, 0);
			af[0].modifier = GetRealLevel(ch) * 2 + GetRealRemort(ch);
			if (GET_ALIGNMENT(victim) >= 0) {
				to_vict = "Исходящий с небес свет на мгновение озарил вас.";
				to_room = "Исходящий с небес свет на мгновение озарил $n3.";
			} else {
				to_vict = "Вас окутало клубящееся облако Тьмы.";
				to_room = "Клубящееся темное облако на мгновение окутало $n3.";
			}
			break;

		case ESpell::kEyeOfGods:
		case ESpell::kSenseLife: to_vict = "Вы способны разглядеть даже микроба.";
			to_room = "$n0 начал$g замечать любые движения.";
			break;

		case ESpell::kWaterwalk:
			to_vict = "На рыбалку вы можете отправляться без лодки.";
			break;

		case ESpell::kBreathingAtDepth:
		case ESpell::kWaterbreath:
			to_vict = "У вас выросли жабры.";
			to_room = "У $n1 выросли жабры.";
			break;

		case ESpell::kHolystrike:
			if (AFF_FLAGGED(victim, EAffect::kForcesOfEvil)) {
				// все решится в дамадже части спелла
				success = false;
				break;
			}
			// тут break не нужен

			// fall through
		case ESpell::kMassHold:
		case ESpell::kPowerHold:
		case ESpell::kHold:
			if (victim->IsFlagged(EMobFlag::kNoHold)
					|| AFF_FLAGGED(victim, EAffect::kBrokenChains)
					|| AFF_FLAGGED(victim, EAffect::kSleep)
					|| (ch != victim && CalcGeneralSaving(ch, victim, ESaving::kWill, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spell_id), spell_id == ESpell::kPowerHold ?
					CalcDuration(victim, 2, level + 7, 8, 2, 5) : CalcDuration(victim, 1, level + 9, 10, 1, 3));
			af[0].affect_type = EAffect::kHold;
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 замер$q на месте!";
			to_vict = "Вы замерли на месте, не в силах пошевельнуться.";
			spell_id = ESpell::kHold;
			break;

		case ESpell::kWarcryOfRage:
		case ESpell::kSonicWave:
		case ESpell::kMassDeafness:
		case ESpell::kPowerDeafness:
		case ESpell::kDeafness:
			switch (spell_id) {
				case ESpell::kWarcryOfRage: savetype = ESaving::kWill;
					modi = GetRealCon(ch);
					break;
				case ESpell::kSonicWave:
				case ESpell::kMassDeafness:
				case ESpell::kPowerDeafness:
				case ESpell::kDeafness: savetype = ESaving::kStability;
					break;
				default: break;
			}
			if (  //victim->IsFlagged(MOB_NODEAFNESS) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			switch (spell_id) {
				case ESpell::kWarcryOfRage:
				case ESpell::kPowerDeafness:
				case ESpell::kSonicWave:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
							CalcDuration(victim, 2, level + 3, 4, 6, 0));
					break;
				case ESpell::kMassDeafness:
				case ESpell::kDeafness:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
							CalcDuration(victim, 2, level + 7, 8, 3, 0));
					break;
				default: break;
			}
			af[0].affect_type = EAffect::kDeafness;
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 оглох$q!";
			to_vict = "Вы оглохли.";
			spell_id = ESpell::kDeafness;
			break;

		case ESpell::kMassSilence:
		case ESpell::kPowerSilence:
		case ESpell::kSilence: savetype = ESaving::kWill;
			if (victim->IsFlagged(EMobFlag::kNoSilence) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spell_id), spell_id == ESpell::kPowerSilence ?
					CalcDuration(victim, 2, level + 3, 4, 6, 0) : CalcDuration(victim, 2, level + 7, 8, 3, 0));
			af[0].affect_type = EAffect::kSilence;
			af[0].battleflag = kAfBattledec;
			to_room = "$n0 прикусил$g язык!";
			to_vict = "Вы не в состоянии вымолвить ни слова.";
			spell_id = ESpell::kSilence;
			break;

		case ESpell::kGroupFly:
		case ESpell::kFly:
			to_room = "$n0 медленно поднял$u в воздух.";
			to_vict = "Вы медленно поднялись в воздух.";
			break;

		case ESpell::kBrokenChains:
			to_room = "Ярко-синий ореол вспыхнул вокруг $n1 и тут же угас.";
			to_vict = "Волна ярко-синего света омыла вас с головы до ног.";
			break;
		case ESpell::kGroupBlink:
		case ESpell::kBlink:
			to_room = "$n начал$g мигать.";
			to_vict = "Вы начали мигать.";
			break;

		case ESpell::kMagicShield:
			to_room = "Сверкающий щит вспыхнул вокруг $n1 и угас.";
			to_vict = "Сверкающий щит вспыхнул вокруг вас и угас.";
			break;

		case ESpell::kNoflee: // "приковать противника"
		case ESpell::kIndriksTeeth:
		case ESpell::kSnare:
			savetype = ESaving::kWill;
			if (AFF_FLAGGED(victim, EAffect::kBrokenChains)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			to_room = "$n0 теперь прикован$a к $N2.";
			to_vict = "Вы не сможете покинуть $N3.";
			break;

		case ESpell::kLight:
			if (!ch->IsNpc() && !group::same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			to_room = "$n0 начал$g светиться ярким светом.";
			to_vict = "Вы засветились, освещая комнату.";
			break;

		case ESpell::kDarkness:
			if (!ch->IsNpc() && !group::same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			to_room = "$n0 погрузил$g комнату во мрак.";
			to_vict = "Вы погрузили комнату в непроглядную тьму.";
			break;
		case ESpell::kVampirism:
			to_room = "Зрачки $n3 приобрели красный оттенок.";
			to_vict = "Ваши зрачки приобрели красный оттенок.";
			break;

		case ESpell::kEviless:
			if (!victim->IsNpc() || victim->get_master() != ch || !victim->IsFlagged(EMobFlag::kCorpse)) {
				//тихо уходим, т.к. заклинание массовое
				break;
			}
			af[0].duration = CalcDuration(victim, 10, GetRealRemort(ch), 1, 0, 0);
			af[0].location = EApply::kDamroll;
			af[0].modifier = 15 + (GetRealRemort(ch) > 8 ? (GetRealRemort(ch) - 8) : 0);
			af[0].affect_type = EAffect::kForcesOfEvil;
			af[1].duration = af[0].duration;
			af[1].location = EApply::kHitroll;
			af[1].modifier = 7 + (GetRealRemort(ch) > 8 ? (GetRealRemort(ch) - 8) : 0);;
			af[1].affect_type = EAffect::kForcesOfEvil;
			af[2].duration = af[0].duration;
			af[2].location = EApply::kHp;
			af[2].affect_type = EAffect::kForcesOfEvil;

			// иначе, при рекасте, модификатор суммируется с текущим аффектом.
			if (!AFF_FLAGGED(victim, EAffect::kForcesOfEvil)) {
				af[2].modifier = victim->get_real_max_hit();
				// не очень красивый способ передать сигнал на лечение в mag_points
				Affect<EApply> tmpaf;
				tmpaf.type = ESpell::kEviless;
				tmpaf.duration = 1;
				tmpaf.modifier = 0;
				tmpaf.location = EApply::kNone;
				tmpaf.battleflag = 0;
				tmpaf.affect_type = EAffect::kForcesOfEvil;
				affect_to_char(ch, tmpaf);
			}
			to_vict = "Черное облако покрыло вас.";
			to_room = "Черное облако покрыло $n3 с головы до пят.";
			break;

		case ESpell::kWarcryOfThunder:
		case ESpell::kIceStorm:
		case ESpell::kEarthfall:
		case ESpell::kShock: {
/*			switch (spell_id) {
				case ESpell::kWarcryOfThunder: savetype = ESaving::kWill;
//					modi = GetRealCon(ch) * 3 / 2;
					break;
				case ESpell::kIceStorm: savetype = ESaving::kReflex;
//					modi = CALC_SUCCESS(modi, 30);
					break;
				case ESpell::kEarthfall: savetype = ESaving::kReflex;
//					modi = CALC_SUCCESS(modi, 95);
					break;
				case ESpell::kShock: savetype = ESaving::kReflex;
					break;
				default: break;
			}
*/
			if (spell_id==ESpell::kEarthfall){
				modi += ch->GetSkill(GetMagicSkillId(spell_id))/5;
			}
			if (victim->IsImmortal() || (!ch->IsImmortal() && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			switch (spell_id) {
				case ESpell::kWarcryOfThunder: af[0].type = ESpell::kDeafness;
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
							 CalcDuration(victim, 2, level + 3, 4, 6, 0));
					af[0].duration = CalcComplexSpellMod(ch, ESpell::kDeafness, GAPPLY_SPELL_EFFECT, af[0].duration);
					af[0].affect_type = EAffect::kDeafness;
					af[0].battleflag = kAfBattledec;
					to_room = "$n0 оглох$q!";
					to_vict = "Вы оглохли.";
					if ((victim->IsNpc()
						&& AFF_FLAGGED(victim, af[0].affect_type))
						|| (ch != victim
							&& IsAffectedBySpell(victim, ESpell::kDeafness))) {
						if (ch->in_room == victim->in_room) {
							SendMsgToChar(NOEFFECT, ch);
						}
					} else {
						ImposeAffect(victim, af[0], accum_duration, false, accum_affect, false);
						act(to_vict, false, victim, nullptr, ch, kToChar);
						act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);
					}
					break;

				case ESpell::kIceStorm:
				case ESpell::kEarthfall: SetWaitState(victim, 2 * kBattleRound);
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
							CalcDuration(victim, 2, 0, 0, 0, 0));
					af[0].affect_type = EAffect::kMagicStopFight;
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					to_room = "$n3 оглушило.";
					to_vict = "Вас оглушило.";
					spell_id = ESpell::kMagicBattle;
					break;

				case ESpell::kShock:
					SetWaitState(victim, 2 * kBattleRound);
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
							CalcDuration(victim, 2, 0, 0, 0, 0));
					af[0].affect_type = EAffect::kMagicStopFight;
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					to_room = "$n3 оглушило.";
					to_vict = "Вас оглушило.";
					spell_id = ESpell::kMagicBattle;
					CastAffect(level, ch, victim, ESpell::kBlindness);
					break;
				default: break;
			}
			break;
		}

		case ESpell::kCrying: {
			if (AFF_FLAGGED(victim, EAffect::kCrying)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = EApply::kHp;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
					CalcDuration(victim, 4, 0, 0, 0, 0));
			af[0].modifier =
				-1 * std::max(1,
						 (std::min(29, GetRealLevel(ch)) - std::min(24, GetRealLevel(victim)) +
							 GetRealRemort(ch) / 3) * victim->get_max_hit() / 100);
			af[0].affect_type = EAffect::kCrying;
			if (victim->IsNpc()) {
				af[1].location = EApply::kLikes;
				af[1].duration = ApplyResist(victim, GetResistType(spell_id),
						CalcDuration(victim, 5, 0, 0, 0, 0));
				af[1].modifier = -1 * std::max(1, ((level + 9) / 2 + 9 - GetRealLevel(victim) / 2));
				af[1].affect_type = EAffect::kCrying;
				af[1].battleflag = kAfBattledec;
				to_room = "$n0 издал$g протяжный стон.";
				break;
			}
			af[1].location = EApply::kCastSuccess;
			af[1].duration = ApplyResist(victim, GetResistType(spell_id),
					CalcDuration(victim, 5, 0, 0, 0, 0));
			af[1].modifier = -1 * std::max(1, (level / 3 + GetRealRemort(ch) / 3 - GetRealLevel(victim) / 10));
			af[1].affect_type = EAffect::kCrying;
			af[1].battleflag = kAfBattledec;
			af[2].location = EApply::kMorale;
			af[2].duration = af[1].duration;
			af[2].modifier = -1 * std::max(1, (level / 3 + GetRealRemort(ch) / 5 - GetRealLevel(victim) / 5));
			af[2].affect_type = EAffect::kCrying;
			af[2].battleflag = kAfBattledec;
			to_room = "$n0 издал$g протяжный стон.";
			to_vict = "Вы впали в уныние.";
			break;
		}
			//Заклинания Забвение, Бремя времени. Далим.
		case ESpell::kOblivion:
		case ESpell::kBurdenOfTime: {
			if (victim->IsImmortal() || CalcGeneralSaving(ch, victim, ESaving::kReflex, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			SetWaitState(victim, (level / 10 + 1) * kBattleRound);
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 3, 0, 0, 0, 0));
			af[0].affect_type = EAffect::kSlow;
			af[0].battleflag = kAfBattledec;
			to_room = "Облако забвения окружило $n3.";
			to_vict = "Ваш разум помутился.";
			spell_id = ESpell::kOblivion;
			break;
		}

		case ESpell::kPeaceful: {
			if (AFF_FLAGGED(victim, EAffect::kPeaceful)
				|| (victim->IsNpc() && !AFF_FLAGGED(victim, EAffect::kCharmed)) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			if (victim->GetEnemy()) {
				stop_fighting(victim, true);
				change_fighting(victim, true);
				SetWaitState(victim, 2 * kBattleRound);
			}
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 2, 0, 0, 0, 0));
			af[0].affect_type = EAffect::kPeaceful;
			to_room = "Взгляд $n1 потускнел, а сам он успокоился.";
			to_vict = "Ваша душа очистилась от зла и странно успокоилась.";
			break;
		}

		case ESpell::kStoneBones: {
			if (GET_MOB_VNUM(victim) < kMobSkeleton || GET_MOB_VNUM(victim) > kLastNecroMob) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
			}
			af[0].location = EApply::kArmour;
			af[0].duration = CalcDuration(victim, 100, level, 1, 0, 0);
			af[0].modifier = level + 10 + GetRealRemort(ch) / 2;
			af[1].location = EApply::kSavingStability;
			af[1].duration = af[0].duration;
			af[1].modifier = level + 10 + GetRealRemort(ch) / 2;
			accum_duration = true;
			to_vict = " ";
			to_room = "Кости $n1 обрели твердость кремня.";
			break;
		}

		case ESpell::kFailure:
		case ESpell::kMassFailure: {
			savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = EApply::kMorale;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 2, level, 2, 0, 0));
			af[0].modifier = -5 - (GetRealLevel(ch) + GetRealRemort(ch)) / 2;
			af[1].location = static_cast<EApply>(number(1, 6));
			af[1].duration = af[0].duration;
			af[1].modifier = -(GetRealLevel(ch) + GetRealRemort(ch) * 3) / 15;
			to_room = "Тяжелое бурое облако сгустилось над $n4.";
			to_vict = "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас.";
			break;
		}

		case ESpell::kGlitterDust: {
			savetype = ESaving::kReflex;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi + 50)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			if (IsAffectedBySpell(victim, ESpell::kInvisible)) {
				RemoveAffectFromChar(victim, ESpell::kInvisible);
			}
			if (IsAffectedBySpell(victim, ESpell::kCamouflage)) {
				RemoveAffectFromChar(victim, ESpell::kCamouflage);
			}
			if (IsAffectedBySpell(victim, ESpell::kHide)) {
				RemoveAffectFromChar(victim, ESpell::kHide);
			}
			af[0].location = EApply::kSavingReflex;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 4, 0, 0, 0, 0));
			af[0].modifier = (GetRealLevel(ch) + GetRealRemort(ch)) / 3;
			af[0].affect_type = EAffect::kGlitterDust;
			accum_duration = true;
			accum_affect = true;
			to_room = "Облако ярко блестящей пыли накрыло $n3.";
			to_vict = "Липкая блестящая пыль покрыла вас с головы до пят.";
			break;
		}

		case ESpell::kScream: {
			savetype = ESaving::kStability;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].affect_type = EAffect::kAffright;
			af[0].location = EApply::kSavingWill;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 2, level, 2, 0, 0));
			af[0].modifier = (2 * GetRealLevel(ch) + GetRealRemort(ch)) / 4;

			af[1].affect_type = EAffect::kAffright;
			af[1].location = EApply::kMorale;
			af[1].duration = af[0].duration;
			af[1].modifier = -(GetRealLevel(ch) + GetRealRemort(ch)) / 6;

			to_room = "$n0 побледнел$g и задрожал$g от страха.";
			to_vict = "Страх сжал ваше сердце ледяными когтями.";
			break;
		}

		case ESpell::kCatGrace: {
			af[0].location = EApply::kDex;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			if (ch == victim)
				af[0].modifier = (level + 5) / 10;
			else
				af[0].modifier = (level + 10) / 15;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Ваши движения обрели невиданную ловкость.";
			to_room = "Движения $n1 обрели невиданную ловкость.";
			break;
		}

		case ESpell::kBullBody: {
			af[0].location = EApply::kCon;
			af[0].duration = CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0);
			if (ch == victim)
				af[0].modifier = (level + 5) / 10;
			else
				af[0].modifier = (level + 10) / 15;
			accum_duration = true;
			accum_affect = true;
			to_vict = "Ваше тело налилось звериной мощью.";
			to_room = "Плечи $n1 раздались вширь, а тело налилось звериной мощью.";
			break;
		}

		case ESpell::kSnakeWisdom:
			to_vict = "Шелест змеиной чешуи коснулся вашего сознания, и вы стали мудрее.";
			to_room = "$n спокойно и мудро посмотрел$g вокруг.";
			break;

		case ESpell::kGimmicry:
			to_vict = "Вы почувствовали, что для вашего ума более нет преград.";
			to_room = "$n хитро прищурил$u и поглядел$g по сторонам.";
			break;

		case ESpell::kWarcryOfMenace: {
			savetype = ESaving::kWill;
			modi = GetRealCon(ch);
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].location = EApply::kMorale;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 2, level + 3, 4, 6, 0));
			af[0].modifier = -RollDices((7 + level) / 8, 3);
			to_vict = "Похоже, сегодня не ваш день.";
			to_room = "Удача покинула $n3.";
			break;
		}

		case ESpell::kWarcryOfMadness: {
			savetype = ESaving::kStability;
			modi = GetRealCon(ch) * 3 / 2;
			if (ch == victim || !CalcGeneralSaving(ch, victim, savetype, modi)) {
				af[0].location = EApply::kInt;
				af[0].duration = ApplyResist(victim, GetResistType(spell_id),
											 CalcDuration(victim, 2, level + 3, 4, 6, 0));
				af[0].modifier = -RollDices((7 + level) / 8, 2);
				to_vict = "Вы потеряли рассудок.";
				to_room = "$n0 потерял$g рассудок.";

				savetype = ESaving::kStability;
				modi = GetRealCon(ch) * 2;
				if (ch == victim || !CalcGeneralSaving(ch, victim, savetype, modi)) {
					af[1].location = EApply::kCastSuccess;
					af[1].duration = af[0].duration;
					af[1].modifier = -(RollDices((2 + level) / 3, 4) + RollDices(GetRealRemort(ch) / 2, 5));

					af[2].location = EApply::kManaRegen;
					af[2].duration = af[1].duration;
					af[2].modifier = af[1].modifier;
					to_vict = "Вы обезумели.";
					to_room = "$n0 обезумел$g.";
				}
			} else {
				savetype = ESaving::kStability;
				modi = GetRealCon(ch) * 2;
				if (!CalcGeneralSaving(ch, victim, savetype, modi)) {
					af[0].location = EApply::kCastSuccess;
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level + 3, 4, 6, 0));
					af[0].modifier = -(RollDices((2 + level) / 3, 4) + RollDices(GetRealRemort(ch) / 2, 5));

					af[1].location = EApply::kManaRegen;
					af[1].duration = af[0].duration;
					af[1].modifier = af[0].modifier;
					to_vict = "Вас охватила паника.";
					to_room = "$n0 начал$g сеять панику.";
				} else {
					SendMsgToChar(NOEFFECT, ch);
					success = false;
				}
			}
			update_spell = true;
			break;
		}

		case ESpell::kWarcryOfLuck: {
			af[0].location = EApply::kMorale;
			af[0].modifier = std::max(1, ch->GetSkill(ESkill::kWarcry) / 20);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfExperience: {
			af[0].location = EApply::kExpPercent;
			af[0].modifier = std::max(1, ch->GetSkill(ESkill::kWarcry) / 20);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfPhysdamage: {
			af[0].location = EApply::kPhysicDamagePercent;
			af[0].modifier = std::max(1, ch->GetSkill(ESkill::kWarcry) / 20);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfBattle: {
			af[0].location = EApply::kAc;
			af[0].modifier = -(10 + std::min(20, 2 * GetRealRemort(ch)));
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfDefence: {
			af[0].location = EApply::kSavingCritical;
			af[0].modifier -= ch->GetSkill(ESkill::kWarcry) / 10.0;
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			af[1].location = EApply::kSavingReflex;
			af[1].modifier -= ch->GetSkill(ESkill::kWarcry) / 10;
			af[1].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			af[2].location = EApply::kSavingStability;
			af[2].modifier -= ch->GetSkill(ESkill::kWarcry) / 10;
			af[2].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			af[3].location = EApply::kSavingWill;
			af[3].modifier -= ch->GetSkill(ESkill::kWarcry) / 10;
			af[3].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			//to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfPower: {
			af[0].location = EApply::kHp;
			af[0].modifier = std::min(200, (4 * ch->get_con() + ch->GetSkill(ESkill::kWarcry)) / 2);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfBless: {
			af[0].location = EApply::kSavingStability;
			af[0].modifier = -(4 * ch->get_con() + ch->GetSkill(ESkill::kWarcry)) / 24;
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			af[1].location = EApply::kSavingWill;
			af[1].modifier = af[0].modifier;
			af[1].duration = af[0].duration;
			to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case ESpell::kWarcryOfCourage: {
			af[0].location = EApply::kHitroll;
			af[0].modifier = (44 + ch->GetSkill(ESkill::kWarcry)) / 45;
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0);
			af[1].location = EApply::kDamroll;
			af[1].modifier = (29 + ch->GetSkill(ESkill::kWarcry)) / 30;
			af[1].duration = af[0].duration;
			to_vict = nullptr;
			to_room = nullptr;
			break;
		}

		case ESpell::kAconitumPoison: af[0].location = EApply::kAconitumPoison;
			af[0].duration = 7;
			af[0].modifier = level;
			af[0].affect_type = EAffect::kPoisoned;
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case ESpell::kScopolaPoison: af[0].location = EApply::kScopolaPoison;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].affect_type = EAffect::kPoisoned;
			af[0].battleflag = kAfSameTime;
			// kScopolaPoison rides on a separate flag-only affect (one EAffect per
			// affect now). Same type/duration => removed together with af[0].
			af[1].duration = 7;
			af[1].affect_type = EAffect::kScopolaPoison;
			af[1].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case ESpell::kBelenaPoison: af[0].location = EApply::kBelenaPoison;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].affect_type = EAffect::kPoisoned;
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case ESpell::kDaturaPoison: af[0].location = EApply::kDaturaPoison;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].affect_type = EAffect::kPoisoned;
			af[0].battleflag = kAfSameTime;
			to_vict = "Вы почувствовали себя отравленным.";
			to_room = "$n позеленел$g от действия яда.";
			break;

		case ESpell::kCombatLuck: af[0].duration = CalcDuration(victim, 6, 0, 0, 0, 0);
			af[0].affect_type = EAffect::kCombatLuck;
			af[0].handler.reset(new CombatLuckAffectHandler());
			af[0].type = ESpell::kCombatLuck;
			af[0].location = EApply::kHitroll;
			af[0].modifier = 0;
			to_room = "$n вдохновенно выпятил$g грудь.";
			to_vict = "Вы почувствовали вдохновение.";
			break;

		case ESpell::kArrowsFire:
		case ESpell::kArrowsWater:
		case ESpell::kArrowsEarth:
		case ESpell::kArrowsAir:
		case ESpell::kArrowsDeath: {
			//Додати обработчик
			break;
		}
		
		case ESpell::kPaladineInspiration:
			/*
         * групповой спелл, развешивающий рандомные аффекты, к сожалению
         * не может быть применен по принципа "сгенерили рандом - и применили"
         * поэтому на каждого члена группы применяется свой аффект, а кастер еще и полечить может
         * */

			if (ch == victim && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena))
				rnd = number(1, 5);
			else
				rnd = number(1, 4);
			af[0].type = ESpell::kPaladineInspiration;
			af[0].battleflag = kAfBattledec | kAfPulsedec;
			switch (rnd) {
				case 1:af[0].location = EApply::kPhysicDamagePercent;
					af[0].duration = CalcDuration(victim, 5, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch);
					break;
				case 2:af[0].location = EApply::kCastSuccess;
					af[0].duration = CalcDuration(victim, 3, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch);
					break;
				case 3:af[0].location = EApply::kManaRegen;
					af[0].duration = CalcDuration(victim, 10, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch) * 5;
					break;
				case 4:af[0].location = EApply::kMagicDamagePercent;
					af[0].duration = CalcDuration(victim, 5, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch);
					break;
				case 5:CallMagic(ch, ch, nullptr, nullptr, ESpell::kGreatHeal, GetRealLevel(ch));
					break;
				default:break;
			}
		default: break;
	}
	// Resolved after the switch so fallthrough cases that reassign spell_id route to
	// the correct spell's <affects> talent (issue #3334).
	const bool has_affect_talent = MUD::Spell(spell_id).actions.Contains(talents_actions::EAction::kAffect);
	ch->send_to_TC(false, true, false, "Кастуем спелл %s по цели %s длительносить %d\r\n", MUD::Spell(af[0].type).GetCName(), GET_PAD(victim, 2), af[0].duration);
	//проверка на обкаст мобов, имеющих от рождения встроенный аффкект
	//чтобы этот аффект не очистился, при спадении спелла
	if (victim->IsNpc() && success) {
		for (i = 0; i < kMaxSpellAffects && success; ++i) {
			if (AFF_FLAGGED(&mob_proto[victim->get_rnum()], af[i].affect_type)) {
				if (ch->in_room == victim->in_room) {
					SendMsgToChar(NOEFFECT, ch);
				}
				success = false;
			}
		}
	}
	// Affect talents drive their own re-cast/update policy through EAffFlag flags
	// (issue #3334), so the legacy IsViolent-based update/recast rules are skipped
	// for spells that use them.
	if (!has_affect_talent) {
		// позитивные аффекты - продлеваем, если они уже на цели
		if (!MUD::Spell(spell_id).IsViolent() && IsAffectedBySpell(victim, spell_id) && success) {
			update_spell = true;
		}
		// вот такой оригинальный способ запретить рекасты негативных аффектов - через флаг апдейта
		if ((ch != victim) && IsAffectedBySpell(victim, spell_id) && success && (!update_spell)) {
			if (ch->in_room == victim->in_room) {
				SendMsgToChar(NOEFFECT, ch);
			}
			success = false;
		}
	}

	for (i = 0; success && i < kMaxSpellAffects; i++) {
		af[i].type = spell_id;
		if (af[i].affect_type != EAffect::kUndefinded || af[i].location != EApply::kNone) {
			af[i].duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[i].duration);

			if (update_spell)
				ImposeAffectNoRecalc(victim, af[i]);
			else
				ImposeAffectNoRecalc(victim, af[i], accum_duration, false, accum_affect, false);
		}
		// тут мы ездим по циклу 16 раз, хотя аффектов 1-3...
//		ch->send_to_TC(true, true, true, "Applied affect type %i\r\n", af[i].type);
	}

	// Affect talent actions (issue #3334): spells that declare <affects> apply them
	// here. The saving throw uses the talent's own saving; the duration is computed
	// once for all applies; each apply's modifier is derived from the cast's potency
	// roll; and the EAffFlag flags drive the per-apply update/accumulate behavior.
	if (success && has_affect_talent) {
		const auto &talent = MUD::Spell(spell_id).actions.GetAffect();
		if (ch != victim && talent.GetSaving() != ESaving::kNone
				&& CalcGeneralSaving(ch, victim, talent.GetSaving(), modi)) {
			SendMsgToChar(NOEFFECT, ch);
			success = false;
		} else {
			const Bitvector flags = talent.GetFlags();
			const bool can_reapply = IS_SET(flags, to_underlying(EAffFlag::kAfAccumulateDuration))
				|| IS_SET(flags, to_underlying(EAffFlag::kAfUpdateDuration));
			if (ch != victim && IsAffectedBySpell(victim, talent.GetSpell()) && !can_reapply) {
				if (ch->in_room == victim->in_room) {
					SendMsgToChar(NOEFFECT, ch);
				}
				success = false;
			} else {
				int duration = ApplyResist(victim, talent.GetResist(),
					CalcDuration(victim, talent.GetDurationConst(), level,
								 talent.GetDurationLevelDivisor(), talent.GetDurationMin(), talent.GetDurationMax()));
				duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, duration);
				const double competencies = potency.skill_coeff + potency.stat_coeff;
				for (const auto &apply: talent.GetApplies()) {
					double raw = competencies * apply.competencies_weight + potency.dices * apply.dices_weight;
					double modifier = apply.min + std::ceil(raw);
					if (apply.stack > 0) {
						modifier = std::min(modifier, modifier * apply.stack);
					}
					Affect<EApply> taf;
					taf.type = talent.GetSpell();
					taf.affect_type = apply.id;
					taf.location = apply.location;
					taf.duration = duration;
					taf.modifier = static_cast<int>(apply.factor * modifier);
					taf.battleflag = flags;
					taf.caster_id = ch->get_uid();
					ApplyTalentAffect(victim, taf, flags);
				}
			}
		}
	}

	affect_total(victim);

	if (success) {
		// вот некрасиво же тут это делать...
		if (spell_id == ESpell::kPoison)
			victim->poisoner = ch->get_uid();
		if (to_vict != nullptr)
			act(to_vict, false, victim, nullptr, ch, kToChar);
		if (to_room != nullptr)
			act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);
		return 1;
	}
	return 0;
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
// Сообщения призыва/оживления вынесены в lib/cfg/spell_msg.xml (issue #3304):
// kSummonToRoom (по заклинанию), kSummonFail / kSummonNoCorpse и прочие
// guard-сообщения в ветви kDefault. См. MUD::SpellMessages().

int CastSummon(int level, CharData *ch, ObjData *obj, ESpell spell_id, bool need_fail) {
	CharData *tmp_mob, *mob = nullptr;
	ObjData *tobj, *next_obj;
	int pfail = 0, handle_corpse = false, keeper = false, cha_num = 0, modifier = 0;
	MobVnum mob_num;

	if (ch == nullptr) {
		return 0;
	}
	if (spell_id == ESpell::kSumonAngel) {
		SummonTutelar(ch);
		return 1;
	}
	if (spell_id == ESpell::kMentalShadow) {
		SpellMentalShadow(ch);
		return 1;
	}

	switch (spell_id) {
		case ESpell::kClone:
			mob_num = kMobDouble;
			pfail =
				50 - GET_CAST_SUCCESS(ch)
					- GetRealRemort(ch) * 5;    // 50% failure, should be based on something later.
			keeper = true;
			break;

		case ESpell::kSummonKeeper:
			mob_num = kMobKeeper;
			if (ch->GetEnemy())
				pfail = 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch);
			else
				pfail = 0;
			keeper = true;
			break;

		case ESpell::kSummonFirekeeper:
			mob_num = kMobFirekeeper;
			if (ch->GetEnemy())
				pfail = 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch);
			else
				pfail = 0;
			keeper = true;
			break;

		case ESpell::kAnimateDead:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(), false, ch, nullptr, nullptr, kToChar);
				return 0;
			}
			mob_num = GET_OBJ_VAL(obj, 2);
			if (mob_num <= 0)
				mob_num = kMobSkeleton;
			else {
				const int real_mob_num = GetMobRnum(mob_num);
				tmp_mob = (mob_proto + real_mob_num);
				pfail = 10 + tmp_mob->get_con() * 2
					- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) * 5;

				int corpse_mob_level = GetRealLevel(mob_proto + real_mob_num);
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
					int rnd = number(1, 100);
					mob_num = kMobNecrodamager;
					if (rnd > 50) {
						mob_num = kMobNecrobreather;
					}
				}

				// kMobNecrocaster disabled, cant cast

				if (GetRealLevel(ch) + GetRealRemort(ch) + 4 < 15 && mob_num > kMobZombie) {
					mob_num = kMobZombie;
				} else if (GetRealLevel(ch) + GetRealRemort(ch) + 4 < 25 && mob_num > kMobBonedog) {
					mob_num = kMobBonedog;
				} else if (GetRealLevel(ch) + GetRealRemort(ch) + 4 < 32 && mob_num > kMobBonedragon) {
					mob_num = kMobBonedragon;
				}
			}

			handle_corpse = true;
			break;

		case ESpell::kResurrection:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(), false, ch, nullptr, nullptr, kToChar);
				return 0;
			}
			if ((mob_num = GET_OBJ_VAL(obj, 2)) <= 0) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectBadCorpse) + "\r\n", ch);
				return 0;
			}

			handle_corpse = true;
			tmp_mob = mob_proto + GetMobRnum(mob_num);
			pfail = 10 + tmp_mob->get_con() * 2
				- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) * 5;
			break;

		default: return 0;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return 0;
	}
	// при перке помощь тьмы гораздо меньше шанс фейла
	if (!ch->IsImmortal() && number(0, 101) < pfail && need_fail) {
		if (CanUseFeat(ch, EFeat::kFavorOfDarkness)) {
			if (number(0, 3) == 0) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
				if (handle_corpse)
					ExtractObjFromWorld(obj);
				return 0;
			}
		} else {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			if (handle_corpse)
				ExtractObjFromWorld(obj);
			return 0;
		}
	}

	if (!(mob = ReadMobile(-mob_num, kVirtual))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return 0;
	}

	if (spell_id == ESpell::kResurrection) {
		ClearMinionTalents(mob);
		if (mob->IsFlagged(EMobFlag::kNoGroup))
			mob->UnsetFlag(EMobFlag::kNoGroup);

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

	if (!ch->IsImmortal() && (AFF_FLAGGED(mob, EAffect::kSanctuary) || mob->IsFlagged(EMobFlag::kProtect))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectConsecrated) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}
	if (!ch->IsImmortal() &&
		(GET_MOB_SPEC(mob) || mob->IsFlagged(EMobFlag::kNoResurrection) ||
			mob->IsFlagged(EMobFlag::kAreaAttack))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectNoPower) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}
	if (!ch->IsImmortal() && AFF_FLAGGED(mob, EAffect::kGodsShield)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectProtected) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}
	if (mob->IsFlagged(EMobFlag::kMounting)) {
		mob->UnsetFlag(EMobFlag::kMounting);
	}
	if (IS_HORSE(mob)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonWarhorse) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}

	if (spell_id == ESpell::kAnimateDead && mob_num >= kMobNecrodamager && mob_num <= kLastNecroMob) {
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
						  ch->in_room,
						  0);
		}
	}

	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		if (handle_corpse)
			ExtractObjFromWorld(obj);
		return 0;
	}

	mob->set_exp(0);
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
	const auto days_from_full_moon =
		(weather_info.moon_day < 14) ? (14 - weather_info.moon_day) : (weather_info.moon_day - 14);
	const auto duration = CalcDuration(mob, GetRealWis(ch) + number(0, days_from_full_moon), 0, 0, 0, 0);
	Affect<EApply> af;
	af.type = ESpell::kCharm;
	af.duration = duration;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.affect_type = EAffect::kCharmed;
	af.battleflag = 0;
	affect_to_char(mob, af);
	if (keeper) {
		af.affect_type = EAffect::kHelper;
		affect_to_char(mob, af);
		mob->set_skill(ESkill::kRescue, 100);
	}

	mob->SetFlag(EMobFlag::kCorpse);
	if (spell_id == ESpell::kClone) {
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
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonToRoom).c_str(), false, ch, nullptr, mob, kToRoom | kToArenaListen);

	PlaceCharToRoom(mob, ch->in_room);
	ch->add_follower(mob);

	if (spell_id == ESpell::kClone) {
		// клоны теперь кастятся все вместе // ужасно некрасиво сделано
		for (auto *k : ch->followers) {
			if (AFF_FLAGGED(k, EAffect::kCharmed)
				&& k->get_master() == ch) {
				cha_num++;
			}
		}
		cha_num = std::max(1, (GetRealLevel(ch) + 4) / 5 - 2) - cha_num;
		if (cha_num < 1)
			return 0;
		CastSummon(level, ch, obj, spell_id, 0);
	}
	if (spell_id == ESpell::kAnimateDead) {
		mob->SetFlag(EMobFlag::kResurrected);
		if (mob_num == kMobSkeleton && CanUseFeat(ch, EFeat::kLoyalAssist))
			mob->set_skill(ESkill::kRescue, 100);

		if (mob_num == kMobBonespirit && CanUseFeat(ch, EFeat::kHauntingSpirit))
			mob->set_skill(ESkill::kRescue, 120);

		// даем всем поднятым, ну наверное не будет чернок 75+ мудры вызывать зомби в щите.
		float eff_wis = CalcEffectiveWis(ch, spell_id);
		if (eff_wis >= 65) {
			// пока не даем, если надо включите
			//af.affect_type = to_underlying(EAffectFlag::AFF_MAGICGLASS);
			//affect_to_char(mob, af);
		}
		if (eff_wis >= 75) {
			Affect<EApply> af;
			af.type = ESpell::kUndefined;
			af.duration = duration * (1 + GetRealRemort(ch));
			af.modifier = 0;
			af.location = EApply::kNone;
			af.affect_type = EAffect::kIceShield;
			af.battleflag = 0;
			affect_to_char(mob, af);
		}

	}

	if (spell_id == ESpell::kSummonKeeper) {
		// Svent TODO: не забыть перенести это в ability
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

	if (spell_id == ESpell::kSummonFirekeeper) {
		Affect<EApply> af;
		af.type = ESpell::kCharm;
		af.duration = duration;
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = 0;
		if (get_effective_cha(ch) >= 30) {
			af.affect_type = EAffect::kFireShield;
			affect_to_char(mob, af);
		} else {
			af.affect_type = EAffect::kFireAura;
			affect_to_char(mob, af);
		}

		modifier = VPOSI((int) get_effective_cha(ch) - 20, 0, 30);

		GET_DR(mob) = 10 + modifier * 3 / 2;
		GET_NDD(mob) = 1;
		GET_SDD(mob) = modifier / 5 + 1;
		mob->mob_specials.extra_attack = 0;

		int m = 300 + number(modifier * 12, modifier * 16);
		mob->set_hit(m);
		mob->set_max_hit(m);
		mob->set_skill(ESkill::kAwake, 50 + modifier * 2);
		mob->SetFlag(EPrf::kAwake);
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	// А надо ли это вообще делать???
	if (handle_corpse) {
		for (tobj = obj->get_contains(); tobj;) {
			next_obj = tobj->get_next_content();
			RemoveObjFromObj(tobj);
			PlaceObjToRoom(tobj, ch->in_room);
			if (!CheckObjDecay(tobj) && tobj->get_in_room() != kNowhere) {
				act("На земле остал$U лежать $o.", false, ch, tobj, nullptr, kToRoom | kToArenaListen);
			}
			tobj = next_obj;
		}
		ExtractObjFromWorld(obj);
	}
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment; //выровняем алигмент чтоб не агрили вдруг
	return 1;
}

int CastToPoints(int level, CharData *ch, CharData *victim, ESpell spell_id) {
	int hit = 0; //если выставить больше нуля, то лечит
	int move = 0; //если выставить больше нуля, то реген хп
	bool extraHealing = false; //если true, то лечит сверх макс.хп

	if (victim == nullptr) {
		log("MAG_POINTS: Ошибка! Не указана цель, spell_id: %d!\r\n", to_underlying(spell_id));
		return 0;
	}

	switch (spell_id) {
		case ESpell::kCureLight:
		case ESpell::kCureSerious:
		case ESpell::kCureCritic:
		case ESpell::kHeal:
		case ESpell::kGroupHeal:
		case ESpell::kGreatHeal:
        case ESpell::kPatronage: [[fallthrough]];
        case ESpell::kWarcryOfPower:
            hit = CalcHeal(ch, victim, spell_id, level);
			break;
		case ESpell::kExtraHits: extraHealing = true;
			hit = CalcHeal(ch, victim, spell_id, level);
			break;
		case ESpell::kEviless:
			if (!victim->IsNpc() || victim->get_master() != ch || !victim->IsFlagged(EMobFlag::kCorpse)) {
                return 1;
            }
			if (AFF_FLAGGED(ch, EAffect::kForcesOfEvil)) {
				hit = CalcHeal(ch, victim, spell_id, level);
				RemoveAffectFromCharAndRecalculate(ch, ESpell::kEviless);
			}
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
			return 0;
			break;
	}
	// issue #3304: сообщение цели лечащего заклинания берётся из spell_msg.xml.
	// Не у всех заклинаний есть такое сообщение (напр. kPatronage, kEviless) -
	// выводим только если оно задано именно для данного заклинания.
	const auto &points_sheaf = MUD::SpellMessages()[spell_id];
	if (points_sheaf.HasMessage(ESpellMsg::kPointsToVict)) {
		SendMsgToChar(points_sheaf.GetMessage(ESpellMsg::kPointsToVict) + "\r\n", victim);
	}
//	log("HEAL: до модификатора  Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
	hit = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, hit);

	if (hit && victim->GetEnemy() && ch != victim) {
		if (!pk_agro_action(ch, victim->GetEnemy()))
			return 0;
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

	return 1;
}

bool CheckNodispel(const Affect<EApply>::shared_ptr &affect) {
	return !affect
		|| MUD::Spell(affect->type).IsInvalid()
		|| affect->affect_type == EAffect::kCharmed
		|| affect->type == ESpell::kCharm
		|| affect->type == ESpell::kQUest
		|| affect->type == ESpell::kPatronage
		|| affect->type == ESpell::kSolobonus
		|| affect->type == ESpell::kEviless;
}

int CastUnaffects(int/* level*/, CharData *ch, CharData *victim, ESpell spell_id) {
	int remove = 0;
	const char *to_vict = nullptr, *to_room = nullptr;

	if (victim == nullptr) {
		return 0;
	}

	auto spell{ESpell::kUndefined};
	switch (spell_id) {
		case ESpell::kCureBlind: spell = ESpell::kBlindness;
			to_vict = "К вам вернулась способность видеть.";
			to_room = "$n прозрел$g.";
			break;
		case ESpell::kRemovePoison: spell = ESpell::kPoison;
			to_vict = "Тепло заполнило ваше тело.";
			to_room = "$n выглядит лучше.";
			break;
		case ESpell::kCureFever: spell = ESpell::kFever;
			to_vict = "Лихорадка прекратилась.";
			break;
		case ESpell::kRemoveCurse: spell = ESpell::kCurse;
			to_vict = "Боги вернули вам свое покровительство.";
			break;
		case ESpell::kRemoveHold: spell = ESpell::kHold;
			to_vict = "К вам вернулась способность двигаться.";
			break;
		case ESpell::kRemoveSilence: spell = ESpell::kSilence;
			to_vict = "К вам вернулась способность разговаривать.";
			break;
		case ESpell::kRemoveDeafness: spell = ESpell::kDeafness;
			to_vict = "К вам вернулась способность слышать.";
			break;
		case ESpell::kDispellMagic:
			if (!ch->IsNpc()
				&& !group::same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);

				return 0;
			}

			{
				// Снимаем случайный аффект из тех, что вообще можно снять.
				// Раньше брали случайный из всех -- если попадали на
				// неснимаемый (charm/quest/patronage/...), dispel фейлился,
				// хотя рядом могли быть снимаемые.
				int dispellable = 0;
				for (const auto &aff : victim->affected) {
					if (!CheckNodispel(aff)) {
						++dispellable;
					}
				}
				if (dispellable == 0) {
					SendMsgToChar(NOEFFECT, ch);
					return 0;
				}

				const auto target = number(1, dispellable);
				auto seen = 0;
				for (const auto &aff : victim->affected) {
					if (CheckNodispel(aff)) {
						continue;
					}
					if (++seen == target) {
						spell = aff->type;
						break;
					}
				}
			}

			remove = true;
			break;

		default: log("SYSERR: unknown spell_id (%d) passed to CastUnaffects.", to_underlying(spell_id));
			return 0;
	}

	if (spell_id == ESpell::kRemovePoison && !IsAffectedBySpell(victim, spell)) {
		if (IsAffectedBySpell(victim, ESpell::kAconitumPoison))
			spell = ESpell::kAconitumPoison;
		else if (IsAffectedBySpell(victim, ESpell::kScopolaPoison))
			spell = ESpell::kScopolaPoison;
		else if (IsAffectedBySpell(victim, ESpell::kBelenaPoison))
			spell = ESpell::kBelenaPoison;
		else if (IsAffectedBySpell(victim, ESpell::kDaturaPoison))
			spell = ESpell::kDaturaPoison;
	}

	if (!IsAffectedBySpell(victim, spell)) {
		if (spell_id != ESpell::kHeal)    // 'cure blindness' message.
			SendMsgToChar(NOEFFECT, ch);
		return 0;
	}
	spell_id = spell;
	if (ch != victim && !remove) {
		if (MUD::Spell(spell_id).IsFlagged(kNpcAffectNpc)) {
			if (!pk_agro_action(ch, victim))
				return 0;
		} else if (MUD::Spell(spell_id).IsFlagged(kNpcAffectPc) && victim->GetEnemy()) {
			if (!pk_agro_action(ch, victim->GetEnemy()))
				return 0;
		}
	}
//Polud затычка для закла !удалить яд!. По хорошему нужно его переделать с параметром - тип удаляемого яда
	if (spell == ESpell::kPoison) {
		RemoveAffectFromChar(victim, ESpell::kAconitumPoison);
		RemoveAffectFromChar(victim, ESpell::kDaturaPoison);
		RemoveAffectFromChar(victim, ESpell::kScopolaPoison);
		RemoveAffectFromChar(victim, ESpell::kBelenaPoison);
	}
	RemoveAffectFromCharAndRecalculate(victim, spell);
	if (to_vict != nullptr)
		act(to_vict, false, victim, nullptr, ch, kToChar);
	if (to_room != nullptr)
		act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);

	return 1;
}

int CastToAlterObjs(int/* level*/, CharData *ch, ObjData *obj, ESpell spell_id) {
	const char *to_char = nullptr;

	if (obj == nullptr) {
		return 0;
	}

	if (obj->has_flag(EObjFlag::kNoalter)) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kObjResist).c_str(), true, ch, obj, nullptr, kToChar);
		return 0;
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

		case ESpell::kEnchantWeapon: {
			if (ch == nullptr || obj == nullptr) {
				return 0;
			}

			// Either already enchanted or not a weapon.
			if (obj->get_type() != EObjType::kWeapon) {
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantNotWeapon).c_str();
				break;
			} else if (obj->has_flag(EObjFlag::kMagic)) {
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantMagic).c_str();
				break;
			}

			if (obj->has_flag(EObjFlag::kSetItem)) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantSetItem) + "\r\n", ch);
				break;
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
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantMono).c_str();
			} else if (GET_RELIGION(ch) == kReligionPoly) {
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantPoly).c_str();
			} else {
				to_char = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kEnchantOther).c_str();
			}
			break;
		}
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

		case ESpell::kAcid: DamageObj(obj, number(GetRealLevel(ch) * 2, GetRealLevel(ch) * 4), 100);
			break;

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
				return 0;
			}
			break;

		case ESpell::kRestoration: {
			if (obj->has_flag(EObjFlag::kMagic)
				&& (obj->get_rnum() != kNothing)) {
				if (obj_proto.at(obj->get_rnum())->has_flag(EObjFlag::kMagic)) {
					return 0;
				}
				obj->unset_enchant();
			} else {
				return 0;
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
				return 1;
			}
			break;
		default: break;
	} // switch

	if (to_char == nullptr) {
		SendMsgToChar(NOEFFECT, ch);
	} else {
		act(to_char, true, ch, obj, nullptr, kToChar);
	}

	return 1;
}

int CastCreation(int/* level*/, CharData *ch, ESpell spell_id) {
	ObjVnum obj_vnum;

	if (ch == nullptr) {
		return 0;
	}

	switch (spell_id) {
		case ESpell::kCreateFood: obj_vnum = kStartBread;
			break;

		case ESpell::kCreateLight: obj_vnum = kCreateLight;
			break;

		default: SendMsgToChar("Spell unimplemented, it would seem.\r\n", ch);
			return 0;
			break;
	}

	const auto tobj = world_objects.create_from_prototype_by_vnum(obj_vnum);
	if (!tobj) {
		SendMsgToChar("Что-то не видно образа для создания.\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found", to_underlying(spell_id), obj_vnum);
		return 0;
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

	return 1;
}

int CastCharRelocate(CharData *caster, CharData *cvict, ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kGroupRecall:
		case ESpell::kWorldOfRecall: 
			SpellRecall(caster, cvict);
			break;
		case ESpell::kTeleport: 
			SpellTeleport(caster, cvict);
			break;
		case ESpell::kSummon: 
			SpellSummon(caster, cvict);
			break;
		case ESpell::kPortal: 
			SpellPortal(caster, cvict);
			break;
		case ESpell::kRelocate: 
			SpellRelocate(caster, cvict);
			break;
		default: return 0;
			break;
	}
	return 1;
}

int CastManual(int level, CharData *caster, CharData *cvict, ObjData *ovict, ESpell spell_id) {
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
		default: return 0;
			break;
	}
	return 1;
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

	if (caster->IsNpc()
		&& caster->get_rnum() == GetMobRnum(kDgCasterProxy))
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
	if (!CAN_SEE(victim, caster) && (GetRealInt(victim) > 25 || GetRealInt(victim) > number(10, 25))) {
		if (!AFF_FLAGGED(victim, EAffect::kDetectInvisible)
			&& GET_SPELL_MEM(victim, ESpell::kDetectInvis) > 0)
			CastSpell(victim, victim, nullptr, nullptr, ESpell::kDetectInvis, ESpell::kDetectInvis);
		else if (!AFF_FLAGGED(victim, EAffect::kDetectLife)
			&& GET_SPELL_MEM(victim, ESpell::kSenseLife) > 0)
			CastSpell(victim, victim, nullptr, nullptr, ESpell::kSenseLife, ESpell::kSenseLife);
		else if (!AFF_FLAGGED(victim, EAffect::kInfravision)
			&& GET_SPELL_MEM(victim, ESpell::kLight) > 0)
			CastSpell(victim, victim, nullptr, nullptr, ESpell::kLight, ESpell::kLight);
	}
}

int CastToSingleTarget(CharData *caster, CharData *cvict, ObjData *ovict, CastRollResult roll) {
	const ESpell spell_id = roll.spell_id;
	const int level = roll.level;
	if (cvict && (caster != cvict))
		if (cvict->IsGod() || (((GetRealLevel(cvict) / 2) > (GetRealLevel(caster) + (GetRealRemort(caster) / 2))) &&
				!caster->IsNpc())) {
			SendMsgToChar(NOEFFECT, caster);
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

	if (MUD::Spell(spell_id).IsFlagged(kMagAffects))
		CastAffect(abs(level), caster, cvict, spell_id, roll.potency);

	if (MUD::Spell(spell_id).IsFlagged(kMagUnaffects))
		CastUnaffects(abs(level), caster, cvict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagPoints))
		CastToPoints(level, caster, cvict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagAlterObjs))
		CastToAlterObjs(abs(level), caster, ovict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagSummons))
		CastSummon(abs(level), caster, ovict, spell_id, true);

	if (MUD::Spell(spell_id).IsFlagged(kMagCreations))
		CastCreation(abs(level), caster, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagManual))
		CastManual(abs(level), caster, cvict, ovict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagCharRelocate))
		CastCharRelocate(caster, cvict, spell_id);

	ReactToCast(cvict, caster, spell_id);
	return 1;
}

// Сообщения массовых/площадных заклинаний вынесены в lib/cfg/spell_msg.xml
// (issue #3304): kAreaToChar / kAreaToRoom / kAreaToVict, доступны через
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
