/**
* \file recalc_mob_params_by_vnum.cpp - a part of the Bylins engine.
 * \authors Created by Svetodar.
 * \date 25.11.2025.
 * \brief Brief description.
 * \details Detailed description.
 */

// ============================================================================
// Пересчёт параметров мобов:
//   1) RecalcMobParamsByVnum(vnum, [apply_to_instances])  ? прототип и/или все инстансы
//   2) RecalcMobParamsInZone(zone_vnum, [mob_vnum_filter]) ? только инстансы в зоне
//   3) RecalcMobParamsInZoneWithLevel(zone_vnum, set_level, [mob_vnum_filter])
// Команды:
//   - recalc_mob (пересчитать)  <vnum> [proto] (временно отключил, так как затрагивает прототипы - при необходимости раскоментить)
//   - recalc_zone (зонапересчитать) <zone_vnum> <level> [mob_vnum]
// ============================================================================

#include "mob_params_setter.h"
#include "engine/entities/char_data.h"
#include "utils/utils.h"                // one_argument, two_arguments, three_arguments, str_cmp, GET_ROOM_VNUM
#include "engine/core/comm.h"           // SendMsgToChar, mudlog

// ---------------------------- SYSLOG helper ----------------------------------

static inline void SysInfo(const char* fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
}

// ------------------------ rnum <- vnum (без real_mobile) ---------------------

extern CharData* mob_proto;  // массив прототипов
extern int top_of_mobt;      // последний индекс прототипов (включительно)

static inline int ResolveMobRnumByVnum(int vnum) {
  for (int r = 0; r <= top_of_mobt; ++r) {
    if (GET_MOB_VNUM(&mob_proto[r]) == vnum) {
      return r;
    }
  }
  return -1;
}

// ------------------------ Роли и уровень -------------------------------------

inline bool HasRole(const CharData* ch, EMobClass role) {
  if (!ch) return false;
  if (role <= EMobClass::kUndefined || role >= EMobClass::kTotal) return false;
  const unsigned base = static_cast<unsigned>(EMobClass::kBoss);
  const unsigned index = static_cast<unsigned>(role) - base;  // сдвиг к битсету
  return ch->get_role(index);
}

static inline std::vector<EMobClass> EnumRoles(const CharData* ch) {
  std::vector<EMobClass> out;
  if (!ch) return out;
  for (int u = static_cast<int>(EMobClass::kBoss);
       u < static_cast<int>(EMobClass::kTotal); ++u) {
    const auto role = static_cast<EMobClass>(u);
    if (HasRole(ch, role)) out.push_back(role);
  }
  return out;
}

static inline int GetMobLevel(const CharData* ch) {
  return std::max(1, GetRealLevel(ch));
}

// Установка уровня NPC-инстансу (единоразовая ответственность).
static inline void SetMobLevel(CharData* ch, int level) {
  if (!ch || !ch->IsNpc()) return;
  const int lvl = std::max(1, level);
  ch->set_level(lvl);
  SysInfo("SYSINFO: SetMobLevel: mob_vnum=%d -> level=%d.", GET_MOB_VNUM(ch), lvl);
}

// --------------------- Комнаты / Зоны ----------------------------------------

// Получить комнатный VNUM по указателю персонажа
static inline int GetRoomVnumFromChar(const CharData* ch) {
  if (!ch || ch->in_room < 0) return -1;
  return GET_ROOM_VNUM(ch->in_room);  // макрос из utils.h
}

// Получить VNUM зоны по VNUM комнаты (классика: 30001..30099 -> зона 300).
static inline int GetZoneVnumFromRoomVnum(int room_vnum) {
  if (room_vnum < 0) return -1;
  return room_vnum / 100;
}

