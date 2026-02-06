//
// Created for Bylins MUD - Loot Tables Module
//

#include "do_loottable.h"

#ifdef HAVE_YAML

#include "gameplay/mechanics/loot_tables/loot_tables.h"
#include "engine/entities/char_data.h"
#include "engine/db/obj_prototypes.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

#include <sstream>

namespace {

void ShowHelp(CharData *ch) {
	SendMsgToChar(ch,
		"Usage: loottable <command> [args]\r\n"
		"Commands:\r\n"
		"  list                    - List all loot tables\r\n"
		"  info <table_id>         - Show table details\r\n"
		"  reload                  - Reload all loot tables\r\n"
		"  test <table_id> [luck]  - Test generation from table\r\n"
		"  generate <mob_vnum> [luck] - Generate loot for mob vnum\r\n"
		"  stats                   - Show usage statistics\r\n"
		"  errors                  - Show loading errors\r\n"
	);
}

void ListTables(CharData *ch) {
	auto &registry = loot_tables::GetGlobalRegistry();
	const auto &tables = registry.GetAllTables();

	if (tables.empty()) {
		SendMsgToChar(ch, "No loot tables loaded.\r\n");
		return;
	}

	std::ostringstream ss;
	ss << "Loot tables (" << tables.size() << " total):\r\n";
	ss << "----------------------------------------\r\n";

	for (const auto &[id, table] : tables) {
		ss << "  " << id << " (priority: " << table->GetPriority()
		   << ", entries: " << table->GetEntries().size() << ")\r\n";
	}

	SendMsgToChar(ch, "%s", ss.str().c_str());
}

void ShowTableInfo(CharData *ch, const char *table_id) {
	auto &registry = loot_tables::GetGlobalRegistry();
	auto table = registry.GetTable(table_id);

	if (!table) {
		SendMsgToChar(ch, "Table '%s' not found.\r\n", table_id);
		return;
	}

	std::ostringstream ss;
	ss << "Table: " << table->GetId() << "\r\n";
	ss << "----------------------------------------\r\n";
	ss << "Type: " << (table->GetTableType() == loot_tables::ELootTableType::kSourceCentric
					   ? "source_centric" : "item_centric") << "\r\n";
	ss << "Source: ";
	switch (table->GetSourceType()) {
		case loot_tables::ELootSourceType::kMobDeath: ss << "mob_death"; break;
		case loot_tables::ELootSourceType::kContainer: ss << "container"; break;
		case loot_tables::ELootSourceType::kQuest: ss << "quest"; break;
	}
	ss << "\r\n";
	ss << "Priority: " << table->GetPriority() << "\r\n";
	ss << "Probability width: " << table->GetProbabilityWidth() << "\r\n";
	ss << "Max items per roll: " << table->GetMaxItemsPerRoll() << "\r\n";

	ss << "\r\nFilters (" << table->GetFilters().size() << "):\r\n";
	for (const auto &filter : table->GetFilters()) {
		ss << "  - ";
		switch (filter.type) {
			case loot_tables::ELootFilterType::kVnum:
				ss << "vnum: " << filter.vnum;
				break;
			case loot_tables::ELootFilterType::kLevelRange:
				ss << "level: " << filter.min_level << "-" << filter.max_level;
				break;
			case loot_tables::ELootFilterType::kZone:
				ss << "zone: " << filter.zone_vnum;
				break;
			case loot_tables::ELootFilterType::kRace:
				ss << "race: " << filter.race;
				break;
			case loot_tables::ELootFilterType::kClass:
				ss << "class: " << filter.mob_class;
				break;
			default:
				ss << "unknown";
		}
		ss << "\r\n";
	}

	ss << "\r\nEntries (" << table->GetEntries().size() << "):\r\n";
	for (const auto &entry : table->GetEntries()) {
		ss << "  - ";
		switch (entry.type) {
			case loot_tables::ELootEntryType::kItem: {
				auto proto = obj_proto[obj_proto.get_rnum(entry.item_vnum)];
				ss << "item " << entry.item_vnum;
				if (proto) {
					ss << " (" << proto->get_short_description() << ")";
				}
				break;
			}
			case loot_tables::ELootEntryType::kCurrency:
				ss << "currency " << entry.currency_id;
				break;
			case loot_tables::ELootEntryType::kNestedTable:
				ss << "nested_table " << entry.nested_table_id;
				break;
		}
		ss << " | weight: " << entry.probability_weight;
		if (entry.fallback_weight > 0) {
			ss << " (fallback: " << entry.fallback_weight << ")";
		}
		ss << " | count: " << entry.count.min << "-" << entry.count.max;
		if (entry.allow_duplicates) {
			ss << " [duplicates]";
		}
		ss << "\r\n";
	}

	SendMsgToChar(ch, "%s", ss.str().c_str());
}

void ReloadTables(CharData *ch) {
	loot_tables::Reload();

	auto &registry = loot_tables::GetGlobalRegistry();
	const auto &errors = loot_tables::GetLoadErrors();

	if (errors.empty()) {
		SendMsgToChar(ch, "Loot tables reloaded successfully. %zu tables loaded.\r\n",
			registry.GetTableCount());
	} else {
		SendMsgToChar(ch, "Loot tables reloaded with %zu errors. %zu tables loaded.\r\n",
			errors.size(), registry.GetTableCount());
		for (const auto &error : errors) {
			SendMsgToChar(ch, "  Error: %s\r\n", error.c_str());
		}
	}
}

void TestTable(CharData *ch, const char *table_id, int luck) {
	auto &registry = loot_tables::GetGlobalRegistry();
	auto table = registry.GetTable(table_id);

	if (!table) {
		SendMsgToChar(ch, "Table '%s' not found.\r\n", table_id);
		return;
	}

	loot_tables::GenerationContext ctx;
	ctx.player = ch;
	ctx.source_mob = nullptr;
	ctx.luck_bonus = luck;

	auto loot = table->Generate(ctx, &registry, nullptr);

	if (loot.IsEmpty()) {
		SendMsgToChar(ch, "Test generation produced no loot.\r\n");
		return;
	}

	std::ostringstream ss;
	ss << "Test generation from '" << table_id << "' (luck: " << luck << "):\r\n";

	for (const auto &[vnum, count] : loot.items) {
		auto proto = obj_proto[obj_proto.get_rnum(vnum)];
		ss << "  Item " << vnum;
		if (proto) {
			ss << " (" << proto->get_short_description() << ")";
		}
		ss << " x" << count << "\r\n";
	}

	for (const auto &[currency_id, amount] : loot.currencies) {
		ss << "  Currency " << currency_id << ": " << amount << "\r\n";
	}

	SendMsgToChar(ch, "%s", ss.str().c_str());
}

void ShowStats(CharData *ch) {
	auto &registry = loot_tables::GetGlobalRegistry();
	const auto &stats = registry.GetStats();

	std::ostringstream ss;
	ss << "Loot tables statistics:\r\n";
	ss << "----------------------------------------\r\n";
	ss << "Total generations: " << stats.total_generations << "\r\n";
	ss << "Total items generated: " << stats.total_items_generated << "\r\n";

	if (!stats.table_usage_counts.empty()) {
		ss << "\r\nTable usage:\r\n";
		for (const auto &[table_id, count] : stats.table_usage_counts) {
			ss << "  " << table_id << ": " << count << "\r\n";
		}
	}

	if (!stats.item_drop_counts.empty()) {
		ss << "\r\nTop dropped items:\r\n";
		std::vector<std::pair<ObjVnum, size_t>> sorted_items(
			stats.item_drop_counts.begin(), stats.item_drop_counts.end());
		std::sort(sorted_items.begin(), sorted_items.end(),
			[](const auto &a, const auto &b) { return a.second > b.second; });

		int shown = 0;
		for (const auto &[vnum, count] : sorted_items) {
			if (++shown > 10) break;
			auto proto = obj_proto[obj_proto.get_rnum(vnum)];
			ss << "  " << vnum;
			if (proto) {
				ss << " (" << proto->get_short_description() << ")";
			}
			ss << ": " << count << "\r\n";
		}
	}

	SendMsgToChar(ch, "%s", ss.str().c_str());
}

void ShowErrors(CharData *ch) {
	const auto &errors = loot_tables::GetLoadErrors();

	if (errors.empty()) {
		SendMsgToChar(ch, "No loading errors.\r\n");
		return;
	}

	std::ostringstream ss;
	ss << "Loading errors (" << errors.size() << "):\r\n";
	for (const auto &error : errors) {
		ss << "  " << error << "\r\n";
	}

	SendMsgToChar(ch, "%s", ss.str().c_str());
}

} // anonymous namespace

void do_loottable(CharData *ch, char *argument, int /*cmd*/, int /*subcmd*/) {
	if (!loot_tables::IsInitialized()) {
		SendMsgToChar(ch, "Loot tables system is not initialized.\r\n");
		return;
	}

	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		ShowHelp(ch);
		return;
	}

	if (utils::IsAbbr(arg1, "list")) {
		ListTables(ch);
	} else if (utils::IsAbbr(arg1, "info")) {
		if (!*arg2) {
			SendMsgToChar(ch, "Usage: loottable info <table_id>\r\n");
			return;
		}
		ShowTableInfo(ch, arg2);
	} else if (utils::IsAbbr(arg1, "reload")) {
		ReloadTables(ch);
	} else if (utils::IsAbbr(arg1, "test")) {
		if (!*arg2) {
			SendMsgToChar(ch, "Usage: loottable test <table_id> [luck]\r\n");
			return;
		}
		char arg3[kMaxInputLength];
		one_argument(argument + strlen(arg1) + strlen(arg2) + 2, arg3);
		int luck = *arg3 ? atoi(arg3) : 0;
		TestTable(ch, arg2, luck);
	} else if (utils::IsAbbr(arg1, "stats")) {
		ShowStats(ch);
	} else if (utils::IsAbbr(arg1, "errors")) {
		ShowErrors(ch);
	} else {
		ShowHelp(ch);
	}
}

#else // HAVE_YAML

void do_loottable(CharData *ch, char * /*argument*/, int /*cmd*/, int /*subcmd*/) {
	SendMsgToChar(ch, "Loot tables system is not compiled in (requires HAVE_YAML).\r\n");
}

#endif // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
