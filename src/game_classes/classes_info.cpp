/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "classes_info.h"
#include "utils/parse.h"
#include "utils/pugixml.h"
#include "structs/global_objects.h"

namespace classes {

using DataNode = parser_wrapper::DataNode;
using Optional = CharClassInfoBuilder::ItemOptional;

void ClassesLoader::Load(DataNode data) {
	MUD::Classes().Init(data.Children());
}

void ClassesLoader::Reload(DataNode data) {
	MUD::Classes().Reload(data.Children());
}

bool CharClassInfo::HasSkill(ESkill skill_id) const {
	return skills->contains(skill_id);
};

int CharClassInfo::GetMinRemort(const ESkill skill_id) const {
	try {
		return skills->at(skill_id)->min_remort;
	} catch (const std::out_of_range &) {
		return kMaxRemort + 1;
	}
};

int CharClassInfo::GetMinLevel(const ESkill skill_id) const {
	try {
		return skills->at(skill_id)->min_level;
	} catch (const std::out_of_range &) {
		return kLevelImplementator;
	}
};

long CharClassInfo::GetImprove(const ESkill skill_id) const {
	try {
		return skills->at(skill_id)->improve;
	} catch (const std::out_of_range &) {
		return kMinImprove;
	}
};

Optional CharClassInfoBuilder::Build(DataNode &node) {
	auto class_info = MUD::Classes().MakeItemOptional();
	auto class_node = SelectDataNode(node);
	try {
		ParseClass(class_info, class_node);
	} catch (std::exception &e) {
		err_log("Incorrect value or id '%s' was detected.", e.what());
		class_info = std::nullopt;
	}
	return class_info;
}

DataNode CharClassInfoBuilder::SelectDataNode(DataNode &node) {
	auto file_name = GetCfgFileName(node);
	if (file_name && std::filesystem::exists(file_name.value())) {
		auto class_node = parser_wrapper::DataNode(file_name.value());
		if (class_node.IsNotEmpty()) {
			return class_node;
		}
	}
	return node;
}

std::optional<std::string> CharClassInfoBuilder::GetCfgFileName(DataNode &node) {
	const char *file_name;
	try {
		file_name = parse::ReadAsStr(node.GetValue("file"));
	} catch (std::exception &) {
		return std::nullopt;
	}
	std::optional<std::string> full_file_name{file_name};
	return full_file_name;
}

void CharClassInfoBuilder::ParseClass(Optional &info, DataNode &node) {
	try {
		info.value()->id = parse::ReadAsConstant<ECharClass>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect class id (%s).", e.what());
		info = std::nullopt;
		return;
	}
	node.GoToChild("skills");
	ParseSkills(info, node);
}

void CharClassInfoBuilder::ParseSkills(Optional &info, DataNode &node) {
	ParseSkillsLevelDecrement(info, node);

	for (auto &skill_node : node.Children("skill")) {
		ParseSingleSkill(info, skill_node);
	}
}

void CharClassInfoBuilder::ParseSkillsLevelDecrement(Optional &info, DataNode &node) {
	try {
		info.value()->skills_level_decrement = parse::ReadAsInt(node.GetValue("level_decrement"));
	} catch (std::exception &e) {
		err_log("Incorrect skill decrement (class %s), set default.",
				NAME_BY_ITEM<ECharClass>(info.value()->id).c_str());
	}
}

void CharClassInfoBuilder::ParseSingleSkill(Optional &info, DataNode &node) {
	auto skill_info = std::make_unique<ClassSkillInfo>();
	try {
		skill_info->id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect skill id (%s) in class %s.",
				e.what(), NAME_BY_ITEM<ECharClass>(info.value()->id).c_str());
		return;
	}
	ParseSkillVals(skill_info, node);
	info.value()->skills->try_emplace(skill_info->id, std::move(skill_info));
}

void CharClassInfoBuilder::ParseSkillVals(ClassSkillInfo::Ptr &info, DataNode &node) {
	try {
		info->min_level = parse::ReadAsInt(node.GetValue("level"));
		info->min_remort = parse::ReadAsInt(node.GetValue("remort"));
		info->improve = parse::ReadAsInt(node.GetValue("improve"));
	} catch (std::exception &) {
		err_log("Incorrect skill min level, remort or improve (skill: %s). Set by default.",
				NAME_BY_ITEM<ESkill>(info->id).c_str());
	};
};

} // namespace clases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
