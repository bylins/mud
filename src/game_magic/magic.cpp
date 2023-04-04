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

#include "third_party_libs/fmt/include/fmt/format.h"

#include "action_targeting.h"
#include "cmd/hire.h"
#include "corpse.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "game_fight/mobact.h"
#include "handler.h"
#include "magic_utils.h"
#include "obj_prototypes.h"
#include "structs/global_objects.h"

extern int what_sky;
extern int interpolate(int min_value, int pulse);
extern int attack_best(CharData *ch, CharData *victim, bool do_mode);

byte GetSavingThrows(ECharClass class_id, ESaving type, int level);    // class.cpp
byte GetExtendSavingThrows(ECharClass class_id, ESaving save, int level);
int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id);
void ReactToCast(CharData *victim, CharData *caster, ESpell spell_id);

bool ProcessMatComponents(CharData *caster, int, ESpell spell_id);

bool IsSpellsReplacingFailed(const talents_actions::Affect &affect, CharData *victim);
bool IsBlockedByMobFlag(const talents_actions::Affect &affect, const CharData *victim);
bool IsBlockedByAffect(const talents_actions::Affect &affect, const CharData *victim);
bool IsBlockedBySpell(const talents_actions::Affect &affect, const CharData *victim);
bool IsAffectBlocked(const talents_actions::Affect &affect, const CharData *victim);
int CalcAffectDuration(const spells::SpellInfo &spell, const CharData *ch, const CharData *vict);
int CalcAffectModifier(const talents_actions::Affect &affect, const CharData *ch);

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
int bonus_saving[] = {
	-9, -8, -7, -6, -5, -4, -3, -2, -1, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 21, 22, 22, 23, 23, 24, 24, 25, 25,
	26, 26, 27, 27, 28, 28, 29, 29, 30, 30,
	31, 31, 32, 32, 33, 33, 34, 34, 35, 35,
	36, 36, 37, 37, 38, 38, 39, 39, 40, 40,
	41, 41, 41, 42, 42, 42, 43, 43, 43, 44,
	44, 44, 45, 45, 45, 46, 46, 46, 47, 47,
	47, 48, 48, 48, 49, 49, 49, 50, 50, 50,
};

int bonus_antisaving[] = {
	-9, -8, -7, -6, -5, -4, -3, -2, -1, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 21, 22, 22, 23, 23, 24, 24, 25, 25,
	26, 26, 27, 27, 28, 28, 29, 29, 30, 30,
	31, 31, 32, 32, 33, 33, 34, 34, 35, 35,
	36, 36, 37, 37, 38, 38, 39, 39, 40, 40,
	41, 41, 41, 42, 42, 42, 43, 43, 43, 44,
	44, 44, 45, 45, 45, 46, 46, 46, 47, 47,
	47, 48, 48, 48, 49, 49, 49, 50, 50, 50,
};

int CalcAntiSavings(CharData *ch) {
	int modi = 0;
	if (IS_IMMORTAL(ch))
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
	return static_cast<int>(mod * skill);
}

