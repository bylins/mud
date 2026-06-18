#include "engine/scripting/lua/lua_script_engine.h"

#include "engine/db/obj_prototypes.h"
#include "engine/db/db.h"
#include "engine/db/world_characters.h"
#include "engine/core/comm.h"
#include "engine/core/handler.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/logger.h"
#include "utils/random.h"

#if defined(WITH_LUAJIT_PROTOTYPE)
#include <sol/sol.hpp>
#endif

namespace lua_scripting {

#if defined(WITH_LUAJIT_PROTOTYPE)
namespace {

struct LuaRoomView {
	RoomRnum room = kNowhere;
};

struct LuaEntityHandle {
	enum class LuaEntityType {
		Char,
		Obj
	};

	explicit LuaEntityHandle(LuaEntityType entity_type) : type(entity_type) {}

	LuaEntityType type;
	CharData *ch = nullptr;
	object_id_t obj_id = 0;
};

LuaEntityHandle MakeCharHandle(CharData *ch)
{
	LuaEntityHandle handle(LuaEntityHandle::LuaEntityType::Char);
	handle.ch = ch;
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

const char* GetTriggerName(const Trigger *trigger)
{
	if (!trigger)
	{
		return "<null>";
	}

	return GET_TRIG_NAME(trigger);
}

bool IsValidRoom(const LuaRoomView &view)
{
	return view.room != kNowhere && view.room >= 0 && view.room <= top_of_world && world[view.room];
}

CharData *ResolveChar(const LuaEntityHandle &handle)
{
	if (handle.type != LuaEntityHandle::LuaEntityType::Char
		|| !handle.ch
		|| !character_list.has(handle.ch)
		|| handle.ch->purged())
	{
		return nullptr;
	}

	return handle.ch;
}

ObjData *ResolveObj(const LuaEntityHandle &handle)
{
	if (handle.type != LuaEntityHandle::LuaEntityType::Obj || handle.obj_id == 0)
	{
		return nullptr;
	}

	return world_objects.find_by_id(handle.obj_id).get();
}

bool IsValidEntity(const LuaEntityHandle &handle)
{
	return ResolveChar(handle) || ResolveObj(handle);
}

std::string GetCharName(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->get_name() : "";
}

long GetCharUid(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->get_uid() : 0;
}

int GetCharVnum(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !ch->IsNpc())
	{
		return 0;
	}

	const auto rnum = ch->get_rnum();
	if (rnum < 0 || rnum > top_of_mobt)
	{
		return 0;
	}

	return mob_index[rnum].vnum;
}

int GetCharHp(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->get_hit() : 0;
}

int GetCharMaxHp(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->get_max_hit() : 0;
}

int GetCharRoomVnum(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	if (!ch || ch->in_room == kNowhere || ch->in_room < 0 || ch->in_room > top_of_world)
	{
		return 0;
	}

	return world[ch->in_room]->vnum;
}

bool IsCharNpc(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch && ch->IsNpc();
}

int GetCharLevel(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? GetRealLevel(ch) : 0;
}

bool SendToChar(const LuaEntityHandle &handle, const sol::object &message)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !ch->desc || !message.is<std::string>())
	{
		return false;
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return false;
	}

