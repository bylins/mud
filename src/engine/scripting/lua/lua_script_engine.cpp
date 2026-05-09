#include "engine/scripting/lua/lua_script_engine.h"

#include "engine/scripting/dg_scripts.h"
#include "utils/logger.h"

#if defined(WITH_LUAJIT_PROTOTYPE)
#include <sol/sol.hpp>
#endif

namespace lua_scripting {

#if defined(WITH_LUAJIT_PROTOTYPE)
namespace {

int ConvertLuaResult(const sol::protected_function_result &result)
{
	if (!result.valid())
	{
		const sol::error err = result;
		log("ERROR: Lua trigger failed: %s", err.what());
		return 1;
	}

	if (result.return_count() == 0)
	{
		return 1;
	}

	const sol::object first = result.get<sol::object>(0);
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

	const auto result = lua.safe_script(trigger->get_lua_script_source(), sol::script_pass_on_error);
	return ConvertLuaResult(result);
#else
	return 1;
#endif
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
