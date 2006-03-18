/* ************************************************************************
*   File: class.cpp                                     Part of Bylins    *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "skills.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"
#include "pk.h"
//MZ.tops
#include "top.h"
//-MZ.tops
#include "features.hpp"
// +newbook.patch (Alisher)
#include "im.h"
// -newbook.patch (Alisher)

extern int siteok_everyone;
extern struct spell_create_type spell_create[];
extern float exp_coefficients[];

struct skillvariables_dig dig_vars;
struct skillvariables_insgem insgem_vars;

// 14 проф, 0-14 мортов
// grouping[class][remorts]
int grouping[14][15];

int calc_loadroom(CHAR_DATA * ch);

/* local functions */
int parse_class(char arg);
int parse_race(char arg);
long find_class_bitvector(char arg);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
void do_start(CHAR_DATA * ch, int newbie);
int backstab_mult(int level);
int invalid_anti_class(CHAR_DATA * ch, OBJ_DATA * obj);
int invalid_no_class(CHAR_DATA * ch, OBJ_DATA * obj);
int level_exp(CHAR_DATA * ch, int level);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);
byte extend_saving_throws(int class_num, int type, int level);
int invalid_anti_class(CHAR_DATA * ch, OBJ_DATA * obj);
int invalid_unique(CHAR_DATA * ch, OBJ_DATA * obj);

/* Names first */

const char *class_abbrevs[] = { "Ле",
	"Ко",
	"Та",
	"Бо",
	"На",
	"Др",
	"Ку",
	"Во",
	"Че",
	"Ви",
	"Ох",
	"Кз",
	"Кп",
	"Вл",
	"\n"
};

const char *kin_abbrevs[] = { "Ру",
	"Ви",
	"Ст",
	"\n"
};


const char *pc_class_types[] = { "Лекарь",
	"Колдун",
	"Тать",
	"Богатырь",
	"Наемник",
	"Дружинник",
	"Кудесник",
	"Волшебник",
	"Чернокнижник",
	"Витязь",
	"Охотник",
	"Кузнец",
	"Купец",
	"Волхв",
	"Жрец",
	"Нойда",
	"Тиуве",
	"Берсерк",
	"Наемник",
	"Хирдман",
	"Заарин",
	"Босоркун",
	"Равк",
	"Кампе",
	"Лучник",
	"Аргун",
	"Кепмен",
	"Скальд",
	"Знахарь",
	"Бакша",
	"Карак",
	"Батыр",
	"Тургауд",
	"Нуке",
	"Капнобатай",
	"Акшаман",
	"Карашаман",
	"Чериг",
	"Шикорхо",
	"Дархан",
	"Сатучы",
	"Сеид",
	"\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
    "\r\n"
    "Выберите профессию :\r\n"
    "  [Л]екарь\r\n"
    "  [К]олдун\r\n"
    "  [Т]ать\r\n"
    "  [Б]огатырь\r\n"
    "  [Н]аемник\r\n"
    "  [Д]ружинник\r\n"
    "  К[у]десник\r\n"
    "  [В]олшебник\r\n"
    "  [Ч]ернокнижник\r\n" "  В[и]тязь\r\n" "  [О]хотник\r\n" "  Ку[з]нец\r\n" "  Ку[п]ец\r\n" "  Вол[x]в\r\n";

const char *class_menu_vik =
    "\r\n"
    "Выберите профессию :\r\n"
    "  [Ж]рец\r\n"
    "  [Н]ойда\r\n"
    "  [Т]иуве\r\n"
    "  [Б]ерсерк\r\n"
    "  Н[а]емник\r\n"
    "  [Х]ирдман\r\n"
    "  [З]аарин\r\n"
    "  Б[о]соркун\r\n"
    "  [Р]авк\r\n"
    "  [К]ампе\r\n"
    "  [Л]учник\r\n"
    "  Ар[г]ун\r\n"
    "  Ке[п]мен\r\n"
    "  [С]кальд\r\n";

const char *class_menu_step =
    "\r\n"
    "Выберите профессию :\r\n"
    "  [З]нахарь\r\n"
    "  [Б]акша\r\n"
    "  [К]арак\r\n"
    "  Б[а]тыр\r\n"
    "  [Т]ургауд\r\n"
    "  [Н]уке\r\n"
    "  Ка[п]нобатай\r\n"
    "  Ак[ш]аман\r\n"
    "  Ка[р]ашаман\r\n"
    "  [Ч]ериг\r\n"
    "  Шик[о]рхо\r\n"
    "  [Д]архан\r\n"
    "  [С]атучы\r\n"
    "  Се[и]д\r\n";

const char *color_menu =
    "\r\n"
    "Выберите режим цвета :\r\n"
    "  [0]Выкл\r\n" 
    "  [1]Простой\r\n" 
    "  [2]Обычный\r\n" 
    "  [3]Полный\r\n";

/* The menu for choosing a religion in interpreter.c: */
const char *religion_menu =
    "\r\n" "Какой религии Вы отдаете предпочтение :\r\n" "  Я[з]ычество\r\n" "  [Х]ристианство\r\n";

#define RELIGION_ANY 100


/* Соответствие классов и религий. RELIGION_POLY-класс не может быть христианином 
                                   RELIGION_MONO-класс не может быть язычником  (Кард)
				   RELIGION_ANY - класс может быть кем угодно */
const int class_religion[] = { RELIGION_ANY,	/*Лекарь */
	RELIGION_POLY,		/*Колдун */
	RELIGION_ANY,		/*Тать */
	RELIGION_POLY,		/*Богатырь */
	RELIGION_MONO,		/*Наемник */
	RELIGION_ANY,		/*Дружинник */
	RELIGION_ANY,		/*Кудесник */
	RELIGION_MONO,		/*Волшебник */
	RELIGION_POLY,		/*Чернокнижник */
	RELIGION_MONO,		/*Витязь */
	RELIGION_ANY,		/*Охотник */
	RELIGION_ANY,		/*Кузнец */
	RELIGION_ANY,		/*Купец */
	RELIGION_POLY		/*Волхв */
};

/*****/

/* The menu for choosing a race in interpreter.c: */
const char *race_menu =
    "\r\n"
    "Какой РОД Вам ближе всего по духу :\r\n"
    "  [С]еверяне\r\n" "  [П]оляне\r\n" "  [К]ривичи\r\n" "  [В]ятичи\r\n" "  В[е]лыняне\r\n" "  [Д]ревляне\r\n";

const char *race_types[] = { "северяне",
	"поляне",
	"кривичи",
	"вятичи",
	"велыняне",
	"древляне"
};

const char *race_menu_step =
   "\r\n"
   "Какой РОД Вам ближе всего по духу :\r\n"
   "  [П]оловцы\r\n"
   "  П[е]ченеги\r\n"
   "  [М]онголы\r\n"
   "  [У]йгуры\r\n"
   "  [К]ангары\r\n"
   "  [Х]азары\r\n";
 
const char *race_types_srep[] = { "половцы",
	"печенеги",
	"монголы",
	"уйгуры",
	"кангары",
	"хазары"
};

const char *race_menu_vik =
   "\r\n"
   "Какой РОД Вам ближе всего по духу :\r\n"
   "  [С]веи\r\n"
   "  [Д]атчане\r\n"
   "  [Г]етты\r\n"
   "  [Ю]тты\r\n"
   "  [Х]алейги\r\n"
   "  [Н]орвежцы\r\n";
 
const char *race_types_vik[] = { "свеи",
	"датчане",
	"гетты",
	"ютты",
	"халейги",
	"норвежцы"
};


/**/
const char *kin_menu =
   "\r\n"
   "Какие племена вам ближе по духу :\r\n"
   "  [Р]усичи\r\n"
   "  [C]тепняки (находится в стадии тестирования)\r\n"
   "  [В]икинги (находится в стадии тестирования)\r\n";

const char *pc_kin_types[] = { "Русичи",
	"Викинги",
	"Степняки",
	"\n"
};


/*
 * The code to interpret a class letter -- used in interpreter.cpp when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
	arg = LOWER(arg);

	switch (arg) {
	case 'л':
		return CLASS_CLERIC;
	case 'к':
		return CLASS_BATTLEMAGE;
	case 'т':
		return CLASS_THIEF;
	case 'б':
		return CLASS_WARRIOR;
	case 'н':
		return CLASS_ASSASINE;
	case 'д':
		return CLASS_GUARD;
	case 'у':
		return CLASS_CHARMMAGE;
	case 'в':
		return CLASS_DEFENDERMAGE;
	case 'ч':
		return CLASS_NECROMANCER;
	case 'и':
		return CLASS_PALADINE;
	case 'о':
		return CLASS_RANGER;
	case 'з':
		return CLASS_SMITH;
	case 'п':
		return CLASS_MERCHANT;
	case 'х':
		return CLASS_DRUID;
	default:
		return CLASS_UNDEFINED;
	}
}

int
parse_class_vik (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'ж':
		return CLASS_CLERIC;
	case 'н':
		return CLASS_BATTLEMAGE;
	case 'т':
		return CLASS_THIEF;
	case 'б':
		return CLASS_WARRIOR;
	case 'а':
		return CLASS_ASSASINE;
	case 'х':
		return CLASS_GUARD;
	case 'з':
		return CLASS_CHARMMAGE;
	case 'о':
		return CLASS_DEFENDERMAGE;
	case 'р':
		return CLASS_NECROMANCER;
	case 'к':
		return CLASS_PALADINE;
	case 'л':
		return CLASS_RANGER;
	case 'г':
		return CLASS_SMITH;
	case 'п':
		return CLASS_MERCHANT;
	case 'с':
		return CLASS_DRUID;
	default:
		return CLASS_UNDEFINED;
	}
}


int
parse_class_step (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'з':
		return CLASS_CLERIC;
	case 'б':
		return CLASS_BATTLEMAGE;
	case 'к':
		return CLASS_THIEF;
	case 'а':
		return CLASS_WARRIOR;
	case 'т':
		return CLASS_ASSASINE;
	case 'н':
		return CLASS_GUARD;
	case 'п':
		return CLASS_CHARMMAGE;
	case 'ш':
		return CLASS_DEFENDERMAGE;
	case 'р':
		return CLASS_NECROMANCER;
	case 'ч':
		return CLASS_PALADINE;
	case 'о':
		return CLASS_RANGER;
	case 'д':
		return CLASS_SMITH;
	case 'с':
		return CLASS_MERCHANT;
	case 'и':
		return CLASS_DRUID;
	default:
		return CLASS_UNDEFINED;
	}
}


int parse_race(char arg)
{
	arg = LOWER(arg);

	switch (arg) {
	case 'с':
		return RACE_SEVERANE;
	case 'п':
		return RACE_POLANE;
	case 'к':
		return RACE_KRIVICHI;
	case 'в':
		return RACE_VATICHI;
	case 'е':
		return RACE_VELANE;
	case 'д':
		return RACE_DREVLANE;
	default:
		return RACE_UNDEFINED;
	}
}

int
parse_race_step (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'п':
		return RACE_POLOVCI;
	case 'е':
		return RACE_PECHENEGI;
	case 'м':
		return RACE_MONGOLI;
	case 'у':
		return RACE_YIGURI;
	case 'к':
		return RACE_KANGARI;
	case 'х':
		return RACE_XAZARI;
	default:
		return RACE_UNDEFINED;
	}
}

int
parse_race_vik (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'с':
		return RACE_SVEI;
	case 'д':
		return RACE_DATCHANE;
	case 'г':
		return RACE_GETTI;
	case 'ю':
		return RACE_UTTI;
	case 'х':
		return RACE_XALEIGI;
	case 'н':
		return RACE_NORVEZCI;
	default:
		return RACE_UNDEFINED;
	}
}

int
parse_kin (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'р':
		return KIN_RUSICHI;
/*	case 'Ё':
		return KIN_STEPNYAKI;
	case '@':
		return KIN_VIKINGI;*//*Отключим пока*/
	default:
		return KIN_UNDEFINED;
	}
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
	arg = LOWER(arg);

	switch (arg) {
	case 'л':
		return (1 << CLASS_CLERIC);
	case 'к':
		return (1 << CLASS_BATTLEMAGE);
	case 'т':
		return (1 << CLASS_THIEF);
	case 'б':
		return (1 << CLASS_WARRIOR);
	case 'н':
		return (1 << CLASS_ASSASINE);
	case 'д':
		return (1 << CLASS_GUARD);
	case 'у':
		return (1 << CLASS_CHARMMAGE);
	case 'в':
		return (1 << CLASS_DEFENDERMAGE);
	case 'ч':
		return (1 << CLASS_NECROMANCER);
	case 'и':
		return (1 << CLASS_PALADINE);
	case 'о':
		return (1 << CLASS_RANGER);
	case 'з':
		return (1 << CLASS_SMITH);
	case 'п':
		return (1 << CLASS_MERCHANT);
	case 'х':
		return (1 << CLASS_DRUID);
	default:
		return 0;
	}
}

