#include "engine/scripting/lua/lua_script_engine.h"

#include "engine/db/db.h"
#include "engine/entities/char_data.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/logger.h"

#if defined(WITH_LUAJIT_PROTOTYPE)
#include <sol/sol.hpp>
#endif

namespace lua_scripting {

#if defined(WITH_LUAJIT_PROTOTYPE)
namespace {

struct LuaCharView {
	CharData *ch = nullptr;
};

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

bool IsValidChar(const LuaCharView &view)
{
	return view.ch && !view.ch->purged();
}

std::string GetCharName(const LuaCharView &view)
{
	return IsValidChar(view) ? view.ch->get_name() : "";
}

long GetCharUid(const LuaCharView &view)
{
	return IsValidChar(view) ? view.ch->get_uid() : 0;
}

int GetCharVnum(const LuaCharView &view)
{
	if (!IsValidChar(view) || !view.ch->IsNpc())
	{
		return 0;
	}

	const auto rnum = view.ch->get_rnum();
	if (rnum < 0 || rnum > top_of_mobt)
	{
		return 0;
	}

	return mob_index[rnum].vnum;
}

int GetCharHp(const LuaCharView &view)
{
	return IsValidChar(view) ? view.ch->get_hit() : 0;
}

int GetCharMaxHp(const LuaCharView &view)
{
	return IsValidChar(view) ? view.ch->get_max_hit() : 0;
}

int GetCharRoomVnum(const LuaCharView &view)
{
	if (!IsValidChar(view) || view.ch->in_room == kNowhere || view.ch->in_room < 0 || view.ch->in_room > top_of_world)
	{
		return 0;
	}

	return world[view.ch->in_room]->vnum;
}

bool IsCharNpc(const LuaCharView &view)
{
	return IsValidChar(view) && view.ch->IsNpc();
}

int GetCharLevel(const LuaCharView &view)
{
	return IsValidChar(view) ? GetRealLevel(view.ch) : 0;
}

sol::object BuildCharView(sol::state &lua, CharData *ch)
{
	if (!ch)
	{
		return sol::make_object(lua, sol::nil);
	}

	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, ch](sol::object, const std::string &key) -> sol::object {
		LuaCharView view{ch};
		if (key == "name")
		{
			return sol::make_object(lua, GetCharName(view));
		}
		if (key == "uid")
		{
			return sol::make_object(lua, GetCharUid(view));
		}
		if (key == "vnum")
		{
			return sol::make_object(lua, GetCharVnum(view));
		}
		if (key == "hp")
		{
			return sol::make_object(lua, GetCharHp(view));
		}
		if (key == "max_hp")
		{
			return sol::make_object(lua, GetCharMaxHp(view));
		}
		if (key == "room_vnum")
		{
			return sol::make_object(lua, GetCharRoomVnum(view));
		}
		if (key == "is_npc")
		{
			return sol::make_object(lua, IsCharNpc(view));
		}
		if (key == "level")
		{
			return sol::make_object(lua, sol::as_function([ch](sol::object) {
				return GetCharLevel(LuaCharView{ch});
			}));
		}
		if (key == "is_valid")
		{
			return sol::make_object(lua, sol::as_function([ch](sol::object) {
				return IsValidChar(LuaCharView{ch});
			}));
		}

		return sol::make_object(lua, sol::nil);
	};
	metatable[sol::meta_function::new_index] = [](sol::this_state state) {
		return luaL_error(state, "CharData Lua view is read-only");
	};
	view[sol::metatable_key] = metatable;

	return sol::make_object(lua, view);
}

void LogLuaError(const Trigger *trigger, const sol::error &err)
{
	char buf[kMaxStringLength];
	snprintf(buf, sizeof(buf), "ERROR: Lua trigger failed: name='%s', vnum=%d: %s",
		GetTriggerName(trigger), GetTriggerVnum(trigger), err.what());
	mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
}

sol::table BuildLuaContext(sol::state &lua, Trigger *trigger, const LuaTriggerContext &source)
{
	sol::table ctx = lua.create_table();
	sol::table trigger_table = lua.create_table();

	trigger_table["vnum"] = GetTriggerVnum(trigger);
	trigger_table["rnum"] = trigger ? trigger->get_rnum() : -1;
	trigger_table["name"] = trigger ? trigger->get_name() : "";
	trigger_table["attach_type"] = trigger ? static_cast<int>(trigger->get_attach_type()) : 0;
	trigger_table["trigger_type"] = trigger ? trigger->get_trigger_type() : 0;
	ctx["trigger"] = trigger_table;
	ctx["owner"] = BuildCharView(lua, source.owner);
	ctx["actor"] = BuildCharView(lua, source.actor);

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

int LuaScriptEngine::RunTrigger(Trigger *trigger, const LuaTriggerContext &ctx)
{
	if (!trigger)
	{
		return 1;
	}

#if defined(WITH_LUAJIT_PROTOTYPE)
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

	sol::table lua_ctx = BuildLuaContext(lua, trigger, ctx);
	const auto result = lua.safe_script(trigger->get_lua_script_source(), sol::script_pass_on_error);
	return ConvertLuaResult(result, trigger, lua_ctx, true);
#else
	(void) ctx;
	return 1;
#endif
}

} // namespace lua_scripting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