int GetBasicSave(CharData *ch, ESaving saving, bool log) {
	auto class_sav = ch->GetClass();
//	std::stringstream ss;
	int save =
		(100 - GetExtendSavingThrows(class_sav, saving, GetRealLevel(ch))) * -1; // Базовые спасброски профессии/уровня
	if (ch->IsNpc()) {
		class_sav = ECharClass::kMob;    // неизвестный класс моба
	} else {
		if (class_sav < ECharClass::kFirst || class_sav > ECharClass::kLast) {
			class_sav = ECharClass::kWarrior;
		}
	}
//	ss << "Базовый save персонажа " << GET_NAME(ch) << " (" << saving_name.find(saving)->second << "): " << save;
	switch (saving) {
		case ESaving::kReflex: save -= bonus_saving[GetRealDex(ch) - 1];
			if (ch->IsOnHorse())
				save += 20;
			break;
		case ESaving::kStability: save -= bonus_saving[GetRealCon(ch) - 1];
			if (ch->IsOnHorse())
				save -= 20;
			break;
		case ESaving::kWill: save -= bonus_saving[GetRealWis(ch) - 1];
			break;
		case ESaving::kCritical: save -= bonus_saving[GetRealCon(ch) - 1];
			break;
		default: break;
	}
//	ss << " с учетом статов: " << save << std::endl;
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
	if (PRF_FLAGGED(victim, EPrf::kAwake)) {
		if (CanUseFeat(victim, EFeat::kImpregnable)) {
			save -= std::max(0, victim->GetSkill(ESkill::kAwake) - 80) / 2;
		}
		save -= victim->GetSkill(ESkill::kAwake) / 5; //CalculateSkillAwakeModifier(killer, victim);
	}
	save += GetSave(victim, saving);    // одежда бафы и слава
	if (need_log) {
		killer->send_to_TC(false,
						   true,
						   true,
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
			victim->send_to_TC(false,
							   true,
							   true,
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
	if (number(1, 100) <= 5) { //абсолютный фейл
		save /= 2;
		sprintf(smallbuf,
				"Тестовое сообщение: &RПротивник %s (%d), ваш бонус: %d, спас '%s' противника: %d, random -200..200: %d, критудача: ДА, шанс успеха: %2.2f%%.\r\n&n",
				GET_NAME(victim),
				GetRealLevel(victim),
				ext_apply,
				saving_name.find(type)->second.c_str(),
				save,
				rnd,
				((std::clamp(save + ext_apply, -200, 200) + 200) / 400.) * 100.);
		killer->send_to_TC(false, true, true, smallbuf);
	} else {
		sprintf(smallbuf,
				"Тестовое сообщение: Противник %s (%d), ваш бонус: %d, спас '%s' противника: %d, random -200..200: %d, критудача: НЕТ, шанс успеха: %2.2f%%.\r\n",
				GET_NAME(victim),
				GetRealLevel(victim),
				ext_apply,
				saving_name.find(type)->second.c_str(),
				save,
				rnd,
				((std::clamp(save + ext_apply, -200, 200) + 200) / 400.) * 100.);
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
	if (!ch->IsNpc() && PLR_FLAGGED(ch, EPlrFlag::kWriting))
		return;

	const auto &msg = MUD::Spell(aff_type).GetMsg(ESpellMsg::kExpired);
	if (!msg.empty()) {
		act(msg.c_str(), false, ch, nullptr, nullptr, kToChar | kToSleep);
		SendMsgToChar("\r\n", ch);
	} else {
		auto log_msg = fmt::format("There is no 'expired' message for spell: !{}!", MUD::Spell(aff_type).GetName());
		err_log("%s", log_msg.c_str());
	}
}

// зависимость длительности закла от скила магии
float CalcDurationCoef(ESpell spell_id, int skill_percent) {
	switch (spell_id) {
		case ESpell::kStrength:
		case ESpell::kDexterity:
		case ESpell::kGroupBlink: [[fallthrough]];
		case ESpell::kBlink: return 1 + skill_percent / 400.00;
			break;
		default: return 1;
			break;
	}
}

// зависимость модификации спелла от скила магии
float CalcModCoef(ESpell spell_id, int percent) {
	switch (spell_id) {
		case ESpell::kStrength:
		case ESpell::kDexterity:
			if (percent > 100)
				return 1;
			return 0;
			break;
		case ESpell::kMassSlow:
		case ESpell::kSlowdown: {
			if (percent >= 80) {
				return (percent - 80) / 20.00 + 1.00;
			}
		}
			break;
		case ESpell::kSonicWave:
			if (percent > 100) {
				return (percent - 80) / 20.00; // после 100% идет прибавка
			}
			return 1;
			break;
		case ESpell::kFascination:
		case ESpell::kHypnoticPattern:
			if (percent >= 80) {
				return (percent - 80) / 20.00 + 1.00;
			}
			return 1;
			break;
		case ESpell::kWhirlwind:
			if (percent > 80) {
				return (percent) / 80; // 160 -2 вихря, 240 - 3 вихря и т.д.
			}
			return 1;
			break;
		case ESpell::kLightingBolt:
			if (percent > 100) {
				return (percent - 70) / 30; // 130 - 2 молнии, 160 -3, 190 -4
			}
			return 1;
			break;
		default: return 1;
	}
	return 0;
}
bool IsBreath(ESpell spell_id) {
	static const std::set<ESpell> magic_breath{
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

int CalcBaseDmg(CharData *ch, ESpell spell_id, const talents_actions::Damage &spell_dmg) {
	auto base_dmg{0};
	if (IsBreath(spell_id)) {
		if (!ch->IsNpc()) {
			return 0;
		}
		base_dmg = RollDices(ch->mob_specials.damnodice, ch->mob_specials.damsizedice) +
			GetRealDamroll(ch) + str_bonus(GetRealStr(ch), STR_TO_DAM);
	} else {
		base_dmg = spell_dmg.RollDices();
	}

	if (!ch->IsNpc()) {
		base_dmg *= ch->get_cond_penalty(P_DAMROLL);
	}
	return base_dmg;
}

int CalcTotalSpellDmg(CharData *ch, CharData *victim, ESpell spell_id) {
	auto spell_dmg = MUD::Spell(spell_id).actions.GetDmg();
	int total_dmg{0};
	if (number(1, 100) > std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, GET_MR(victim))) {
		auto base_dmg = CalcBaseDmg(ch, spell_id, spell_dmg);
		auto skill_mod = base_dmg * spell_dmg.CalcSkillCoeff(ch);
		auto wis_mod = base_dmg * spell_dmg.CalcBaseStatCoeff(ch);
		auto bonus_mod = ch->add_abils.percent_magdam_add / 100.0;
//		auto complex_mod = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, base_dmg) - base_dmg;
		auto poison_mod = AFF_FLAGGED(ch, EAffect::kDaturaPoison) ? (-base_dmg * GET_POISON(ch) / 100) : 0;
		auto elem_coeff = CalcMagicElementCoeff(victim, spell_id);

		total_dmg = static_cast<int>((base_dmg + skill_mod + wis_mod + poison_mod) * elem_coeff);
		total_dmg += static_cast<int>(total_dmg * bonus_mod);
		int complex_mod = total_dmg;
		total_dmg = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, total_dmg);
		complex_mod = total_dmg - complex_mod;
		ch->send_to_TC(false,
					   true,
					   true,
					   "&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Wis: %2.2f, Bonus: %1.2f, Cmplx: %d, Poison: %1.2f, Elem.coeff: %1.2f, Total: %d &n\r\n",
					   GET_NAME(victim),
					   base_dmg,
					   skill_mod,
					   wis_mod,
					   1 + bonus_mod,
					   complex_mod,
					   poison_mod,
					   elem_coeff,
					   total_dmg);

		victim->send_to_TC(false,
						   true,
						   true,
						   "&CMag.dmg (%s). Base: %2.2f, Skill: %2.2f, Wis: %2.2f, Bonus: %1.2f, Cmplx: %d, Poison: %1.2f, Elem.coeff: %1.2f, Total: %d &n\r\n",
						   GET_NAME(ch),
						   base_dmg,
						   skill_mod,
						   wis_mod,
						   bonus_mod,
						   complex_mod,
						   poison_mod,
						   elem_coeff,
						   total_dmg);
	}

	return total_dmg;
}

int CastDamage(int level, CharData *ch, CharData *victim, ESpell spell_id) {
	int rand = 0, count = 1, modi = 0;
	ObjData *obj = nullptr;

	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr) {
		return 0;
	}

	if (!pk_agro_action(ch, victim)) {
		return 0;
	}

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
			if (ch != victim && spell_id <= ESpell::kLast && IS_GOD(victim)
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
			(number(1, 100) <
				((victim->GetSkill(ESkill::kShieldBlock)) / 20 + GET_OBJ_WEIGHT(GET_EQ(victim, kShield)) / 2))) {
			act("Ловким движением щита $N отразил вашу магию.", false, ch, nullptr, victim, kToChar);
			act("Ловким движением щита $N отразил магию $n1.", false, ch, nullptr, victim, kToNotVict);
			act("Вы отразили своим щитом магию $n1.", false, ch, nullptr, victim, kToVict);
			return 0;
		}
	}

	auto ch_start_pos = GET_POS(ch);
	auto victim_start_pos = GET_POS(victim);

	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
		if (PRF_FLAGGED(ch, EPrf::kAwake) && !victim->IsNpc())
			modi = modi - 50;
	}
//	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
//		modi += (GetRealLevel(ch) - 10);

	auto instant_death{false};
	switch (spell_id) {
		case ESpell::kMagicMissile: {
			if (CanUseFeat(ch, EFeat::kMagicArrows))
				count = (level + 9) / 5;
			else
				count = (level + 9) / 10;
			break;
		}
		case ESpell::kLightingBolt: {
			count = CalcModCoef(spell_id, ch->GetSkill(GetMagicSkillId(spell_id)));
			count += number(1, 5) == 1 ? 1 : 0;
			count = std::min(count, 4);
			break;
		}
		case ESpell::kWhirlwind: {
			count = CalcModCoef(spell_id, ch->GetSkill(GetMagicSkillId(spell_id)));
			count += number(1, 7) == 1 ? 1 : 0;
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
				act("Кислота покрыла $o3.", false, victim, obj, nullptr, kToChar);
				alterate_object(obj, number(level * 2, level * 4), 100);
			}
			break;
		}
		case ESpell::kClod: {
			if (GET_POS(victim) > EPosition::kSit && !IS_IMMORTAL(victim) && (number(1, 100) > GET_AR(victim)) &&
				(AFF_FLAGGED(victim, EAffect::kHold) || !CalcGeneralSaving(ch, victim, ESaving::kReflex, modi))) {
				if (IS_HORSE(victim))
					victim->drop_from_horse();
				act("$n3 придавило глыбой камня.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Огромная глыба камня свалила вас на землю!", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kAcidArrow: {
			// шанс доп аффекта 25% при 100% магии, 45 - 200, 85 -400..
			if (number(1, 100) > (5 + (ch->GetSkill(GetMagicSkillId(spell_id)) / 5))) {
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
					act("$n позеленел от действия кислотной стрелы.",
						false,
						victim,
						nullptr,
						nullptr,
						kToRoom | kToArenaListen);
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
					< GET_SKILL(ch, ESkill::kEarthMagic) * number(1, 6)) {//фейл
					ch->drop_from_horse();
					break;
				}
			}
			if (GET_POS(victim) > EPosition::kSit && !IS_IMMORTAL(victim) && (number(1, 100) > GET_AR(victim)) &&
				(AFF_FLAGGED(victim, EAffect::kHold) || !CalcGeneralSaving(ch, victim, ESaving::kReflex, modi))) {
				if (IS_HORSE(ch))
					ch->drop_from_horse();
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
				SetWaitState(victim, 2 * kBattleRound);
			}
			break;
		}
		case ESpell::kSonicWave: {
			if (GET_POS(victim) > EPosition::kSit &&
				!IS_IMMORTAL(victim) && (number(1, 100) > GET_AR(victim))
				&& (AFF_FLAGGED(victim, EAffect::kHold) || !CalcGeneralSaving(ch, victim, ESaving::kStability, modi))) {
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
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
					act("Ваше каменное проклятие отшибло сознание у $N1.", false, ch, nullptr, victim, kToChar);
					act("Каменное проклятие $n1 отшибло сознание у $N1.", false, ch, nullptr, victim, kToNotVict);
					act("У вас отшибло сознание, вам очень плохо...", false, ch, nullptr, victim, kToVict);
					GET_POS(victim) = EPosition::kStun;
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
					act("Круг пустоты отшиб сознание у $N1.", false, ch, nullptr, victim, kToChar);
					act("Круг пустоты $n1 отшиб сознание у $N1.", false, ch, nullptr, victim, kToNotVict);
					act("У вас отшибло сознание, вам очень плохо...", false, ch, nullptr, victim, kToVict);
					auto wait = 4 + std::max(1, GetRealLevel(ch) + (GetRealWis(ch) - 29)) / 7;
					Affect<EApply> af;
					af.type = spell_id;
					af.affect_bits = to_underlying(EAffect::kMagicStopFight);
					af.modifier = 0;
					af.duration = wait * kBattleRound;
					ch->send_to_TC(false, true, true, "Круг пустоты длительность = %d пульсов.\r\n", af.duration);
					af.flags = kAfPulsedec;
					af.location = EApply::kNone;
					ImposeAffect(victim, af);
					GET_POS(victim) = EPosition::kStun;
//			SetWaitState(victim, wait * kBattleRound);
				}
			}
			break;
		}
		case ESpell::kDispelEvil: {
			if (ch != victim && IS_EVIL(ch) && !IS_IMMORTAL(ch) && GET_HIT(ch) > 1) {
				SendMsgToChar("Ваша магия обратилась против вас.", ch);
				GET_HIT(ch) = 1;
			}
			if (!IS_EVIL(victim)) {
				if (victim != ch)
					act("Боги защитили $N3 от вашей магии.", false, ch, nullptr, victim, kToChar);
				return 0;
			};
			break;
		}
		case ESpell::kDispelGood: {
			if (ch != victim && IS_GOOD(ch) && !IS_IMMORTAL(ch) && GET_HIT(ch) > 1) {
				SendMsgToChar("Ваша магия обратилась против вас.", ch);
				GET_HIT(ch) = 1;
			}
			if (!IS_GOOD(victim)) {
				if (victim != ch)
					act("Боги защитили $N3 от вашей магии.", false, ch, nullptr, victim, kToChar);
				return 0;
			};
			break;
		}
		case ESpell::kSacrifice: {
			if (IS_IMMORTAL(victim))
				break;
			break;
		}
		case ESpell::kDustStorm: {
			if (GET_POS(victim) > EPosition::kSit &&
				!IS_IMMORTAL(victim) && (number(1, 100) > GET_AR(victim)) &&
				(!CalcGeneralSaving(ch, victim, ESaving::kReflex, modi))) {
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
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
			if (GET_POS(victim) > EPosition::kSit && !IS_IMMORTAL(victim) && (AFF_FLAGGED(victim, EAffect::kHold) ||
				!CalcGeneralSaving(ch, victim, ESaving::kStability, modi))) {
				act("$n3 повалило на землю.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				act("Вас повалило на землю.", false, victim, nullptr, nullptr, kToChar);
				GET_POS(victim) = EPosition::kSit;
				update_pos(victim);
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
				act("Ваша магическая стрела поразила $N1.", false, ch, nullptr, victim, kToChar);
				act("Магическая стрела $n1 поразила $N1.", false, ch, nullptr, victim, kToNotVict);
				act("Магическая стрела настигла вас.", false, ch, nullptr, victim, kToVict);
			}
			break;
		}
		default: break;
	}

	int total_dmg{0};
	if (instant_death) {
		total_dmg = std::max(1, GET_HIT(victim) + 1);
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
			&& IN_ROOM(victim) != kNowhere
			&& GET_POS(ch) > EPosition::kStun
			&& GET_POS(victim) > EPosition::kDead) {
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

int CalcDuration(CharData *ch, int cnst, int level, int level_divisor, int min, int max) {
	int result = 0;
	if (ch->IsNpc()) {
		result = cnst;
		if (level > 0 && level_divisor > 0)
			level = level / level_divisor;
		else
			level = 0;
		if (min > 0)
			level = std::min(level, min);
		if (max > 0)
			level = std::max(level, max);
		return (level + result);
	}
	result = cnst * kSecsPerMudHour;
	if (level > 0 && level_divisor > 0)
		level = level * kSecsPerMudHour / level_divisor;
	else
		level = 0;
	if (min > 0)
		level = std::min(level, min * kSecsPerMudHour);
	if (max > 0)
		level = std::max(level, max * kSecsPerMudHour);
	result = (level + result) / kSecsPerPlayerAffect;
	return (result);
}

bool ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id) {
	int vnum = 0;
	switch (spell_id) {
		case ESpell::kFascination:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (GET_OBJ_TYPE(i) == EObjType::kIngredient && i->get_val(1) == 3000) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			break;
		case ESpell::kHypnoticPattern:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (GET_OBJ_TYPE(i) == EObjType::kIngredient && i->get_val(1) == 3006) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			break;
		case ESpell::kEnchantWeapon:
			for (auto i = caster->carrying; i; i = i->get_next_content()) {
				if (GET_OBJ_TYPE(i) == EObjType::kIngredient && i->get_val(1) == 1930) {
					vnum = GET_OBJ_VNUM(i);
					break;
				}
			}
			break;

		default: log("WARNING: wrong spell_id %d in %s:%d", to_underlying(spell_id), __FILE__, __LINE__);
			return false;
	}
	ObjData *tobj = GetObjByVnumInContent(vnum, caster->carrying);
	const auto &spell = MUD::Spell(spell_id);
	if (!tobj) {
		auto msg = spell.GetMsg(ESpellMsg::kComponentMissing);
		act(msg, false, victim, nullptr, caster, kToChar);
		return (true);
	}
	tobj->dec_val(2);
	auto msg = spell.GetMsg(ESpellMsg::kComponentUse);
	act(msg, false, caster, tobj, nullptr, kToChar);
	if (GET_OBJ_VAL(tobj, 2) < 1) {
		auto msg_exhausted = spell.GetMsg(ESpellMsg::kComponentExhausted);
		act(msg_exhausted, false, caster, tobj, nullptr, kToChar);
		RemoveObjFromChar(tobj);
		ExtractObjFromWorld(tobj);
	}
	return (false);
}

bool ProcessMatComponents(CharData *caster, int /*vnum*/, ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kEnchantWeapon: break;
		default: log("WARNING: wrong spell_id %d in %s:%d", to_underlying(spell_id), __FILE__, __LINE__);
			return false;
	}
	ObjData *tobj = GET_EQ(caster, EEquipPos::kHold);
	const auto &spell = MUD::Spell(spell_id);
	if (!tobj) {
		auto &missing = spell.GetMsg(ESpellMsg::kComponentMissing);
		act(missing, false, caster, nullptr, caster, kToChar);
		return (true);
	}
	tobj->dec_val(2);
	auto &use = spell.GetMsg(ESpellMsg::kComponentUse);
	act(use, false, caster, tobj, nullptr, kToChar);
	if (GET_OBJ_VAL(tobj, 2) < 1) {
		auto &exhausted = spell.GetMsg(ESpellMsg::kComponentExhausted);
		act(exhausted, false, caster, tobj, nullptr, kToChar);
		RemoveObjFromChar(tobj);
		ExtractObjFromWorld(tobj);
	}
	return (false);
}

int CastAffect(int level, CharData *ch, CharData *victim, ESpell spell_id) {
	int decline_mod = 0;
	if (victim == nullptr || IN_ROOM(victim) == kNowhere || ch == nullptr) {
		return 0;
	}

	// Calculate PKILL's affects
	if (ch != victim) {
		if (MUD::Spell(spell_id).IsFlagged(kNpcAffectPc)) {
			if (!pk_agro_action(ch, victim)) {
				return 0;
			}
		} else if (MUD::Spell(spell_id).IsFlagged(kNpcAffectNpc) && victim->GetEnemy()) {
			if (!pk_agro_action(ch, victim->GetEnemy()))
				return 0;
		}
	}
	// Magic glass
	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry)) {
		if (ch != victim
			&& MUD::Spell(spell_id).IsViolent()
			&& ((!IS_GOD(ch)
				&& AFF_FLAGGED(victim, EAffect::kMagicGlass)
				&& (ch->in_room == IN_ROOM(victim)) //зеркало сработает только если оба в одной комнате
				&& number(1, 100) < (GetRealLevel(victim) / 3))
				|| (IS_GOD(victim)
					&& (ch->IsNpc()
						|| GetRealLevel(victim) > (GetRealLevel(ch)))))) {
			act("Магическое зеркало $N1 отразило вашу магию!", false, ch, nullptr, victim, kToChar);
			act("Магическое зеркало $N1 отразило магию $n1!", false, ch, nullptr, victim, kToNotVict);
			act("Ваше магическое зеркало отразило поражение $n1!", false, ch, nullptr, victim, kToVict);
			CastAffect(level, ch, ch, spell_id);
			return 0;
		}
	} else {
		if (ch != victim && MUD::Spell(spell_id).IsViolent() && IS_GOD(victim)
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
			+ GET_OBJ_WEIGHT(GET_EQ(victim, EEquipPos::kShield)) / 2))) {
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

	// decrease modi for failing, increese fo success
	int modi{0};
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
	}

	if (PRF_FLAGGED(ch, EPrf::kAwake) && !victim->IsNpc()) {
		modi = modi - 50;
	}

	const auto koef_duration = CalcDurationCoef(spell_id, ch->GetSkill(GetMagicSkillId(spell_id)));
	const auto koef_modifier = CalcModCoef(spell_id, ch->GetSkill(GetMagicSkillId(spell_id)));

	Affect<EApply> af[kMaxSpellAffects];
	auto savetype{ESaving::kStability};
	bool success{true};
	bool accum_duration{false};
	bool accum_affect{false};
	switch (spell_id) {
		case ESpell::kEnergyDrain:
		case ESpell::kWeaknes:
			if (IsAffectedBySpell(victim, ESpell::kStrength)) {
				RemoveAffectFromChar(victim, ESpell::kStrength);
				success = false;
				break;
			}
			if (IsAffectedBySpell(victim, ESpell::kDexterity)) {
				RemoveAffectFromChar(victim, ESpell::kDexterity);
				success = false;
				break;
			}
			break;

		case ESpell::kFascination:
			if (ProcessMatComponents(ch, victim, spell_id)) {
				success = false;
				break;
			}
			break;

		case ESpell::kEnlarge:
			if (IsAffectedBySpell(victim, ESpell::kLessening)) {
				RemoveAffectFromChar(victim, ESpell::kLessening);
				success = false;
				break;
			}

		case ESpell::kLessening:
			if (IsAffectedBySpell(victim, ESpell::kEnlarge)) {
				RemoveAffectFromChar(victim, ESpell::kEnlarge);
				success = false;
				break;
			}

		case ESpell::kGroupPrismaticAura:
		case ESpell::kPrismaticAura:
			if (!ch->IsNpc() && !same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			if (IsAffectedBySpell(victim, ESpell::kSanctuary)) {
				RemoveAffectFromChar(victim, ESpell::kSanctuary);
				success = false;
				break;
			}
			if (AFF_FLAGGED(victim, EAffect::kSanctuary)) {
				success = false;
				break;
			}
			af[0].affect_bits = to_underlying(EAffect::kPrismaticAura);
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			accum_duration = true;
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
										 CalcDuration(victim, 0, GetRealWis(ch) + GetRealInt(ch), 10, 0, 0))
				* koef_duration;
			af[1].location = EApply::kCastSuccess;
			af[1].modifier = -50;
			af[1].duration = af[0].duration;
			af[2].location = EApply::kHitroll;
			af[2].modifier = -5;
			af[2].duration = af[0].duration;
			break;

		case ESpell::kDustStorm:
		case ESpell::kShineFlash:
		case ESpell::kMassBlindness:
		case ESpell::kPowerBlindness:
		case ESpell::kBlindness:
			savetype = ESaving::kStability;
			if (MOB_FLAGGED(victim, EMobFlag::kNoBlind) ||
				IS_IMMORTAL(victim) ||
				((ch != victim) &&
					!GET_GOD_FLAG(victim, EGf::kGodscurse) && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			switch (spell_id) {
				case ESpell::kDustStorm:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 3, level, 6, 0, 0)) * koef_duration;
					break;
				case ESpell::kShineFlash:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level + 7, 8, 0, 0))
						* koef_duration;
					break;
				case ESpell::kMassBlindness:
				case ESpell::kBlindness:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level, 8, 0, 0)) * koef_duration;
					break;
				case ESpell::kPowerBlindness:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 3, level, 6, 0, 0)) * koef_duration;
					break;
				default: break;
			}
			af[0].affect_bits = to_underlying(EAffect::kBlind);
			af[0].flags = kAfBattledec;
			spell_id = ESpell::kBlindness;
			break;

		case ESpell::kMadness: savetype = ESaving::kWill;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 3, 0, 0, 0, 0)) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kNoFlee);
			af[1].location = EApply::kMadness;
			af[1].duration = af[0].duration;
			af[1].modifier = level;
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
										 CalcDuration(victim, 3, level, 6, 0, 0)) * koef_duration;
			af[0].flags = kAfBattledec;
			af[0].affect_bits = to_underlying(EAffect::kNoFlee);
			af[1].location = EApply::kAc;
			af[1].modifier = 20;
			af[1].duration = af[0].duration;
			af[1].flags = kAfBattledec;
			af[1].affect_bits = to_underlying(EAffect::kNoFlee);
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
										 CalcDuration(victim, 1, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = -(5 + decline_mod);
			af[0].affect_bits = to_underlying(EAffect::kCurse);

			af[1].location = EApply::kHitroll;
			af[1].duration = af[0].duration;
			af[1].modifier = -(level / 6 + decline_mod + GetRealRemort(ch) / 5);
			af[1].affect_bits = to_underlying(EAffect::kCurse);

			if (level >= 20) {
				af[2].location = EApply::kCastSuccess;
				af[2].duration = af[0].duration;
				af[2].modifier = -(level / 3 + GetRealRemort(ch));
				if (ch->IsNpc() && level >= (kLvlImmortal))
					af[2].modifier += (kLvlImmortal - level - 1);    //1 cast per mob level above 30
				af[2].affect_bits = to_underlying(EAffect::kCurse);
			}
			accum_duration = true;
			accum_affect = true;
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
				RemoveAffectFromChar(victim, ESpell::kHaste);
				success = false;
				break;
			}

			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 9, 0, 0, 0, 0)) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kSlow);
			af[1].duration =
				ApplyResist(victim, GetResistType(spell_id), CalcDuration(victim, 9, 0, 0, 0, 0))
					* koef_duration;
			af[1].location = EApply::kDex;
			af[1].modifier = -koef_modifier;
			spell_id = ESpell::kSlowdown;
			break;

		case ESpell::kGroupSincerity:
		case ESpell::kDetectAlign:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kDetectAlign);
			accum_duration = true;
			spell_id = ESpell::kDetectAlign;
			break;

		case ESpell::kAllSeeingEye:
		case ESpell::kDetectInvis:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kDetectInvisible);
			accum_duration = true;
			spell_id = ESpell::kDetectInvis;
			break;

		case ESpell::kMagicalGaze:
		case ESpell::kDetectMagic:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kDetectMagic);
			accum_duration = true;
			spell_id = ESpell::kDetectMagic;
			break;

		case ESpell::kSightOfDarkness:
		case ESpell::kInfravision:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kInfravision);
			accum_duration = true;
			spell_id = ESpell::kInfravision;
			break;

		case ESpell::kSnakeEyes:
		case ESpell::kDetectPoison:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kDetectPoison);
			accum_duration = true;
			spell_id = ESpell::kDetectPoison;
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
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].modifier = -40;
			af[0].location = EApply::kAc;
			af[0].affect_bits = to_underlying(EAffect::kInvisible);
			accum_duration = true;
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
										 CalcDuration(victim, 0, level, 2, 0, 0)) * koef_duration;
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
			af[0].duration =
				ApplyResist(victim, GetResistType(spell_id), CalcDuration(victim, 0, level, 1, 0, 0)) * koef_duration;
			af[0].modifier = -2;
			af[0].affect_bits = to_underlying(EAffect::kPoisoned);
			af[0].flags = kAfSameTime;

			af[1].location = EApply::kPoison;
			af[1].duration = af[0].duration;
			af[1].modifier = level + GetRealRemort(ch) / 2;
			af[1].affect_bits = to_underlying(EAffect::kPoisoned);
			af[1].flags = kAfSameTime;
			break;

		case ESpell::kProtectFromEvil:
		case ESpell::kGroupProtectFromEvil:
			if (!ch->IsNpc() && !same_group(ch, victim)) {
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
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kProtectFromDark);
			accum_duration = true;
			break;

		case ESpell::kGroupSanctuary:
		case ESpell::kSanctuary:
			if (!ch->IsNpc() && !same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			if (IsAffectedBySpell(victim, ESpell::kPrismaticAura)) {
				RemoveAffectFromChar(victim, ESpell::kPrismaticAura);
				success = false;
				break;
			}
			if (AFF_FLAGGED(victim, EAffect::kPrismaticAura)) {
				success = false;
				break;
			}

			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kSanctuary);
			spell_id = ESpell::kSanctuary;
			break;

		case ESpell::kSleep: savetype = ESaving::kWill;
			if (AFF_FLAGGED(victim, EAffect::kHold) || MOB_FLAGGED(victim, EMobFlag::kNoSleep)
				|| (ch != victim && CalcGeneralSaving(ch, victim, ESaving::kWill, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			};

			if (victim->GetEnemy())
				stop_fighting(victim, false);
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 1, level, 6, 1, 6)) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kSleep);
			af[0].flags = kAfBattledec;
			if (GET_POS(victim) > EPosition::kSleep && success) {
				if (victim->IsOnHorse()) {
					victim->drop_from_horse();
				}
				SendMsgToChar("Вы слишком устали... Спать... Спа...\r\n", victim);
				act("$n прилег$q подремать.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);

				GET_POS(victim) = EPosition::kSleep;
			}
			break;

		case ESpell::kGroupStrength:
			//case ESpell::kStrength:
			if (IsAffectedBySpell(victim, ESpell::kWeaknes)) {
				RemoveAffectFromChar(victim, ESpell::kWeaknes);
				success = false;
				break;
			}
			af[0].location = EApply::kStr;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + koef_modifier + GetRealRemort(ch) / 5;
			else
				af[0].modifier = (level + 14) / 15 + koef_modifier + GetRealRemort(ch) / 5;
			accum_duration = true;
			accum_affect = true;
			spell_id = ESpell::kStrength;
			break;

		case ESpell::kDexterity:
			if (IsAffectedBySpell(victim, ESpell::kWeaknes)) {
				RemoveAffectFromChar(victim, ESpell::kWeaknes);
				success = false;
				break;
			}
			af[0].location = EApply::kDex;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 9) / 10 + koef_modifier + GetRealRemort(ch) / 5;
			else
				af[0].modifier = (level + 14) / 15 + koef_modifier + GetRealRemort(ch) / 5;
			accum_duration = true;
			accum_affect = true;
			spell_id = ESpell::kDexterity;
			break;

		case ESpell::kPatronage: af[0].location = EApply::kHp;
			af[0].duration = CalcDuration(victim, 3, level, 10, 0, 0) * koef_duration;
			af[0].modifier = GetRealLevel(ch) * 2 + GetRealRemort(ch);
			break;

		case ESpell::kEyeOfGods:
		case ESpell::kSenseLife:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kDetectLife);
			accum_duration = true;
			spell_id = ESpell::kSenseLife;
			break;

		case ESpell::kWaterwalk:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kWaterWalk);
			accum_duration = true;
			break;

		case ESpell::kBreathingAtDepth:
		case ESpell::kWaterbreath:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kWaterBreath);
			accum_duration = true;
			spell_id = ESpell::kWaterbreath;
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
			if (MOB_FLAGGED(victim, EMobFlag::kNoHold)
				|| AFF_FLAGGED(victim, EAffect::kBrokenChains)
				|| AFF_FLAGGED(victim, EAffect::kSleep)
				|| (ch != victim && CalcGeneralSaving(ch, victim, ESaving::kWill, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spell_id), spell_id == ESpell::kPowerHold ?
																		  CalcDuration(victim, 2, level + 7, 8, 2, 5)
																										 : CalcDuration(
					victim,
					1,
					level + 9,
					10,
					1,
					3))
				* koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kHold);
			af[0].flags = kAfBattledec;
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
			if (  //MOB_FLAGGED(victim, MOB_NODEAFNESS) ||
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
												 CalcDuration(victim, 2, level + 3, 4, 6, 0))
						* koef_duration;
					break;
				case ESpell::kMassDeafness:
				case ESpell::kDeafness:
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level + 7, 8, 3, 0))
						* koef_duration;
					break;
				default: break;
			}
			af[0].affect_bits = to_underlying(EAffect::kDeafness);
			af[0].flags = kAfBattledec;
			spell_id = ESpell::kDeafness;
			break;

		case ESpell::kMassSilence:
		case ESpell::kPowerSilence:
		case ESpell::kSilence: savetype = ESaving::kWill;
			if (MOB_FLAGGED(victim, EMobFlag::kNoSilence) ||
				(ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spell_id), spell_id == ESpell::kPowerSilence ?
																		  CalcDuration(victim, 2, level + 3, 4, 6, 0)
																											: CalcDuration(
					victim,
					2,
					level + 7,
					8,
					3,
					0))
				* koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kSilence);
			af[0].flags = kAfBattledec;
			spell_id = ESpell::kSilence;
			break;

		case ESpell::kGroupFly:
		case ESpell::kFly:
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kFly);
			spell_id = ESpell::kFly;
			break;

		case ESpell::kBrokenChains:
			af[0].duration = CalcDuration(victim, 10, GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kBrokenChains);
			af[0].flags = kAfBattledec;
			break;
		case ESpell::kGroupBlink:
		case ESpell::kBlink: af[0].location = EApply::kSpelledBlink;
			af[0].modifier = 10 + GetRealRemort(ch);
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			spell_id = ESpell::kBlink;
			break;

		case ESpell::kMagicShield: af[0].location = EApply::kAc;
			af[0].modifier = -GetRealLevel(ch) * 10 / 6;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[1].location = EApply::kSavingReflex;
			af[1].modifier = -GetRealLevel(ch) / 5;
			af[1].duration = af[0].duration;
			af[2].location = EApply::kSavingStability;
			af[2].modifier = -GetRealLevel(ch) / 5;
			af[2].duration = af[0].duration;
			accum_duration = true;
			break;

		case ESpell::kNoflee: // "приковать противника"
		case ESpell::kIndriksTeeth:
		case ESpell::kSnare: af[0].flags = kAfBattledec;
			savetype = ESaving::kWill;
			if (AFF_FLAGGED(victim, EAffect::kBrokenChains)
				|| (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 3, level, 4, 4, 0)) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kNoTeleport);
			break;

		case ESpell::kLight:
			if (!ch->IsNpc() && !same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kHolyLight);
			break;

		case ESpell::kDarkness:
			if (!ch->IsNpc() && !same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);
				return 0;
			}
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kHolyDark);
			break;
		case ESpell::kVampirism: af[0].duration = CalcDuration(victim, 10, GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].location = EApply::kDamroll;
			af[0].modifier = 0;
			af[0].affect_bits = to_underlying(EAffect::kVampirism);
			break;

		case ESpell::kEviless:
			if (!victim->IsNpc() || victim->get_master() != ch || !MOB_FLAGGED(victim, EMobFlag::kCorpse)) {
				//тихо уходим, т.к. заклинание массовое
				break;
			}
			af[0].duration = CalcDuration(victim, 10, GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].location = EApply::kDamroll;
			af[0].modifier = 15 + (GetRealRemort(ch) > 8 ? (GetRealRemort(ch) - 8) : 0);
			af[0].affect_bits = to_underlying(EAffect::kForcesOfEvil);
			af[1].duration = af[0].duration;
			af[1].location = EApply::kHitroll;
			af[1].modifier = 7 + (GetRealRemort(ch) > 8 ? (GetRealRemort(ch) - 8) : 0);;
			af[1].affect_bits = to_underlying(EAffect::kForcesOfEvil);
			af[2].duration = af[0].duration;
			af[2].location = EApply::kHp;
			af[2].affect_bits = to_underlying(EAffect::kForcesOfEvil);

			// иначе, при рекасте, модификатор суммируется с текущим аффектом.
			if (!AFF_FLAGGED(victim, EAffect::kForcesOfEvil)) {
				af[2].modifier = GET_REAL_MAX_HIT(victim);
				// не очень красивый способ передать сигнал на лечение в mag_points
				Affect<EApply> tmpaf;
				tmpaf.type = ESpell::kEviless;
				tmpaf.duration = 1;
				tmpaf.modifier = 0;
				tmpaf.location = EApply::kNone;
				tmpaf.flags = 0;
				tmpaf.affect_bits = to_underlying(EAffect::kForcesOfEvil);
				affect_to_char(ch, tmpaf);
			}
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
			if (spell_id == ESpell::kEarthfall) {
				modi += ch->GetSkill(GetMagicSkillId(spell_id)) / 5;
			}
			if (IS_IMMORTAL(victim) || (!IS_IMMORTAL(ch) && CalcGeneralSaving(ch, victim, savetype, modi))) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			switch (spell_id) {
				case ESpell::kWarcryOfThunder: af[0].type = ESpell::kDeafness;
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
					af[0].duration = CalcComplexSpellMod(ch, ESpell::kDeafness, GAPPLY_SPELL_EFFECT, af[0].duration);
					af[0].affect_bits = to_underlying(EAffect::kDeafness);
					af[0].flags = kAfBattledec;
					if ((victim->IsNpc()
						&& AFF_FLAGGED(victim, static_cast<EAffect>(af[0].affect_bits)))
						|| (ch != victim
							&& IsAffectedBySpell(victim, ESpell::kDeafness))) {
						if (ch->in_room == IN_ROOM(victim))
							SendMsgToChar(NOEFFECT, ch);
					} else {
						ImposeAffect(victim, af[0], accum_duration, false, accum_affect, false);
//						act(to_vict, false, victim, nullptr, ch, kToChar);
//						act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);
					}
					break;

				case ESpell::kIceStorm:
				case ESpell::kEarthfall: SetWaitState(victim, 2 * kBattleRound);
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, 0, 0, 0, 0)) * koef_duration;
					af[0].affect_bits = to_underlying(EAffect::kMagicStopFight);
					af[0].flags = kAfBattledec | kAfPulsedec;
					spell_id = ESpell::kMagicBattle;
					break;

				case ESpell::kShock: SetWaitState(victim, 2 * kBattleRound);
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, 0, 0, 0, 0)) * koef_duration;
					af[0].affect_bits = to_underlying(EAffect::kMagicStopFight);
					af[0].flags = kAfBattledec | kAfPulsedec;
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
										 CalcDuration(victim, 4, 0, 0, 0, 0)) * koef_duration;
			af[0].modifier =
				-1 * std::max(1,
							  (std::min(29, GetRealLevel(ch)) - std::min(24, GetRealLevel(victim)) +
								  GetRealRemort(ch) / 3) * GET_MAX_HIT(victim) / 100);
			af[0].affect_bits = to_underlying(EAffect::kCrying);
			if (victim->IsNpc()) {
				af[1].location = EApply::kLikes;
				af[1].duration = ApplyResist(victim, GetResistType(spell_id),
											 CalcDuration(victim, 5, 0, 0, 0, 0));
				af[1].modifier = -1 * std::max(1, ((level + 9) / 2 + 9 - GetRealLevel(victim) / 2));
				af[1].affect_bits = to_underlying(EAffect::kCrying);
				af[1].flags = kAfBattledec;
				//			to_room = "$n0 издал$g протяжный стон.";
				break;
			}
			af[1].location = EApply::kCastSuccess;
			af[1].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 5, 0, 0, 0, 0));
			af[1].modifier = -1 * std::max(1, (level / 3 + GetRealRemort(ch) / 3 - GetRealLevel(victim) / 10));
			af[1].affect_bits = to_underlying(EAffect::kCrying);
			af[1].flags = kAfBattledec;
			af[2].location = EApply::kMorale;
			af[2].duration = af[1].duration;
			af[2].modifier = -1 * std::max(1, (level / 3 + GetRealRemort(ch) / 5 - GetRealLevel(victim) / 5));
			af[2].affect_bits = to_underlying(EAffect::kCrying);
			af[2].flags = kAfBattledec;
			break;
		}
		case ESpell::kOblivion:
		case ESpell::kBurdenOfTime: {
			if (IS_IMMORTAL(victim) || CalcGeneralSaving(ch, victim, ESaving::kReflex, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			SetWaitState(victim, (level / 10 + 1) * kBattleRound);
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 3, 0, 0, 0, 0)) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kSlow);
			af[0].flags = kAfBattledec;
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
										 CalcDuration(victim, 2, 0, 0, 0, 0)) * koef_duration;
			af[0].affect_bits = to_underlying(EAffect::kPeaceful);
			break;
		}

		case ESpell::kStoneBones: {
			if (GET_MOB_VNUM(victim) < kMobSkeleton || GET_MOB_VNUM(victim) > kLastNecroMob) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
			}
			af[0].location = EApply::kArmour;
			af[0].duration = CalcDuration(victim, 100, level, 1, 0, 0) * koef_duration;
			af[0].modifier = level + 10 + GetRealRemort(ch) / 2;
			af[1].location = EApply::kSavingStability;
			af[1].duration = af[0].duration;
			af[1].modifier = level + 10 + GetRealRemort(ch) / 2;
			accum_duration = true;
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
										 CalcDuration(victim, 2, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = -5 - (GetRealLevel(ch) + GetRealRemort(ch)) / 2;
			af[1].location = static_cast<EApply>(number(1, 6));
			af[1].duration = af[0].duration;
			af[1].modifier = -(GetRealLevel(ch) + GetRealRemort(ch) * 3) / 15;
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
										 CalcDuration(victim, 4, 0, 0, 0, 0)) * koef_duration;
			af[0].modifier = (GetRealLevel(ch) + GetRealRemort(ch)) / 3;
			af[0].affect_bits = to_underlying(EAffect::kGlitterDust);
			accum_duration = true;
			accum_affect = true;
			break;
		}

		case ESpell::kScream: {
			savetype = ESaving::kStability;
			if (ch != victim && CalcGeneralSaving(ch, victim, savetype, modi)) {
				SendMsgToChar(NOEFFECT, ch);
				success = false;
				break;
			}
			af[0].affect_bits = to_underlying(EAffect::kAffright);
			af[0].location = EApply::kSavingWill;
			af[0].duration = ApplyResist(victim, GetResistType(spell_id),
										 CalcDuration(victim, 2, level, 2, 0, 0)) * koef_duration;
			af[0].modifier = (2 * GetRealLevel(ch) + GetRealRemort(ch)) / 4;

			af[1].affect_bits = to_underlying(EAffect::kAffright);
			af[1].location = EApply::kMorale;
			af[1].duration = af[0].duration;
			af[1].modifier = -(GetRealLevel(ch) + GetRealRemort(ch)) / 6;
			break;
		}

		case ESpell::kCatGrace: {
			af[0].location = EApply::kDex;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			if (ch == victim)
				af[0].modifier = (level + 5) / 10;
			else
				af[0].modifier = (level + 10) / 15;
			accum_duration = true;
			accum_affect = true;
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
			break;
		}

		case ESpell::kSnakeWisdom: {
			af[0].location = EApply::kWis;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].modifier = (level + 6) / 15;
			accum_duration = true;
			accum_affect = true;
			break;
		}

		case ESpell::kGimmicry: {
			af[0].location = EApply::kInt;
			af[0].duration =
				CalcDuration(victim, 20, kSecsPerPlayerAffect * GetRealRemort(ch), 1, 0, 0) * koef_duration;
			af[0].modifier = (level + 6) / 15;
			accum_duration = true;
			accum_affect = true;
			break;
		}

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
										 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
			af[0].modifier = -RollDices((7 + level) / 8, 3);
			break;
		}

		case ESpell::kWarcryOfMadness: {
			savetype = ESaving::kStability;
			modi = GetRealCon(ch) * 3 / 2;
			if (ch == victim || !CalcGeneralSaving(ch, victim, savetype, modi)) {
				af[0].location = EApply::kInt;
				af[0].duration = ApplyResist(victim, GetResistType(spell_id),
											 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
				af[0].modifier = -RollDices((7 + level) / 8, 2);

				savetype = ESaving::kStability;
				modi = GetRealCon(ch) * 2;
				if (ch == victim || !CalcGeneralSaving(ch, victim, savetype, modi)) {
					af[1].location = EApply::kCastSuccess;
					af[1].duration = af[0].duration;
					af[1].modifier = -(RollDices((2 + level) / 3, 4) + RollDices(GetRealRemort(ch) / 2, 5));

					af[2].location = EApply::kManaRegen;
					af[2].duration = af[1].duration;
					af[2].modifier = af[1].modifier;
				}
			} else {
				savetype = ESaving::kStability;
				modi = GetRealCon(ch) * 2;
				if (!CalcGeneralSaving(ch, victim, savetype, modi)) {
					af[0].location = EApply::kCastSuccess;
					af[0].duration = ApplyResist(victim, GetResistType(spell_id),
												 CalcDuration(victim, 2, level + 3, 4, 6, 0)) * koef_duration;
					af[0].modifier = -(RollDices((2 + level) / 3, 4) + RollDices(GetRealRemort(ch) / 2, 5));

					af[1].location = EApply::kManaRegen;
					af[1].duration = af[0].duration;
					af[1].modifier = af[0].modifier;
				} else {
					SendMsgToChar(NOEFFECT, ch);
					success = false;
				}
			}
	//		update_spell = true;
			break;
		}

		case ESpell::kWarcryOfLuck: {
			af[0].location = EApply::kMorale;
			af[0].modifier = std::max(1, ch->GetSkill(ESkill::kWarcry) / 20);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			break;
		}

		case ESpell::kWarcryOfExperience: {
			af[0].location = EApply::kExpPercent;
			af[0].modifier = std::max(1, ch->GetSkill(ESkill::kWarcry) / 20);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			break;
		}

		case ESpell::kWarcryOfPhysdamage: {
			af[0].location = EApply::kPhysicDamagePercent;
			af[0].modifier = std::max(1, ch->GetSkill(ESkill::kWarcry) / 20);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			break;
		}

		case ESpell::kWarcryOfBattle: {
			af[0].location = EApply::kAc;
			af[0].modifier = -(10 + std::min(20, 2 * GetRealRemort(ch)));
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			break;
		}

		case ESpell::kWarcryOfDefence: {
			af[0].location = EApply::kSavingCritical;
			af[0].modifier -= ch->GetSkill(ESkill::kWarcry) / 10.0;
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[1].location = EApply::kSavingReflex;
			af[1].modifier -= ch->GetSkill(ESkill::kWarcry) / 10;
			af[1].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[2].location = EApply::kSavingStability;
			af[2].modifier -= ch->GetSkill(ESkill::kWarcry) / 10;
			af[2].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[3].location = EApply::kSavingWill;
			af[3].modifier -= ch->GetSkill(ESkill::kWarcry) / 10;
			af[3].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			break;
		}

		case ESpell::kWarcryOfPower: {
			af[0].location = EApply::kHp;
			af[0].modifier = std::min(200, (4 * ch->get_con() + ch->GetSkill(ESkill::kWarcry)) / 2);
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			break;
		}

		case ESpell::kWarcryOfBless: {
			af[0].location = EApply::kSavingStability;
			af[0].modifier = -(4 * ch->get_con() + ch->GetSkill(ESkill::kWarcry)) / 24;
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[1].location = EApply::kSavingWill;
			af[1].modifier = af[0].modifier;
			af[1].duration = af[0].duration;
			break;
		}

		case ESpell::kWarcryOfCourage: {
			af[0].location = EApply::kHitroll;
			af[0].modifier = (44 + ch->GetSkill(ESkill::kWarcry)) / 45;
			af[0].duration = CalcDuration(victim, 2, ch->GetSkill(ESkill::kWarcry), 20, 10, 0) * koef_duration;
			af[1].location = EApply::kDamroll;
			af[1].modifier = (29 + ch->GetSkill(ESkill::kWarcry)) / 30;
			af[1].duration = af[0].duration;
			break;
		}

		case ESpell::kAconitumPoison: af[0].location = EApply::kAconitumPoison;
			af[0].duration = 7;
			af[0].modifier = level;
			af[0].affect_bits = to_underlying(EAffect::kPoisoned);
			af[0].flags = kAfSameTime;
			break;

		case ESpell::kScopolaPoison: af[0].location = EApply::kScopolaPoison;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].affect_bits = to_underlying(EAffect::kPoisoned) | to_underlying(EAffect::kScopolaPoison);
			af[0].flags = kAfSameTime;
			break;

		case ESpell::kBelenaPoison: af[0].location = EApply::kBelenaPoison;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].affect_bits = to_underlying(EAffect::kPoisoned);
			af[0].flags = kAfSameTime;
			break;

		case ESpell::kDaturaPoison: af[0].location = EApply::kDaturaPoison;
			af[0].duration = 7;
			af[0].modifier = 5;
			af[0].affect_bits = to_underlying(EAffect::kPoisoned);
			af[0].flags = kAfSameTime;
			break;

		case ESpell::kCombatLuck: af[0].duration = CalcDuration(victim, 6, 0, 0, 0, 0);
			af[0].affect_bits = to_underlying(EAffect::kCombatLuck);
			//Polud пробный обработчик аффектов
			af[0].handler.reset(new CombatLuckAffectHandler());
			af[0].type = ESpell::kCombatLuck;
			af[0].location = EApply::kHitroll;
			af[0].modifier = 0;
			break;

		case ESpell::kArrowsFire:
		case ESpell::kArrowsWater:
		case ESpell::kArrowsEarth:
		case ESpell::kArrowsAir:
		case ESpell::kArrowsDeath: {
			//Додати обработчик
			break;
		}

		case ESpell::kPaladineInspiration: {
			/*
         * групповой спелл, развешивающий рандомные аффекты, к сожалению
         * не может быть применен по принципа "сгенерили рандом - и применили"
         * поэтому на каждого члена группы применяется свой аффект, а кастер еще и полечить может
         * */
			int rnd{0};
			if (ch == victim && !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena))
				rnd = number(1, 5);
			else
				rnd = number(1, 4);
			af[0].type = ESpell::kPaladineInspiration;
			af[0].flags = kAfBattledec | kAfPulsedec;
			switch (rnd) {
				case 1: af[0].location = EApply::kPhysicDamagePercent;
					af[0].duration = CalcDuration(victim, 5, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch);
					break;
				case 2: af[0].location = EApply::kCastSuccess;
					af[0].duration = CalcDuration(victim, 3, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch);
					break;
				case 3: af[0].location = EApply::kManaRegen;
					af[0].duration = CalcDuration(victim, 10, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch) * 5;
					break;
				case 4: af[0].location = EApply::kMagicDamagePercent;
					af[0].duration = CalcDuration(victim, 5, 0, 0, 0, 0);
					af[0].modifier = GetRealRemort(ch) / 5 * 2 + GetRealRemort(ch);
					break;
				case 5: CallMagic(ch, ch, nullptr, nullptr, ESpell::kGroupHeal, GetRealLevel(ch));
					break;
				default: break;
					break;
			}
		}
		default: break;
	}

