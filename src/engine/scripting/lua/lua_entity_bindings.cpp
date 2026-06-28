#include "engine/scripting/lua/lua_internal.h"

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "administration/privilege.h"
#include "engine/core/comm.h"
#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/structs/flag_data.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/interpreter.h"
#include "engine/core/target_resolver.h"
#include "gameplay/core/constants.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/fight/fight_constants.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/damage.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/spells.h"
#include "gameplay/skills/skills.h"
#include "utils/grammar/gender.h"
#include "utils/random.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <array>
#include <optional>
#include <sstream>
#include <tuple>

void ExtractTrigger(Trigger *trig);
extern char arg[kMaxInputLength];

namespace lua_scripting {

namespace {

constexpr int kLuaHandleTypeChar = 1;
constexpr int kLuaHandleTypeObj = 2;
char kLuaEntityHandleKey;

bool GetEntityHandleTable(sol::table entity_table)
{
	lua_State *state = entity_table.lua_state();
	entity_table.push();
	lua_pushlightuserdata(state, &kLuaEntityHandleKey);
	lua_rawget(state, -2);
	lua_remove(state, -2);
	return lua_istable(state, -1);
}

void PopEntityHandleTable(lua_State *state)
{
	lua_pop(state, 1);
}

int GetEntityHandleIntField(lua_State *state, const char *field)
{
	lua_getfield(state, -1, field);
	const auto result = lua_isnumber(state, -1) ? static_cast<int>(lua_tointeger(state, -1)) : 0;
	lua_pop(state, 1);
	return result;
}

long GetEntityHandleLongField(lua_State *state, const char *field)
{
	lua_getfield(state, -1, field);
	const auto result = lua_isnumber(state, -1) ? static_cast<long>(lua_tointeger(state, -1)) : 0;
	lua_pop(state, 1);
	return result;
}

RoomRnum GetEntityHandleRoomField(lua_State *state, const char *field)
{
	lua_getfield(state, -1, field);
	const auto result = lua_isnumber(state, -1) ? static_cast<RoomRnum>(lua_tointeger(state, -1)) : kNowhere;
	lua_pop(state, 1);
	return result;
}

CharData *GetLuaCharFromObject(const sol::object &entity, LuaRuntimeContext)
{
	if (!entity.is<sol::table>())
	{
		return nullptr;
	}

	sol::table entity_table = entity;
	if (!GetEntityHandleTable(entity_table))
	{
		return nullptr;
	}

	lua_State *state = entity_table.lua_state();
	const auto type = GetEntityHandleIntField(state, "type");
	const auto uid = GetEntityHandleLongField(state, "uid");
	const auto required_room = GetEntityHandleRoomField(state, "required_room");
	PopEntityHandleTable(state);
	if (type != kLuaHandleTypeChar || uid == 0)
	{
		return nullptr;
	}

	LuaEntityHandle char_handle(LuaEntityHandle::LuaEntityType::Char);
	char_handle.char_uid = uid;
	char_handle.required_room = required_room;
	return ResolveChar(char_handle);
}

void RegisterCharView(sol::state &, sol::table view, CharData *ch, RoomRnum required_room)
{
	lua_State *state = view.lua_state();
	view.push();
	lua_pushlightuserdata(state, &kLuaEntityHandleKey);
	lua_newtable(state);
	lua_pushinteger(state, kLuaHandleTypeChar);
	lua_setfield(state, -2, "type");
	lua_pushinteger(state, ch ? ch->get_uid() : 0);
	lua_setfield(state, -2, "uid");
	lua_pushinteger(state, required_room);
	lua_setfield(state, -2, "required_room");
	lua_rawset(state, -3);
	lua_pop(state, 1);
}

void RegisterObjView(sol::state &, sol::table view, ObjData *obj)
{
	lua_State *state = view.lua_state();
	view.push();
	lua_pushlightuserdata(state, &kLuaEntityHandleKey);
	lua_newtable(state);
	lua_pushinteger(state, kLuaHandleTypeObj);
	lua_setfield(state, -2, "type");
	lua_pushinteger(state, obj ? static_cast<lua_Integer>(obj->get_id()) : 0);
	lua_setfield(state, -2, "id");
	lua_rawset(state, -3);
	lua_pop(state, 1);
}

bool GetLuaSkillId(const sol::object &value, ESkill &skill_id)
{
	if (value.is<std::string>())
	{
		auto skill_name = value.as<std::string>();
		if (skill_name.empty())
		{
			return false;
		}
		skill_id = FixNameAndFindSkillId(skill_name.data());
		return MUD::Skills().IsValid(skill_id);
	}
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

bool GetLuaSpellId(const sol::object &value, ESpell &spell_id)
{
	if (value.is<std::string>())
	{
		auto spell_name = value.as<std::string>();
		if (spell_name.empty())
		{
			return false;
		}
		spell_id = FixNameAndFindSpellId(spell_name);
		return spell_id != ESpell::kUndefined;
	}
	if (!value.is<int>())
	{
		return false;
	}

	const auto raw_id = value.as<int>();
	if (raw_id < to_underlying(ESpell::kFirst) || raw_id > to_underlying(ESpell::kLast))
	{
		return false;
	}

	spell_id = static_cast<ESpell>(raw_id);
	return true;
}

bool GetLuaTurnValue(const sol::object &value, bool &enabled)
{
	if (value.is<bool>())
	{
		enabled = value.as<bool>();
		return true;
	}
	if (!value.is<std::string>())
	{
		return false;
	}

	const auto text = value.as<std::string>();
	if (text == "set")
	{
		enabled = true;
		return true;
	}
	if (text == "clear")
	{
		enabled = false;
		return true;
	}
	return false;
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
	if (affect == EAffect::kUndefined)
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

CharData *GetCharLeader(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch && ch->has_master() ? ch->get_master() : nullptr;
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
	return ch && privilege::IsImmortal(ch);
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

int GetCharClass(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? to_underlying(ch->GetClass()) : 0;
}

int GetCharSex(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? to_underlying(ch->get_sex()) : 0;
}

ObjData *GetCharEquipment(const LuaEntityHandle &handle, const sol::object &position)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !position.is<int>())
	{
		return nullptr;
	}

	const auto pos = position.as<int>();
	return pos >= 0 && pos < EEquipPos::kNumEquipPos ? GET_EQ(ch, pos) : nullptr;
}

ObjData *GetCharInventoryObject(const LuaEntityHandle &handle, const sol::object &target)
{
	auto *ch = ResolveChar(handle);
	if (!ch)
	{
		return nullptr;
	}

	if (target.is<int>())
	{
		const auto vnum = target.as<int>();
		for (auto *obj = ch->carrying; obj; obj = obj->get_next_content())
		{
			if (GET_OBJ_VNUM(obj) == vnum)
			{
				return obj;
			}
		}
		return nullptr;
	}

	if (target.is<std::string>())
	{
		const auto name = target.as<std::string>();
		return name.empty() ? nullptr : get_obj_in_list_vis(ch, name.c_str(), ch->carrying);
	}

	return nullptr;
}

bool CharLag(LuaRuntimeContext runtime, const LuaEntityHandle &handle, const sol::object &value, const sol::object &unit)
{
	auto *ch = ResolveChar(handle);
	if (!ch)
	{
		return LogLuaApiError(runtime, "lag: invalid character");
	}
	if (!value.is<int>())
	{
		return LogLuaApiError(runtime, "lag: value must be an integer");
	}

	const auto delay = value.as<int>();
	if (delay <= 0)
	{
		return LogLuaApiError(runtime, "lag: value must be positive");
	}
	if (privilege::IsImmortal(ch))
	{
		return true;
	}

	if (unit.get_type() == sol::type::lua_nil)
	{
		SetBattleLag(ch, delay);
		return true;
	}
	if (!unit.is<std::string>())
	{
		return LogLuaApiError(runtime, "lag: unit must be a string");
	}

	const auto unit_name = unit.as<std::string>();
	if (unit_name == "p")
	{
		SetWaitState(ch, delay);
		return true;
	}
	if (unit_name == "battle" || unit_name == "round")
	{
		SetBattleLag(ch, delay);
		return true;
	}

	return LogLuaApiError(runtime, "lag: unsupported unit");
}

std::optional<std::string> GetCharNamesField(const LuaEntityHandle &handle, const std::string &field_name)
{
	auto *ch = ResolveChar(handle);
	if (!ch)
	{
		return std::nullopt;
	}

	if (field_name == "name")
	{
		return GET_NAME(ch);
	}
	if (field_name == "iname")
	{
		return GET_PAD(ch, 0);
	}
	if (field_name == "rname")
	{
		return GET_PAD(ch, 1);
	}
	if (field_name == "dname")
	{
		return GET_PAD(ch, 2);
	}
	if (field_name == "vname")
	{
		return GET_PAD(ch, 3);
	}
	if (field_name == "tname")
	{
		return GET_PAD(ch, 4);
	}
	if (field_name == "pname")
	{
		return GET_PAD(ch, 5);
	}
	if (field_name == "UPname")
	{
		auto name = std::string(GET_NAME(ch));
		return utils::colorCAP(name);
	}
	if (field_name == "UPiname")
	{
		auto name = std::string(GET_PAD(ch, 0));
		return utils::colorCAP(name);
	}
	if (field_name == "UPrname")
	{
		auto name = std::string(GET_PAD(ch, 1));
		return utils::colorCAP(name);
	}
	if (field_name == "UPdname")
	{
		auto name = std::string(GET_PAD(ch, 2));
		return utils::colorCAP(name);
	}
	if (field_name == "UPvname")
	{
		auto name = std::string(GET_PAD(ch, 3));
		return utils::colorCAP(name);
	}
	if (field_name == "UPtname")
	{
		auto name = std::string(GET_PAD(ch, 4));
		return utils::colorCAP(name);
	}
	if (field_name == "UPpname")
	{
		auto name = std::string(GET_PAD(ch, 5));
		return utils::colorCAP(name);
	}

	if (field_name == "m")
	{
		return grammar::DativePronoun(ch->get_sex());
	}
	if (field_name == "s")
	{
		return grammar::PossessivePronoun(ch->get_sex());
	}
	if (field_name == "e")
	{
		return grammar::PersonalPronoun(ch->get_sex());
	}
	if (field_name == "g")
	{
		return grammar::SexEnding(ch->get_sex(), 1);
	}
	if (field_name == "u")
	{
		return grammar::SexEnding(ch->get_sex(), 2);
	}
	if (field_name == "w")
	{
		return grammar::SexEnding(ch->get_sex(), 3);
	}
	if (field_name == "q")
	{
		return grammar::SexEnding(ch->get_sex(), 4);
	}
	if (field_name == "y")
	{
		return grammar::SexEnding(ch->get_sex(), 5);
	}
	if (field_name == "a")
	{
		return grammar::SexEnding(ch->get_sex(), 6);
	}
	if (field_name == "r")
	{
		return grammar::SexEnding(ch->get_sex(), 7);
	}
	if (field_name == "x")
	{
		return grammar::SexEnding(ch->get_sex(), 8);
	}
	if (field_name == "h")
	{
		return grammar::InstrEnding(ch->get_sex());
	}

	return std::nullopt;
}

sol::object BuildCharNamesView(sol::state &lua, const LuaEntityHandle &handle)
{
	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle](sol::object, const std::string &key) -> sol::object {
		const auto value = GetCharNamesField(handle, key);
		return value ? sol::make_object(lua, *value) : sol::make_object(lua, sol::lua_nil);
	};
	metatable[sol::meta_function::new_index] = [](sol::this_state state) {
		return luaL_error(state, "CharData names Lua view is read-only");
	};
	view[sol::metatable_key] = metatable;
	return sol::make_object(lua, view);
}

bool CharHasAffect(const LuaEntityHandle &handle, const sol::object &affect)
{
	auto *ch = ResolveChar(handle);
	EAffect affect_id = EAffect::kUndefined;
	return ch && GetLuaAffectId(affect, affect_id) && AFF_FLAGGED(ch, affect_id);
}

bool CharCanSee(LuaRuntimeContext runtime, const LuaEntityHandle &handle, const sol::object &target)
{
	auto *ch = ResolveChar(handle);
	auto *victim = GetLuaCharFromObject(target, runtime);
	return ch && victim && sight::CanSee(ch, victim);
}

CharData *GetCharEnemy(const LuaEntityHandle &handle)
{
	auto *ch = ResolveChar(handle);
	return ch ? ch->GetEnemy() : nullptr;
}

long CharGold(const LuaEntityHandle &handle, const sol::object &delta)
{
	auto *ch = ResolveChar(handle);
	if (!ch)
	{
		return 0;
	}

	if (delta.is<long>())
	{
		const auto amount = delta.as<long>();
		if (amount > 0)
		{
			currencies::AddHand(*ch, currencies::kGold, amount);
		}
		else if (amount < 0)
		{
			currencies::RemoveHand(*ch, currencies::kGold, -amount);
		}
	}
	else if (delta.is<int>())
	{
		const auto amount = static_cast<long>(delta.as<int>());
		if (amount > 0)
		{
			currencies::AddHand(*ch, currencies::kGold, amount);
		}
		else if (amount < 0)
		{
			currencies::RemoveHand(*ch, currencies::kGold, -amount);
		}
	}

	return currencies::GetHand(*ch, currencies::kGold);
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
		SetSkill(ch, skill_id, value.as<int>());
	}
	return GetSkill(ch, skill_id);
}

bool CharSkillTurn(const LuaEntityHandle &handle, const sol::object &skill, const sol::object &value)
{
	auto *ch = ResolveChar(handle);
	ESkill skill_id = ESkill::kUndefined;
	bool enabled = false;
	if (!ch || !GetLuaSkillId(skill, skill_id) || !GetLuaTurnValue(value, enabled))
	{
		return false;
	}
	if (MUD::Class(ch->GetClass()).skills[skill_id].IsUnavailable())
	{
		return false;
	}

	if (GetSkillBonus(ch, skill_id))
	{
		if (!enabled)
		{
			SetSkill(ch, skill_id, 0);
		}
	}
	else if (enabled)
	{
		SetSkill(ch, skill_id, 5);
	}
	return true;
}

bool CharSpellTurn(const LuaEntityHandle &handle, const sol::object &spell, const sol::object &value)
{
	auto *ch = ResolveChar(handle);
	ESpell spell_id = ESpell::kUndefined;
	bool enabled = false;
	if (!ch || !GetLuaSpellId(spell, spell_id) || !GetLuaTurnValue(value, enabled) || !CanGetSpell(ch, spell_id))
	{
		return false;
	}

	if (IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow))
	{
		if (!enabled)
		{
			REMOVE_BIT(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow);
			if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp))
			{
				GET_SPELL_MEM(ch, spell_id) = 0;
			}
		}
	}
	else if (enabled)
	{
		SET_BIT(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow);
	}
	return true;
}

