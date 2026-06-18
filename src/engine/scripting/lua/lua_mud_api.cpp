#include "engine/scripting/lua/lua_internal.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/world_objects.h"
#include "engine/entities/char_data.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/random.h"

#include <limits>

namespace lua_scripting {
namespace {

constexpr long kLuaPulsesPerMudHour = kSecsPerMudHour * kPassesPerSec;
constexpr long kLuaMaxWaitPulses = std::numeric_limits<int>::max();

bool IsValidWaitTime(long hr, long min)
{
	return hr >= 0 && hr < 24 && min >= 0 && min < 60;
}

bool IsValidWaitDelay(long time)
{
	return time > 0 && time <= kLuaMaxWaitPulses;
}

ObjData *LoadObjToRoom(const sol::object &vnum, const sol::object &room)
{
	const auto room_rnum = GetRoomFromLua(room);
	if (!vnum.is<int>() || !IsValidRoom(LuaRoomView{room_rnum}))
	{
		return nullptr;
	}

	const auto obj_rnum = GetObjRnum(vnum.as<int>());
	if (obj_rnum < 0)
	{
		return nullptr;
	}

	const auto obj = world_objects.create_from_prototype_by_rnum(obj_rnum);
	if (!obj)
	{
		return nullptr;
	}

	obj->set_vnum_zone_from(zone_table[world[room_rnum]->zone_rn].vnum);
	PlaceObjToRoom(obj.get(), room_rnum);
	load_otrigger(obj.get());
	if (CheckObjDecay(obj.get()))
	{
		return nullptr;
	}
	return obj.get();
}

CharData *LoadMobToRoom(const sol::object &vnum, const sol::object &room)
{
	const auto room_rnum = GetRoomFromLua(room);
	if (!vnum.is<int>() || !IsValidRoom(LuaRoomView{room_rnum}))
	{
		return nullptr;
	}

	const auto mob_rnum = GetMobRnum(vnum.as<int>());
	if (mob_rnum < 0)
	{
		return nullptr;
	}

	auto *mob = ReadMobile(mob_rnum, kReal);
	if (!mob)
	{
		return nullptr;
	}

	PlaceCharToRoom(mob, room_rnum);
	load_mtrigger(mob);

	return mob;
}

bool MudPurge(const sol::object &lua_entity)
{
	if (!lua_entity.is<sol::table>())
	{
		return false;
	}

	sol::table entity_table = lua_entity;
	const sol::object purge = entity_table["purge"];
	if (purge.get_type() != sol::type::function)
	{
		return false;
	}

	const sol::protected_function purge_function = purge;
	const sol::protected_function_result result = purge_function();
	return result.valid() && result.get_type(0) == sol::type::boolean && result.get<bool>();
}

int GetCurrentObjectCount(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return 0;
	}

	const auto rnum = GetObjRnum(vnum.as<int>());
	if (rnum == kNothing)
	{
		return 0;
	}