// Применить параметры по ролям/конфигу к одному CharData (proto или instance).
static inline bool ApplyMobParams(CharData* ch) {
  if (!ch) return false;

  auto result = mob_classes::MobParamsSetter(*ch)
      .WithIsMob([](const CharData* x) { return x && x->IsNpc(); })
      .WithGetLevel([](const CharData* x) { return GetMobLevel(x); })
      .WithEnumerateRoles([](const CharData* x) { return EnumRoles(x); })
      .OnSetSaving([](CharData* x, ESaving id, int v) { SetSave(x, id, v); })
      .OnSetResist([](CharData* x, EResist id, int v) { GET_RESIST(x, id) = v; })
      .OnSetSkill([](CharData* x, ESkill id, int v) { x->set_skill(id, v); })
      .UseDeviation(true)                  // при необходимости можно выключить
      .UseFallbackIfNoRoles(true)
      .WithFallbackRole(EMobClass::kUndefined)
      .Apply();

  SysInfo(
      "SYSINFO: ApplyMobParams: vnum=%d applied=%s (savings=%zu, resists=%zu, skills=%zu).",
      ch->IsNpc() ? GET_MOB_VNUM(ch) : -1,
      result.applied ? "true" : "false",
      result.savings.size(), result.resists.size(), result.skills.size());

  return result.applied;
}

// -------------------------- Пересчёт по VNUM ---------------------------------

// Пересчитать параметры для ПРОТОТИПА моба по vnum.
// apply_to_instances = true -> также применить ко всем живым инстансам с этим vnum.
/* ============================================================
bool RecalcMobParamsByVnum(int vnum, bool apply_to_instances = true) {
  SysInfo("SYSINFO: RecalcMobParams start (vnum=%d).", vnum);

  const int rnum = ResolveMobRnumByVnum(vnum);
  if (rnum < 0) {
    SysInfo("SYSERROR: RecalcMobParams: vnum %d not found (rnum<0).", vnum);
    return false;
  }

  CharData* proto = &mob_proto[rnum];
  if (!proto) {
    SysInfo("SYSERROR: RecalcMobParams: vnum %d has null proto.", vnum);
    return false;
  }

  // Применяем к прототипу (single responsibility внутри ApplyMobParams).
  SysInfo("SYSINFO: RecalcMobParams: applying to proto (vnum=%d, rnum=%d).", vnum, rnum);
  const bool proto_ok = ApplyMobParams(proto);

  size_t touched = 0;
  if (apply_to_instances) {
    for (const auto& node : character_list) {
      CharData* ch = node.get();
      if (!ch || !ch->IsNpc() || GET_MOB_VNUM(ch) != vnum) continue;
      ++touched;
      const bool ok = ApplyMobParams(ch);
      SysInfo("SYSINFO: instance recalc: vnum=%d -> applied=%s.",
              vnum, ok ? "true" : "false");
    }
  }

  SysInfo("SYSINFO: RecalcMobParams done (vnum=%d). instances_touched=%zu.",
          vnum, touched);
  return proto_ok || touched > 0;
}
============================================================ */

// --------------------- Проставить уровни инстансам (мобам) в зоне --------------------

// Только проставить уровень всем инстансам зоны (опционально ? только конкретному vnum).
// НИЧЕГО больше (single responsibility).
void SetLevelsForInstancesInZone(int zone_vnum, int set_level, int mob_vnum_filter = -1) {
	SysInfo("SYSINFO: SetLevelsInZone start (zone=%d, set_level=%d, mob_filter=%d).", zone_vnum, set_level, mob_vnum_filter);
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	MobRnum mrn_first = zone_table[zrn].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn].RnumMobsLocation.second;

	for (MobRnum mrn = mrn_first; mrn <= mrn_last; mrn++) {
		const int mob_vnum = mob_index[mrn].vnum;

	 	if (mob_vnum_filter > 0 && mob_vnum != mob_vnum_filter)
			continue;
		mob_proto[mrn].set_level(set_level);
		SysInfo("SYSINFO: SetLevelsInZone: mob_vnum=%d level=%d.", mob_vnum, set_level);
	}
	SysInfo("SYSINFO: SetLevelsInZone done (zone=%d)", zone_vnum);
}

// --------------------- Пересчитать инстансы в зоне ---------------------------

