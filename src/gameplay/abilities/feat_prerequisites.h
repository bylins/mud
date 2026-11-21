/**
\file feat_prerequisites.h - a part of the Bylins engine.
\brief Prerequisites for learning a feat, read from the <prerequisites> section of cfg/feats.xml.
\details A feat may list several <prerequisite> alternatives; the feat can be learned if at least one
 of them is met, and an alternative is met when all of its conditions are. Class, level, remort and
 slot requirements live in the class files, race access in pc_races.xml -- not here.
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_PREREQUISITES_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_PREREQUISITES_H_

#include "feats_constants.h"
#include "gameplay/skills/skills.h"
#include "utils/parser_wrapper.h"

#include <functional>
#include <vector>

class CharData;

namespace feats {

class FeatPrerequisites {
 public:
	// <skill id="kParry" min="1"/>: the skill, without equipment bonuses, is at least `min`.
	struct SkillCondition {
		ESkill id{ESkill::kUndefined};
		int min{1};
	};
	// <feat id="kPowerAttack"/>: the character has the feat; absence="Y" -- does not have it.
	struct FeatCondition {
		EFeat id{EFeat::kUndefined};
		bool absence{false};
	};
	struct Alternative {
		std::vector<SkillCondition> skills;
		std::vector<FeatCondition> feats;
	};
	using SkillLookup = std::function<int(ESkill)>;
	using FeatLookup = std::function<bool(EFeat)>;

	// Reads the <prerequisites> child of the <feat> node `node` points at; no child -- no prerequisites.
	// An alternative with an invalid entry is logged and dropped as a whole, so a mistake in the file
	// can only make a feat harder to learn, never easier: if every alternative was dropped, the feat
	// cannot be learned at all.
	void Build(parser_wrapper::DataNode &node, EFeat owner);

	[[nodiscard]] bool IsMet(const CharData *ch) const;
	[[nodiscard]] bool IsMet(const SkillLookup &skill, const FeatLookup &has_feat) const;

	// No prerequisites at all: the feat is not restricted by this file.
	[[nodiscard]] bool IsEmpty() const { return alternatives_.empty() && !broken_; };
	[[nodiscard]] const std::vector<Alternative> &GetAlternatives() const { return alternatives_; };

 private:
	std::vector<Alternative> alternatives_;
	// The section listed alternatives, but none of them was valid.
	bool broken_{false};
};

} // namespace feats

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_PREREQUISITES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
