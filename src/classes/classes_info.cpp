/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#include "classes_info.h"

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
	for (ECharClass id = PLAYER_CLASS_FIRST; id <= PLAYER_CLASS_LAST; ++id) {
		InitCharClass(id);
	}
	InitCharClass(CLASS_UNDEFINED);
	InitCharClass(CLASS_MOB);
	InitCharClass(NPC_CLASS_BASE);
}

const CharClassInfo &ClassesInfo::operator[](ECharClass class_id) const {
	try {
		return *classes_.at(class_id);
	} catch (const std::out_of_range &) {
		err_log("Unknown class id (%d) passed into CharClassInfo", to_underlying(class_id));
		return *classes_.at(CLASS_UNDEFINED);
	}
}

} // namespace clases

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