//	if (victim->IsNpc() && success) {
//		for (auto i = 0; i < kMaxSpellAffects && success; ++i) {
//			if (AFF_FLAGGED(&mob_proto[victim->get_rnum()], static_cast<EAffect>(af[i].affect_bits))) {
//				if (ch->in_room == IN_ROOM(victim)) {
//					SendMsgToChar(NOEFFECT, ch);
//				}
//				success = false;
//			}
//		}
//	}

	// позитивные аффекты - продлеваем, если они уже на цели
//	if (!MUD::Spell(spell_id).IsViolent() && IsAffectedBySpell(victim, spell_id) && success) {
//		update_spell = true;
//	}
	// вот такой оригинальный способ запретить рекасты негативных аффектов - через флаг апдейта
//	if ((ch != victim) && IsAffectedBySpell(victim, spell_id) && success && (!update_spell)) {
//		if (ch->in_room == IN_ROOM(victim)) {
//			SendMsgToChar(NOEFFECT, ch);
//		}
//		success = false;
//	}

	// ============================================================================================
	const auto &spell = MUD::Spell(spell_id);
	const auto &affects = spell.actions.GetImposedAffects();
	auto duration = spell.actions.GetDuration();
	auto duration_time = CalcAffectDuration(spell, ch, victim);
	bool failed{true};
	for (const auto &affect_desc: affects) {
		if (IsAffectedBySpell(victim, affect_desc.Type()) && !affect_desc.IsFlagged(EAffectFlag::kAfUpdate)) {
			continue;
		}
		if (IsAffectBlocked(affect_desc, victim)) {
			continue;
		}
		if (IsSpellsReplacingFailed(affect_desc, victim)) {
			continue;
		}
		if (spell.IsViolent() && ch != victim && CalcGeneralSaving(ch, victim, affect_desc.Saving(), modi)) {
			continue;
		}

		Affect<EApply> affect;
		affect.type = affect_desc.Type();
		affect.duration = duration_time;
		affect.modifier = CalcAffectModifier(affect_desc, ch);
		affect.location = affect_desc.Location();
		affect.flags = affect_desc.Flags();
		affect.affect_bits = affect_desc.AffectBits();
		affect.caster_id = ch->get_uid();

		if (affect.IsFlagged(EAffectFlag::kAfUpdate)) {
			UpdateAffect(victim, affect,
						 duration.Accumulate(), duration.Cap(), affect_desc.AccumulateMod(), affect_desc.ModCap());
		} else {
			affect_to_char(victim, affect);
		}
		failed = false;
		ch->send_to_TC(true, true, true, "%sImpose affect '%s'. Location: %s, Mod: %d, Duration: %d.%s\r\n",
					   KICYN,
					   NAME_BY_ITEM<ESpell>(affect.type).c_str(),
					   NAME_BY_ITEM<EApply>(affect.location).c_str(),
					   affect.modifier,
					   affect.duration,
					   KNRM);
	}

	if (failed) {
		auto &to_char = spell.GetMsg(ESpellMsg::kFailedForChar);
		if (to_char.empty()) {
			SendMsgToChar(NOEFFECT, ch);
		} else {
			act(to_char, false, ch, nullptr, ch, kToChar);
		}
		auto &to_room = spell.GetMsg(ESpellMsg::kFailedForRoom);
		act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen | kToNotVict);
	} else {
		// Этот костыль для ядов надо как-то убирать
		if (spell_id == ESpell::kPoison) {
			victim->poisoner = ch->id;
		}
		auto &to_char = spell.GetMsg(ESpellMsg::kImposedForChar);
		act(to_char, false, ch, nullptr, ch, kToChar);
		auto &to_vict = spell.GetMsg(ESpellMsg::kImposedForVict);
		act(to_vict, false, victim, nullptr, ch, kToChar);
		auto &to_room = spell.GetMsg(ESpellMsg::kImposedForRoom);
		act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen | kToNotVict);
	}

	return !failed;

	// ============================================================================================

