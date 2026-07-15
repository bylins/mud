#include "engine/scripting/lua/lua_internal.h"
#include "engine/scripting/lua/lua_line_numbers.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/entities/char_data.h"
#include "utils/logger.h"

namespace lua_scripting {
namespace {

std::optional<int> GetLuaErrorLine(Trigger *trigger, const char *error)
{
	if (!trigger || !error)
	{
		return std::nullopt;
	}
	const std::string marker = "[string \"trigger:" + std::to_string(GetTriggerVnum(trigger)) + "\"]:";
	const auto marker_pos = std::string(error).find(marker);
	if (marker_pos == std::string::npos)
	{
		return std::nullopt;
	}
	const char *begin = error + marker_pos + marker.size();
	char *end = nullptr;
	const auto line = strtol(begin, &end, 10);
	if (end == begin || !end || *end != ':' || line < 1 || line > std::numeric_limits<int>::max())
	{
		return std::nullopt;
	}
	return static_cast<int>(line);
}

int LuaIpairs(lua_State *state)
{
	if (PushLuaLiveCollectionIpairs(state))
	{
		return 3;
	}

	lua_pushvalue(state, lua_upvalueindex(1));
	lua_pushvalue(state, 1);
	lua_call(state, 1, LUA_MULTRET);
	const auto result_count = lua_gettop(state) - 1;
	lua_remove(state, 1);
	return result_count;
}

} // namespace

LuaEntityHandle MakeCharHandle(CharData *ch, RoomRnum required_room)
{
	LuaEntityHandle handle(LuaEntityHandle::LuaEntityType::Char);
	handle.char_uid = ch ? ch->get_uid() : 0;
	handle.required_room = required_room;
	return handle;
}

LuaEntityHandle MakeObjHandle(ObjData *obj)
{
	LuaEntityHandle handle(LuaEntityHandle::LuaEntityType::Obj);
	handle.obj_id = obj ? obj->get_id() : 0;
	return handle;
}

int GetTriggerVnum(const Trigger *trigger)
{
	if (!trigger)
	{
		return 0;
	}

	const auto rnum = trigger->get_rnum();
	if (rnum < 0 || rnum >= top_of_trigt || !trig_index || !trig_index[rnum])
	{
		return 0;
	}

	return GET_TRIG_VNUM(trigger);
}

const char *GetTriggerName(const Trigger *trigger)
{
	if (!trigger)
	{
		return "<null>";
	}

	return GET_TRIG_NAME(trigger);
}

int GetOwnerVnum(const CharData *owner)
{
	if (!owner || !owner->IsNpc())
	{
		return 0;
	}

	const auto rnum = owner->get_rnum();
	if (rnum < 0 || rnum > top_of_mobt)
	{
		return 0;
	}

	return mob_index[rnum].vnum;
}

long GetOwnerUid(const CharData *owner)
{
	return owner ? owner->get_uid() : 0;
}

CharData *ResolveChar(const LuaEntityHandle &handle)
{
	if (handle.type != LuaEntityHandle::LuaEntityType::Char || handle.char_uid == 0)
	{
		return nullptr;
	}

	CharData *result = nullptr;
	character_list.foreach([&result, uid = handle.char_uid](const CharData::shared_ptr &ch) {
		if (!result && ch && ch->get_uid() == uid && !ch->purged())
		{
			result = ch.get();
		}
	});
	if (result && handle.required_room != kNowhere && result->in_room != handle.required_room)
	{
		return nullptr;
	}
	return result;
}

ObjData *ResolveObj(const LuaEntityHandle &handle)
{
	if (handle.type != LuaEntityHandle::LuaEntityType::Obj || handle.obj_id == 0)
	{
		return nullptr;
	}

	const auto obj = world_objects.find_by_id(handle.obj_id);
	return obj && !obj->get_extracted_list() ? obj.get() : nullptr;
}

bool IsValidEntity(const LuaEntityHandle &handle)
{
	return ResolveChar(handle) || ResolveObj(handle);
}

bool IsValidRoom(const LuaRoomView &view)
{
	return view.room != kNowhere && view.room >= 0 && view.room <= top_of_world && world[view.room];
}

bool LogLuaApiError(LuaRuntimeContext runtime, const char *message)
{
	char buf[kMaxStringLength];
	snprintf(buf, sizeof(buf), "(Lua trigger: %s, VNum: %d, owner vnum: %d, owner uid: %ld) : %s",
		GetTriggerName(runtime.trigger),
		GetTriggerVnum(runtime.trigger),
		GetOwnerVnum(runtime.owner),
		GetOwnerUid(runtime.owner),
		message);
	script_log(buf, LogMode::OFF);
	return false;
}

void LogLuaError(LuaRuntimeContext runtime, const char *phase, const sol::error &err)
{
	char buf[kMaxStringLength];
	snprintf(buf, sizeof(buf), "ERROR: Lua trigger failed: phase='%s', name='%s', vnum=%d, owner_vnum=%d, owner_uid=%ld: %s",
		phase,
		GetTriggerName(runtime.trigger),
		GetTriggerVnum(runtime.trigger),
		GetOwnerVnum(runtime.owner),
		GetOwnerUid(runtime.owner),
		err.what());
	std::string message(buf);
	const auto line_number = GetLuaErrorLine(runtime.trigger, err.what());
	if (line_number && runtime.trigger)
	{
		const auto source_line = GetSourceLine(runtime.trigger->get_lua_script_source(), *line_number);
		if (source_line)
		{
			message += " source_line[" + std::to_string(*line_number) + "]=\"" + *source_line + "\"";
		}
	}
	mudlog(message, BRF, kLvlBuilder, ERRLOG, true);
}

void LogLuaReturnDiagnostic(LuaRuntimeContext runtime, const sol::object &value)
{
	const auto type_name = sol::type_name(value.lua_state(), value.get_type());
	char buf[kMaxStringLength];
	snprintf(buf, sizeof(buf), "ERROR: Lua trigger returned unsupported value: name='%s', vnum=%d, owner_vnum=%d, owner_uid=%ld, type='%s'",
		GetTriggerName(runtime.trigger),
		GetTriggerVnum(runtime.trigger),
		GetOwnerVnum(runtime.owner),
		GetOwnerUid(runtime.owner),
		type_name.c_str());
	mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
}

void HardenLuaState(sol::state &lua)
{
	lua_State *state = lua.lua_state();
	lua_getglobal(state, "ipairs");
	lua_pushcclosure(state, LuaIpairs, 1);
	lua_setglobal(state, "ipairs");

	for (const auto *name : kLuaDisabledGlobals) {
		lua[name] = sol::lua_nil;
	}
}

void ConfigureLuaGc(sol::state &lua)
{
	lua_State *state = lua.lua_state();
	lua_gc(state, LUA_GCSETPAUSE, kLuaGcPause);
	lua_gc(state, LUA_GCSETSTEPMUL, kLuaGcStepMul);
}

namespace {

char kLuaExecutionBudgetRegistryKey;

void LuaInstructionHook(lua_State *state, lua_Debug *)
{
	lua_pushlightuserdata(state, &kLuaExecutionBudgetRegistryKey);
	lua_rawget(state, LUA_REGISTRYINDEX);
	auto *budget = static_cast<LuaExecutionBudget *>(lua_touserdata(state, -1));
	lua_pop(state, 1);
	if (!budget)
	{
		luaL_error(state, "Lua execution budget is unavailable");
		return;
	}

	budget->instructions_left -= kLuaInstructionHookStep;
	if (budget->instructions_left <= 0)
	{
		budget->exhausted = true;
		luaL_error(state, "Lua execution budget exceeded");
	}
}

} // namespace

void InstallLuaRuntimeLimits(sol::state &lua, LuaExecutionBudget &budget)
{
	InstallLuaRuntimeLimits(lua.lua_state(), budget);
}

void InstallLuaRuntimeLimits(lua_State *state, LuaExecutionBudget &budget)
{
	budget.instructions_left = kLuaInstructionBudget;
	budget.exhausted = false;
	lua_pushlightuserdata(state, &kLuaExecutionBudgetRegistryKey);
	lua_pushlightuserdata(state, &budget);
	lua_rawset(state, LUA_REGISTRYINDEX);
	lua_sethook(state, LuaInstructionHook, LUA_MASKCOUNT, kLuaInstructionHookStep);
}

void ClearLuaRuntimeLimits(sol::state &lua)
{
	ClearLuaRuntimeLimits(lua.lua_state());
}

void ClearLuaRuntimeLimits(lua_State *state)
{
	lua_sethook(state, nullptr, 0, 0);
	lua_pushlightuserdata(state, &kLuaExecutionBudgetRegistryKey);
	lua_pushnil(state);
	lua_rawset(state, LUA_REGISTRYINDEX);
}

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
