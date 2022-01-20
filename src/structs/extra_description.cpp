/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Класс экстра-описаний предметов, мобов и комнат.
*/

#include "extra_description.h"

#include "utils/utils.h"

void ExtraDescription::set_keyword(std::string const &value) {
	if (keyword != nullptr)
		free(keyword);
	keyword = str_dup(value.c_str());
}

void ExtraDescription::set_description(std::string const &value) {
	if (description != nullptr)
		free(description);
	description = str_dup(value.c_str());
}

ExtraDescription::~ExtraDescription() {
	if (nullptr != keyword) {
		free(keyword);
	}

	if (nullptr != description) {
		free(description);
	}
	// we don't take care of items in list. So, we don't do anything with the next field.
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
