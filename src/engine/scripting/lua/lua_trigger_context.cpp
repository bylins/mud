#include "engine/scripting/lua/lua_internal.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

namespace lua_scripting {
namespace {

int ConvertLuaValue(LuaRuntimeContext runtime, const sol::object &first)
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

	LogLuaReturnDiagnostic(runtime, first);
	return 1;
}

} // namespace

sol::table BuildLuaContext(sol::state &lua, const LuaTriggerContext &source, LuaRuntimeContext runtime)
{
	sol::table ctx = lua.create_table();
	sol::table trigger_table = lua.create_table();

	trigger_table["vnum"] = GetTriggerVnum(runtime.trigger);
	trigger_table["rnum"] = runtime.trigger ? runtime.trigger->get_rnum() : -1;
	trigger_table["name"] = runtime.trigger ? runtime.trigger->get_name() : "";
	trigger_table["attach_type"] = runtime.trigger ? static_cast<int>(runtime.trigger->get_attach_type()) : 0;
	trigger_table["trigger_type"] = runtime.trigger ? runtime.trigger->get_trigger_type() : 0;
	ctx["trigger"] = trigger_table;
	ctx["owner"] = BuildCharView(lua, source.owner, runtime);
	ctx["actor"] = BuildCharView(lua, source.actor, runtime);
	ctx["room"] = BuildRoomView(lua, source.owner, runtime);

	return ctx;
}

int ConvertLuaResult(const sol::protected_function_result &result, LuaRuntimeContext runtime, sol::table ctx, bool call_function)
{
	if (!result.valid())
	{
		const sol::error err = result;
		LogLuaError(runtime, call_function ? "script" : "return_function", err);
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
		return ConvertLuaResult(function_result, runtime, ctx, false);
	}

	return ConvertLuaValue(runtime, first);
}

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
