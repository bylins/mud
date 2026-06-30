#include "engine/scripting/lua/lua_internal.h"
#include "engine/core/comm.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/entities/char_data.h"
#include "engine/scripting/dg_scripts.h"
#include "gameplay/core/constants.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/fight/pk.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/weather.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/random.h"

#include <chrono>
#include <ctime>
#include <cmath>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <sstream>

namespace lua_scripting {
namespace {

constexpr long kLuaPulsesPerMudHour = kSecsPerMudHour * kPassesPerSec;
constexpr long kLuaMaxWaitPulses = std::numeric_limits<int>::max();
constexpr int kLuaMaxRollDice = 1000;
constexpr int kLuaMaxRollSides = 1000000;

using LuaWorldVarKey = std::pair<long, std::string>;

std::map<LuaWorldVarKey, std::string> &LuaWorldVars()
{
	static std::map<LuaWorldVarKey, std::string> vars;
	return vars;
}

bool IsValidWaitTime(long hr, long min)
{
	return hr >= 0 && hr < 24 && min >= 0 && min < 60;
}

bool IsValidWaitDelay(long time)
{
	return time > 0 && time <= kLuaMaxWaitPulses;
}

bool GetLuaLong(const sol::object &value, long &result)
{
	if (value.is<long>())
	{
		result = value.as<long>();
		return true;
	}
	if (value.is<int>())
	{
		result = value.as<int>();
		return true;
	}
	if (value.get_type() == sol::type::number)
	{
		const auto numeric_value = value.as<double>();
		if (!std::isfinite(numeric_value)
			|| std::floor(numeric_value) != numeric_value
			|| numeric_value < static_cast<double>(std::numeric_limits<long>::min())
			|| numeric_value > static_cast<double>(std::numeric_limits<long>::max()))
		{
			return false;
		}
		result = static_cast<long>(numeric_value);
		return true;
	}
	return false;
}

std::string NormalizeLuaWorldVarName(std::string name)
{
	utils::ConvertToLow(name);
	return name;
}

bool GetLuaWorldVarKey(const sol::object &context, const sol::object &name, LuaWorldVarKey &key)
{
	long context_id = 0;
	if (!GetLuaLong(context, context_id) || !name.is<std::string>())
	{
		return false;
	}

	auto var_name = NormalizeLuaWorldVarName(name.as<std::string>());
	if (var_name.empty())
	{
		return false;
	}

	key = {context_id, std::move(var_name)};
	return true;
}

std::string LuaWorldVarValueToString(const sol::object &value)
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
		auto result = value.as<std::string>();
		utils::ConvertToLow(result);
		return result;
	}

	return "";
}

bool SetLuaWorldVar(const sol::object &context, const sol::object &name, const sol::object &value)
{
	if (!value.is<bool>() && !value.is<int>() && value.get_type() != sol::type::number && !value.is<std::string>())
	{
		return false;
	}

	LuaWorldVarKey key;
	if (!GetLuaWorldVarKey(context, name, key))
	{
		return false;
	}

	LuaWorldVars()[key] = LuaWorldVarValueToString(value);
	return true;
}

sol::object GetLuaWorldVar(
	sol::state &lua,
	const sol::object &context,
	const sol::object &name,
	const sol::object &default_value)
{
	LuaWorldVarKey key;
	if (!GetLuaWorldVarKey(context, name, key))
	{
		return sol::make_object(lua, sol::lua_nil);
	}

	auto &vars = LuaWorldVars();
	const auto it = vars.find(key);
	if (it == vars.end())
	{
		return default_value.get_type() == sol::type::none
			? sol::make_object(lua, sol::lua_nil)
			: default_value;
	}

	return sol::make_object(lua, it->second);
}

bool DeleteLuaWorldVar(const sol::object &context, const sol::object &name)
{
	LuaWorldVarKey key;
	return GetLuaWorldVarKey(context, name, key) && LuaWorldVars().erase(key) > 0;
}

bool ClearLuaWorldContext(const sol::object &context)
{
	long context_id = 0;
	if (!GetLuaLong(context, context_id) || context_id == 0)
	{
		return false;
	}

	auto &vars = LuaWorldVars();
	std::erase_if(vars, [context_id](const auto &entry) {
		return entry.first.first == context_id;
	});
	return true;
}

CharData *FindCharByUid(const sol::object &uid)
{
	long value = 0;
	if (!GetLuaLong(uid, value))
	{
		return nullptr;
	}

	for (const auto &character : character_list.get_list())
	{
		if (character && character->get_uid() == value && !character->purged())
		{
			return character.get();
		}
	}
	return nullptr;
}

