// ============================================================================
// recalc_mob_params_by_vnum.cpp
// Пересчёт параметров мобов:
//   1) RecalcMobParamsByVnum(vnum, [apply_to_instances])  ? прототип и/или все инстансы
//   2) RecalcMobParamsInZone(zone_vnum, [mob_vnum_filter]) ? только инстансы в зоне
//   3) RecalcMobParamsInZoneWithLevel(zone_vnum, set_level, [mob_vnum_filter])
// Команды:
//   - recalc_mob  <vnum> [proto]
//   - recalc_zone <zone_vnum> <level> [mob_vnum]
// ============================================================================

#include "mob_params_setter.h"
#include "gameplay/classes/mob_classes_info.h"
#include "engine/db/global_objects.h"   // MUD::MobClasses(), character_list, mob_proto, top_of_mobt
#include "engine/entities/char_data.h"

#include "utils/utils.h"                // one_argument, two_arguments, three_arguments, str_cmp, GET_ROOM_VNUM
#include "engine/core/comm.h"           // SendMsgToChar, mudlog

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <vector>

// ---------------------------- SYSLOG helper ----------------------------------
static inline void SysInfo(const char *fmt, ...) {
	char buf[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
}

// ------------------------ rnum <- vnum (без real_mobile) ---------------------
extern CharData *mob_proto;   // массив прототипов
extern int top_of_mobt;       // последний индекс прототипов (включительно)

static inline int ResolveMobRnumByVnum(int vnum) {
	for (int r = 0; r <= top_of_mobt; ++r) {
		if (GET_MOB_VNUM(&mob_proto[r]) == vnum)
			return r;
	}
	return -1;
}

// ------------------------ Вспомогательные функции ----------------------------

// Единая точка истины: проверка роли без ручных "-1" к битсетам
inline bool HasRole(const CharData* ch, EMobClass role) {
	if (!ch) return false;
	// валидные роли: (kBoss .. kTotal-1)
	if (role <= EMobClass::kUndefined || role >= EMobClass::kTotal) return false;
	const unsigned base  = static_cast<unsigned>(EMobClass::kBoss);
	const unsigned index = static_cast<unsigned>(role) - base; // сдвиг к битсету
	return ch->get_role(index);
}

static inline std::vector<EMobClass> EnumRoles(const CharData *ch) {
	std::vector<EMobClass> out;
	if (!ch) return out;
	for (int u = static_cast<int>(EMobClass::kBoss);
	     u < static_cast<int>(EMobClass::kTotal); ++u) {
		const auto role = static_cast<EMobClass>(u);
		if (HasRole(ch, role)) out.push_back(role);
	}
	return out;
}

static inline int GetMobLevel(const CharData *ch) {
	return std::max(1, GetRealLevel(ch));
}

// Установка уровня NPC-инстансу (используем предоставленный сеттер).
static inline void SetMobLevel(CharData* ch, int level) {
	if (!ch || !ch->IsNpc()) return;
	const int lvl = std::max(1, level);
	ch->set_level(lvl);
	SysInfo("SYSINFO: SetMobLevel: mob_vnum=%d -> level=%d.", GET_MOB_VNUM(ch), lvl);
}

// --------------------- Комнаты/Зоны (по макросам) ----------------------
// Получить комнатный VNUM по указателю персонажа
static inline int GetRoomVnumFromChar(const CharData* ch) {
	if (!ch || ch->in_room < 0) return -1;
	return GET_ROOM_VNUM(ch->in_room); // макрос из utils.h
}

// Получить VNUM зоны по VNUM комнаты.
// NOTE: если у вас зона определяется иначе ? подставь свою реализацию.
// По умолчанию классическая схема: 30001..30099 -> зона 300 (деление на 100).
static inline int GetZoneVnumFromRoomVnum(int room_vnum) {
	if (room_vnum < 0) return -1;
	return room_vnum / 100;
}

// -------------------------- Пересчёт по VNUM ---------------------------------
// Пересчитать параметры для моба по vnum.
// apply_to_instances = true -> также применить ко всем живым мобам с этим vnum (по всему миру).
bool RecalcMobParamsByVnum(int vnum, bool apply_to_instances = true) {
	SysInfo("SYSINFO: RecalcMobParams start (vnum=%d).", vnum);

	const int rnum = ResolveMobRnumByVnum(vnum);
	if (rnum < 0) {
		SysInfo("SYSERROR: RecalcMobParams: vnum %d not found (rnum<0).", vnum);
		return false;
	}

	CharData *proto = &mob_proto[rnum];
	if (!proto) {
		SysInfo("SYSERROR: RecalcMobParams: vnum %d has null proto.", vnum);
		return false;
	}

	// --- применяем к прототипу
	SysInfo("SYSINFO: RecalcMobParams: applying to proto (vnum=%d, rnum=%d).", vnum, rnum);

	auto protoRes = mob_classes::MobParamsSetter(*proto) // ctor по ссылке
		.WithIsMob([](const CharData *ch){ return ch && ch->IsNpc(); })
		.WithGetLevel([](const CharData *ch){ return GetMobLevel(ch); })
		.WithEnumerateRoles([](const CharData *ch){ return EnumRoles(ch); })
		.OnSetSaving([](CharData *ch, ESaving id, int v){ SetSave(ch, id, v); })
		.OnSetResist([](CharData *ch, EResist id, int v){ GET_RESIST(ch, id) = v; })
		.OnSetSkill ([](CharData *ch, ESkill  id, int v){ ch->set_skill(id, v); })
		.UseDeviation(true)                         // при необходимости отключай
		.UseFallbackIfNoRoles(true)                 // если ролей нет ? fallback
		.WithFallbackRole(EMobClass::kUndefined)
		.Apply();

	SysInfo("SYSINFO: proto result: applied=%s (savings=%zu, resists=%zu, skills=%zu).",
	        protoRes.applied ? "true" : "false",
	        protoRes.savings.size(), protoRes.resists.size(), protoRes.skills.size());

	size_t touched = 0;

	// --- прогон по всем живым инстансам (глобально)
	if (apply_to_instances) {
		for (const auto &node : character_list) {
			CharData *ch = node.get();
			if (!ch || !ch->IsNpc() || GET_MOB_VNUM(ch) != vnum) continue;

			auto r = mob_classes::MobParamsSetter(*ch)
				.WithIsMob([](const CharData *x){ return x && x->IsNpc(); })
				.WithGetLevel([](const CharData *x){ return GetMobLevel(x); })
				.WithEnumerateRoles([](const CharData *x){ return EnumRoles(x); })
				.OnSetSaving([](CharData *x, ESaving id, int v){ SetSave(x, id, v); })
				.OnSetResist([](CharData *x, EResist id, int v){ GET_RESIST(x, id) = v; })
				.OnSetSkill ([](CharData *x, ESkill  id, int v){ x->set_skill(id, v); })
				.UseDeviation(true)
				.UseFallbackIfNoRoles(true)
				.WithFallbackRole(EMobClass::kUndefined)
				.Apply();

			++touched;
			SysInfo("SYSINFO: instance recalc: vnum=%d -> applied=%s.",
			        vnum, r.applied ? "true" : "false");
		}
	}

	SysInfo("SYSINFO: RecalcMobParams done (vnum=%d). instances_touched=%zu.", vnum, touched);
	return protoRes.applied || touched > 0;
}

// --------------------- Пересчёт инстансов в зоне без учёта уровня -----------------------------
// Пересчитать параметры всех инстансов мобов в указанной зоне.
//   zone_vnum       ? номер зоны по VNUM (например, комната 30001 -> зона 300).
//   mob_vnum_filter ? если >0, пересчитываем только этот vnum моба; иначе ? всех мобов зоны.
// Прототипы НЕ трогаем.
bool RecalcMobParamsInZone(int zone_vnum, int mob_vnum_filter = -1) {
	SysInfo("SYSINFO: RecalcInZone start (zone_vnum=%d, mob_filter=%d).",
	        zone_vnum, mob_vnum_filter);

	size_t touched = 0;

	for (const auto &node : character_list) {
		CharData *ch = node.get();
		if (!ch || !ch->IsNpc()) continue;

		// Фильтр по vnum моба (если указан)
		const int mob_vnum = GET_MOB_VNUM(ch);
		if (mob_vnum_filter > 0 && mob_vnum != mob_vnum_filter) continue;

		// Фильтр по зоне
		const int room_vnum = GetRoomVnumFromChar(ch);
		const int mob_zone  = GetZoneVnumFromRoomVnum(room_vnum);
		if (mob_zone != zone_vnum) continue;

		// Применяем параметры к конкретной копии
		auto r = mob_classes::MobParamsSetter(*ch)
			.WithIsMob([](const CharData *x){ return x && x->IsNpc(); })
			.WithGetLevel([](const CharData *x){ return GetMobLevel(x); })
			.WithEnumerateRoles([](const CharData *x){ return EnumRoles(x); })
			.OnSetSaving([](CharData *x, ESaving id, int v){ SetSave(x, id, v); })
			.OnSetResist([](CharData *x, EResist id, int v){ GET_RESIST(x, id) = v; })
			.OnSetSkill ([](CharData *x, ESkill  id, int v){ x->set_skill(id, v); })
			.UseDeviation(true)              // при необходимости можно выключить
			.UseFallbackIfNoRoles(true)
			.WithFallbackRole(EMobClass::kUndefined)
			.Apply();

		++touched;
		SysInfo("SYSINFO: zone recalc: room_vnum=%d zone=%d mob_vnum=%d -> applied=%s.",
		        room_vnum, mob_zone, mob_vnum, r.applied ? "true" : "false");
	}

	SysInfo("SYSINFO: RecalcInZone done (zone_vnum=%d). instances_touched=%zu.",
	        zone_vnum, touched);
	return touched > 0;
}

// --------------------- Пересчёт в зоне с проставлением уровня ----------------
// Пересчитать параметры всех ИНСТАНСОВ мобов в зоне, предварительно проставив им уровень.
//   zone_vnum       ? номер зоны по VNUM
//   set_level       ? уровень, который надо выдать всем инстансам перед пересчётом
//   mob_vnum_filter ? если >0, ограничить конкретным vnum моба
bool RecalcMobParamsInZoneWithLevel(int zone_vnum, int set_level, int mob_vnum_filter = -1) {
	SysInfo("SYSINFO: RecalcInZoneWithLevel start (zone=%d, set_level=%d, mob_filter=%d).",
	        zone_vnum, set_level, mob_vnum_filter);

	size_t touched = 0;

	for (const auto &node : character_list) {
		CharData *ch = node.get();
		if (!ch || !ch->IsNpc()) continue;

		// Фильтр по vnum моба (если указан)
		const int mob_vnum = GET_MOB_VNUM(ch);
		if (mob_vnum_filter > 0 && mob_vnum != mob_vnum_filter) continue;

		// Фильтр по зоне
		const int room_vnum = GetRoomVnumFromChar(ch);
		const int mob_zone  = GetZoneVnumFromRoomVnum(room_vnum);
		if (mob_zone != zone_vnum) continue;

		// 1) Сначала ставим заданный уровень ИНСТАНСУ (не прототипу!)
		SetMobLevel(ch, set_level);

		// 2) Теперь пересчитываем параметры по ролям
		auto r = mob_classes::MobParamsSetter(*ch)
			.WithIsMob([](const CharData *x){ return x && x->IsNpc(); })
			.WithGetLevel([](const CharData *x){ return GetMobLevel(x); })
			.WithEnumerateRoles([](const CharData *x){ return EnumRoles(x); })
			.OnSetSaving([](CharData *x, ESaving id, int v){ SetSave(x, id, v); })
			.OnSetResist([](CharData *x, EResist id, int v){ GET_RESIST(x, id) = v; })
			.OnSetSkill ([](CharData *x, ESkill  id, int v){ x->set_skill(id, v); })
			.UseDeviation(true)
			.UseFallbackIfNoRoles(true)
			.WithFallbackRole(EMobClass::kUndefined)
			.Apply();

		++touched;
		SysInfo("SYSINFO: zone set-level+recalc: room=%d zone=%d mob_vnum=%d level=%d -> applied=%s.",
		        room_vnum, mob_zone, mob_vnum, GetMobLevel(ch), r.applied ? "true" : "false");
	}

	SysInfo("SYSINFO: RecalcInZoneWithLevel done (zone=%d). instances_touched=%zu.",
	        zone_vnum, touched);
	return touched > 0;
}

// ----------------------------- Команды (GM) ----------------------------------
//   { "recalc_mob" }
//   { "recalc_zone"}

void do_recalc_mob(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	constexpr size_t kCmdBuf = 512; // локальный размер буфера
	char arg [kCmdBuf]{};
	char arg2[kCmdBuf]{};
	two_arguments(argument, arg, arg2);

	if (!*arg) {
		SendMsgToChar(ch, "Usage: recalc_mob <vnum> [proto]\r\n");
		return;
	}

	const int  vnum       = atoi(arg);
	const bool proto_only = (*arg2 && !str_cmp(arg2, "proto"));

	const bool ok = RecalcMobParamsByVnum(vnum, /*apply_to_instances=*/!proto_only);
	SendMsgToChar(ch, "Recalc for mob %d %s.\r\n", vnum, ok ? "done" : "failed");
}

void do_recalc_zone(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	constexpr size_t kBuf = 256;
	char arg1[kBuf]{}; // zone_vnum
	char arg2[kBuf]{}; // level
	char arg3[kBuf]{}; // [mob_vnum]
	three_arguments(argument, arg1, arg2, arg3); // <zone_vnum> <level> [mob_vnum]

	if (!*arg1 || !*arg2) {
		SendMsgToChar(ch, "Usage: recalc_zone <zone_vnum> <level> [mob_vnum]\r\n");
		return;
	}

	const int zone_vnum = atoi(arg1);
	const int set_level = atoi(arg2);
	const int mob_vnum  = (*arg3 ? atoi(arg3) : -1);

	if (set_level <= 0) {
		SendMsgToChar(ch, "Level must be positive.\r\n");
		return;
	}

	SysInfo("SYSINFO: do_recalc_zone called (zone=%d, level=%d, mob_filter=%d).",
	        zone_vnum, set_level, mob_vnum);

	const bool ok = RecalcMobParamsInZoneWithLevel(zone_vnum, set_level, mob_vnum);
	if (mob_vnum > 0) {
		SendMsgToChar(ch, "Zone recalc %s. (zone=%d, level=%d, mob=%d)\r\n",
		              ok ? "done" : "no targets", zone_vnum, set_level, mob_vnum);
	} else {
		SendMsgToChar(ch, "Zone recalc %s. (zone=%d, level=%d)\r\n",
		              ok ? "done" : "no targets", zone_vnum, set_level);
	}
}
