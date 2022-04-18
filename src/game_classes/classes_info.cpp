/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "classes_info.h"

#include "color.h"
#include "utils/parse.h"
#include "utils/pugixml/pugixml.h"
#include "game_magic/spells_info.h"
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
	try {
		info.value()->mode = parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
	}
	node.GoToChild("name");
	ParseName(info, node);
	node.GoToSibling("skills");
	ParseSkills(info, node);
	node.GoToSibling("spells");
	ParseSpells(info, node);
}

void CharClassInfoBuilder::ParseName(Optional &info, DataNode &node) {
	try {
		info.value()->abbr = parse::ReadAsStr(node.GetValue("abbr"));
	} catch (std::exception &) {
		info.value()->abbr = "--";
	}
	info.value()->names = base_structs::ItemName::Build(node);
}

void CharClassInfoBuilder::ParseSkills(Optional &info, DataNode &node) {
	info.value()->skill_level_decrement_ = ParseLevelDecrement(info.value()->id, node);
	info.value()->skills.Init(node.Children());
}

void CharClassInfoBuilder::ParseSpells(Optional &info, DataNode &node) {
	info.value()->spell_level_decrement_ = ParseLevelDecrement(info.value()->id, node);
	info.value()->spells.Init(node.Children());
}

int CharClassInfoBuilder::ParseLevelDecrement(ECharClass class_id, DataNode &node) {
	try {
		return parse::ReadAsInt(node.GetValue("level_decrement"));
	} catch (std::exception &e) {
		err_log("Incorrect skill/spell level decrement (class %s), set by default.",
				NAME_BY_ITEM<ECharClass>(class_id).c_str());
		return kMinTalentLevelDecrement;
	}
}

void CharClassInfo::Print(std::stringstream &buffer) const {
	buffer << "Print class:" << "\n"
		   << "    Id: " << KGRN << NAME_BY_ITEM<ECharClass>(id) << KNRM << std::endl
		   << "    Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(mode) << KNRM << std::endl
		   << "    Abbr: " << KGRN << GetAbbr() << KNRM << std::endl
		   << "    Name: " << KGRN << GetName()
		   << "/" << names->GetSingular(ECase::kGen)
		   << "/" << names->GetSingular(ECase::kDat)
		   << "/" << names->GetSingular(ECase::kAcc)
		   << "/" << names->GetSingular(ECase::kIns)
		   << "/" << names->GetSingular(ECase::kPre) << KNRM << std::endl
		   << "    Available skills (level decrement " << GetSkillLvlDecrement() << "):" << std::endl;
	for (const auto &skill : skills) {
		skill.Print(buffer);
	}
	buffer	<< "    Available spells (level decrement " << GetSkillLvlDecrement() << "):" << std::endl;
	for (const auto &spell : spells) {
		spell.Print(buffer);
	}
	buffer << std::endl;
}

const std::string &CharClassInfo::GetAbbr() const {
	return abbr;
};

const std::string &CharClassInfo::GetName(ECase name_case) const {
	return names->GetSingular(name_case);
}

const std::string &CharClassInfo::GetPluralName(ECase name_case) const {
	return names->GetPlural(name_case);
}

const char *CharClassInfo::GetCName(ECase name_case) const {
	return names->GetSingular(name_case).c_str();
}

const char *CharClassInfo::GetPluralCName(ECase name_case) const {
	return names->GetPlural(name_case).c_str();
}

void CharClassInfo::SkillInfo::Print(std::stringstream &buffer) const {
	buffer << KNRM << "        Skill: " << KCYN << MUD::Skills()[id_].name
		<< KNRM << " level: " << KGRN << min_level_ << KNRM
		<< KNRM << " remort: " << KGRN << min_remort_ << KNRM
		<< KNRM << " improve: " << KGRN << improve_ << KNRM
		<< KNRM << " mode: " << KGRN << NAME_BY_ITEM<EItemMode>(mode_) << KNRM << std::endl;
}

CharClassInfo::SkillsInfoBuilder::ItemOptional ParseSingleSkill(DataNode &node) {
	auto id{ESkill::kIncorrect};
	try {
		id = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect skill id (%s).", e.what());
		return std::nullopt;
	}
	auto mode{EItemMode::kEnabled};
	try {
		mode = parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
	}
	int min_lvl, min_remort;
	long improve;
	try {
		min_lvl = parse::ReadAsInt(node.GetValue("level"));
		min_remort = parse::ReadAsInt(node.GetValue("remort"));
		improve = parse::ReadAsInt(node.GetValue("improve"));
	} catch (std::exception &) {
		err_log("Incorrect skill min level, remort or improve (skill: %s). Set by default.",
				NAME_BY_ITEM<ESkill>(id).c_str());
	};

	// \todo Нужно подумать, как переделать интерфейс, возможно - через ссылку на метод или лямбду
	return std::make_optional(std::make_shared<CharClassInfo::SkillInfo>(id, min_lvl, min_remort, improve, mode));
}

CharClassInfo::SkillsInfoBuilder::ItemOptional CharClassInfo::SkillsInfoBuilder::Build(DataNode &node) {
	try {
		return ParseSingleSkill(node);
	} catch (std::exception &e) {
		err_log("Incorrect value or id '%s' was detected.", e.what());
		return std::nullopt;
	}
}

void CharClassInfo::SpellInfo::Print(std::stringstream &buffer) const {
	buffer << KNRM << "        Spell: " << KCYN << spell_info[id_].name
			<< KNRM << " circle: " << KGRN << circle_ << KNRM
			<< KNRM << " level: " << KGRN << min_level_ << KNRM
			<< KNRM << " remort: " << KGRN << min_remort_ << KNRM
			<< KNRM << " mem mod: " << KGRN << mem_mod_ << KNRM
			<< KNRM << " cast mod: " << KGRN << cast_mod_ << KNRM
			<< KNRM << " mode: " << KGRN << NAME_BY_ITEM<EItemMode>(mode_) << KNRM << std::endl;
}

CharClassInfo::SpellsInfoBuilder::ItemOptional ParseSingleSpell(DataNode &node) {
	auto id{ESpell::kIncorrect};
	try {
		id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect spell id (%s).", e.what());
		return std::nullopt;
	}
	auto mode{EItemMode::kEnabled};
	try {
		mode = parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
	}
	int min_lvl, min_remort, circle, mem, cast;
	try {
		min_lvl = parse::ReadAsInt(node.GetValue("level"));
		min_remort = parse::ReadAsInt(node.GetValue("remort"));
		circle = parse::ReadAsInt(node.GetValue("circle"));
		mem = parse::ReadAsInt(node.GetValue("mem"));
		cast = parse::ReadAsInt(node.GetValue("cast"));
	} catch (std::exception &) {
		err_log("Incorrect skill min level, remort or improve (skill: %s). Set by default.",
				NAME_BY_ITEM<ESpell>(id).c_str());
	};

	// \todo Нужно подумать, как переделать интерфейс, возможно - через ссылку на метод или лямбду
	return std::make_optional(std::make_shared<CharClassInfo::SpellInfo>(id, min_lvl, min_remort, circle, mem, cast, mode));
}

CharClassInfo::SpellsInfoBuilder::ItemOptional CharClassInfo::SpellsInfoBuilder::Build(DataNode &node) {
	try {
		return ParseSingleSpell(node);
	} catch (std::exception &e) {
		err_log("Incorrect value or id '%s' was detected.", e.what());
		return std::nullopt;
	}
}

} // namespace clases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
