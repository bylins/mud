/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "classes_info.h"

#include <filesystem>

#include "boot/boot_constants.h"
#include "logger.h"
#include "utils/pugixml.h"

using pugi::xml_node;
extern pugi::xml_node XMLLoad(const std::string &PathToFile, const std::string &MainTag, const std::string &ErrorStr, pugi::xml_document &Doc);

namespace classes {

const bool kStrictParsing = true;
constexpr bool kTolerantParsing = !kStrictParsing;

const std::string CharClassInfo::cfg_file_name{LIB_CFG_CLASSES "pc_classes.xml"};
const std::string CharClassInfo::xml_main_tag{"classes"};
const std::string CharClassInfo::xml_entity_tag{"class"};
const std::string CharClassInfo::load_fail_msg{"PC classes loading failed. The cfg file is missing or damaged."};

bool ClassesInfo::RegisterBuilder::strict_pasring_{true};

template<typename T>
T FindConstantByAttributeValue(const char *attribute_name, const xml_node &node) {
	auto constants_name = node.attribute(attribute_name).value();
	try {
		return ITEM_BY_NAME<T>(constants_name);
	} catch (const std::out_of_range &) {
		throw std::runtime_error(constants_name);
	}
}

void ClassesInfo::Init() {
	CharClassInfo CLassInfo;
	items_ = std::move(RegisterBuilder::Build(kTolerantParsing).value());
}

void ClassesInfo::Reload(const std::string &arg) {
	auto new_items = RegisterBuilder::Build(kStrictParsing);
	if (new_items) {
		items_ = std::move(new_items.value());
	} else {
		err_log("%s reloading canceled: file damaged.", arg.c_str());
	}
}

ClassesInfo::Optional ClassesInfo::RegisterBuilder::Build(bool strict_parsing) {
	strict_pasring_ = strict_parsing;
	pugi::xml_document doc;
	auto nodes = XMLLoad(CharClassInfo::cfg_file_name, CharClassInfo::xml_main_tag, CharClassInfo::load_fail_msg, doc);
	if (nodes.empty()) {
		return std::nullopt;
	}
	return Parse(nodes, CharClassInfo::xml_entity_tag);
}

ClassesInfo::Optional ClassesInfo::RegisterBuilder::Parse(const xml_node &nodes, const std::string &tag) {
	auto items = std::make_optional(std::make_unique<Register>());
	CharClassInfo::Optional item;
	for (auto &node : nodes.children(tag.c_str())) {
		item = ItemBuilder::Build(node);
		if (item) {
			EmplaceItem(items, item);
		} else if (strict_pasring_) {
			return std::nullopt;
		}
	}
	EmplaceDefaultItems(items);

	return items;
}

void ClassesInfo::RegisterBuilder::EmplaceItem(ClassesInfo::Optional &items, CharClassInfo::Optional &item) {
	auto it = items.value()->try_emplace(item.value().first, std::move(item.value().second));
	if (!it.second) {
		err_log("Item '%s' has already exist. Redundant definition had been ignored.\n",
				NAME_BY_ITEM<ECharClass>(item.value().first).c_str());
	}
}

void ClassesInfo::RegisterBuilder::EmplaceDefaultItems(ClassesInfo::Optional &items) {
	items.value()->try_emplace(ECharClass::kUndefined, std::make_unique<CharClassInfo>());
}

const CharClassInfo &ClassesInfo::operator[](ECharClass id) const {
	try {
		return *items_->at(id);
	} catch (const std::out_of_range &) {
		err_log("Unknown id (%d) passed into %s.", to_underlying(id), typeid(this).name());
		return *items_->at(ECharClass::kUndefined);
	}
}

bool CharClassInfo::IsKnown(const ESkill id) const {
	return skillls_->contains(id);
};

int CharClassInfo::GetMinRemort(const ESkill id) const {
	try {
		return skillls_->at(id)->min_remort;
	} catch (const std::out_of_range &) {
		return kMaxRemort + 1;
	}
};

int CharClassInfo::GetMinLevel(const ESkill id) const {
	try {
		return skillls_->at(id)->min_level;
	} catch (const std::out_of_range &) {
		return kLevelImplementator;
	}
};

long CharClassInfo::GetImprove(const ESkill id) const {
	try {
		return skillls_->at(id)->improve;
	} catch (const std::out_of_range &) {
		return kMinImprove;
	}
};

/*
 *  Строитель информации одного отдельного класса.
 */
CharClassInfo::Optional CharClassInfoBuilder::Build(const xml_node &node) {
	auto class_node = SelectXmlNode(node);
	auto class_info = std::make_optional(CharClassInfo::Pair());
	try {
		ParseXml(class_info, class_node);
	} catch (std::exception &e) {
		err_log("Incorrect value or id '%s' was detected.", e.what());
		class_info = std::nullopt;
	}
	return class_info;
}

xml_node CharClassInfoBuilder::SelectXmlNode(const xml_node &node) {
	auto file_name = GetCfgFileName(node);
	if (file_name && std::filesystem::exists(file_name.value())) {
		pugi::xml_document doc;
		auto class_node = XMLLoad(file_name.value(), "class", "...fail.", doc);
		if (!class_node.empty()) {
			return class_node;
		}
	}
	return node;
}

std::optional<std::string> CharClassInfoBuilder::GetCfgFileName(const xml_node &node) {
	auto file_name = node.attribute("file").value();
	if (!*file_name) {
		return std::nullopt;
	}
	std::optional<std::string> full_file_name{LIB_CFG_CLASSES};
	return (full_file_name.value() += file_name);
}

void CharClassInfoBuilder::ParseXml(CharClassInfo::Optional &info, const xml_node &node) {
	info.value().first = FindConstantByAttributeValue<ECharClass>("id", node);
	info.value().second = std::make_unique<CharClassInfo>();
	ParseSkills(info.value().second, node.child("skills"));
}

void CharClassInfoBuilder::ParseSkills(CharClassInfo::Ptr &info, const xml_node &nodes) {
	info->skills_level_decrement_ = nodes.attribute("level_decrement").as_int();
	for (auto &node : nodes.children("skill")) {
		auto skill_id = FindConstantByAttributeValue<ESkill>("id", node);
		auto skill_info = std::make_unique<ClassSkillInfo>();
		ParseSingleSkill(skill_info, node);
		info->skillls_->try_emplace(skill_id, std::move(skill_info));
	}
}

void CharClassInfoBuilder::ParseSingleSkill(ClassSkillInfo::Ptr &info, const xml_node &node) {
	info->min_level = node.attribute("level").as_int();
	info->min_remort = node.attribute("remort").as_int();
	info->improve =  node.attribute("improve").as_int();
};

} // namespace clases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
