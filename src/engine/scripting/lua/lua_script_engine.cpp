#include "engine/scripting/lua/lua_internal.h"

namespace lua_scripting {

int LuaScriptEngine::RunTrigger(Trigger *trigger, const LuaTriggerContext &ctx)
{
	if (!trigger)
	{
		return 1;
	}

#if defined(WITH_LUAJIT_PROTOTYPE)
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
	HardenLuaState(lua);
	ConfigureLuaGc(lua);

	LuaRuntimeContext runtime;
	runtime.trigger = trigger;
	runtime.owner = ctx.owner;
	runtime.entity_handles = lua.create_table();

	LuaExecutionBudget budget;
	InstallLuaRuntimeLimits(lua, budget);

	lua["mud"] = BuildMudNamespace(lua, runtime);
	sol::table lua_ctx = BuildLuaContext(lua, ctx, runtime);
	const auto result = lua.safe_script(trigger->get_lua_script_source(), sol::script_pass_on_error);
	const auto converted_result = ConvertLuaResult(result, runtime, lua_ctx, true);

	ClearLuaRuntimeLimits(lua);
	return converted_result;
#else
	(void) ctx;
	return 1;
#endif
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
