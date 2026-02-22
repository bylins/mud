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
	obj_data["aliases"] = Koi8rToUtf8(obj.get_aliases());
	obj_data["short_desc"] = Koi8rToUtf8(obj.get_short_description());
	obj_data["description"] = Koi8rToUtf8(obj.get_description());
	obj_data["action_desc"] = Koi8rToUtf8(obj.get_action_description());
	obj_data["type"] = static_cast<int>(obj.get_type());

	// Extra flags (4 planes)
	obj_data["extra_flags"] = json::array();
	for (size_t i = 0; i < 4; ++i)
	{
		obj_data["extra_flags"].push_back(obj.get_extra_flags().get_plane(i));
	}

	obj_data["wear_flags"] = obj.get_wear_flags();

	// Anti flags (4 planes)
	obj_data["anti_flags"] = json::array();
	for (size_t i = 0; i < 4; ++i)
	{
		obj_data["anti_flags"].push_back(obj.get_anti_flags().get_plane(i));
	}

	// No flags (4 planes)
	obj_data["no_flags"] = json::array();
	for (size_t i = 0; i < 4; ++i)
	{
		obj_data["no_flags"].push_back(obj.get_no_flags().get_plane(i));
	}

	obj_data["weight"] = obj.get_weight();
	obj_data["cost"] = obj.get_cost();
	obj_data["rent_on"] = obj.get_rent_on();
	obj_data["rent_off"] = obj.get_rent_off();

	// Names (7 case forms)
	json names;
	for (int c = ECase::kFirstCase; c <= ECase::kLastCase; ++c)
	{
		ECase ecase = static_cast<ECase>(c);
		std::string case_name;
		switch (ecase)
		{
			case ECase::kNom: case_name = "nominative"; break;
			case ECase::kGen: case_name = "genitive"; break;
			case ECase::kDat: case_name = "dative"; break;
			case ECase::kAcc: case_name = "accusative"; break;
			case ECase::kIns: case_name = "instrumental"; break;
			case ECase::kPre: case_name = "prepositional"; break;
			default: continue;
		}
		names[case_name] = Koi8rToUtf8(obj.get_PName(ecase));
	}
	obj_data["names"] = names;

	// Additional stats
	obj_data["level"] = obj.get_minimum_remorts();
	obj_data["sex"] = static_cast<int>(obj.get_sex());
	obj_data["material"] = static_cast<int>(obj.get_material());
	obj_data["timer"] = obj.get_timer();

	// Durability
	json durability;
	durability["current"] = obj.get_current_durability();
	durability["maximum"] = obj.get_maximum_durability();
	obj_data["durability"] = durability;

	// Type-specific values (value0-3)
	json type_specific;
	type_specific["value0"] = obj.get_val(0);
	type_specific["value1"] = obj.get_val(1);
	type_specific["value2"] = obj.get_val(2);
	type_specific["value3"] = obj.get_val(3);
	obj_data["type_specific"] = type_specific;

	// Affects (stat modifiers)
	json affects = json::array();
	for (int i = 0; i < kMaxObjAffect; ++i)
	{
		if (obj.get_affected(i).location != EApply::kNone)
		{
			json affect;
			affect["location"] = static_cast<int>(obj.get_affected(i).location);
			affect["modifier"] = obj.get_affected(i).modifier;
			affects.push_back(affect);
		}
	}
	obj_data["affects"] = affects;

	// Extra descriptions (linked list)
	json extra_descs = json::array();
	for (auto ed = obj.get_ex_description(); ed; ed = ed->next)
	{
		json extra;
		extra["keywords"] = Koi8rToUtf8(ed->keyword ? ed->keyword : "");
		extra["description"] = Koi8rToUtf8(ed->description ? ed->description : "");
		extra_descs.push_back(extra);
	}
	obj_data["extra_descriptions"] = extra_descs;

	// Triggers
	json triggers = json::array();
	for (const auto &trig_vnum : obj.get_proto_script())
	{
		triggers.push_back(trig_vnum);
	}
	obj_data["triggers"] = triggers;

	return obj_data;
}

// ============================================================================
// Room Serialization (stub for now)
// ============================================================================

json SerializeRoom(RoomData& room, int vnum)
{
	json room_data;
	room_data["vnum"] = vnum;
	room_data["name"] = Koi8rToUtf8(room.name);

	// Description stored in GlobalObjects::descriptions()
	if (room.temp_description)
	{
		room_data["description"] = Koi8rToUtf8(room.temp_description);
	}
	room_data["sector_type"] = static_cast<int>(room.sector_type);

	// Room flags (4 planes)
	{
		FlagData fl = room.read_flags();
		room_data["room_flags"] = json::array();
		for (size_t i = 0; i < 4; ++i)
		{
			room_data["room_flags"].push_back(fl.get_plane(i));
		}
	}

	// Exits
	json exits = json::array();
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
	{
		if (room.dir_option[dir])
		{
			json exit_obj;
			exit_obj["direction"] = dir;
			exit_obj["to_room"] = room.dir_option[dir]->to_room() != kNowhere ?
				world[room.dir_option[dir]->to_room()]->vnum : -1;
			if (!room.dir_option[dir]->general_description.empty())
			{
				exit_obj["description"] = Koi8rToUtf8(room.dir_option[dir]->general_description);
			}
			if (room.dir_option[dir]->keyword)
			{
				exit_obj["keyword"] = Koi8rToUtf8(room.dir_option[dir]->keyword);
			}
			exit_obj["exit_info"] = room.dir_option[dir]->exit_info;
			exit_obj["key_vnum"] = room.dir_option[dir]->key;
			exits.push_back(exit_obj);
		}
	}
	room_data["exits"] = exits;

	// Extra descriptions
	json extra_descrs = json::array();
	for (auto ed = room.ex_description; ed; ed = ed->next)
	{
		json ed_obj;
		if (ed->keyword)
		{
			ed_obj["keyword"] = Koi8rToUtf8(ed->keyword);
		}
		if (ed->description)
		{
			ed_obj["description"] = Koi8rToUtf8(ed->description);
		}
		extra_descrs.push_back(ed_obj);
	}
	room_data["extra_descriptions"] = extra_descrs;

	// Triggers
	json triggers = json::array();
	if (room.proto_script && !room.proto_script->empty())
	{
		for (const auto &trig_vnum : *room.proto_script)
		{
			triggers.push_back(trig_vnum);
		}
	}
	room_data["triggers"] = triggers;

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

// ============================================================================
// Trigger Serialization (stub for now)
// ============================================================================

json SerializeTrigger(const Trigger& trig, int vnum)
{
	json trig_data;
	trig_data["vnum"] = vnum;
	trig_data["name"] = Koi8rToUtf8(trig.get_name());
	trig_data["attach_type"] = static_cast<int>(trig.get_attach_type());
	trig_data["trigger_type"] = trig.get_trigger_type();
	trig_data["narg"] = trig.narg;
	trig_data["add_flag"] = trig.add_flag;

	// Argument
	if (!trig.arglist.empty())
	{
		trig_data["arglist"] = Koi8rToUtf8(trig.arglist);
	}

	// Command list
	json commands = json::array();
	if (trig.cmdlist)
	{
		auto cmd = *trig.cmdlist;
		while (cmd)
		{
			commands.push_back(Koi8rToUtf8(cmd->cmd));
			cmd = cmd->next;
		}
	}
	trig_data["commands"] = commands;

	return trig_data;
}

}  // namespace admin_api::serializers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
