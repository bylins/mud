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
	// TODO: Extract from admin_api_update_object
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
	// TODO: Extract from admin_api_update_room
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
	// TODO: Extract from admin_api_update_zone
}

// ============================================================================
// Trigger Parsing (stub for now)
// ============================================================================

void ParseTriggerUpdate(Trigger* trig, const nlohmann::json& data)
{
	if (!trig)
	{
		return;
	}
	// TODO: Extract from admin_api_update_trigger
}

}  // namespace admin_api::parsers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
