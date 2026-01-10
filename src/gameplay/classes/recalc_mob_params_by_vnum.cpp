/**
 * \file recalc_mob_params_by_vnum.cpp - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 10.11.2025.
 * \brief Пересчёт параметров мобов по зонам/ролям.
 **/

#include "gameplay/classes/mob_classes_info.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/magic.h"
#include "utils/utils.h"
#include "gameplay/mechanics/dungeons.cpp"

static constexpr int kWorstPossibleSaving = 300;
static constexpr int kMaxMobResist = 95;
static constexpr int kMaxMobMorale = 200;

// ------------------------ Роли и уровень -------------------------------------

// Проверяет есть ли конкретная роль у моба
inline bool HasRole(const CharData* ch, EMobClass role) {
	if (!ch) {
		return false;
	}
	if (role <= EMobClass::kUndefined || role >= EMobClass::kTotal) {
		return false;
	}
	constexpr auto base  = (unsigned)EMobClass::kBoss;
	const unsigned index = (unsigned)role - base;  // сдвиг к битсету
	return ch->get_role(index);
}

// Заполняет массив ролей, возвращает количество в *count
static void EnumRoles(const CharData* ch, EMobClass* roles, int* count) {
	*count = 0;

	for (int role_index = (int)EMobClass::kBoss; role_index < (int)EMobClass::kTotal; ++role_index) {
		auto role = (EMobClass)role_index;
		if (HasRole(ch, role)) {
			roles[*count] = role;
			(*count)++;
			if (*count >= MOB_ROLE_COUNT) {
				break;
			}
		}
	}
}

// Проверяет есть ли моба вообще какая-либо роль
static bool HasAnyMobRole(const CharData *ch) {
	for (int u = (int)EMobClass::kBoss; u < (int)EMobClass::kTotal; ++u) {
		if (HasRole(ch, (EMobClass)u)) {
			return true;
		}
	}
	return false;
}

// Проставляет роль треша, если у моба вообще нет никакой роли
static void EnsureTrashRole(CharData *ch) {
	if (HasAnyMobRole(ch)) {
		return;
	}

	// role_.bitset index = role - kBoss (как в HasRole())
	constexpr auto base = (unsigned)EMobClass::kBoss;
	const unsigned index = (unsigned)EMobClass::kTrash - base;

	ch->set_role(index, true);
}

// =====================================
//      ПРИМЕНЕНИЕ ПАРАМЕТРОВ
// =====================================

// Расчёт базового значения проставляемого параметра
static int CalcBaseValue(const mob_classes::MobClassInfo::ParametersData *param_data, int level, int remorts) {
	if (level < 1) level = 1;

	int threshold = param_data->threshold_mort;
	if (threshold < 0) threshold = 0;

	const int low_morts  = std::min(remorts, threshold);
	const int high_morts = std::max(0, remorts - threshold);

	const double inc_sum =
		(double)param_data->low_increment * (double)low_morts +
		(double)param_data->increment      * (double)high_morts;

	const double scale = (double)level / 30.0;

	const double raw =
		((double)param_data->base + inc_sum) * scale;

	return (int)std::lround(raw);
}

// --- Отклонение ---
static int ApplyDeviation(const mob_classes::MobClassInfo::ParametersData *param, int base_value) {
	if (!param) {
		return base_value;
	}

	int value = base_value;

	if (param->deviation != 0.f) {
		if (param->deviation_type == 0) {
			const int dev = (int)param->deviation;
			value += number(-dev, dev);
		} else {
			const int dev_percent = (int)param->deviation;
			const int percent = number(-dev_percent, dev_percent);
			int scale = 100 + percent;
			if (scale < 1) {
				scale = 1;
			}
			value = value * scale / 100;
		}
	}

	return value;
}

// Находит описание класса моба
static const mob_classes::MobClassInfo *FindMobClassInfoPlain(EMobClass id) {
	for (const auto & info : MUD::MobClasses()) {
			if (info.GetId() == id && info.IsAvailable()) {
			return &info;
		}
	}
	return nullptr;
}

