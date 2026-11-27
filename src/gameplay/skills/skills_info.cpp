#include "engine/ui/color.h"
#include "engine/db/global_objects.h"

#include <stdexcept>

using DataNode = parser_wrapper::DataNode;
using ItemPtr = SkillInfoBuilder::ItemPtr;

template<>
const std::string &NAME_BY_ITEM<ESkillFlag>(const ESkillFlag item) {
	static const std::string kSelfTrain{"kSelfTrain"};
	static const std::string kHidden{"kHidden"};
	static const std::string kTrainInPeaceful{"kTrainInPeaceful"};
	static const std::string kTrainInHouse{"kTrainInHouse"};
	static const std::string kObligatory{"kObligatory"};
	static const std::string kKeepOnRemort{"kKeepOnRemort"};
	static const std::string kUnknown{"kUndefined"};
	switch (item) {
		case ESkillFlag::kSelfTrain: return kSelfTrain;
		case ESkillFlag::kHidden: return kHidden;
		case ESkillFlag::kTrainInPeaceful: return kTrainInPeaceful;
		case ESkillFlag::kTrainInHouse: return kTrainInHouse;
		case ESkillFlag::kObligatory: return kObligatory;
		case ESkillFlag::kKeepOnRemort: return kKeepOnRemort;
		case ESkillFlag::kCount: break;
	}
	return kUnknown;
}

template<>
ESkillFlag ITEM_BY_NAME<ESkillFlag>(const std::string &name) {
	for (int i = 0; i < static_cast<int>(ESkillFlag::kCount); ++i) {
		const auto flag = static_cast<ESkillFlag>(i);
		if (NAME_BY_ITEM<ESkillFlag>(flag) == name) {
			return flag;
		}
	}
	throw std::out_of_range(name);
}

namespace {

// " kHidden kObligatory", or " -" when no flag is set.
std::string SkillFlagsString(const BitsetFlags<ESkillFlag> &flags) {
	std::string out;
	for (int i = 0; i < static_cast<int>(ESkillFlag::kCount); ++i) {
		const auto flag = static_cast<ESkillFlag>(i);
		if (flags.get(flag)) {
			out += " " + NAME_BY_ITEM<ESkillFlag>(flag);
		}
	}
	return out.empty() ? " -" : out;
}

} // namespace

void SkillsLoader::Load(DataNode data) {
	MUD::Skills().Init(data.Children());
}

void SkillsLoader::Reload(DataNode data) {
	MUD::Skills().Reload(data.Children());
	CheckObligatorySkillsInClasses();
}

ItemPtr SkillInfoBuilder::Build(DataNode &node) {
	auto skill_info = ParseObligatoryValues(node);
	if (skill_info) {
		ParseDispensableValues(skill_info, node);
	}
	return skill_info;
}

void SkillInfoBuilder::ParseDispensableValues(ItemPtr &item_ptr, DataNode &node) {
	// issue.thing-names: name + abbr now live in skill_msg.xml (loaded before skills); read them from
	// the message container by id. Gameplay params stay in skills.xml.
	item_ptr->name = MUD::SkillMessages().GetName(item_ptr->GetId());
	item_ptr->short_name = MUD::SkillMessages().GetAbbr(item_ptr->GetId());
	try {
		item_ptr->save_type = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
		item_ptr->difficulty = parse::ReadAsInt(node.GetValue("difficulty"));
		item_ptr->cap = parse::ReadAsInt(node.GetValue("cap"));
		if (const char *v = node.GetValue("flags"); v && *v) {
			item_ptr->flags = parse::ReadAsConstantsFlags<ESkillFlag>(v);
		}
		// Устаревший вид флага kSelfTrain: пока читаем, но просим заменить.
		if (const char *v = node.GetValue("self_train"); v && *v) {
			err_log("skills.xml: у умения %s устаревший атрибут self_train, замените его на flags=\"kSelfTrain\".",
					NAME_BY_ITEM<ESkill>(item_ptr->GetId()).c_str());
			if (parse::ReadAsBool(v)) {
				item_ptr->flags.set(ESkillFlag::kSelfTrain);
			}
		}
	} catch (std::exception &e) {
		err_log("invalid skill description (incorrect value: %s). Setted by default;", e.what());
	}
}

ItemPtr SkillInfoBuilder::ParseObligatoryValues(DataNode &node) {
	auto id{ESkill::kUndefined};
	auto mode{EItemMode::kDisabled};
	try {
		id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
		mode = SkillInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	} catch (std::exception &e) {
		err_log("incorrect skill id (%s). ", e.what());
		return nullptr;
	}

	return std::make_shared<SkillInfo>(id, mode);
}

void SkillInfo::Print(std::ostringstream &buffer) const {
	buffer << "Print skill:" << "\r\n"
		   << " Id: " << kColorGrn << NAME_BY_ITEM<ESkill>(GetId()) << kColorNrm << "\r\n"
		   << " Name: " << kColorGrn << name << kColorNrm << "\r\n"
		   << " Abbreviation: " << kColorGrn << short_name << kColorNrm << "\r\n"
		   << " Save type: " << kColorGrn << NAME_BY_ITEM<ESaving>(save_type) << kColorNrm << "\r\n"
		   << " Difficulty: " << kColorGrn << difficulty << kColorNrm << "\r\n"
		   << " Skill cap: " << kColorGrn << cap << kColorNrm << "\r\n"
		   << " Flags:" << kColorGrn << SkillFlagsString(flags) << kColorNrm << "\r\n"
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