long
find_class_bitvector_vik (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'ж':
		return (1 << CLASS_CLERIC);
	case 'н':
		return (1 << CLASS_BATTLEMAGE);
	case 'т':
		return (1 << CLASS_THIEF);
	case 'б':
		return (1 << CLASS_WARRIOR);
	case 'а':
		return (1 << CLASS_ASSASINE);
	case 'х':
		return (1 << CLASS_GUARD);
	case 'з':
		return (1 << CLASS_CHARMMAGE);
	case 'о':
		return (1 << CLASS_DEFENDERMAGE);
	case 'р':
		return (1 << CLASS_NECROMANCER);
	case 'к':
		return (1 << CLASS_PALADINE);
	case 'л':
		return (1 << CLASS_RANGER);
	case 'г':
		return (1 << CLASS_SMITH);
	case 'п':
		return (1 << CLASS_MERCHANT);
	case 'в':
		return (1 << CLASS_DRUID);
	default:
		return CLASS_UNDEFINED;
	}
}



long
find_class_bitvector_step (char arg)
{
	arg = LOWER (arg);

	switch (arg){
	case 'з':
		return (1 << CLASS_CLERIC);
	case 'б':
		return (1 << CLASS_BATTLEMAGE);
	case 'к':
		return (1 << CLASS_THIEF);
	case 'а':
		return (1 << CLASS_WARRIOR);
	case 'т':
		return (1 << CLASS_ASSASINE);
	case 'н':
		return (1 << CLASS_GUARD);
	case 'п':
		return (1 << CLASS_CHARMMAGE);
	case 'ш':
		return (1 << CLASS_DEFENDERMAGE);
	case 'р':
		return (1 << CLASS_NECROMANCER);
	case 'ч':
		return (1 << CLASS_PALADINE);
	case 'о':
		return (1 << CLASS_RANGER);
	case 'д':
		return (1 << CLASS_SMITH);
	case 'с':
		return (1 << CLASS_MERCHANT);
	case 'и':
		return (1 << CLASS_DRUID);
	default:
		return CLASS_UNDEFINED;
	}
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 *
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 *
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {	/* MAG        CLE             THE             WAR */
	{95, 95, 85, 80},	/* learned level */
	{100, 100, 12, 12},	/* max per prac */
	{25, 25, 0, 0,},	/* min per pac */
	{SPELL, SPELL, SKILL, SKILL}	/* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.cpp if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
int guild_info[][3] = {

/* Midgaard */
	{CLASS_BATTLEMAGE, 3017, SCMD_SOUTH},
	{CLASS_CLERIC, 3004, SCMD_NORTH},
	{CLASS_THIEF, 3027, SCMD_EAST},
	{CLASS_WARRIOR, 3021, SCMD_EAST},

/* Brass Dragon */
	{-999 /* all */ , 5065, SCMD_WEST},

/* this must go last -- add new guards above! */
	{-1, -1, -1}
};


// Таблицы бызовых спасбросков

const byte sav_01[50] = {
	90, 90, 90, 90, 90, 89, 89, 88, 88, 87,	// 00-09
	86, 85, 84, 83, 81, 79, 78, 75, 73, 71,	// 10-19
	68, 65, 62, 59, 56, 52, 48, 44, 40, 35,	// 20-29
	30, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_02[50] = {
	90, 90, 90, 90, 90, 89, 89, 88, 87, 87,	// 00-09
	86, 84, 83, 81, 80, 78, 75, 73, 70, 68,	// 10-19
	65, 61, 58, 54, 50, 46, 41, 36, 31, 26,	// 20-29
	20, 15, 10, 9, 8, 7, 6, 5, 4, 3,	// 30-39
	2, 1, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_03[50] = {
	90, 90, 90, 90, 90, 89, 89, 88, 88, 87,	// 00-09
	86, 85, 83, 82, 80, 79, 76, 74, 72, 69,	// 10-19
	66, 63, 60, 57, 53, 49, 45, 40, 35, 30,	// 20-29
	25, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_04[50] = {
	90, 90, 90, 90, 90, 90, 89, 89, 89, 88,	// 00-09
	87, 87, 86, 85, 84, 83, 82, 80, 79, 77,	// 10-19
	75, 74, 72, 69, 67, 65, 62, 59, 56, 53,	// 20-29
	50, 45, 40, 35, 30, 25, 20, 15, 10, 5,	// 30-39
	4, 3, 2, 1, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_05[50] = {
	90, 90, 90, 90, 90, 89, 89, 89, 88, 87,	// 00-09
	86, 86, 84, 83, 82, 80, 79, 77, 75, 72,	// 10-19
	70, 67, 65, 62, 59, 55, 52, 48, 44, 39,	// 20-29
	35, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_06[50] = {
	90, 90, 90, 90, 90, 90, 89, 89, 88, 88,	// 00-09
	87, 86, 85, 84, 82, 80, 78, 76, 74, 72,	// 10-19
	70, 67, 64, 61, 58, 55, 52, 49, 46, 43,	// 20-29
	40, 45, 41, 37, 33, 25, 20, 15, 10, 5,	// 30-39
	4, 3, 2, 1, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_08[50] = {
	90, 90, 90, 90, 90, 89, 89, 89, 88, 88,	// 00-09
	87, 86, 85, 84, 83, 81, 80, 78, 76, 74,	// 10-19
	72, 70, 67, 64, 61, 58, 55, 52, 48, 44,	// 20-29
	40, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_09[50] = {
	90, 75, 73, 71, 69, 67, 65, 63, 61, 60,	// 00-09
	59, 57, 55, 53, 51, 50, 49, 47, 45, 43,	// 10-19
	41, 40, 39, 37, 35, 33, 31, 29, 27, 25,	// 20-29
	23, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_10[50] = {
	90, 80, 79, 78, 76, 75, 73, 70, 67, 65,	// 00-09
	64, 63, 61, 60, 59, 57, 56, 55, 54, 53,	// 10-19
	52, 51, 50, 48, 45, 44, 42, 40, 39, 38,	// 20-29
	37, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_11[50] = {
	90, 80, 79, 78, 77, 76, 75, 74, 73, 72,	// 00-09
	71, 70, 69, 68, 67, 66, 65, 64, 63, 62,	// 10-19
	61, 60, 59, 58, 57, 56, 55, 54, 53, 52,	// 20-29
	51, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_12[50] = {
	90, 85, 83, 82, 80, 75, 70, 65, 63, 62,	// 00-09
	60, 55, 50, 45, 43, 42, 40, 37, 33, 30,	// 10-19
	29, 28, 26, 25, 24, 23, 21, 20, 19, 18,	// 20-29
	17, 16, 15, 14, 13, 12, 11, 10, 9, 8,	// 30-39
	7, 6, 5, 4, 3, 2, 1, 0, 0, 0	// 40-49
};
const byte sav_13[50] = {
	90, 83, 81, 79, 77, 75, 72, 68, 65, 63,	// 00-09
	61, 58, 56, 53, 50, 47, 45, 43, 42, 41,	// 10-19
	40, 38, 37, 36, 34, 33, 32, 31, 30, 29,	// 20-29
	27, 26, 25, 24, 23, 22, 21, 20, 18, 16,	// 30-39
	12, 10, 8, 6, 4, 2, 1, 0, 0, 0	// 40-49
};
const byte sav_14[50] = {
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,	// 00-09
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,	// 10-19
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,	// 20-29
	100, 70, 70, 70, 70, 70, 70, 70, 70, 70,	// 30-39
	70, 70, 70, 70, 70, 70, 70, 70, 70, 70	// 40-49
};
const byte sav_15[50] = {
	100, 99, 98, 97, 96, 95, 94, 93, 92, 91,	// 00-09
	90, 89, 88, 87, 86, 85, 84, 83, 82, 81,	// 10-19
	80, 79, 78, 77, 76, 75, 74, 73, 72, 71,	// 20-29
	70, 50, 50, 50, 50, 50, 50, 50, 50, 50,	// 30-39
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50	// 40-49
};
const byte sav_16[50] = {
	100, 99, 97, 96, 95, 94, 92, 91, 89, 88,	// 00-09
	86, 85, 84, 83, 81, 80, 79, 77, 76, 75,	// 10-19
	74, 72, 71, 70, 68, 65, 64, 63, 62, 61,	// 20-29
	60, 58, 57, 56, 54, 52, 51, 49, 47, 46,	// 30-39
	45, 43, 42, 41, 39, 37, 35, 34, 32, 31	// 40-49
};
const byte sav_17[50] = {
	100, 99, 97, 96, 95, 94, 92, 91, 89, 88,	// 00-09
	86, 84, 82, 80, 78, 76, 74, 72, 70, 68,	// 10-19
	64, 62, 60, 58, 56, 54, 52, 50, 49, 47,	// 20-29
	45, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_18[50] = {
	100, 100, 100, 100, 100, 99, 99, 99, 99, 99,	// 00-09
	98, 98, 98, 98, 98, 97, 97, 97, 97, 97,	// 10-19
	96, 96, 96, 96, 96, 95, 95, 95, 95, 95,	// 20-29
	94, 94, 94, 94, 94, 93, 93, 93, 93, 93,	// 30-39
	92, 92, 92, 92, 92, 91, 91, 91, 91, 91	// 40-49
};


// {CLASS,{PARA,ROD,AFFECT,BREATH,SPELL,BASIC}}
struct {
	int chclass;
	const byte *saves[SAVING_COUNT];
} std_saving[] = {
	{
		CLASS_CLERIC, {
	sav_01, sav_10, sav_08, sav_14}}, {
		CLASS_BATTLEMAGE, {
	sav_01, sav_09, sav_02, sav_14}}, {
		CLASS_CHARMMAGE, {
	sav_02, sav_09, sav_02, sav_14}}, {
		CLASS_DEFENDERMAGE, {
	sav_01, sav_09, sav_01, sav_14}}, {
		CLASS_NECROMANCER, {
	sav_01, sav_09, sav_01, sav_14}}, {
		CLASS_DRUID, {
	sav_03, sav_10, sav_03, sav_14}}, {
		CLASS_THIEF, {
	sav_08, sav_11, sav_08, sav_15}}, {
		CLASS_ASSASINE, {
	sav_08, sav_11, sav_08, sav_15}}, {
		CLASS_MERCHANT, {
	sav_08, sav_11, sav_08, sav_15}}, {
		CLASS_WARRIOR, {
	sav_04, sav_12, sav_04, sav_16}}, {
		CLASS_GUARD, {
	sav_04, sav_12, sav_04, sav_16}}, {
		CLASS_SMITH, {
	sav_04, sav_12, sav_04, sav_16}}, {
		CLASS_PALADINE, {
	sav_05, sav_12, sav_05, sav_17}}, {
		CLASS_RANGER, {
	sav_05, sav_12, sav_05, sav_17}}, {
		CLASS_MOB, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_BASIC_NPC, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_UNDEAD, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_HUMAN, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_ANIMAL, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_HERO_WARRIOR, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_HERO_MAGIC, {
	sav_06, sav_13, sav_06, sav_18}}, {
		CLASS_NPC_BATLEMAGE, {
	sav_06, sav_13, sav_06, sav_18}}, {
		-1, {
	sav_02, sav_12, sav_02, sav_16}}
};

//****************************************************************************
//****************************************************************************
//****************************************************************************


byte saving_throws(int class_num, int type, int level)
{
	return extend_saving_throws(class_num, type, level);
}

byte extend_saving_throws(int class_num, int type, int level)
{
	int i;
	if (type < 0 || type >= SAVING_COUNT)
		return 100;
	if (level <= 0 || level > 50)
		return 100;
	--level;

	for (i = 0; std_saving[i].chclass != -1 && std_saving[i].chclass != class_num; ++i);

	return std_saving[i].saves[type][level];
}


/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level)
{
	switch (class_num) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level) {
		case 0:
			return 100;
		case 1:
			return 20;
		case 2:
			return 20;
		case 3:
			return 20;
		case 4:
			return 19;
		case 5:
			return 19;
		case 6:
			return 19;
		case 7:
			return 18;
		case 8:
			return 18;
		case 9:
			return 18;
		case 10:
			return 17;
		case 11:
			return 17;
		case 12:
			return 17;
		case 13:
			return 16;
		case 14:
			return 16;
		case 15:
			return 16;
		case 16:
			return 15;
		case 17:
			return 15;
		case 18:
			return 15;
		case 19:
			return 14;
		case 20:
			return 14;
		case 21:
			return 14;
		case 22:
			return 13;
		case 23:
			return 13;
		case 24:
			return 13;
		case 25:
			return 12;
		case 26:
			return 12;
		case 27:
			return 12;
		case 28:
			return 11;
		case 29:
			return 11;
		case 30:
			return 10;
		case 31:
			return 0;
		case 32:
			return 0;
		case 33:
			return 0;
		case 34:
			return 0;
		default:
			log("SYSERR: Missing level for mage thac0.");
		}
	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level) {
		case 0:
			return 100;
		case 1:
			return 20;
		case 2:
			return 20;
		case 3:
			return 20;
		case 4:
			return 18;
		case 5:
			return 18;
		case 6:
			return 18;
		case 7:
			return 16;
		case 8:
			return 16;
		case 9:
			return 16;
		case 10:
			return 14;
		case 11:
			return 14;
		case 12:
			return 14;
		case 13:
			return 12;
		case 14:
			return 12;
		case 15:
			return 12;
		case 16:
			return 10;
		case 17:
			return 10;
		case 18:
			return 10;
		case 19:
			return 8;
		case 20:
			return 8;
		case 21:
			return 8;
		case 22:
			return 6;
		case 23:
			return 6;
		case 24:
			return 6;
		case 25:
			return 4;
		case 26:
			return 4;
		case 27:
			return 4;
		case 28:
			return 2;
		case 29:
			return 2;
		case 30:
			return 1;
		case 31:
			return 0;
		case 32:
			return 0;
		case 33:
			return 0;
		case 34:
			return 0;
		default:
			log("SYSERR: Missing level for cleric thac0.");
		}
	case CLASS_ASSASINE:
	case CLASS_THIEF:
	case CLASS_MERCHANT:
		switch (level) {
		case 0:
			return 100;
		case 1:
			return 20;
		case 2:
			return 20;
		case 3:
			return 19;
		case 4:
			return 19;
		case 5:
			return 18;
		case 6:
			return 18;
		case 7:
			return 17;
		case 8:
			return 17;
		case 9:
			return 16;
		case 10:
			return 16;
		case 11:
			return 15;
		case 12:
			return 15;
		case 13:
			return 14;
		case 14:
			return 14;
		case 15:
			return 13;
		case 16:
			return 13;
		case 17:
			return 12;
		case 18:
			return 12;
		case 19:
			return 11;
		case 20:
			return 11;
		case 21:
			return 10;
		case 22:
			return 10;
		case 23:
			return 9;
		case 24:
			return 9;
		case 25:
			return 8;
		case 26:
			return 8;
		case 27:
			return 7;
		case 28:
			return 7;
		case 29:
			return 6;
		case 30:
			return 5;
		case 31:
			return 0;
		case 32:
			return 0;
		case 33:
			return 0;
		case 34:
			return 0;
		default:
			log("SYSERR: Missing level for thief thac0.");
		}
	case CLASS_WARRIOR:
	case CLASS_GUARD:
		switch (level) {
		case 0:
			return 100;
		case 1:
			return 20;
		case 2:
			return 19;
		case 3:
			return 18;
		case 4:
			return 17;
		case 5:
			return 16;
		case 6:
			return 15;
		case 7:
			return 14;
		case 8:
			return 14;
		case 9:
			return 13;
		case 10:
			return 12;
		case 11:
			return 11;
		case 12:
			return 10;
		case 13:
			return 9;
		case 14:
			return 8;
		case 15:
			return 7;
		case 16:
			return 6;
		case 17:
			return 5;
		case 18:
			return 4;
		case 19:
			return 3;
		case 20:
			return 2;
		case 21:
			return 1;
		case 22:
			return 1;
		case 23:
			return 1;
		case 24:
			return 1;
		case 25:
			return 1;
		case 26:
			return 1;
		case 27:
			return 1;
		case 28:
			return 1;
		case 29:
			return 1;
		case 30:
			return 0;
		case 31:
			return 0;
		case 32:
			return 0;
		case 33:
			return 0;
		case 34:
			return 0;
		default:
			log("SYSERR: Missing level for warrior thac0.");
		}
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level) {
		case 0:
			return 100;
		case 1:
			return 20;
		case 2:
			return 19;
		case 3:
			return 18;
		case 4:
			return 18;
		case 5:
			return 17;
		case 6:
			return 16;
		case 7:
			return 16;
		case 8:
			return 15;
		case 9:
			return 14;
		case 10:
			return 14;
		case 11:
			return 13;
		case 12:
			return 12;
		case 13:
			return 11;
		case 14:
			return 11;
		case 15:
			return 10;
		case 16:
			return 10;
		case 17:
			return 9;
		case 18:
			return 8;
		case 19:
			return 7;
		case 20:
			return 6;
		case 21:
			return 6;
		case 22:
			return 5;
		case 23:
			return 5;
		case 24:
			return 4;
		case 25:
			return 4;
		case 26:
			return 3;
		case 27:
			return 3;
		case 28:
			return 2;
		case 29:
			return 1;
		case 30:
			return 0;
		case 31:
			return 0;
		case 32:
			return 0;
		case 33:
			return 0;
		case 34:
			return 0;
		default:
			log("SYSERR: Missing level for warrior thac0.");
		}

	default:
		log("SYSERR: Unknown class in thac0 chart.");
	}

	/* Will not get there unless something is wrong. */
	return 100;
}

//*************************************************************************

/* AC0 for classes and levels. */
int extra_aco(int class_num, int level)
{
	switch (class_num) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return 0;
		case 9:
			return 0;
		case 10:
			return 0;
		case 11:
			return 0;
		case 12:
			return 0;
		case 13:
			return 0;
		case 14:
			return 0;
		case 15:
			return 0;
		case 16:
			return -1;
		case 17:
			return -1;
		case 18:
			return -1;
		case 19:
			return -1;
		case 20:
			return -1;
		case 21:
			return -1;
		case 22:
			return -1;
		case 23:
			return -1;
		case 24:
			return -1;
		case 25:
			return -1;
		case 26:
			return -1;
		case 27:
			return -1;
		case 28:
			return -1;
		case 29:
			return -1;
		case 30:
			return -1;
		case 31:
			return -5;
		case 32:
			return -10;
		case 33:
			return -15;
		case 34:
			return -20;
		default:
			return 0;
		}
	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return 0;
		case 9:
			return 0;
		case 10:
			return 0;
		case 11:
			return -1;
		case 12:
			return -1;
		case 13:
			return -1;
		case 14:
			return -1;
		case 15:
			return -1;
		case 16:
			return -1;
		case 17:
			return -1;
		case 18:
			return -1;
		case 19:
			return -1;
		case 20:
			return -1;
		case 21:
			return -2;
		case 22:
			return -2;
		case 23:
			return -2;
		case 24:
			return -2;
		case 25:
			return -2;
		case 26:
			return -2;
		case 27:
			return -2;
		case 28:
			return -2;
		case 29:
			return -2;
		case 30:
			return -2;
		case 31:
			return -5;
		case 32:
			return -10;
		case 33:
			return -15;
		case 34:
			return -20;
		default:
			return 0;
		}
	case CLASS_ASSASINE:
	case CLASS_THIEF:
	case CLASS_MERCHANT:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return 0;
		case 9:
			return -1;
		case 10:
			return -1;
		case 11:
			return -1;
		case 12:
			return -1;
		case 13:
			return -1;
		case 14:
			return -1;
		case 15:
			return -1;
		case 16:
			return -1;
		case 17:
			return -2;
		case 18:
			return -2;
		case 19:
			return -2;
		case 20:
			return -2;
		case 21:
			return -2;
		case 22:
			return -2;
		case 23:
			return -2;
		case 24:
			return -2;
		case 25:
			return -3;
		case 26:
			return -3;
		case 27:
			return -3;
		case 28:
			return -3;
		case 29:
			return -3;
		case 30:
			return -3;
		case 31:
			return -5;
		case 32:
			return -10;
		case 33:
			return -15;
		case 34:
			return -20;
		default:
			return 0;
		}
	case CLASS_WARRIOR:
	case CLASS_GUARD:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return -1;
		case 7:
			return -1;
		case 8:
			return -1;
		case 9:
			return -1;
		case 10:
			return -1;
		case 11:
			return -2;
		case 12:
			return -2;
		case 13:
			return -2;
		case 14:
			return -2;
		case 15:
			return -2;
		case 16:
			return -3;
		case 17:
			return -3;
		case 18:
			return -3;
		case 19:
			return -3;
		case 20:
			return -3;
		case 21:
			return -4;
		case 22:
			return -4;
		case 23:
			return -4;
		case 24:
			return -4;
		case 25:
			return -4;
		case 26:
			return -5;
		case 27:
			return -5;
		case 28:
			return -5;
		case 29:
			return -5;
		case 30:
			return -5;
		case 31:
			return -10;
		case 32:
			return -15;
		case 33:
			return -20;
		case 34:
			return -25;
		default:
			return 0;
		}
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return -1;
		case 9:
			return -1;
		case 10:
			return -1;
		case 11:
			return -1;
		case 12:
			return -1;
		case 13:
			return -1;
		case 14:
			return -1;
		case 15:
			return -2;
		case 16:
			return -2;
		case 17:
			return -2;
		case 18:
			return -2;
		case 19:
			return -2;
		case 20:
			return -2;
		case 21:
			return -2;
		case 22:
			return -3;
		case 23:
			return -3;
		case 24:
			return -3;
		case 25:
			return -3;
		case 26:
			return -3;
		case 27:
			return -3;
		case 28:
			return -3;
		case 29:
			return -4;
		case 30:
			return -4;
		case 31:
			return -5;
		case 32:
			return -10;
		case 33:
			return -15;
		case 34:
			return -20;
		default:
			return 0;
		}
	}
	return 0;
}


/* DAMROLL for classes and levels. */
int extra_damroll(int class_num, int level)
{
	switch (class_num) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return 0;
		case 9:
			return 0;
		case 10:
			return 0;
		case 11:
			return 1;
		case 12:
			return 1;
		case 13:
			return 1;
		case 14:
			return 1;
		case 15:
			return 1;
		case 16:
			return 1;
		case 17:
			return 1;
		case 18:
			return 1;
		case 19:
			return 1;
		case 20:
			return 1;
		case 21:
			return 2;
		case 22:
			return 2;
		case 23:
			return 2;
		case 24:
			return 2;
		case 25:
			return 2;
		case 26:
			return 2;
		case 27:
			return 2;
		case 28:
			return 2;
		case 29:
			return 2;
		case 30:
			return 2;
		case 31:
			return 4;
		case 32:
			return 6;
		case 33:
			return 8;
		case 34:
			return 10;
		default:
			return 0;
		}
	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return 0;
		case 9:
			return 1;
		case 10:
			return 1;
		case 11:
			return 1;
		case 12:
			return 1;
		case 13:
			return 1;
		case 14:
			return 1;
		case 15:
			return 1;
		case 16:
			return 1;
		case 17:
			return 2;
		case 18:
			return 2;
		case 19:
			return 2;
		case 20:
			return 2;
		case 21:
			return 2;
		case 22:
			return 2;
		case 23:
			return 2;
		case 24:
			return 2;
		case 25:
			return 3;
		case 26:
			return 3;
		case 27:
			return 3;
		case 28:
			return 3;
		case 29:
			return 3;
		case 30:
			return 3;
		case 31:
			return 5;
		case 32:
			return 7;
		case 33:
			return 9;
		case 34:
			return 10;
		default:
			return 0;
		}
	case CLASS_ASSASINE:
	case CLASS_THIEF:
	case CLASS_MERCHANT:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 0;
		case 8:
			return 1;
		case 9:
			return 1;
		case 10:
			return 1;
		case 11:
			return 1;
		case 12:
			return 1;
		case 13:
			return 1;
		case 14:
			return 1;
		case 15:
			return 2;
		case 16:
			return 2;
		case 17:
			return 2;
		case 18:
			return 2;
		case 19:
			return 2;
		case 20:
			return 2;
		case 21:
			return 2;
		case 22:
			return 3;
		case 23:
			return 3;
		case 24:
			return 3;
		case 25:
			return 3;
		case 26:
			return 3;
		case 27:
			return 3;
		case 28:
			return 3;
		case 29:
			return 4;
		case 30:
			return 4;
		case 31:
			return 5;
		case 32:
			return 7;
		case 33:
			return 9;
		case 34:
			return 11;
		default:
			return 0;
		}
	case CLASS_WARRIOR:
	case CLASS_GUARD:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 1;
		case 7:
			return 1;
		case 8:
			return 1;
		case 9:
			return 1;
		case 10:
			return 1;
		case 11:
			return 2;
		case 12:
			return 2;
		case 13:
			return 2;
		case 14:
			return 2;
		case 15:
			return 2;
		case 16:
			return 3;
		case 17:
			return 3;
		case 18:
			return 3;
		case 19:
			return 3;
		case 20:
			return 3;
		case 21:
			return 4;
		case 22:
			return 4;
		case 23:
			return 4;
		case 24:
			return 4;
		case 25:
			return 4;
		case 26:
			return 5;
		case 27:
			return 5;
		case 28:
			return 5;
		case 29:
			return 5;
		case 30:
			return 5;
		case 31:
			return 10;
		case 32:
			return 15;
		case 33:
			return 20;
		case 34:
			return 25;
		default:
			return 0;
		}
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 0;
		case 2:
			return 0;
		case 3:
			return 0;
		case 4:
			return 0;
		case 5:
			return 0;
		case 6:
			return 0;
		case 7:
			return 1;
		case 8:
			return 1;
		case 9:
			return 1;
		case 10:
			return 1;
		case 11:
			return 1;
		case 12:
			return 1;
		case 13:
			return 2;
		case 14:
			return 2;
		case 15:
			return 2;
		case 16:
			return 2;
		case 17:
			return 2;
		case 18:
			return 2;
		case 19:
			return 3;
		case 20:
			return 3;
		case 21:
			return 3;
		case 22:
			return 3;
		case 23:
			return 3;
		case 24:
			return 3;
		case 25:
			return 4;
		case 26:
			return 4;
		case 27:
			return 4;
		case 28:
			return 4;
		case 29:
			return 4;
		case 30:
			return 4;
		case 31:
			return 5;
		case 32:
			return 10;
		case 33:
			return 15;
		case 34:
			return 20;
		default:
			return 0;
		}
	}
	return 0;
}

