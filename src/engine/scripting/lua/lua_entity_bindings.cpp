#include "engine/scripting/lua/lua_internal.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/core/comm.h"
#include "engine/core/handler.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/world_characters.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/interpreter.h"
#include "gameplay/mechanics/damage.h"
#include "gameplay/magic/spells.h"
#include "utils/random.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <array>
#include <sstream>

bool mob_script_command_interpreter(CharData *ch, char *argument, Trigger *trig);

namespace lua_scripting {

namespace {

CharData *GetLuaCharFromObject(const sol::object &entity, LuaRuntimeContext runtime)
{
	if (!entity.is<sol::table>())
	{
		return nullptr;
	}

	sol::table entity_table = entity;
	const sol::object handle = runtime.entity_handles[entity_table];
	if (!handle.is<void*>())
	{
		return nullptr;
	}

	auto *ch = static_cast<CharData *>(handle.as<void*>());
	return ResolveChar(MakeCharHandle(ch));
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

bool ForceCharCommand(LuaRuntimeContext runtime, const LuaEntityHandle &handle, const sol::object &command)
{
	auto *ch = ResolveChar(handle);
	if (!ch)
	{
		return LogLuaApiError(runtime, "force: invalid character");
	}
	if (!command.is<std::string>())
	{
		return LogLuaApiError(runtime, "force: command must be a string");
	}

	auto text = command.as<std::string>();
	utils::Trim(text);
	if (text.empty())
	{
		return LogLuaApiError(runtime, "force: empty command");
	}
	if (text.size() >= kMaxInputLength)
	{
		return LogLuaApiError(runtime, "force: command is too long");
	}
	if (!ch->IsNpc() && !ch->desc)
	{
		return LogLuaApiError(runtime, "force: player has no descriptor");
	}
	if (!ch->IsNpc() && GetRealLevel(ch) >= kLvlImmortal)
	{
		return LogLuaApiError(runtime, "force: immortal target");
	}

	std::array<char, kMaxInputLength> command_buffer{};
	std::copy(text.begin(), text.end(), command_buffer.begin());

	if (ch->IsNpc() && mob_script_command_interpreter(ch, command_buffer.data(), runtime.trigger))
	{
		return LogLuaApiError(runtime, "force: mob trigger commands are not allowed");
	}

	command_interpreter(ch, command_buffer.data());
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

bool PurgeCharEntity(const LuaEntityHandle &handle, LuaRuntimeContext runtime)
{
	auto *ch = ResolveChar(handle);
	if (!ch)
	{
		return false;
	}
	if (!ch->IsNpc())
	{
		return LogLuaApiError(runtime, "purge: purging a PC");
	}
	if (ch == runtime.owner)
	{
		return LogLuaApiError(runtime, "purge: purging current trigger owner is not allowed");
	}
	if (!ch->followers.empty() || ch->has_master())
	{
		die_follower(ch);
	}

	character_list.AddToExtractedList(ch);
	return true;
}

bool PurgeObjEntity(const LuaEntityHandle &handle)
{
	auto *obj = ResolveObj(handle);
	if (!obj)
	{
		return false;
	}

	world_objects.AddToExtractedList(obj);
	return true;
}

bool ParseLuaDamageType(const sol::object &type, fight::DmgType &damage_type)
{
	if (type.get_type() == sol::type::nil)
	{
		damage_type = fight::kPureDmg;
		return true;
	}
	if (!type.is<std::string>())
	{
		return false;
	}

	const auto name = type.as<std::string>();
	if (name == "physic")
	{
		damage_type = fight::kPhysDmg;
		return true;
	}
	if (name == "magic")
	{
		damage_type = fight::kMagicDmg;
		return true;
	}
	if (name == "poisonous")
	{
		damage_type = fight::kPoisonDmg;
		return true;
	}

	return false;
}

std::string LuaContextValueToString(const sol::object &value)
{
	if (value.is<bool>())
	{
		return value.as<bool>() ? "1" : "0";
	}
	if (value.is<int>())
	{
		return std::to_string(value.as<int>());
	}
	if (value.get_type() == sol::type::number)
	{
		std::ostringstream out;
		out << value.as<double>();
		return out.str();
	}
	if (value.is<std::string>())
	{
		return value.as<std::string>();
	}

	return "";
}

std::string NormalizeScriptContextKey(std::string key)
{
	utils::ConvertToLow(key);
	return key;
}

std::list<TriggerVar>::const_iterator FindScriptContextVar(
	const std::list<TriggerVar> &var_list,
	const std::string &key,
	long context)
{
	const auto normalized_key = NormalizeScriptContextKey(key);
	return std::find_if(var_list.begin(), var_list.end(), [&normalized_key, context](const TriggerVar &var) {
		return var.name == normalized_key && var.context == context;
	});
}

std::list<TriggerVar>::iterator FindScriptContextVar(
	std::list<TriggerVar> &var_list,
	const std::string &key,
	long context)
{
	const auto normalized_key = NormalizeScriptContextKey(key);
	return std::find_if(var_list.begin(), var_list.end(), [&normalized_key, context](const TriggerVar &var) {
		return var.name == normalized_key && var.context == context;
	});
}

sol::object GetScriptContextValue(
	sol::state &lua,
	const Script *script,
	long context,
	const std::string &key,
	const sol::object &default_value)
{
	if (!script || key.empty())
	{
		return default_value.get_type() == sol::type::none
			? sol::make_object(lua, sol::nil)
			: default_value;
	}

	const auto value = FindScriptContextVar(script->global_vars, key, context);
	if (value == script->global_vars.end())
	{
		return default_value.get_type() == sol::type::none
			? sol::make_object(lua, sol::nil)
			: default_value;
	}

	return sol::make_object(lua, value->value);
}

bool SetScriptContextValue(Script *script, long context, const std::string &key, const sol::object &value)
{
	if (!script || key.empty())
	{
		return false;
	}
	if (!value.is<bool>() && !value.is<int>() && value.get_type() != sol::type::number && !value.is<std::string>())
	{
		return false;
	}

	TriggerVar variable;
	variable.name = NormalizeScriptContextKey(key);
	variable.value = LuaContextValueToString(value);
	variable.context = context;

	const auto existing = FindScriptContextVar(script->global_vars, key, context);
	if (existing != script->global_vars.end())
	{
		*existing = variable;
	}
	else
	{
		script->global_vars.push_back(variable);
	}
	return true;
}

bool DeleteScriptContextValue(Script *script, long context, const std::string &key)
{
	return script && !key.empty() && remove_var_cntx(script->global_vars, key, context);
}

} // namespace

bool MudDamage(
	LuaRuntimeContext runtime,
	const sol::object &victim_object,
	const sol::object &amount_object,
	const sol::object &type_object)
{
	auto *victim = GetLuaCharFromObject(victim_object, runtime);
	if (!victim)
	{
		return LogLuaApiError(runtime, "damage: victim must be a character");
	}
	if (!amount_object.is<int>())
	{
		return LogLuaApiError(runtime, "damage: amount must be an integer");
	}

	const auto amount = amount_object.as<int>();
	if (amount <= 0 || amount > kLuaMaxTriggerDamage)
	{
		return LogLuaApiError(runtime, "damage: amount is out of range");
	}

	fight::DmgType damage_type = fight::kPureDmg;
	if (!ParseLuaDamageType(type_object, damage_type))
	{
		return LogLuaApiError(runtime, "damage: incorrect damage type");
	}

	auto *attacker = runtime.owner;
	if (!attacker || !ResolveChar(MakeCharHandle(attacker)))
	{
		attacker = victim;
	}
	if (victim->in_room == kNowhere || attacker->in_room == kNowhere)
	{
		return LogLuaApiError(runtime, "damage: character is not in room");
	}
	if (attacker->in_room != victim->in_room)
	{
		return LogLuaApiError(runtime, "damage: attacker and victim must be in the same room");
	}
	if (victim->IsImmortal() && amount > 0)
	{
		return LogLuaApiError(runtime, "damage: immortal victim");
	}

	Damage damage(SimpleDmg(kTypeTriggerdeath), amount, damage_type);
	return damage.Process(attacker, victim) != 0;
}

sol::object BuildScriptContextView(sol::state &lua, Script *script, long context)
{
	if (!script)
	{
		return sol::make_object(lua, sol::nil);
	}

	// DG owner context lifetime follows Script::global_vars: it survives mud.wait
	// and trigger reruns for the same owner/context, and survives reboot only when
	// the backing owner/script storage is saved by the world layer. Variable names
	// are DG-style case-insensitive; values keep the Lua-written string case.
	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, script, context](sol::object, const std::string &key) -> sol::object {
		if (key == "get")
		{
			return sol::make_object(lua, sol::as_function([&lua, script, context](
				sol::object,
				const std::string &name,
				sol::variadic_args args) {
				const sol::object default_value = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::nil);
				return GetScriptContextValue(lua, script, context, name, default_value);
			}));
		}
		if (key == "delete")
		{
			return sol::make_object(lua, sol::as_function([script, context](sol::object, const std::string &name) {
				return DeleteScriptContextValue(script, context, name);
			}));
		}

		return GetScriptContextValue(lua, script, context, key, sol::make_object(lua, sol::nil));
	};
	metatable[sol::meta_function::new_index] = [script, context](
		sol::this_state state,
		sol::object,
		const std::string &key,
		sol::object value) {
		if (!SetScriptContextValue(script, context, key, value))
		{
			return luaL_error(state, "ctx.owner.context supports only string, number and boolean values");
		}
		return 0;
	};
	view[sol::metatable_key] = metatable;

	return sol::make_object(lua, view);
}

sol::object BuildCharView(sol::state &lua, CharData *ch, LuaRuntimeContext runtime)
{
	if (!ch)
	{
		return sol::make_object(lua, sol::nil);
	}

	auto handle = MakeCharHandle(ch);
	sol::table view = lua.create_table();
	runtime.entity_handles[view] = static_cast<void *>(ch);
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle, runtime](sol::object, const std::string &key) -> sol::object {
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
		if (key == "context")
		{
			auto *ch = ResolveChar(handle);
			const auto context = runtime.trigger ? runtime.trigger->context : 0;
			return BuildScriptContextView(lua, ch ? SCRIPT(ch).get() : nullptr, context);
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
		if (key == "force")
		{
			return sol::make_object(lua, sol::as_function([runtime, handle](sol::object, sol::object command) {
				return ForceCharCommand(runtime, handle, command);
			}));
		}
		if (key == "purge")
		{
			return sol::make_object(lua, sol::as_function([handle, runtime]() {
				return PurgeCharEntity(handle, runtime);
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
				return PurgeObjEntity(handle);
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

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
