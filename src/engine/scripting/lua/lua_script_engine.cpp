#include "engine/scripting/lua/lua_script_engine.h"

#include "engine/db/obj_prototypes.h"
#include "engine/db/db.h"
#include "engine/core/comm.h"
#include "engine/entities/char_data.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/logger.h"
#include "utils/random.h"

#if defined(WITH_LUAJIT_PROTOTYPE)
#include <sol/sol.hpp>
#endif

namespace lua_scripting {

#if defined(WITH_LUAJIT_PROTOTYPE)
namespace {

struct LuaCharView {
	CharData *ch = nullptr;
};

struct LuaRoomView {
	RoomRnum room = kNowhere;
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

bool IsValidRoom(const LuaRoomView &view)
{
	return view.room != kNowhere && view.room >= 0 && view.room <= top_of_world && world[view.room];
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

bool SendToChar(const LuaCharView &view, const sol::object &message)
{
	if (!IsValidChar(view) || !view.ch->desc || !message.is<std::string>())
	{
		return false;
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return false;
	}

	SendMsgToChar(text + "\r\n", view.ch);
	return true;
}

int GetRoomVnum(const LuaRoomView &view)
{
	return IsValidRoom(view) ? world[view.room]->vnum : 0;
}

bool EchoToRoom(const LuaRoomView &view, const sol::object &message)
{
	if (!IsValidRoom(view) || !message.is<std::string>())
	{
		return false;
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return false;
	}

	SendMsgToRoom(text.c_str(), view.room, 0);
	return true;
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
		if (key == "send")
		{
			return sol::make_object(lua, sol::as_function([ch](sol::object, sol::object message) {
				return SendToChar(LuaCharView{ch}, message);
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

sol::object BuildRoomView(sol::state &lua, const LuaRoomView &room, bool allow_echo)
{
	if (!IsValidRoom(room))
	{
		return sol::make_object(lua, sol::nil);
	}

	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, room, allow_echo](sol::object, const std::string &key) -> sol::object {
		if (key == "vnum")
		{
			return sol::make_object(lua, GetRoomVnum(room));
		}
		if (allow_echo && key == "echo")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object message) {
				return EchoToRoom(room, message);
			}));
		}

		return sol::make_object(lua, sol::nil);
	};
	metatable[sol::meta_function::new_index] = [](sol::this_state state) {
		return luaL_error(state, "RoomData Lua view is read-only");
	};
	view[sol::metatable_key] = metatable;

	return sol::make_object(lua, view);
}

sol::object BuildRoomView(sol::state &lua, CharData *owner)
{
	if (!owner || owner->in_room == kNowhere)
	{
		return sol::make_object(lua, sol::nil);
	}

	return BuildRoomView(lua, LuaRoomView{owner->in_room}, true);
}

sol::object BuildRoomViewByVnum(sol::state &lua, const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return sol::make_object(lua, sol::nil);
	}

	const auto room = GetRoomRnum(vnum.as<int>());
	return BuildRoomView(lua, LuaRoomView{room}, false);
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

bool MudLog(Trigger *trigger, const sol::object &message)
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
	snprintf(buf, sizeof(buf), "(Lua trigger: %s, VNum: %d) : %s",
		GetTriggerName(trigger), GetTriggerVnum(trigger), text.c_str());
	script_log(buf, LogMode::OFF);
	return true;
}

sol::table BuildMudNamespace(sol::state &lua, Trigger *trigger)
{
	sol::table mud = lua.create_table();
	mud["log"] = [trigger](const sol::object &message) {
		return MudLog(trigger, message);
	};
	mud["random"] = [](const sol::object &limit) {
		return MudRandom(limit);
	};
	mud["room"] = [&lua](const sol::object &vnum) {
		return BuildRoomViewByVnum(lua, vnum);
	};

	sol::table world_table = lua.create_table();
	world_table["cur_obj_count"] = [](const sol::object &vnum) {
		return GetCurrentObjectCount(vnum);
	};
	mud["world"] = world_table;

	return mud;
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
	ctx["room"] = BuildRoomView(lua, source.owner);

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

	lua["mud"] = BuildMudNamespace(lua, trigger);
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