/* Some initializations for characters, including initial skills */
void do_start(CHAR_DATA * ch, int newbie)
{
	OBJ_DATA *obj;
	int i;

	GET_LEVEL(ch) = 1;
	GET_EXP(ch) = 1;

//MZ.tops
	upd_p_max_remort_top(ch);
//-MZ.tops

	if (GET_REMORT(ch) == 0)
		set_title(ch, NULL);

	if (newbie && GET_CLASS(ch) == CLASS_DRUID)
		for (i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(ch, i) = SPELL_RUNES;

	ch->points.max_hit = 10;

	obj = read_object(START_BOTTLE, VIRTUAL);
	if (obj)
		obj_to_char(obj, ch);
	obj = read_object(START_BREAD, VIRTUAL);
	if (obj)
		obj_to_char(obj, ch);
	obj = read_object(START_BREAD, VIRTUAL);
	if (obj)
		obj_to_char(obj, ch);
	obj = read_object(START_SCROLL, VIRTUAL);
	if (obj)
		obj_to_char(obj, ch);
	obj = read_object(START_LIGHT, VIRTUAL);
	if (obj)
		obj_to_char(obj, ch);
	SET_SKILL(ch, SKILL_DRUNKOFF, 10);

	switch (calc_loadroom(ch)) {
	case 4056:
		obj = read_object(4004, VIRTUAL);
		break;
	case 5000:
		obj = read_object(5017, VIRTUAL);
		break;
	case 6049:
		obj = read_object(6004, VIRTUAL);
		break;
	default:
		obj = NULL;
		break;
	}
	if (obj)
		obj_to_char(obj, ch);

	switch (GET_CLASS(ch)) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		obj = read_object(START_KNIFE, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_WIELD);
		SET_SKILL(ch, SKILL_SATTACK, 10);
		break;

	case CLASS_DRUID:
		obj = read_object(START_ERUNE, VIRTUAL);
		if (obj)
			obj_to_char(obj, ch);
		obj = read_object(START_WRUNE, VIRTUAL);
		if (obj)
			obj_to_char(obj, ch);
		obj = read_object(START_ARUNE, VIRTUAL);
		if (obj)
			obj_to_char(obj, ch);
		obj = read_object(START_FRUNE, VIRTUAL);
		if (obj)
			obj_to_char(obj, ch);
		break;
		
	case CLASS_CLERIC:
		obj = read_object(START_CLUB, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_WIELD);
		SET_SKILL(ch, SKILL_SATTACK, 50);
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		obj = read_object(START_KNIFE, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_WIELD);
		SET_SKILL(ch, SKILL_SATTACK, 75);
		break;

	case CLASS_WARRIOR:
		obj = read_object(START_SWORD, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_WIELD);
		obj = read_object(START_ARMOR, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_BODY);
		SET_SKILL(ch, SKILL_SATTACK, 95);
		SET_SKILL(ch, SKILL_HORSE, 10);
		break;

	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_SMITH:
		obj = read_object(START_SWORD, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_WIELD);
		obj = read_object(START_ARMOR, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_BODY);
		SET_SKILL(ch, SKILL_SATTACK, 95);
		if (GET_CLASS(ch) != CLASS_SMITH)
			SET_SKILL(ch, SKILL_HORSE, 10);
		break;

	case CLASS_RANGER:
		obj = read_object(START_BOW, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_BOTHS);
		obj = read_object(START_ARMOR, VIRTUAL);
		if (obj)
			equip_char(ch, obj, WEAR_BODY);
		SET_SKILL(ch, SKILL_SATTACK, 95);
		SET_SKILL(ch, SKILL_HORSE, 10);
		break;

	}

	switch (GET_KIN(ch)) {
	case KIN_RUSICHI:
		break;
	case KIN_VIKINGI:
		break;		
	case KIN_STEPNYAKI:
		break;		
	}

	switch (GET_RACE(ch)) {
	case RACE_SEVERANE:
		break;
	case RACE_POLANE:
		break;
	case RACE_KRIVICHI:
		break;
	case RACE_VATICHI:
		break;
	case RACE_VELANE:
		break;
	case RACE_DREVLANE:
		break;
	case RACE_POLOVCI:
		break;        
	case RACE_PECHENEGI:      
		break;
	case RACE_MONGOLI:        
		break;
	case RACE_YIGURI:        
		break;
	case RACE_KANGARI:        
		break;
	case RACE_XAZARI:         
		break;
	case RACE_SVEI:         
		break;
	case RACE_DATCHANE:       
		break;
	case RACE_GETTI:        
		break;
	case RACE_UTTI:        
		break;
	case RACE_XALEIGI:        
		break;
	case RACE_NORVEZCI:       
		break;

	}

	switch (GET_RELIGION(ch)) {
	case RELIGION_POLY:
		break;
	case RELIGION_MONO:
		break;
	}

	advance_level(ch);
	sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
	mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);

	GET_HIT(ch) = GET_REAL_MAX_HIT(ch);
	GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);

	GET_COND(ch, THIRST) = 24;
	GET_COND(ch, FULL) = 24;
	GET_COND(ch, DRUNK) = 0;

