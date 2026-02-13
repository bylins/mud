/**
 \file serializers.cpp
 \brief Entity to JSON serialization implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "serializers.h"
#include "json_helpers.h"
#include "../../../engine/structs/structs.h"
#include "../../../engine/db/obj_prototypes.h"
#include "../../../engine/entities/room_data.h"
#include "../../../engine/scripting/dg_scripts.h"
#include "../../../gameplay/core/constants.h"

namespace admin_api::serializers {

using namespace admin_api::json;

// ============================================================================
// Mob Serialization
// ============================================================================

json SerializeMob(const CharData& mob, int vnum)
{
	json mob_obj;
	mob_obj["vnum"] = vnum;

	// Names (all 6 Russian cases + aliases)
	json names;
	names["aliases"] = Koi8rToUtf8(mob.get_npc_name());
	names["nominative"] = Koi8rToUtf8(mob.player_data.PNames[ECase::kNom]);
	names["genitive"] = Koi8rToUtf8(mob.player_data.PNames[ECase::kGen]);
	names["dative"] = Koi8rToUtf8(mob.player_data.PNames[ECase::kDat]);
	names["accusative"] = Koi8rToUtf8(mob.player_data.PNames[ECase::kAcc]);
	names["instrumental"] = Koi8rToUtf8(mob.player_data.PNames[ECase::kIns]);
	names["prepositional"] = Koi8rToUtf8(mob.player_data.PNames[ECase::kPre]);
	mob_obj["names"] = names;

	// Descriptions
	json descriptions;
	descriptions["short_desc"] = Koi8rToUtf8(mob.player_data.long_descr);
	descriptions["long_desc"] = Koi8rToUtf8(mob.player_data.description);
	mob_obj["descriptions"] = descriptions;

	// Stats (level, HP, damage, etc.)
	json stats;
	stats["level"] = mob.GetLevel();
	stats["hitroll_penalty"] = GET_HR(&mob);
	stats["armor"] = GET_AC(&mob);

	// HP (dice format)
	json hp;
	hp["dice_count"] = static_cast<int>(mob.mem_queue.total);
	hp["dice_size"] = static_cast<int>(mob.mem_queue.stored);
	hp["bonus"] = mob.get_hit();
	stats["hp"] = hp;

	// Damage (dice format)
	json damage;
	damage["dice_count"] = static_cast<int>(mob.mob_specials.damnodice);
	damage["dice_size"] = static_cast<int>(mob.mob_specials.damsizedice);
	damage["bonus"] = mob.real_abils.damroll;
	stats["damage"] = damage;

	// Sex, race, alignment
	stats["sex"] = static_cast<int>(mob.get_sex());
	stats["race"] = mob.get_race();
	stats["alignment"] = mob.char_specials.saved.alignment;

	mob_obj["stats"] = stats;

	// Physical characteristics
	json physical;
	physical["height"] = static_cast<int>(GET_HEIGHT(&mob));
	physical["weight"] = static_cast<int>(GET_WEIGHT(&mob));
	physical["size"] = static_cast<int>(GET_SIZE(&mob));
	physical["extra_attack"] = static_cast<int>(mob.mob_specials.extra_attack);
	physical["like_work"] = static_cast<int>(mob.mob_specials.like_work);
	physical["maxfactor"] = mob.mob_specials.MaxFactor;
	physical["remort"] = mob.get_remort();
	mob_obj["physical"] = physical;

	// Death load (items dropped on death)
	json death_load_arr = json::array();
	for (const auto& item : mob.dl_list)
	{
		json dl_item;
		dl_item["obj_vnum"] = item.obj_vnum;
		dl_item["load_prob"] = item.load_prob;
		dl_item["load_type"] = item.load_type;
		dl_item["spec_param"] = item.spec_param;
		death_load_arr.push_back(dl_item);
	}
	mob_obj["death_load"] = death_load_arr;

	// NPC Roles (bitset<9>)
	json roles = json::array();
	for (unsigned i = 0; i < mob.get_role_bits().size(); ++i)
	{
		if (mob.get_role(i))
		{
			roles.push_back(static_cast<int>(i));
		}
	}
	mob_obj["roles"] = roles;

	// Abilities (6 core stats)
	json abilities;
	abilities["strength"] = mob.get_str();
	abilities["dexterity"] = mob.get_dex();
	abilities["constitution"] = mob.get_con();
	abilities["intelligence"] = mob.get_int();
	abilities["wisdom"] = mob.get_wis();
	abilities["charisma"] = mob.get_cha();
	mob_obj["abilities"] = abilities;

	// Resistances (7 types)
	json resistances;
	resistances["fire"] = mob.add_abils.apply_resistance[to_underlying(EResist::kFire)];
	resistances["air"] = mob.add_abils.apply_resistance[to_underlying(EResist::kAir)];
	resistances["water"] = mob.add_abils.apply_resistance[to_underlying(EResist::kWater)];
	resistances["earth"] = mob.add_abils.apply_resistance[to_underlying(EResist::kEarth)];
	resistances["vitality"] = mob.add_abils.apply_resistance[to_underlying(EResist::kVitality)];
	resistances["mind"] = mob.add_abils.apply_resistance[to_underlying(EResist::kMind)];
	resistances["immunity"] = mob.add_abils.apply_resistance[to_underlying(EResist::kImmunity)];
	mob_obj["resistances"] = resistances;

	// Savings (3 types)
	json savings;
	savings["will"] = mob.add_abils.apply_saving_throw[to_underlying(ESaving::kWill)];
	savings["stability"] = mob.add_abils.apply_saving_throw[to_underlying(ESaving::kStability)];
	savings["reflex"] = mob.add_abils.apply_saving_throw[to_underlying(ESaving::kReflex)];
	mob_obj["savings"] = savings;

	// Position
	json position;
	position["default_position"] = static_cast<int>(mob.mob_specials.default_pos);
	position["load_position"] = static_cast<int>(mob.GetPosition());
	mob_obj["position"] = position;

	// Behavior
	json behavior;
	behavior["class"] = static_cast<int>(mob.GetClass());
	behavior["attack_type"] = mob.mob_specials.attack_type;

	// Gold (dice format)
	json gold;
	gold["dice_count"] = mob.mob_specials.GoldNoDs;
	gold["dice_size"] = mob.mob_specials.GoldSiDs;
	gold["bonus"] = 0;  // No bonus field in mob_specials
	behavior["gold"] = gold;

	// Helpers (array of mob vnums) - empty for now
	behavior["helpers"] = json::array();

	mob_obj["behavior"] = behavior;

	// Flags - use SerializeFlags helper
	json flags;
	flags["mob_flags"] = SerializeFlags(mob.char_specials.saved.act);
	flags["affect_flags"] = SerializeFlags(mob.char_specials.saved.affected_by);
	flags["npc_flags"] = SerializeFlags(mob.mob_specials.npc_flags);
	mob_obj["flags"] = flags;

	// Triggers
	if (mob.proto_script)
	{
		json triggers = json::array();
		for (const auto& trig_vnum : *mob.proto_script)
		{
			triggers.push_back(trig_vnum);
		}
		mob_obj["triggers"] = triggers;
	}
	else
	{
		mob_obj["triggers"] = json::array();
	}

	return mob_obj;
}

// ============================================================================
// Object Serialization (stub for now)
// ============================================================================

json SerializeObject(const CObjectPrototype& obj, int vnum)
{
	json obj_data;
	obj_data["vnum"] = vnum;
	// TODO: Extract from admin_api_get_object
	return obj_data;
}

// ============================================================================
// Room Serialization (stub for now)
// ============================================================================

json SerializeRoom(const RoomData& room, int vnum)
{
	json room_data;
	room_data["vnum"] = vnum;
	// TODO: Extract from admin_api_get_room
	return room_data;
}

// ============================================================================
// ZoneData Serialization (stub for now)
// ============================================================================

json SerializeZoneData(const ZoneData& zone, int vnum)
{
	json zone_data;
	zone_data["vnum"] = vnum;
	zone_data["name"] = Koi8rToUtf8(zone.name);
	
	if (!zone.comment.empty()) {
		zone_data["comment"] = Koi8rToUtf8(zone.comment);
	}
	if (!zone.author.empty()) {
		zone_data["author"] = Koi8rToUtf8(zone.author);
	}
	if (!zone.location.empty()) {
		zone_data["location"] = Koi8rToUtf8(zone.location);
	}
	if (!zone.description.empty()) {
		zone_data["description"] = Koi8rToUtf8(zone.description);
	}
	
	zone_data["level"] = zone.level;
	zone_data["type"] = zone.type;
	zone_data["lifespan"] = zone.lifespan;
	zone_data["reset_mode"] = zone.reset_mode;
	zone_data["reset_idle"] = zone.reset_idle;
	zone_data["top"] = zone.top;
	zone_data["under_construction"] = zone.under_construction;
	zone_data["group"] = zone.group;
	zone_data["is_town"] = zone.is_town;
	zone_data["locked"] = zone.locked;
	
	if (zone.typeA_count > 0 && zone.typeA_list) {
		json typeA = json::array();
		for (int i = 0; i < zone.typeA_count; ++i) {
			typeA.push_back(zone.typeA_list[i]);
		}
		zone_data["type_a_list"] = typeA;
	}
	if (zone.typeB_count > 0 && zone.typeB_list) {
		json typeB = json::array();
		for (int i = 0; i < zone.typeB_count; ++i) {
			typeB.push_back(zone.typeB_list[i]);
		}
		zone_data["type_b_list"] = typeB;
	}
	
	return zone_data;
}
}

// ============================================================================
// Trigger Serialization (stub for now)
// ============================================================================

json SerializeTrigger(const Trigger& trig, int vnum)
{
	json trig_data;
	trig_data["vnum"] = vnum;
	// TODO: Extract from admin_api_get_trigger
	return trig_data;
}

}  // namespace admin_api::serializers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
