/**
\file spell_usage.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_STATISTICS_SPELL_USAGE_H_
#define BYLINS_SRC_GAMEPLAY_STATISTICS_SPELL_USAGE_H_

#include "gameplay/classes/classes_constants.h"

#include <map>

using SpellCountType = std::map<ESpell, int>;
namespace SpellUsage {
extern bool is_active;
extern time_t start;
void AddSpellStat(ECharClass char_class, ESpell spell_id);
std::string StatToPrint();
void Save();
void Clear();
};

#endif //BYLINS_SRC_GAMEPLAY_STATISTICS_SPELL_USAGE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
