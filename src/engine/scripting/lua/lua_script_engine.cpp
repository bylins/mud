#include "engine/scripting/lua/lua_script_engine.h"

#include "engine/db/db.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/logger.h"

#if defined(WITH_LUAJIT_PROTOTYPE)
#include <sol/sol.hpp>
#endif

namespace lua_scripting {

#if defined(WITH_LUAJIT_PROTOTYPE)
namespace {

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

const char* GetTriggerName(const Trigger *trigger)
{
	if (!trigger)
	{
		return "<null>";
	}

	return GET_TRIG_NAME(trigger);
}

void LogLuaError(const Trigger *trigger, const sol::error &err)
{
	char buf[kMaxStringLength];
	snprintf(buf, sizeof(buf), "ERROR: Lua trigger failed: name='%s', vnum=%d: %s",
		GetTriggerName(trigger), GetTriggerVnum(trigger), err.what());
	mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
}

sol::table BuildLuaContext(sol::state &lua, Trigger *trigger)
{
	sol::table ctx = lua.create_table();
	sol::table trigger_table = lua.create_table();

	trigger_table["vnum"] = GetTriggerVnum(trigger);
	trigger_table["rnum"] = trigger ? trigger->get_rnum() : -1;
	trigger_table["name"] = trigger ? trigger->get_name() : "";
	trigger_table["attach_type"] = trigger ? static_cast<int>(trigger->get_attach_type()) : 0;
	trigger_table["trigger_type"] = trigger ? trigger->get_trigger_type() : 0;
	ctx["trigger"] = trigger_table;

	return ctx;
}

int ConvertLuaValue(const sol::object &first)
{
	if (first.get_type() == sol::type::nil)
	{
		return 1;
	}
	if (first.is<bool>())
	{
		return first.as<bool>() ? 1 : 0;
	}
	if (first.get_type() == sol::type::number)
	{
		return first.as<int>();
	}

	return 1;
}

int ConvertLuaResult(const sol::protected_function_result &result, Trigger *trigger, sol::table ctx, bool call_function)
{
	if (!result.valid())
	{
		const sol::error err = result;
		LogLuaError(trigger, err);
		return 1;
	}

	if (result.return_count() == 0)
	{
		return 1;
	}

	const sol::object first = result.get<sol::object>(0);
	if (call_function && first.get_type() == sol::type::function)
	{
		sol::protected_function function = first;
		const auto function_result = function(ctx);
		return ConvertLuaResult(function_result, trigger, ctx, false);
	}

	return ConvertLuaValue(first);
}

} // namespace
#endif

int LuaScriptEngine::RunTrigger(Trigger *trigger, const LuaTriggerContext &)
{
	if (!trigger)
	{
		return 1;
	}

#if defined(WITH_LUAJIT_PROTOTYPE)
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

	sol::table ctx = BuildLuaContext(lua, trigger);
	const auto result = lua.safe_script(trigger->get_lua_script_source(), sol::script_pass_on_error);
	return ConvertLuaResult(result, trigger, ctx, true);
#else
	return 1;
#endif
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
