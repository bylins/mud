/*************************************************************************
*   File: item.creation.hpp                            Part of Bylins    *
*   Item creation from magic ingidients functions header                 *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                      *
**************************************************************************/

#ifndef __ITEM_CREATION_HPP__
#define __ITEM_CREATION_HPP__

#include "skills.h"
#include "interpreter.h"
#include "features.hpp"
#include "conf.h"

#include <string>
#include <list>
#include <iostream>
#include <fstream>

#define MAX_ITEMS 	9

#define MAX_PARTS	3

#define MAX_PROTO 	3
#define COAL_PROTO 	311
#define WOOD_PROTO      313
#define TETIVA_PROTO    314


#define MREDIT_MAIN_MENU	0
#define MREDIT_OBJ_PROTO 	1
#define MREDIT_SKILL		2
#define MREDIT_LOCK	 	3
#define MREDIT_INGR_MENU	4
#define MREDIT_INGR_PROTO	5
#define MREDIT_INGR_WEIGHT	6
#define MREDIT_INGR_POWER	7
#define MREDIT_DEL 	 	8
#define MREDIT_CONFIRM_SAVE     9
#define MREDIT_SELECT_PROF     10

#define MAKE_ANY 	0
#define MAKE_POTION	1
#define MAKE_WEAR	2
#define MAKE_METALL	3
#define MAKE_CRAFT	4
#define MAKE_AMULET 5

// определяем минимальное количество мувов для возможности что-то сделать.
#define MIN_MAKE_MOVE   10

using std::string;
using std::ifstream;
using std::ofstream;
using std::list;
using std::endl;

void mredit_parse(struct DESCRIPTOR_DATA *d, char *arg);
void mredit_disp_menu(struct DESCRIPTOR_DATA *d);
void mredit_disp_ingr_menu(struct DESCRIPTOR_DATA *d);

void do_list_make(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_edit_make(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_make_item(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

void init_make_items();
// Старая структура мы ее используем в перековке.
struct create_item_type
{
	int obj_vnum;
	int material_bits;
	int min_weight;
	int max_weight;
	std::array<int, MAX_PROTO> proto;
	ESkill skill;
	std::underlying_type<EWearFlag>::type wear;
};
// Новая структура мы ее используем при создании вещей из ингридиентов
struct ingr_part_type
{
	int proto;
	int min_weight;
	int min_power;
};
struct make_skill_type
{
	const char *name;
	const char *short_name;
	ESkill num;
};

class MakeRecept;

class MakeReceptList
{
	list < MakeRecept * >recepts;

public:
	// Создание деструктор загрузка рецептов.
	MakeReceptList();

	~MakeReceptList();

	// Вывод списка рецептов по всем компонентам у персонажа
	int can_make_list(CHAR_DATA * ch);

	// загрузить рецепты .
	int load();

	// сохранить рецепты.
	int save();

	// сделать рецепт по названию его прототипа из листа.
	MakeRecept *get_by_name(string & rname);

	MakeReceptList *can_make(CHAR_DATA * ch, MakeReceptList * canlist, int use_skill);

	// число элементов рецептов
	size_t size();

	MakeRecept *operator[](size_t i);

	// освобождение списка рецептов.
	void clear();

	void add(MakeRecept * recept);

	void add(MakeReceptList * recept);

	void del(MakeRecept * recept);

	void sort();
};

class MakeRecept
{
	int stat_modify(CHAR_DATA * ch, int value, float devider);

	int add_flags(CHAR_DATA * ch, FLAG_DATA * base_flag, const FLAG_DATA* add_flag, int delta);

	int add_affects(CHAR_DATA * ch, std::array<obj_affected_type, MAX_OBJ_AFFECT>& base, const std::array<obj_affected_type, MAX_OBJ_AFFECT>& add, int delta);

        
public:
	bool locked;

	ESkill skill;
	int obj_proto;
	short ch_class;  // Полель крафт теперь может иметь разные профессии. для обратной совместимости если не заполненно то все.
	
	std::array<ingr_part_type, MAX_PARTS> parts;

	// конструктор деструктор загрузка из строчки.
	// изготовление рецепта указанным чаром.
	MakeRecept();
	// определяем может ли в принципе из компонентов находящихся в инвентаре
	int can_make(CHAR_DATA *ch);
	// создать предмет по рецепту
	int make(CHAR_DATA *ch);
	// вытащить рецепт из строки.
	int load_from_str(string & rstr);
	// сохранить рецепт в строку.
	int save_to_str(string & rstr);
};

class AbstractCreateObjectType
{
	public:
		OBJ_DATA *obj;
		OBJ_DATA *ingrs[MAX_PARTS];
		ESkill skillnum;		
		string  tmpstr, charwork, roomwork, charfail, roomfail, charsucc, roomsucc, chardam, roomdam, tagging, itemtag;
		int		dam = 0;
		int		ingr_cnt = 0,ingr_pow = 0;
		int		craft_weight = 0;
		//virtual AbstractCreateObjectType() {};
		virtual ~AbstractCreateObjectType() {};
		virtual void CreateObject(CHAR_DATA */* params*/) {};

		int add_flags(CHAR_DATA * ch, FLAG_DATA * base_flag, const FLAG_DATA* add_flag,int delta);

		bool fail_create(CHAR_DATA* ch);
		bool check_list_ingr(CHAR_DATA* ch , std::array<ingr_part_type, MAX_PARTS> parts);
		
		int add_affects(CHAR_DATA * ch, std::array<obj_affected_type, MAX_OBJ_AFFECT>& base, const std::array<obj_affected_type, MAX_OBJ_AFFECT>& add, int delta);
        //к сожалению у нас не прототип. прийдется расчитывать отдельно
        float count_mort_requred();
        
        float count_affect_weight(int num, int mod);      

		int stat_modify(CHAR_DATA * ch, int value, float devider);
		void make_object(CHAR_DATA *ch, OBJ_DATA *obj, int ingr_cnt);
		void load_ingr_in_create(OBJ_DATA *ingrs[MAX_PARTS], int ingr_cnt);
		void add_rnd_skills(CHAR_DATA * ch, OBJ_DATA * obj_from, OBJ_DATA *obj_to);
		
		
};

class CreateStuff : public AbstractCreateObjectType
{
	public:

	CreateStuff();
	~CreateStuff() {};
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreateWear : public AbstractCreateObjectType
{
	public:

	CreateWear();
	~CreateWear() {};
	bool check_list_ingr(CHAR_DATA* ch , std::array<ingr_part_type, MAX_PARTS> parts);
	
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreateAmulet : public AbstractCreateObjectType
{
	public:

	CreateAmulet();
	~CreateAmulet() {};
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreateJewel : public AbstractCreateObjectType
{
	public:

	CreateJewel();
	~CreateJewel() {};
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreatePotion : public AbstractCreateObjectType
{
	public:

	CreatePotion();
	~CreatePotion() {};
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreateArmor : public AbstractCreateObjectType
{
	public:

	CreateArmor();
	~CreateArmor() {};
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreateWeapon : public AbstractCreateObjectType
{
	public:

	CreateWeapon();
	~CreateWeapon() {};
	virtual void CreateObject(CHAR_DATA * ch );
};

class CreateBow : public AbstractCreateObjectType
{
	public:

	CreateBow();
	~CreateBow() {};
	virtual void CreateObject(CHAR_DATA * ch );
};


#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