bool CharCanGetSkill(const LuaEntityHandle &handle, const sol::object &skill)
{
	auto *ch = ResolveChar(handle);
	ESkill skill_id = ESkill::kUndefined;
	return ch && GetLuaSkillId(skill, skill_id) && CanGetSkill(ch, skill_id);
}

bool CharCanGetSpell(const LuaEntityHandle &handle, const sol::object &spell)
{
	auto *ch = ResolveChar(handle);
	ESpell spell_id = ESpell::kUndefined;
	return ch && GetLuaSpellId(spell, spell_id) && CanGetSpell(ch, spell_id);
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

	std::array<char, kMaxInputLength> saved_arg{};
	std::copy(arg, arg + kMaxInputLength, saved_arg.begin());
	command_interpreter(ch, command_buffer.data());
	std::copy(saved_arg.begin(), saved_arg.end(), arg);
	return true;
}

bool RewardDailyQuest(const LuaEntityHandle &handle, const sol::object &id)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !id.is<int>())
	{
		return false;
	}

	if (IsCharmice(ch))
	{
		if (!ch->has_master())
		{
			return false;
		}
		ch->get_master()->dquest(id.as<int>());
		return true;
	}

	ch->dquest(id.as<int>());
	return true;
}

bool HasQuest(const LuaEntityHandle &handle, const sol::object &id)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !id.is<int>())
	{
		return false;
	}

	const auto quest_id = id.as<int>();
	return quest_id > 0 && ch->quested_get(quest_id);
}