//	for (auto i = 0; success && i < kMaxSpellAffects; i++) {
//		af[i].type = spell_id;
//		if (af[i].affect_bits || af[i].location != EApply::kNone) {
//			af[i].duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[i].duration);
//
//			if (update_spell) {
//				ImposeAffect(victim, af[i]);
//			} else {
//				ImposeAffect(victim, af[i], accum_duration, false, accum_affect, false);
//			}
//		}
//	}

//	if (success) {
//		if (spell_id == ESpell::kPoison) {
//			victim->poisoner = ch->id;
//		}
//		auto to_vict = MUD::Spell(spell_id).GetMsg(ESpellMsg::kImposedForVict);
//		act(to_vict, false, victim, nullptr, ch, kToChar);
//		auto to_room = MUD::Spell(spell_id).GetMsg(ESpellMsg::kImposedForRoom);
//		act(to_room, true, victim, nullptr, ch, kToRoom | kToArenaListen);
//		return 1;
//	}
//	return 0;


}

bool IsSpellsReplacingFailed(const talents_actions::Affect &affect, CharData *vict) {
	const auto &replaced_spells = affect.ReplacedSpellAffects();
	if (replaced_spells.empty()) {
		return false;
	}

	int count {0};
	auto predicate = [vict, &count](auto &spell_id) {
		count += RemoveAffectFromChar(vict, spell_id);
	};
	std::for_each(replaced_spells.begin(), replaced_spells.end(), predicate);

	if (!affect.IsObligatoryReplacing() || ((count != 0) && affect.IsObligatoryReplacing())) {
		return false;
	}
	return true;
}

