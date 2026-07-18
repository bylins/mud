/* ************************************************************************
*   File: im.cpp                                        Part of Bylins    *
*  Usage: Ingradient handling function                                    *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _IM_H_
#define _IM_H_

#include "engine/entities/entities_constants.h"
#include "gameplay/classes/classes_constants.h"

class ObjData;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
struct RoomData;    //

// Классы ингредиентов (росль/живь/твердь) -- см. enum EIngredientClass в
// entities_constants.h (заменил старые #define IM_CLASS_*).

// Слоты значений магингредиента (индексы в values). Plain enum: имена
// по-прежнему используются как int-индексы в GET_OBJ_VAL/set_val.
enum EIngredientSlot {
	IM_POWER_SLOT = 1,   // val[1] -- уровень (сила) ингредиента
	IM_TYPE_SLOT = 2,    // val[2] -- номер типа (из im.lst)
	IM_INDEX_SLOT = 3    // val[3] -- уровень ингредиента или VNUM моба
};

#define        IM_NPARAM            3

struct _im_tlist_tag {
	long size;        // Количество битов
	long *types;        // Битовая маска
};
typedef struct _im_tlist_tag im_tlist;

struct _im_memb_tag {
	int power;        // сила ингредиента
	EGender sex;        // род описателя (0-сред,1-муж,2-жен,3-мн.ч)
	char **aliases;        // массив пар алиасов
	struct _im_memb_tag *next;    // ссылка на следующий
};
typedef struct _im_memb_tag im_memb;

// Описание примитивного типа ингредиента
struct _im_type_tag {
	int id;            // номер из im.lst
	char *name;        // название типа ингредиента
	int proto_vnum;        // vnum объекта-прототипа
	im_tlist tlst;        // список ID типов/метатипов, которым
	// принадлежит данный тип
	im_memb *head;        // список описателей видов
	// не используется для ингредиентов класса живь
};
typedef struct _im_type_tag im_type;

// Описание метатипа не отличается от описания примитивного типа
// Поле cls в описании метатипа не используется
// Поле members в описании метатипа не используется

// Описание дополнительного компонента
struct _im_addon_tag {
	int id;            // тип ингредиента, индекс массива
	int k0, k1, k2;        // распределение энергии
	ObjData *obj;        // подставляемый объект
	struct _im_addon_tag *link;    // ссылка
};
typedef struct _im_addon_tag im_addon;

const int IM_MSG_OK = 0;
const int IM_MSG_FAIL = 1;
const int kImMsgDam = 2;
extern int top_imtypes;

// Описание рецепта
struct _im_recipe_tag {
	int id;            // номер из im.lst
	char *name;        // название рецепта
	char *str_id;      // уникальный строковый id (англ., CamelCase)
	int k_improve;        // сложность прокачки
	int result;        // VNUM прототипа результата
	float k[IM_NPARAM], kp;    // курсы перевода
	int *require;        // массив обязательных компонентов
	int nAddon;        // количество добавочных компонентов
	im_addon *addon;    // массив добавочных компонентов
	std::array<char *, 3> msg_char;    // сообщения OK,FAIL,DAM
	std::array<char *, 3> msg_room;    // сообщения OK,FAIL,DAM
	int x, y;        // XdY - повреждения
	bool damage_enabled;    // наносить ли урон при критическом провале
	// issue.class-recipes: принадлежность рецепта классам (владение, уровень, реморт)
	// больше НЕ хранится здесь. Это свойство класса, а не рецепта: см. секцию
	// <ingredient_magic> в cfg/classes/pc_*.xml и CharClassInfo::FindIngredientRecipe().
	// Рецепт адресуется из класса по str_id.
};
typedef struct _im_recipe_tag im_recipe;

// Описание рецепта-умения
struct im_rskill {
	int rid;        // индекс в главном массиве рецептов
	int perc;        // уровень владения умением
	im_rskill *link;    // указатель на следующее умение в цепочке
};

extern im_recipe *imrecipes;
extern im_type *imtypes;

void im_parse(int **ing_list, char *line);
void im_reset_room(RoomData *room, int level, int type);
ObjData *try_make_ingr(CharData *mob, int prob_default);
int im_assign_power(ObjData *obj);
int im_get_recipe(int id);
int im_get_type_by_name(char *name, int mode);
ObjData *load_ingredient(int index, int power, int rnum);
int im_ing_dump(int *ping, char *s);
void im_inglist_copy(int **pdst, int *src);
void im_extract_ing(int **pdst, int num);
int im_get_char_rskill_count(CharData *ch);
void trg_recipeturn(CharData *ch, int rid, int recipediff);
void AddRecipe(CharData *ch, int rid, int recipediff);
int im_get_recipe_by_name(char *name);
int im_get_recipe_by_str_id(const char *str_id);
im_rskill *im_get_char_rskill(CharData *ch, int rid);
void compose_recipe(CharData *ch, char *argument, int subcmd);
void forget_recipe(CharData *ch, char *argument, int subcmd);
int im_get_idx_by_type(int type);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
