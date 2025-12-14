/**
 * \file recalc_mob_params_by_vnum.cpp - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 10.11.2025.
 * \brief Пересчёт параметров мобов по зонам/ролям.
 */

#include "gameplay/classes/mob_classes_info.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "utils/utils.h"

// ------------------------ Роли и уровень -------------------------------------

inline bool HasRole(const CharData* ch, EMobClass role) {
	if (!ch) {
		return false;
	}
	if (role <= EMobClass::kUndefined || role >= EMobClass::kTotal) {
		return false;
	}
	const unsigned base  = (unsigned)EMobClass::kBoss;
	const unsigned index = (unsigned)role - base;  // сдвиг к битсету
	return ch->get_role(index);
}

// Заполняет массив ролей, возвращает количество в *count
static void EnumRoles(const CharData* ch, EMobClass* roles, int* count) {
	*count = 0;
	if (!ch) {
		return;
	}

	for (int u = (int)EMobClass::kBoss; u < (int)EMobClass::kTotal; ++u) {
		EMobClass role = (EMobClass)u;
		if (HasRole(ch, role)) {
			roles[*count] = role;
			(*count)++;
			if (*count >= MOB_ROLE_COUNT) {
				break;  // защитимся от выхода за границу массива
			}
		}
	}
}

static inline int GetMobLevel(const CharData* ch) {
	int lvl = GetRealLevel(ch);
	if (lvl < 1) {
		lvl = 1;
	}
	return lvl;
}

static inline void SetMobLevel(CharData* ch, int level) {
	if (!ch || !ch->IsNpc() || IS_CHARMICE(ch)) {
		return;
	}
	if (level < 1) {
		level = 1;
	}
	ch->set_level(level);
}

// ------------------------ Комнаты / Зоны -------------------------------------
static inline int GetRoomVnumFromChar(const CharData* ch) {
	if (!ch || ch->in_room < 0) {
		return -1;
	}
	return GET_ROOM_VNUM(ch->in_room);
}

static inline int GetZoneVnumFromRoomVnum(int room_vnum) {
	if (room_vnum < 0) {
		return -1;
	}
	return room_vnum / 100;
}

// =====================================
//      ПРИМЕНЕНИЕ ПАРАМЕТРОВ
// =====================================

static const int kMaxMobResist = 75;
static const int kMaxMobMorale = 200;

static int CalcBaseValue(
	const mob_classes::MobClassInfo::ParametersData *p,
	int level,
	int low_skill_lvl,
	int apply_low_inc) {
	if (level < 1) {
		level = 1;
	}

	const int start_lvl = std::max(1, p->min_lvl);

	if (level < start_lvl) {
		return p->base;
	}

	const int steps = level - start_lvl;

	const int use_low = apply_low_inc && p->low_increment > 0.0f && level <= low_skill_lvl;
	const double inc = use_low ? (double)p->low_increment
							   : (double)p->increment;
	const double raw = (double)p->base + inc * (double)steps;

	return static_cast<int>(std::lround(raw));
}


// применение deviation (случайная дельта из [-deviation; deviation])
static int ApplyDeviationPlain(int v, int deviation) {
	if (deviation <= 0) {
		return v;
	}
	int delta = number(-deviation, deviation);
	return v + delta;
}

// найти описание класса моба
static const mob_classes::MobClassInfo *FindMobClassInfoPlain(EMobClass id) {
	for (auto it = MUD::MobClasses().begin(); it != MUD::MobClasses().end(); ++it) {
		const mob_classes::MobClassInfo &info = *it;
		if (info.GetId() == id && info.IsAvailable()) {
			return &info;
		}
	}
	return nullptr;
}

static void RemoveAllMobSpells(CharData *ch) {
	if (!ch) {
		return;
	}

	ch->mob_specials.have_spell = false;

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		if (MUD::Spells().IsValid(spell_id)) {
			SET_SPELL_MEM(ch, spell_id, 0);
		}
	}
}

