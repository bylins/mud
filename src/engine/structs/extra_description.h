/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Класс экстра-описаний предметов, мобов и комнат.
*/

#ifndef BYLINS_SRC_STRUCTS_EXTRA_DESCRIPTION_H_
#define BYLINS_SRC_STRUCTS_EXTRA_DESCRIPTION_H_

#include <string>

// Extra description: набор пар "ключ -> текст" у объекта и комнаты (look по
// под-ключам). Хранятся в std::vector<ExtraDescription> у самой сущности, без
// ручной next-цепочки. У моба "описание" -- отдельное одиночное поле
// player_data.description (std::string), эту структуру не использует.
struct ExtraDescription {
	void set_keyword(std::string const &keyword);
	void set_description(std::string const &description);

	std::string keyword;        // Keyword in look/examine          //
	std::string description;    // What to see                      //
};

#endif //BYLINS_SRC_STRUCTS_EXTRA_DESCRIPTION_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