std::string GetQuest(const LuaEntityHandle &handle, const sol::object &id)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !id.is<int>())
	{
		return "";
	}

	const auto quest_id = id.as<int>();
	return quest_id > 0 ? ch->quested_get_text(quest_id) : "";
}

bool SetQuest(const LuaEntityHandle &handle, const sol::object &id, const sol::object &text)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !id.is<int>() || !text.is<std::string>())
	{
		return false;
	}

	const auto quest_id = id.as<int>();
	if (quest_id <= 0)
	{
		return false;
	}

	auto quest_text = text.as<std::string>();
	ch->quested_add(ch, quest_id, quest_text.data());
	return ch->quested_get(quest_id);
}

bool UnsetQuest(const LuaEntityHandle &handle, const sol::object &id)
{
	auto *ch = ResolveChar(handle);
	if (!ch || !id.is<int>())
	{
		return false;
	}

	const auto quest_id = id.as<int>();
	return quest_id > 0 && ch->quested_remove(quest_id);
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
		const sol::object hide_invisible_object = table["hideInvisible"];
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

bool MoveCharForDgTeleport(CharData *ch, RoomRnum target, bool look, bool greet)
{
	if (!ch || ch->in_room == kNowhere || ch->in_room == target)
	{
		return false;
	}

	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, target);
	if (look && !ch->IsNpc())
	{
		sight::look_at_room(ch, true);
	}
	if (greet)
	{
		greet_mtrigger(ch, -1);
		greet_otrigger(ch, -1);
	}
	return true;
}

