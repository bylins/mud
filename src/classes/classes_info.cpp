/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "classes_info.h"

#include "boot/boot_constants.h"
#include "logger.h"
#include "utils/pugixml.h"

using pugi::xml_node;
extern xml_node XMLLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document &Doc);

namespace classes {

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
	// todo сделать строгий/нестрогий парсинг
	//ClassesInfo::ClassesInfoBuilder builder;
	items_ = std::move(RegisterBuilder::Build().value());
}

void ClassesInfo::Reload(std::string &arg) {
	auto new_items = RegisterBuilder::Build();
	if (new_items) {
		items_ = std::move(new_items.value());
	} else {
		err_log("%s reloading canceled: file damaged.", arg.c_str());
	}
}

ClassesInfo::Optional ClassesInfo::RegisterBuilder::Build() {

	const std::string kPath{LIB_CFG};
	const std::string kFileName{"pc_classes.xml"};

	const char *kMainTag = "classes";
	const char *kItemTag = "class";
	const char *kFailMsg = "PC classes loading failed. The cfg file is missing or damaged.";

	std::string kFullFileName = kPath + kFileName;
	pugi::xml_document doc;
	xml_node nodes = XMLLoad(kFullFileName.c_str(), kMainTag, kFailMsg, doc);
	if (nodes.empty()) {
		return std::nullopt;
	}
	auto items = std::make_optional(std::make_unique<Register>());
	for (auto &node : nodes.children(kItemTag)) {
		try {
			auto item = ItemBuilder::Build(node);
			auto it = items.value()->try_emplace(item.value().first, std::move(item.value().second));
			if (!it.second) {
				err_log("Item '%s' has already exist. Redundant definition had been ignored.\n",
						NAME_BY_ITEM<ECharClass>(item.value().first).c_str());
			}
		} catch (std::exception &e) {
			err_log("Incorrect value or id '%s' was detected.", e.what());
			return std::nullopt;
		}
	}
	items.value()->try_emplace(ECharClass::kUndefined, std::make_unique<CharClassInfo>());
	items.value()->try_emplace(ECharClass::kMob, std::make_unique<CharClassInfo>());
	items.value()->try_emplace(ECharClass::kNpcBase, std::make_unique<CharClassInfo>());

	//throw std::exception();
	//AddDefaultValues(abilities.value());
	//strict_parsing_ = false;
	//std::cout << "Crash here! #3\n";
	return items;
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
 *  Билдер информации одного отдельного класса.
 */
CharClassInfo::Optional CharClassInfoBuilder::Build(const xml_node &node) {
	auto class_info = std::make_optional(CharClassInfo::Pair());
	class_info.value().first = FindConstantByAttributeValue<ECharClass>("id", node);
	class_info.value().second = std::make_unique<CharClassInfo>();
	ParseSkills(class_info.value().second, node.child("skills"));
	return class_info;
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
