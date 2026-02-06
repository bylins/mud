//
// Created for Bylins MUD - Loot Tables Module
//

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_LOGGER_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_LOGGER_H_

#include "loot_table.h"

#include <string>

class CharData;

namespace loot_tables {

// Log types for loot events
enum class ELootLogType {
	kRareDrop,      // Item with low probability dropped
	kMiwOverride,   // MIW limit triggered, fallback used
	kError,         // Error during generation
	kDebug          // Debug information
};

// Logger for loot events
class LootLogger {
 public:
	// Singleton instance
	static LootLogger &Instance();

	// Set rare drop threshold (weight below which drops are logged)
	void SetRareThreshold(int threshold) { rare_threshold_ = threshold; }

	// Enable/disable logging
	void SetEnabled(bool enabled) { enabled_ = enabled; }

	// Log a rare drop
	void LogRareDrop(const std::string &table_id,
					 ObjVnum item_vnum,
					 int count,
					 const CharData *killer,
					 int weight,
					 int probability_width);

	// Log MIW override
	void LogMiwOverride(const std::string &table_id,
						ObjVnum item_vnum,
						const CharData *killer,
						int fallback_weight,
						int probability_width);

	// Log error
	void LogError(const std::string &message);

	// Log debug message
	void LogDebug(const std::string &message);

	// Check if item drop is considered rare
	[[nodiscard]] bool IsRareDrop(int weight, int probability_width) const;

 private:
	LootLogger() = default;

	bool enabled_{true};
	int rare_threshold_{500};  // Default: 5% (500/10000) is considered rare

	void WriteLog(ELootLogType type, const std::string &message);
};

} // namespace loot_tables

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_LOGGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
