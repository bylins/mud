//
// Created for Bylins MUD - Loot Tables Module
//
// Public API for loot tables system
//

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLES_H_

#include "loot_table_types.h"
#include "loot_table.h"
#include "loot_registry.h"
#include "loot_loader.h"
#include "loot_logger.h"

namespace loot_tables {

// Get global loot tables registry
LootTablesRegistry &GetGlobalRegistry();

// Initialize loot tables system
void Initialize();

// Shutdown loot tables system
void Shutdown();

// Reload all loot tables
void Reload();

// Check if loot tables system is initialized
bool IsInitialized();

// Generate loot for a mob death
// Returns items and currencies to place in corpse
GeneratedLoot GenerateMobLoot(const CharData *mob, const CharData *killer, int luck_bonus = 0);

// Set MIW check function (checks if item is at max_in_world limit)
void SetMiwCheckFunc(LootTable::MiwCheckFunc func);

// Get loading errors from last load/reload
const std::vector<std::string> &GetLoadErrors();

} // namespace loot_tables

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_LOOT_TABLES_LOOT_TABLES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