bool DgTeleportSingleChar(CharData *ch, RoomRnum source, RoomRnum target)
{
	if (!ch || ch->in_room != source || ch->in_room == target)
	{
		return false;
	}

	bool onhorse = false;
	if (IsCharmice(ch) && ch->has_master() && ch->in_room == ch->get_master()->in_room)
	{
		ch = ch->get_master();
	}

	const auto people_copy = world[ch->in_room]->people;
	for (const auto charmee : people_copy)
	{
		if (IsCharmice(charmee) && charmee->get_master() == ch)
		{
			MoveCharForDgTeleport(charmee, target, false, false);
		}
	}

	if (mount::GetHorse(ch) && (mount::IsOnHorse(ch) || mount::HasHorse(ch, true)))
	{
		MoveCharForDgTeleport(mount::GetHorse(ch), target, false, false);
		onhorse = true;
	}

	MoveCharForDgTeleport(ch, target, true, true);
	if (!onhorse)
	{
		mount::Dismount(ch);
	}
	return true;
}

bool DgTeleportRoomPeople(LuaRoomView source, RoomRnum target, bool chars_only)
{
	if (!IsValidRoom(source) || !IsValidRoom(LuaRoomView{target}))
	{
		return false;
	}

	bool moved = false;
	CharData *lastchar = nullptr;
	const auto people_copy = world[source.room]->people;
	for (auto *ch : people_copy)
	{
		if (!ch || ch->in_room != source.room)
		{
			continue;
		}
		if (chars_only && ch->IsNpc() && !IsCharmice(ch))
		{
			continue;
		}
		if (ch->in_room == target)
		{
			continue;
		}

		bool onhorse = false;
		if (chars_only && mount::GetHorse(ch) && (mount::IsOnHorse(ch) || mount::HasHorse(ch, true)))
		{
			MoveCharForDgTeleport(mount::GetHorse(ch), target, false, false);
			onhorse = true;
		}

		moved = MoveCharForDgTeleport(ch, target, true, false) || moved;
		if (chars_only && !onhorse)
		{
			mount::Dismount(ch);
		}
		if (!ch->IsNpc())
		{
			lastchar = ch;
		}
	}

	if (lastchar)
	{
		greet_mtrigger(lastchar, -1);
		greet_otrigger(lastchar, -1);
	}
	return moved;
}

bool DgTeleportFromRoom(LuaRuntimeContext runtime, LuaRoomView source, const sol::object &target, const sol::object &room)
{
	if (!IsValidRoom(source))
	{
		return LogLuaApiError(runtime, "wteleport: invalid source room");
	}

	const auto room_rnum = GetRoomFromLua(room);
	if (!IsValidRoom(LuaRoomView{room_rnum}))
	{
		return LogLuaApiError(runtime, "wteleport: invalid target room");
	}

	if (target.is<std::string>())
	{
		const auto target_name = target.as<std::string>();
		if (target_name == "all")
		{
			return DgTeleportRoomPeople(source, room_rnum, false);
		}
		if (target_name == "allchar")
		{
			return DgTeleportRoomPeople(source, room_rnum, true);
		}
		return LogLuaApiError(runtime, "wteleport: string target must be all or allchar");
	}

	auto *ch = GetLuaCharFromObject(target, runtime);
	if (!ch)
	{
		return LogLuaApiError(runtime, "wteleport: target must be all, allchar, or Char");
	}
	return DgTeleportSingleChar(ch, source.room, room_rnum);
}

