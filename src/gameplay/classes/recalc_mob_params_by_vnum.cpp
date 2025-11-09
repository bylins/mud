// ============================================================================
// recalc_mob_params_by_vnum.cpp
// ============================================================================

#include "mob_params_setter.h"
#include "gameplay/classes/mob_classes_info.h"
#include "engine/db/global_objects.h"   // MUD::MobClasses(), character_list, mob_proto, top_of_mobt
#include "engine/entities/char_data.h"

#include "utils/utils.h"                // two_arguments, str_cmp
#include "engine/core/comm.h"           // SendMsgToChar, mudlog

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

// -------------------------- Основная функция ---------------------------------
// Пересчитать параметры для моба по vnum.
// apply_to_instances = true -> также применить ко всем живым мобам с этим vnum.
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
		.UseDeviation(true)                         // для тестов: шум включён
		.UseFallbackIfNoRoles(true)                 // если ролей нет ? применим fallback
		.WithFallbackRole(EMobClass::kUndefined)
		.Apply();

	SysInfo("SYSINFO: proto result: applied=%s (savings=%zu, resists=%zu, skills=%zu).",
	        protoRes.applied ? "true" : "false",
	        protoRes.savings.size(), protoRes.resists.size(), protoRes.skills.size());

	size_t touched = 0;

	// --- прогон по всем живым инстансам
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

// ----------------------------- Команда (GM) ----------------------------------
// Зарегистрируй в интерпретаторе таблицей команд, например:
//   { "recalc_mob", POS_DEAD, do_recalc_mob, LVL_IMMORT, 0 },

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
