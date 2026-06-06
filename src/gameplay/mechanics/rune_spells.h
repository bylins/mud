/**
 * \brief Rune-spell registry, loaded from lib/cfg/mechanics/rune_spells.xml.
 * \details Replaces the hand-rolled lib/misc/runes.lst parser
 *          (legacy InitSpellLevels() in pc_classes.cpp), which carried two
 *          structural limits the new schema sheds:
 *            1. Fixed 4-rune slot ceiling (was an array<int,3>+int).
 *            2. No way to comment out a spell without losing its data --
 *               the old loader silently skipped ';' lines.
 *          (issue.runes-migrate)
 */

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_SPELLS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_SPELLS_H_

#include <map>
#include <vector>

#include "engine/boot/cfg_manager.h"
#include "gameplay/magic/spells_constants.h"
#include "utils/parser_wrapper.h"

namespace rune_spells {

struct RuneSpellInfo {
	std::vector<int> runes;
	int min_caster_level{1};
};

using Registry = std::map<ESpell, RuneSpellInfo>;

class RuneSpellsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

}  // namespace rune_spells

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
