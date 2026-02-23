/**
 \file parsers.cpp
 \brief JSON to Entity parsing implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "parsers.h"
#include "json_helpers.h"
#include "../../../engine/structs/structs.h"
#include "../../../gameplay/mechanics/dead_load.h"
#include "../../../gameplay/core/constants.h"

namespace admin_api::parsers {

using namespace admin_api::json;

// ============================================================================
// Mob Parsing
// ============================================================================

void ParseMobUpdate(CharData* mob, const nlohmann::json& data)
{
	if (!mob)
	{
		return;
	}

	// === NAMES ===
	if (HasObject(data, "names"))
	{
		const auto& names = data["names"];
		if (HasString(names, "aliases"))
		{
			mob->set_npc_name(Utf8ToKoi8r(names["aliases"].get<std::string>()));
		}
		if (HasString(names, "nominative"))
		{
			mob->player_data.PNames[ECase::kNom] = Utf8ToKoi8r(names["nominative"].get<std::string>());
		}
		if (HasString(names, "genitive"))
		{
			mob->player_data.PNames[ECase::kGen] = Utf8ToKoi8r(names["genitive"].get<std::string>());
		}
		if (HasString(names, "dative"))
		{
			mob->player_data.PNames[ECase::kDat] = Utf8ToKoi8r(names["dative"].get<std::string>());
		}
		if (HasString(names, "accusative"))
		{
			mob->player_data.PNames[ECase::kAcc] = Utf8ToKoi8r(names["accusative"].get<std::string>());
		}
		if (HasString(names, "instrumental"))
		{
			mob->player_data.PNames[ECase::kIns] = Utf8ToKoi8r(names["instrumental"].get<std::string>());
		}
		if (HasString(names, "prepositional"))
		{
			mob->player_data.PNames[ECase::kPre] = Utf8ToKoi8r(names["prepositional"].get<std::string>());
		}
	}

	// === DESCRIPTIONS ===
	if (HasObject(data, "descriptions"))
	{
		const auto& descriptions = data["descriptions"];
		if (HasString(descriptions, "short_desc"))
		{
			mob->player_data.long_descr = Utf8ToKoi8r(descriptions["short_desc"].get<std::string>());
		}
		if (HasString(descriptions, "long_desc"))
		{
			mob->player_data.description = Utf8ToKoi8r(descriptions["long_desc"].get<std::string>());
		}
	}

	// === STATS ===
	if (HasObject(data, "stats"))
	{
		const auto& stats = data["stats"];

		if (HasNumber(stats, "level"))
		{
			mob->set_level(stats["level"].get<int>());
		}
		if (HasNumber(stats, "armor"))
		{
			mob->real_abils.armor = stats["armor"].get<int>();
		}
		if (HasNumber(stats, "hitroll_penalty"))
		{
			mob->real_abils.hitroll = stats["hitroll_penalty"].get<int>();
		}
		if (HasNumber(stats, "sex"))
		{
			mob->set_sex(static_cast<EGender>(stats["sex"].get<int>()));
		}
		if (HasNumber(stats, "race"))
		{
			mob->set_race(stats["race"].get<int>());
		}
		if (HasNumber(stats, "alignment"))
		{
			mob->char_specials.saved.alignment = stats["alignment"].get<int>();
		}

		// HP (dice format)
		if (HasObject(stats, "hp"))
		{
			const auto& hp = stats["hp"];
			if (HasNumber(hp, "dice_count"))
			{
				mob->mem_queue.total = hp["dice_count"].get<int>();
			}
			if (HasNumber(hp, "dice_size"))
			{
				mob->mem_queue.stored = hp["dice_size"].get<int>();
			}
			if (HasNumber(hp, "bonus"))
			{
				mob->set_hit(hp["bonus"].get<int>());
			}
		}

		// Damage (dice format)
		if (HasObject(stats, "damage"))
		{
			const auto& dmg = stats["damage"];
			if (HasNumber(dmg, "dice_count"))
			{
				mob->mob_specials.damnodice = dmg["dice_count"].get<int>();
			}
			if (HasNumber(dmg, "dice_size"))
			{
				mob->mob_specials.damsizedice = dmg["dice_size"].get<int>();
			}
			if (HasNumber(dmg, "bonus"))
			{
				mob->real_abils.damroll = dmg["bonus"].get<int>();
			}
		}
	}

	// === ABILITIES ===
	if (HasObject(data, "abilities"))
	{
		const auto& abilities = data["abilities"];
		if (HasNumber(abilities, "strength"))
		{
			mob->set_str(abilities["strength"].get<int>());
		}
		if (HasNumber(abilities, "dexterity"))
		{
			mob->set_dex(abilities["dexterity"].get<int>());
		}
		if (HasNumber(abilities, "constitution"))
		{
			mob->set_con(abilities["constitution"].get<int>());
		}
		if (HasNumber(abilities, "intelligence"))
		{
			mob->set_int(abilities["intelligence"].get<int>());
		}
		if (HasNumber(abilities, "wisdom"))
		{
			mob->set_wis(abilities["wisdom"].get<int>());
		}
		if (HasNumber(abilities, "charisma"))
		{
			mob->set_cha(abilities["charisma"].get<int>());
		}
	}

	// === PHYSICAL CHARACTERISTICS ===
	if (HasObject(data, "physical"))
	{
		const auto& physical = data["physical"];
		if (HasNumber(physical, "height"))
		{
			GET_HEIGHT(mob) = static_cast<ubyte>(physical["height"].get<int>());
		}
		if (HasNumber(physical, "weight"))
		{
			GET_WEIGHT(mob) = static_cast<ubyte>(physical["weight"].get<int>());
		}
		if (HasNumber(physical, "size"))
		{
			GET_SIZE(mob) = static_cast<sbyte>(physical["size"].get<int>());
		}
		if (HasNumber(physical, "extra_attack"))
		{
			mob->mob_specials.extra_attack = static_cast<byte>(physical["extra_attack"].get<int>());
		}
		if (HasNumber(physical, "like_work"))
		{
			mob->mob_specials.like_work = static_cast<byte>(physical["like_work"].get<int>());
		}
		if (HasNumber(physical, "maxfactor"))
		{
			mob->mob_specials.MaxFactor = physical["maxfactor"].get<int>();
		}
		if (HasNumber(physical, "remort"))
		{
			mob->set_remort(physical["remort"].get<int>());
		}
	}

	// === DEATH LOAD ===
	if (HasArray(data, "death_load"))
	{
		mob->dl_list.clear();
		for (const auto& dl_item : data["death_load"])
		{
			dead_load::LoadingItem item;
			item.obj_vnum = GetField(dl_item, "obj_vnum", 0);
			item.load_prob = GetField(dl_item, "load_prob", 0);
			item.load_type = GetField(dl_item, "load_type", 0);
			item.spec_param = GetField(dl_item, "spec_param", 0);
			if (item.obj_vnum > 0)
			{
				mob->dl_list.push_back(item);
			}
		}
	}

	// === NPC ROLES ===
	if (HasArray(data, "roles"))
	{
		CharData::role_t new_roles;
		for (const auto& role_idx : data["roles"])
		{
			if (role_idx.is_number_unsigned())
			{
				unsigned idx = role_idx.get<unsigned>();
				if (idx < new_roles.size())
				{
					new_roles.set(idx);
				}
			}
		}
		mob->set_role(new_roles);
	}

	// === RESISTANCES ===
	if (HasObject(data, "resistances"))
	{
		const auto& resistances = data["resistances"];
		if (HasNumber(resistances, "fire"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kFire)] = resistances["fire"].get<int>();
		}
		if (HasNumber(resistances, "air"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kAir)] = resistances["air"].get<int>();
		}
		if (HasNumber(resistances, "water"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kWater)] = resistances["water"].get<int>();
		}
		if (HasNumber(resistances, "earth"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kEarth)] = resistances["earth"].get<int>();
		}
		if (HasNumber(resistances, "vitality"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kVitality)] = resistances["vitality"].get<int>();
		}
		if (HasNumber(resistances, "mind"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kMind)] = resistances["mind"].get<int>();
		}
		if (HasNumber(resistances, "immunity"))
		{
			mob->add_abils.apply_resistance[to_underlying(EResist::kImmunity)] = resistances["immunity"].get<int>();
		}
	}

	// === SAVINGS ===
	if (HasObject(data, "savings"))
	{
		const auto& savings = data["savings"];
		if (HasNumber(savings, "will"))
		{
			mob->add_abils.apply_saving_throw[to_underlying(ESaving::kWill)] = savings["will"].get<int>();
		}
		if (HasNumber(savings, "stability"))
		{
			mob->add_abils.apply_saving_throw[to_underlying(ESaving::kStability)] = savings["stability"].get<int>();
		}
		if (HasNumber(savings, "reflex"))
		{
			mob->add_abils.apply_saving_throw[to_underlying(ESaving::kReflex)] = savings["reflex"].get<int>();
		}
	}

	// === POSITION ===
	if (HasObject(data, "position"))
	{
		const auto& position = data["position"];
		if (HasNumber(position, "default_position"))
		{
			mob->mob_specials.default_pos = static_cast<EPosition>(position["default_position"].get<int>());
		}
		if (HasNumber(position, "load_position"))
		{
			mob->SetPosition(static_cast<EPosition>(position["load_position"].get<int>()));
		}
	}

	// === BEHAVIOR ===
	if (HasObject(data, "behavior"))
	{
		const auto& behavior = data["behavior"];
		if (HasNumber(behavior, "class"))
		{
			mob->set_class(static_cast<ECharClass>(behavior["class"].get<int>()));
		}
		if (HasNumber(behavior, "attack_type"))
		{
			mob->mob_specials.attack_type = behavior["attack_type"].get<int>();
		}

		// Gold (dice format)
		if (HasObject(behavior, "gold"))
		{
			const auto& gold = behavior["gold"];
			if (HasNumber(gold, "dice_count"))
			{
				mob->mob_specials.GoldNoDs = gold["dice_count"].get<int>();
			}
			if (HasNumber(gold, "dice_size"))
			{
				mob->mob_specials.GoldSiDs = gold["dice_size"].get<int>();
			}
		}

		// Helpers (array of mob vnums) - not implemented yet
		if (HasArray(behavior, "helpers"))
		{
			// TODO: Parse helpers array and update mob->mob_specials.helper
		}
	}

	// === FLAGS ===
	if (HasObject(data, "flags"))
	{
		const auto& flags = data["flags"];

		// mob_flags array
		ParseFlags<EMobFlag>(flags, "mob_flags", mob->char_specials.saved.act);

		// npc_flags array
		ParseFlags<ENpcFlag>(flags, "npc_flags", mob->mob_specials.npc_flags);

		// affect_flags array
		ParseFlags<EAffect>(flags, "affect_flags", mob->char_specials.saved.affected_by);
	}
	// Legacy flat flags array (backward compatibility)
	else if (HasArray(data, "flags"))
	{
		ParseFlags<EMobFlag>(data, "flags", mob->char_specials.saved.act);
	}

	// Legacy npc_flags array at root level
	if (HasArray(data, "npc_flags"))
	{
		ParseFlags<ENpcFlag>(data, "npc_flags", mob->mob_specials.npc_flags);
	}

	// === TRIGGERS ===
	if (HasArray(data, "triggers"))
	{
		// Allocate proto_script if needed
		if (!mob->proto_script)
		{
			mob->proto_script = std::make_shared<std::list<int>>();
		}
		mob->proto_script->clear();

		for (const auto& trig_vnum : data["triggers"])
		{
			if (trig_vnum.is_number_integer())
			{
				mob->proto_script->push_back(trig_vnum.get<int>());
			}
		}
	}
}

// ============================================================================
// Object Parsing (stub for now)
// ============================================================================

void ParseObjectUpdate(CObjectPrototype* obj, const nlohmann::json& data)
{
	if (!obj)
	{
		return;
	}

	// Basic text fields
	if (HasString(data, "aliases"))
	{
		obj->set_aliases(Utf8ToKoi8r(data["aliases"].get<std::string>()));
	}
	if (HasString(data, "short_desc"))
	{
		obj->set_short_description(Utf8ToKoi8r(data["short_desc"].get<std::string>()));
	}
	if (HasString(data, "description"))
	{
		obj->set_description(Utf8ToKoi8r(data["description"].get<std::string>()));
	}
	if (HasString(data, "action_desc"))
	{
		obj->set_action_description(Utf8ToKoi8r(data["action_desc"].get<std::string>()));
	}

	// Grammatical cases (support both "pnames" and "names" keys)
	auto names_data = data.contains("names") ? data["names"] : (data.contains("pnames") ? data["pnames"] : nlohmann::json{});
	if (!names_data.is_null())
	{
		if (names_data.contains("nominative"))
		{
			obj->set_PName(ECase::kNom, Utf8ToKoi8r(names_data["nominative"].get<std::string>()));
		}
		if (names_data.contains("genitive"))
		{
			obj->set_PName(ECase::kGen, Utf8ToKoi8r(names_data["genitive"].get<std::string>()));
		}
		if (names_data.contains("dative"))
		{
			obj->set_PName(ECase::kDat, Utf8ToKoi8r(names_data["dative"].get<std::string>()));
		}
		if (names_data.contains("accusative"))
		{
			obj->set_PName(ECase::kAcc, Utf8ToKoi8r(names_data["accusative"].get<std::string>()));
		}
		if (names_data.contains("instrumental"))
		{
			obj->set_PName(ECase::kIns, Utf8ToKoi8r(names_data["instrumental"].get<std::string>()));
		}
		if (names_data.contains("prepositional"))
		{
			obj->set_PName(ECase::kPre, Utf8ToKoi8r(names_data["prepositional"].get<std::string>()));
		}
	}

	// Numeric properties
	if (data.contains("weight"))
	{
		obj->set_weight(data["weight"].get<int>());
	}
	if (data.contains("cost"))
	{
		obj->set_cost(data["cost"].get<int>());
	}
	if (data.contains("rent_on"))
	{
		obj->set_rent_on(data["rent_on"].get<int>());
	}
	if (data.contains("rent_off"))
	{
		obj->set_rent_off(data["rent_off"].get<int>());
	}

	// Type and material
	if (data.contains("type"))
	{
		obj->set_type(static_cast<EObjType>(data["type"].get<int>()));
	}
	if (data.contains("material"))
	{
		obj->set_material(static_cast<EObjMaterial>(data["material"].get<int>()));
	}

	// Sex
	if (data.contains("sex"))
	{
		obj->set_sex(static_cast<EGender>(data["sex"].get<int>()));
	}

	// Level (maps to minimum_remorts to match serializer)
	if (data.contains("level"))
	{
		obj->set_minimum_remorts(data["level"].get<int>());
	}

	// Timer
	if (data.contains("timer"))
	{
		obj->set_timer(data["timer"].get<int>());
	}

	// Values (support both "values" array and "type_specific" object)
	if (data.contains("type_specific") && data["type_specific"].is_object())
	{
		auto &ts = data["type_specific"];
		if (ts.contains("value0"))
		{
			obj->set_val(0, ts["value0"].get<int>());
		}
		if (ts.contains("value1"))
		{
			obj->set_val(1, ts["value1"].get<int>());
		}
		if (ts.contains("value2"))
		{
			obj->set_val(2, ts["value2"].get<int>());
		}
		if (ts.contains("value3"))
		{
			obj->set_val(3, ts["value3"].get<int>());
		}
	}
	else if (data.contains("values") && data["values"].is_array())
	{
		for (size_t i = 0; i < 4 && i < data["values"].size(); ++i)
		{
			obj->set_val(i, data["values"][i].get<int>());
		}
	}

	// Extra flags (array of 4 plane values)
	if (data.contains("extra_flags") && data["extra_flags"].is_array())
	{
		FlagData flags;
		for (size_t i = 0; i < 4 && i < data["extra_flags"].size(); ++i)
		{
			flags.set_plane(i, data["extra_flags"][i].get<Bitvector>());
		}
		obj->set_extra_flags(flags);
	}

	// Wear flags (single integer bitmask)
	if (data.contains("wear_flags"))
	{
		obj->set_wear_flags(data["wear_flags"].get<int>());
	}

	// Anti flags (array of 4 plane values)
	if (data.contains("anti_flags") && data["anti_flags"].is_array())
	{
		FlagData flags;
		for (size_t i = 0; i < 4 && i < data["anti_flags"].size(); ++i)
		{
			flags.set_plane(i, data["anti_flags"][i].get<Bitvector>());
		}
		obj->set_anti_flags(flags);
	}

	// No flags (array of 4 plane values)
	if (data.contains("no_flags") && data["no_flags"].is_array())
	{
		FlagData flags;
		for (size_t i = 0; i < 4 && i < data["no_flags"].size(); ++i)
		{
			flags.set_plane(i, data["no_flags"][i].get<Bitvector>());
		}
		obj->set_no_flags(flags);
	}

	// Affects
	if (data.contains("affects") && data["affects"].is_array())
	{
		size_t idx = 0;
		for (const auto &affect : data["affects"])
		{
			if (idx >= kMaxObjAffect)
			{
				break;
			}
			if (affect.contains("location") && affect.contains("modifier"))
			{
				obj->set_affected(idx,
					static_cast<EApply>(affect["location"].get<int>()),
					affect["modifier"].get<int>());
				++idx;
			}
		}
	}

	// Durability (support both object and separate fields)
	if (data.contains("durability") && data["durability"].is_object())
	{
		auto &dur = data["durability"];
		if (dur.contains("current"))
		{
			obj->set_current_durability(dur["current"].get<int>());
		}
		if (dur.contains("maximum"))
		{
			obj->set_maximum_durability(dur["maximum"].get<int>());
		}
	}
	else
	{
		if (data.contains("current_durability"))
		{
			obj->set_current_durability(data["current_durability"].get<int>());
		}
		if (data.contains("maximum_durability"))
		{
			obj->set_maximum_durability(data["maximum_durability"].get<int>());
		}
	}

	// Extra descriptions
	if (data.contains("extra_descriptions") && data["extra_descriptions"].is_array())
	{
		// Clear existing extra descriptions
		obj->set_ex_description(nullptr);

		// Build linked list from array (in reverse to maintain order)
		ExtraDescription::shared_ptr prev = nullptr;
		for (auto it = data["extra_descriptions"].rbegin(); it != data["extra_descriptions"].rend(); ++it)
		{
			const auto &ed_data = *it;
			if (!ed_data.contains("keywords") || !ed_data.contains("description"))
			{
				continue;
			}

			auto ed = std::make_shared<ExtraDescription>();
			ed->set_keyword(Utf8ToKoi8r(ed_data["keywords"].get<std::string>()));
			ed->set_description(Utf8ToKoi8r(ed_data["description"].get<std::string>()));
			ed->next = prev;
			prev = ed;
		}

		if (prev)
		{
			obj->set_ex_description(prev);
		}
	}

	// Triggers
	if (data.contains("triggers") && data["triggers"].is_array())
	{
		std::list<int> trigger_list;
		for (const auto &trig_vnum : data["triggers"])
		{
			if (trig_vnum.is_number_integer())
			{
				trigger_list.push_back(trig_vnum.get<int>());
			}
		}
		obj->set_proto_script(trigger_list);
	}
}

// ============================================================================
// Room Parsing (stub for now)
// ============================================================================

void ParseRoomUpdate(RoomData* room, const nlohmann::json& data)
{
	if (!room)
	{
		return;
	}

	if (HasString(data, "name"))
	{
		room->set_name(Utf8ToKoi8r(data["name"].get<std::string>()));
	}
	if (data.contains("sector_type"))
	{
		room->sector_type = static_cast<ESector>(data["sector_type"].get<int>());
	}
	if (data.contains("room_flags") && data["room_flags"].is_array())
	{
		FlagData flags;
		for (size_t i = 0; i < 4 && i < data["room_flags"].size(); ++i)
		{
			flags.set_plane(i, data["room_flags"][i].get<Bitvector>());
		}
		room->write_flags(flags);
	}

	// Description
	if (HasString(data, "description"))
	{
		if (room->temp_description)
		{
			free(room->temp_description);
		}
		room->temp_description = str_dup(Utf8ToKoi8r(data["description"].get<std::string>()).c_str());
	}

	// Exits (array format: [{direction: 0, to_room: 101, ...}, ...])
	if (data.contains("exits") && data["exits"].is_array())
	{
		// Clear all existing exits first
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
		{
			room->dir_option_proto[dir] = nullptr;
		}

		// Rebuild exits from JSON
		for (const auto &exit_json : data["exits"])
		{
			// Parse direction as number (0-5)
			if (!exit_json.contains("direction")) continue;
			int dir = exit_json["direction"].get<int>();
			if (dir < 0 || dir >= EDirection::kMaxDirNum) continue;

			// Create exit
			room->dir_option_proto[dir] = std::make_shared<ExitData>();
			auto &exit = room->dir_option_proto[dir];

			// Update exit fields
			if (exit_json.contains("to_room"))
			{
				int target_vnum = exit_json["to_room"].get<int>();
				RoomRnum target_rnum = GetRoomRnum(target_vnum);
				exit->to_room(target_rnum);
			}
			if (exit_json.contains("general_description"))
			{
				exit->general_description = Utf8ToKoi8r(exit_json["general_description"].get<std::string>());
			}
			if (exit_json.contains("keyword"))
			{
				exit->set_keyword(Utf8ToKoi8r(exit_json["keyword"].get<std::string>()));
			}
			if (exit_json.contains("exit_info"))
			{
				exit->exit_info = exit_json["exit_info"].get<int>();
			}
			if (exit_json.contains("key"))
			{
				exit->key = exit_json["key"].get<int>();
			}
			if (exit_json.contains("lock_complexity"))
			{
				exit->lock_complexity = exit_json["lock_complexity"].get<int>();
			}
		}
	}

	// Triggers (array of trigger vnums)
	if (data.contains("triggers") && data["triggers"].is_array())
	{
		room->proto_script->clear();
		for (const auto &trigger_vnum : data["triggers"])
		{
			if (trigger_vnum.is_number_integer())
			{
				room->proto_script->push_back(trigger_vnum.get<int>());
			}
		}
	}
}

// ============================================================================
// Zone Parsing (stub for now)
// ============================================================================

void ParseZoneUpdate(ZoneData* zone, const nlohmann::json& data)
{
	if (!zone)
	{
		return;
	}

	if (HasString(data, "name"))
	{
		zone->name = Utf8ToKoi8r(data["name"].get<std::string>());
	}
	if (HasString(data, "comment"))
	{
		zone->comment = Utf8ToKoi8r(data["comment"].get<std::string>());
	}
	if (HasString(data, "author"))
	{
		zone->author = Utf8ToKoi8r(data["author"].get<std::string>());
	}
	if (HasString(data, "location"))
	{
		zone->location = Utf8ToKoi8r(data["location"].get<std::string>());
	}
	if (HasString(data, "description"))
	{
		zone->description = Utf8ToKoi8r(data["description"].get<std::string>());
	}
	if (data.contains("level"))
	{
		zone->level = data["level"].get<int>();
	}
	if (data.contains("type"))
	{
		zone->type = data["type"].get<int>();
	}
	if (data.contains("lifespan"))
	{
		zone->lifespan = data["lifespan"].get<int>();
	}
	if (data.contains("reset_mode"))
	{
		zone->reset_mode = data["reset_mode"].get<int>();
	}
	if (data.contains("reset_idle"))
	{
		zone->reset_idle = data["reset_idle"].get<bool>();
	}
	if (data.contains("top"))
	{
		zone->top = data["top"].get<int>();
	}
	if (data.contains("under_construction"))
	{
		zone->under_construction = data["under_construction"].get<int>();
	}
	if (data.contains("group"))
	{
		zone->group = data["group"].get<int>();
	}
	if (data.contains("is_town"))
	{
		zone->is_town = data["is_town"].get<bool>();
	}
	if (data.contains("locked"))
	{
		zone->locked = data["locked"].get<bool>();
	}
}

// ============================================================================
// Trigger Parsing
// ============================================================================

void ParseTriggerUpdate(Trigger* trig, const nlohmann::json& data)
{
	if (!trig)
	{
		return;
	}

	// Basic text fields
	if (HasString(data, "name"))
	{
		trig->set_name(Utf8ToKoi8r(data["name"].get<std::string>()));
	}
	if (HasString(data, "arglist"))
	{
		trig->arglist = Utf8ToKoi8r(data["arglist"].get<std::string>());
	}

	// Numeric fields
	if (data.contains("narg"))
	{
		trig->narg = data["narg"].get<int>();
	}
	if (data.contains("trigger_type"))
	{
		trig->set_trigger_type(data["trigger_type"].get<long>());
	}
	if (data.contains("attach_type"))
	{
		trig->set_attach_type(static_cast<byte>(data["attach_type"].get<int>()));
	}

	// Boolean flags
	if (data.contains("add_flag"))
	{
		trig->add_flag = data["add_flag"].get<bool>();
	}

	// Note: Script handling is done in the handler via temp_d->olc->storage
	// because it requires OLC-specific processing (cmdlist parsing)
}

}  // namespace admin_api::parsers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