// Gunner а вот тут дырка после реморта тоже выполняется do_start и 
// нам не нужно обнулять время в игре ни время последнего логона
//  ch->player.time.played = 0;
//  ch->player.time.logon  = time(0);

	if (siteok_everyone)
		SET_BIT(PLR_FLAGS(ch, PLR_SITEOK), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void o_advance_level(CHAR_DATA * ch)
{
	int add_hp_min, add_hp_max, add_move = 0, i;

	add_hp_min = MIN(GET_REAL_CON(ch), GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp);
	add_hp_max = MAX(GET_REAL_CON(ch), GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp);

	switch (GET_CLASS(ch)) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		// Not more then BORN CON - 1
		add_hp_min = MIN(add_hp_min, GET_CON(ch) - 3);
		add_hp_max = MIN(add_hp_max, GET_CON(ch) - 1);
		add_move = 2;
		break;

	case CLASS_CLERIC:
		// Not more then BORN CON + 1
		add_hp_min = MIN(add_hp_min, GET_CON(ch) - 2);
		add_hp_max = MIN(add_hp_max, GET_CON(ch) + 2);
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;

	case CLASS_DRUID:
		// Not more then BORN CON + 1
		add_hp_min = MIN(add_hp_min, GET_CON(ch) - 2);
		add_hp_max = MIN(add_hp_max, GET_CON(ch) + 1);
		add_move = 2;
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		// Not more then BORN CON + 2
		add_hp_min = MIN(add_hp_min, GET_CON(ch) - 1);
		add_hp_max = MIN(add_hp_max, GET_CON(ch) + 2);
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;

	case CLASS_WARRIOR:
		// Not less then BORN CON
		add_hp_min = MAX(add_hp_min, GET_CON(ch));
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;

	case CLASS_GUARD:
	case CLASS_RANGER:
	case CLASS_PALADINE:
	case CLASS_SMITH:
		// Not more then BORN CON + 5
		// Not less then BORN CON - 2
		add_hp_min = MAX(add_hp_min, GET_CON(ch) - 2);
		add_hp_max = MIN(add_hp_max, GET_CON(ch) + 5);
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;
	}

	for (i = 1; i < MAX_FEATS; i++)	
		if (feat_info[i].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] && can_get_feat(ch, i))
			SET_FEAT(ch, i); 

	add_hp_min = MIN(add_hp_min, add_hp_max);
	add_hp_min = MAX(1, add_hp_min);
	add_hp_max = MAX(1, add_hp_max);
	log("Add hp for %s in range %d..%d", GET_NAME(ch), add_hp_min, add_hp_max);
	ch->points.max_hit += number(add_hp_min, add_hp_max);
	ch->points.max_move += MAX(1, add_move);

	if (IS_IMMORTAL(ch)) {
		for (i = 0; i < 3; i++)
			GET_COND(ch, i) = (char) -1;
		SET_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);
	}

	save_char(ch, NOWHERE);
}

void o_decrease_level(CHAR_DATA * ch)
{
	int add_hp, add_move = 0;

	add_hp =
	    MAX(MAX
		(GET_REAL_CON(ch),
		 GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp),
		MAX(GET_CON(ch), GET_CON(ch) + con_app[GET_CON(ch)].hitp));

	switch (GET_CLASS(ch)) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		add_move = 2;
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		add_move = 2;
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		add_move = GET_DEX(ch) / 5 + 1;
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		add_move = GET_DEX(ch) / 5 + 1;
		break;
	}

	log("Dec hp for %s set ot %d", GET_NAME(ch), add_hp);
	ch->points.max_hit -= MIN(ch->points.max_hit, MAX(1, add_hp));
	ch->points.max_move -= MIN(ch->points.max_move, MAX(1, add_move));

	if (!IS_IMMORTAL(ch))
		REMOVE_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);

	save_char(ch, NOWHERE);
}

void advance_level(CHAR_DATA * ch)
{
	int con, add_hp = 0, add_move = 0, i;

	con = MAX(class_app[(int) GET_CLASS(ch)].min_con, MIN(GET_CON(ch), class_app[(int) GET_CLASS(ch)].max_con));
	add_hp = class_app[(int) GET_CLASS(ch)].base_con + (con - class_app[(int)
									    GET_CLASS(ch)].
							    base_con) *
	    class_app[(int) GET_CLASS(ch)].koef_con / 100 + number(1, 3);

	switch (GET_CLASS(ch)) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		add_move = 2;
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		add_move = 2;
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;

	case CLASS_WARRIOR:
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;

	case CLASS_GUARD:
	case CLASS_RANGER:
	case CLASS_PALADINE:
	case CLASS_SMITH:
		add_move = number(GET_DEX(ch) / 6 + 1, GET_DEX(ch) / 5 + 1);
		break;
	}

	log("Add hp for %s is %d", GET_NAME(ch), add_hp);
	ch->points.max_hit += MAX(1, add_hp);
	ch->points.max_move += MAX(1, add_move);

        for (i = 1; i < MAX_FEATS; i++)
                if (feat_info[i].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] && can_get_feat(ch, i))
	                SET_FEAT(ch, i);

	if (IS_IMMORTAL(ch)) {
		for (i = 0; i < 3; i++)
			GET_COND(ch, i) = (char) -1;
		SET_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);
	}

	save_char(ch, NOWHERE);
}

void decrease_level(CHAR_DATA * ch)
{
	int con, add_hp, add_move = 0;
	int prob, sval, max;

	con = MAX(class_app[(int) GET_CLASS(ch)].min_con, MIN(GET_CON(ch), class_app[(int) GET_CLASS(ch)].max_con));
	add_hp = class_app[(int) GET_CLASS(ch)].base_con + (con - class_app[(int)
									    GET_CLASS(ch)].
							    base_con) *
	    class_app[(int) GET_CLASS(ch)].koef_con / 100 + 3;

	switch (GET_CLASS(ch)) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		add_move = 2;
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		add_move = 2;
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		add_move = GET_DEX(ch) / 5 + 1;
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		add_move = GET_DEX(ch) / 5 + 1;
		break;
	}

	log("Dec hp for %s set ot %d", GET_NAME(ch), add_hp);
	ch->points.max_hit -= MIN(ch->points.max_hit, MAX(1, add_hp));
	ch->points.max_move -= MIN(ch->points.max_move, MAX(1, add_move));
	GET_WIMP_LEV(ch) = MAX(0, MIN(GET_WIMP_LEV(ch), GET_REAL_MAX_HIT(ch) / 2));
	if (!IS_IMMORTAL(ch))
		REMOVE_BIT(PRF_FLAGS(ch, PRF_HOLYLIGHT), PRF_HOLYLIGHT);

	for (prob = 0; prob <= MAX_SKILLS; prob++)
		if (GET_SKILL(ch, prob) && prob != SKILL_SATTACK) {
			max = wis_app[GET_REAL_WIS(ch)].max_learn_l20 * (GET_LEVEL(ch) + 1) / 20;
			if (max > MAX_EXP_PERCENT)
				max = MAX_EXP_PERCENT;
			sval = GET_SKILL(ch, prob) - max - GET_REMORT(ch) * 5;
			if (sval < 0)
				sval = 0;
			if ((GET_SKILL(ch, prob) - sval) >
			    (wis_app[GET_REAL_WIS(ch)].max_learn_l20 * GET_LEVEL(ch) / 20))
				GET_SKILL(ch, prob) =
				    (wis_app[GET_REAL_WIS(ch)].max_learn_l20 * GET_LEVEL(ch) / 20) + sval;
		}

	save_char(ch, NOWHERE);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
	if (level <= 0)
		return 1;	/* level 0 */
	else if (level <= 5)
		return 2;	/* level 1 - 7 */
	else if (level <= 10)
		return 3;	/* level 8 - 13 */
	else if (level <= 15)
		return 4;	/* level 14 - 20 */
	else if (level <= 20)
		return 5;	/* level 21 - 28 */
	else if (level <= 25)
		return 6;	/* level 21 - 28 */
	else if (level <= 30)
		return 7;	/* level 21 - 28 */
	else
		return 10;
	