bool IsAffectBlocked(const talents_actions::Affect &affect, const CharData *vict) {
	return (IsBlockedByMobFlag(affect, vict) || IsBlockedByAffect(affect, vict) || IsBlockedBySpell(affect, vict));
}

bool IsBlockedBySpell(const talents_actions::Affect &affect, const CharData *vict) {
	const auto &blocking_spells = affect.BlockingSpells();
	auto predicate = [vict](ESpell spell_id) { return IsAffectedBySpell(vict, spell_id); };
	return std::any_of(blocking_spells.begin(), blocking_spells.end(), predicate);
}

bool IsBlockedByAffect(const talents_actions::Affect &affect, const CharData *vict) {
	const auto &blocking_affects = affect.BlockingAffects();
	auto predicate = [vict](EAffect vict_affect) { return AFF_FLAGGED(vict, vict_affect); };
	return std::any_of(blocking_affects.begin(), blocking_affects.end(), predicate);
}

bool IsBlockedByMobFlag(const talents_actions::Affect &affect, const CharData *victim) {
	const auto &mob_flags = affect.BlockingMobFlags();
	auto predicate = [victim](EMobFlag flag) { return MOB_FLAGGED(victim, flag); };
	return std::any_of(mob_flags.begin(), mob_flags.end(), predicate);
}

