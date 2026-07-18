/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Класс экстра-описаний предметов, мобов и комнат.
*/

#include "extra_description.h"

#include "utils/utils.h"

void ExtraDescription::set_keyword(std::string const &value) {
	keyword = value;
}

void ExtraDescription::set_description(std::string const &value) {
	description = value;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