ObjData *FindObjById(const sol::object &id)
{
	long value = 0;
	if (!GetLuaLong(id, value) || value <= 0)
	{
		return nullptr;
	}

	const auto obj = world_objects.find_by_id(static_cast<object_id_t>(value));
	return obj && !obj->get_extracted_list() ? obj.get() : nullptr;
}

ObjData *GetObjArgument(const sol::object &object)
{
	if (!object.is<sol::table>())
	{
		return nullptr;
	}

	sol::table object_table = object;
	const sol::object id = object_table["id"];
	return FindObjById(id);
}

int CountWorldObjectsByRnum(ObjRnum rnum)
{
	std::list<ObjData *> objs;
	world_objects.GetObjListByRnum(rnum, objs);
	return static_cast<int>(objs.size());
}

ObjData *FindObjByVnum(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return nullptr;
	}

	const auto obj = world_objects.find_first_by_vnum(vnum.as<int>());
	return obj && !obj->get_extracted_list() ? obj.get() : nullptr;
}

CharData *FindMobByVnum(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return nullptr;
	}

	Characters::list_t mobs;
	character_list.get_mobs_by_vnum(vnum.as<int>(), mobs);
	for (const auto &mob : mobs)
	{
		if (mob && !mob->purged())
		{
			return mob.get();
		}
	}
	return nullptr;
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

	obj->set_vnum_zone_from(zone_table[world[room_rnum]->zone_rn].vnum);
	PlaceObjToRoom(obj.get(), room_rnum);
	load_otrigger(obj.get());
	if (CheckObjDecay(obj.get()))
	{
		return nullptr;
	}
	return obj.get();
}

CharData *GetCharArgument(const sol::object &character)
{
	if (!character.is<sol::table>())
	{
		return nullptr;
	}

	sol::table character_table = character;
	const sol::object uid = character_table["uid"];
	return FindCharByUid(uid);
}

CharData *FindObjectCarrier(ObjData *obj, int depth = 0)
{
	if (!obj || depth > 10)
	{
		return nullptr;
	}
	if (obj->get_carried_by())
	{
		return obj->get_carried_by();
	}
	if (obj->get_worn_by())
	{
		return obj->get_worn_by();
	}
	return FindObjectCarrier(obj->get_in_obj(), depth + 1);
}

CharData *CreateLuaSpellProxyCaster(LuaRuntimeContext runtime)
{
	if (runtime.owner_room == kNowhere)
	{
		return nullptr;
	}

	auto *caster = ReadMobile(kDgCasterProxy, kVirtual);
	if (!caster)
	{
		return nullptr;
	}

	PlaceCharToRoom(caster, runtime.owner_room);
	return caster;
}

CharData *ResolveLuaSpellCaster(LuaRuntimeContext runtime, bool &temporary)
{
	temporary = false;
	if (runtime.owner && runtime.owner->in_room != kNowhere)
	{
		return runtime.owner;
	}

	if (runtime.owner_obj)
	{
		if (auto *carrier = FindObjectCarrier(runtime.owner_obj))
		{
			if (carrier->in_room != kNowhere)
			{
				return carrier;
			}
		}
	}

	temporary = true;
	return CreateLuaSpellProxyCaster(runtime);
}

bool ValidateLuaSpellCharTarget(
	LuaRuntimeContext runtime,
	ESpell spell_id,
	CharData *caster,
	CharData *target_char)
{
	if (!target_char || target_char->purged() || target_char->in_room == kNowhere)
	{
		return LogLuaApiError(runtime, "cast_spell: character target is not in world");
	}

	if (!MUD::Spell(spell_id).AllowTarget(kTarCharRoom | kTarCharWorld))
	{
		return LogLuaApiError(runtime, "cast_spell: spell does not accept character target");
	}

	const bool room_target = MUD::Spell(spell_id).AllowTarget(kTarCharRoom) && target_char->in_room == caster->in_room;
	const bool world_target = MUD::Spell(spell_id).AllowTarget(kTarCharWorld)
		&& (caster->IsNpc() || !target_char->IsNpc() || target_char->in_room == caster->in_room);
	if (!room_target && !world_target)
	{
		return LogLuaApiError(runtime, "cast_spell: character target is outside allowed scope");
	}

	if (!sight::CanSee(caster, target_char))
	{
		return LogLuaApiError(runtime, "cast_spell: character target is not visible to caster");
	}

	if (MUD::Spell(spell_id).IsViolentAgainst(caster, target_char) && !check_pkill(caster, target_char, target_char->get_name()))
	{
		return false;
	}

	return true;
}

