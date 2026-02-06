//
// Created for Bylins MUD - Loot Tables Module
//

#include "loot_logger.h"

#include "engine/entities/char_data.h"
#include "utils/logger.h"

#include <ctime>
#include <sstream>
#include <iomanip>

namespace loot_tables {

LootLogger &LootLogger::Instance() {
	static LootLogger instance;
	return instance;
}

void LootLogger::LogRareDrop(const std::string &table_id,
							 ObjVnum item_vnum,
							 int count,
							 const CharData *killer,
							 int weight,
							 int probability_width) {
	if (!enabled_) {
		return;
	}

	std::ostringstream ss;
	ss << "RARE: " << table_id << " -> item[" << item_vnum << "] x" << count;

	if (killer && !killer->IsNpc()) {
		ss << " | killer: " << killer->get_name() << "(" << GetRealLevel(killer) << ")";
	}

	ss << " | weight: " << weight << "/" << probability_width;

	WriteLog(ELootLogType::kRareDrop, ss.str());
}

void LootLogger::LogMiwOverride(const std::string &table_id,
								ObjVnum item_vnum,
								const CharData *killer,
								int fallback_weight,
								int probability_width) {
	if (!enabled_) {
		return;
	}

	std::ostringstream ss;
	ss << "MIW_OVERRIDE: " << table_id << " -> item[" << item_vnum << "]";
	ss << " | fallback: " << fallback_weight << "/" << probability_width;

	if (killer && !killer->IsNpc()) {
		ss << " | killer: " << killer->get_name() << "(" << GetRealLevel(killer) << ")";
	}

	WriteLog(ELootLogType::kMiwOverride, ss.str());
}

void LootLogger::LogError(const std::string &message) {
	if (!enabled_) {
		return;
	}

	WriteLog(ELootLogType::kError, "ERROR: " + message);
}

void LootLogger::LogDebug(const std::string &message) {
	if (!enabled_) {
		return;
	}

	WriteLog(ELootLogType::kDebug, "DEBUG: " + message);
}

bool LootLogger::IsRareDrop(int weight, int probability_width) const {
	if (probability_width <= 0) {
		return false;
	}

	// Normalize weight to 10000 scale for comparison
	int normalized = (weight * 10000) / probability_width;
	return normalized < rare_threshold_;
}

void LootLogger::WriteLog(ELootLogType type, const std::string &message) {
	// Get current time
	std::time_t now = std::time(nullptr);
	std::tm *tm_info = std::localtime(&now);

	std::ostringstream timestamp;
	timestamp << std::put_time(tm_info, "[%Y-%m-%d %H:%M:%S]");

	std::string full_message = timestamp.str() + " " + message;

	// Log to syslog
	switch (type) {
		case ELootLogType::kRareDrop:
		case ELootLogType::kMiwOverride:
			log("LOOT_TABLES: %s", full_message.c_str());
			break;

		case ELootLogType::kError:
			log("LOOT_TABLES ERROR: %s", full_message.c_str());
			break;

		case ELootLogType::kDebug:
			// Only log debug in debug mode
#ifdef DEBUG
			log("LOOT_TABLES DEBUG: %s", full_message.c_str());
#endif
			break;
	}
}

} // namespace loot_tables

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
