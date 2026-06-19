#include "engine/scripting/lua/lua_internal.h"

#include "engine/entities/room_data.h"

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
	if (source.owner)
	{
		ctx["owner"] = BuildCharView(lua, source.owner, runtime);
	}
	else if (source.owner_obj)
	{
		ctx["owner"] = BuildObjView(lua, source.owner_obj, runtime);
	}
	else if (source.owner_room)
	{
		ctx["owner"] = BuildRoomView(lua, LuaRoomView{GetRoomRnum(source.owner_room->vnum)}, true, runtime);
	}
	else
	{
		ctx["owner"] = sol::nil;
	}
	ctx["actor"] = BuildCharView(lua, source.actor, runtime);
	ctx["victim"] = BuildCharView(lua, source.victim, runtime);
	ctx["object"] = BuildObjView(lua, source.object, runtime);
	ctx["command"] = source.command;
	ctx["argument"] = source.argument;
	ctx["speech"] = source.speech;
	ctx["direction"] = source.direction;
	ctx["damage_amount"] = source.damage_amount;
	ctx["damage_type"] = source.damage_type;
	ctx["where"] = source.where;
	ctx["time"] = source.time;
	ctx["time_day"] = source.time_day;
	if (source.owner)
	{
		ctx["room"] = BuildRoomView(lua, source.owner, runtime);
	}
	else if (source.owner_obj)
	{
		ctx["room"] = BuildRoomView(lua, LuaRoomView{runtime.owner_room}, true, runtime);
	}
	else if (source.owner_room)
	{
		ctx["room"] = BuildRoomView(lua, LuaRoomView{GetRoomRnum(source.owner_room->vnum)}, true, runtime);
	}
	else
	{
		ctx["room"] = sol::nil;
	}

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
