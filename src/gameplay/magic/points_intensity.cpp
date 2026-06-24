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
		kCategoryNames{"heal", "moves", "thirst", "full", "damage"};

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
// Sorts the result descending by percent so Resolve's single linear walk hits
// the highest tier whose threshold the input has reached, in either direction.
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
	// One sort direction (descending by signed percent) serves both improve and
	// degrade tables: improve thresholds are positive so descending = strongest
	// first; degrade thresholds are negative so descending puts the LEAST
	// negative (weakest) first. Resolve walks this order and returns the first
	// tier the input has reached (largest T such that T <= input), which yields
	// the strongest crossed threshold in either direction. (issue.point-bugs #5
	// retired the separate std::reverse for degrade.)
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
			// issue.point-bugs #5: keep the descending sort produced by
			// ParseDirection. Resolve uses the same "largest T such that
			// T <= input" walk for both improve and degrade.
			ParseDirection(node, cat.degrade);
			node.GoToParent();
		}
		node.GoToParent();
	}
}
const std::vector<std::pair<std::string, std::string>> PointsIntensity::ShowHelpDamage() const {
	std::pair<std::string, std::string> dam;
	std::vector<std::pair<std::string, std::string>>  out_damage;
	int min_val = 1;
	for (const auto &g : reverse(categories_[static_cast<size_t>(ECategory::kDamage)].improve)) {
		dam.first = g.text;
		dam.second = std::to_string(min_val) + ".." + std::to_string(g.percent);
		out_damage.push_back(dam);
		min_val = g.percent + 1;
	}
	return out_damage;
}

const std::string &PointsIntensity::Resolve(ECategory category, int percent) const {
	static const std::string kEmpty;
	if (category >= ECategory::kCount) {
		return kEmpty;
	}
	const auto &cat = categories_[static_cast<size_t>(category)];
	// Unified algorithm (issue.point-bugs #5): walk descending and return the
	// first tier T such that T <= input. That picks the largest signed
	// threshold the input has reached, which is the strongest crossed tier
	// in both directions:
	//   improve (input >= 0): input=35 matches tier 35 (T=35 <= 35), input=80
	//     matches tier 75 (T=99 fails, T=75 wins).
	//   degrade (input <  0): input=-38 matches tier -40 (T in {-20,-40,...},
	//     -20 fails since -20 > -38, -40 wins since -40 <= -38).
	// An input below the weakest threshold returns an empty string -- callers
	// (CastToPoints) treat that as "no narration this category."
	const auto &table = (percent >= 0) ? cat.improve : cat.degrade;

	if (category == ECategory::kDamage) {  //тут  цифры относительны
		for (const auto &g : reverse(table)) {
			if (percent <= g.percent) {
				return g.text;
			}
		}
	} else {
		for (const auto &g : table) {
			if (g.percent <= percent) {
				return g.text;
			}
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
