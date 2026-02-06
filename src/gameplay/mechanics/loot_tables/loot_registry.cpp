//
// Created for Bylins MUD - Loot Tables Module
//

#include "loot_registry.h"
#include "loot_loader.h"

#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include "engine/db/db.h"
#include "utils/utils.h"
#include "utils/logger.h"

#include <algorithm>
#include <unordered_set>

namespace loot_tables {

void LootStats::Reset() {
	total_generations = 0;
	total_items_generated = 0;
	item_drop_counts.clear();
	table_usage_counts.clear();
}

void LootTablesRegistry::RegisterTable(TablePtr table) {
	if (!table) {
		return;
	}

	const std::string &id = table->GetId();
	if (id.empty()) {
		log("LOOT_TABLES: Attempted to register table with empty ID");
		return;
	}

	all_tables_[id] = table;
}

void LootTablesRegistry::UnregisterTable(const std::string &id) {
	all_tables_.erase(id);
}

void LootTablesRegistry::Clear() {
	all_tables_.clear();
	mob_vnum_index_.clear();
	zone_index_.clear();
	filtered_tables_.clear();
}

LootTablesRegistry::TableConstPtr LootTablesRegistry::GetTable(const std::string &id) const {
	auto it = all_tables_.find(id);
	if (it != all_tables_.end()) {
		return it->second;
	}
	return nullptr;
}

LootTablesRegistry::TablePtr LootTablesRegistry::GetMutableTable(const std::string &id) {
	auto it = all_tables_.find(id);
	if (it != all_tables_.end()) {
		return it->second;
	}
	return nullptr;
}

const std::unordered_map<std::string, LootTablesRegistry::TablePtr> &
LootTablesRegistry::GetAllTables() const {
	return all_tables_;
}

void LootTablesRegistry::BuildIndices() {
	mob_vnum_index_.clear();
	zone_index_.clear();
	filtered_tables_.clear();

	for (const auto &[id, table] : all_tables_) {
		if (table->GetTableType() != ELootTableType::kSourceCentric) {
			continue;
		}

		bool has_vnum_filter = false;
		bool has_zone_filter = false;

		for (const auto &filter : table->GetFilters()) {
			switch (filter.type) {
				case ELootFilterType::kVnum:
					mob_vnum_index_.emplace(filter.vnum, table);
					has_vnum_filter = true;
					break;

				case ELootFilterType::kZone:
					zone_index_.emplace(filter.zone_vnum, table);
					has_zone_filter = true;
					break;

				default:
					// Complex filters go to filtered_tables_
					break;
			}
		}

		// If table has no simple filters, add to filtered list
		if (!has_vnum_filter && !has_zone_filter) {
			filtered_tables_.push_back(table);
		}
	}
}

std::vector<LootTablesRegistry::TableConstPtr>
LootTablesRegistry::FindTablesForMob(const CharData *mob) const {
	std::vector<TableConstPtr> result;

	if (!mob || !mob->IsNpc()) {
		return result;
	}

	ObjVnum mob_vnum = GET_MOB_VNUM(mob);
	ZoneVnum zone_vnum = GetZoneVnumByCharPlace(const_cast<CharData*>(mob));

	// Check vnum index
	auto vnum_range = mob_vnum_index_.equal_range(mob_vnum);
	for (auto it = vnum_range.first; it != vnum_range.second; ++it) {
		result.push_back(it->second);
	}

	// Check zone index
	auto zone_range = zone_index_.equal_range(zone_vnum);
	for (auto it = zone_range.first; it != zone_range.second; ++it) {
		// Avoid duplicates
		if (std::find(result.begin(), result.end(), it->second) == result.end()) {
			result.push_back(it->second);
		}
	}

	// Check filtered tables
	for (const auto &table : filtered_tables_) {
		if (table->IsApplicable(mob)) {
			if (std::find(result.begin(), result.end(), table) == result.end()) {
				result.push_back(table);
			}
		}
	}

	// Sort by priority (higher first)
	std::sort(result.begin(), result.end(),
		[](const TableConstPtr &a, const TableConstPtr &b) {
			return a->GetPriority() > b->GetPriority();
		});

	return result;
}

GeneratedLoot LootTablesRegistry::GenerateFromMob(
	const CharData *mob,
	const CharData *killer,
	int luck_bonus) const {

	GeneratedLoot result;

	auto tables = FindTablesForMob(mob);
	if (tables.empty()) {
		return result;
	}

	GenerationContext ctx;
	ctx.player = killer;
	ctx.source_mob = mob;
	ctx.luck_bonus = luck_bonus;
	ctx.recursion_depth = 0;

	for (const auto &table : tables) {
		auto table_loot = table->Generate(ctx, this, miw_check_func_);
		result.Merge(table_loot);

		// Update statistics
		if (!table_loot.IsEmpty()) {
			stats_.table_usage_counts[table->GetId()]++;
		}
	}

	// Update statistics
	stats_.total_generations++;
	stats_.total_items_generated += result.TotalItemCount();

	for (const auto &[vnum, count] : result.items) {
		stats_.item_drop_counts[vnum] += count;
	}

	return result;
}

std::vector<std::string> LootTablesRegistry::Validate() const {
	std::vector<std::string> errors;

	// Check for cycles in nested tables
	std::unordered_set<std::string> visited;
	std::unordered_set<std::string> in_path;

	for (const auto &[id, table] : all_tables_) {
		visited.clear();
		in_path.clear();

		if (HasCycle(id, visited, in_path)) {
			errors.push_back("Cycle detected in table: " + id);
		}
	}

	// Check for invalid vnums in entries
	for (const auto &[id, table] : all_tables_) {
		for (const auto &entry : table->GetEntries()) {
			if (entry.type == ELootEntryType::kNestedTable) {
				if (all_tables_.find(entry.nested_table_id) == all_tables_.end()) {
					errors.push_back("Table '" + id + "' references non-existent table: "
						+ entry.nested_table_id);
				}
			}
		}
	}

	return errors;
}

bool LootTablesRegistry::HasCycle(
	const std::string &table_id,
	std::unordered_set<std::string> &visited,
	std::unordered_set<std::string> &in_path) const {

	if (in_path.count(table_id)) {
		return true;  // Cycle detected
	}

	if (visited.count(table_id)) {
		return false;  // Already checked, no cycle
	}

	visited.insert(table_id);
	in_path.insert(table_id);

	auto it = all_tables_.find(table_id);
	if (it != all_tables_.end()) {
		for (const auto &entry : it->second->GetEntries()) {
			if (entry.type == ELootEntryType::kNestedTable) {
				if (HasCycle(entry.nested_table_id, visited, in_path)) {
					return true;
				}
			}
		}
	}

	in_path.erase(table_id);
	return false;
}

// Global registry instance
static LootTablesRegistry g_registry;
static LootTablesLoader g_loader(&g_registry);
static bool g_initialized = false;

LootTablesRegistry &GetGlobalRegistry() {
	return g_registry;
}

void Initialize() {
	if (g_initialized) {
		return;
	}

	g_loader.ClearErrors();
	g_loader.LoadFromDirectory("lib/cfg/loot_tables");
	g_loader.LoadFromDirectory("lib/world/ltt");

	g_initialized = true;

	log("LOOT_TABLES: Initialized with %zu tables", g_registry.GetTableCount());
}

void Shutdown() {
	g_registry.Clear();
	g_initialized = false;
}

void Reload() {
	g_registry.Clear();
	g_loader.ClearErrors();
	g_loader.LoadFromDirectory("lib/cfg/loot_tables");
	g_loader.LoadFromDirectory("lib/world/ltt");

	log("LOOT_TABLES: Reloaded with %zu tables", g_registry.GetTableCount());
}

bool IsInitialized() {
	return g_initialized;
}

GeneratedLoot GenerateMobLoot(const CharData *mob, const CharData *killer, int luck_bonus) {
	if (!g_initialized) {
		return {};
	}
	return g_registry.GenerateFromMob(mob, killer, luck_bonus);
}

void SetMiwCheckFunc(LootTable::MiwCheckFunc func) {
	g_registry.SetMiwCheckFunc(std::move(func));
}

const std::vector<std::string> &GetLoadErrors() {
	return g_loader.GetErrors();
}

} // namespace loot_tables

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