// Только применить роли/конфиг ко всем инстансам зоны (опционально ? фильтр по vnum).
// НИЧЕГО больше (single responsibility).
void RecalcMobParamsInZone(int zone_vnum, int mob_vnum_filter = -1) {
	SysInfo("SYSINFO: RecalcInZone start (zone_vnum=%d, mob_filter=%d).", zone_vnum, mob_vnum_filter);
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	MobRnum mrn_first = zone_table[zrn].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn].RnumMobsLocation.second;

	for (MobRnum mrn = mrn_first; mrn <= mrn_last; mrn++) {
		const int mob_vnum = mob_index[mrn].vnum;

	 	if (mob_vnum_filter > 0 && mob_vnum != mob_vnum_filter)
			continue;
		ApplyMobParams(&mob_proto[mrn]);
//    SysInfo("SYSINFO: zone recalc: room_vnum=%d zone=%d mob_vnum=%d -> applied=%s.",
//          room_vnum, mob_zone, mob_vnum, ok ? "true" : "false");
	}
	SysInfo("SYSINFO: RecalcInZone done");
}

// --------------------- Оркестратор: уровень -> пересчёт ----------------------

// Последовательность из двух одноцелевых шагов:
//   1) SetLevelsForInstancesInZone(...)
//   2) RecalcMobParamsInZone(...)
bool RecalcMobParamsInZoneWithLevel(int zone_vnum, int set_level, int mob_vnum_filter = -1) {
	SysInfo("SYSINFO: RecalcInZoneWithLevel start (zone=%d, set_level=%d, mob_filter=%d).", zone_vnum, set_level, mob_vnum_filter);
	SetLevelsForInstancesInZone(zone_vnum, set_level, mob_vnum_filter);
	RecalcMobParamsInZone(zone_vnum, mob_vnum_filter);
	SysInfo("SYSINFO: RecalcInZoneWithLevel done.");
	return true;
}


//void do_recalc_mob(CharData* ch, char* argument, int /*cmd*/, int /*subcmd*/) {
//  constexpr size_t kCmdBuf = 512;
//  char arg[kCmdBuf]{};
//  char arg2[kCmdBuf]{};
//  two_arguments(argument, arg, arg2);
//
//  if (!*arg) {
//    SendMsgToChar(ch, "Usage: recalc_mob <vnum> [proto]\r\n");
//    return;
//  }
//
//  const int vnum = atoi(arg);
//  const bool proto_only = (*arg2 && !str_cmp(arg2, "proto"));
//
//
//  const bool ok = RecalcMobParamsByVnum(vnum, /*apply_to_instances=*/!proto_only);
//  SendMsgToChar(ch, "Recalc for mob %d %s.\r\n", vnum, ok ? "done" : "failed");
//}

void do_recalc_zone(CharData* ch, char* argument, int /*cmd*/, int /*subcmd*/) {
  constexpr size_t kBuf = 256;
  char arg1[kBuf]{};  // zone_vnum
  char arg2[kBuf]{};  // level
  char arg3[kBuf]{};  // [mob_vnum]
  three_arguments(argument, arg1, arg2, arg3);  // <zone_vnum> <level> [mob_vnum]

  if (!*arg1 || !*arg2) {
    SendMsgToChar(ch, "Usage: recalc_zone <zone_vnum> <level> [mob_vnum]\r\n");
    return;
  }

  const int zone_vnum = atoi(arg1);
  const int set_level = atoi(arg2);
  const int mob_vnum = (*arg3 ? atoi(arg3) : -1);

  if (set_level <= 0) {
    SendMsgToChar(ch, "Level must be positive.\r\n");
    return;
  }

  SysInfo("SYSINFO: do_recalc_zone called (zone=%d, level=%d, mob_filter=%d).",
          zone_vnum, set_level, mob_vnum);

  const bool ok = RecalcMobParamsInZoneWithLevel(zone_vnum, set_level, mob_vnum);
  if (mob_vnum > 0) {
    SendMsgToChar(ch,
                  "Zone recalc %s. (zone=%d, level=%d, mob=%d)\r\n",
                  ok ? "done" : "no targets", zone_vnum, set_level, mob_vnum);
  } else {
    SendMsgToChar(ch,
                  "Zone recalc %s. (zone=%d, level=%d)\r\n",
                  ok ? "done" : "no targets", zone_vnum, set_level);
  }
}