	SendMsgToChar(text + "\r\n", ch);
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

int GetObjVnum(const LuaEntityHandle &handle)
{
	auto *obj = ResolveObj(handle);
	return obj ? obj->get_vnum() : 0;
}

long GetObjId(const LuaEntityHandle &handle)
{
	auto *obj = ResolveObj(handle);
	return obj ? obj->get_id() : 0;
}

std::string GetObjName(const LuaEntityHandle &handle)
{
	auto *obj = ResolveObj(handle);
	return obj ? obj->get_short_description() : "";
}

int GetObjRoomVnum(const LuaEntityHandle &handle)
{
	auto *obj = ResolveObj(handle);
	if (!obj || obj->get_in_room() == kNowhere)
	{
		return 0;
	}

	return world[obj->get_in_room()]->vnum;
}

bool PurgeEntity(const LuaEntityHandle &handle)
{
	if (handle.type == LuaEntityHandle::LuaEntityType::Char)
	{
		auto *ch = ResolveChar(handle);
		if (!ch)
		{
			return false;
		}
		ExtractCharFromWorld(ch, false);
	}
	else
	{
		auto *obj = ResolveObj(handle);
		if (!obj)
		{
			return false;
		}
		ExtractObjFromWorld(obj);
	}

	return true;
}

sol::object BuildCharView(sol::state &lua, CharData *ch)
{
	if (!ch)
	{
		return sol::make_object(lua, sol::nil);
	}

	auto handle = MakeCharHandle(ch);
	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle](sol::object, const std::string &key) -> sol::object {
		if (key == "name")
		{
			return sol::make_object(lua, GetCharName(handle));
		}
		if (key == "uid")
		{
			return sol::make_object(lua, GetCharUid(handle));
		}
		if (key == "vnum")
		{
			return sol::make_object(lua, GetCharVnum(handle));
		}
		if (key == "hp")
		{
			return sol::make_object(lua, GetCharHp(handle));
		}
		if (key == "max_hp")
		{
			return sol::make_object(lua, GetCharMaxHp(handle));
		}
		if (key == "room_vnum")
		{
			return sol::make_object(lua, GetCharRoomVnum(handle));
		}
		if (key == "is_npc")
		{
			return sol::make_object(lua, IsCharNpc(handle));
		}
		if (key == "level")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return GetCharLevel(handle);
			}));
		}
		if (key == "is_valid")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return IsValidEntity(handle);
			}));
		}
		if (key == "send")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object message) {
				return SendToChar(handle, message);
			}));
		}
		if (key == "purge")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return PurgeEntity(handle);
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

sol::object BuildObjView(sol::state &lua, ObjData *obj)
{
	if (!obj)
	{
		return sol::make_object(lua, sol::nil);
	}

	auto handle = MakeObjHandle(obj);
	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle](sol::object, const std::string &key) -> sol::object {
		if (key == "name")
		{
			return sol::make_object(lua, GetObjName(handle));
		}
		if (key == "id")
		{
			return sol::make_object(lua, GetObjId(handle));
		}
		if (key == "vnum")
		{
			return sol::make_object(lua, GetObjVnum(handle));
		}
		if (key == "room_vnum")
		{
			return sol::make_object(lua, GetObjRoomVnum(handle));
		}
		if (key == "is_valid")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return IsValidEntity(handle);
			}));
		}
		if (key == "purge")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return PurgeEntity(handle);
			}));
		}

		return sol::make_object(lua, sol::nil);
	};
	metatable[sol::meta_function::new_index] = [](sol::this_state state) {
		return luaL_error(state, "ObjData Lua view is read-only");
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

RoomRnum GetRoomFromLua(const sol::object &room)
{
	if (room.is<int>())
	{
		return GetRoomRnum(room.as<int>());
	}
	if (!room.is<sol::table>())
	{
		return kNowhere;
	}

	sol::table room_table = room;
	const sol::object vnum = room_table["vnum"];
	return vnum.is<int>() ? GetRoomRnum(vnum.as<int>()) : kNowhere;
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

	PlaceObjToRoom(obj.get(), room_rnum);
	load_otrigger(obj.get());
	CheckObjDecay(obj.get());
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
	mud["load_obj"] = [&lua](const sol::object &vnum, const sol::object &room) {
		return BuildObjView(lua, LoadObjToRoom(vnum, room));
	};
	mud["load_mob"] = [&lua](const sol::object &vnum, const sol::object &room) {
		return BuildCharView(lua, LoadMobToRoom(vnum, room));
	};
	mud["purge"] = [](const sol::object &entity) {
		return MudPurge(entity);
	};

	sol::table world_table = lua.create_table();
	world_table["cur_obj_count"] = [](const sol::object &vnum) {
		return GetCurrentObjectCount(vnum);
	};
	mud["world"] = world_table;

	return mud;
}

void HardenLuaState(sol::state &lua)
{
	lua["io"] = sol::nil;
	lua["os"] = sol::nil;
	lua["debug"] = sol::nil;
	lua["ffi"] = sol::nil;
	lua["jit"] = sol::nil;
	lua["bit"] = sol::nil;
	lua["package"] = sol::nil;
	lua["require"] = sol::nil;
	lua["module"] = sol::nil;
	lua["load"] = sol::nil;
	lua["dofile"] = sol::nil;
	lua["loadfile"] = sol::nil;
	lua["loadstring"] = sol::nil;
	lua["collectgarbage"] = sol::nil;
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
	HardenLuaState(lua);

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
