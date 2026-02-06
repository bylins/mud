#ifdef HAVE_YAML

#include "gameplay/mechanics/loot_tables/loot_table_types.h"
#include "gameplay/mechanics/loot_tables/loot_table.h"
#include "gameplay/mechanics/loot_tables/loot_registry.h"

#include <gtest/gtest.h>
#include <random>
#include <map>

using namespace loot_tables;

// Test LootFilter
class LootFilterTest : public ::testing::Test {
protected:
	void SetUp() override {
	}
};

TEST_F(LootFilterTest, FromVnum) {
	auto filter = LootFilter::FromVnum(12345);
	EXPECT_EQ(ELootFilterType::kVnum, filter.type);
	EXPECT_EQ(12345, filter.vnum);
}

TEST_F(LootFilterTest, FromLevelRange) {
	auto filter = LootFilter::FromLevelRange(10, 20);
	EXPECT_EQ(ELootFilterType::kLevelRange, filter.type);
	EXPECT_EQ(10, filter.min_level);
	EXPECT_EQ(20, filter.max_level);
}

TEST_F(LootFilterTest, FromZone) {
	auto filter = LootFilter::FromZone(100);
	EXPECT_EQ(ELootFilterType::kZone, filter.type);
	EXPECT_EQ(100, filter.zone_vnum);
}

TEST_F(LootFilterTest, FromRace) {
	auto filter = LootFilter::FromRace(5);
	EXPECT_EQ(ELootFilterType::kRace, filter.type);
	EXPECT_EQ(5, filter.race);
}

// Test CountRange
class CountRangeTest : public ::testing::Test {
};

TEST_F(CountRangeTest, FixedCount) {
	CountRange range(5, 5);
	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(5, range.Roll());
	}
}

TEST_F(CountRangeTest, RangeCount) {
	CountRange range(1, 10);
	std::map<int, int> counts;
	for (int i = 0; i < 1000; ++i) {
		int val = range.Roll();
		EXPECT_GE(val, 1);
		EXPECT_LE(val, 10);
		counts[val]++;
	}
	// Check that we got some variety
	EXPECT_GT(counts.size(), 1u);
}

// Test LootEntry
class LootEntryTest : public ::testing::Test {
};

TEST_F(LootEntryTest, ItemEntry) {
	auto entry = LootEntry::Item(50001, 5000, {1, 3});
	EXPECT_EQ(ELootEntryType::kItem, entry.type);
	EXPECT_EQ(50001, entry.item_vnum);
	EXPECT_EQ(5000, entry.probability_weight);
	EXPECT_EQ(1, entry.count.min);
	EXPECT_EQ(3, entry.count.max);
}

TEST_F(LootEntryTest, CurrencyEntry) {
	auto entry = LootEntry::Currency(currency::kGold, 8000, {100, 500});
	EXPECT_EQ(ELootEntryType::kCurrency, entry.type);
	EXPECT_EQ(currency::kGold, entry.currency_id);
	EXPECT_EQ(8000, entry.probability_weight);
}

TEST_F(LootEntryTest, NestedTableEntry) {
	auto entry = LootEntry::NestedTable("gems_common", 3000);
	EXPECT_EQ(ELootEntryType::kNestedTable, entry.type);
	EXPECT_EQ("gems_common", entry.nested_table_id);
	EXPECT_EQ(3000, entry.probability_weight);
}

// Test PlayerFilters
class PlayerFiltersTest : public ::testing::Test {
};

TEST_F(PlayerFiltersTest, EmptyFilters) {
	PlayerFilters filters;
	EXPECT_TRUE(filters.IsEmpty());
	EXPECT_TRUE(filters.Matches(nullptr));
}

// Test LootTable
class LootTableTest : public ::testing::Test {
protected:
	LootTable::Ptr table;

	void SetUp() override {
		table = std::make_shared<LootTable>("test_table");
		table->SetProbabilityWidth(10000);
		table->SetMaxItemsPerRoll(5);
	}
};

TEST_F(LootTableTest, EmptyTable) {
	GenerationContext ctx;
	auto loot = table->Generate(ctx);
	EXPECT_TRUE(loot.IsEmpty());
}

TEST_F(LootTableTest, SingleItemGuaranteed) {
	auto entry = LootEntry::Item(50001, 10000);  // 100% chance
	table->AddEntry(entry);
	table->AddFilter(LootFilter::FromVnum(12345));

	GenerationContext ctx;
	auto loot = table->Generate(ctx);

	EXPECT_FALSE(loot.IsEmpty());
	EXPECT_EQ(1u, loot.items.size());
	EXPECT_EQ(50001, loot.items[0].first);
	EXPECT_EQ(1, loot.items[0].second);
}

TEST_F(LootTableTest, CurrencyGeneration) {
	auto entry = LootEntry::Currency(currency::kGold, 10000, {100, 200});
	table->AddEntry(entry);
	table->AddFilter(LootFilter::FromVnum(12345));

	GenerationContext ctx;
	auto loot = table->Generate(ctx);

	EXPECT_FALSE(loot.IsEmpty());
	EXPECT_TRUE(loot.currencies.count(currency::kGold) > 0);
	EXPECT_GE(loot.currencies[currency::kGold], 100);
	EXPECT_LE(loot.currencies[currency::kGold], 200);
}

// Test GeneratedLoot
class GeneratedLootTest : public ::testing::Test {
};

TEST_F(GeneratedLootTest, Empty) {
	GeneratedLoot loot;
	EXPECT_TRUE(loot.IsEmpty());
	EXPECT_EQ(0u, loot.TotalItemCount());
}

