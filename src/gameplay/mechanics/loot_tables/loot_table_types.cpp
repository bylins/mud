//
// Created for Bylins MUD - Loot Tables Module
//

#include "loot_table_types.h"

#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include "engine/db/db.h"
#include "utils/utils.h"
#include "utils/random.h"

namespace loot_tables {

bool LootFilter::Matches(const CharData *mob) const {
	if (!mob || !mob->IsNpc()) {
		return false;
	}

	switch (type) {
		case ELootFilterType::kVnum:
			return GET_MOB_VNUM(mob) == vnum;

		case ELootFilterType::kLevelRange: {
			int mob_level = GetRealLevel(mob);
			if (min_level > 0 && mob_level < min_level) {
				return false;
			}
			if (max_level > 0 && mob_level > max_level) {
				return false;
			}
			return true;
		}

		case ELootFilterType::kRace:
			return mob->get_race() == race;

		case ELootFilterType::kClass:
			return static_cast<int>(mob->GetClass()) == mob_class;

		case ELootFilterType::kFlags:
			return false; // TODO: implement flags matching

		case ELootFilterType::kZone:
			return GetZoneVnumByCharPlace(const_cast<CharData*>(mob)) == zone_vnum;
	}

	return false;
}

LootFilter LootFilter::FromVnum(ObjVnum obj_vnum) {
	LootFilter filter;
	filter.type = ELootFilterType::kVnum;
	filter.vnum = obj_vnum;
	return filter;
}

LootFilter LootFilter::FromLevelRange(int min_lvl, int max_lvl) {
	LootFilter filter;
	filter.type = ELootFilterType::kLevelRange;
	filter.min_level = min_lvl;
	filter.max_level = max_lvl;
	return filter;
}

LootFilter LootFilter::FromZone(ZoneVnum zone) {
	LootFilter filter;
	filter.type = ELootFilterType::kZone;
	filter.zone_vnum = zone;
	return filter;
}

LootFilter LootFilter::FromRace(int race_id) {
	LootFilter filter;
	filter.type = ELootFilterType::kRace;
	filter.race = race_id;
	return filter;
}

int CountRange::Roll() const {
	if (min >= max) {
		return min;
	}
	return number(min, max);
}

bool PlayerFilters::Matches(const CharData *player) const {
	if (!player || player->IsNpc()) {
		return true;  // No player = no restrictions
	}

	// Check level
	int player_level = GetRealLevel(player);
	if (min_player_level > 0 && player_level < min_player_level) {
		return false;
	}
	if (max_player_level > 0 && player_level > max_player_level) {
		return false;
	}

	// Check class
	if (!allowed_classes.empty()) {
		int player_class = to_underlying(player->GetClass());
		bool class_found = false;
		for (int allowed_class : allowed_classes) {
			if (allowed_class == player_class) {
				class_found = true;
				break;
			}
		}
		if (!class_found) {
			return false;
		}
	}

	return true;
}

bool PlayerFilters::IsEmpty() const {
	return min_player_level == 0
		&& max_player_level == 0
		&& allowed_classes.empty();
}

bool LootEntry::CheckPlayerFilters(const CharData *player) const {
	return player_filters.Matches(player);
}

int LootEntry::RollCount() const {
	return count.Roll();
}

LootEntry LootEntry::Item(ObjVnum vnum, int weight, CountRange cnt) {
	LootEntry entry;
	entry.type = ELootEntryType::kItem;
	entry.item_vnum = vnum;
	entry.probability_weight = weight;
	entry.count = cnt;
	return entry;
}

LootEntry LootEntry::Currency(int curr_id, int weight, CountRange cnt) {
	LootEntry entry;
	entry.type = ELootEntryType::kCurrency;
	entry.currency_id = curr_id;
	entry.probability_weight = weight;
	entry.count = cnt;
	return entry;
}

LootEntry LootEntry::NestedTable(const std::string &table_id, int weight) {
	LootEntry entry;
	entry.type = ELootEntryType::kNestedTable;
	entry.nested_table_id = table_id;
	entry.probability_weight = weight;
	return entry;
}

} // namespace loot_tables

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
