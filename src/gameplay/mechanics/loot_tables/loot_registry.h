//
// Created for Bylins MUD - Loot Tables Module
//

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_REGISTRY_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_REGISTRY_H_

#include "loot_table.h"

#include <unordered_map>
#include <map>
#include <mutex>

namespace loot_tables {

// Statistics for loot generation
struct LootStats {
	size_t total_generations{0};
	size_t total_items_generated{0};
	std::map<ObjVnum, size_t> item_drop_counts;
	std::map<std::string, size_t> table_usage_counts;

	void Reset();
};

// Registry for all loot tables
class LootTablesRegistry {
 public:
	using TablePtr = LootTable::Ptr;
	using TableConstPtr = LootTable::ConstPtr;

	LootTablesRegistry() = default;

	// Register a table
	void RegisterTable(TablePtr table);

	// Unregister a table by ID
	void UnregisterTable(const std::string &id);

	// Clear all tables
	void Clear();

	// Get table by ID
	[[nodiscard]] TableConstPtr GetTable(const std::string &id) const;

	// Get mutable table by ID (for editing)
	[[nodiscard]] TablePtr GetMutableTable(const std::string &id);

	// Get all tables
	[[nodiscard]] const std::unordered_map<std::string, TablePtr> &GetAllTables() const;

	// Get table count
	[[nodiscard]] size_t GetTableCount() const { return all_tables_.size(); }

	// Find all tables applicable to the given mob
	[[nodiscard]] std::vector<TableConstPtr> FindTablesForMob(const CharData *mob) const;

	// Generate loot from all applicable tables for a mob
	[[nodiscard]] GeneratedLoot GenerateFromMob(
		const CharData *mob,
		const CharData *killer,
		int luck_bonus = 0) const;

	// Build/rebuild indices for fast lookup
	void BuildIndices();

	// Validate all tables (check for cycles, invalid vnums, etc.)
	[[nodiscard]] std::vector<std::string> Validate() const;

	// Get statistics
	[[nodiscard]] const LootStats &GetStats() const { return stats_; }

	// Reset statistics
	void ResetStats() { stats_.Reset(); }

	// Set MIW check function
	void SetMiwCheckFunc(LootTable::MiwCheckFunc func) { miw_check_func_ = std::move(func); }

 private:
	// All tables by ID
	std::unordered_map<std::string, TablePtr> all_tables_;

	// Index by mob vnum (for SOURCE_CENTRIC tables with kVnum filter)
	std::multimap<ObjVnum, TablePtr> mob_vnum_index_;

	// Index by zone vnum (for SOURCE_CENTRIC tables with kZone filter)
	std::multimap<ZoneVnum, TablePtr> zone_index_;

	// Tables with complex filters (level range, race, etc.)
	std::vector<TablePtr> filtered_tables_;

	// MIW check function
	LootTable::MiwCheckFunc miw_check_func_;

	// Statistics
	mutable LootStats stats_;

	// Check for cycles in nested tables
	[[nodiscard]] bool HasCycle(const std::string &table_id,
								std::unordered_set<std::string> &visited,
								std::unordered_set<std::string> &in_path) const;
};

} // namespace loot_tables

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_REGISTRY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
