/*************************************************************************
*   File: item.creation.hpp                            Part of Bylins    *
*   Item creation from magic ingidients functions header                 *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                      *
**************************************************************************/

#ifndef ITEM_CREATION_HPP_
#define ITEM_CREATION_HPP_

#include "game_affects/affect_data.h"
#include "conf.h"
#include "entities/entities_constants.h"
#include "features.h"
#include "interpreter.h"
#include "game_skills/skills.h"

#include <string>
#include <list>
#include <iostream>
#include <fstream>

const int MAX_ITEMS = 9;
const int MAX_PARTS = 3;

const int MAX_PROTO = 3;
const int COAL_PROTO = 311;
const int WOOD_PROTO = 313;
const int TETIVA_PROTO = 314;

const int MREDIT_MAIN_MENU = 0;
const int MREDIT_OBJ_PROTO = 1;
const int MREDIT_SKILL = 2;
const int MREDIT_LOCK = 3;
const int MREDIT_INGR_MENU = 4;
const int MREDIT_INGR_PROTO = 5;
const int MREDIT_INGR_WEIGHT = 6;
const int MREDIT_INGR_POWER = 7;
const int MREDIT_DEL = 8;
const int MREDIT_CONFIRM_SAVE = 9;

const int MAKE_ANY = 0;
const int MAKE_POTION = 1;
const int MAKE_WEAR = 2;
const int MAKE_METALL = 3;
const int MAKE_CRAFT = 4;
const int MAKE_AMULET = 5;

const int SCMD_TRANSFORMWEAPON = 0;
const int SCMD_CREATEBOW = 1;

// определяем минимальное количество мувов для возможности что-то сделать.
const int MIN_MAKE_MOVE = 10;

using std::string;
using std::ifstream;
using std::ofstream;
using std::list;
using std::endl;

void mredit_parse(struct DescriptorData *d, char *arg);
void mredit_disp_menu(struct DescriptorData *d);
void mredit_disp_ingr_menu(struct DescriptorData *d);

void do_list_make(CharData *ch, char *argument, int cmd, int subcmd);
void do_edit_make(CharData *ch, char *argument, int cmd, int subcmd);
void do_make_item(CharData *ch, char *argument, int cmd, int subcmd);

void init_make_items();
// Старая структура мы ее используем в перековке.
struct create_item_type {
	int obj_vnum;
	int material_bits;
	int min_weight;
	int max_weight;
	std::array<int, MAX_PROTO> proto;
	ESkill skill;
	std::underlying_type<EWearFlag>::type wear;
};
// Новая структура мы ее используем при создании вещей из ингридиентов
struct ingr_part_type {
	int proto;
	int min_weight;
	int min_power;
};
struct make_skill_type {
	const char *name;
	const char *short_name;
	ESkill num;
};

class MakeRecept;

class MakeReceptList {
	list<MakeRecept *> recepts;

 public:
	// Создание деструктор загрузка рецептов.
	MakeReceptList();

	~MakeReceptList();

	// Вывод списка рецептов по всем компонентам у персонажа
	int can_make_list(CharData *ch);

	// загрузить рецепты .
	int load();

	// сохранить рецепты.
	int save();

	// сделать рецепт по названию его прототипа из листа.
	MakeRecept *get_by_name(string &rname);

	MakeReceptList *can_make(CharData *ch, MakeReceptList *canlist, ESkill use_skill);

	// число элементов рецептов
	size_t size();

	MakeRecept *operator[](size_t i);

	// освобождение списка рецептов.
	void clear();

	void add(MakeRecept *recept);

	void add(MakeReceptList *recept);

	void del(MakeRecept *recept);

	void sort();
};

class MakeRecept {
	int stat_modify(CharData *ch, int value, float devider);

	int add_flags(CharData *ch, FlagData *base_flag, const FlagData *add_flag, int delta);

	int add_affects(CharData *ch,
					std::array<obj_affected_type, kMaxObjAffect> &base,
					const std::array<obj_affected_type, kMaxObjAffect> &add,
					int delta);

	int get_ingr_lev(ObjData *ingrobj);

	void make_object(CharData *ch, ObjData *obj, ObjData *ingrs[MAX_PARTS], int ingr_cnt);

	void make_value_wear(CharData *ch, ObjData *obj, ObjData *ingrs[MAX_PARTS]);
	//к сожалению у нас не прототип. прийдется расчитывать отдельно
	float count_mort_requred(ObjData *obj);

	float count_affect_weight(int num, int mod);

	int get_ingr_pow(ObjData *ingrobj);

	void add_rnd_skills(CharData *ch, ObjData *obj_from, ObjData *obj_to);

 public:
	bool locked;

	ESkill skill;
	int obj_proto;
	std::array<ingr_part_type, MAX_PARTS> parts;

	// конструктор деструктор загрузка из строчки.
	// изготовление рецепта указанным чаром.
	MakeRecept();
	// определяем может ли в принципе из компонентов находящихся в инвентаре
	int can_make(CharData *ch);
	// создать предмет по рецепту
	int make(CharData *ch);
	// вытащить рецепт из строки.
	int load_from_str(string &rstr);
	// сохранить рецепт в строку.
	int save_to_str(string &rstr);
};

#endif // ITEM_CREATION_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
