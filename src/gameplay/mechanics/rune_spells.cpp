/**
 * \brief Rune-spell registry implementation (issue.runes-migrate).
 */

#include "rune_spells.h"

#include <algorithm>

#include "engine/db/global_objects.h"
#include "gameplay/magic/spells_info.h"
#include "utils/parse.h"
#include "utils/utils_string.h"

namespace rune_spells {

namespace {

constexpr int kMinCasterLevel = 1;
constexpr int kMaxCasterLevel = 34;

// Write the legacy spell_create[id].runes shadow used by the rune-recipe
// consumers in spells.cpp (issue.runes-migrate scope: leave the recipe-loop
// machinery intact; only the loader changes). The legacy shape carries
// items[0..2] + rnumber (the 4th rune) and a min_caster_level mirror.
// Runes beyond the 4th slot are dropped from the shadow but kept in the
// new RuneSpells() map for the simple consumers.
void WriteLegacyShadow(ESpell spell_id, const RuneSpellInfo &info) {
	auto &shadow = spell_create[spell_id].runes;
	shadow.items[0] = info.runes.size() > 0 ? info.runes[0] : -1;
	shadow.items[1] = info.runes.size() > 1 ? info.runes[1] : -1;
	shadow.items[2] = info.runes.size() > 2 ? info.runes[2] : -1;
	shadow.rnumber  = info.runes.size() > 3 ? info.runes[3] : -1;
	shadow.min_caster_level = info.min_caster_level;
}

void ParseSpellNode(Registry &registry, parser_wrapper::DataNode &node) {
	ESpell spell_id = ESpell::kUndefined;
	try {
		spell_id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("rune_spells: unknown ESpell id '%s' in <spell>", e.what());
		return;
	}
	if (spell_id == ESpell::kUndefined) {
		return;
	}

	RuneSpellInfo info;
	const char *level_str = node.GetValue("level");
	int level = (level_str && *level_str) ? parse::ReadAsInt(level_str) : kMinCasterLevel;
	info.min_caster_level = std::clamp(level, kMinCasterLevel, kMaxCasterLevel);

	const char *runes_str = node.GetValue("runes");
	if (runes_str && *runes_str) {
		for (const auto &token : utils::Split(runes_str, '|')) {
			try {
				info.runes.push_back(parse::ReadAsInt(token.c_str()));
			} catch (std::exception &e) {
				err_log("rune_spells: bad rune vnum '%s' for spell %s",
						token.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
			}
		}
	}

	WriteLegacyShadow(spell_id, info);
	registry[spell_id] = std::move(info);
}

}  // namespace

void RuneSpellsLoader::Load(parser_wrapper::DataNode data) {
	auto &registry = MUD::RuneSpells();
	registry.clear();
	spell_create.clear();
	for (auto &node : data.Children()) {
		if (strcmp(node.GetName(), "spell") != 0) {
			continue;
		}
		ParseSpellNode(registry, node);
	}
}

void RuneSpellsLoader::Reload(parser_wrapper::DataNode data) {
	Load(data);
}

}  // namespace rune_spells

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
