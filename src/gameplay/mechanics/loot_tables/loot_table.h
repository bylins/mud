//
// Created for Bylins MUD - Loot Tables Module
//

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLE_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLE_H_

#include "loot_table_types.h"

#include <memory>
#include <map>
#include <functional>

namespace loot_tables {

// Forward declaration
class LootTablesRegistry;

// Context for loot generation
struct GenerationContext {
	const CharData *player{nullptr};      // Player who killed the mob (for filters)
	const CharData *source_mob{nullptr};  // Source mob (for reference)
	int luck_bonus{0};                    // Bonus to drop chance (0-100)
	int recursion_depth{0};               // Current recursion depth (for nested tables)

	// Maximum recursion depth for nested tables
	static constexpr int kMaxRecursionDepth = 10;
};

// Result of loot generation
struct GeneratedLoot {
	// Generated items: vnum -> count
	std::vector<std::pair<ObjVnum, int>> items;

	// Generated currencies: currency_id -> amount
	std::map<int, long> currencies;

	// Merge another GeneratedLoot into this one
	void Merge(const GeneratedLoot &other);

	// Check if empty
	[[nodiscard]] bool IsEmpty() const;

	// Get total item count
	[[nodiscard]] size_t TotalItemCount() const;
};

// Loot table class
class LootTable {
 public:
	using Ptr = std::shared_ptr<LootTable>;
	using ConstPtr = std::shared_ptr<const LootTable>;

	// MIW check function type
	using MiwCheckFunc = std::function<bool(ObjVnum)>;

	LootTable() = default;
	explicit LootTable(std::string id);

	// Getters
	[[nodiscard]] const std::string &GetId() const { return id_; }
	[[nodiscard]] ELootTableType GetTableType() const { return table_type_; }
	[[nodiscard]] ELootSourceType GetSourceType() const { return source_type_; }
	[[nodiscard]] int GetPriority() const { return priority_; }
	[[nodiscard]] int GetProbabilityWidth() const { return probability_width_; }
	[[nodiscard]] int GetMaxItemsPerRoll() const { return max_items_per_roll_; }
	[[nodiscard]] const std::vector<LootFilter> &GetFilters() const { return source_filters_; }
	[[nodiscard]] const std::vector<LootEntry> &GetEntries() const { return entries_; }

	// Setters
	void SetId(const std::string &id) { id_ = id; }
	void SetTableType(ELootTableType type) { table_type_ = type; }
	void SetSourceType(ELootSourceType type) { source_type_ = type; }
	void SetPriority(int priority) { priority_ = priority; }
	void SetProbabilityWidth(int width) { probability_width_ = width; }
	void SetMaxItemsPerRoll(int max_items) { max_items_per_roll_ = max_items; }

	// Add filter
	void AddFilter(const LootFilter &filter);

	// Add entry
	void AddEntry(const LootEntry &entry);

	// Clear all entries
	void ClearEntries();

	// Check if this table is applicable to the given mob
	[[nodiscard]] bool IsApplicable(const CharData *mob) const;

	// Generate loot from this table
	[[nodiscard]] GeneratedLoot Generate(
		const GenerationContext &ctx,
		const LootTablesRegistry *registry = nullptr,
		const MiwCheckFunc &miw_check = nullptr) const;

 private:
	std::string id_;
	ELootTableType table_type_{ELootTableType::kSourceCentric};
	ELootSourceType source_type_{ELootSourceType::kMobDeath};
	std::vector<LootFilter> source_filters_;
	std::vector<LootEntry> entries_;

	int probability_width_{10000};   // Base for probability (10000 = 100.00%)
	int max_items_per_roll_{10};     // Maximum items per single generation
	int priority_{100};              // Table priority (higher = processed first)

	// Internal generation with tracking of already selected entries
	void GenerateInternal(
		const GenerationContext &ctx,
		const LootTablesRegistry *registry,
		const MiwCheckFunc &miw_check,
		GeneratedLoot &result,
		std::vector<bool> &entry_used) const;

	// Calculate effective weight for entry considering MIW
	[[nodiscard]] int GetEffectiveWeight(
		const LootEntry &entry,
		const MiwCheckFunc &miw_check) const;
};

} // namespace loot_tables

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
