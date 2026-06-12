/**
 \file saving.cpp - a part of the Bylins engine.
 \brief issue.saving-mechanics: saving-throw system (extracted from entities_constants /
        core/constants / magic).
*/

#include "gameplay/mechanics/saving.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "gameplay/abilities/feats.h"               // CanUseFeat
#include "gameplay/abilities/abilities_constants.h" // abilities::kSaveWeight
#include "gameplay/magic/spell_trace.h"             // spell_trace::Line
#include "utils/random.h"                            // number

#include <cmath>
#include <cstdio>

// Defined in pc_classes.cpp (no header); the class/level base saving table.
byte GetExtendSavingThrows(ECharClass class_id, ESaving save, int level);

typedef std::map<ESaving, std::string> ESaving_name_by_value_t;
typedef std::map<const std::string, ESaving> ESaving_value_by_name_t;
ESaving_name_by_value_t ESaving_name_by_value;
ESaving_value_by_name_t ESaving_value_by_name;

void init_ESaving_ITEM_NAMES() {
	ESaving_name_by_value.clear();
	ESaving_value_by_name.clear();

	ESaving_name_by_value[ESaving::kWill] = "kWill";
	ESaving_name_by_value[ESaving::kStability] = "kStability";
	ESaving_name_by_value[ESaving::kCritical] = "kCritical";
	ESaving_name_by_value[ESaving::kReflex] = "kReflex";
	ESaving_name_by_value[ESaving::kNone] = "kNone";

	for (const auto &i : ESaving_name_by_value) {
		ESaving_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESaving>(const ESaving item) {
	if (ESaving_name_by_value.empty()) {
		init_ESaving_ITEM_NAMES();
	}
	return ESaving_name_by_value.at(item);
}

template<>
const std::map<ESaving, std::string> &NAMES_OF<ESaving>() {
	if (ESaving_name_by_value.empty()) {
		init_ESaving_ITEM_NAMES();
	}
	return ESaving_name_by_value;
}

template<>
ESaving ITEM_BY_NAME<ESaving>(const std::string &name) {
	if (ESaving_name_by_value.empty()) {
		init_ESaving_ITEM_NAMES();
	}
	return ESaving_value_by_name.at(name);
}

// Все, связанное с религиями, нужно вынести в отдельный модуль
const religion_names_t religion_name = {
	religion_genders_t{"Язычник", "Язычник", "Язычница", "Язычники"},
	religion_genders_t{"Христианин", "Христианин", "Христианка", "Христиане"},
	religion_genders_t{"", "", "", ""}        // for undefined religion
};

ESaving& operator++(ESaving &s) {
	s =  static_cast<ESaving>(to_underlying(s) + 1);
	return s;
}

static int bonus_saving[] ={
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

const std::vector<ApplyNegative> apply_negative = {
	{"защита", EApply::kAc, ESaving::kNone},
	{"воля", EApply::kSavingWill, ESaving::kWill},
	{"здоровье", EApply::kSavingCritical, ESaving::kCritical},
	{"стойкость", EApply::kSavingStability, ESaving::kStability},
	{"реакция", EApply::kSavingReflex, ESaving::kReflex},
};

const std::map<ESaving, std::string> saving_name = {
	{ESaving::kWill, "воля"},
	{ESaving::kCritical, "здоровье"},
	{ESaving::kStability, "стойкость"},
	{ESaving::kReflex, "реакция"},
	{ESaving::kNone, "отсутствует"},
};

int GetBasicSave(CharData *ch, ESaving saving, bool log) {
//	std::stringstream ss;
	int save = (100 - GetExtendSavingThrows(ch->GetClass(), saving, GetRealLevel(ch))) * -1; // Базовые спасброски профессии/уровня

//	ss << "Базовый save персонажа " << GET_NAME(ch) << " (" << saving_name.find(saving)->second << "): " << save;
	switch (saving) {
		case ESaving::kReflex:
			save -= bonus_saving[GetRealDex(ch) - 1];
			break;
		case ESaving::kStability:
			save -= bonus_saving[GetRealCon(ch) - 1];
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
	save += mount::SavingModifier(ch, saving);
//	ss << " с учетом статов: " << save << "\r\n";
	if (log) {
//		ch->send_to_TC(false, true, true, "%s", ss.str().c_str());
//		mudlog(ss.str(), CMP, kLvlImmortal, SYSLOG, true);
	}
	return save;
}

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
			save -= std::max(0, skills::GetSkill(victim, ESkill::kAwake) - 80) / 2;
		// справка танцующая: "Осторожный стиль" добавочно увеличивает спас-броски персонажа. почему-то было давно убрано с комментом "фикс осторожки дружинника"
		} else if (CanUseFeat(victim, EFeat::kShadowStrike)) {
			save -= std::max(0, skills::GetSkill(victim, ESkill::kAwake) - 80) / 2.5;
		}

		save -= skills::GetSkill(victim, ESkill::kAwake) / 5; //CalculateSkillAwakeModifier(killer, victim);
	}
	save += round(GetSave(victim, saving) * abilities::kSaveWeight);    // одежда бафы и слава
	if (need_log) {
		spell_trace::Line(killer, victim,
				"SAVING (%s): Killer==%s  Target==%s vnum==%d Level==%d base_save==%d save_equip==%d save_awake=-%d result_save=%d\r\n",
				saving_name.find(saving)->second.c_str(),
				GET_NAME(killer), GET_NAME(victim), GET_MOB_VNUM(victim), GetRealLevel(victim),
				GetBasicSave(victim, saving, false), GetSave(victim, saving),
				skills::GetSkill(victim, ESkill::kAwake) / 5, save);
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
	// Saving-throw debug trace: the 3 hardcoded immortal-name
	// short-circuits (Верий/Кудояр/Рогоза) were removed -- send_to_TC already gates the
	// message on EPrf::kTester / kCoderinfo / IsImpl, which is the correct way to opt in.
	if (number(1, 100) <= 5 || (AFF_FLAGGED(victim, EAffect::kHold) && type == ESaving::kReflex)) { //абсолютный фейл
		save /= 2;
		sprintf(smallbuf, "Тестовое сообщение: &RПротивник %s (%d), ваш бонус: %d, спас '%s' противника: %d, random -200..200: %d, критудача: ДА, шанс успеха: %2.2f%%.\r\n&n",
				GET_NAME(victim), GetRealLevel(victim), ext_apply, saving_name.find(type)->second.c_str(), save, rnd, ((std::clamp(save +ext_apply, -200, 200) + 200) / 400.) * 100.);
		spell_trace::Line(killer, nullptr, "%s", smallbuf);
	} else {
		sprintf(smallbuf, "Тестовое сообщение: Противник %s (%d), ваш бонус: %d, спас '%s' противника: %d, random -200..200: %d, критудача: НЕТ, шанс успеха: %2.2f%%.\r\n",
				GET_NAME(victim), GetRealLevel(victim), ext_apply, saving_name.find(type)->second.c_str(), save, rnd, ((std::clamp(save +ext_apply, -200, 200) + 200) / 400.) * 100.);
		spell_trace::Line(killer, nullptr, "%s", smallbuf);
	}
	save += ext_apply;    // внешний модификатор (обычно +каст)

	if (save <= rnd) {
		// савинги прошли
		return true;
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