int CountRoomPeople(LuaRoomView room)
{
	if (!IsValidRoom(room))
	{
		return 0;
	}
	return static_cast<int>(world[room.room]->people.size());
}

CharData *GetRoomPersonAt(LuaRoomView room, int index)
{
	if (!IsValidRoom(room))
	{
		return nullptr;
	}
	if (index <= 0)
	{
		return nullptr;
	}
	int current = 1;
	for (auto *ch : world[room.room]->people)
	{
		if (current++ == index)
		{
			return ch;
		}
	}
	return nullptr;
}

int CountRoomObjects(LuaRoomView room)
{
	if (!IsValidRoom(room))
	{
		return 0;
	}
	return static_cast<int>(world[room.room]->contents.size());
}

ObjData *GetRoomObjectAt(LuaRoomView room, int index)
{
	if (!IsValidRoom(room))
	{
		return nullptr;
	}
	if (index <= 0)
	{
		return nullptr;
	}
	int current = 1;
	for (auto *obj : world[room.room]->contents)
	{
		if (current++ == index)
		{
			return obj;
		}
	}
	return nullptr;
}

sol::object GetLiveCollectionItem(sol::state &lua, LuaRuntimeContext runtime, LuaRoomView room, bool people, int index)
{
	return people
		? BuildCharView(lua, GetRoomPersonAt(room, index), runtime)
		: BuildObjView(lua, GetRoomObjectAt(room, index), runtime);
}

sol::object BuildLiveRoomCollection(sol::state &lua, LuaRoomView room, LuaRuntimeContext runtime, bool people)
{
	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, room, runtime, people](sol::object, sol::object key) -> sol::object {
		if (key.is<int>())
		{
			return GetLiveCollectionItem(lua, runtime, room, people, key.as<int>());
		}
		if (key.is<std::string>())
		{
			const auto name = key.as<std::string>();
			if (name == "count" || name == "size")
			{
				const auto count = people ? CountRoomPeople(room) : CountRoomObjects(room);
				return sol::make_object(lua, count);
			}
		}
		return sol::make_object(lua, sol::lua_nil);
	};
	metatable[sol::meta_function::length] = [room, people]() {
		return people ? CountRoomPeople(room) : CountRoomObjects(room);
	};
	metatable["__live_ipairs"] = [&lua, room, runtime, people](sol::object collection) {
		auto iterator = sol::make_object(lua, sol::as_function(
			[&lua, room, runtime, people](sol::object, int previous) -> std::tuple<sol::object, sol::object> {
				const auto next = previous + 1;
				auto value = GetLiveCollectionItem(lua, runtime, room, people, next);
				if (value == sol::lua_nil)
				{
					return std::make_tuple(sol::make_object(lua, sol::lua_nil), sol::make_object(lua, sol::lua_nil));
				}
				return std::make_tuple(sol::make_object(lua, next), value);
			}));
		return std::make_tuple(iterator, collection, 0);
	};
	metatable[sol::meta_function::new_index] = [](sol::this_state state) {
		return luaL_error(state, "Room collection Lua view is read-only");
	};
	view[sol::metatable_key] = metatable;
	return sol::make_object(lua, view);
}

