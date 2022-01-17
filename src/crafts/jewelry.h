/**
 \authors Created by Sventovit
 \date 17.01.2022.
 \brief Умение "ювелир" - заголовок.
*/

#ifndef BYLINS_SRC_CRAFTS_JEWELRY_H_
#define BYLINS_SRC_CRAFTS_JEWELRY_H_

#include <string>
#include <unordered_map>

class CHAR_DATA;

// Значения по умолчанию могут быть изменены при чтении файла
struct skillvariables_insgem {
	int lag = 4;
	int minus_for_affect = 5;
	int prob_divide = 1;
	int dikey_percent = 10;
	int timer_plus_percent = 10;
	int timer_minus_percent = 10;
};

//insert_wanted_gem
struct int3 {
	int type;
	int bit;
	int qty;
};

typedef std::unordered_map<std::string, int3> alias_type;

class insert_wanted_gem {
	std::unordered_map<int, alias_type> content;

 public:
	void init();
	void show(CHAR_DATA *ch, int gem_vnum);
	int get_type(int gem_vnum, const std::string &str);
	int get_bit(int gem_vnum, const std::string &str);
	int get_qty(int gem_vnum, const std::string &str);
	int exist(int gem_vnum, const std::string &str) const;
	bool is_gem(int get_vnum);
	std::string get_random_str_for(int gem_vnum);
};

// Это все надо перенести в глобальные объекты
struct skillvariables_insgem insgem_vars;
insert_wanted_gem iwg;

void InitJewelryVars();

#endif //BYLINS_SRC_CRAFTS_JEWELRY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
