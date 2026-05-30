// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
// issue.mag-points: intensity-grade vocabulary loader.

#include "points_intensity.h"

#include "engine/db/global_objects.h"
#include "utils/logger.h"
#include "utils/parse.h"

#include <algorithm>

namespace points_intensity {

namespace {

// XML category names lined up with the ECategory enum. Indexed by the
// ECategory underlying value so the load loop stays trivial.
constexpr std::array<const char *, static_cast<size_t>(ECategory::kCount)>
		kCategoryNames{"heal", "moves", "thirst", "cond"};

// Parse a <grade percent= text=/> element into a Grade. Skips entries with
// no text -- the table only stores actionable rows.
bool ParseGrade(parser_wrapper::DataNode &node, Grade &out) {
	const char *percent = node.GetValue("percent");
	const char *text = node.GetValue("text");
	if (!text || !*text) {
		return false;
	}
	out.percent = (percent && *percent) ? parse::ReadAsInt(percent) : 0;
	out.text = text;
	return true;
}

// Parse all <grade .../> children of `direction_node` into `grades`. The
// direction_node should already be positioned at <improve> or <degrade>.
// Sorts the result descending by percent (so the highest tier matches first
// in the linear walk that Resolve uses).
void ParseDirection(parser_wrapper::DataNode direction_node, std::vector<Grade> &grades) {
	grades.clear();
	for (auto &child : direction_node.Children()) {
		if (strcmp(child.GetName(), "grade") != 0) {
			continue;
		}
		Grade g;
		if (ParseGrade(child, g)) {
			grades.push_back(std::move(g));
		}
	}
	// Descending sort for improve (positive percents); for degrade the natural
	// "most negative first" order matches a descending sort by `percent` too
	// (-99 > -75 > -50 from the int-comparison standpoint is FALSE -- -99 is
	// smaller than -75). So degrade needs ASCENDING sort instead.
	// We don't know which list this is at parse time; the caller flips it.
	std::sort(grades.begin(), grades.end(),
		[](const Grade &a, const Grade &b) { return a.percent > b.percent; });
}

} // namespace

PointsIntensity::PointsIntensity() = default;

void PointsIntensity::Reload(const parser_wrapper::DataNode &node_in) {
	// DataNode::GoToChild mutates the node; take a local copy so we don't
	// disturb the caller's traversal cursor.
	parser_wrapper::DataNode node = node_in;

	for (size_t cat_idx = 0; cat_idx < static_cast<size_t>(ECategory::kCount); ++cat_idx) {
		auto &cat = categories_[cat_idx];
		cat.improve.clear();
		cat.degrade.clear();
		if (!node.GoToChild(kCategoryNames[cat_idx])) {
			continue;
		}
		if (node.GoToChild("improve")) {
			ParseDirection(node, cat.improve);
			node.GoToParent();
		}
		if (node.GoToChild("degrade")) {
			ParseDirection(node, cat.degrade);
			// Degrade grades use negative percent values; the descending sort
			// from ParseDirection puts least-negative first. Reverse to get
			// most-negative-first so Resolve picks the strongest tier the
			// input drops below.
			std::reverse(cat.degrade.begin(), cat.degrade.end());
			node.GoToParent();
		}
		node.GoToParent();
	}
}

const std::string &PointsIntensity::Resolve(ECategory category, int percent) const {
	static const std::string kEmpty;
	if (category >= ECategory::kCount) {
		return kEmpty;
	}
	const auto &cat = categories_[static_cast<size_t>(category)];
	if (percent >= 0) {
		// Improve table is sorted DESCENDING. First grade whose threshold the
		// input strictly exceeds wins -- so >99 matches the >99 tier, 75
		// matches the >50 tier (since 75 > 50 but not > 75), etc.
		for (const auto &g : cat.improve) {
			if (percent > g.percent) {
				return g.text;
			}
		}
		return kEmpty;
	}
	// Degrade table is sorted MOST-NEGATIVE-FIRST. First grade whose threshold
	// the input strictly subverts wins -- so -99 matches the <-99 tier, -50
	// matches the <-25 tier.
	for (const auto &g : cat.degrade) {
		if (percent < g.percent) {
			return g.text;
		}
	}
	return kEmpty;
}

bool PointsIntensity::HasImprove(ECategory category) const {
	if (category >= ECategory::kCount) {
		return false;
	}
	return !categories_[static_cast<size_t>(category)].improve.empty();
}

bool PointsIntensity::HasDegrade(ECategory category) const {
	if (category >= ECategory::kCount) {
		return false;
	}
	return !categories_[static_cast<size_t>(category)].degrade.empty();
}

void PointsIntensityLoader::Load(parser_wrapper::DataNode data) {
	MUD::PointsIntensity().Reload(data);
}

void PointsIntensityLoader::Reload(parser_wrapper::DataNode data) {
	// The user explicitly noted hot-reload is not needed, but registering a
	// Reload that does the same as Load lets a god still trigger a refresh
	// via `/reload points_intensity` if desired.
	MUD::PointsIntensity().Reload(data);
}

} // namespace points_intensity