bool PushLuaLiveCollectionIpairsInternal(lua_State *state)
{
	if (!lua_istable(state, 1) || !lua_getmetatable(state, 1))
	{
		return false;
	}

	lua_getfield(state, -1, "__live_ipairs");
	if (!lua_isfunction(state, -1))
	{
		lua_pop(state, 2);
		return false;
	}

	lua_pushvalue(state, 1);
	lua_call(state, 1, 3);
	lua_remove(state, -4);
	lua_remove(state, 1);
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

bool PurgeRoomExit(const LuaRoomView &view, const sol::object &direction)
{
	int dir = 0;
	if (!IsValidRoom(view) || !GetLuaDirection(direction, dir))
	{
		return false;
	}

	if (world[view.room]->dir_option[dir])
	{
		world[view.room]->dir_option[dir].reset();
	}
	return true;
}

bool SetRoomExit(const LuaRoomView &view, const sol::object &direction, const sol::object &options)
{
	int dir = 0;
	if (!IsValidRoom(view) || !GetLuaDirection(direction, dir) || !options.is<sol::table>())
	{
		return false;
	}

	auto exit = world[view.room]->dir_option[dir];
	if (!exit)
	{
		exit = std::make_shared<ExitData>();
		world[view.room]->dir_option[dir] = exit;
	}

	bool result = true;
	sol::table table = options;
	const sol::object flags = table["flags"];
	if (flags.is<std::string>())
	{
		const auto value = flags.as<std::string>();
		asciiflag_conv(value.c_str(), &exit->exit_info);
	}

	const sol::object to_room = table["toRoom"];
	if (to_room.valid() && to_room != sol::lua_nil)
	{
		const auto room_rnum = GetRoomFromLua(to_room);
		if (room_rnum != kNowhere)
		{
			exit->to_room(room_rnum);
		}
		else
		{
			result = false;
		}
	}

	const sol::object description = table["description"];
	if (description.is<std::string>())
	{
		exit->general_description = description.as<std::string>() + "\r\n";
	}

	const sol::object key = table["key"];
	if (key.is<int>())
	{
		exit->key = key.as<int>();
	}

	const sol::object name = table["name"];
	if (name.is<std::string>())
	{
		const auto value = name.as<std::string>();
		exit->set_keywords(value.c_str());
	}

	const sol::object lock = table["lock"];
	if (lock.is<int>())
	{
		const auto value = lock.as<int>();
		if (value >= 0 && value <= 255)
		{
			exit->lock_complexity = value;
		}
		else
		{
			result = false;
		}
	}

	return result;
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

std::optional<std::string> GetObjNamesField(const LuaEntityHandle &handle, const std::string &field_name)
{
	auto *obj = ResolveObj(handle);
	if (!obj)
	{
		return std::nullopt;
	}

	const auto get_pname_or_aliases = [obj](grammar::ECase name_case) -> std::string {
		const auto &pname = obj->get_PName(name_case);
		return pname.empty() ? obj->get_aliases() : pname;
	};

	if (field_name == "name")
	{
		return obj->get_aliases();
	}
	if (field_name == "iname")
	{
		return get_pname_or_aliases(grammar::ECase::kNom);
	}
	if (field_name == "rname")
	{
		return get_pname_or_aliases(grammar::ECase::kGen);
	}
	if (field_name == "dname")
	{
		return get_pname_or_aliases(grammar::ECase::kDat);
	}
	if (field_name == "vname")
	{
		return get_pname_or_aliases(grammar::ECase::kAcc);
	}
	if (field_name == "tname")
	{
		return get_pname_or_aliases(grammar::ECase::kIns);
	}
	if (field_name == "pname")
	{
		return get_pname_or_aliases(grammar::ECase::kPre);
	}
	return std::nullopt;
}

sol::object BuildObjNamesView(sol::state &lua, const LuaEntityHandle &handle)
{
	sol::table view = lua.create_table();
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle](sol::object, const std::string &key) -> sol::object {
		const auto value = GetObjNamesField(handle, key);
		return value ? sol::make_object(lua, *value) : sol::make_object(lua, sol::lua_nil);
	};
	metatable[sol::meta_function::new_index] = [](sol::this_state state) {
		return luaL_error(state, "ObjData names Lua view is read-only");
	};
	view[sol::metatable_key] = metatable;
	return sol::make_object(lua, view);
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

int ObjVal(const LuaEntityHandle &handle, const sol::object &index, const sol::object &value)
{
	auto *obj = ResolveObj(handle);
	if (!obj || !index.is<int>())
	{
		return 0;
	}

	const auto val_index = index.as<int>();
	if (val_index < 0 || val_index > 3)
	{
		return 0;
	}

	if (value.is<int>())
	{
		obj->set_val(static_cast<size_t>(val_index), value.as<int>());
	}
	return GET_OBJ_VAL(obj, static_cast<size_t>(val_index));
}

ObjData *ObjContains(const LuaEntityHandle &handle, const sol::object &target)
{
	auto *obj = ResolveObj(handle);
	if (!obj)
	{
		return nullptr;
	}

	if (target.is<int>())
	{
		const auto vnum = target.as<int>();
		for (auto *item = obj->get_contains(); item; item = item->get_next_content())
		{
			if (GET_OBJ_VNUM(item) == vnum)
			{
				return item;
			}
		}
		return nullptr;
	}

	if (target.is<std::string>())
	{
		const auto name = target.as<std::string>();
		if (name.empty())
		{
			return nullptr;
		}
		for (auto *item = obj->get_contains(); item; item = item->get_next_content())
		{
			if (isname(name.c_str(), item->get_aliases()))
			{
				return item;
			}
		}
	}
	return nullptr;
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
		// mimic DG: halt the trigger so no further script execution
		if (runtime.trigger) {
			GET_TRIG_DEPTH(runtime.trigger) = 0;
		}
		// cancel any pending waits for this trigger/owner
		LuaScriptEngine::CancelWaitsForTrigger(runtime.trigger);
	}
	if (!ch->followers.empty() || ch->has_master())
	{
		follow::DieFollower(ch);
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
	if (type.get_type() == sol::type::lua_nil)
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
			? sol::make_object(lua, sol::lua_nil)
			: default_value;
	}

	const auto value = FindScriptContextVar(script->global_vars, key, context);
	if (value == script->global_vars.end())
	{
		return default_value.get_type() == sol::type::none
			? sol::make_object(lua, sol::lua_nil)
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

bool ApplyDirectTriggerDamage(CharData *victim, int amount)
{
	victim->set_hit(victim->get_hit() - amount);
	update_pos(victim);
	char_dam_message(amount, victim, victim, false);
	if (victim->GetPosition() == EPosition::kDead)
	{
		die(victim, nullptr);
	}
	return true;
}

} // namespace

bool PushLuaLiveCollectionIpairs(lua_State *state)
{
	return PushLuaLiveCollectionIpairsInternal(state);
}

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
	if (privilege::IsImmortal(victim) && amount > 0)
	{
		return LogLuaApiError(runtime, "damage: immortal victim");
	}
	if (type_object.get_type() == sol::type::lua_nil)
	{
		return ApplyDirectTriggerDamage(victim, amount);
	}

	Damage damage(SimpleDmg(fight::EDamageSource::kTriggerDeath), amount, damage_type);
	return damage.Process(attacker, victim) != 0;
}

sol::object BuildScriptContextView(sol::state &lua, Script *script, long context)
{
	if (!script)
	{
		return sol::make_object(lua, sol::lua_nil);
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
					: sol::make_object(lua, sol::lua_nil);
				return GetScriptContextValue(lua, script, context, name, default_value);
			}));
		}
		if (key == "delete")
		{
			return sol::make_object(lua, sol::as_function([script, context](sol::object, const std::string &name) {
				return DeleteScriptContextValue(script, context, name);
			}));
		}

		return GetScriptContextValue(lua, script, context, key, sol::make_object(lua, sol::lua_nil));
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