//	Adept: убрал, бо хайлевел мобы со стаба батыров сносили :)
//	 if (level < LVL_GRGOD)
//		return 10;	/* all remaining mortal levels */
//	else
//		return 20;	/* immortals */ 
}


/*
 * invalid_class is used by handler.cpp to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_unique(CHAR_DATA * ch, OBJ_DATA * obj)
{
	OBJ_DATA *object;
	if (!IS_CORPSE(obj))
		for (object = obj->contains; object; object = object->next_content)
			if (invalid_unique(ch, object))
				return (TRUE);
	if (!ch ||
	    !obj ||
	    (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM)) ||
	    IS_IMMORTAL(ch) || obj->obj_flags.Obj_owner == 0 || obj->obj_flags.Obj_owner == GET_UNIQUE(ch))
		return (FALSE);
	return (TRUE);
}

int invalid_anti_class(CHAR_DATA * ch, OBJ_DATA * obj)
{
	OBJ_DATA *object;
	if (!IS_CORPSE(obj))
		for (object = obj->contains; object; object = object->next_content)
			if (invalid_anti_class(ch, object))
				return (TRUE);
	if (IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE) && AFF_FLAGGED(ch, AFF_CHARM))
		return (TRUE);
	if ((IS_NPC(ch) || WAITLESS(ch)) && !IS_CHARMICE(ch))
		return (FALSE);

	if (			// (IS_OBJ_ANTI(obj, ITEM_ANTI_MONO)     && GET_RELIGION(ch) == RELIGION_MONO) ||
		   // (IS_OBJ_ANTI(obj, ITEM_ANTI_POLY)     && GET_RELIGION(ch) == RELIGION_POLY) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_BATTLEMAGE) && IS_BATTLEMAGE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_CHARMMAGE) && IS_CHARMMAGE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_DEFENDERMAGE) && IS_DEFENDERMAGE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_NECROMANCER) && IS_NECROMANCER(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_MALE) && IS_MALE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_FEMALE) && IS_FEMALE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_CLERIC) && IS_CLERIC(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_WARRIOR) && IS_WARRIOR(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_GUARD) && IS_GUARD(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_THIEF) && IS_THIEF(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_ASSASINE) && IS_ASSASINE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_PALADINE) && IS_PALADINE(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_RANGER) && IS_RANGER(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_SMITH) && IS_SMITH(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_MERCHANT) && IS_MERCHANT(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_DRUID) && IS_DRUID(ch)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_KILLER) && PLR_FLAGGED(ch, PLR_KILLER)) ||
		   (IS_OBJ_ANTI(obj, ITEM_AN_KILLERONLY)
		    && !PLR_FLAGGED(ch, PLR_KILLER))
		   || (IS_OBJ_ANTI(obj, ITEM_AN_COLORED) && IS_COLORED(ch))
		   || (IS_OBJ_ANTI(obj, ITEM_AN_SEVERANE)
		       && GET_RACE(ch) == RACE_SEVERANE)
		   || (IS_OBJ_ANTI(obj, ITEM_AN_POLANE) && GET_RACE(ch) == RACE_POLANE)
		   || (IS_OBJ_ANTI(obj, ITEM_AN_KRIVICHI)
		       && GET_RACE(ch) == RACE_KRIVICHI)
		   || (IS_OBJ_ANTI(obj, ITEM_AN_VATICHI)
		       && GET_RACE(ch) == RACE_VATICHI)
		   || (IS_OBJ_ANTI(obj, ITEM_AN_VELANE) && GET_RACE(ch) == RACE_VELANE)
		   || (IS_OBJ_ANTI(obj, ITEM_AN_DREVLANE)
		       && GET_RACE(ch) == RACE_DREVLANE)
		   || (IS_OBJ_ANTI (obj, ITEM_AN_POLOVCI)
		       && GET_RACE (ch) == RACE_POLOVCI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_PECHENEGI) 
           	       && GET_RACE (ch) == RACE_PECHENEGI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_MONGOLI)
		       && GET_RACE (ch) == RACE_MONGOLI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_YIGURI)
  	               && GET_RACE (ch) == RACE_YIGURI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_KANGARI)
	               && GET_RACE (ch) == RACE_KANGARI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_XAZARI)
		       && GET_RACE (ch) == RACE_XAZARI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_SVEI)
		       && GET_RACE (ch) == RACE_SVEI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_DATCHANE) 
	               && GET_RACE (ch) == RACE_DATCHANE)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_GETTI)
		       && GET_RACE (ch) == RACE_GETTI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_UTTI)
		       && GET_RACE (ch) == RACE_UTTI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_XALEIGI)
		       && GET_RACE (ch) == RACE_XALEIGI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_NORVEZCI)
		       && GET_RACE (ch) == RACE_NORVEZCI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_RUSICHI)
		       && GET_KIN (ch) == KIN_RUSICHI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_STEPNYAKI) 
	               && GET_KIN (ch) == KIN_STEPNYAKI)
	           || (IS_OBJ_ANTI (obj, ITEM_AN_VIKINGI)
		       && GET_KIN (ch) == KIN_VIKINGI))
		return (TRUE);
	return (FALSE);
}

int invalid_no_class(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_OBJ_NO(obj, ITEM_NO_CHARMICE) && AFF_FLAGGED(ch, AFF_CHARM))
		return (TRUE);
	if ((IS_NPC(ch) || WAITLESS(ch)) && !IS_CHARMICE(ch))
		return (FALSE);

	if ((IS_OBJ_NO(obj, ITEM_NO_MONO) && GET_RELIGION(ch) == RELIGION_MONO) ||
	    (IS_OBJ_NO(obj, ITEM_NO_POLY) && GET_RELIGION(ch) == RELIGION_POLY) ||
	    (IS_OBJ_NO(obj, ITEM_NO_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_BATTLEMAGE) && IS_BATTLEMAGE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_CHARMMAGE) && IS_CHARMMAGE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_DEFENDERMAGE) && IS_DEFENDERMAGE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_NECROMANCER) && IS_NECROMANCER(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_MALE) && IS_MALE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_FEMALE) && IS_FEMALE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_CLERIC) && IS_CLERIC(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_WARRIOR) && IS_WARRIOR(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_GUARD) && IS_GUARD(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_THIEF) && IS_THIEF(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_ASSASINE) && IS_ASSASINE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_PALADINE) && IS_PALADINE(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_RANGER) && IS_RANGER(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_SMITH) && IS_SMITH(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_MERCHANT) && IS_MERCHANT(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_DRUID) && IS_DRUID(ch)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_KILLER) && PLR_FLAGGED(ch, PLR_KILLER)) ||
	    (IS_OBJ_NO(obj, ITEM_NO_KILLERONLY) && !PLR_FLAGGED(ch, PLR_KILLER))
	    || (IS_OBJ_NO(obj, ITEM_NO_COLORED) && IS_COLORED(ch))
	    || (IS_OBJ_NO(obj, ITEM_NO_SEVERANE)
		&& GET_RACE(ch) == RACE_SEVERANE)
	    || (IS_OBJ_NO(obj, ITEM_NO_POLANE) && GET_RACE(ch) == RACE_POLANE)
	    || (IS_OBJ_NO(obj, ITEM_NO_KRIVICHI)
		&& GET_RACE(ch) == RACE_KRIVICHI)
	    || (IS_OBJ_NO(obj, ITEM_NO_VATICHI) && GET_RACE(ch) == RACE_VATICHI)
	    || (IS_OBJ_NO(obj, ITEM_NO_VELANE) && GET_RACE(ch) == RACE_VELANE)
	    || (IS_OBJ_NO(obj, ITEM_NO_DREVLANE)
		&& GET_RACE(ch) == RACE_DREVLANE)
	    || (IS_OBJ_NO (obj, ITEM_AN_POLOVCI)
	        && GET_RACE (ch) == RACE_POLOVCI)
   	    || (IS_OBJ_NO (obj, ITEM_AN_PECHENEGI) 
	        && GET_RACE (ch) == RACE_PECHENEGI)
	    || (IS_OBJ_NO (obj, ITEM_AN_MONGOLI)
	        && GET_RACE (ch) == RACE_MONGOLI)
	    || (IS_OBJ_NO (obj, ITEM_AN_YIGURI)
	        && GET_RACE (ch) == RACE_YIGURI)
	    || (IS_OBJ_NO (obj, ITEM_AN_KANGARI)
	        && GET_RACE (ch) == RACE_KANGARI)
	    || (IS_OBJ_NO (obj, ITEM_AN_XAZARI)      
	        && GET_RACE (ch) == RACE_XAZARI)
	    || (IS_OBJ_NO (obj, ITEM_AN_SVEI)
	        && GET_RACE (ch) == RACE_SVEI)
	    || (IS_OBJ_NO (obj, ITEM_AN_DATCHANE) 
	        && GET_RACE (ch) == RACE_DATCHANE)
	    || (IS_OBJ_NO (obj, ITEM_AN_GETTI)
	        && GET_RACE (ch) == RACE_GETTI)
	    || (IS_OBJ_NO (obj, ITEM_AN_UTTI)
	        && GET_RACE (ch) == RACE_UTTI)
	    || (IS_OBJ_NO (obj, ITEM_AN_XALEIGI)
	        && GET_RACE (ch) == RACE_XALEIGI)
	    || (IS_OBJ_NO (obj, ITEM_AN_NORVEZCI)
	        && GET_RACE (ch) == RACE_NORVEZCI)
	    || (IS_OBJ_NO (obj, ITEM_AN_RUSICHI)
	        && GET_KIN (ch) == KIN_RUSICHI)
	    || (IS_OBJ_NO (obj, ITEM_AN_STEPNYAKI) 
	        && GET_KIN (ch) == KIN_STEPNYAKI)
	    || (IS_OBJ_NO (obj, ITEM_AN_VIKINGI)
	        && GET_KIN (ch) == KIN_VIKINGI)
	    || ((OBJ_FLAGGED(obj, ITEM_ARMORED) || OBJ_FLAGGED(obj, ITEM_SHARPEN))
		&& !IS_SMITH(ch)))
		return (TRUE);
	return (FALSE);
}




/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void)
{
	FILE *magic;
	char line1[256], line2[256], line3[256], line4[256], name[256];
	int i[15], c, j, sp_num, l;
	if (!(magic = fopen(LIB_MISC "magic.lst", "r"))) {
		log("Cann't open magic list file...");
		_exit(1);
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %d %d %d %d %d %d", line1, line2, i, i + 1, i + 2, i + 3, i + 4, i+5 ) != 8) {
			log("Bad format for magic string !\r\n"
			    "Format : <spell name (%%s %%s)> <kin (%%d)> <classes (%%d)> <remort (%%d)> <slot (%%d)> <level (%%d)>");
			_exit(1);
		}

		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*') {
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}

		if ((sp_num = find_spell_num(name)) < 0) {
			log("Spell '%s' not found...", name);
			_exit(1);
		}

		if (i[0] < 0 || i[0] >= NUM_KIN){
			log ("Bad kin type for spell '%s' \"%d\"...", name, sp_num);
			_exit (1);
		}	
		if (i[1] < 0 || i[1] >= NUM_CLASSES){
			log ("Bad class type for spell '%s'  \"%d\"...", name, sp_num);
			_exit (1);
		}
		if (i[2] < 0 || i[2] >= MAX_REMORT){
			log ("Bad remort type for spell '%s'  \"%d\"...", name, sp_num);
			_exit (1);
		}
		mspell_remort (name,sp_num, i[0], i[1], i[2]);
		mspell_level (name,sp_num, i[0] ,i[1], i[4]);
		mspell_slot (name,sp_num, i[0], i[1], i[3]);
		mspell_change(name,sp_num, i[0], i[1], i[5]);

	}
	fclose(magic);
	if (!(magic = fopen(LIB_MISC "items.lst", "r"))) {
		log("Cann't open items list file...");
		_exit(1);
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %s %d %d %d %d %d", line1, line2, line3, i, i + 1, i + 2, i + 3, i + 4) != 8) {
			log("Bad format for magic string !\r\n"
			    "Format : <spell name (%%s %%s)> <type (%%s)> <items_vnum (%%d %%d %%d %%d)>");
			_exit(1);
		}

		if (i[4] > 34)
			i[4] = 34;
		if (i[4] < 1)
			i[4] = 1;

		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*') {
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = find_spell_num(name)) < 0) {
			log("Spell '%s' not found...", name);
			_exit(1);
		}
		c = strlen(line3);
		if (!strn_cmp(line3, "potion", c)) {
			spell_create[sp_num].potion.items[0] = i[0];
			spell_create[sp_num].potion.items[1] = i[1];
			spell_create[sp_num].potion.items[2] = i[2];
			spell_create[sp_num].potion.rnumber = i[3];
			spell_create[sp_num].potion.min_caster_level = i[4];
			log("CREATE potion FOR MAGIC '%s'", spell_name(sp_num));
		} else if (!strn_cmp(line3, "wand", c)) {
			spell_create[sp_num].wand.items[0] = i[0];
			spell_create[sp_num].wand.items[1] = i[1];
			spell_create[sp_num].wand.items[2] = i[2];
			spell_create[sp_num].wand.rnumber = i[3];
			spell_create[sp_num].wand.min_caster_level = i[4];
			log("CREATE wand FOR MAGIC '%s'", spell_name(sp_num));
		} else if (!strn_cmp(line3, "scroll", c)) {
			spell_create[sp_num].scroll.items[0] = i[0];
			spell_create[sp_num].scroll.items[1] = i[1];
			spell_create[sp_num].scroll.items[2] = i[2];
			spell_create[sp_num].scroll.rnumber = i[3];
			spell_create[sp_num].scroll.min_caster_level = i[4];
			log("CREATE scroll FOR MAGIC '%s'", spell_name(sp_num));
		} else if (!strn_cmp(line3, "items", c)) {
			spell_create[sp_num].items.items[0] = i[0];
			spell_create[sp_num].items.items[1] = i[1];
			spell_create[sp_num].items.items[2] = i[2];
			spell_create[sp_num].items.rnumber = i[3];
			spell_create[sp_num].items.min_caster_level = i[4];
			log("CREATE items FOR MAGIC '%s'", spell_name(sp_num));
		} else if (!strn_cmp(line3, "runes", c)) {
			spell_create[sp_num].runes.items[0] = i[0];
			spell_create[sp_num].runes.items[1] = i[1];
			spell_create[sp_num].runes.items[2] = i[2];
			spell_create[sp_num].runes.rnumber = i[3];
			spell_create[sp_num].runes.min_caster_level = i[4];
			log("CREATE runes FOR MAGIC '%s'", spell_name(sp_num));
		} else {
			log("Unknown items option : %s", line3);
			_exit(1);
		}
	}
	fclose(magic);

/* Load features variables - added by Gorrah */
	if (!(magic = fopen(LIB_MISC "features.lst", "r"))) {
		log("Cann't open features list file...");
		_exit(1);
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf (name, "%s %s %d %d %d %d %d %d %d",	line1, line2, i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6) != 9) {
			log("Bad format for feature string !\r\n"
			 	 "Format : <feature name (%%s %%s)>  <kin (%%d %%d %%d)> <class (%%d)> <remort (%%d)> <level (%%d)> <naturalfeat (%%d)>!");
			_exit(1);
		}
		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*') {
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = find_feat_num(name)) <= 0) {
			log("Feat '%s' not found...", name);
			_exit(1);                 
		}
		for (j = 0; j < NUM_KIN; j++)
			if (i[j] < 0 || i[j] > 1){
				log ("Bad race feat know type for feat \"%s\"... 0 or 1 expected", feat_info[sp_num].name);
				_exit (1);
			}
		if (i[3] < 0 || i[3] >= NUM_CLASSES) {
			log("Bad class type for feat \"%s\"...", feat_info[sp_num].name);
			_exit(1);
		}
		if (i[4] < 0 || i[4] >= MAX_REMORT){
			log ("Bad remort type for feat \"%s\"...", feat_info[sp_num].name);
			_exit (1);
		}
		if (i[6] < 0 || i[6] > 1){
			log ("Bad natural classfeat type for feat \"%s\"... 0 or 1 expected", feat_info[sp_num].name);
			_exit (1);
		}
		for (j = 0; j < NUM_KIN; j++) 
			if (i[j] == 1) { 
				feat_info[sp_num].classknow[i[3]][j] = TRUE;
				log ("Classknow feat set '%d' kin '%d' classes %d", sp_num, j, i[3]);

				feat_info[sp_num].min_remort[i[3]][j] = i[4];
				log ("Remort feat set '%d' kin '%d' classes %d value %d", sp_num, j, i[3], i[4]);

				feat_info[sp_num].min_level[i[3]][j] = i[5];
				log ("Level feat set '%d' kin '%d' classes %d value %d", sp_num, j, i[3], i[5]);

				feat_info[sp_num].natural_classfeat[i[3]][j] = i[6];
				log ("Natural classfeature set '%d' kin '%d' classes %d", sp_num, j, i[3]);
			}       
	}
	fclose(magic); 