bool ValidateLuaSpellObjTarget(
	LuaRuntimeContext runtime,
	ESpell spell_id,
	CharData *caster,
	ObjData *target_obj)
{
	if (!target_obj || target_obj->get_extracted_list())
	{
		return LogLuaApiError(runtime, "cast_spell: object target is not in world");
	}

	if (!MUD::Spell(spell_id).AllowTarget(kTarObjInv | kTarObjEquip | kTarObjRoom | kTarObjWorld))
	{
		return LogLuaApiError(runtime, "cast_spell: spell does not accept object target");
	}
	if (spell_id == ESpell::kLocateObject)
	{
		return LogLuaApiError(runtime, "cast_spell: locate object requires string target");
	}

	const bool inventory_target = MUD::Spell(spell_id).AllowTarget(kTarObjInv) && target_obj->get_carried_by() == caster;
	const bool equipment_target = MUD::Spell(spell_id).AllowTarget(kTarObjEquip) && target_obj->get_worn_by() == caster;
	const bool room_target = MUD::Spell(spell_id).AllowTarget(kTarObjRoom) && target_obj->get_in_room() == caster->in_room;
	if (!inventory_target && !equipment_target && !room_target)
	{
		return LogLuaApiError(runtime, "cast_spell: object target is outside allowed scope");
	}

	if (!sight::CanSeeObj(caster, target_obj))
	{
		return LogLuaApiError(runtime, "cast_spell: object target is not visible to caster");
	}

	return true;
}

bool ResolveLuaSpellTarget(
	LuaRuntimeContext runtime,
	ESpell spell_id,
	CharData *caster,
	const sol::object &target,
	CharData **tch,
	ObjData **tobj,
	RoomData **troom)
{
	*tch = nullptr;
	*tobj = nullptr;
	*troom = caster && caster->in_room != kNowhere ? world[caster->in_room] : nullptr;
	if (!caster || caster->in_room == kNowhere)
	{
		return LogLuaApiError(runtime, "cast_spell: caster is not in room");
	}

	if (target.get_type() == sol::type::lua_nil)
	{
		return FindCastTarget(spell_id, "", caster, tch, tobj, troom);
	}
	if (target.is<std::string>())
	{
		const auto target_name = target.as<std::string>();
		return FindCastTarget(spell_id, target_name.c_str(), caster, tch, tobj, troom);
	}
	if (!target.is<sol::table>())
	{
		return LogLuaApiError(runtime, "cast_spell: target must be nil, string, Char, Obj, or Room");
	}

	if (auto *target_char = GetCharArgument(target))
	{
		if (!ValidateLuaSpellCharTarget(runtime, spell_id, caster, target_char))
		{
			return false;
		}
		*tch = target_char;
		return true;
	}

	if (auto *target_obj = GetObjArgument(target))
	{
		if (!ValidateLuaSpellObjTarget(runtime, spell_id, caster, target_obj))
		{
			return false;
		}
		*tobj = target_obj;
		return true;
	}

	const auto room_rnum = GetRoomFromLua(target);
	if (room_rnum != kNowhere)
	{
		if (!MUD::Spell(spell_id).AllowTarget(kTarRoomThis))
		{
			return LogLuaApiError(runtime, "cast_spell: spell does not accept room target");
		}
		if (room_rnum != caster->in_room)
		{
			return LogLuaApiError(runtime, "cast_spell: room target must be caster room");
		}
		*troom = world[room_rnum];
		return true;
	}

	return LogLuaApiError(runtime, "cast_spell: invalid target");
}

bool MudCastSpell(LuaRuntimeContext runtime, const sol::object &spell_name, const sol::object &target)
{
	if (!spell_name.is<std::string>())
	{
		return LogLuaApiError(runtime, "cast_spell: spell name must be a string");
	}

	auto spell_name_text = spell_name.as<std::string>();
	const auto spell_id = FixNameAndFindSpellId(spell_name_text);
	if (spell_id == ESpell::kUndefined)
	{
		return LogLuaApiError(runtime, "cast_spell: unknown spell");
	}

	bool temporary_caster = false;
	auto *caster = ResolveLuaSpellCaster(runtime, temporary_caster);
	if (!caster)
	{
		return LogLuaApiError(runtime, "cast_spell: unable to resolve caster");
	}

	CharData *tch = nullptr;
	ObjData *tobj = nullptr;
	RoomData *troom = nullptr;
	const bool target_found = ResolveLuaSpellTarget(runtime, spell_id, caster, target, &tch, &tobj, &troom);
	const auto result = target_found
		? CallMagic(caster, tch, tobj, troom, spell_id, GetRealLevel(caster))
		: ECastResult::kNotCast;

	if (temporary_caster)
	{
		ExtractCharFromWorld(caster, false);
	}
	return CastTookEffect(result);
}

