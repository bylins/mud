// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_ENCHANT_HPP_INCLUDED
#define OBJ_ENCHANT_HPP_INCLUDED

//#include "features.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <string>
#include <vector>

namespace obj_sets
{
	struct ench_type;
}

namespace ObjectEnchant
{

enum
{
	// из предмета типа ITEM_ENCHANT
	ENCHANT_FROM_OBJ,
	// из сетового бонуса
	ENCHANT_FROM_SET
};

// список аффектов от какого-то одного источника
struct enchant
{
	enchant();
	// инит свои аффекты из указанного предмета (ENCHANT_FROM_OBJ)
	enchant(OBJ_DATA *obj);
	// распечатка аффектов для опознания
	void print(CHAR_DATA *ch) const;
	// генерация строки с энчантом для файла объекта
	std::string print_to_file() const;
	// добавить энчант на предмет
	void apply_to_obj(OBJ_DATA *obj) const;

	// имя источника аффектов
	std::string name_;
	// тип источника аффектов
	int type_;
	// список APPLY аффектов (affected[MAX_OBJ_AFFECT])
	std::vector<obj_affected_type> affected_;
	// аффекты обкаста (obj_flags.affects)
	FLAG_DATA affects_flags_;
	// экстра аффекты (obj_flags.extra_flags)
	FLAG_DATA extra_flags_;
	// запреты на ношение (obj_flags.no_flag)
	FLAG_DATA no_flags_;
	// изменение веса (+-)
	int weight_;
	// кубики на пушки (пока вешаются только с сетовых энчантов)
	int ndice_;
	int sdice_;
};

class Enchants
{
public:
	bool empty() const;
	std::string print_to_file() const;
	void print(CHAR_DATA *ch) const;
	bool check(int type) const;
	void add(const enchant &ench);
	// сеты используют только вес (который накопительный, а не флаг), поэтому
	// их допускается менять, т.к. сколько прибавили, столько можно и отнять
	void update_set_bonus(OBJ_DATA *obj, const obj_sets::ench_type& ench);
	void remove_set_bonus(OBJ_DATA *obj);

private:
	std::vector<enchant> list_;
};

} // obj

#endif // OBJ_ENCHANT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