// ------------------------ Base stats helpers ------------------------
static void ApplyBaseStatToChar(CharData *ch, EBaseStat stat, int value) {
	if (!ch) {
		return;
	}

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


// ------------------------ Основное применение к одному мобу ------------------
//
// Возвращает true, если хоть что-то было применено.
//
static bool ApplyMobParams(CharData* ch) {
	if (!ch || !ch->IsNpc() || IS_CHARMICE(ch)) {
		return false;
	}

	// Количество атак всегда фиксируем в 1
	ch->mob_specials.extra_attack = 0;

	//Очищаем ненужное
	RemoveAllSkills(ch);
	RemoveAllMobSpells(ch);
	AFF_FLAGS(ch).unset(EAffect::kFireShield);
	AFF_FLAGS(ch).unset(EAffect::kMagicGlass);

	int level = GetMobLevel(ch);

	// Собираем роли в статический массив
	EMobClass roles[MOB_ROLE_COUNT];
	int role_count = 0;
	EnumRoles(ch, roles, &role_count);

	// Fallback-роль, если ролей нет
	if (role_count == 0) {
		const mob_classes::MobClassInfo *fb = FindMobClassInfoPlain(EMobClass::kUndefined);
		if (fb != nullptr) {
			roles[0] = EMobClass::kUndefined;
			role_count = 1;
		}
	}

	if (role_count == 0) {
		return false;
	}

	int applied_any = 0;

	// Обходим роли
	for (int r = 0; r < role_count; ++r) {
		EMobClass role = roles[r];
		const mob_classes::MobClassInfo *info = FindMobClassInfoPlain(role);
		if (!info) {
			continue;
		}

		const mob_classes::MobClassInfo::ParametersData *p_data;

		// -------- Base stats --------
		for (auto it = info->base_stats_map.begin();
			it != info->base_stats_map.end(); ++it) {
			EBaseStat id = it->first;
			p_data = &it->second;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);

			ApplyBaseStatToChar(ch, id, v);
			applied_any = 1;
			}


		// -------- Savings --------
		for (auto it = info->savings_map.begin(); it != info->savings_map.end(); ++it) {
			ESaving id = it->first;
			p_data = &it->second;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);
			v = -v;	// <<< вот это добавляем: применяем сейв как уменьшение

			SetSave(ch, id, v);
			applied_any = 1;
		}

		// -------- Resists --------
		for (auto it = info->resists_map.begin(); it != info->resists_map.end(); ++it) {
			EResist id = it->first;
			p_data = &it->second;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, std::min(v, kMaxMobResist));

			GET_RESIST(ch, id) = v;
			applied_any = 1;
		}

		// -------- Extra resists (MR / PR / AR) --------
		if (info->has_magic_resist) {
			p_data = &info->magic_resist;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, std::min(v, kMaxMobResist));

			GET_MR(ch) = v;
			applied_any = 1;
		}

		if (info->has_physical_resist) {
			p_data = &info->physical_resist;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, std::min(v, kMaxMobResist));

			GET_PR(ch) = v;
			applied_any = 1;
		}

		if (info->has_affect_resist) {
			p_data = &info->affect_resist;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, std::min(v, kMaxMobResist));

			GET_AR(ch) = v;
			applied_any = 1;
		}

		// -------- Combat stats (armour / absorb) --------
		if (info->has_armour) {
			p_data = &info->armour;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);

			GET_ARMOUR(ch) = v;
			applied_any = 1;
		}

		if (info->has_absorb) {
			p_data = &info->absorb;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);

			GET_ABSORBE(ch) = v;
			applied_any = 1;
		}

		// -------- Damage dice (ndd / sdd) --------
		if (info->has_dam_n_dice) {
			p_data = &info->dam_n_dice;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			if (v < 1) {
				v = 1;
			}

			GET_NDD(ch) = v;
			applied_any = 1;
		}

		if (info->has_dam_s_dice) {
			p_data = &info->dam_s_dice;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			if (v < 1) {
				v = 1;
			}

			GET_SDD(ch) = v;
			applied_any = 1;
		}

		// -------- Hitroll / morale / cast success --------
		if (info->has_hitroll) {
			p_data = &info->hitroll;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);

			GET_HR(ch) = v;
			applied_any = 1;
		}

		if (info->has_morale) {
			p_data = &info->morale;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);

			GET_MORALE(ch) = v;
			applied_any = 1;
		}

		if (info->has_cast_success) {
			p_data = &info->cast_success;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);

			GET_CAST_SUCCESS(ch) = v;
			applied_any = 1;
		}

		// -------- HP / size / exp / likes_work --------

		if (info->has_hit_points) {
			p_data = &info->hit_points;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);
			if (v < 1) {
				v = 1;
			}

			ch->set_hit(v);
			applied_any = 1;
		}

		if (info->has_size) {
			p_data = &info->size;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, std::min(v, 100));

			GET_SIZE(ch) = v;
			applied_any = 1;
		}


		if (info->has_exp) {
			p_data = &info->exp;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);

			ch->set_exp((long)v);
			applied_any = 1;
		}

		if (info->has_likes_work) {
			p_data = &info->likes_work;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, std::min(v, 100));

			GET_LIKES(ch) = v;
			applied_any = 1;
		}

		// -------- Percent phys / spell damage --------

		if (info->has_phys_damage) {
			p_data = &info->phys_damage;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);

			ch->add_abils.percent_physdam_add = v;
			applied_any = 1;
		}

		if (info->has_spell_power) {
			p_data = &info->spell_power;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);

			ch->add_abils.percent_spellpower_add = v;
			applied_any = 1;
		}


		// -------- Skills --------
		for (auto it = info->mob_skills_map.begin(); it != info->mob_skills_map.end(); ++it) {
			ESkill id = it->first;
			p_data = &it->second;

			if (level < p_data->min_lvl) {
				continue;
			}

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 1);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = std::max(0, v);

			ch->set_skill(id, v);
			applied_any = 1;
		}

		// ------- Spells --------
		for (auto it = info->mob_spells_map.begin(); it != info->mob_spells_map.end(); ++it) {
			ESpell spell_id = it->first;
			p_data = &it->second;

			if (level < p_data->min_lvl) {
				continue;
			}

			int charges = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			charges = ApplyDeviationPlain(charges, p_data->deviation);
			charges = std::max(0, charges);

			if (charges > 0) {
				ch->mob_specials.have_spell = true;
				SET_SPELL_MEM(ch, spell_id, charges);
				applied_any = 1;
			}
		}
	}
	return applied_any != 0;
}