ObjData *LoadObjToChar(const sol::object &vnum, const sol::object &character)
{
	auto *ch = GetCharArgument(character);
	if (!vnum.is<int>() || !ch || !IsValidRoom(LuaRoomView{ch->in_room}))
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

	obj->set_vnum_zone_from(zone_table[world[ch->in_room]->zone_rn].vnum);
	PlaceObjToInventory(obj.get(), ch);
	load_otrigger(obj.get());
	if (CheckObjDecay(obj.get()))
	{
		return nullptr;
	}
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

int GetWorldCurrentObjectCount(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return 0;
	}

	const auto rnum = GetObjRnum(vnum.as<int>());
	if (rnum <= 0)
	{
		return 0;
	}

	if (stable_objs::IsTimerUnlimited(obj_proto[rnum].get()))
	{
		return 0;
	}
	return obj_proto.actual_count(rnum);
}

int GetWorldGameObjectCount(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return 0;
	}

	const auto rnum = GetObjRnum(vnum.as<int>());
	if (rnum <= 0)
	{
		return 0;
	}

	const ObjRnum parent_rnum = obj_proto[rnum]->get_parent_rnum();
	if (parent_rnum > -1
		&& CAN_WEAR(obj_proto[rnum].get(), EWearFlag::kTake)
		&& !obj_proto[rnum]->has_flag(EObjFlag::kQuestItem))
	{
		return CountWorldObjectsByRnum(parent_rnum);
	}
	return CountWorldObjectsByRnum(rnum);
}

int GetWorldMaxObjectCount(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return 0;
	}

	const auto rnum = GetObjRnum(vnum.as<int>());
	if (rnum < 0)
	{
		return 0;
	}

	if (stable_objs::IsTimerUnlimited(obj_proto[rnum].get()) || GetObjMIW(rnum) < 0)
	{
		return 9999999;
	}
	return GetObjMIW(rnum);
}

int GetCurrentMobCount(const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return 0;
	}

	const auto rnum = GetMobRnum(vnum.as<int>());
	if (rnum < 0 || rnum > top_of_mobt)
	{
		return 0;
	}

	return mob_index[rnum].total_online;
}

sol::object BuildZoneView(sol::state &lua, const sol::object &vnum)
{
	if (!vnum.is<int>())
	{
		return sol::make_object(lua, sol::lua_nil);
	}

	const auto rnum = GetZoneRnum(vnum.as<int>());
	if (rnum < 0 || rnum >= static_cast<ZoneRnum>(zone_table.size()))
	{
		return sol::make_object(lua, sol::lua_nil);
	}

	sol::table zone = lua.create_table();
	zone["vnum"] = zone_table[rnum].vnum;
	zone["name"] = zone_table[rnum].name;
	zone["top"] = zone_table[rnum].top;
	zone["age"] = zone_table[rnum].age;
	zone["lifespan"] = zone_table[rnum].lifespan;
	zone["resetMode"] = zone_table[rnum].reset_mode;
	zone["used"] = zone_table[rnum].used;
	zone["locked"] = zone_table[rnum].locked;
	zone["underConstruction"] = zone_table[rnum].under_construction;
	return sol::make_object(lua, zone);
}

sol::table BuildTimeInfo(sol::state &lua)
{
	sol::table time = lua.create_table();
	time["hour"] = time_info.hours;
	time["day"] = time_info.day + 1;
	time["month"] = time_info.month + 1;
	time["year"] = time_info.year;
	return time;
}

sol::table BuildWeatherInfo(sol::state &lua)
{
	sol::table weather = lua.create_table();
	weather["temperature"] = weather_info.temperature;
	weather["pressure"] = weather_info.pressure;
	weather["change"] = weather_info.change;
	weather["sky"] = weather_info.sky;
	weather["sunlight"] = weather_info.sunlight;
	weather["moonDay"] = weather_info.moon_day;
	weather["season"] = static_cast<int>(weather_info.season);
	weather["weatherType"] = weather_info.weather_type;
	weather["rainlevel"] = weather_info.rainlevel;
	weather["snowlevel"] = weather_info.snowlevel;
	weather["icelevel"] = weather_info.icelevel;
	return weather;
}