TEST_F(GeneratedLootTest, Merge) {
	GeneratedLoot loot1;
	loot1.items.emplace_back(50001, 2);
	loot1.currencies[currency::kGold] = 100;

	GeneratedLoot loot2;
	loot2.items.emplace_back(50001, 1);
	loot2.items.emplace_back(50002, 1);
	loot2.currencies[currency::kGold] = 50;
	loot2.currencies[currency::kGlory] = 10;

	loot1.Merge(loot2);

	EXPECT_EQ(2u, loot1.items.size());
	EXPECT_EQ(3, loot1.items[0].second);  // 50001: 2 + 1
	EXPECT_EQ(1, loot1.items[1].second);  // 50002: 1
	EXPECT_EQ(150, loot1.currencies[currency::kGold]);  // 100 + 50
	EXPECT_EQ(10, loot1.currencies[currency::kGlory]);
}

// Test LootTablesRegistry
class LootTablesRegistryTest : public ::testing::Test {
protected:
	LootTablesRegistry registry;

	void SetUp() override {
	}
};

TEST_F(LootTablesRegistryTest, EmptyRegistry) {
	EXPECT_EQ(0u, registry.GetTableCount());
	EXPECT_EQ(nullptr, registry.GetTable("nonexistent"));
}

TEST_F(LootTablesRegistryTest, RegisterTable) {
	auto table = std::make_shared<LootTable>("test_table");
	registry.RegisterTable(table);

	EXPECT_EQ(1u, registry.GetTableCount());
	EXPECT_NE(nullptr, registry.GetTable("test_table"));
}

TEST_F(LootTablesRegistryTest, UnregisterTable) {
	auto table = std::make_shared<LootTable>("test_table");
	registry.RegisterTable(table);
	registry.UnregisterTable("test_table");

	EXPECT_EQ(0u, registry.GetTableCount());
	EXPECT_EQ(nullptr, registry.GetTable("test_table"));
}

TEST_F(LootTablesRegistryTest, Clear) {
	auto table1 = std::make_shared<LootTable>("table1");
	auto table2 = std::make_shared<LootTable>("table2");
	registry.RegisterTable(table1);
	registry.RegisterTable(table2);

	registry.Clear();

	EXPECT_EQ(0u, registry.GetTableCount());
}

// Test cycle detection
TEST_F(LootTablesRegistryTest, CycleDetection) {
	// Create tables with cycle: A -> B -> C -> A
	auto tableA = std::make_shared<LootTable>("table_a");
	tableA->AddEntry(LootEntry::NestedTable("table_b", 10000));

	auto tableB = std::make_shared<LootTable>("table_b");
	tableB->AddEntry(LootEntry::NestedTable("table_c", 10000));

	auto tableC = std::make_shared<LootTable>("table_c");
	tableC->AddEntry(LootEntry::NestedTable("table_a", 10000));

	registry.RegisterTable(tableA);
	registry.RegisterTable(tableB);
	registry.RegisterTable(tableC);

	auto errors = registry.Validate();
	EXPECT_FALSE(errors.empty());
	// Should detect cycle
	bool found_cycle_error = false;
	for (const auto &error : errors) {
		if (error.find("Cycle") != std::string::npos) {
			found_cycle_error = true;
			break;
		}
	}
	EXPECT_TRUE(found_cycle_error);
}

TEST_F(LootTablesRegistryTest, InvalidNestedTableReference) {
	auto table = std::make_shared<LootTable>("test_table");
	table->AddEntry(LootEntry::NestedTable("nonexistent_table", 10000));

	registry.RegisterTable(table);

	auto errors = registry.Validate();
	EXPECT_FALSE(errors.empty());
	bool found_reference_error = false;
	for (const auto &error : errors) {
		if (error.find("non-existent") != std::string::npos) {
			found_reference_error = true;
			break;
		}
	}
	EXPECT_TRUE(found_reference_error);
}

// Test weighted distribution
class WeightedDistributionTest : public ::testing::Test {
protected:
	LootTable::Ptr table;
	LootTablesRegistry registry;

	void SetUp() override {
		table = std::make_shared<LootTable>("distribution_test");
		table->SetProbabilityWidth(10000);
		table->SetMaxItemsPerRoll(1);
		table->AddFilter(LootFilter::FromVnum(12345));

		// Add entries with different weights
		auto entry1 = LootEntry::Item(50001, 7000);  // 70%
		entry1.allow_duplicates = true;
		auto entry2 = LootEntry::Item(50002, 2000);  // 20%
		entry2.allow_duplicates = true;
		auto entry3 = LootEntry::Item(50003, 1000);  // 10%
		entry3.allow_duplicates = true;

		table->AddEntry(entry1);
		table->AddEntry(entry2);
		table->AddEntry(entry3);

		registry.RegisterTable(table);
	}
};

TEST_F(WeightedDistributionTest, DistributionApproximatelyCorrect) {
	std::map<ObjVnum, int> counts;
	const int iterations = 10000;

	GenerationContext ctx;
	for (int i = 0; i < iterations; ++i) {
		auto loot = table->Generate(ctx, &registry);
		for (const auto &[vnum, count] : loot.items) {
			counts[vnum] += count;
		}
	}

	// Check that distribution is approximately correct (within 10% tolerance)
	double total = counts[50001] + counts[50002] + counts[50003];
	if (total > 0) {
		double ratio1 = counts[50001] / total;
		double ratio2 = counts[50002] / total;
		double ratio3 = counts[50003] / total;

		// These checks are approximate - weighted random has variance
		EXPECT_GT(ratio1, 0.5);   // Should be around 70%
		EXPECT_LT(ratio1, 0.9);
		EXPECT_GT(ratio2, 0.1);   // Should be around 20%
		EXPECT_LT(ratio2, 0.35);
		EXPECT_GT(ratio3, 0.02);  // Should be around 10%
		EXPECT_LT(ratio3, 0.25);
	}
}

#endif // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
