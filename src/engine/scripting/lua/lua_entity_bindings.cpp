#include "engine/scripting/lua/lua_internal.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/core/comm.h"
#include "engine/core/handler.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/interpreter.h"
#include "gameplay/core/constants.h"
#include "gameplay/mechanics/damage.h"
#include "gameplay/magic/spells.h"
#include "utils/random.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <array>
#include <sstream>

bool mob_script_command_interpreter(CharData *ch, char *argument, Trigger *trig);
void ExtractTrigger(Trigger *trig);

namespace lua_scripting {

namespace {

constexpr int kLuaHandleTypeChar = 1;
constexpr int kLuaHandleTypeObj = 2;

CharData *GetLuaCharFromObject(const sol::object &entity, LuaRuntimeContext runtime)
{
	if (!entity.is<sol::table>())
	{
		return nullptr;
	}

	sol::table entity_table = entity;
	const sol::object handle = runtime.entity_handles[entity_table];
	if (!handle.is<sol::table>())
	{
		return nullptr;
	}

	sol::table handle_table = handle;
	const sol::object type = handle_table["type"];
	const sol::object ptr = handle_table["ptr"];
	if (!type.is<int>() || type.as<int>() != kLuaHandleTypeChar || !ptr.is<void*>())
	{
		return nullptr;
	}

	auto *ch = static_cast<CharData *>(ptr.as<void*>());
	return ResolveChar(MakeCharHandle(ch));
}

void RegisterCharView(sol::state &lua, sol::table entity_handles, sol::table view, CharData *ch)
{
	sol::table handle = lua.create_table();
	handle["type"] = kLuaHandleTypeChar;
	handle["ptr"] = static_cast<void *>(ch);
	entity_handles[view] = handle;
}

void RegisterObjView(sol::state &lua, sol::table entity_handles, sol::table view, ObjData *obj)
{
	sol::table handle = lua.create_table();
	handle["type"] = kLuaHandleTypeObj;
	handle["id"] = obj->get_id();
	entity_handles[view] = handle;
}

bool GetLuaSkillId(const sol::object &value, ESkill &skill_id)
{
	if (!value.is<int>())
	{
		return false;
	}

	const auto raw_id = value.as<int>();
	if (raw_id < to_underlying(ESkill::kFirst) || raw_id > to_underlying(ESkill::kLast))
	{
		return false;
	}

	skill_id = static_cast<ESkill>(raw_id);
	return true;
}

bool GetLuaFeatId(const sol::object &value, EFeat &feat_id)
{
	if (!value.is<int>())
	{
		return false;
	}

	const auto raw_id = value.as<int>();
	if (raw_id <= to_underlying(EFeat::kUndefined) || raw_id > to_underlying(EFeat::kLast))
	{
		return false;
	}

	feat_id = static_cast<EFeat>(raw_id);
	return true;
}

bool GetLuaAffectId(const sol::object &value, EAffect &affect_id)
{
	if (!value.is<std::string>())
	{
		return false;
	}

	const auto affect = ITEM_BY_NAME<EAffect>(value.as<std::string>());
	if (affect == EAffect::kUndefinded)
	{
		return false;
	}

	affect_id = affect;
	return true;
}

bool GetLuaDirection(const sol::object &value, int &dir)
{
	if (value.is<int>())
	{
		dir = value.as<int>();
		return dir >= 0 && dir < EDirection::kMaxDirNum;
	}
	if (!value.is<std::string>())
	{
		return false;
	}

	auto direction = value.as<std::string>();
	dir = search_block(direction.data(), dirs, false);
	return dir >= 0 && dir < EDirection::kMaxDirNum;
}

bool RemoveObjFromCurrentLocation(ObjData *obj)
{
	if (!obj)
	{
		return false;
	}
	if (obj->get_worn_by())
	{
		return UnequipChar(obj->get_worn_by(), obj->get_worn_on(), CharEquipFlags()) == obj;
	}
	if (obj->get_carried_by())
	{
		RemoveObjFromChar(obj);
		return true;
	}
	if (obj->get_in_obj())
	{
		RemoveObjFromObj(obj);
		return true;
	}
	if (obj->get_in_room() != kNowhere)
	{
		RemoveObjFromRoom(obj);
		return true;
	}

	return true;
}

bool AttachTriggerToScript(Script *script, int vnum, int attach_type)
{
	const auto rnum = GetTriggerRnum(vnum);
	if (rnum < 0 || trig_index[rnum]->proto->get_attach_type() != attach_type)
	{
		return false;
	}

	auto *trigger = read_trigger(rnum);
	if (!trigger)
	{
		return false;
	}
	if (!add_trigger(script, trigger, -1))
	{
		ExtractTrigger(trigger);
		return false;
	}

	return true;
}

ObjData *LoadObjToRoom(ObjVnum vnum, RoomRnum room)
{
	if (!IsValidRoom(LuaRoomView{room}))
	{
		return nullptr;
	}

	const auto obj_rnum = GetObjRnum(vnum);
	if (obj_rnum < 0)
	{
		return nullptr;
	}

	const auto obj = world_objects.create_from_prototype_by_rnum(obj_rnum);
	if (!obj)
	{
		return nullptr;
	}

	obj->set_vnum_zone_from(zone_table[world[room]->zone_rn].vnum);
	PlaceObjToRoom(obj.get(), room);
	load_otrigger(obj.get());
	if (CheckObjDecay(obj.get()))
	{
		return nullptr;
	}
	return obj.get();
}

CharData *LoadMobToRoom(MobVnum vnum, RoomRnum room)
{
	if (!IsValidRoom(LuaRoomView{room}))
	{
		return nullptr;
	}

	const auto mob_rnum = GetMobRnum(vnum);
	if (mob_rnum < 0)
	{
		return nullptr;
	}

	auto *mob = ReadMobile(mob_rnum, kReal);
	if (!mob)
	{
		return nullptr;
	}

	PlaceCharToRoom(mob, room);
	load_mtrigger(mob);
	return mob;
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

int GetCharMana(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->mem_queue.stored : 0;
}

int GetCharMove(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->get_move() : 0;
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

bool IsCharPc(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch && !ch->IsNpc();
}

bool IsCharImmortal(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch && ch->IsImmortal();
}

int GetCharLevel(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? GetRealLevel(ch) : 0;
}

int GetCharPosition(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? static_cast<int>(ch->GetPosition()) : 0;
}

bool CharHasAffect(const LuaEntityHandle &handle, const sol::object &affect)
{
	auto *ch = ResolveChar(handle);
	EAffect affect_id = EAffect::kUndefinded;
	return ch && GetLuaAffectId(affect, affect_id) && AFF_FLAGGED(ch, affect_id);
}

bool CharCanSee(LuaRuntimeContext runtime, const LuaEntityHandle &handle, const sol::object &target)
{
	auto *ch = ResolveChar(handle);
	auto *victim = GetLuaCharFromObject(target, runtime);
	return ch && victim && CAN_SEE(ch, victim);
}

CharData *GetCharEnemy(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->GetEnemy() : nullptr;
}

bool TeleportChar(const LuaEntityHandle &handle, const sol::object &room)
{
	auto *ch = ResolveChar(handle);
	const auto room_rnum = GetRoomFromLua(room);
	if (!ch || !IsValidRoom(LuaRoomView{room_rnum}))
	{
		return false;
	}

	if (ch->in_room != kNowhere)
	{
		RemoveCharFromRoom(ch);
	}
	PlaceCharToRoom(ch, room_rnum);
	return true;
}

int CharSkill(const LuaEntityHandle &handle, const sol::object &skill, const sol::object &value)
{
	auto *ch = ResolveChar(handle);
	ESkill skill_id = ESkill::kUndefined;
	if (!ch || !GetLuaSkillId(skill, skill_id))
	{
		return 0;
	}
	if (value.is<int>())
	{
		ch->set_skill(skill_id, value.as<int>());
	}
	return ch->GetSkill(skill_id);
}

bool CharFeat(const LuaEntityHandle &handle, const sol::object &feat, const sol::object &value)
{
	auto *ch = ResolveChar(handle);
	EFeat feat_id = EFeat::kUndefined;
	if (!ch || !GetLuaFeatId(feat, feat_id))
	{
		return false;
	}
	if (value.is<bool>())
	{
		if (value.as<bool>())
		{
			ch->SetFeat(feat_id);
		}
		else
		{
			ch->UnsetFeat(feat_id);
		}
	}
	return ch->HaveFeat(feat_id);
}

bool AttachTriggerToChar(const LuaEntityHandle &handle, const sol::object &vnum)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !ch->IsNpc() || !vnum.is<int>())
	{
		return false;
	}

	const auto result = AttachTriggerToScript(SCRIPT(ch).get(), vnum.as<int>(), MOB_TRIGGER);
	if (result)
	{
		timechange_register_mob(ch);
	}
	return result;
}

bool DetachTriggerFromChar(const LuaEntityHandle &handle, const sol::object &vnum)
{
	auto *ch = ResolveChar(handle);
	return ch && ch->IsNpc() && vnum.is<int>() && SCRIPT(ch)->remove_trigger(vnum.as<int>());
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

bool ActFromChar(
	LuaRuntimeContext runtime,
	const LuaEntityHandle &handle,
	const sol::object &message,
	const sol::object &options)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !message.is<std::string>())
	{
		return false;
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return false;
	}