int CalcAffectDuration(const spells::SpellInfo &spell, const CharData *ch, const CharData *vict) {
	const auto &duration = spell.actions.GetDuration();
	const auto &roll = duration.Roll();
	auto caster_rating = roll.CalcRating(ch, vict);
	auto cap = duration.Cap();

	if (!spell.IsViolent()) {
		auto duration_time = std::max(static_cast<int>(duration.Min() + caster_rating), cap);
		if (!vict->IsNpc()) {
			return duration_time / kSecsPerPlayerAffect * kSecsPerMudHour;
		}
		return duration_time;
	}

	auto resist_type = GetResisTypeWithElement(spell.GetElement());
	auto vict_rating = static_cast<ullong>(vict->add_abils.apply_resistance[resist_type] * duration.ResistWeight());
	int difficulty = std::max(0, static_cast<int>(caster_rating - vict_rating));
	int roll_result = number(1, abilities::kMainDiceSize);

	int duration_time{0};
	if (roll_result > roll.CritfailThreshold()) {
		duration_time = duration.Min();
	} else if (roll_result < roll.CritsuccessThreshold()) {
		duration_time = cap;
	} else {
		duration_time = duration.Min() + (cap - duration.Min()) / 2;
		int test_result = difficulty - roll_result;
		if (test_result >= 0) {
			duration_time += static_cast<int>(roll_result / abilities::kDegreeDivider * duration.DegreeWeight());
		} else {
			duration_time -= static_cast<int>(roll_result / abilities::kDegreeDivider * duration.DegreeWeight());
		}
	}
	duration_time = std::clamp(duration_time, duration.Min(), cap);
	if (!vict->IsNpc()) {
		return duration_time * kSecsPerMudHour / kSecsPerPlayerAffect;
	}
	return duration_time;
}