static void RemoveAllMobSpells(CharData *ch) {

	ch->mob_specials.have_spell = false;

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		if (MUD::Spells().IsValid(spell_id)) {
			SET_SPELL_MEM(ch, spell_id, 0);
		}
	}
}

// ------------------------ Хелперы для базовых статов ------------------------
static void ApplyBaseStatToChar(CharData *ch, EBaseStat stat, int value) {

	// Ограничения: минимум 1, максимум 100
	if (value < 1) {
		value = 1;
	} else if (value > 100) {
		value = 100;
	}

	switch (stat) {
	case EBaseStat::kStr:
		ch->set_str(value);
		break;
	case EBaseStat::kDex:
		ch->set_dex(value);
		break;
	case EBaseStat::kCon:
		ch->set_con(value);
		break;
	case EBaseStat::kWis:
		ch->set_wis(value);
		break;
	case EBaseStat::kInt:
		ch->set_int(value);
		break;
	case EBaseStat::kCha:
		ch->set_cha(value);
		break;
	default:
		break;
	}
}

static int BaseStatIndex(EBaseStat stat) {
	switch (stat) {
	case EBaseStat::kStr: return 0;
	case EBaseStat::kDex: return 1;
	case EBaseStat::kCon: return 2;
	case EBaseStat::kWis: return 3;
	case EBaseStat::kInt: return 4;
	case EBaseStat::kCha: return 5;
	default: return -1;
	}
}

static EBaseStat BaseStatByIndex(int idx) {
	static constexpr EBaseStat kOrder[6] = {
		EBaseStat::kStr, EBaseStat::kDex, EBaseStat::kCon,
		EBaseStat::kWis, EBaseStat::kInt, EBaseStat::kCha
	};
	if (idx < 0 || idx >= 6) {
		return EBaseStat::kStr; // не должно случаться
	}
	return kOrder[idx];
}


