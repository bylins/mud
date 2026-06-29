#include "engine/scripting/lua/lua_internal.h"

#include "engine/entities/room_data.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

namespace lua_scripting {
namespace {

RoomRnum GetContextRoom(const LuaTriggerContext &source, LuaRuntimeContext runtime)
{
	if (runtime.owner_room != kNowhere)
	{
		return runtime.owner_room;
	}
	if (source.actor && source.actor->in_room != kNowhere)
	{
		return source.actor->in_room;
	}
	if (source.victim && source.victim->in_room != kNowhere)
	{
		return source.victim->in_room;
	}
	return kNowhere;
}

int ConvertLuaValue(LuaRuntimeContext runtime, const sol::object &first)
{
	if (first.get_type() == sol::type::lua_nil)
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

const char *GetLuaContextGlobalField(const std::string &key)
{
	if (key == "tgTrigger")
	{
		return "trigger";
	}
	if (key == "tgOwner")
	{
		return "owner";
	}
	if (key == "tgActor")
	{
		return "actor";
	}
	if (key == "tgVictim")
	{
		return "victim";
	}
	if (key == "tgObject")
	{
		return "object";
	}
	if (key == "tgRoom")
	{
		return "room";
	}
	if (key == "tgCommand")
	{
		return "command";
	}
	if (key == "tgArgument")
	{
		return "argument";
	}
	if (key == "tgSpeech")
	{
		return "speech";
	}
	if (key == "tgDirection")
	{
		return "direction";
	}
	if (key == "tgDamageAmount")
	{
		return "damageAmount";
	}
	if (key == "tgDamageType")
	{
		return "damageType";
	}
	if (key == "tgWhere")
	{
		return "where";
	}
	if (key == "tgTime")
	{
		return "time";
	}
	if (key == "tgTimeDay")
	{
		return "timeDay";
	}
	return nullptr;
}

} // namespace

sol::table BuildLuaContext(sol::state &lua, const LuaTriggerContext &source, LuaRuntimeContext runtime)
{
	sol::table ctx = lua.create_table();
	RefreshLuaContext(lua, ctx, source, runtime);
	return ctx;
}

void RefreshLuaContext(sol::state &lua, sol::table ctx, const LuaTriggerContext &source, LuaRuntimeContext runtime)
{
	sol::table trigger_table = lua.create_table();
	const auto context_room = GetContextRoom(source, runtime);

	trigger_table["vnum"] = GetTriggerVnum(runtime.trigger);
	trigger_table["rnum"] = runtime.trigger ? runtime.trigger->get_rnum() : -1;
	trigger_table["name"] = runtime.trigger ? runtime.trigger->get_name() : "";
	trigger_table["attachType"] = runtime.trigger ? static_cast<int>(runtime.trigger->get_attach_type()) : 0;
	trigger_table["triggerType"] = runtime.trigger ? runtime.trigger->get_trigger_type() : 0;
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
		ctx["owner"] = sol::lua_nil;
	}
	ctx["actor"] = BuildCharView(lua, source.actor, runtime, context_room);
	ctx["victim"] = BuildCharView(lua, source.victim, runtime, context_room);
	ctx["object"] = BuildObjView(lua, source.object, runtime);
	ctx["command"] = source.command;
	ctx["argument"] = source.argument;
	ctx["speech"] = source.speech;
	ctx["direction"] = source.direction;
	ctx["damageAmount"] = source.damage_amount;
	ctx["damageType"] = source.damage_type;
	ctx["where"] = source.where;
	ctx["time"] = source.time;
	ctx["timeDay"] = source.time_day;
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
		ctx["room"] = sol::lua_nil;
	}
}

void InstallLuaContextGlobals(sol::state &lua, sol::environment environment, sol::table ctx)
{
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, ctx](sol::object, const std::string &key) -> sol::object {
		const char *field = GetLuaContextGlobalField(key);
		if (field)
		{
			return ctx[field];
		}
		return lua.globals()[key];
	};
	environment[sol::metatable_key] = metatable;
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