sol::object BuildRealDate(sol::state &lua, const sol::object &format, const sol::object &ts)
{
	std::time_t t = std::time(nullptr);
	bool has_ts = false;
	long double exact_ts_seconds = 0.0L;
	if (ts.is<long>()) {
		t = ts.as<long>();
		exact_ts_seconds = static_cast<long double>(t);
		has_ts = true;
	} else if (ts.is<int>()) {
		t = static_cast<std::time_t>(ts.as<int>());
		exact_ts_seconds = static_cast<long double>(t);
		has_ts = true;
	} else if (ts.is<double>()) {
		t = static_cast<std::time_t>(ts.as<double>());
		exact_ts_seconds = static_cast<long double>(ts.as<double>());
		has_ts = true;
	}

	if (format.is<std::string>()) {
		const std::string fmt = format.as<std::string>();
		if (fmt == "exact") {
			if (has_ts) {
				return sol::make_object(lua, std::to_string(static_cast<long long>(exact_ts_seconds * 1000.0L)));
			}
			const auto now = std::chrono::system_clock::now();
			const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
			return sol::make_object(lua, std::to_string(now_ms.time_since_epoch().count()));
		}
		if (fmt == "*t" || fmt == "!*t") {
			const bool utc = (fmt[0] == '!');
			struct tm *tm = utc ? std::gmtime(&t) : std::localtime(&t);
			if (!tm) {
				return sol::make_object(lua, sol::lua_nil);
			}
			sol::table tbl = lua.create_table();
			tbl["year"] = tm->tm_year + 1900;
			tbl["month"] = tm->tm_mon + 1;
			tbl["day"] = tm->tm_mday;
			tbl["hour"] = tm->tm_hour;
			tbl["min"] = tm->tm_min;
			tbl["sec"] = tm->tm_sec;
			tbl["wday"] = tm->tm_wday + 1;
			tbl["yday"] = tm->tm_yday + 1;
			tbl["isdst"] = tm->tm_isdst != 0;
			return tbl;
		}
		char buf[256] = {};
		struct tm *tm = std::localtime(&t);
		if (tm && std::strftime(buf, sizeof(buf), fmt.c_str(), tm) != 0) {
			return sol::make_object(lua, std::string(buf));
		}
		return sol::make_object(lua, std::string(""));
	}

	// default
	char buf[256] = {};
	struct tm *tm = std::localtime(&t);
	if (tm && std::strftime(buf, sizeof(buf), "%c", tm) != 0) {
		return sol::make_object(lua, std::string(buf));
	}
	return sol::make_object(lua, std::string(""));
}

int MudRandom(const sol::object &first, const sol::object &second)
{
	if (!first.is<int>())
	{
		return 0;
	}

	const auto m = first.as<int>();
	if (second == sol::lua_nil || second.get_type() == sol::type::none)
	{
		return m > 0 ? number(1, m) : 0;
	}

	if (!second.is<int>())
	{
		return 0;
	}

	const auto n = second.as<int>();
	return number(m, n);
}

int MudRoll(const sol::object &count, const sol::object &sides)
{
	if (!count.is<int>() || !sides.is<int>())
	{
		return 0;
	}

	const auto dice_count = count.as<int>();
	const auto dice_sides = sides.as<int>();
	if (dice_count <= 0
		|| dice_count > kLuaMaxRollDice
		|| dice_sides <= 0
		|| dice_sides > kLuaMaxRollSides)
	{
		return 0;
	}

	long long result = 0;
	for (int i = 0; i < dice_count; ++i)
	{
		result += number(1, dice_sides);
	}
	return result > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(result);
}

bool MudPercent(const sol::object &chance)
{
	if (!chance.is<int>())
	{
		return false;
	}

	const auto value = chance.as<int>();
	if (value <= 0)
	{
		return false;
	}
	if (value >= 100)
	{
		return true;
	}
	return number(1, 100) <= value;
}

RoomRnum GetRuntimeOwnerRoom(LuaRuntimeContext runtime)
{
	return runtime.owner_room;
}

bool MudEcho(LuaRuntimeContext runtime, const sol::object &message)
{
	const auto owner_room = GetRuntimeOwnerRoom(runtime);
	if (owner_room == kNowhere || !message.is<std::string>())
	{
		return false;
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return false;
	}

	SendMsgToRoom(text.c_str(), owner_room, 0);
	return true;
}