// ------------------------ Основное применение к одному мобу ------------------
//
// Возвращает true, если хоть что-то было применено.
//
static bool ApplyMobParams(CharData* ch, int level, int remorts, int difficulty) {
	if (!ch || !ch->IsNpc() || IS_CHARMICE(ch)) {
		return false;
	}

	int effective_remorts = remorts;
	if (effective_remorts < 0) {
		effective_remorts = 0;
	}

	// Сложность теперь повышает УРОВЕНЬ, а не морты
	const int lvl_per_difficulty = mob_classes::GetLvlPerDifficulty();
	const int boss_add_lvl = mob_classes::GetBossAddLvl();

	int effective_level = level + difficulty * lvl_per_difficulty;

	// Если моб - босс, добавляем бонусные уровни
	if (HasRole(ch, EMobClass::kBoss)) {
		effective_level += boss_add_lvl;
	}

	if (effective_level < 1) {
		effective_level = 1;
	}

	ch->set_level(effective_level);

	// Количество атак всегда фиксируем в 1
	ch->mob_specials.extra_attack = 0;

	// --- Сохранить текущие идентификаторы навыков/заклинаний (чтобы мы могли их сохранить и применить значения по умолчанию, если они не указаны в конфиге). ---
	std::vector<ESkill> old_skills;
	old_skills.reserve(ch->get_skills_count());
	for (auto id = ESkill::kFirst; id <= ESkill::kLast; ++id) {
		if (id == ESkill::kUndefined) {
			continue;
		}
		if (ch->GetTrainedSkill(id) > 0) {
			old_skills.push_back(id);
		}
	}

	std::vector<ESpell> old_spells;
	for (auto id = ESpell::kFirst; id <= ESpell::kLast; ++id) {
		if (id == ESpell::kUndefined) {
			continue;
		}
		if (GET_SPELL_MEM(ch, id) > 0) {
			old_spells.push_back(id);
		}
	}

	// Очищаем ненужные флаги и аффекты
	AFF_FLAGS(ch).unset(EAffect::kFireShield);
	AFF_FLAGS(ch).unset(EAffect::kMagicGlass);
	ch->UnsetFlag(EMobFlag::kNotKillPunctual);
	ch->UnsetFlag(EMobFlag::kNoBash);
	ch->UnsetFlag(EMobFlag::kNoBattleExp);
	ch->UnsetFlag(EMobFlag::kNoBlind);
	ch->UnsetFlag(EMobFlag::kNoFight);
	ch->UnsetFlag(EMobFlag::kNoHammer);
	ch->UnsetFlag(EMobFlag::kNoHold);
	ch->UnsetFlag(EMobFlag::kNoOverwhelm);
	ch->UnsetFlag(EMobFlag::kNoSilence);
	ch->UnsetFlag(EMobFlag::kNoUndercut);
	ch->UnsetFlag(EMobFlag::kAware);
	ch->UnsetFlag(EMobFlag::kProtect);


	const int calc = effective_level;      // расчётный уровень с учётом difficulty и boss_add_lvl

	GET_ABSORBE(ch) = 0;
	GET_ARMOUR(ch) = 0;
	GET_INITIATIVE(ch) = 0;

	// Если есть стаб - проставляем роль разбойника
	if (ch->GetSkill(ESkill::kBackstab) > 0) {
		ch->set_role(static_cast<unsigned>(EMobClass::kRogue) -1, true);
	}

	// Если ролей нет - реально проставляем kTrash
	EnsureTrashRole(ch);

	// Собираем роли в статический массив
	EMobClass roles[MOB_ROLE_COUNT];
	int role_count = 0;
	EnumRoles(ch, roles, &role_count);

	if (role_count == 0) {
		return false;
	}

	int applied_any = 0;

	// сбрасываем модификаторы сейвов (чтобы потом в итоге получить ровно то, что в конфиге)
	SetSave(ch, ESaving::kWill, 0);
	SetSave(ch, ESaving::kCritical, 0);
	SetSave(ch, ESaving::kStability, 0);
	SetSave(ch, ESaving::kReflex, 0);

	// desired = ИТОГОВЫЕ ("полные") сейвы из конфига
	int desired_will = kWorstPossibleSaving;
	int desired_crit = kWorstPossibleSaving;
	int desired_stab = kWorstPossibleSaving;
	int desired_refl = kWorstPossibleSaving;

	int best_base[6];
	int has_base[6];
	for (int i = 0; i < 6; ++i) {
		best_base[i] = 0;
		has_base[i] = 0;
	}

	// --- План финальных навыков/заклинаний: выбираем лучшие (максимальные) из всех ролей ---
	std::map<ESkill, int> planned_skills;
	std::map<ESpell, int> planned_spells;
	std::unordered_map<EFeat, bool> planned_feats;
	std::unordered_map<EFeat, bool> configured_feats;

	int best_default_skill = 0;
	int best_default_spell = 0;

	bool is_first_role_pass = true;

	// Обходим роли
	for (int r = 0; r < role_count; ++r) {
		EMobClass role = roles[r];
		const mob_classes::MobClassInfo *info = FindMobClassInfoPlain(role);
		if (!info) {
			continue;
		}

		const mob_classes::MobClassInfo::ParametersData *p_data;

		// -------- Базовые статы --------
		for (const auto & it : info->base_stats_map) {
			EBaseStat id = it.first;
			p_data = &it.second;

			int v = CalcBaseValue(p_data, calc, effective_remorts);
			v = ApplyDeviation(p_data, v);
			v = std::max(0, v);

			int idx = BaseStatIndex(id);
			if (idx >= 0) {
				if (!has_base[idx] || v > best_base[idx]) {
					best_base[idx] = v;
					has_base[idx] = 1;
				}
				applied_any = 1;
			}
		}


		// -------- Сависы --------
		for (const auto & it : info->savings_map) {
			ESaving id = it.first;
			p_data = &it.second;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			// берём лучший, то есть минимальный
			switch (id) {
			case ESaving::kWill:
				if (base_value < desired_will) { desired_will = base_value; }
				applied_any = 1;
				break;
			case ESaving::kCritical:
				if (base_value < desired_crit) { desired_crit = base_value; }
				applied_any = 1;
				break;
			case ESaving::kStability:
				if (base_value < desired_stab) { desired_stab = base_value; }
				applied_any = 1;
				break;
			case ESaving::kReflex:
				if (base_value < desired_refl) { desired_refl = base_value; }
				applied_any = 1;
				break;
			default:
				break;
			}
		}

		// -------- Резисты --------
		for (const auto & it : info->resists_map) {
			EResist id = it.first;
			p_data = &it.second;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, std::min(base_value, kMaxMobResist));

			if (is_first_role_pass) {
				GET_RESIST(ch, id) = base_value;
			} else {
				if (base_value > GET_RESIST(ch, id)) {
					GET_RESIST(ch, id) = base_value;
				}
			}
			applied_any = 1;
		}

		// -------- Иные резисты (MR / PR / AR) --------
		if (info->has_magic_resist) {
			p_data = &info->magic_resist;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, std::min(base_value, kMaxMobResist));

			if (is_first_role_pass) {
				GET_MR(ch) = base_value;
			} else {
				if (base_value > GET_MR(ch)) {
					GET_MR(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_physical_resist) {
			p_data = &info->physical_resist;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, std::min(base_value, kMaxMobResist));

			if (is_first_role_pass) {
				GET_PR(ch) = base_value;
			} else {
				if (base_value > GET_PR(ch)) {
					GET_PR(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_affect_resist) {
			p_data = &info->affect_resist;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, std::min(base_value, kMaxMobResist));

			if (is_first_role_pass) {
				GET_AR(ch) = base_value;
			} else {
				if (base_value > GET_AR(ch)) {
					GET_AR(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		// -------- Прочие статы (броня / поглощение) --------
		if (info->has_armour) {
			p_data = &info->armour;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, base_value);

			if (is_first_role_pass) {
				GET_ARMOUR(ch) = base_value;
			} else {
				if (base_value > GET_ARMOUR(ch)) {
					GET_ARMOUR(ch) = base_value;
				}
			}
			applied_any = 1;

		}

		if (info->has_absorb) {
			p_data = &info->absorb;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, base_value);

			if (is_first_role_pass) {
				GET_ABSORBE(ch) = base_value;
			} else {
				if (base_value > GET_ABSORBE(ch)) {
					GET_ABSORBE(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		// -------- Кубики дамага --------
		if (info->has_dam_n_dice) {
			p_data = &info->dam_n_dice;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			if (base_value < 1) {
				base_value = 1;
			}

			if (is_first_role_pass) {
				GET_NDD(ch) = base_value;
			} else {
				if (base_value > GET_NDD(ch)) {
					GET_NDD(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_dam_s_dice) {
			p_data = &info->dam_s_dice;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			if (base_value < 1) {
				base_value = 1;
			}

			if (is_first_role_pass) {
				GET_SDD(ch) = base_value;
			} else {
				if (base_value > GET_SDD(ch)) {
					GET_SDD(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		// -------- Хитролы/удача/каст --------
		if (info->has_hitroll) {
			p_data = &info->hitroll;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			if (is_first_role_pass) {
				GET_HR(ch) = base_value;
			} else {
				if (base_value > GET_HR(ch)) {
					GET_HR(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_morale) {
			p_data = &info->morale;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			if (is_first_role_pass) {
				GET_MORALE(ch) = base_value;
			} else {
				if (base_value > GET_MORALE(ch)) {
					GET_MORALE(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_initiative) {
			p_data = &info->initiative;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			if (is_first_role_pass) {
				GET_INITIATIVE(ch) = base_value;
			} else {
				if (base_value > GET_INITIATIVE(ch)) {
					GET_INITIATIVE(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_cast_success) {
			p_data = &info->cast_success;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			if (is_first_role_pass) {
				GET_CAST_SUCCESS(ch) = base_value;
			} else {
				if (base_value > GET_CAST_SUCCESS(ch)) {
					GET_CAST_SUCCESS(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		// -------- HP / размер / exp / шанс применения умений --------

		if (info->has_hit_points) {
			p_data = &info->hit_points;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, base_value);
			if (base_value < 1) {
				base_value = 1;
			}

			if (is_first_role_pass) {
				ch->set_hit(base_value);
			} else {
				if (base_value > ch->get_hit()) {
					ch->set_hit(base_value);
				}
			}
			applied_any = 1;
		}

		if (info->has_size) {
			p_data = &info->size;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, std::min(base_value, 100));

			if (is_first_role_pass) {
				GET_SIZE(ch) = static_cast<sbyte>(base_value);
			} else {
				if (base_value > GET_SIZE(ch)) {
					GET_SIZE(ch) = static_cast<sbyte>(base_value);
				}
			}
			applied_any = 1;
		}


		if (info->has_exp) {
			p_data = &info->exp;

			// --- 30-й уровень: полный опыт из конфига + вклад мортов (low до порога, high после) ---
			int threshold = p_data->threshold_mort;
			if (threshold < 0) threshold = 0;

			const int remorts = std::max(0, effective_remorts);
			const int low_morts  = std::min(remorts, threshold);
			const int high_morts = std::max(0, remorts - threshold);

			const double inc_sum =
				(double)p_data->low_increment * (double)low_morts +
				(double)p_data->increment      * (double)high_morts;

			const double exp_full_30 = (double)p_data->base + inc_sum;

			// --- кривая по уровню: lvl1=1%, lvl30=100% ---
			const int lvl = std::max(1, calc);
			const double koeff = (double)(lvl - 1) / 29.0;          // 0..1
			const double exp_scale = 0.01 + 0.99 * (koeff * koeff);      // можно koeff^3, если нужно жёстче

			int base_value = (int)std::lround(exp_full_30 * exp_scale);

			// deviation применяем ПОСЛЕ кривой, чтобы ?% было относительно итогового exp на этом уровне
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, base_value);

			if (is_first_role_pass) {
				ch->set_exp(base_value);
			} else {
				if ((long)base_value > ch->get_exp()) {
					ch->set_exp(base_value);
				}
			}
			applied_any = 1;
		}


		if (info->has_likes_work) {
			p_data = &info->likes_work;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, std::min(base_value, 100));

			if (is_first_role_pass) {
				GET_LIKES(ch) = base_value;
			} else {
				if (base_value > GET_LIKES(ch)) {
					GET_LIKES(ch) = base_value;
				}
			}
			applied_any = 1;
		}

		// -------- Добавочные phys / spell дамаг --------

		if (info->has_phys_damage) {
			p_data = &info->phys_damage;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			if (is_first_role_pass) {
				ch->add_abils.percent_physdam_add = base_value;
			} else {
				if (base_value > ch->add_abils.percent_physdam_add) {
					ch->add_abils.percent_physdam_add = base_value;
				}
			}
			applied_any = 1;
		}

		if (info->has_spell_power) {
			p_data = &info->spell_power;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);

			if (is_first_role_pass) {
				ch->add_abils.percent_spellpower_add = base_value;
			} else {
				if (base_value > ch->add_abils.percent_spellpower_add) {
					ch->add_abils.percent_spellpower_add = base_value;
				}
			}
			applied_any = 1;
		}


		// -------- Умения (планируемые) --------

		// Значения умения по умолчанию (kUndefined) для данной роли
		auto def_skill_it = info->mob_skills_map.find(ESkill::kUndefined);
		if (def_skill_it != info->mob_skills_map.end()) {
			const auto *p = &def_skill_it->second;
			int dv = CalcBaseValue(p, calc, effective_remorts);
			dv = ApplyDeviation(p, dv);
			dv = std::max(0, dv);

			if (dv > best_default_skill) {
				best_default_skill = dv;
			}
		}

		for (const auto & it : info->mob_skills_map) {
			ESkill id = it.first;

			if (id == ESkill::kUndefined) {
				continue; // служебное: дефолт
			}

			p_data = &it.second;

			int base_value = CalcBaseValue(p_data, calc, effective_remorts);
			base_value = ApplyDeviation(p_data, base_value);
			base_value = std::max(0, base_value);

			if (base_value > 0) {
				auto pit = planned_skills.find(id);
				if (pit == planned_skills.end() || base_value > pit->second) {
					planned_skills[id] = base_value; // берём лучший по ролям
				}
				applied_any = 1;
			}
		}

		// ------- Заклинания (планируемые) --------

		// Значения заклинания по умолчанию (kUndefined) для данной роли
		auto def_spell_it = info->mob_spells_map.find(ESpell::kUndefined);
		if (def_spell_it != info->mob_spells_map.end()) {
			const auto *p = &def_spell_it->second;
			int dv = CalcBaseValue(p, calc, effective_remorts);
			dv = ApplyDeviation(p, dv);
			dv = std::max(0, dv);

			if (dv > best_default_spell) {
				best_default_spell = dv;
			}
		}

		for (const auto & it : info->mob_spells_map) {
			ESpell spell_id = it.first;

			if (spell_id == ESpell::kUndefined) {
				continue; // служебное: дефолт
			}

			p_data = &it.second;

			int charges = CalcBaseValue(p_data, calc, effective_remorts);
			charges = ApplyDeviation(p_data, charges);
			charges = std::max(0, charges);

			if (charges > 0) {
				auto pit = planned_spells.find(spell_id);
				if (pit == planned_spells.end() || charges > pit->second) {
					planned_spells[spell_id] = charges; // берём лучший по ролям
				}
				applied_any = 1;
			}
		}

		for (const auto &feat_pair : info->mob_feats_map) {
			const EFeat feat_id = feat_pair.first;
			const int threshold_mort = feat_pair.second;

			configured_feats[feat_id] = true;

			if (effective_remorts >= threshold_mort) {
				planned_feats[feat_id] = true;
			}
		}
		is_first_role_pass = false;
	}

	// --- Проставляем умения и заклинания ---
	// Сначала затираем, чтобы была возможность проставить нужные значения тем умениям/заклинаниям, которые прописаны мобу, но не прописаны классу
	ch->clear_skills();
	RemoveAllMobSpells(ch);

	// Проставляем умения из конфига класса
	for (const auto &key_value : planned_skills) {
		ch->set_skill(key_value.first, key_value.second);
	}

	// Применяем значения по умолчанию для умений, которые есть у моба, но не прописаны в классе
	if (best_default_skill > 0) {
		for (ESkill id : old_skills) {
			if (planned_skills.find(id) != planned_skills.end()) {
				continue;
			}
			ch->set_skill(id, best_default_skill);
		}
	}

	// Проставляем заклинания из конфига класса
	if (!planned_spells.empty()) {
		ch->mob_specials.have_spell = true;
		for (const auto &kv : planned_spells) {
			SET_SPELL_MEM(ch, kv.first, kv.second);
		}
	}

	// Применяем значения по умолчанию для заклинаний, которые есть у моба, но не прописаны в классе
	if (best_default_spell > 0) {
		for (ESpell id : old_spells) {
			if (planned_spells.find(id) != planned_spells.end()) {
				continue;
			}
			ch->mob_specials.have_spell = true;
			SET_SPELL_MEM(ch, id, best_default_spell);
		}
	}

	for (int i = 0; i < 6; ++i) {
		if (has_base[i]) {
			ApplyBaseStatToChar(ch, BaseStatByIndex(i), best_base[i]);
		}
	}

	// --- Применяем окончательные сависы ---
	int basic_saving = CalcSaving(ch, ch, ESaving::kWill, false);
	SetSave(ch, ESaving::kWill, desired_will - basic_saving);

	basic_saving = CalcSaving(ch, ch, ESaving::kCritical, false);
	SetSave(ch, ESaving::kCritical, desired_crit - basic_saving);

	basic_saving = CalcSaving(ch, ch, ESaving::kStability, false);
	SetSave(ch, ESaving::kStability, desired_stab - basic_saving);

	basic_saving = CalcSaving(ch, ch, ESaving::kReflex, false);
	SetSave(ch, ESaving::kReflex, desired_refl - basic_saving);

	// --- Проставляем окончательные способности ---
	for (const auto &feat_pair : configured_feats) {
		const EFeat feat_id = feat_pair.first;

		const bool should_have_feat =
			(planned_feats.find(feat_id) != planned_feats.end() && planned_feats[feat_id]);

		if (should_have_feat) {
			ch->SetFeat(feat_id);
		} else {
			ch->UnsetFeat(feat_id);
		}
	}


	return applied_any != 0;
}

// --------------------- Пересчёт в зоне ---------------------------------------

void RecalcMobParamsInZone(int zone_vnum, int remorts, int level, int difficulty) {
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	MobRnum mrn_first = zone_table[zrn].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn].RnumMobsLocation.second;

	for (MobRnum mrn = mrn_first; mrn <= mrn_last; ++mrn) {
		ApplyMobParams(&mob_proto[mrn], level, remorts, difficulty);
	}
}

// --------------------- Проставить уровни мобам ---------------------------
void SetLevelsForInstancesInZone(int zone_vnum, int set_level) {
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	MobRnum mrn_first = zone_table[zrn].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn].RnumMobsLocation.second;

	for (MobRnum mrn = mrn_first; mrn <= mrn_last; ++mrn) {
		mob_proto[mrn].set_level(set_level);
	}
}

// --------------------- Комбинированный пересчёт ------------------------------
bool RecalcMobParamsInZoneWithLevel(int zone_vnum, int remorts, int set_level, int difficulty) {
	SetLevelsForInstancesInZone(zone_vnum, set_level);
	RecalcMobParamsInZone(zone_vnum, remorts, set_level, difficulty);
	return true;
}

// --------------------- Команда recalc_zone -----------------------------------
void do_recalc_zone(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	constexpr size_t kBuf = 256;

	char arg1[kBuf]{}; // zone_vnum
	char arg2[kBuf]{}; // remorts
	char arg3[kBuf]{}; // player_level
	char arg4[kBuf]{}; // difficulty

	// <zone_vnum> <remorts> <player_level> <difficulty>
	argument = three_arguments(argument, arg1, arg2, arg3);
	one_argument(argument, arg4);

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4) {
		SendMsgToChar(ch,
			"Usage: recalc_zone <zone_vnum> <remorts> <player_level> <difficulty>\r\n");
		return;
	}

	const int zone_vnum		= atoi(arg1);
	const int remorts		= atoi(arg2);
	const int player_level  = atoi(arg3);
	const int difficulty    = atoi(arg4);

//	if (player_level > 30) {
//		SendMsgToChar(ch, "Player level must be <= 30.\r\n");
//		return;
//	}

	RecalcMobParamsInZoneWithLevel(zone_vnum, remorts, player_level, difficulty);
	const int added_level_by_difficulty = difficulty * mob_classes::GetLvlPerDifficulty();
	SendMsgToChar(ch,
		"Zone recalc done. (zone=%d, remorts=%d, base_lvl=%d, difficulty=%d, +lvl=%d)\r\n",
		zone_vnum, remorts, player_level, difficulty, added_level_by_difficulty);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :