//
// Created for Bylins MUD - Loot Tables Module
//

#include "loot_loader.h"

#include "utils/logger.h"

#include <fstream>

namespace loot_tables {

LootTablesLoader::LootTablesLoader(LootTablesRegistry *registry)
	: registry_(registry) {
}

void LootTablesLoader::Load(parser_wrapper::DataNode /*data*/) {
	// Load from default YAML paths
	LoadFromDirectory("lib/cfg/loot_tables");
	LoadFromDirectory("lib/world/ltt");
}

void LootTablesLoader::Reload(parser_wrapper::DataNode data) {
	if (registry_) {
		registry_->Clear();
	}
	Load(data);
}

void LootTablesLoader::LoadFromYaml(const std::filesystem::path &file_path) {
#ifdef HAVE_YAML
	if (!std::filesystem::exists(file_path)) {
		errors_.push_back("File not found: " + file_path.string());
		return;
	}

	try {
		YAML::Node root = YAML::LoadFile(file_path.string());

		if (!root["tables"]) {
			errors_.push_back("No 'tables' section in file: " + file_path.string());
			return;
		}

		int loaded_count = 0;
		for (const auto &table_node : root["tables"]) {
			auto table = ParseTable(table_node);
			if (table) {
				registry_->RegisterTable(table);
				++loaded_count;
			}
		}

		log("LOOT_TABLES: Loaded %d tables from %s", loaded_count, file_path.string().c_str());

	} catch (const YAML::Exception &e) {
		errors_.push_back("YAML parse error in " + file_path.string() + ": " + e.what());
		log("LOOT_TABLES: YAML error in %s: %s", file_path.string().c_str(), e.what());
	}
#else
	errors_.push_back("YAML support not compiled in");
	log("LOOT_TABLES: Cannot load %s - YAML support not compiled in", file_path.string().c_str());
#endif
}

void LootTablesLoader::LoadFromDirectory(const std::filesystem::path &dir_path) {
	if (!std::filesystem::exists(dir_path)) {
		return;  // Directory doesn't exist, skip silently
	}

	if (!std::filesystem::is_directory(dir_path)) {
		errors_.push_back("Not a directory: " + dir_path.string());
		return;
	}

	for (const auto &entry : std::filesystem::directory_iterator(dir_path)) {
		if (entry.is_regular_file()) {
			auto ext = entry.path().extension().string();
			if (ext == ".yaml" || ext == ".yml") {
				LoadFromYaml(entry.path());
			}
		}
	}

	// Rebuild indices after loading
	if (registry_) {
		registry_->BuildIndices();

		// Validate
		auto validation_errors = registry_->Validate();
		for (const auto &error : validation_errors) {
			errors_.push_back(error);
			log("LOOT_TABLES: Validation error: %s", error.c_str());
		}
	}
}

#ifdef HAVE_YAML

LootTable::Ptr LootTablesLoader::ParseTable(const YAML::Node &node) {
	std::string id = GetString(node, "id");
	if (id.empty()) {
		errors_.push_back("Table without 'id' field");
		return nullptr;
	}

	auto table = std::make_shared<LootTable>(id);

	// Parse table type
	std::string type_str = GetString(node, "type", "source_centric");
	if (type_str == "source_centric") {
		table->SetTableType(ELootTableType::kSourceCentric);
	} else if (type_str == "item_centric") {
		table->SetTableType(ELootTableType::kItemCentric);
	}

	// Parse source type
	std::string source_str = GetString(node, "source", "mob_death");
	if (source_str == "mob_death") {
		table->SetSourceType(ELootSourceType::kMobDeath);
	} else if (source_str == "container") {
		table->SetSourceType(ELootSourceType::kContainer);
	} else if (source_str == "quest") {
		table->SetSourceType(ELootSourceType::kQuest);
	}

	// Parse priority
	table->SetPriority(GetInt(node, "priority", 100));

	// Parse params
	if (node["params"]) {
		const auto &params = node["params"];
		table->SetProbabilityWidth(GetInt(params, "probability_width", 10000));
		table->SetMaxItemsPerRoll(GetInt(params, "max_items", 10));
	}

	// Parse filters
	if (node["filters"]) {
		for (const auto &filter_node : node["filters"]) {
			auto filter = ParseFilter(filter_node);
			table->AddFilter(filter);
		}
	}

	// Parse entries
	if (node["entries"]) {
		for (const auto &entry_node : node["entries"]) {
			auto entry = ParseEntry(entry_node);
			table->AddEntry(entry);
		}
	}

	return table;
}

LootFilter LootTablesLoader::ParseFilter(const YAML::Node &node) {
	LootFilter filter;

	std::string type_str = GetString(node, "type", "vnum");

	if (type_str == "vnum") {
		filter.type = ELootFilterType::kVnum;
		filter.vnum = GetInt(node, "vnum");
	} else if (type_str == "level_range") {
		filter.type = ELootFilterType::kLevelRange;
		filter.min_level = GetInt(node, "min");
		filter.max_level = GetInt(node, "max");
	} else if (type_str == "zone") {
		filter.type = ELootFilterType::kZone;
		filter.zone_vnum = GetInt(node, "zone");
	} else if (type_str == "race") {
		filter.type = ELootFilterType::kRace;
		filter.race = GetInt(node, "race");
	} else if (type_str == "class") {
		filter.type = ELootFilterType::kClass;
		filter.mob_class = GetInt(node, "class");
	}

	return filter;
}

LootEntry LootTablesLoader::ParseEntry(const YAML::Node &node) {
	LootEntry entry;

	std::string type_str = GetString(node, "type", "item");

	if (type_str == "item") {
		entry.type = ELootEntryType::kItem;
		entry.item_vnum = GetInt(node, "vnum");
	} else if (type_str == "currency") {
		entry.type = ELootEntryType::kCurrency;
		entry.currency_id = GetInt(node, "currency_id", currency::kGold);
	} else if (type_str == "nested_table") {
		entry.type = ELootEntryType::kNestedTable;
		entry.nested_table_id = GetString(node, "table_id");
	}

	// Parse count
	if (node["count"]) {
		entry.count = ParseCountRange(node["count"]);
	}

	// Parse weights
	entry.probability_weight = GetInt(node, "weight", 10000);
	entry.fallback_weight = GetInt(node, "fallback_weight", 0);
	entry.allow_duplicates = GetBool(node, "allow_duplicates", false);

	// Parse player filters
	if (node["player_filters"]) {
		entry.player_filters = ParsePlayerFilters(node["player_filters"]);
	}

	return entry;
}

CountRange LootTablesLoader::ParseCountRange(const YAML::Node &node) {
	CountRange range;

	if (node.IsScalar()) {
		// Single value: count: 5
		range.min = range.max = node.as<int>();
	} else if (node.IsMap()) {
		// Map: count: {min: 1, max: 5}
		range.min = GetInt(node, "min", 1);
		range.max = GetInt(node, "max", 1);
	}

	return range;
}

PlayerFilters LootTablesLoader::ParsePlayerFilters(const YAML::Node &node) {
	PlayerFilters filters;

	if (node["level"]) {
		const auto &level_node = node["level"];
		filters.min_player_level = GetInt(level_node, "min", 0);
		filters.max_player_level = GetInt(level_node, "max", 0);
	}

	if (node["classes"]) {
		for (const auto &class_node : node["classes"]) {
			filters.allowed_classes.push_back(class_node.as<int>());
		}
	}

	return filters;
}

std::string LootTablesLoader::GetString(const YAML::Node &node, const std::string &key,
										const std::string &default_val) {
	if (node[key]) {
		return node[key].as<std::string>();
	}
	return default_val;
}

int LootTablesLoader::GetInt(const YAML::Node &node, const std::string &key, int default_val) {
	if (node[key]) {
		return node[key].as<int>();
	}
	return default_val;
}

bool LootTablesLoader::GetBool(const YAML::Node &node, const std::string &key, bool default_val) {
	if (node[key]) {
		return node[key].as<bool>();
	}
	return default_val;
}

#endif // HAVE_YAML

} // namespace loot_tables

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