	auto target = kToRoom;
	CharData *victim = nullptr;
	bool hide_invisible = false;
	if (options.is<sol::table>())
	{
		sol::table table = options;
		const sol::object victim_object = table["victim"];
		victim = GetLuaCharFromObject(victim_object, runtime);
		const sol::object hide_invisible_object = table["hide_invisible"];
		if (hide_invisible_object.is<bool>())
		{
			hide_invisible = hide_invisible_object.as<bool>();
		}
		const sol::object to = table["to"];
		if (to.is<std::string>())
		{
			const auto to_value = to.as<std::string>();
			if (to_value == "char")
			{
				target = kToChar;
			}
			else if (to_value == "victim")
			{
				target = kToVict;
			}
			else if (to_value == "notvictim")
			{
				target = kToNotVict;
			}
			else if (to_value != "room")
			{
				return LogLuaApiError(runtime, "act: invalid target");
			}
		}
	}
	if ((target == kToVict || target == kToNotVict) && !victim)
	{
		return LogLuaApiError(runtime, "act: victim target requires valid victim");
	}

	act(text.c_str(), hide_invisible, ch, nullptr, victim, target);
	return true;
}

int GetRoomVnum(const LuaRoomView &view)
{
	return IsValidRoom(view) ? world[view.room]->vnum : 0;
}

std::string GetRoomName(const LuaRoomView &view)
{
	return IsValidRoom(view) && world[view.room]->name ? world[view.room]->name : "";
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

RoomRnum GetRoomExit(const LuaRoomView &view, const sol::object &direction)
{
	int dir = 0;
	if (!IsValidRoom(view) || !GetLuaDirection(direction, dir) || !world[view.room]->dir_option[dir])
	{
		return kNowhere;
	}

	return world[view.room]->dir_option[dir]->to_room();
}

bool AttachTriggerToRoom(const LuaRoomView &view, const sol::object &vnum)
{
	if (!IsValidRoom(view) || !vnum.is<int>())
	{
		return false;
	}

	const auto result = AttachTriggerToScript(SCRIPT(world[view.room]).get(), vnum.as<int>(), WLD_TRIGGER);
	if (result)
	{
		timechange_register_room(world[view.room]);
	}
	return result;
}

bool DetachTriggerFromRoom(const LuaRoomView &view, const sol::object &vnum)
{
	return IsValidRoom(view) && vnum.is<int>() && SCRIPT(world[view.room])->remove_trigger(vnum.as<int>());
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
	if (!obj || obj->get_in_room() == kNowhere || obj->get_in_room() < 0 || obj->get_in_room() > top_of_world)
	{
		return 0;
	}

	return world[obj->get_in_room()]->vnum;
}

int GetObjTimer(const LuaEntityHandle &handle)
{
	auto *obj = ResolveObj(handle);
	return obj ? obj->get_timer() : 0;
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

bool MoveObjToRoom(const LuaEntityHandle &handle, const sol::object &room)
{
	auto *obj = ResolveObj(handle);
	const auto room_rnum = GetRoomFromLua(room);
	if (!obj || !IsValidRoom(LuaRoomView{room_rnum}) || !RemoveObjFromCurrentLocation(obj))
	{
		return false;
	}

	return PlaceObjToRoom(obj, room_rnum);
}

bool GiveObjToChar(LuaRuntimeContext runtime, const LuaEntityHandle &handle, const sol::object &target)
{
	auto *obj = ResolveObj(handle);
	auto *ch = GetLuaCharFromObject(target, runtime);
	if (!obj || !ch || !RemoveObjFromCurrentLocation(obj))
	{
		return false;
	}

	PlaceObjToInventory(obj, ch);
	return true;
}

bool AttachTriggerToObj(const LuaEntityHandle &handle, const sol::object &vnum)
{
	auto *obj = ResolveObj(handle);
	if (!obj || !vnum.is<int>())
	{
		return false;
	}

	const auto result = AttachTriggerToScript(obj->get_script().get(), vnum.as<int>(), OBJ_TRIGGER);
	if (result)
	{
		timechange_register_obj(obj);
		world_objects.update_obj_indices(obj);
	}
	return result;
}

bool DetachTriggerFromObj(const LuaEntityHandle &handle, const sol::object &vnum)
{
	auto *obj = ResolveObj(handle);
	return obj && vnum.is<int>() && obj->get_script()->remove_trigger(vnum.as<int>());
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
	RegisterCharView(lua, runtime.entity_handles, view, ch);
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
		if (key == "mana")
		{
			return sol::make_object(lua, GetCharMana(handle));
		}
		if (key == "move")
		{
			return sol::make_object(lua, GetCharMove(handle));
		}
		if (key == "room")
		{
			auto *ch = ResolveChar(handle);
			return BuildRoomView(lua, ch ? LuaRoomView{ch->in_room} : LuaRoomView{}, true, runtime);
		}
		if (key == "room_vnum")
		{
			return sol::make_object(lua, GetCharRoomVnum(handle));
		}
		if (key == "is_npc")
		{
			return sol::make_object(lua, IsCharNpc(handle));
		}
		if (key == "is_pc")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return IsCharPc(handle);
			}));
		}
		if (key == "is_immortal")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return IsCharImmortal(handle);
			}));
		}
		if (key == "level")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return GetCharLevel(handle);
			}));
		}
		if (key == "position")
		{
			return sol::make_object(lua, sol::as_function([handle]() {
				return GetCharPosition(handle);
			}));
		}
		if (key == "has_affect")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object affect) {
				return CharHasAffect(handle, affect);
			}));
		}
		if (key == "can_see")
		{
			return sol::make_object(lua, sol::as_function([runtime, handle](sol::object, sol::object target) {
				return CharCanSee(runtime, handle, target);
			}));
		}
		if (key == "enemy")
		{
			return sol::make_object(lua, sol::as_function([&lua, runtime, handle](sol::object) {
				return BuildCharView(lua, GetCharEnemy(handle), runtime);
			}));
		}
		if (key == "teleport")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object room) {
				return TeleportChar(handle, room);
			}));
		}
		if (key == "skill")
		{
			return sol::make_object(lua, sol::as_function([&lua, handle](sol::object, sol::object skill, sol::variadic_args args) {
				const sol::object value = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::nil);
				return CharSkill(handle, skill, value);
			}));
		}
		if (key == "feat")
		{
			return sol::make_object(lua, sol::as_function([&lua, handle](sol::object, sol::object feat, sol::variadic_args args) {
				const sol::object value = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::nil);
				return CharFeat(handle, feat, value);
			}));
		}
		if (key == "attach_trigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return AttachTriggerToChar(handle, vnum);
			}));
		}
		if (key == "detach_trigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return DetachTriggerFromChar(handle, vnum);
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
		if (key == "act")
		{
			return sol::make_object(lua, sol::as_function([&lua, runtime, handle](
				sol::object,
				sol::object message,
				sol::variadic_args args) {
				const sol::object options = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::nil);
				return ActFromChar(runtime, handle, message, options);
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

sol::object BuildObjView(sol::state &lua, ObjData *obj, LuaRuntimeContext runtime)
{
	if (!obj)
	{
		return sol::make_object(lua, sol::nil);
	}

	auto handle = MakeObjHandle(obj);
	sol::table view = lua.create_table();
	RegisterObjView(lua, runtime.entity_handles, view, obj);
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle, runtime](sol::object, const std::string &key) -> sol::object {
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
		if (key == "room")
		{
			auto *obj = ResolveObj(handle);
			return BuildRoomView(lua, obj ? LuaRoomView{obj->get_in_room()} : LuaRoomView{}, true, runtime);
		}
		if (key == "carried_by")
		{
			auto *obj = ResolveObj(handle);
			return BuildCharView(lua, obj ? obj->get_carried_by() : nullptr, runtime);
		}
		if (key == "worn_by")
		{
			auto *obj = ResolveObj(handle);
			return BuildCharView(lua, obj ? obj->get_worn_by() : nullptr, runtime);
		}
		if (key == "container")
		{
			auto *obj = ResolveObj(handle);
			return BuildObjView(lua, obj ? obj->get_in_obj() : nullptr, runtime);
		}
		if (key == "timer")
		{
			return sol::make_object(lua, GetObjTimer(handle));
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
		if (key == "move_to_room")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object room) {
				return MoveObjToRoom(handle, room);
			}));
		}
		if (key == "give_to")
		{
			return sol::make_object(lua, sol::as_function([runtime, handle](sol::object, sol::object ch) {
				return GiveObjToChar(runtime, handle, ch);
			}));
		}
		if (key == "attach_trigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return AttachTriggerToObj(handle, vnum);
			}));
		}
		if (key == "detach_trigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return DetachTriggerFromObj(handle, vnum);
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

sol::object BuildRoomView(sol::state &lua, const LuaRoomView &room, bool allow_echo, LuaRuntimeContext runtime)
{
	if (!IsValidRoom(room))
	{
		return sol::make_object(lua, sol::nil);
	}

	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, room, allow_echo, runtime](sol::object, const std::string &key) -> sol::object {
		if (key == "vnum")
		{
			return sol::make_object(lua, GetRoomVnum(room));
		}
		if (key == "name")
		{
			return sol::make_object(lua, GetRoomName(room));
		}
		if (allow_echo && key == "echo")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object message) {
				return EchoToRoom(room, message);
			}));
		}
		if (key == "people")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object) {
				sol::table result = lua.create_table();
				if (!IsValidRoom(room))
				{
					return result;
				}
				int index = 1;
				for (auto *ch : world[room.room]->people)
				{
					result[index++] = BuildCharView(lua, ch, runtime);
				}
				return result;
			}));
		}
		if (key == "objects")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object) {
				sol::table result = lua.create_table();
				if (!IsValidRoom(room))
				{
					return result;
				}
				int index = 1;
				for (auto *obj : world[room.room]->contents)
				{
					result[index++] = BuildObjView(lua, obj, runtime);
				}
				return result;
			}));
		}
		if (key == "exit")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object, sol::object direction) {
				return BuildRoomView(lua, LuaRoomView{GetRoomExit(room, direction)}, false, runtime);
			}));
		}
		if (key == "load_obj")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object, sol::object vnum) {
				return vnum.is<int>()
					? BuildObjView(lua, LoadObjToRoom(vnum.as<int>(), room.room), runtime)
					: sol::make_object(lua, sol::nil);
			}));
		}
		if (key == "load_mob")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object, sol::object vnum) {
				return vnum.is<int>()
					? BuildCharView(lua, LoadMobToRoom(vnum.as<int>(), room.room), runtime)
					: sol::make_object(lua, sol::nil);
			}));
		}
		if (key == "attach_trigger")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object vnum) {
				return AttachTriggerToRoom(room, vnum);
			}));
		}
		if (key == "detach_trigger")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object vnum) {
				return DetachTriggerFromRoom(room, vnum);
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

sol::object BuildRoomView(sol::state &lua, CharData *owner, LuaRuntimeContext runtime)
{
	if (!owner || owner->in_room == kNowhere)
	{
		return sol::make_object(lua, sol::nil);
	}

	return BuildRoomView(lua, LuaRoomView{owner->in_room}, true, runtime);
}

sol::object BuildRoomViewByVnum(sol::state &lua, const sol::object &vnum, LuaRuntimeContext runtime)
{
	if (!vnum.is<int>())
	{
		return sol::make_object(lua, sol::nil);
	}

	const auto room = GetRoomRnum(vnum.as<int>());
	return BuildRoomView(lua, LuaRoomView{room}, false, runtime);
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
