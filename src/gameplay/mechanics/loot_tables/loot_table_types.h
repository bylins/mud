//
// Created for Bylins MUD - Loot Tables Module
//

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLE_TYPES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLE_TYPES_H_

#include "engine/structs/structs.h"
#include "engine/structs/flag_data.h"

#include <string>
#include <vector>
#include <optional>

class CharData;

namespace loot_tables {

// Source type for loot generation
enum class ELootSourceType {
	kMobDeath,      // Loot from mob death (corpse)
	kContainer,     // Loot from container opening
	kQuest          // Loot from quest completion
};

// Table type determines how entries are organized
enum class ELootTableType {
	kSourceCentric,  // Table is attached to sources (mobs), entries are items
	kItemCentric     // Table is attached to items, entries are sources where they drop
};

// Filter type for matching sources
enum class ELootFilterType {
	kVnum,           // Match by exact vnum
	kLevelRange,     // Match by mob level range
	kRace,           // Match by mob race
	kClass,          // Match by mob class
	kFlags,          // Match by mob flags (NPC flags)
	kZone            // Match by zone vnum
};

// Entry type in loot table
enum class ELootEntryType {
	kItem,           // Generate an item
	kCurrency,       // Generate currency (gold, glory, etc.)
	kNestedTable     // Reference to another loot table
};

// Currency identifiers
namespace currency {
constexpr int kGold = 0;
constexpr int kGlory = 1;
constexpr int kIce = 2;
constexpr int kNogata = 3;
} // namespace currency

// Filter for matching loot sources (mobs, containers, etc.)
struct LootFilter {
	ELootFilterType type{ELootFilterType::kVnum};

	// For kVnum filter
	ObjVnum vnum{0};

	// For kLevelRange filter
	int min_level{0};
	int max_level{0};

	// For kRace filter
	int race{0};

	// For kClass filter
	int mob_class{0};

	// For kFlags filter
	FlagData flags;

	// For kZone filter
	ZoneVnum zone_vnum{0};

	// Check if this filter matches the given mob
	[[nodiscard]] bool Matches(const CharData *mob) const;

	// Create filter from vnum
	static LootFilter FromVnum(ObjVnum vnum);

	// Create filter from level range
	static LootFilter FromLevelRange(int min_lvl, int max_lvl);

	// Create filter from zone
	static LootFilter FromZone(ZoneVnum zone);

	// Create filter from race
	static LootFilter FromRace(int race);
};

// Count range for item generation
struct CountRange {
	int min{1};
	int max{1};

	CountRange() = default;
	CountRange(int min_val, int max_val) : min(min_val), max(max_val) {}

	// Roll random count in range [min, max]
	[[nodiscard]] int Roll() const;
};

// Player-specific filters for loot entries
struct PlayerFilters {
	// Level restrictions
	int min_player_level{0};
	int max_player_level{0};  // 0 = no limit

	// Class restrictions (empty = all classes)
	std::vector<int> allowed_classes;

	// Check if player matches these filters
	[[nodiscard]] bool Matches(const CharData *player) const;

	// Check if filters are empty (no restrictions)
	[[nodiscard]] bool IsEmpty() const;
};

// Single entry in a loot table
struct LootEntry {
	ELootEntryType type{ELootEntryType::kItem};

	// For kItem type
	ObjVnum item_vnum{0};

	// For kCurrency type
	int currency_id{currency::kGold};

	// For kNestedTable type
	std::string nested_table_id;

	// Generation parameters
	CountRange count;
	int probability_weight{10000};    // Primary weight (out of probability_width)
	int fallback_weight{0};           // Secondary weight when MIW limit reached
	bool allow_duplicates{false};     // Can this entry drop multiple times per generation?

	// Player-specific filters
	PlayerFilters player_filters;

	// Check if entry can drop for the given player
	[[nodiscard]] bool CheckPlayerFilters(const CharData *player) const;

	// Roll random count for this entry
	[[nodiscard]] int RollCount() const;

	// Create item entry
	static LootEntry Item(ObjVnum vnum, int weight, CountRange count = {1, 1});

	// Create currency entry
	static LootEntry Currency(int currency_id, int weight, CountRange count);

	// Create nested table entry
	static LootEntry NestedTable(const std::string &table_id, int weight);
};

} // namespace loot_tables

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLE_TYPES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
