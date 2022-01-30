/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "classes_info.h"

#include "boot/boot_constants.h"
#include "logger.h"

namespace classes {

void ClassesInfo::InitCharClass(ECharClass class_id) {
	auto class_info = std::make_unique<CharClassInfo>();
	auto it = classes_.try_emplace(class_id, std::move(class_info));
	if (!it.second) {
/*		err_log("CLass '%s' has already exist. Redundant definition had been ignored.\n",
				NAME_BY_ITEM<ECharClass>(class_id).c_str());*/
		err_log("Class '%d' has already exist. Redundant definition had been ignored.\n", class_id);
	}
}

void ClassesInfo::Init() {
	for (ECharClass id = ECharClass::kFirst; id <= ECharClass::kLast; ++id) {
		InitCharClass(id);
	}
	InitCharClass(ECharClass::kUndefined);
	InitCharClass(ECharClass::kMob);
	InitCharClass(ECharClass::kNPCBase);
}

const CharClassInfo &ClassesInfo::operator[](ECharClass class_id) const {
	try {
		return *classes_.at(class_id);
	} catch (const std::out_of_range &) {
		err_log("Unknown class id (%d) passed into CharClassInfo", to_underlying(class_id));
		return *classes_.at(ECharClass::kUndefined);
	}
}

bool CharClassInfo::IsKnown(const ESkill id) const {
	//return skillls_info_->contains(id);
	return true;
};
/*
using pugi::xml_node;

extern xml_node XMLLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document &Doc);

CharClassInfo::ClassSkillInfoRegisterPtr BuildClassesSkillsInfo() {
	xml_node node = XMLLoad(LIB_MISC "class.skills.xml", "classes", "Classes skills loading failed.", doc);
}


//Polud Читает данные из файла хранения параметров умений ABYRVALG - перенести в классес инфо
void LoadClassSkills() {
	const char *CLASS_SKILLS_FILE = LIB_MISC"class.skills.xml";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CLASS_SKILLS_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node node_list = doc.child("skills");

	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...class.skills.xml read fail");
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node xNodeClass, xNodeSkill, race;
	int pc_class, level_decrement;
	for (xNodeClass = race.child("class"); xNodeClass; xNodeClass = xNodeClass.next_sibling("class")) {
		pc_class = xNodeClass.attribute("class_num").as_int();
		level_decrement = xNodeClass.attribute("level_decrement").as_int();
		for (xNodeSkill = xNodeClass.child("skill"); xNodeSkill; xNodeSkill = xNodeSkill.next_sibling("skill")) {
			std::string name = std::string(xNodeSkill.attribute("name").value());
			auto sk_num = FixNameFndFindSkillNum(name);
			if (MUD::Skills().IsInvalid(sk_num)) {
				log("Skill '%s' not found...", name.c_str());
				graceful_exit(1);
			}
			skill_info[sk_num].classknow[pc_class] = kKnowSkill;
			if ((level_decrement < 1 && level_decrement != -1) || level_decrement > kMaxRemort) {
				log("ERROR: Недопустимый параметр level decrement класса %d.", pc_class);
				skill_info[sk_num].level_decrement[pc_class] = -1;
			} else {
				skill_info[sk_num].level_decrement[pc_class] = level_decrement;
			}
			auto value = xNodeSkill.attribute("improve").as_int();
			skill_info[sk_num].k_improve[pc_class] = MAX(1, value);
			value = xNodeSkill.attribute("level").as_int();
			if (value > 0 && value < kLevelImmortal) {
				skill_info[sk_num].min_level[pc_class] = value;
			} else {
				log("ERROR: Недопустимый минимальный уровень изучения умения '%s' - %d",
					skill_info[sk_num].name,
					value);
				graceful_exit(1);
			}
			value = xNodeSkill.attribute("remort").as_int();
			if (value >= 0 && value < kMaxRemort) {
				skill_info[sk_num].min_remort[pc_class] = value;
			} else {
				log("ERROR: Недопустимое минимальное количество ремортов для умения '%s' - %d",
					skill_info[sk_num].name,
					value);
				graceful_exit(1);
			}
		}
	}
}*/

} // namespace clases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
