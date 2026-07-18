/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Класс экстра-описаний предметов, мобов и комнат.
*/

#ifndef BYLINS_SRC_STRUCTS_EXTRA_DESCRIPTION_H_
#define BYLINS_SRC_STRUCTS_EXTRA_DESCRIPTION_H_

#include <memory>
#include <string>

// Extra description: used in objects, mobiles, and rooms //
struct ExtraDescription {
	using shared_ptr = std::shared_ptr<ExtraDescription>;
	void set_keyword(std::string const &keyword);
	void set_description(std::string const &description);

	std::string keyword;        // Keyword in look/examine          //
	std::string description;    // What to see                      //
	shared_ptr next;    // Next in list                     //
};

#endif //BYLINS_SRC_STRUCTS_EXTRA_DESCRIPTION_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