bool CanReceiveLuaEchoAround(CharData *ch)
{
	return ch && ch->desc && AWAKE(ch) && (ch->IsNpc() || !ch->IsFlagged(EPlrFlag::kWriting));
}

bool MudEchoAround(LuaRuntimeContext runtime, const sol::object &actor, const sol::object &message)
{
	auto *ch = GetCharArgument(actor);
	if (!ch || ch->in_room == kNowhere)
	{
		return LogLuaApiError(runtime, "echoaround: invalid actor");
	}
	if (!message.is<std::string>())
	{
		return LogLuaApiError(runtime, "echoaround: message must be a string");
	}

	const auto text = message.as<std::string>();
	if (text.empty())
	{
		return LogLuaApiError(runtime, "echoaround: empty message");
	}
	if (text.size() >= kMaxInputLength)
	{
		return LogLuaApiError(runtime, "echoaround: message is too long");
	}

	const auto output = text + "\n\r";
	for (auto *to : world[ch->in_room]->people)
	{
		if (to != ch && CanReceiveLuaEchoAround(to))
		{
			SendMsgToChar(output, to);
		}
	}
	return true;
}

bool CallEntityMethod(
	LuaRuntimeContext runtime,
	const sol::object &entity,
	const char *method_name,
	const sol::object &argument)
{
	if (!entity.is<sol::table>())
	{
		return false;
	}

	sol::table entity_table = entity;
	const sol::object method = entity_table[method_name];
	if (method.get_type() != sol::type::function)
	{
		return false;
	}

	const sol::protected_function function = method;
	const sol::protected_function_result result = function(entity, argument);
	if (!result.valid())
	{
		const sol::error err = result;
		LogLuaError(runtime, method_name, err);
		return false;
	}
	return result.valid() && result.get_type(0) == sol::type::boolean && result.get<bool>();
}

bool MudTransfer(LuaRuntimeContext runtime, const sol::object &entity, const sol::object &room)
{
	if (CallEntityMethod(runtime, entity, "teleport", room))
	{
		return true;
	}
	return CallEntityMethod(runtime, entity, "moveToRoom", room);
}

bool MudForce(LuaRuntimeContext runtime, const sol::object &entity, const sol::object &command)
{
	const auto owner_room = GetRuntimeOwnerRoom(runtime);
	if (owner_room == kNowhere || !entity.is<sol::table>())
	{
		return LogLuaApiError(runtime, "force: invalid character");
	}

	sol::table entity_table = entity;
	const sol::object room_vnum = entity_table["roomVnum"];
	if (!room_vnum.is<int>() || room_vnum.as<int>() != world[owner_room]->vnum)
	{
		return LogLuaApiError(runtime, "force: target must be in trigger owner room");
	}

	return CallEntityMethod(runtime, entity, "force", command);
}

bool MudLog(LuaRuntimeContext runtime, const sol::object &message)
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
	snprintf(buf, sizeof(buf), "(Lua trigger: %s, VNum: %d, owner vnum: %d, owner uid: %ld) : %s",
		GetTriggerName(runtime.trigger),
		GetTriggerVnum(runtime.trigger),
		GetOwnerVnum(runtime.owner),
		GetOwnerUid(runtime.owner),
		text.c_str());
	script_log(buf, LogMode::OFF);
	return true;
}