int CalcAffectModifier(const talents_actions::Affect &affect, const CharData *ch) {
	auto cap = affect.ModCap();
	auto base_modifier = affect.BaseModifier();
	if (base_modifier == cap) {
		return cap;
	}

	base_modifier += affect.RollBaseModifier(ch);
	if (cap) {
		if (cap > 0) {
			base_modifier = std::min(base_modifier, cap);
		} else {
			base_modifier = std::max(base_modifier, cap);
		}
	}
	return base_modifier;
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
const char *mag_summon_msgs[] =
	{
		"\r\n",
		"$n сделал$g несколько изящних пассов - вы почувствовали странное дуновение!",
		"$n поднял$g труп!",
		"$N появил$U из клубов голубого дыма!",
		"$N появил$U из клубов зеленого дыма!",
		"$N появил$U из клубов красного дыма!",
		"$n сделал$g несколько изящных пассов - вас обдало порывом холодного ветра.",
		"$n сделал$g несколько изящных пассов, от чего ваши волосы встали дыбом.",
		"$n сделал$g несколько изящных пассов, обдав вас нестерпимым жаром.",
		"$n сделал$g несколько изящных пассов, вызвав у вас приступ тошноты.",
		"$n раздвоил$u!",
		"$n оживил$g труп!",
		"$n призвал$g защитника!",
		"Огненный хранитель появился из вихря пламени!"
	};

// * Keep the \r\n because these use SendMsgToChar.
const char *mag_summon_fail_msgs[] =
	{
		"\r\n",
		"Нет такого существа в мире.\r\n",
		"Жаль, сорвалось...\r\n",
		"Ничего.\r\n",
		"Черт! Ничего не вышло.\r\n",
		"Вы не смогли сделать этого!\r\n",
		"Ваша магия провалилась.\r\n",
		"У вас нет подходящего трупа!\r\n"
	};

int CastSummon(int level, CharData *ch, ObjData *obj, ESpell spell_id, bool need_fail) {
	CharData *tmp_mob, *mob = nullptr;
	ObjData *tobj, *next_obj;
	struct FollowerType *k;
	int pfail = 0, msg = 0, fmsg = 0, handle_corpse = false, keeper = false, cha_num = 0, modifier = 0;
	MobVnum mob_num;

	if (ch == nullptr) {
		return 0;
	}

	switch (spell_id) {
		case ESpell::kClone: msg = 10;
			fmsg = number(3, 5);    // Random fail message.
			mob_num = kMobDouble;
			pfail =
				50 - GET_CAST_SUCCESS(ch)
					- GetRealRemort(ch) * 5;    // 50% failure, should be based on something later.
			keeper = true;
			break;

		case ESpell::kSummonKeeper: msg = 12;
			fmsg = number(2, 6);
			mob_num = kMobKeeper;
			if (ch->GetEnemy())
				pfail = 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch);
			else
				pfail = 0;
			keeper = true;
			break;

		case ESpell::kSummonFirekeeper: msg = 13;
			fmsg = number(2, 6);
			mob_num = kMobFirekeeper;
			if (ch->GetEnemy())
				pfail = 50 - GET_CAST_SUCCESS(ch) - GetRealRemort(ch);
			else
				pfail = 0;
			keeper = true;
			break;

		case ESpell::kAnimateDead:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(mag_summon_fail_msgs[7], false, ch, nullptr, nullptr, kToChar);
				return 0;
			}
			mob_num = GET_OBJ_VAL(obj, 2);
			if (mob_num <= 0)
				mob_num = kMobSkeleton;
			else {
				const int real_mob_num = real_mobile(mob_num);
				tmp_mob = (mob_proto + real_mob_num);
				tmp_mob->set_normal_morph();
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
			msg = number(1, 9);
			fmsg = number(2, 6);
			break;

		case ESpell::kResurrection:
			if (obj == nullptr || !IS_CORPSE(obj)) {
				act(mag_summon_fail_msgs[7], false, ch, nullptr, nullptr, kToChar);
				return 0;
			}
			if ((mob_num = GET_OBJ_VAL(obj, 2)) <= 0) {
				SendMsgToChar("Вы не можете поднять труп этого монстра!\r\n", ch);
				return 0;
			}

			handle_corpse = true;
			msg = 11;
			fmsg = number(2, 6);

			tmp_mob = mob_proto + real_mobile(mob_num);
			tmp_mob->set_normal_morph();

			pfail = 10 + tmp_mob->get_con() * 2
				- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - GetRealRemort(ch) * 5;
			break;

		default: return 0;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar("Вы слишком зависимы, чтобы искать себе последователей!\r\n", ch);
		return 0;
	}
	// при перке помощь тьмы гораздо меньше шанс фейла
	if (!IS_IMMORTAL(ch) && number(0, 101) < pfail && need_fail) {
		if (CanUseFeat(ch, EFeat::kFavorOfDarkness)) {
			if (number(0, 3) == 0) {
				SendMsgToChar(mag_summon_fail_msgs[fmsg], ch);
				if (handle_corpse)
					ExtractObjFromWorld(obj);
				return 0;
			}
		} else {
			SendMsgToChar(mag_summon_fail_msgs[fmsg], ch);
			if (handle_corpse)
				ExtractObjFromWorld(obj);
			return 0;
		}
	}

	if (!(mob = read_mobile(-mob_num, VIRTUAL))) {
		SendMsgToChar("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return 0;
	}

	if (spell_id == ESpell::kResurrection) {
		ClearCharTalents(mob);

		sprintf(buf2, "умертвие %s %s", GET_PAD(mob, 1), GET_NAME(mob));
		mob->SetCharAliases(buf2);
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->set_npc_name(buf2);
		mob->player_data.long_descr = "";
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->player_data.PNames[0] = std::string(buf2);
		sprintf(buf2, "умертвию %s", GET_PAD(mob, 1));
		mob->player_data.PNames[2] = std::string(buf2);
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->player_data.PNames[3] = std::string(buf2);
		sprintf(buf2, "умертвием %s", GET_PAD(mob, 1));
		mob->player_data.PNames[4] = std::string(buf2);
		sprintf(buf2, "умертвии %s", GET_PAD(mob, 1));
		mob->player_data.PNames[5] = std::string(buf2);
		sprintf(buf2, "умертвия %s", GET_PAD(mob, 1));
		mob->player_data.PNames[1] = std::string(buf2);
		mob->set_sex(EGender::kNeutral);
		MOB_FLAGS(mob).set(EMobFlag::kResurrected);
		if (CanUseFeat(ch, EFeat::kFuryOfDarkness)) {
			GET_DR(mob) = GET_DR(mob) + GET_DR(mob) * 0.20;
			GET_MAX_HIT(mob) = GET_MAX_HIT(mob) + GET_MAX_HIT(mob) * 0.20;
			GET_HIT(mob) = GET_MAX_HIT(mob);
			GET_HR(mob) = GET_HR(mob) + GET_HR(mob) * 0.20;
		}
	}

	if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(mob, EAffect::kSanctuary) || MOB_FLAGGED(mob, EMobFlag::kProtect))) {
		SendMsgToChar("Оживляемый был освящен Богами и противится этому!\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}
	if (!IS_IMMORTAL(ch) &&
		(GET_MOB_SPEC(mob) || MOB_FLAGGED(mob, EMobFlag::kNoResurrection) ||
			MOB_FLAGGED(mob, EMobFlag::kAreaAttack))) {
		SendMsgToChar("Вы не можете обрести власть над этим созданием!\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}
	if (!IS_IMMORTAL(ch) && AFF_FLAGGED(mob, EAffect::kGodsShield)) {
		SendMsgToChar("Боги защищают это существо даже после смерти.\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return 0;
	}
	if (MOB_FLAGGED(mob, EMobFlag::kMounting)) {
		MOB_FLAGS(mob).unset(EMobFlag::kMounting);
	}
	if (IS_HORSE(mob)) {
		SendMsgToChar("Это был боевой скакун, а не хухры-мухры.\r\n", ch);
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
	af.affect_bits = to_underlying(EAffect::kCharmed);
	af.flags = 0;
	affect_to_char(mob, af);
	if (keeper) {
		af.affect_bits = to_underlying(EAffect::kHelper);
		affect_to_char(mob, af);
		mob->set_skill(ESkill::kRescue, 100);
	}

	MOB_FLAGS(mob).set(EMobFlag::kCorpse);
	if (spell_id == ESpell::kClone) {
		sprintf(buf2, "двойник %s %s", GET_PAD(ch, 1), GET_NAME(ch));
		mob->SetCharAliases(buf2);
		sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
		mob->set_npc_name(buf2);
		mob->player_data.long_descr = "";
		sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
		mob->player_data.PNames[0] = std::string(buf2);
		sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
		mob->player_data.PNames[1] = std::string(buf2);
		sprintf(buf2, "двойнику %s", GET_PAD(ch, 1));
		mob->player_data.PNames[2] = std::string(buf2);
		sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
		mob->player_data.PNames[3] = std::string(buf2);
		sprintf(buf2, "двойником %s", GET_PAD(ch, 1));
		mob->player_data.PNames[4] = std::string(buf2);
		sprintf(buf2, "двойнике %s", GET_PAD(ch, 1));
		mob->player_data.PNames[5] = std::string(buf2);

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

		GET_MAX_HIT(mob) = GET_MAX_HIT(ch);
		GET_HIT(mob) = GET_MAX_HIT(ch);
		mob->mob_specials.damnodice = 0;
		mob->mob_specials.damsizedice = 0;
		mob->set_gold(0);
		GET_GOLD_NoDs(mob) = 0;
		GET_GOLD_SiDs(mob) = 0;
		mob->set_exp(0);

		GET_POS(mob) = EPosition::kStand;
		GET_DEFAULT_POS(mob) = EPosition::kStand;
		mob->set_sex(EGender::kMale);

		mob->set_class(ch->GetClass());
		GET_WEIGHT(mob) = GET_WEIGHT(ch);
		GET_HEIGHT(mob) = GET_HEIGHT(ch);
		GET_SIZE(mob) = GET_SIZE(ch);
		MOB_FLAGS(mob).set(EMobFlag::kClone);
		MOB_FLAGS(mob).unset(EMobFlag::kMounting);
	}
	act(mag_summon_msgs[msg], false, ch, nullptr, mob, kToRoom | kToArenaListen);

	PlaceCharToRoom(mob, ch->in_room);
	ch->add_follower(mob);

	if (spell_id == ESpell::kClone) {
		// клоны теперь кастятся все вместе // ужасно некрасиво сделано
		for (k = ch->followers; k; k = k->next) {
			if (AFF_FLAGGED(k->follower, EAffect::kCharmed)
				&& k->follower->get_master() == ch) {
				cha_num++;
			}
		}
		cha_num = std::max(1, (GetRealLevel(ch) + 4) / 5 - 2) - cha_num;
		if (cha_num < 1)
			return 0;
		CastSummon(level, ch, obj, spell_id, 0);
	}
	if (spell_id == ESpell::kAnimateDead) {
		MOB_FLAGS(mob).set(EMobFlag::kResurrected);
		if (mob_num == kMobSkeleton && CanUseFeat(ch, EFeat::kLoyalAssist))
			mob->set_skill(ESkill::kRescue, 100);

		if (mob_num == kMobBonespirit && CanUseFeat(ch, EFeat::kHauntingSpirit))
			mob->set_skill(ESkill::kRescue, 120);

		// даем всем поднятым, ну наверное не будет чернок 75+ мудры вызывать зомби в щите.
		float eff_wis = CalcEffectiveWis(ch, spell_id);
		if (eff_wis >= 65) {
			// пока не даем, если надо включите
			//af.bitvector = to_underlying(EAffectFlag::AFF_MAGICGLASS);
			//affect_to_char(mob, af);
		}
		if (eff_wis >= 75) {
			Affect<EApply> af;
			af.type = ESpell::kUndefined;
			af.duration = duration * (1 + GetRealRemort(ch));
			af.modifier = 0;
			af.location = EApply::kNone;
			af.affect_bits = to_underlying(EAffect::kIceShield);
			af.flags = 0;
			affect_to_char(mob, af);
		}

	}

	if (spell_id == ESpell::kSummonKeeper) {
		// Svent TODO: не забыть перенести это в ability
		mob->set_level(GetRealLevel(ch));
		int rating = (ch->GetSkill(ESkill::kLightMagic) + GetRealCha(ch)) / 2;
		GET_MAX_HIT(mob) = GET_HIT(mob) = 50 + RollDices(10, 10) + rating * 6;
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
		af.flags = 0;
		if (get_effective_cha(ch) >= 30) {
			af.affect_bits = to_underlying(EAffect::kFireShield);
			affect_to_char(mob, af);
		} else {
			af.affect_bits = to_underlying(EAffect::kFireAura);
			affect_to_char(mob, af);
		}

		modifier = VPOSI((int) get_effective_cha(ch) - 20, 0, 30);

		GET_DR(mob) = 10 + modifier * 3 / 2;
		GET_NDD(mob) = 1;
		GET_SDD(mob) = modifier / 5 + 1;
		mob->mob_specials.extra_attack = 0;

		GET_MAX_HIT(mob) = GET_HIT(mob) = 300 + number(modifier * 12, modifier * 16);
		mob->set_skill(ESkill::kAwake, 50 + modifier * 2);
		PRF_FLAGS(mob).set(EPrf::kAwake);
	}
	MOB_FLAGS(mob).set(EMobFlag::kNoSkillTrain);
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
			hit = GET_REAL_MAX_HIT(victim) / 100 * GetRealInt(ch) / 3 + ch->GetSkill(ESkill::kLifeMagic) / 2;
			SendMsgToChar("Вы почувствовали себя немножко лучше.\r\n", victim);
			break;
		case ESpell::kCureSerious:
			hit = GET_REAL_MAX_HIT(victim) / 100 * GetRealInt(ch) / 2 + ch->GetSkill(ESkill::kLifeMagic) / 2;
			SendMsgToChar("Вы почувствовали себя намного лучше.\r\n", victim);
			break;
		case ESpell::kCureCritic:
			hit = int(GET_REAL_MAX_HIT(victim) / 100 * GetRealInt(ch) / 1.5) + ch->GetSkill(ESkill::kLifeMagic) / 2;
			SendMsgToChar("Вы почувствовали себя значительно лучше.\r\n", victim);
			break;
		case ESpell::kHeal:
		case ESpell::kGroupHeal: hit = GET_REAL_MAX_HIT(victim) - GET_HIT(victim);
			SendMsgToChar("Вы почувствовали себя здоровым.\r\n", victim);
			break;
		case ESpell::kPatronage: hit = (GetRealLevel(victim) + GetRealRemort(victim)) * 2;
			break;
		case ESpell::kWarcryOfPower: hit = std::min(200, (4 * ch->get_con() + ch->GetSkill(ESkill::kWarcry)) / 2);
			SendMsgToChar("По вашему телу начала струиться живительная сила.\r\n", victim);
			break;
		case ESpell::kExtraHits: extraHealing = true;
			hit = RollDices(10, level / 3) + level;
			SendMsgToChar("По вашему телу начала струиться живительная сила.\r\n", victim);
			break;
		case ESpell::kEviless:
			//лечим только умертвия-чармисы
			if (!victim->IsNpc() || victim->get_master() != ch || !MOB_FLAGGED(victim, EMobFlag::kCorpse))
				return 1;
			//при рекасте - не лечим
			if (AFF_FLAGGED(ch, EAffect::kForcesOfEvil)) {
				hit = GET_REAL_MAX_HIT(victim) - GET_HIT(victim);
				RemoveAffectFromChar(ch, ESpell::kEviless); //сбрасываем аффект с хозяина
			}
			break;
		case ESpell::kResfresh:
		case ESpell::kGroupRefresh: move = GET_REAL_MAX_MOVE(victim) - GET_MOVE(victim);
			SendMsgToChar("Вы почувствовали себя полным сил.\r\n", victim);
			break;
		case ESpell::kFullFeed:
		case ESpell::kCommonMeal: {
			if (GET_COND(victim, THIRST) > 0)
				GET_COND(victim, THIRST) = 0;
			if (GET_COND(victim, FULL) > 0)
				GET_COND(victim, FULL) = 0;
			SendMsgToChar("Вы полностью насытились.\r\n", victim);
		}
			break;
		default:
			log("MAG_POINTS: Ошибка! Передан неопределенный лечащий спелл spell_id: %d!\r\n",
				to_underlying(spell_id));
			return 0;
			break;
	}
//	log("HEAL: до модификатора  Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
	hit = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, hit);

	if (hit && victim->GetEnemy() && ch != victim) {
		if (!pk_agro_action(ch, victim->GetEnemy()))
			return 0;
	}
	// лечение
	if (GET_HIT(victim) < kMaxHits && hit != 0) {
		if (!extraHealing && GET_HIT(victim) < GET_REAL_MAX_HIT(victim)) {
			if (AFF_FLAGGED(victim, EAffect::kLacerations)) {
//				log("HEAL: порез Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
				GET_HIT(victim) = std::min(GET_HIT(victim) + hit / 2, GET_REAL_MAX_HIT(victim));
			} else {
//				log("HEAL: Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
				GET_HIT(victim) = std::min(GET_HIT(victim) + hit, GET_REAL_MAX_HIT(victim));
			}
		}
		if (extraHealing) {
//			log("HEAL: наддув Игрок: %s hit: %d GET_HIT: %d GET_REAL_MAX_HIT: %d", GET_NAME(victim), hit, GET_HIT(victim), GET_REAL_MAX_HIT(victim));
			if (GET_REAL_MAX_HIT(victim) <= 0) {
				GET_HIT(victim) = std::max(GET_HIT(victim), std::min(GET_HIT(victim) + hit, 1));
			} else {
				GET_HIT(victim) = std::clamp(GET_HIT(victim) + hit, GET_HIT(victim),
											 GET_REAL_MAX_HIT(victim) + GET_REAL_MAX_HIT(victim) * 33 / 100);
			}
		}
	}
	if (move != 0 && GET_MOVE(victim) < GET_REAL_MAX_MOVE(victim)) {
		GET_MOVE(victim) = std::min(GET_MOVE(victim) + move, GET_REAL_MAX_MOVE(victim));
	}
	update_pos(victim);

	return 1;
}

bool CheckNodispel(const Affect<EApply>::shared_ptr &affect) {
	return !affect
		|| MUD::Spell(affect->type).IsInvalid()
		|| affect->affect_bits == to_underlying(EAffect::kCharmed)
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
				&& !same_group(ch, victim)) {
				SendMsgToChar("Только на себя или одногруппника!\r\n", ch);

				return 0;
			}

			{
				const auto affects_count = victim->affected.size();
				if (0 == affects_count) {
					SendMsgToChar(NOEFFECT, ch);
					return 0;
				}

				auto count = 1;
				const auto rspell = number(1, static_cast<int>(affects_count));
				auto affect_i = victim->affected.begin();
				while (count < rspell) {
					++affect_i;
					++count;
				}

				if (CheckNodispel(*affect_i)) {
					SendMsgToChar(NOEFFECT, ch);
					return 0;
				}

				spell = (*affect_i)->type;
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
	RemoveAffectFromChar(victim, spell);
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
		act("$o устойчив$A к вашей магии.", true, ch, obj, nullptr, kToChar);
		return 0;
	}

	switch (spell_id) {
		case ESpell::kBless:
			if (!obj->has_flag(EObjFlag::kBless)
				&& (GET_OBJ_WEIGHT(obj) <= 5 * GetRealLevel(ch))) {
				obj->set_extra_flag(EObjFlag::kBless);
				if (obj->has_flag(EObjFlag::kNodrop)) {
					obj->unset_extraflag(EObjFlag::kNodrop);
					if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
						obj->inc_val(2);
					}
				}
				obj->add_maximum(std::max(GET_OBJ_MAX(obj) >> 2, 1));
				obj->set_current_durability(GET_OBJ_MAX(obj));
				to_char = "$o вспыхнул$G голубым светом и тут же погас$Q.";
				obj->add_timed_spell(ESpell::kBless, -1);
			}
			break;

		case ESpell::kCurse:
			if (!obj->has_flag(EObjFlag::kNodrop)) {
				obj->set_extra_flag(EObjFlag::kNodrop);
				if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
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
				to_char = "$o вспыхнул$G красным светом и тут же погас$Q.";
			}
			break;

		case ESpell::kInvisible:
			if (!obj->has_flag(EObjFlag::kNoinvis)
				&& !obj->has_flag(EObjFlag::kInvisible)) {
				obj->set_extra_flag(EObjFlag::kInvisible);
				to_char = "$o растворил$U в пустоте.";
			}
			break;

		case ESpell::kPoison:
			if (!GET_OBJ_VAL(obj, 3)
				&& (GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer
					|| GET_OBJ_TYPE(obj) == EObjType::kFountain
					|| GET_OBJ_TYPE(obj) == EObjType::kFood)) {
				obj->set_val(3, 1);
				to_char = "$o отравлен$G.";
			}
			break;

		case ESpell::kRemoveCurse:
			if (obj->has_flag(EObjFlag::kNodrop)) {
				obj->unset_extraflag(EObjFlag::kNodrop);
				if (GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
					obj->inc_val(2);
				}
				to_char = "$o вспыхнул$G розовым светом и тут же погас$Q.";
			}
			break;

		case ESpell::kEnchantWeapon: {
			if (ch == nullptr || obj == nullptr) {
				return 0;
			}

			// Either already enchanted or not a weapon.
			if (GET_OBJ_TYPE(obj) != EObjType::kWeapon) {
				to_char = "Еще раз ударьтесь головой об стену, авось зрение вернется...";
				break;
			} else if (obj->has_flag(EObjFlag::kMagic)) {
				to_char = "Вам не под силу зачаровать магическую вещь.";
				break;
			}

			if (obj->has_flag(EObjFlag::KSetItem)) {
				SendMsgToChar(ch, "Сетовый предмет не может быть заколдован.\r\n");
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
				to_char = "$o вспыхнул$G на миг голубым светом и тут же потух$Q.";
			} else if (GET_RELIGION(ch) == kReligionPoly) {
				to_char = "$o вспыхнул$G на миг красным светом и тут же потух$Q.";
			} else {
				to_char = "$o вспыхнул$G на миг желтым светом и тут же потух$Q.";
			}
			break;
		}
		case ESpell::kRemovePoison:
			if (GET_OBJ_RNUM(obj) < 0) {
				to_char = "Ничего не случилось.";
				char message[100];
				sprintf(message,
						"неизвестный прототип объекта : %s (VNUM=%d)",
						GET_OBJ_PNAME(obj, 0).c_str(),
						obj->get_vnum());
				mudlog(message, BRF, kLvlBuilder, SYSLOG, 1);
				break;
			}
			if (obj_proto[GET_OBJ_RNUM(obj)]->get_val(3) > 1 && GET_OBJ_VAL(obj, 3) == 1) {
				to_char = "Содержимое $o1 протухло и не поддается магии.";
				break;
			}
			if ((GET_OBJ_VAL(obj, 3) == 1)
				&& ((GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer)
					|| GET_OBJ_TYPE(obj) == EObjType::kFountain
					|| GET_OBJ_TYPE(obj) == EObjType::kFood)) {
				obj->set_val(3, 0);
				to_char = "$o стал$G вполне пригодным к применению.";
			}
			break;

		case ESpell::kFly: obj->add_timed_spell(ESpell::kFly, -1);
			obj->set_extra_flag(EObjFlag::kFlying);
			to_char = "$o вспыхнул$G зеленоватым светом и тут же погас$Q.";
			break;

		case ESpell::kAcid: alterate_object(obj, number(GetRealLevel(ch) * 2, GetRealLevel(ch) * 4), 100);
			break;

		case ESpell::kRepair: obj->set_current_durability(GET_OBJ_MAX(obj));
			to_char = "Вы полностью восстановили $o3.";
			break;

		case ESpell::kTimerRestore:
			if (GET_OBJ_RNUM(obj) != kNothing) {
				obj->set_current_durability(GET_OBJ_MAX(obj));
				obj->set_timer(obj_proto.at(GET_OBJ_RNUM(obj))->get_timer());
				to_char = "Вы полностью восстановили $o3.";
				log("%s used magic repair", GET_NAME(ch));
			} else {
				return 0;
			}
			break;

		case ESpell::kRestoration: {
			if (obj->has_flag(EObjFlag::kMagic)
				&& (GET_OBJ_RNUM(obj) != kNothing)) {
				if (obj_proto.at(GET_OBJ_RNUM(obj))->has_flag(EObjFlag::kMagic)) {
					return 0;
				}
				obj->unset_enchant();
			} else {
				return 0;
			}
			to_char = "$o осветил$U на миг внутренним светом и тут же потух$Q.";
		}
			break;

		case ESpell::kLight: obj->add_timed_spell(ESpell::kLight, -1);
			obj->set_extra_flag(EObjFlag::kGlow);
			to_char = "$o засветил$U ровным зеленоватым светом.";
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

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch)) {
		SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else {
		PlaceObjToInventory(tobj.get(), ch);
	}

	return 1;
}

int CastManual(int level, CharData *caster, CharData *cvict, ObjData *ovict, ESpell spell_id) {
	switch (spell_id) {
		case ESpell::kGroupRecall:
		case ESpell::kWorldOfRecall: SpellRecall(level, caster, cvict, ovict);
			break;
		case ESpell::kTeleport: SpellTeleport(level, caster, cvict, ovict);
			break;
		case ESpell::kControlWeather: SpellControlWeather(level, caster, cvict, ovict);
			break;
		case ESpell::kCreateWater: SpellCreateWater(level, caster, cvict, ovict);
			break;
		case ESpell::kLocateObject: SpellLocateObject(level, caster, cvict, ovict);
			break;
		case ESpell::kSummon: SpellSummon(level, caster, cvict, ovict);
			break;
		case ESpell::kPortal: SpellPortal(level, caster, cvict, ovict);
			break;
		case ESpell::kCreateWeapon: SpellCreateWeapon(level, caster, cvict, ovict);
			break;
		case ESpell::kRelocate: SpellRelocate(level, caster, cvict, ovict);
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
		case ESpell::kSumonAngel: SpellSummonAngel(level, caster, cvict, ovict);
			break;
		case ESpell::kVampirism: SpellVampirism(level, caster, cvict, ovict);
			break;
		case ESpell::kMentalShadow: SpellMentalShadow(level, caster, cvict, ovict);
			break;
		default: return 0;
			break;
	}
	return 1;
}

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
		&& GET_MOB_RNUM(caster) == real_mobile(kDgCasterProxy))
		return;

	if (CAN_SEE(victim, caster) && MAY_ATTACK(victim) && IN_ROOM(victim) == IN_ROOM(caster)) {
		if (victim->IsNpc())
			attack_best(victim, caster, false);
		else
			hit(victim, caster, ESkill::kUndefined, fight::kMainHand);
	} else if (CAN_SEE(victim, caster) && !caster->IsNpc() && victim->IsNpc()
		&& MOB_FLAGGED(victim, EMobFlag::kMemory)) {
		mobRemember(victim, caster);
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

int CastToSingleTarget(int level, CharData *caster, CharData *cvict, ObjData *ovict, ESpell spell_id) {
	if (cvict && (caster != cvict))
		if (IS_GOD(cvict) || (((GetRealLevel(cvict) / 2) > (GetRealLevel(caster) + (GetRealRemort(caster) / 2))) &&
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
		CastAffect(level, caster, cvict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagUnaffects))
		CastUnaffects(level, caster, cvict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagPoints))
		CastToPoints(level, caster, cvict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagAlterObjs))
		CastToAlterObjs(level, caster, ovict, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagSummons))
		CastSummon(level, caster, ovict, spell_id, true);

	if (MUD::Spell(spell_id).IsFlagged(kMagCreations))
		CastCreation(level, caster, spell_id);

	if (MUD::Spell(spell_id).IsFlagged(kMagManual))
		CastManual(level, caster, cvict, ovict, spell_id);

	ReactToCast(cvict, caster, spell_id);
	return 1;
}

