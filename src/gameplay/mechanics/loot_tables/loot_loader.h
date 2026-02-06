//
// Created for Bylins MUD - Loot Tables Module
//

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_LOADER_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_LOADER_H_

#include "loot_registry.h"
#include "engine/boot/cfg_manager.h"

#include <filesystem>
#include <vector>

#ifdef HAVE_YAML
#include <yaml-cpp/yaml.h>
#endif

namespace loot_tables {

// Loader for loot tables from YAML files
class LootTablesLoader : public cfg_manager::ICfgLoader {
 public:
	explicit LootTablesLoader(LootTablesRegistry *registry);

	// ICfgLoader interface
	void Load(parser_wrapper::DataNode data) override;
	void Reload(parser_wrapper::DataNode data) override;

	// Load from YAML file directly
	void LoadFromYaml(const std::filesystem::path &file_path);

	// Load all YAML files from directory
	void LoadFromDirectory(const std::filesystem::path &dir_path);

	// Get load errors
	[[nodiscard]] const std::vector<std::string> &GetErrors() const { return errors_; }

	// Clear errors
	void ClearErrors() { errors_.clear(); }

 private:
	LootTablesRegistry *registry_;
	std::vector<std::string> errors_;

#ifdef HAVE_YAML
	// Parse single table from YAML node
	LootTable::Ptr ParseTable(const YAML::Node &node);

	// Parse filter from YAML node
	LootFilter ParseFilter(const YAML::Node &node);

	// Parse entry from YAML node
	LootEntry ParseEntry(const YAML::Node &node);

	// Parse count range from YAML node
	CountRange ParseCountRange(const YAML::Node &node);

	// Parse player filters from YAML node
	PlayerFilters ParsePlayerFilters(const YAML::Node &node);

	// Helper to get string value with default
	static std::string GetString(const YAML::Node &node, const std::string &key,
								 const std::string &default_val = "");

	// Helper to get int value with default
	static int GetInt(const YAML::Node &node, const std::string &key, int default_val = 0);

	// Helper to get bool value with default
	static bool GetBool(const YAML::Node &node, const std::string &key, bool default_val = false);
#endif
};

} // namespace loot_tables

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_LOADER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