// --------------------- Пересчёт в зоне ---------------------------------------

void RecalcMobParamsInZone(int zone_vnum, int mob_vnum_filter = -1) {
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	MobRnum mrn_first = zone_table[zrn].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn].RnumMobsLocation.second;

	for (MobRnum mrn = mrn_first; mrn <= mrn_last; mrn++) {
		const int mob_vnum = mob_index[mrn].vnum;

		if (mob_vnum_filter > 0 && mob_vnum != mob_vnum_filter)
			continue;
		ApplyMobParams(&mob_proto[mrn]);
	}
}


// --------------------- Проставить уровни инстансам ---------------------------

void SetLevelsForInstancesInZone(int zone_vnum, int set_level, int mob_vnum_filter = -1) {
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	MobRnum mrn_first = zone_table[zrn].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn].RnumMobsLocation.second;

	for (MobRnum mrn = mrn_first; mrn <= mrn_last; mrn++) {
		const int mob_vnum = mob_index[mrn].vnum;

		if (mob_vnum_filter > 0 && mob_vnum != mob_vnum_filter)
			continue;
		mob_proto[mrn].set_level(set_level);
	}
}

// --------------------- Комбинированный пересчёт ------------------------------
bool RecalcMobParamsInZoneWithLevel(int zone_vnum, int set_level, int mob_vnum_filter = -1) {
	SetLevelsForInstancesInZone(zone_vnum, set_level, mob_vnum_filter);
	RecalcMobParamsInZone(zone_vnum, mob_vnum_filter);
	return true;
}

// --------------------- Команда recalc_zone -----------------------------------
void do_recalc_zone(CharData* ch, char* argument, int /*cmd*/, int /*subcmd*/) {

	const size_t kBuf = 256;
	char arg1[kBuf]{};  // zone_vnum
	char arg2[kBuf]{};  // level
	char arg3[kBuf]{};  // [mob_vnum]

	// <zone_vnum> <total_level_with_remorts> [mob_vnum]
	three_arguments(argument, arg1, arg2, arg3);

	if (!*arg1 || !*arg2) {
		SendMsgToChar(ch,
			"Usage: recalc_zone <zone_vnum> <total_level_with_remorts> [mob_vnum]\r\n");
		return;
	}

	const int zone_vnum = atoi(arg1);
	const int set_level = atoi(arg2);
	const int mob_vnum  = (*arg3 ? atoi(arg3) : -1);


	if (zone_vnum < 30000) {
		SendMsgToChar(ch,
			"Zone %d is not a dungeon. Recalc is allowed only for zones 30000+.\r\n",
			zone_vnum);
		return;
	}

	if (set_level <= 0) {
		SendMsgToChar(ch, "Level must be positive.\r\n");
		return;
	}

	const bool ok = RecalcMobParamsInZoneWithLevel(zone_vnum, set_level, mob_vnum);
	SendMsgToChar(ch, "Zone recalc %s. (zone=%d, level=%d%s%s)\r\n",
		ok ? "done" : "no targets",
		zone_vnum, set_level,
		mob_vnum > 0 ? ", mob=" : "",
		mob_vnum > 0 ? arg3 : "");
}