bool ParseWaitDelay(sol::variadic_args args, long &time)
{
	time = 0;
	if (args.size() == 0)
	{
		return false;
	}

	if (args[0].is<int>())
	{
		time = args[0].as<int>();
		if (args.size() > 1 && args[1].is<std::string>())
		{
			const auto suffix = args[1].as<std::string>();
			if (suffix == "t")
			{
				time *= kLuaPulsesPerMudHour;
			}
			else if (suffix == "s")
			{
				time *= kPassesPerSec;
			}
		}
		return IsValidWaitDelay(time);
	}

	if (!args[0].is<std::string>())
	{
		return false;
	}

	const auto text = args[0].as<std::string>();
	long value = 0;
	long hr = 0;
	long min = 0;
	char suffix = 0;
	if (sscanf(text.c_str(), "until %ld:%ld", &hr, &min) == 2)
	{
		if (!IsValidWaitTime(hr, min))
		{
			return false;
		}
		min += hr * 60;
		const auto target_time = (min * kLuaPulsesPerMudHour) / 60;
		const auto current_time = static_cast<long>(GlobalObjects::heartbeat().global_pulse_number() % kLuaPulsesPerMudHour)
			+ (time_info.hours * kLuaPulsesPerMudHour);
		time = current_time >= target_time
			? (kSecsPerMudDay * kPassesPerSec) - current_time + target_time
			: target_time - current_time;
		return IsValidWaitDelay(time);
	}
	if (sscanf(text.c_str(), "until %ld", &hr) == 1)
	{
		const auto parsed_hour = hr / 100;
		const auto parsed_min = hr % 100;
		if (!IsValidWaitTime(parsed_hour, parsed_min))
		{
			return false;
		}
		min = parsed_min + (parsed_hour * 60);
		const auto target_time = (min * kLuaPulsesPerMudHour) / 60;
		const auto current_time = static_cast<long>(GlobalObjects::heartbeat().global_pulse_number() % kLuaPulsesPerMudHour)
			+ (time_info.hours * kLuaPulsesPerMudHour);
		time = current_time >= target_time
			? (kSecsPerMudDay * kPassesPerSec) - current_time + target_time
			: target_time - current_time;
		return IsValidWaitDelay(time);
	}
	if (sscanf(text.c_str(), "%ld %c", &value, &suffix) >= 1)
	{
		time = value;
		if (suffix == 't')
		{
			time *= kLuaPulsesPerMudHour;
		}
		else if (suffix == 's')
		{
			time *= kPassesPerSec;
		}
		return IsValidWaitDelay(time);
	}
	return false;
}

LuaRuntimeContext CurrentRuntime(LuaRuntimeContext *runtime)
{
	return runtime ? *runtime : LuaRuntimeContext{};
}

} // namespace

int MudWait(LuaRuntimeContext runtime, sol::this_state state, sol::variadic_args args)
{
	long time = 0;
	if (!ParseWaitDelay(args, time))
	{
		LogLuaApiError(runtime, "wait: invalid delay");
		return luaL_error(state, "mud.wait: invalid delay");
	}
	if (runtime.trigger
		&& ((runtime.trigger->get_attach_type() == MOB_TRIGGER && IS_SET(GET_TRIG_TYPE(runtime.trigger), MTRIG_DEATH))
			|| (runtime.trigger->get_attach_type() == OBJ_TRIGGER && IS_SET(GET_TRIG_TYPE(runtime.trigger), OTRIG_PURGE))))
	{
		LogLuaApiError(runtime, "wait: unsupported for death/purge triggers");
		return luaL_error(state, "mud.wait: unsupported for this trigger");
	}
	if (!IsValidWaitDelay(time) || !LuaWaitRegistrySchedule(runtime, static_cast<int>(time)))
	{
		LogLuaApiError(runtime, "wait: unable to schedule");
		return luaL_error(state, "mud.wait: unable to schedule");
	}
	return 0;
}

