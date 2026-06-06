// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
/**
\file points_intensity.h - a part of the Bylins engine.
\authors Created for issue.mag-points.
\brief Intensity-grade vocabulary for CastToPoints narration.
\details Loads lib/cfg/messages/ru/points_intensity.xml at boot. Each of the four
		 restoration categories (heal / moves / thirst / full) carries
		 <improve> and <degrade> lists of <grade percent= text="..."/>
		 entries; given a computed intensity percentage, Resolve() returns
		 the text of the highest tier whose threshold the percentage has
		 reached.

		 Lookup pseudo-code: walk grades sorted descending by signed
		 percent; return the first whose `percent` is <= the input
		 (issue.point-bugs #5 -- unified for improve and degrade, was
		 strict < with a special-case reverse for degrade).

		 The returned text is fed through act() so suffix codes like $h /
		 $r / $g resolve against the target's gender on emission.
*/

#ifndef BYLINS_GAMEPLAY_MAGIC_POINTS_INTENSITY_H_
#define BYLINS_GAMEPLAY_MAGIC_POINTS_INTENSITY_H_

#include "engine/boot/cfg_manager.h"
#include "utils/parser_wrapper.h"

#include <array>
#include <string>
#include <vector>

namespace points_intensity {

// One restoration category in the table. Indexed by the order in which the
// XML's <heal>/<moves>/<thirst>/<full> sections appear -- ECategory is the
// canonical key used in the public API.
enum class ECategory {
	kHeal   = 0,
	kMoves  = 1,
	kThirst = 2,
	kFull   = 3,   // hunger axis (engine COND[FULL]; XML tag <full>, renamed from <cond>
	               // in issue.point-bugs to free "cond" for the overall-state triplet).
	kDamage = 4,   // weapon-hit narration (issue.mag-points step 2: hit_msg.xml
	               // grading retired). Percentage formula = damage * 100 / striker's
	               // max HP (every viewer sees the same scale). Top tier crosses 150%.
	kCount,
};

struct Grade {
	int percent{0};      // lower bound (inclusive) -- tier matches when percent <= input
	                     // (issue.point-bugs #5: was "strict <", changed to "<=" so input
	                     // exactly at the threshold maps to that threshold's tier).
	std::string text;    // act()-ready string, may contain $h/$r/etc. suffix codes.
};

class PointsIntensity {
 public:
	PointsIntensity();

	// Replace the entire table from a parsed <points_intensity> node. Each
	// category's <improve>/<degrade> grades are stored sorted descending by
	// percent (so iterating tier-first picks the highest matching slot).
	void Reload(const parser_wrapper::DataNode &node);

	// Returns the text of the matching grade or an empty string when nothing
	// matches (e.g. percent below the lowest configured tier, or a category
	// with no grades configured). The returned string is owned by this
	// container -- callers must not mutate it; copying is fine.
	[[nodiscard]] const std::string &Resolve(ECategory category, int percent) const;

	// True when the table holds at least one grade for the given category /
	// direction. Useful for the CastToPoints "should I bother substituting"
	// short-circuit.
	[[nodiscard]] bool HasImprove(ECategory category) const;
	[[nodiscard]] bool HasDegrade(ECategory category) const;

 private:
	struct CategoryData {
		std::vector<Grade> improve;  // sorted descending by percent
		std::vector<Grade> degrade;  // sorted ascending by percent (most negative first)
	};
	std::array<CategoryData, static_cast<size_t>(ECategory::kCount)> categories_;
};

class PointsIntensityLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace points_intensity

#endif // BYLINS_GAMEPLAY_MAGIC_POINTS_INTENSITY_H_