void TrySendCastMessages(CharData *ch, CharData *victim, RoomData *room, ESpell spell_id) {
	if (room && world[ch->in_room] == room) {
		if (IsAbleToSay(ch)) {
			const auto &spell = MUD::Spell(spell_id);
			auto to_char = spell.GetMsg(ESpellMsg::kAreaForChar);
			act(to_char, false, ch, nullptr, victim, kToChar);
			auto to_room = spell.GetMsg(ESpellMsg::kAreaForRoom);
			act(to_room, false, ch, nullptr, victim, kToRoom | kToArenaListen);
		}
	}
};

int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, ESpell spell_id, int level) {
	if (ch == nullptr || IN_ROOM(ch) == kNowhere) {
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
		auto to_vict = MUD::Spell(spell_id).GetMsg(ESpellMsg::kAreaForVict);
		for (const auto &target: roster) {
			act(to_vict, false, ch, nullptr, target, kToVict);
			CastToSingleTarget(level, ch, target, nullptr, spell_id);
			if (ch->purged()) {
				return 1;
			}
			if (!ch->IsNpc()) {
				++targets_counter;
				if (targets_counter > params.free_targets) {
					int tax = kCasterCastSuccess * cast_decay * (targets_counter - params.free_targets);
					GET_CAST_SUCCESS(ch) = std::max(-200, kCasterCastSuccess - tax);
					level = std::max(1, level - lvl_decay);
					if (PRF_FLAGGED(ch, EPrf::kTester)) {
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
int CallMagicToGroup(int level, CharData *ch, ESpell spell_id) {
	if (ch == nullptr) {
		return 0;
	}

	TrySendCastMessages(ch, nullptr, world[IN_ROOM(ch)], spell_id);
	ActionTargeting::FriendsRosterType roster{ch, ch};
	roster.flip();
	for (const auto target: roster) {
		CastToSingleTarget(level, ch, target, nullptr, spell_id);
	}
	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
