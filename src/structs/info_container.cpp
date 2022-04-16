/**
\authors Created by Sventovit
\date 2.02.2022.
\brief Универсальный, насколько возможно, контейнер для хранения информации об игровых сущностях типа скиллов и классов.
*/

#include "info_container.h"

#include <map>

typedef std::map<EItemMode, std::string> EItemMode_name_by_value_t;
typedef std::map<const std::string, EItemMode> EItemMode_value_by_name_t;
EItemMode_name_by_value_t EItemMode_name_by_value;
EItemMode_value_by_name_t EItemMode_value_by_name;

void init_EItemMode_ITEM_NAMES() {
	EItemMode_name_by_value.clear();
	EItemMode_value_by_name.clear();

	EItemMode_name_by_value[EItemMode::kDisabled] = "kDisabled";
	EItemMode_name_by_value[EItemMode::kService] = "kService";
	EItemMode_name_by_value[EItemMode::kFrozen] = "kkFrozen";
	EItemMode_name_by_value[EItemMode::kTesting] = "kTesting";
	EItemMode_name_by_value[EItemMode::kEnabled] = "kEnabled";

	for (const auto &i : EItemMode_name_by_value) {
		EItemMode_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EItemMode>(const EItemMode item) {
	if (EItemMode_name_by_value.empty()) {
		init_EItemMode_ITEM_NAMES();
	}
	return EItemMode_name_by_value.at(item);
}

template<>
EItemMode ITEM_BY_NAME<EItemMode>(const std::string &name) {
	if (EItemMode_name_by_value.empty()) {
		init_EItemMode_ITEM_NAMES();
	}
	return EItemMode_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
