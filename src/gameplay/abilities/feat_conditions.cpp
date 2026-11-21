#include "feat_conditions.h"

#include "engine/entities/char_data.h"
#include "utils/logger.h"
#include "utils/utils_parse.h"

#include <cstring>
#include <stdexcept>
#include <string>

namespace feats {

namespace {

const char *SectionTag(FeatConditions::EKind kind) {
	return kind == FeatConditions::EKind::kPrerequisites ? "prerequisites" : "requirements";
}

const char *AlternativeTag(FeatConditions::EKind kind) {
	return kind == FeatConditions::EKind::kPrerequisites ? "prerequisite" : "requirement";
}

template<typename T>
T ReadId(const char *id, const char *what) {
	try {
		return parse::ReadAsConstant<T>(id);
	} catch (const std::exception &) {
		throw std::runtime_error(std::string("unknown ") + what + " id " + id);
	}
}

// Reads the optional positive `min` of <skill>/<stat>; returns false if there is none.
bool ReadMin(parser_wrapper::DataNode &cond, const char *tag, const char *id, int &out) {
	const char *min = cond.GetValue("min");
	if (!min || !*min) {
		return false;
	}
	try {
		out = parse::ReadAsInt(min);
	} catch (const std::exception &) {
		throw std::runtime_error(std::string(tag) + " " + id + ": bad min " + min);
	}
	if (out < 1) {
		throw std::runtime_error(std::string(tag) + " " + id + ": min must be at least 1");
	}
	return true;
}

// Reads one alternative; throws std::runtime_error naming the first invalid entry.
FeatConditions::Alternative ParseAlternative(parser_wrapper::DataNode &alt_node,
											 FeatConditions::EKind kind, EFeat owner) {
	if (strcmp(alt_node.GetName(), AlternativeTag(kind)) != 0) {
		throw std::runtime_error(std::string("unknown tag <") + alt_node.GetName() + ">");
	}

	FeatConditions::Alternative alt;
	for (auto &cond : alt_node.Children()) {
		const std::string tag = cond.GetName();
		const char *id = cond.GetValue("id");
		if (!id || !*id) {
			throw std::runtime_error("<" + tag + "> without id");
		}
		if (tag == "skill") {
			FeatConditions::SkillCondition skill;
			skill.id = ReadId<ESkill>(id, "skill");
			if (skill.id == ESkill::kUndefined) {
				throw std::runtime_error("skill id kUndefined");
			}
			ReadMin(cond, "skill", id, skill.min);
			alt.skills.push_back(skill);
		} else if (tag == "feat") {
			FeatConditions::FeatCondition feat;
			feat.id = ReadId<EFeat>(id, "feat");
			if (feat.id == EFeat::kUndefined) {
				throw std::runtime_error("feat id kUndefined");
			}
			const char *absence = cond.GetValue("absence");
			if (absence && *absence) {
				feat.absence = parse::ReadAsBool(absence);
			}
			if (feat.id == owner && !feat.absence) {
				throw std::runtime_error("the feat requires itself");
			}
			alt.feats.push_back(feat);
		} else if (tag == "stat") {
			FeatConditions::StatCondition stat;
			stat.id = ReadId<EBaseStat>(id, "stat");
			const bool has_min = ReadMin(cond, "stat", id, stat.min);
			const char *above = cond.GetValue("above");
			if (above && *above) {
				stat.above = ReadId<EBaseStat>(above, "stat");
				stat.has_above = true;
				if (stat.above == stat.id) {
					throw std::runtime_error(std::string("stat ") + id + " is compared with itself");
				}
			}
			if (!has_min && !stat.has_above) {
				throw std::runtime_error(std::string("stat ") + id + ": needs min or above");
			}
			alt.stats.push_back(stat);
		} else {
			throw std::runtime_error("unknown tag <" + tag + ">");
		}
	}

	if (alt.skills.empty() && alt.feats.empty() && alt.stats.empty()) {
		throw std::runtime_error(std::string("empty <") + AlternativeTag(kind) + ">");
	}
	return alt;
}

int GetStat(const CharData *ch, EBaseStat id) {
	switch (id) {
		case EBaseStat::kStr: return GetRealStr(ch);
		case EBaseStat::kDex: return GetRealDex(ch);
		case EBaseStat::kCon: return GetRealCon(ch);
		case EBaseStat::kWis: return GetRealWis(ch);
		case EBaseStat::kInt: return GetRealInt(ch);
		case EBaseStat::kCha: return GetRealCha(ch);
	}
	return 0;
}

} // namespace

void FeatConditions::Build(parser_wrapper::DataNode &node, EFeat owner) {
	alternatives_.clear();
	broken_ = false;
	if (!node.GoToChild(SectionTag(kind_))) {
		return;
	}

	int dropped{0};
	for (auto &alt_node : node.Children()) {
		try {
			alternatives_.push_back(ParseAlternative(alt_node, kind_, owner));
		} catch (const std::exception &e) {
			err_log("Feat [%s]: invalid <%s> dropped (%s).",
					NAME_BY_ITEM<EFeat>(owner).c_str(), AlternativeTag(kind_), e.what());
			++dropped;
		}
	}
	node.GoToParent();

	if (alternatives_.empty() && dropped > 0) {
		err_log("Feat [%s]: no valid <%s> left, the conditions can never be met.",
				NAME_BY_ITEM<EFeat>(owner).c_str(), AlternativeTag(kind_));
		broken_ = true;
	}
}

bool FeatConditions::IsMetBy(const CharData *ch) const {
	auto has_feat = [ch](EFeat id) { return ch->HaveFeat(id); };
	auto stat = [ch](EBaseStat id) { return GetStat(ch, id); };
	if (kind_ == EKind::kPrerequisites) {
		return IsMet([ch](ESkill id) { return GetSkillWithoutEquip(ch, id); }, has_feat, stat);
	}
	return IsMet([ch](ESkill id) { return GetSkill(ch, id); }, has_feat, stat);
}

} // namespace feats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