/* End of changed */

	if (!(magic = fopen(LIB_MISC "skills.lst", "r"))) {
		log("Cann't open skills list file...");
		_exit(1);
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf (name, "%s %s %d %d %d %d %d", line1, line2, i, i + 1,i + 2, i +3, i +4 ) != 7){
			log("Bad format for skill string !\r\n"
			    "Format : <skill name (%%s %%s)>  <kin (%%d)> <class (%%d)> <remort (%%d)> <improove (%%d)> !");
			_exit(1);
		}
		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*') {
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = find_skill_num(name)) < 0) {
			log("Skill '%s' not found...", name);
			_exit(1);
		}
		if (i[0] < 0 || i[0] >= NUM_KIN){
			log ("Bad kin type for skill \"%s\"...", skill_info[sp_num].name);
			_exit (1);
		}
		if (i[1] < 0 || i[1] >= NUM_CLASSES) {
			log("Bad class type for skill \"%s\"...", skill_info[sp_num].name);
			_exit(1);
		}
		if (i[2] < 0 || i[2] >= MAX_REMORT){
			log ("Bad remort type for skill \"%s\"...", skill_info[sp_num].name);
			_exit (1);
		}
		if (i[4]){
			skill_info[sp_num].k_improove[i[1]][i[0]] = MAX (1, i[4]);
			log ("Improove set '%d' kin '%d' classes %d value %d", sp_num,i[0], i[1], i[4]);
		}
		if (i[3]){
			skill_info[sp_num].min_level[i[1]][i[0]] = i[3];
			log ("Level set '%d' kin '%d' classes %d value %d", sp_num,i[0], i[1], i[3]);
		}	
		skill_info[sp_num].min_remort[i[1]][i[0]] = i[2];
		log ("Remort set '%d' kin '%d' classes %d value %d", sp_num,i[0], i[1], i[2]);
	}
	fclose(magic);
	if (!(magic = fopen(LIB_MISC "classskill.lst", "r"))) {
		log("Cann't open classskill list file...");
		_exit(1);
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf (name, "%s %s %s %s", line1, line2, line3, line4) != 4) {
			log("Bad format for skill string !\r\n" "Format : <skill name (%%s %%s)> <kin (%%s)> <skills (%%s)> !");
			_exit(1);
		}
		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*') {
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = find_skill_num(name)) < 0) {
			log("Skill '%s' not found...", name);
			_exit(1);
		}
		for (l = 0; line3[l] && l < NUM_KIN; l++){
			if (!strchr ("1xX!", line3[l]))
				continue;			
			for (j = 0; line4[j] && j < NUM_CLASSES; j++) {
				if (!strchr ("1xX!", line4[j]))
					continue;
				skill_info[sp_num].classknow[l][j] = KNOW_SKILL;
				log ("Set skill '%s' kin %d classes %d is Know",skill_info[sp_num].name, l, j);
			}
		}
	}
	fclose(magic);
	if (!(magic = fopen(LIB_MISC "skillvariables.lst", "r"))) {
		log("Cann't open skillvariables list file...");
		_exit(1);
	}

  /*** Загружаем переменные скилов из файла ***/

  /** ГОРНОЕ ДЕЛО **/
	// Предварительно ставим значения по дефолту

	dig_vars.hole_max_deep = DIG_DFLT_HOLE_MAX_DEEP;
	dig_vars.instr_crash_chance = DIG_DFLT_INSTR_CRASH_CHANCE;
	dig_vars.treasure_chance = DIG_DFLT_TREASURE_CHANCE;
	dig_vars.pandora_chance = DIG_DFLT_PANDORA_CHANCE;
	dig_vars.mob_chance = DIG_DFLT_MOB_CHANCE;
	dig_vars.trash_chance = DIG_DFLT_TRASH_CHANCE;
	dig_vars.lag = DIG_DFLT_LAG;
	dig_vars.prob_divide = DIG_DFLT_PROB_DIVIDE;
	dig_vars.glass_chance = DIG_DFLT_GLASS_CHANCE;
	dig_vars.need_moves = DIG_DFLT_NEED_MOVES;

	dig_vars.stone1_skill = DIG_DFLT_STONE1_SKILL;
	dig_vars.stone2_skill = DIG_DFLT_STONE2_SKILL;
	dig_vars.stone3_skill = DIG_DFLT_STONE3_SKILL;
	dig_vars.stone4_skill = DIG_DFLT_STONE4_SKILL;
	dig_vars.stone5_skill = DIG_DFLT_STONE5_SKILL;
	dig_vars.stone6_skill = DIG_DFLT_STONE6_SKILL;
	dig_vars.stone7_skill = DIG_DFLT_STONE7_SKILL;
	dig_vars.stone8_skill = DIG_DFLT_STONE8_SKILL;
	dig_vars.stone9_skill = DIG_DFLT_STONE9_SKILL;

	dig_vars.stone1_vnum = DIG_DFLT_STONE1_VNUM;
	dig_vars.trash_vnum_start = DIG_DFLT_TRASH_VNUM_START;
	dig_vars.trash_vnum_end = DIG_DFLT_TRASH_VNUM_END;
	dig_vars.mob_vnum_start = DIG_DFLT_MOB_VNUM_START;
	dig_vars.mob_vnum_end = DIG_DFLT_MOB_VNUM_END;
	dig_vars.pandora_vnum = DIG_DFLT_PANDORA_VNUM;

  /** ЮВЕЛИР **/
	// Предварительно ставим значения по дефолту

	insgem_vars.lag = INSGEM_DFLT_LAG;
	insgem_vars.minus_for_affect = INSGEM_DFLT_MINUS_FOR_AFFECT;
	insgem_vars.prob_divide = INSGEM_DFLT_PROB_DIVIDE;
	insgem_vars.dikey_percent = INSGEM_DFLT_DIKEY_PERCENT;
	insgem_vars.timer_plus_percent = INSGEM_DFLT_TIMER_PLUS_PERCENT;
	insgem_vars.timer_minus_percent = INSGEM_DFLT_TIMER_MINUS_PERCENT;



	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;

		sscanf(name, "dig_hole_max_deep %d", &dig_vars.hole_max_deep);
		sscanf(name, "dig_instr_crash_chance %d", &dig_vars.instr_crash_chance);
		sscanf(name, "dig_treasure_chance %d", &dig_vars.treasure_chance);
		sscanf(name, "dig_pandora_chance %d", &dig_vars.pandora_chance);
		sscanf(name, "dig_mob_chance %d", &dig_vars.mob_chance);
		sscanf(name, "dig_trash_chance %d", &dig_vars.trash_chance);
		sscanf(name, "dig_lag %d", &dig_vars.lag);
		sscanf(name, "dig_prob_divide %d", &dig_vars.prob_divide);
		sscanf(name, "dig_glass_chance %d", &dig_vars.glass_chance);
		sscanf(name, "dig_need_moves %d", &dig_vars.need_moves);

		sscanf(name, "dig_stone1_skill %d", &dig_vars.stone1_skill);
		sscanf(name, "dig_stone2_skill %d", &dig_vars.stone2_skill);
		sscanf(name, "dig_stone3_skill %d", &dig_vars.stone3_skill);
		sscanf(name, "dig_stone4_skill %d", &dig_vars.stone4_skill);
		sscanf(name, "dig_stone5_skill %d", &dig_vars.stone5_skill);
		sscanf(name, "dig_stone6_skill %d", &dig_vars.stone6_skill);
		sscanf(name, "dig_stone7_skill %d", &dig_vars.stone7_skill);
		sscanf(name, "dig_stone8_skill %d", &dig_vars.stone8_skill);
		sscanf(name, "dig_stone9_skill %d", &dig_vars.stone9_skill);

		sscanf(name, "dig_stone1_vnum %d", &dig_vars.stone1_vnum);
		sscanf(name, "dig_trash_vnum_start %d", &dig_vars.trash_vnum_start);
		sscanf(name, "dig_trash_vnum_end %d", &dig_vars.trash_vnum_end);
		sscanf(name, "dig_mob_vnum_start %d", &dig_vars.mob_vnum_start);
		sscanf(name, "dig_mob_vnum_end %d", &dig_vars.mob_vnum_end);
		sscanf(name, "dig_pandora_vnum %d", &dig_vars.pandora_vnum);

		sscanf(name, "insgem_lag %d", &insgem_vars.lag);
		sscanf(name, "insgem_minus_for_affect %d", &insgem_vars.minus_for_affect);
		sscanf(name, "insgem_prob_divide %d", &insgem_vars.prob_divide);
		sscanf(name, "insgem_dikey_percent %d", &insgem_vars.dikey_percent);
		sscanf(name, "insgem_timer_plus_percent %d", &insgem_vars.timer_plus_percent);
		sscanf(name, "insgem_timer_minus_percent %d", &insgem_vars.timer_minus_percent);

		name[0] = '\0';
	}
	fclose(magic);

