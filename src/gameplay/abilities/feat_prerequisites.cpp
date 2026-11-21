#include "feat_prerequisites.h"

#include "engine/entities/char_data.h"
#include "utils/logger.h"
#include "utils/utils_parse.h"

#include <cstring>
#include <stdexcept>
#include <string>

namespace feats {

namespace {

template<typename T>
T ReadId(const char *id, const char *what) {
	try {
		return parse::ReadAsConstant<T>(id);
	} catch (const std::exception &) {
		throw std::runtime_error(std::string("unknown ") + what + " id " + id);
	}
}

// Reads one <prerequisite>; throws std::runtime_error naming the first invalid entry.
FeatPrerequisites::Alternative ParseAlternative(parser_wrapper::DataNode &alt_node, EFeat owner) {
	if (strcmp(alt_node.GetName(), "prerequisite") != 0) {
		throw std::runtime_error(std::string("unknown tag <") + alt_node.GetName() + ">");
	}

	FeatPrerequisites::Alternative alt;
	for (auto &cond : alt_node.Children()) {
		const std::string tag = cond.GetName();
		const char *id = cond.GetValue("id");
		if (!id || !*id) {
			throw std::runtime_error("<" + tag + "> without id");
		}
		if (tag == "skill") {
			FeatPrerequisites::SkillCondition skill;
			skill.id = ReadId<ESkill>(id, "skill");
			if (skill.id == ESkill::kUndefined) {
				throw std::runtime_error("skill id kUndefined");
			}
			const char *min = cond.GetValue("min");
			if (min && *min) {
				try {
					skill.min = parse::ReadAsInt(min);
				} catch (const std::exception &) {
					throw std::runtime_error(std::string("skill ") + id + ": bad min " + min);
				}
				if (skill.min < 1) {
					throw std::runtime_error(std::string("skill ") + id + ": min must be at least 1");
				}
			}
			alt.skills.push_back(skill);
		} else if (tag == "feat") {
			FeatPrerequisites::FeatCondition feat;
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
		} else {
			throw std::runtime_error("unknown tag <" + tag + ">");
		}
	}

	if (alt.skills.empty() && alt.feats.empty()) {
		throw std::runtime_error("empty <prerequisite>");
	}
	return alt;
}

} // namespace

void FeatPrerequisites::Build(parser_wrapper::DataNode &node, EFeat owner) {
	alternatives_.clear();
	broken_ = false;
	if (!node.GoToChild("prerequisites")) {
		return;
	}

	int dropped{0};
	for (auto &alt_node : node.Children()) {
		try {
			alternatives_.push_back(ParseAlternative(alt_node, owner));
		} catch (const std::exception &e) {
			err_log("Feat [%s]: invalid prerequisite dropped (%s).", NAME_BY_ITEM<EFeat>(owner).c_str(), e.what());
			++dropped;
		}
	}
	node.GoToParent();

	if (alternatives_.empty() && dropped > 0) {
		err_log("Feat [%s]: no valid prerequisite left, the feat cannot be learned.",
				NAME_BY_ITEM<EFeat>(owner).c_str());
		broken_ = true;
	}
}

bool FeatPrerequisites::IsMet(const SkillLookup &skill, const FeatLookup &has_feat) const {
	if (broken_) {
		return false;
	}
	if (alternatives_.empty()) {
		return true;
	}

	for (const auto &alt : alternatives_) {
		bool met{true};
		for (const auto &cond : alt.skills) {
			if (skill(cond.id) < cond.min) {
				met = false;
				break;
			}
		}
		for (const auto &cond : alt.feats) {
			if (!met) {
				break;
			}
			if (has_feat(cond.id) == cond.absence) {
				met = false;
			}
		}
		if (met) {
			return true;
		}
	}
	return false;
}

bool FeatPrerequisites::IsMet(const CharData *ch) const {
	return IsMet(
		[ch](ESkill id) { return GetSkillWithoutEquip(ch, id); },
		[ch](EFeat id) { return ch->HaveFeat(id); });
}

} // namespace feats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