	return obj_proto.actual_count(rnum);
}

int MudRandom(const sol::object &limit)
{
	if (!limit.is<int>())
	{
		return 0;
	}

	const auto n = limit.as<int>();
	return n > 0 ? number(1, n) : 0;
}

bool MudLog(LuaRuntimeContext runtime, const sol::object &message)
{
	if (!message.is<std::string>())
	{
		return false;
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return false;
	}

	char buf[kMaxStringLength];
	snprintf(buf, sizeof(buf), "(Lua trigger: %s, VNum: %d, owner vnum: %d, owner uid: %ld) : %s",
		GetTriggerName(runtime.trigger),
		GetTriggerVnum(runtime.trigger),
		GetOwnerVnum(runtime.owner),
		GetOwnerUid(runtime.owner),
		text.c_str());
	script_log(buf, LogMode::OFF);
	return true;
}

bool ParseWaitDelay(sol::variadic_args args, long &time)
{
	time = 0;
	if (args.size() == 0)
	{
		return false;
	}

	if (args[0].is<int>())
	{
		time = args[0].as<int>();
		if (args.size() > 1 && args[1].is<std::string>())
		{
			const auto suffix = args[1].as<std::string>();
			if (suffix == "t")
			{
				time *= kLuaPulsesPerMudHour;
			}
			else if (suffix == "s")
			{
				time *= kPassesPerSec;
			}
		}
		return IsValidWaitDelay(time);
	}

	if (!args[0].is<std::string>())
	{
		return false;
	}

	const auto text = args[0].as<std::string>();
	long value = 0;
	long hr = 0;
	long min = 0;
	char suffix = 0;
	if (sscanf(text.c_str(), "until %ld:%ld", &hr, &min) == 2)
	{
		if (!IsValidWaitTime(hr, min))
		{
			return false;
		}
		min += hr * 60;
		const auto target_time = (min * kLuaPulsesPerMudHour) / 60;
		const auto current_time = static_cast<long>(GlobalObjects::heartbeat().global_pulse_number() % kLuaPulsesPerMudHour)
			+ (time_info.hours * kLuaPulsesPerMudHour);
		time = current_time >= target_time
			? (kSecsPerMudDay * kPassesPerSec) - current_time + target_time
			: target_time - current_time;
		return IsValidWaitDelay(time);
	}
	if (sscanf(text.c_str(), "until %ld", &hr) == 1)
	{
		const auto parsed_hour = hr / 100;
		const auto parsed_min = hr % 100;
		if (!IsValidWaitTime(parsed_hour, parsed_min))
		{
			return false;
		}
		min = parsed_min + (parsed_hour * 60);
		const auto target_time = (min * kLuaPulsesPerMudHour) / 60;
		const auto current_time = static_cast<long>(GlobalObjects::heartbeat().global_pulse_number() % kLuaPulsesPerMudHour)
			+ (time_info.hours * kLuaPulsesPerMudHour);
		time = current_time >= target_time
			? (kSecsPerMudDay * kPassesPerSec) - current_time + target_time
			: target_time - current_time;
		return IsValidWaitDelay(time);
	}
	if (sscanf(text.c_str(), "%ld %c", &value, &suffix) >= 1)
	{
		time = value;
		if (suffix == 't')
		{
			time *= kLuaPulsesPerMudHour;
		}
		else if (suffix == 's')
		{
			time *= kPassesPerSec;
		}
		return IsValidWaitDelay(time);
	}
	return false;
}

} // namespace

int MudWait(LuaRuntimeContext runtime, sol::this_state state, sol::variadic_args args)
{
	long time = 0;
	if (!ParseWaitDelay(args, time))
	{
		LogLuaApiError(runtime, "wait: invalid delay");
		return luaL_error(state, "mud.wait: invalid delay");
	}
	if (runtime.trigger
		&& ((runtime.trigger->get_attach_type() == MOB_TRIGGER && IS_SET(GET_TRIG_TYPE(runtime.trigger), MTRIG_DEATH))
			|| (runtime.trigger->get_attach_type() == OBJ_TRIGGER && IS_SET(GET_TRIG_TYPE(runtime.trigger), OTRIG_PURGE))))
	{
		LogLuaApiError(runtime, "wait: unsupported for death/purge triggers");
		return luaL_error(state, "mud.wait: unsupported for this trigger");
	}
	if (!IsValidWaitDelay(time) || !LuaWaitRegistrySchedule(runtime, static_cast<int>(time)))
	{
		LogLuaApiError(runtime, "wait: unable to schedule");
		return luaL_error(state, "mud.wait: unable to schedule");
	}
	return lua_yield(state, 0);
}

sol::table BuildMudNamespace(sol::state &lua, LuaRuntimeContext runtime)
{
	sol::table mud = lua.create_table();
	mud["log"] = [runtime](const sol::object &message) {
		return MudLog(runtime, message);
	};
	mud["random"] = [](const sol::object &limit) {
		return MudRandom(limit);
	};
	mud["room"] = [&lua](const sol::object &vnum) {
		return BuildRoomViewByVnum(lua, vnum);
	};
	mud["load_obj"] = [&lua](const sol::object &vnum, const sol::object &room) {
		return BuildObjView(lua, LoadObjToRoom(vnum, room));
	};
	mud["load_mob"] = [&lua, runtime](const sol::object &vnum, const sol::object &room) {
		return BuildCharView(lua, LoadMobToRoom(vnum, room), runtime);
	};
	mud["purge"] = [](const sol::object &entity) {
		return MudPurge(entity);
	};
	mud["damage"] = [runtime](const sol::object &victim, const sol::object &amount, const sol::object &type) {
		return MudDamage(runtime, victim, amount, type);
	};
	mud["wait"] = [runtime](sol::this_state state, sol::variadic_args args) {
		return MudWait(runtime, state, args);
	};

	sol::table world_table = lua.create_table();
	world_table["cur_obj_count"] = [](const sol::object &vnum) {
		return GetCurrentObjectCount(vnum);
	};
	mud["world"] = world_table;

	return mud;
}

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