/* Remove to init_im::im.cpp - Gorrah
// +newbook.patch (Alisher)
	if (!(magic = fopen(LIB_MISC "classrecipe.lst", "r"))) {
		log("Cann't open classrecipe list file...");
		_exit(1);
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%d %s %s", i, line1, line2) != 3) {
			log("Bad format for magic string !\r\n"
			    "Format : <recipe number (%%d)> <races (%%s)> <classes (%%d)>");
			_exit(1);
		}

		rcpt = im_get_recipe(i[0]);

		if (rcpt < 0) {
			log("Invalid recipe (%d)", i[0]);
			_exit(1);
		}

// line1 - ограничения для рас еще не реализованы

		for (j = 0; line2[j] && j < NUM_CLASSES; j++) {
			if (!strchr("1xX!", line2[j]))
				continue;
			imrecipes[rcpt].classknow[j] = KNOW_RECIPE;
			log("Set recipe (%d '%s') classes %d is Know", i[0], imrecipes[rcpt].name, j);
		}
	}
	fclose(magic);
// -newbook.patch (Alisher)
*/
	return;
}


void init_basic_values(void)
{
	FILE *magic;
	char line[256], name[MAX_INPUT_LENGTH];
	int i[10], c, j, mode = 0, *pointer;
	if (!(magic = fopen(LIB_MISC "basic.lst", "r"))) {
		log("Cann't open basic values list file...");
		return;
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		i[0] = i[1] = i[2] = i[3] = i[4] = i[5] = 100000;
		if (sscanf(name, "%s %d %d %d %d %d %d", line, i, i + 1, i + 2, i + 3, i + 4, i + 5) < 1)
			continue;
		if (!str_cmp(line, "str"))
			mode = 1;
		else if (!str_cmp(line, "dex_app"))
			mode = 2;
		else if (!str_cmp(line, "dex_skill"))
			mode = 3;
		else if (!str_cmp(line, "con"))
			mode = 4;
		else if (!str_cmp(line, "wis"))
			mode = 5;
		else if (!str_cmp(line, "int"))
			mode = 6;
		else if (!str_cmp(line, "cha"))
			mode = 7;
		else if (!str_cmp(line, "size"))
			mode = 8;
		else if (!str_cmp(line, "weapon"))
			mode = 9;
		else if ((c = atoi(line)) > 0 && c <= 50 && mode > 0 && mode < 10) {
			switch (mode) {
			case 1:
				pointer = (int *) &(str_app[c].tohit);
				break;
			case 2:
				pointer = (int *) &(dex_app[c].reaction);
				break;
			case 3:
				pointer = (int *) &(dex_app_skill[c].p_pocket);
				break;
			case 4:
				pointer = (int *) &(con_app[c].hitp);
				break;
			case 5:
				pointer = (int *) &(wis_app[c].spell_additional);
				break;
			case 6:
				pointer = (int *) &(int_app[c].spell_aknowlege);
				break;
			case 7:
				pointer = (int *) &(cha_app[c].leadership);
				break;
			case 8:
				pointer = (int *) &(size_app[c].ac);
				break;
			case 9:
				pointer = (int *) &(weapon_app[c].shocking);
				break;
			default:
				pointer = NULL;
			}
			if (pointer) {	//log("Mode %d - %d = %d %d %d %d %d %d",mode,c,
				//    *i, *(i+1), *(i+2), *(i+3), *(i+4), *(i+5));
				for (j = 0; j < 6; j++)
					if (i[j] != 100000) {	//log("[%d] %d <-> %d",j,*(pointer+j),*(i+j));
						*(pointer + j) = *(i + j);
					}
				//getchar();
			}
		}
	}
	fclose(magic);
	return;
}

int init_grouping(void)
{
	FILE *f;
	char buf[MAX_INPUT_LENGTH];
	int tmp = 0, remorts = 0, rows_assigned = 0, i = 0, pos = 0;

// пре-инициализация
	for (i = 0; i < 15; i++)
		for (tmp = 0; tmp < 14; tmp++)
			grouping[tmp][i] = -1;

	if (!(f = fopen(LIB_MISC "grouping", "r"))) {
		log("Невозможно открыть файл %s", LIB_MISC "grouping");
		return 1;
	}

	while (get_line(f, buf)) {
		if (!buf[0] || buf[0] == ';' || buf[0] == '\n')
			continue;
		i = 0;
		pos = 0;
		while (sscanf(&buf[pos], "%d", &tmp) == 1) {
			if (i >= 15) {
				i = 16;
				break;
			}
			while (buf[pos] == ' ' || buf[pos] == '\t')
				pos++;
			if (i == 0) {
				remorts = tmp;
				if (grouping[0][remorts] != -1) {
					log("Ошибка при чтении файла %s: дублирование параметров для %d ремортов",
						LIB_MISC "grouping", remorts);
					return 2;
				}
				if (remorts > 14 || remorts < 0) {
					log("Ошибка при чтении файла %s: неверное значение количества ремортов: %d, "
					     "должно быть в промежутке от 0 до 14",
						LIB_MISC "grouping", remorts);
					return 3;
				}
			} else
				grouping[i - 1][remorts] = tmp;
			i++;
			while (buf[pos] != ' ' && buf[pos] != '\t' && buf[pos] != 0)
				pos++;
		}
		if (i != 15) {
			log("Ошибка при чтении файла %s: неверный формат строки '%s', должно быть 15 "
			     "целых чисел, прочитали %d", LIB_MISC "grouping", buf, i);
			return 4;
		}
		rows_assigned++;
	}
	if (rows_assigned != 15) {
		log("Ошибка при чтении файла %s: необходимо 15 строк от 0 до 14 "
		    "ремортов, прочитано строк: %d.", LIB_MISC "grouping", rows_assigned);
		return 5;
	}
	fclose(f);
	return 0;
}

/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */

/* Function to return the exp required for each class/level */
int level_exp(CHAR_DATA * ch, int level)
{
	if (level > LVL_IMPL || level < 0) {
		log("SYSERR: Requesting exp for invalid level %d!", level);
		return 0;
	}

	/*
	 * Gods have exp close to EXP_MAX.  This statement should never have to
	 * changed, regardless of how many mortal or immortal levels exist.
	 */
	if (level > LVL_IMMORT) {
		return EXP_IMPL - ((LVL_IMPL - level) * 1000);
	}

	/* Exp required for normal mortals is below */
	float exp_modifier;
	if (GET_REMORT(ch) < MAX_EXP_COEFFICIENTS_USED)
		exp_modifier = exp_coefficients[GET_REMORT(ch)];
	else
		exp_modifier = exp_coefficients[MAX_EXP_COEFFICIENTS_USED];

	switch (GET_CLASS(ch)) {

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return int (exp_modifier * 2500);
		case 3:
			return int (exp_modifier * 5000);
		case 4:
			return int (exp_modifier * 9000);
		case 5:
			return int (exp_modifier * 17000);
		case 6:
			return int (exp_modifier * 27000);
		case 7:
			return int (exp_modifier * 47000);
		case 8:
			return int (exp_modifier * 77000);
		case 9:
			return int (exp_modifier * 127000);
		case 10:
			return int (exp_modifier * 197000);
		case 11:
			return int (exp_modifier * 297000);
		case 12:
			return int (exp_modifier * 427000);
		case 13:
			return int (exp_modifier * 587000);
		case 14:
			return int (exp_modifier * 817000);
		case 15:
			return int (exp_modifier * 1107000);
		case 16:
			return int (exp_modifier * 1447000);
		case 17:
			return int (exp_modifier * 1847000);
		case 18:
			return int (exp_modifier * 2310000);
		case 19:
			return int (exp_modifier * 2830000);
		case 20:
			return int (exp_modifier * 3580000);
		case 21:
			return int (exp_modifier * 4580000);
		case 22:
			return int (exp_modifier * 5830000);
		case 23:
			return int (exp_modifier * 7330000);
		case 24:
			return int (exp_modifier * 9080000);
		case 25:
			return int (exp_modifier * 11080000);
		case 26:
			return int (exp_modifier * 15000000);
		case 27:
			return int (exp_modifier * 22000000);
		case 28:
			return int (exp_modifier * 33000000);
		case 29:
			return int (exp_modifier * 47000000);
		case 30:
			return int (exp_modifier * 64000000);
			/* add new levels here */
		case LVL_IMMORT:
			return int (exp_modifier * 94000000);
		}
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return int (exp_modifier * 2500);
		case 3:
			return int (exp_modifier * 5000);
		case 4:
			return int (exp_modifier * 9000);
		case 5:
			return int (exp_modifier * 17000);
		case 6:
			return int (exp_modifier * 27000);
		case 7:
			return int (exp_modifier * 47000);
		case 8:
			return int (exp_modifier * 77000);
		case 9:
			return int (exp_modifier * 127000);
		case 10:
			return int (exp_modifier * 197000);
		case 11:
			return int (exp_modifier * 297000);
		case 12:
			return int (exp_modifier * 427000);
		case 13:
			return int (exp_modifier * 587000);
		case 14:
			return int (exp_modifier * 817000);
		case 15:
			return int (exp_modifier * 1107000);
		case 16:
			return int (exp_modifier * 1447000);
		case 17:
			return int (exp_modifier * 1847000);
		case 18:
			return int (exp_modifier * 2310000);
		case 19:
			return int (exp_modifier * 2830000);
		case 20:
			return int (exp_modifier * 3580000);
		case 21:
			return int (exp_modifier * 4580000);
		case 22:
			return int (exp_modifier * 5830000);
		case 23:
			return int (exp_modifier * 7330000);
		case 24:
			return int (exp_modifier * 9080000);
		case 25:
			return int (exp_modifier * 11080000);
		case 26:
			return int (exp_modifier * 15000000);
		case 27:
			return int (exp_modifier * 22000000);
		case 28:
			return int (exp_modifier * 33000000);
		case 29:
			return int (exp_modifier * 47000000);
		case 30:
			return int (exp_modifier * 64000000);
			/* add new levels here */
		case LVL_IMMORT:
			return int (exp_modifier * 87000000);
		}
		break;

	case CLASS_THIEF:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return int (exp_modifier * 750);
		case 3:
			return int (exp_modifier * 1500);
		case 4:
			return int (exp_modifier * 3000);
		case 5:
			return int (exp_modifier * 6000);
		case 6:
			return int (exp_modifier * 11000);
		case 7:
			return int (exp_modifier * 21000);
		case 8:
			return int (exp_modifier * 34000);
		case 9:
			return int (exp_modifier * 63000);
		case 10:
			return int (exp_modifier * 96000);
		case 11:
			return int (exp_modifier * 170000);
		case 12:
			return int (exp_modifier * 203000);
		case 13:
			return int (exp_modifier * 260000);
		case 14:
			return int (exp_modifier * 380000);
		case 15:
			return int (exp_modifier * 500000);
		case 16:
			return int (exp_modifier * 604000);
		case 17:
			return int (exp_modifier * 750000);
		case 18:
			return int (exp_modifier * 950000);
		case 19:
			return int (exp_modifier * 1200000);
		case 20:
			return int (exp_modifier * 1500000);
		case 21:
			return int (exp_modifier * 1900000);
		case 22:
			return int (exp_modifier * 2530000);
		case 23:
			return int (exp_modifier * 3250000);
		case 24:
			return int (exp_modifier * 4100000);
		case 25:
			return int (exp_modifier * 5000000);
		case 26:
			return int (exp_modifier * 6500000);
		case 27:
			return int (exp_modifier * 10000000);
		case 28:
			return int (exp_modifier * 15000000);
		case 29:
			return int (exp_modifier * 21500000);
		case 30:
			return int (exp_modifier * 30000000);
			/* add new levels here */
		case LVL_IMMORT:
			return int (exp_modifier * 50000000);
		}
		break;

	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return int (exp_modifier * 1500);
		case 3:
			return int (exp_modifier * 3000);
		case 4:
			return int (exp_modifier * 6000);
		case 5:
			return int (exp_modifier * 12000);
		case 6:
			return int (exp_modifier * 22000);
		case 7:
			return int (exp_modifier * 42000);
		case 8:
			return int (exp_modifier * 77000);
		case 9:
			return int (exp_modifier * 127000);
		case 10:
			return int (exp_modifier * 197000);
		case 11:
			return int (exp_modifier * 287000);
		case 12:
			return int (exp_modifier * 407000);
		case 13:
			return int (exp_modifier * 557000);
		case 14:
			return int (exp_modifier * 767000);
		case 15:
			return int (exp_modifier * 1007000);
		case 16:
			return int (exp_modifier * 1280000);
		case 17:
			return int (exp_modifier * 1570000);
		case 18:
			return int (exp_modifier * 1910000);
		case 19:
			return int (exp_modifier * 2300000);
		case 20:
			return int (exp_modifier * 2790000);
		case 21:
			return int (exp_modifier * 3780000);
		case 22:
			return int (exp_modifier * 5070000);
		case 23:
			return int (exp_modifier * 6560000);
		case 24:
			return int (exp_modifier * 8250000);
		case 25:
			return int (exp_modifier * 10240000);
		case 26:
			return int (exp_modifier * 13000000);
		case 27:
			return int (exp_modifier * 20000000);
		case 28:
			return int (exp_modifier * 30000000);
		case 29:
			return int (exp_modifier * 43000000);
		case 30:
			return int (exp_modifier * 59000000);
			/* add new levels here */
		case LVL_IMMORT:
			return int (exp_modifier * 79000000);
		}
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return int (exp_modifier * 2000);
		case 3:
			return int (exp_modifier * 4000);
		case 4:
			return int (exp_modifier * 8000);
		case 5:
			return int (exp_modifier * 14000);
		case 6:
			return int (exp_modifier * 24000);
		case 7:
			return int (exp_modifier * 39000);
		case 8:
			return int (exp_modifier * 69000);
		case 9:
			return int (exp_modifier * 119000);
		case 10:
			return int (exp_modifier * 189000);
		case 11:
			return int (exp_modifier * 289000);
		case 12:
			return int (exp_modifier * 419000);
		case 13:
			return int (exp_modifier * 579000);
		case 14:
			return int (exp_modifier * 800000);
		case 15:
			return int (exp_modifier * 1070000);
		case 16:
			return int (exp_modifier * 1340000);
		case 17:
			return int (exp_modifier * 1660000);
		case 18:
			return int (exp_modifier * 2030000);
		case 19:
			return int (exp_modifier * 2450000);
		case 20:
			return int (exp_modifier * 2950000);
		case 21:
			return int (exp_modifier * 3950000);
		case 22:
			return int (exp_modifier * 5250000);
		case 23:
			return int (exp_modifier * 6750000);
		case 24:
			return int (exp_modifier * 8450000);
		case 25:
			return int (exp_modifier * 10350000);
		case 26:
			return int (exp_modifier * 14000000);
		case 27:
			return int (exp_modifier * 21000000);
		case 28:
			return int (exp_modifier * 31000000);
		case 29:
			return int (exp_modifier * 44000000);
		case 30:
			return int (exp_modifier * 64000000);
			/* add new levels here */
		case LVL_IMMORT:
			return int (exp_modifier * 79000000);
		}
		break;
	}

	/*
	 * This statement should never be reached if the exp tables in this function
	 * are set up properly.  If you see exp of 123456 then the tables above are
	 * incomplete -- so, complete them!
	 */
	log("SYSERR: XP tables not set up correctly in class.c!");
	return 123456;
}