sol::table BuildMudNamespace(sol::state &lua, LuaRuntimeContext *runtime)
{
	sol::table mud = lua.create_table();
	mud["log"] = [runtime](const sol::object &message) {
		return MudLog(CurrentRuntime(runtime), message);
	};
	mud["random"] = [](const sol::object &first, sol::optional<sol::object> second) {
		return MudRandom(first, second.value_or(sol::lua_nil));
	};
	mud["roll"] = [](const sol::object &count, const sol::object &sides) {
		return MudRoll(count, sides);
	};
	mud["percent"] = [](const sol::object &chance) {
		return MudPercent(chance);
	};
	mud["charByUid"] = [&lua, runtime](const sol::object &uid) {
		return BuildCharView(lua, FindCharByUid(uid), CurrentRuntime(runtime));
	};
	mud["objById"] = [&lua, runtime](const sol::object &id) {
		return BuildObjView(lua, FindObjById(id), CurrentRuntime(runtime));
	};
	mud["findObj"] = [&lua, runtime](const sol::object &vnum) {
		return BuildObjView(lua, FindObjByVnum(vnum), CurrentRuntime(runtime));
	};
	mud["findMob"] = [&lua, runtime](const sol::object &vnum) {
		return BuildCharView(lua, FindMobByVnum(vnum), CurrentRuntime(runtime));
	};
	mud["mobCount"] = [](const sol::object &vnum) {
		return GetCurrentMobCount(vnum);
	};
	mud["objCount"] = [](const sol::object &vnum) {
		return GetCurrentObjectCount(vnum);
	};
	mud["room"] = [&lua, runtime](const sol::object &vnum) {
		return BuildRoomViewByVnum(lua, vnum, CurrentRuntime(runtime));
	};
	mud["zone"] = [&lua](const sol::object &vnum) {
		return BuildZoneView(lua, vnum);
	};
	mud["time"] = [&lua]() {
		return BuildTimeInfo(lua);
	};
	mud["weather"] = [&lua]() {
		return BuildWeatherInfo(lua);
	};
	mud["date"] = [&lua](sol::variadic_args args) -> sol::object {
		sol::object fmt = (args.size() > 0 ? args[0].get<sol::object>() : sol::lua_nil);
		sol::object ts = (args.size() > 1 ? args[1].get<sol::object>() : sol::lua_nil);
		return BuildRealDate(lua, fmt, ts);
	};
	mud["loadObj"] = [&lua, runtime](const sol::object &vnum, const sol::object &room) {
		return BuildObjView(lua, LoadObjToRoom(vnum, room), CurrentRuntime(runtime));
	};
	mud["loadObjToChar"] = [&lua, runtime](const sol::object &vnum, const sol::object &character) {
		return BuildObjView(lua, LoadObjToChar(vnum, character), CurrentRuntime(runtime));
	};
	mud["loadMob"] = [&lua, runtime](const sol::object &vnum, const sol::object &room) {
		return BuildCharView(lua, LoadMobToRoom(vnum, room), CurrentRuntime(runtime));
	};
	mud["purge"] = [](const sol::object &entity) {
		return MudPurge(entity);
	};
	mud["damage"] = [runtime](const sol::object &victim, const sol::object &amount, const sol::object &type) {
		return MudDamage(CurrentRuntime(runtime), victim, amount, type);
	};
	mud["castSpell"] = [runtime](const sol::object &spell_name, const sol::object &target) {
		return MudCastSpell(CurrentRuntime(runtime), spell_name, target);
	};
	mud["transfer"] = [runtime](const sol::object &entity, const sol::object &room) {
		return MudTransfer(CurrentRuntime(runtime), entity, room);
	};
	mud["force"] = [runtime](const sol::object &entity, const sol::object &command) {
		return MudForce(CurrentRuntime(runtime), entity, command);
	};
	mud["echo"] = [runtime](const sol::object &message) {
		return MudEcho(CurrentRuntime(runtime), message);
	};
	mud["echoaround"] = [runtime](const sol::object &actor, const sol::object &message) {
		return MudEchoAround(CurrentRuntime(runtime), actor, message);
	};
	mud["wait"] = sol::yielding([runtime](sol::this_state state, sol::variadic_args args) {
		return MudWait(CurrentRuntime(runtime), state, args);
	});

	sol::table world_table = lua.create_table();
	world_table["curobjCount"] = [](const sol::object &vnum) {
		return GetWorldCurrentObjectCount(vnum);
	};
	world_table["gameobjCount"] = [](const sol::object &vnum) {
		return GetWorldGameObjectCount(vnum);
	};
	world_table["maxobjCount"] = [](const sol::object &vnum) {
		return GetWorldMaxObjectCount(vnum);
	};
	world_table["set"] = [](const sol::object &context, const sol::object &name, const sol::object &value) {
		return SetLuaWorldVar(context, name, value);
	};
	world_table["get"] = [&lua](const sol::object &context, const sol::object &name, sol::variadic_args args) {
		const sol::object default_value = args.size() > 0
			? static_cast<sol::object>(args[0])
			: sol::make_object(lua, sol::lua_nil);
		return GetLuaWorldVar(lua, context, name, default_value);
	};
	world_table["delete"] = [](const sol::object &context, const sol::object &name) {
		return DeleteLuaWorldVar(context, name);
	};
	world_table["clearContext"] = [](const sol::object &context) {
		return ClearLuaWorldContext(context);
	};
	mud["world"] = world_table;

	return mud;
}

void PrintLuaWorldVars(CharData *ch, std::optional<long> context)
{
	SendMsgToChar("Список Lua world-переменных:\r\n", ch);
	for (const auto &current : LuaWorldVars())
	{
		if (context && context.value() != current.first.first)
		{
			continue;
		}

		std::stringstream str_out;
		str_out << "Context: " << current.first.first;
		str_out << ", Name: " << (!current.first.second.empty() ? current.first.second : "[not set]");
		str_out << ", Value: " << (!current.second.empty() ? current.second : "[not set]");
		str_out << "\r\n";
		SendMsgToChar(str_out.str(), ch);
	}
}

} // namespace lua_scripting

#else

namespace lua_scripting {

void PrintLuaWorldVars(CharData *ch, std::optional<long> /*context*/)
{
	SendMsgToChar("LuaJIT-прототип не собран.\r\n", ch);
}

} // namespace lua_scripting

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
