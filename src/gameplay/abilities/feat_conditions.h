/**
\file feat_conditions.h - a part of the Bylins engine.
\brief Conditions on a feat read from cfg/feats.xml: <prerequisites> to learn it, <requirements> to use it.
\details A section lists several alternatives (<prerequisite> or <requirement>); at least one of them must be
 met, and an alternative is met when all of its conditions are. Class, level, remort and slot requirements
 live in the class files, race access in pc_races.xml -- not here.
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_CONDITIONS_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_CONDITIONS_H_

#include "feats_constants.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/skills/skills.h"
#include "utils/parser_wrapper.h"

#include <vector>

class CharData;

namespace feats {

class FeatConditions {
 public:
	// Which section the conditions come from. It also sets how a skill is read: learning a feat looks
	// at the skill without equipment bonuses, using it -- at the full skill, as the fight code does.
	enum class EKind {
		kPrerequisites,	// <prerequisites>/<prerequisite>
		kRequirements	// <requirements>/<requirement>
	};

	// <skill id="kParry" min="1"/>: the skill is at least `min`.
	struct SkillCondition {
		ESkill id{ESkill::kUndefined};
		int min{1};
	};
	// <feat id="kPowerAttack"/>: the character has the feat; absence="Y" -- does not have it.
	struct FeatCondition {
		EFeat id{EFeat::kUndefined};
		bool absence{false};
	};
	// <stat id="kDex" min="18" above="kStr"/>: the stat is at least `min` and greater than the `above`
	// stat. At least one of the two is given.
	struct StatCondition {
		EBaseStat id{EBaseStat::kStr};
		int min{0};
		bool has_above{false};
		EBaseStat above{EBaseStat::kStr};
	};
	// <level min="20" max="24" remort_divisor="3"/>: level + remorts / remort_divisor is within
	// [min, max]. At least one of min and max is given; remort_divisor 0 (default) ignores remorts.
	struct LevelCondition {
		int min{0};
		bool has_max{false};
		int max{0};
		int remort_divisor{0};
	};
	struct Alternative {
		std::vector<SkillCondition> skills;
		std::vector<FeatCondition> feats;
		std::vector<StatCondition> stats;
		std::vector<LevelCondition> levels;
	};

	explicit FeatConditions(EKind kind) : kind_{kind} {};

	// Reads this kind's section from the <feat> node `node` points at; no section -- no conditions.
	// An alternative with an invalid entry is logged and dropped as a whole, so a mistake in the file
	// can only make a feat harder to get, never easier: if every alternative was dropped, the
	// conditions can never be met.
	void Build(parser_wrapper::DataNode &node, EFeat owner);

	[[nodiscard]] bool IsMet(const CharData *ch) const {
		return IsEmpty() || IsMetByChar(ch);
	};
	// `subject` answers Skill(ESkill) -> int, HasFeat(EFeat) -> bool, Stat(EBaseStat) -> int,
	// Level() -> int and Remorts() -> int.
	template<typename Subject>
	[[nodiscard]] bool IsMetBy(const Subject &subject) const;

	// No conditions at all: the feat is not restricted by this section.
	[[nodiscard]] bool IsEmpty() const { return alternatives_.empty() && !broken_; };
	[[nodiscard]] EKind GetKind() const { return kind_; };
	[[nodiscard]] const std::vector<Alternative> &GetAlternatives() const { return alternatives_; };

 private:
	[[nodiscard]] bool IsMetByChar(const CharData *ch) const;

	EKind kind_;
	std::vector<Alternative> alternatives_;
	// The section listed alternatives, but none of them was valid.
	bool broken_{false};
};

template<typename Subject>
bool FeatConditions::IsMetBy(const Subject &subject) const {
	if (broken_) {
		return false;
	}
	if (alternatives_.empty()) {
		return true;
	}

	auto is_met = [&](const Alternative &alt) {
		for (const auto &cond : alt.skills) {
			if (subject.Skill(cond.id) < cond.min) {
				return false;
			}
		}
		for (const auto &cond : alt.feats) {
			if (subject.HasFeat(cond.id) == cond.absence) {
				return false;
			}
		}
		for (const auto &cond : alt.stats) {
			const int value = subject.Stat(cond.id);
			if (value < cond.min || (cond.has_above && value <= subject.Stat(cond.above))) {
				return false;
			}
		}
		for (const auto &cond : alt.levels) {
			int value = subject.Level();
			if (cond.remort_divisor > 0) {
				value += subject.Remorts() / cond.remort_divisor;
			}
			if (value < cond.min || (cond.has_max && value > cond.max)) {
				return false;
			}
		}
		return true;
	};
	for (const auto &alt : alternatives_) {
		if (is_met(alt)) {
			return true;
		}
	}
	return false;
}

} // namespace feats

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_CONDITIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