/*
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
	if (level < 0 || level > LVL_IMPL)
		return "усталый путник";
	if (level == LVL_IMPL)
		return "дитя Творца";

	return ("\0");

	switch (chclass) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level) {
		case 1:
			return "the Apprentice of Magic";
		case 2:
			return "the Spell Student";
		case 3:
			return "the Scholar of Magic";
		case 4:
			return "the Delver in Spells";
		case 5:
			return "the Medium of Magic";
		case 6:
			return "the Scribe of Magic";
		case 7:
			return "the Seer";
		case 8:
			return "the Sage";
		case 9:
			return "the Illusionist";
		case 10:
			return "the Abjurer";
		case 11:
			return "the Invoker";
		case 12:
			return "the Enchanter";
		case 13:
			return "the Conjurer";
		case 14:
			return "the Magician";
		case 15:
			return "the Creator";
		case 16:
			return "the Savant";
		case 17:
			return "the Magus";
		case 18:
			return "the Wizard";
		case 19:
			return "the Warlock";
		case 20:
			return "the Sorcerer";
		case 21:
			return "the Necromancer";
		case 22:
			return "the Thaumaturge";
		case 23:
			return "the Student of the Occult";
		case 24:
			return "the Disciple of the Uncanny";
		case 25:
			return "the Minor Elemental";
		case 26:
			return "the Greater Elemental";
		case 27:
			return "the Crafter of Magics";
		case 28:
			return "the Shaman";
		case 29:
			return "the Keeper of Talismans";
		case 30:
			return "the Archmage";
		case LVL_IMMORT:
			return "the Immortal Warlock";
		case LVL_GOD:
			return "the Avatar of Magic";
		case LVL_GRGOD:
			return "the God of Magic";
		default:
			return "the Mage";
		}
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level) {
		case 1:
			return "the Believer";
		case 2:
			return "the Attendant";
		case 3:
			return "the Acolyte";
		case 4:
			return "the Novice";
		case 5:
			return "the Missionary";
		case 6:
			return "the Adept";
		case 7:
			return "the Deacon";
		case 8:
			return "the Vicar";
		case 9:
			return "the Priest";
		case 10:
			return "the Minister";
		case 11:
			return "the Canon";
		case 12:
			return "the Levite";
		case 13:
			return "the Curate";
		case 14:
			return "the Monk";
		case 15:
			return "the Healer";
		case 16:
			return "the Chaplain";
		case 17:
			return "the Expositor";
		case 18:
			return "the Bishop";
		case 19:
			return "the Arch Bishop";
		case 20:
			return "the Patriarch";
			/* no one ever thought up these titles 21-30 */
		case LVL_IMMORT:
			return "the Immortal Cardinal";
		case LVL_GOD:
			return "the Inquisitor";
		case LVL_GRGOD:
			return "the God of good and evil";
		default:
			return "the Cleric";
		}
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		switch (level) {
		case 1:
			return "the Pilferer";
		case 2:
			return "the Footpad";
		case 3:
			return "the Filcher";
		case 4:
			return "the Pick-Pocket";
		case 5:
			return "the Sneak";
		case 6:
			return "the Pincher";
		case 7:
			return "the Cut-Purse";
		case 8:
			return "the Snatcher";
		case 9:
			return "the Sharper";
		case 10:
			return "the Rogue";
		case 11:
			return "the Robber";
		case 12:
			return "the Magsman";
		case 13:
			return "the Highwayman";
		case 14:
			return "the Burglar";
		case 15:
			return "the Thief";
		case 16:
			return "the Knifer";
		case 17:
			return "the Quick-Blade";
		case 18:
			return "the Killer";
		case 19:
			return "the Brigand";
		case 20:
			return "the Cut-Throat";
			/* no one ever thought up these titles 21-30 */
		case LVL_IMMORT:
			return "the Immortal Assasin";
		case LVL_GOD:
			return "the Demi God of thieves";
		case LVL_GRGOD:
			return "the God of thieves and tradesmen";
		default:
			return "the Thief";
		}
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level) {
		case 1:
			return "the Swordpupil";
		case 2:
			return "the Recruit";
		case 3:
			return "the Sentry";
		case 4:
			return "the Fighter";
		case 5:
			return "the Soldier";
		case 6:
			return "the Warrior";
		case 7:
			return "the Veteran";
		case 8:
			return "the Swordsman";
		case 9:
			return "the Fencer";
		case 10:
			return "the Combatant";
		case 11:
			return "the Hero";
		case 12:
			return "the Myrmidon";
		case 13:
			return "the Swashbuckler";
		case 14:
			return "the Mercenary";
		case 15:
			return "the Swordmaster";
		case 16:
			return "the Lieutenant";
		case 17:
			return "the Champion";
		case 18:
			return "the Dragoon";
		case 19:
			return "the Cavalier";
		case 20:
			return "the Knight";
			/* no one ever thought up these titles 21-30 */
		case LVL_IMMORT:
			return "the Immortal Warlord";
		case LVL_GOD:
			return "the Extirpator";
		case LVL_GRGOD:
			return "the God of war";
		default:
			return "the Warrior";
		}
		break;
	}

	/* Default title for classes which do not have titles defined */
	return "the Classless";
}


/*
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
	if (level < 0 || level > LVL_IMPL)
		return "усталая путница";
	if (level == LVL_IMPL)
		return "дочь Солнца";

	return ("\0");
	switch (chclass) {
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level) {
		case 1:
			return "the Apprentice of Magic";
		case 2:
			return "the Spell Student";
		case 3:
			return "the Scholar of Magic";
		case 4:
			return "the Delveress in Spells";
		case 5:
			return "the Medium of Magic";
		case 6:
			return "the Scribess of Magic";
		case 7:
			return "the Seeress";
		case 8:
			return "the Sage";
		case 9:
			return "the Illusionist";
		case 10:
			return "the Abjuress";
		case 11:
			return "the Invoker";
		case 12:
			return "the Enchantress";
		case 13:
			return "the Conjuress";
		case 14:
			return "the Witch";
		case 15:
			return "the Creator";
		case 16:
			return "the Savant";
		case 17:
			return "the Craftess";
		case 18:
			return "the Wizard";
		case 19:
			return "the War Witch";
		case 20:
			return "the Sorceress";
		case 21:
			return "the Necromancress";
		case 22:
			return "the Thaumaturgess";
		case 23:
			return "the Student of the Occult";
		case 24:
			return "the Disciple of the Uncanny";
		case 25:
			return "the Minor Elementress";
		case 26:
			return "the Greater Elementress";
		case 27:
			return "the Crafter of Magics";
		case 28:
			return "Shaman";
		case 29:
			return "the Keeper of Talismans";
		case 30:
			return "Archwitch";
		case LVL_IMMORT:
			return "the Immortal Enchantress";
		case LVL_GOD:
			return "the Empress of Magic";
		case LVL_GRGOD:
			return "the Goddess of Magic";
		default:
			return "the Witch";
		}
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level) {
		case 1:
			return "the Believer";
		case 2:
			return "the Attendant";
		case 3:
			return "the Acolyte";
		case 4:
			return "the Novice";
		case 5:
			return "the Missionary";
		case 6:
			return "the Adept";
		case 7:
			return "the Deaconess";
		case 8:
			return "the Vicaress";
		case 9:
			return "the Priestess";
		case 10:
			return "the Lady Minister";
		case 11:
			return "the Canon";
		case 12:
			return "the Levitess";
		case 13:
			return "the Curess";
		case 14:
			return "the Nunne";
		case 15:
			return "the Healess";
		case 16:
			return "the Chaplain";
		case 17:
			return "the Expositress";
		case 18:
			return "the Bishop";
		case 19:
			return "the Arch Lady of the Church";
		case 20:
			return "the Matriarch";
			/* no one ever thought up these titles 21-30 */
		case LVL_IMMORT:
			return "the Immortal Priestess";
		case LVL_GOD:
			return "the Inquisitress";
		case LVL_GRGOD:
			return "the Goddess of good and evil";
		default:
			return "the Cleric";
		}
		break;

	case CLASS_THIEF:
	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		switch (level) {
		case 1:
			return "the Pilferess";
		case 2:
			return "the Footpad";
		case 3:
			return "the Filcheress";
		case 4:
			return "the Pick-Pocket";
		case 5:
			return "the Sneak";
		case 6:
			return "the Pincheress";
		case 7:
			return "the Cut-Purse";
		case 8:
			return "the Snatcheress";
		case 9:
			return "the Sharpress";
		case 10:
			return "the Rogue";
		case 11:
			return "the Robber";
		case 12:
			return "the Magswoman";
		case 13:
			return "the Highwaywoman";
		case 14:
			return "the Burglaress";
		case 15:
			return "the Thief";
		case 16:
			return "the Knifer";
		case 17:
			return "the Quick-Blade";
		case 18:
			return "the Murderess";
		case 19:
			return "the Brigand";
		case 20:
			return "the Cut-Throat";
		case 34:
			return "the Implementress";
			/* no one ever thought up these titles 21-30 */
		case LVL_IMMORT:
			return "the Immortal Assasin";
		case LVL_GOD:
			return "the Demi Goddess of thieves";
		case LVL_GRGOD:
			return "the Goddess of thieves and tradesmen";
		default:
			return "the Thief";
		}
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level) {
		case 1:
			return "the Swordpupil";
		case 2:
			return "the Recruit";
		case 3:
			return "the Sentress";
		case 4:
			return "the Fighter";
		case 5:
			return "the Soldier";
		case 6:
			return "the Warrior";
		case 7:
			return "the Veteran";
		case 8:
			return "the Swordswoman";
		case 9:
			return "the Fenceress";
		case 10:
			return "the Combatess";
		case 11:
			return "the Heroine";
		case 12:
			return "the Myrmidon";
		case 13:
			return "the Swashbuckleress";
		case 14:
			return "the Mercenaress";
		case 15:
			return "the Swordmistress";
		case 16:
			return "the Lieutenant";
		case 17:
			return "the Lady Champion";
		case 18:
			return "the Lady Dragoon";
		case 19:
			return "the Cavalier";
		case 20:
			return "the Lady Knight";
			/* no one ever thought up these titles 21-30 */
		case LVL_IMMORT:
			return "the Immortal Lady of War";
		case LVL_GOD:
			return "the Queen of Destruction";
		case LVL_GRGOD:
			return "the Goddess of war";
		default:
			return "the Warrior";
		}
		break;
	}

	/* Default title for classes which do not have titles defined */
	return "the Classless";
}
