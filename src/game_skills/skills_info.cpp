#include "color.h"
#include "structs/global_objects.h"
#include "utils/parse.h"

struct AttackMessages fight_messages[kMaxMessages];

using DataNode = parser_wrapper::DataNode;
using Optional = SkillInfoBuilder::ItemOptional;

void SkillsLoader::Load(DataNode data) {
	MUD::Skills().Init(data.Children());
}

void SkillsLoader::Reload(DataNode data) {
	MUD::Skills().Reload(data.Children());
}

Optional SkillInfoBuilder::Build(DataNode &node) {
	auto skill_info = MUD::Skills().MakeItemOptional();
	skill_info = std::move(ParseDispensableValues(skill_info, node));
	skill_info = std::move(ParseObligatoryValues(skill_info, node));
	return skill_info;
}

Optional &SkillInfoBuilder::ParseDispensableValues(Optional &optional, DataNode &node) {
	try {
		optional.value()->difficulty = parse::ReadAsInt(node.GetValue("difficulty"));
	} catch (std::exception &) {}
	try {
		optional.value()->save_type = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
	} catch (std::exception &) {}
	try {
		optional.value()->mode = parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
		optional.value()->mode = EItemMode::kEnabled;
	}
	return optional;
}

Optional &SkillInfoBuilder::ParseObligatoryValues(Optional &optional, DataNode &node) {
	try {
		optional.value()->id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
		optional.value()->name = parse::ReadAsStr(node.GetValue("name"));
		optional.value()->short_name = parse::ReadAsStr(node.GetValue("abbr"));
		optional.value()->cap = parse::ReadAsInt(node.GetValue("cap"));
	} catch (std::exception &e) {
		err_log("invalid skill description (incorrect value: %s).", e.what());
		optional = std::nullopt;
	}
	return optional;
}

void SkillInfo::Print(std::stringstream &buffer) const {
	buffer << "Print skill:" << "\n"
		   << " Id: " << KGRN << NAME_BY_ITEM<ESkill>(id) << KNRM << "\n"
		   << " Name: " << KGRN << name << KNRM << "\n"
		   << " Abbreviation: " << KGRN << short_name << KNRM << "\n"
		   << " Save type: " << KGRN << NAME_BY_ITEM<ESaving>(save_type) << KNRM << "\n"
		   << " Difficulty: " << KGRN << difficulty << KNRM << "\n"
		   << " Skill cap: " << KGRN << cap << KNRM << "\n"
		   << " Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(mode) << KNRM << std::endl;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