sol::object BuildCharView(sol::state &lua, CharData *ch, LuaRuntimeContext runtime, RoomRnum required_room)
{
	if (!ch)
	{
		return sol::make_object(lua, sol::lua_nil);
	}

	auto handle = MakeCharHandle(ch, required_room);
	sol::table view = lua.create_table();
	RegisterCharView(lua, view, ch, required_room);
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
		if (key == "maxHp")
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
		if (key == "roomVnum")
		{
			return sol::make_object(lua, GetCharRoomVnum(handle));
		}
		if (key == "leader")
		{
			return BuildCharView(lua, GetCharLeader(handle), runtime);
		}
		if (key == "isNpc")
		{
			return sol::make_object(lua, IsCharNpc(handle));
		}
		if (key == "isPc")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return IsCharPc(handle);
			}));
		}
		if (key == "isImmortal")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return IsCharImmortal(handle);
			}));
		}
		if (key == "level")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return GetCharLevel(handle);
			}));
		}
		if (key == "position")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return GetCharPosition(handle);
			}));
		}
		if (key == "class")
		{
			return sol::make_object(lua, GetCharClass(handle));
		}
		if (key == "sex")
		{
			return sol::make_object(lua, GetCharSex(handle));
		}
		if (key == "names")
		{
			return BuildCharNamesView(lua, handle);
		}
		if (key == "hasAffect")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object affect) {
				return CharHasAffect(handle, affect);
			}));
		}
		if (key == "canSee")
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
		if (key == "gold")
		{
			return sol::make_object(lua, sol::as_function([&lua, handle](sol::object, sol::variadic_args args) {
				const sol::object delta = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::lua_nil);
				return CharGold(handle, delta);
			}));
		}
		if (key == "teleport")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object room) {
				return TeleportChar(handle, room);
			}));
		}
		if (key == "equipment" || key == "eq")
		{
			return sol::make_object(lua, sol::as_function([&lua, runtime, handle](sol::object, sol::object position) {
				return BuildObjView(lua, GetCharEquipment(handle, position), runtime);
			}));
		}
		if (key == "haveObj" || key == "haveobj")
		{
			return sol::make_object(lua, sol::as_function([&lua, runtime, handle](sol::object, sol::object target) {
				return BuildObjView(lua, GetCharInventoryObject(handle, target), runtime);
			}));
		}
		if (key == "lag")
		{
			return sol::make_object(lua, sol::as_function([&lua, runtime, handle](
				sol::object,
				sol::object value,
				sol::variadic_args args) {
				const sol::object unit = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::lua_nil);
				return CharLag(runtime, handle, value, unit);
			}));
		}
		if (key == "skill")
		{
			return sol::make_object(lua, sol::as_function([&lua, handle](sol::object, sol::object skill, sol::variadic_args args) {
				const sol::object value = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::lua_nil);
				return CharSkill(handle, skill, value);
			}));
		}
		if (key == "skillTurn")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object skill, sol::object value) {
				return CharSkillTurn(handle, skill, value);
			}));
		}
		if (key == "spellTurn")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object spell, sol::object value) {
				return CharSpellTurn(handle, spell, value);
			}));
		}
		if (key == "canGetSkill")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object skill) {
				return CharCanGetSkill(handle, skill);
			}));
		}
		if (key == "canGetSpell")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object spell) {
				return CharCanGetSpell(handle, spell);
			}));
		}
		if (key == "feat")
		{
			return sol::make_object(lua, sol::as_function([&lua, handle](sol::object, sol::object feat, sol::variadic_args args) {
				const sol::object value = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::lua_nil);
				return CharFeat(handle, feat, value);
			}));
		}
		if (key == "attachTrigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return AttachTriggerToChar(handle, vnum);
			}));
		}
		if (key == "detachTrigger")
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
		if (key == "isValid")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return IsValidEntity(handle);
			}));
		}
		if (key == "send")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object message) {
				return SendToChar(handle, message);
			}));
		}
		if (key == "force")
		{
			return sol::make_object(lua, sol::as_function([runtime, handle](sol::object, sol::object command) {
				return ForceCharCommand(runtime, handle, command);
			}));
		}
		if (key == "rewardDailyQuest")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object id) {
				return RewardDailyQuest(handle, id);
			}));
		}
		if (key == "hasQuest")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object id) {
				return HasQuest(handle, id);
			}));
		}
		if (key == "getQuest")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object id) {
				return GetQuest(handle, id);
			}));
		}
		if (key == "setQuest")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object id, sol::object text) {
				return SetQuest(handle, id, text);
			}));
		}
		if (key == "unsetQuest")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object id) {
				return UnsetQuest(handle, id);
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
					: sol::make_object(lua, sol::lua_nil);
				return ActFromChar(runtime, handle, message, options);
			}));
		}
		if (key == "purge")
		{
			return sol::make_object(lua, sol::as_function([handle, runtime](sol::object) {
				return PurgeCharEntity(handle, runtime);
			}));
		}

		return sol::make_object(lua, sol::lua_nil);
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
		return sol::make_object(lua, sol::lua_nil);
	}

	auto handle = MakeObjHandle(obj);
	sol::table view = lua.create_table();
	RegisterObjView(lua, view, obj);
	sol::table metatable = lua.create_table();
	metatable[sol::meta_function::index] = [&lua, handle, runtime](sol::object, const std::string &key) -> sol::object {
		if (key == "name")
		{
			return sol::make_object(lua, GetObjName(handle));
		}
		if (key == "names")
		{
			return BuildObjNamesView(lua, handle);
		}
		if (key == "iname" || key == "rname" || key == "dname" || key == "vname" || key == "tname" || key == "pname")
		{
			const auto value = GetObjNamesField(handle, key);
			return value ? sol::make_object(lua, *value) : sol::make_object(lua, sol::lua_nil);
		}
		if (key == "id")
		{
			return sol::make_object(lua, GetObjId(handle));
		}
		if (key == "vnum")
		{
			return sol::make_object(lua, GetObjVnum(handle));
		}
		if (key == "roomVnum")
		{
			return sol::make_object(lua, GetObjRoomVnum(handle));
		}
		if (key == "room")
		{
			auto *obj = ResolveObj(handle);
			return BuildRoomView(lua, obj ? LuaRoomView{obj->get_in_room()} : LuaRoomView{}, true, runtime);
		}
		if (key == "carriedBy")
		{
			auto *obj = ResolveObj(handle);
			return BuildCharView(lua, obj ? obj->get_carried_by() : nullptr, runtime);
		}
		if (key == "wornBy")
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
		if (key == "val")
		{
			return sol::make_object(lua, sol::as_function([&lua, handle](
				sol::object,
				sol::object index,
				sol::variadic_args args) {
				const sol::object value = args.size() > 0
					? static_cast<sol::object>(args[0])
					: sol::make_object(lua, sol::lua_nil);
				return ObjVal(handle, index, value);
			}));
		}
		if (key == "isValid")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return IsValidEntity(handle);
			}));
		}
		if (key == "purge")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object) {
				return PurgeObjEntity(handle);
			}));
		}
		if (key == "moveToRoom")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object room) {
				return MoveObjToRoom(handle, room);
			}));
		}
		if (key == "giveTo")
		{
			return sol::make_object(lua, sol::as_function([runtime, handle](sol::object, sol::object ch) {
				return GiveObjToChar(runtime, handle, ch);
			}));
		}
		if (key == "contains")
		{
			return sol::make_object(lua, sol::as_function([&lua, runtime, handle](sol::object, sol::object target) {
				return BuildObjView(lua, ObjContains(handle, target), runtime);
			}));
		}
		if (key == "attachTrigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return AttachTriggerToObj(handle, vnum);
			}));
		}
		if (key == "detachTrigger")
		{
			return sol::make_object(lua, sol::as_function([handle](sol::object, sol::object vnum) {
				return DetachTriggerFromObj(handle, vnum);
			}));
		}

		return sol::make_object(lua, sol::lua_nil);
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
		return sol::make_object(lua, sol::lua_nil);
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
				return BuildLiveRoomCollection(lua, room, runtime, true);
			}));
		}
		if (key == "objects")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object) {
				return BuildLiveRoomCollection(lua, room, runtime, false);
			}));
		}
		if (key == "wteleport")
		{
			return sol::make_object(lua, sol::as_function([runtime, room](
				sol::object,
				sol::object target,
				sol::object destination) {
				return DgTeleportFromRoom(runtime, room, target, destination);
			}));
		}
		if (key == "exit")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object, sol::object direction) {
				return BuildRoomView(lua, LuaRoomView{GetRoomExit(room, direction)}, false, runtime);
			}));
		}
		if (key == "setExit")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object direction, sol::object options) {
				return SetRoomExit(room, direction, options);
			}));
		}
		if (key == "purgeExit")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object direction) {
				return PurgeRoomExit(room, direction);
			}));
		}
		if (key == "loadObj")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object, sol::object vnum) {
				return vnum.is<int>()
					? BuildObjView(lua, LoadObjToRoom(vnum.as<int>(), room.room), runtime)
					: sol::make_object(lua, sol::lua_nil);
			}));
		}
		if (key == "loadMob")
		{
			return sol::make_object(lua, sol::as_function([&lua, room, runtime](sol::object, sol::object vnum) {
				return vnum.is<int>()
					? BuildCharView(lua, LoadMobToRoom(vnum.as<int>(), room.room), runtime)
					: sol::make_object(lua, sol::lua_nil);
			}));
		}
		if (key == "attachTrigger")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object vnum) {
				return AttachTriggerToRoom(room, vnum);
			}));
		}
		if (key == "detachTrigger")
		{
			return sol::make_object(lua, sol::as_function([room](sol::object, sol::object vnum) {
				return DetachTriggerFromRoom(room, vnum);
			}));
		}

		return sol::make_object(lua, sol::lua_nil);
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
		return sol::make_object(lua, sol::lua_nil);
	}

	return BuildRoomView(lua, LuaRoomView{owner->in_room}, true, runtime);
}

sol::object BuildRoomViewByVnum(sol::state &lua, const sol::object &vnum, LuaRuntimeContext runtime)
{
	if (!vnum.is<int>())
	{
		return sol::make_object(lua, sol::lua_nil);
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
