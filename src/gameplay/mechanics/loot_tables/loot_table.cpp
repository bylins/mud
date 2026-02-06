//
// Created for Bylins MUD - Loot Tables Module
//

#include "loot_table.h"
#include "loot_registry.h"

#include "utils/random.h"

namespace loot_tables {

void GeneratedLoot::Merge(const GeneratedLoot &other) {
	// Merge items
	for (const auto &[vnum, count] : other.items) {
		bool found = false;
		for (auto &[existing_vnum, existing_count] : items) {
			if (existing_vnum == vnum) {
				existing_count += count;
				found = true;
				break;
			}
		}
		if (!found) {
			items.emplace_back(vnum, count);
		}
	}

	// Merge currencies
	for (const auto &[currency_id, amount] : other.currencies) {
		currencies[currency_id] += amount;
	}
}

bool GeneratedLoot::IsEmpty() const {
	return items.empty() && currencies.empty();
}

size_t GeneratedLoot::TotalItemCount() const {
	size_t total = 0;
	for (const auto &[vnum, count] : items) {
		total += count;
	}
	return total;
}

LootTable::LootTable(std::string id)
	: id_(std::move(id)) {
}

void LootTable::AddFilter(const LootFilter &filter) {
	source_filters_.push_back(filter);
}

void LootTable::AddEntry(const LootEntry &entry) {
	entries_.push_back(entry);
}

void LootTable::ClearEntries() {
	entries_.clear();
}

bool LootTable::IsApplicable(const CharData *mob) const {
	if (!mob) {
		return false;
	}

	// If no filters, table applies to nothing
	if (source_filters_.empty()) {
		return false;
	}

	// Check if any filter matches
	for (const auto &filter : source_filters_) {
		if (filter.Matches(mob)) {
			return true;
		}
	}

	return false;
}

GeneratedLoot LootTable::Generate(
	const GenerationContext &ctx,
	const LootTablesRegistry *registry,
	const MiwCheckFunc &miw_check) const {

	GeneratedLoot result;

	if (entries_.empty()) {
		return result;
	}

	// Check recursion depth
	if (ctx.recursion_depth >= GenerationContext::kMaxRecursionDepth) {
		return result;
	}

	// Track which entries have been used (for allow_duplicates=false)
	std::vector<bool> entry_used(entries_.size(), false);

	GenerateInternal(ctx, registry, miw_check, result, entry_used);

	return result;
}

void LootTable::GenerateInternal(
	const GenerationContext &ctx,
	const LootTablesRegistry *registry,
	const MiwCheckFunc &miw_check,
	GeneratedLoot &result,
	std::vector<bool> &entry_used) const {

	// Calculate total weight of available entries
	int total_weight = 0;
	std::vector<int> effective_weights(entries_.size(), 0);

	for (size_t i = 0; i < entries_.size(); ++i) {
		const auto &entry = entries_[i];

		// Skip if already used and duplicates not allowed
		if (entry_used[i] && !entry.allow_duplicates) {
			continue;
		}

		// Check player filters
		if (!entry.CheckPlayerFilters(ctx.player)) {
			continue;
		}

		int weight = GetEffectiveWeight(entry, miw_check);
		effective_weights[i] = weight;
		total_weight += weight;
	}

	if (total_weight <= 0) {
		return;
	}

	// Generate items up to max_items_per_roll
	int items_generated = 0;
	int max_attempts = max_items_per_roll_ * 3;  // Prevent infinite loops

	while (items_generated < max_items_per_roll_ && max_attempts > 0) {
		--max_attempts;

		// Roll for probability
		int roll = number(1, probability_width_);

		// Apply luck bonus
		if (ctx.luck_bonus > 0) {
			// Luck bonus reduces the roll, making it easier to hit lower weights
			roll = std::max(1, roll - (roll * ctx.luck_bonus / 100));
		}

		// Select entry based on weighted random
		int weight_roll = number(1, total_weight);
		int cumulative = 0;
		int selected_idx = -1;

		for (size_t i = 0; i < entries_.size(); ++i) {
			if (effective_weights[i] <= 0) {
				continue;
			}

			cumulative += effective_weights[i];
			if (weight_roll <= cumulative) {
				selected_idx = static_cast<int>(i);
				break;
			}
		}

		if (selected_idx < 0) {
			break;
		}

		const auto &entry = entries_[selected_idx];

		// Check if roll hits probability
		if (roll > entry.probability_weight) {
			continue;
		}

		// Generate loot based on entry type
		switch (entry.type) {
			case ELootEntryType::kItem: {
				int count = entry.RollCount();
				if (count > 0) {
					result.items.emplace_back(entry.item_vnum, count);
					++items_generated;
				}
				break;
			}

			case ELootEntryType::kCurrency: {
				long amount = entry.RollCount();
				if (amount > 0) {
					result.currencies[entry.currency_id] += amount;
					++items_generated;
				}
				break;
			}

			case ELootEntryType::kNestedTable: {
				if (registry && !entry.nested_table_id.empty()) {
					auto nested_table = registry->GetTable(entry.nested_table_id);
					if (nested_table) {
						GenerationContext nested_ctx = ctx;
						nested_ctx.recursion_depth++;

						auto nested_loot = nested_table->Generate(nested_ctx, registry, miw_check);
						result.Merge(nested_loot);
						items_generated += static_cast<int>(nested_loot.TotalItemCount());
					}
				}
				break;
			}
		}

		// Mark entry as used if duplicates not allowed
		if (!entry.allow_duplicates) {
			entry_used[selected_idx] = true;

			// Recalculate total weight
			total_weight -= effective_weights[selected_idx];
			effective_weights[selected_idx] = 0;

			if (total_weight <= 0) {
				break;
			}
		}
	}
}

int LootTable::GetEffectiveWeight(
	const LootEntry &entry,
	const MiwCheckFunc &miw_check) const {

	// For items, check MIW limit
	if (entry.type == ELootEntryType::kItem && miw_check) {
		if (miw_check(entry.item_vnum)) {
			// MIW limit reached, use fallback weight
			return entry.fallback_weight;
		}
	}

	return entry.probability_weight;
}

} // namespace loot_tables

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
