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
	if (!ch || !ch->IsNpc()) {
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

static inline int clamp_nonneg_plain(int v) {
	return v < 0 ? 0 : v;
}

static inline int clamp_resist_plain(int v) {
	v = clamp_nonneg_plain(v);
	if (v > kMaxMobResist) {
		v = kMaxMobResist;
	}
	return v;
}

static inline int round_to_int_plain(double x) {
	return (int)(x >= 0.0 ? x + 0.5 : x - 0.5);
}

static int CalcBaseValue(
	const mob_classes::MobClassInfo::ParametersData *p,
	int level,
	int low_skill_lvl,
	int apply_low_inc) {
	if (level < 1) {
		level = 1;
	}

	const int steps = level - 1;
	const int use_low = apply_low_inc && p->low_increment > 0.0f && level <= low_skill_lvl;
	const double inc = use_low ? (double)p->low_increment
	                           : (double)p->increment;
	const double raw = (double)p->base + inc * (double)steps;
	return round_to_int_plain(raw);
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

// ------------------------ Основное применение к одному мобу ------------------
//
// Возвращает true, если хоть что-то было применено.
//
static bool ApplyMobParams(CharData* ch) {
	if (!ch || !ch->IsNpc()) {
		return false;
	}

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

		// -------- Savings --------
		for (auto it = info->savings_map.begin(); it != info->savings_map.end(); ++it) {
			ESaving id = it->first;
			p_data = &it->second;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 0);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = clamp_nonneg_plain(v);
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
			v = clamp_resist_plain(v);

			GET_RESIST(ch, id) = v;
			applied_any = 1;
		}

		// -------- Skills --------
		for (auto it = info->mob_skills_map.begin(); it != info->mob_skills_map.end(); ++it) {
			ESkill id = it->first;
			p_data = &it->second;

			int v = CalcBaseValue(p_data, level, info->low_skill_lvl, 1);
			v = ApplyDeviationPlain(v, p_data->deviation);
			v = clamp_nonneg_plain(v);

			ch->set_skill(id, v);
			applied_any = 1;
		}

		// -------- Spells --------
		for (std::size_t i = 0; i < info->mob_spells.size(); ++i) {
			ESpell spell_id = info->mob_spells[i];

			//Выдаём мобу спелл:
			ch->mob_specials.have_spell = true;
			SET_SPELL_MEM(ch, spell_id, 6);

			applied_any = true;
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
