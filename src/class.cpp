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
#include "class.hpp"

#include "world.objects.hpp"
#include "obj.hpp"
#include "comm.h"
#include "db.h"
#include "spell_parser.hpp"
#include "spells.h"
#include "skills.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"
#include "pk.h"
#include "top.h"
#include "features.hpp"
#include "im.h"
#include "char.hpp"
#include "spam.hpp"
#include "screen.h"
#include "char_player.hpp"
#include "named_stuff.hpp"
#include "player_races.hpp"
#include "noob.hpp"
#include "exchange.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <iostream>

extern int siteok_everyone;
extern struct spell_create_type spell_create[];
extern double exp_coefficients[];

struct skillvariables_dig dig_vars;
struct skillvariables_insgem insgem_vars;

// local functions
int parse_class(char arg);
long find_class_bitvector(char arg);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
void do_start(CHAR_DATA * ch, int newbie);
int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
int level_exp(CHAR_DATA * ch, int level);
byte extend_saving_throws(int class_num, int type, int level);
int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
extern bool char_to_pk_clan(CHAR_DATA *ch);
// Names first

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


// The menu for choosing a class in interpreter.c:
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
	"  [Ч]ернокнижник\r\n"
	"  В[и]тязь\r\n"
	"  [О]хотник\r\n"
	"  Ку[з]нец\r\n"
	"  Ку[п]ец\r\n"
	"  Вол[x]в\r\n";

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

// The menu for choosing a religion in interpreter.c:
const char *religion_menu =
	"\r\n" "Какой религии вы отдаете предпочтение :\r\n" "  Я[з]ычество\r\n" "  [Х]ристианство\r\n";

#define RELIGION_ANY 100


/* Соответствие классов и религий. RELIGION_POLY-класс не может быть христианином
                                   RELIGION_MONO-класс не может быть язычником  (Кард)
				   RELIGION_ANY - класс может быть кем угодно */
const int class_religion[] = { RELIGION_ANY,		//Лекарь
							   RELIGION_ANY,		//Колдун
							   RELIGION_ANY,		//Тать
							   RELIGION_ANY,		//Богатырь
							   RELIGION_ANY,		//Наемник
							   RELIGION_ANY,		//Дружинник
							   RELIGION_ANY,		//Кудесник
							   RELIGION_ANY,		//Волшебник
							   RELIGION_ANY,		//Чернокнижник
							   RELIGION_ANY,		//Витязь
							   RELIGION_ANY,		//Охотник
							   RELIGION_ANY,		//Кузнец
							   RELIGION_ANY,		//Купец
							   RELIGION_POLY		//Волхв
							 };



//str dex con wis int cha
int class_stats_limit[NUM_PLAYER_CLASSES][6];

/*
 * The code to interpret a class letter -- used in interpreter.cpp when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
	arg = LOWER(arg);

	switch (arg)
	{
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
parse_class_vik(char arg)
{
	arg = LOWER(arg);

	switch (arg)
	{
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
parse_class_step(char arg)
{
	arg = LOWER(arg);

	switch (arg)
	{
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

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
	arg = LOWER(arg);

	switch (arg)
	{
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
find_class_bitvector_vik(char arg)
{
	arg = LOWER(arg);

	switch (arg)
	{
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
find_class_bitvector_step(char arg)
{
	arg = LOWER(arg);

	switch (arg)
	{
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

// #define LEARNED_LEVEL	0  % known which is considered "learned"
// #define MAX_PER_PRAC		1  max percent gain in skill per practice
// #define MIN_PER_PRAC		2  min percent gain in skill per practice
// #define PRAC_TYPE		3  should it say 'spell' or 'skill'?

int prac_params[4][NUM_PLAYER_CLASSES] =  	// MAG        CLE             THE             WAR
{
	{95, 95, 85, 80},	// learned level
	{100, 100, 12, 12},	// max per prac
	{25, 25, 0, 0, },	// min per pac
	{SPELL, SPELL, SKILL, SKILL}	// prac name
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
int guild_info[][3] =
{

	// Midgaard
	{CLASS_BATTLEMAGE, 3017, SCMD_SOUTH},
	{CLASS_CLERIC, 3004, SCMD_NORTH},
	{CLASS_THIEF, 3027, SCMD_EAST},
	{CLASS_WARRIOR, 3021, SCMD_EAST},

	// Brass Dragon
	{ -999 /* all */ , 5065, SCMD_WEST},

	// this must go last -- add new guards above!
	{ -1, -1, -1}
};


// Таблицы бызовых спасбросков

const byte sav_01[50] =
{
	90, 90, 90, 90, 90, 89, 89, 88, 88, 87,	// 00-09
	86, 85, 84, 83, 81, 79, 78, 75, 73, 71,	// 10-19
	68, 65, 62, 59, 56, 52, 48, 44, 40, 35,	// 20-29
	30, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_02[50] =
{
	90, 90, 90, 90, 90, 89, 89, 88, 87, 87,	// 00-09
	86, 84, 83, 81, 80, 78, 75, 73, 70, 68,	// 10-19
	65, 61, 58, 54, 50, 46, 41, 36, 31, 26,	// 20-29
	20, 15, 10, 9, 8, 7, 6, 5, 4, 3,	// 30-39
	2, 1, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_03[50] =
{
	90, 90, 90, 90, 90, 89, 89, 88, 88, 87,	// 00-09
	86, 85, 83, 82, 80, 79, 76, 74, 72, 69,	// 10-19
	66, 63, 60, 57, 53, 49, 45, 40, 35, 30,	// 20-29
	25, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_04[50] =
{
	90, 90, 90, 90, 90, 90, 89, 89, 89, 88,	// 00-09
	87, 87, 86, 85, 84, 83, 82, 80, 79, 77,	// 10-19
	75, 74, 72, 69, 67, 65, 62, 59, 56, 53,	// 20-29
	50, 45, 40, 35, 30, 25, 20, 15, 10, 5,	// 30-39
	4, 3, 2, 1, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_05[50] =
{
	90, 90, 90, 90, 90, 89, 89, 89, 88, 87,	// 00-09
	86, 86, 84, 83, 82, 80, 79, 77, 75, 72,	// 10-19
	70, 67, 65, 62, 59, 55, 52, 48, 44, 39,	// 20-29
	35, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
//CLASS_MOB
const byte sav_06[100] =
{
	90, 90, 90, 90, 90, 90, 89, 89, 88, 88,	// 00-09
	87, 86, 85, 84, 82, 80, 78, 76, 74, 72,	// 10-19
	70, 67, 64, 61, 58, 55, 52, 49, 46, 43,	// 20-29
	41, 39, 38, 37, 33, 25, 20, 15, 10, 5,	// 30-39
	4, 3, 2, 1, 0, 0, 0, 0, 0, 0,			// 40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 50-59
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 60-69
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 70-79
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 80-89
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0			// 90-99
};
const byte sav_08[50] =
{
	90, 90, 90, 90, 90, 89, 89, 89, 88, 88,	// 00-09
	87, 86, 85, 84, 83, 81, 80, 78, 76, 74,	// 10-19
	72, 70, 67, 64, 61, 58, 55, 52, 48, 44,	// 20-29
	40, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_09[50] =
{
	90, 75, 73, 71, 69, 67, 65, 63, 61, 60,	// 00-09
	59, 57, 55, 53, 51, 50, 49, 47, 45, 43,	// 10-19
	41, 40, 39, 37, 35, 33, 31, 29, 27, 25,	// 20-29
	23, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_10[50] =
{
	90, 80, 79, 78, 76, 75, 73, 70, 67, 65,	// 00-09
	64, 63, 61, 60, 59, 57, 56, 55, 54, 53,	// 10-19
	52, 51, 50, 48, 45, 44, 42, 40, 39, 38,	// 20-29
	37, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_11[50] =
{
	90, 80, 79, 78, 77, 76, 75, 74, 73, 72,	// 00-09
	71, 70, 69, 68, 67, 66, 65, 64, 63, 62,	// 10-19
	61, 60, 59, 58, 57, 56, 55, 54, 53, 52,	// 20-29
	51, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
const byte sav_12[50] =
{
	90, 85, 83, 82, 80, 75, 70, 65, 63, 62,	// 00-09
	60, 55, 50, 45, 43, 42, 40, 37, 33, 30,	// 10-19
	29, 28, 26, 25, 24, 23, 21, 20, 19, 18,	// 20-29
	17, 16, 15, 14, 13, 12, 11, 10, 9, 8,	// 30-39
	7, 6, 5, 4, 3, 2, 1, 0, 0, 0	// 40-49
};
//CLASS_MOB
const byte sav_13[100] =
{
	90, 83, 81, 79, 77, 75, 72, 68, 65, 63,	// 00-09
	61, 58, 56, 53, 50, 47, 45, 43, 42, 41,	// 10-19
	40, 38, 37, 36, 34, 33, 32, 31, 30, 29,	// 20-29
	27, 26, 25, 24, 23, 22, 21, 20, 18, 16,	// 30-39
	12, 10, 8, 6, 4, 2, 1, 0, 0, 0,			// 40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 50-59
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 60-69
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 70-79
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 80-89
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0			// 90-99
};
const byte sav_14[50] =
{
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,	// 00-09
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,	// 10-19
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100,	// 20-29
	100, 70, 70, 70, 70, 70, 70, 70, 70, 70,	// 30-39
	70, 70, 70, 70, 70, 70, 70, 70, 70, 70	// 40-49
};
const byte sav_15[50] =
{
	100, 99, 98, 97, 96, 95, 94, 93, 92, 91,	// 00-09
	90, 89, 88, 87, 86, 85, 84, 83, 82, 81,	// 10-19
	80, 79, 78, 77, 76, 75, 74, 73, 72, 71,	// 20-29
	70, 50, 50, 50, 50, 50, 50, 50, 50, 50,	// 30-39
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50	// 40-49
};
const byte sav_16[50] =
{
	100, 99, 97, 96, 95, 94, 92, 91, 89, 88,	// 00-09
	86, 85, 84, 83, 81, 80, 79, 77, 76, 75,	// 10-19
	74, 72, 71, 70, 68, 65, 64, 63, 62, 61,	// 20-29
	60, 58, 57, 56, 54, 52, 51, 49, 47, 46,	// 30-39
	45, 43, 42, 41, 39, 37, 35, 34, 32, 31	// 40-49
};
const byte sav_17[50] =
{
	100, 99, 97, 96, 95, 94, 92, 91, 89, 88,	// 00-09
	86, 84, 82, 80, 78, 76, 74, 72, 70, 68,	// 10-19
	64, 62, 60, 58, 56, 54, 52, 50, 49, 47,	// 20-29
	45, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 30-39
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 40-49
};
//CLASS_MOB
const byte sav_18[100] =
{
	100, 100, 100, 100, 100, 99, 99, 99, 99, 99,	// 00-09
	98, 98, 98, 98, 98, 97, 97, 97, 97, 97,			// 10-19
	96, 96, 96, 96, 96, 95, 95, 95, 95, 95,			// 20-29
	94, 94, 94, 94, 94, 93, 93, 93, 93, 93,			// 30-39
	92, 92, 92, 92, 92, 91, 91, 91, 91, 91,			// 40-49
	91, 91, 91, 91, 91, 90, 90, 90, 90, 90,			// 50-59
	89, 89, 89, 89, 89, 88, 88, 88, 88, 88,			// 60-69
	87, 87, 87, 87, 87, 86, 86, 86, 86, 86,			// 70-79
	85, 85, 85, 85, 85, 84, 84, 84, 84, 84,			// 80-89
	83, 83, 83, 83, 83, 82, 82, 82, 82, 82			// 90-99
};


// {CLASS,{PARA,ROD,AFFECT,BREATH,SPELL,BASIC}}
struct std_saving_type
{
	int chclass;
	const byte *saves[SAVING_COUNT];
};

std_saving_type std_saving[] =
{
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
		-1, {
			sav_02, sav_12, sav_02, sav_16}}
};

byte saving_throws(int class_num, int type, int level)
{
	return extend_saving_throws(class_num, type, level);
}

byte extend_saving_throws(int class_num, int type, int level)
{
	int i;
	if (type < 0 || type >= SAVING_COUNT)
		return 100;
	if (level <= 0 || level > 100)
		return 100;
	--level;

	for (i = 0; std_saving[i].chclass != -1 && std_saving[i].chclass != class_num; ++i);

	return std_saving[i].saves[type][level];
}


// THAC0 for classes and levels.  (To Hit Armor Class 0)
int thaco(int class_num, int level)
{
	switch (class_num)
	{
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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

	// Will not get there unless something is wrong.
	return 100;
}

// AC0 for classes and levels.
int extra_aco(int class_num, int level)
{
	switch (class_num)
	{
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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


// DAMROLL for classes and levels.
int extra_damroll(int class_num, int level)
{
	switch (class_num)
	{
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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
		switch (level)
		{
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

// Some initializations for characters, including initial skills
void init_warcry(CHAR_DATA *ch) // проставление кличей в обход античита
{
	if (GET_CLASS(ch) == CLASS_GUARD)
		SET_BIT(GET_SPELL_TYPE(ch, SPELL_WC_OF_DEFENSE), SPELL_KNOW); // клич призыв к обороне
	if (GET_CLASS(ch) == CLASS_WARRIOR)
	{
		SET_BIT(GET_SPELL_TYPE(ch, SPELL_WC_OF_BATTLE), SPELL_KNOW); // клич призыв битвы
		SET_BIT(GET_SPELL_TYPE(ch, SPELL_WC_OF_POWER), SPELL_KNOW); // клич призыв мощи
		SET_BIT(GET_SPELL_TYPE(ch, SPELL_WC_OF_BLESS), SPELL_KNOW); // клич призывы доблести
		SET_BIT(GET_SPELL_TYPE(ch, SPELL_WC_OF_COURAGE), SPELL_KNOW); // клич призыв отваги
	}

}

void do_start(CHAR_DATA * ch, int newbie)
{
	ch->set_level(1);
	ch->set_exp(1);
	ch->points.max_hit = 10;
	if (newbie || (ch->get_remort() >= 9 && ch->get_remort() % 3 == 0))
	{
		ch->set_skill(SKILL_DRUNKOFF, 10);
	}

	if (newbie && GET_CLASS(ch) == CLASS_DRUID)
	{
		for (int i = 1; i <= MAX_SPELLS; i++)
		{
			GET_SPELL_TYPE(ch, i) = SPELL_RUNES;
		}
	}

	if (newbie)
	{
		std::vector<int> outfit_list(Noob::get_start_outfit(ch));
		for (auto i = outfit_list.begin(); i != outfit_list.end(); ++i)
		{
			const OBJ_DATA::shared_ptr obj = world_objects.create_from_prototype_by_vnum(*i);
			if (obj)
			{
				obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);
				obj->set_extra_flag(EExtraFlag::ITEM_DECAY);
				obj->set_cost(0);
				obj->set_rent_off(0);
				obj->set_rent_on(0);
				obj_to_char(obj.get(), ch);
				Noob::equip_start_outfit(ch, obj.get());
			}
		}
	}

	if (newbie || (ch->get_remort() >= 9 && ch->get_remort() % 3 == 0))
	{
		switch (GET_CLASS(ch))
		{
		case CLASS_BATTLEMAGE:
		case CLASS_DEFENDERMAGE:
		case CLASS_CHARMMAGE:
		case CLASS_NECROMANCER:
		case CLASS_DRUID:
			ch->set_skill(SKILL_SATTACK, 10);
			break;
		case CLASS_CLERIC:
			ch->set_skill(SKILL_SATTACK, 50);
			break;
		case CLASS_THIEF:
		case CLASS_ASSASINE:
		case CLASS_MERCHANT:
			ch->set_skill(SKILL_SATTACK, 75);
			break;
		case CLASS_GUARD:
		case CLASS_PALADINE:
		case CLASS_WARRIOR:
		case CLASS_RANGER:
			ch->set_skill(SKILL_HORSE, 10);
		case CLASS_SMITH:
			ch->set_skill(SKILL_SATTACK, 95);
			break;
		}
	}

	advance_level(ch);
	sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
	mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);

	GET_HIT(ch) = GET_REAL_MAX_HIT(ch);
	GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);

	GET_COND(ch, THIRST) = 0;
	GET_COND(ch, FULL) = 0;
	GET_COND(ch, DRUNK) = 0;
	// проставим кличи
	init_warcry(ch);
	if (siteok_everyone)
	{
		PLR_FLAGS(ch).set(PLR_SITEOK);
	}
}

// * Перерасчет максимальных родных хп персонажа.
// * При входе в игру, левеле/делевеле, добавлении/удалении славы.
void check_max_hp(CHAR_DATA *ch)
{
	GET_MAX_HIT(ch) = PlayerSystem::con_natural_hp(ch);
}

// * Обработка событий при левел-апе.
void levelup_events(CHAR_DATA *ch)
{
	if (SpamSystem::MIN_OFFTOP_LVL == GET_LEVEL(ch)
		&& !ch->get_disposable_flag(DIS_OFFTOP_MESSAGE))
	{
		PRF_FLAGS(ch).set(PRF_OFFTOP_MODE);
		ch->set_disposable_flag(DIS_OFFTOP_MESSAGE);
		send_to_char(ch,
			"%sТеперь вы можете пользоваться каналом оффтоп ('справка оффтоп').%s\r\n",
			CCIGRN(ch, C_SPR), CCNRM(ch, C_SPR));
	}
	if (EXCHANGE_MIN_CHAR_LEV == GET_LEVEL(ch)
		&& !ch->get_disposable_flag(DIS_EXCHANGE_MESSAGE))
	{
		// по умолчанию базар у всех включен, поэтому не спамим даже однократно
		if (GET_REMORT(ch) <= 0)
		{
			send_to_char(ch,
				"%sТеперь вы можете покупать и продавать вещи на базаре ('справка базар!').%s\r\n",
				CCIGRN(ch, C_SPR), CCNRM(ch, C_SPR));
		}
		ch->set_disposable_flag(DIS_EXCHANGE_MESSAGE);
	}
}

void advance_level(CHAR_DATA * ch)
{
	int add_move = 0, i;

	switch (GET_CLASS(ch))
	{
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
		add_move = number(ch->get_inborn_dex() / 6 + 1, ch->get_inborn_dex() / 5 + 1);
		break;

	case CLASS_WARRIOR:
		add_move = number(ch->get_inborn_dex() / 6 + 1, ch->get_inborn_dex() / 5 + 1);
		break;

	case CLASS_GUARD:
	case CLASS_RANGER:
	case CLASS_PALADINE:
	case CLASS_SMITH:
		add_move = number(ch->get_inborn_dex() / 6 + 1, ch->get_inborn_dex() / 5 + 1);
		break;
	}

	check_max_hp(ch);
	ch->points.max_move += MAX(1, add_move);

	// Set natural & race features
	set_natural_feats(ch);

	if (IS_IMMORTAL(ch))
	{
		for (i = 0; i < 3; i++)
			GET_COND(ch, i) = (char) - 1;
		PRF_FLAGS(ch).set(PRF_HOLYLIGHT);
	}

	TopPlayer::Refresh(ch);
	levelup_events(ch);
	ch->save_char();
}

void check_max_skills(CHAR_DATA *ch)
{
	for (const auto i : AVAILABLE_SKILLS)
	{
		if (ch->get_inborn_skill(i)  && i != SKILL_SATTACK)
		{
			int max = wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * (GET_LEVEL(ch) + 1) / 20;
			if (max > MAX_EXP_PERCENT)
				max = MAX_EXP_PERCENT;
			int sval = ch->get_inborn_skill(i) - max - GET_REMORT(ch) * 5;
			if (sval < 0)
				sval = 0;
			if ((ch->get_inborn_skill(i) - sval) > (wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * GET_LEVEL(ch) / 20))
			{
				ch->set_skill(i, ((wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * GET_LEVEL(ch) / 20) + sval));
			}
		}
	}
}

void decrease_level(CHAR_DATA * ch)
{
	int add_move = 0;

	switch (GET_CLASS(ch))
	{
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
		add_move = ch->get_inborn_dex() / 5 + 1;
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		add_move = ch->get_inborn_dex() / 5 + 1;
		break;
	}

	check_max_hp(ch);
	ch->points.max_move -= MIN(ch->points.max_move, MAX(1, add_move));

	GET_WIMP_LEV(ch) = MAX(0, MIN(GET_WIMP_LEV(ch), GET_REAL_MAX_HIT(ch) / 2));
	if (!IS_IMMORTAL(ch))
	{
		PRF_FLAGS(ch).unset(PRF_HOLYLIGHT);
	}

	//check_max_skills(ch);
	TopPlayer::Refresh(ch);
	ch->save_char();
}

/*
 * invalid_class is used by handler.cpp to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj)
{
	OBJ_DATA *object;
	if (!IS_CORPSE(obj))
	{
		for (object = obj->get_contains(); object; object = object->get_next_content())
		{
			if (invalid_unique(ch, object))
			{
				return (TRUE);
			}
		}
	}
	if (!ch
		|| !obj
		|| (IS_NPC(ch)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		|| IS_IMMORTAL(ch)
		|| obj->get_owner() == 0
		|| obj->get_owner() == GET_UNIQUE(ch))
	{
		return (FALSE);
	}
	return (TRUE);
}


bool unique_stuff(const CHAR_DATA *ch, const OBJ_DATA *obj)
{
	for (unsigned int i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i) && (GET_OBJ_VNUM(GET_EQ(ch, i)) == GET_OBJ_VNUM(obj)))
		{
			return true;
		}
	return false;
}


int invalid_anti_class(CHAR_DATA *ch, const OBJ_DATA* obj)
{
	if (!IS_CORPSE(obj))
	{
		for (const OBJ_DATA* object = obj->get_contains(); object; object = object->get_next_content())
		{
			if (invalid_anti_class(ch, object) || NamedStuff::check_named(ch, object, 0))
			{
				return (TRUE);
			}
		}
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE)
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return (TRUE);
	}
	if ((IS_NPC(ch) || WAITLESS(ch)) && !IS_CHARMICE(ch))
	{
		return (FALSE);
	}
	if ((IS_OBJ_ANTI(obj, EAntiFlag::ITEM_NOT_FOR_NOPK) && char_to_pk_clan(ch)))
	{		
		return (TRUE);
	}

	if ((IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MONO) && GET_RELIGION(ch) == RELIGION_MONO)
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLY) && GET_RELIGION(ch) == RELIGION_POLY)
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MAGIC_USER) && IS_MAGIC_USER(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_BATTLEMAGE) && IS_BATTLEMAGE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMMAGE) && IS_CHARMMAGE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_DEFENDERMAGE) && IS_DEFENDERMAGE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_NECROMANCER) && IS_NECROMANCER(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MALE) && IS_MALE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_FEMALE) && IS_FEMALE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CLERIC) && IS_CLERIC(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_WARRIOR) && IS_WARRIOR(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_GUARD) && IS_GUARD(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_THIEF) && IS_THIEF(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_ASSASINE) && IS_ASSASINE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_PALADINE) && IS_PALADINE(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_RANGER) && IS_RANGER(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_SMITH) && IS_SMITH(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MERCHANT) && IS_MERCHANT(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_DRUID) && IS_DRUID(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_KILLER) && PLR_FLAGGED(ch, PLR_KILLER))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_BD) && check_agrobd(ch))
		|| (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_COLORED) && IS_COLORED(ch)))
	{
		return (TRUE);
	}
	return (FALSE);
}

int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj)
{
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE)
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return TRUE;
	}

	if (!IS_CHARMICE(ch)
		&& (IS_NPC(ch)
			|| WAITLESS(ch)))
	{
		return FALSE;
	}

	if ((IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MONO) && GET_RELIGION(ch) == RELIGION_MONO)
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_POLY) && GET_RELIGION(ch) == RELIGION_POLY)
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MAGIC_USER) && IS_MAGIC_USER(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_BATTLEMAGE) && IS_BATTLEMAGE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMMAGE) && IS_CHARMMAGE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_DEFENDERMAGE) && IS_DEFENDERMAGE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_NECROMANCER) && IS_NECROMANCER(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MALE) && IS_MALE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_FEMALE) && IS_FEMALE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CLERIC) && IS_CLERIC(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_WARRIOR) && IS_WARRIOR(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_GUARD) && IS_GUARD(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_THIEF) && IS_THIEF(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_ASSASINE) && IS_ASSASINE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_PALADINE) && IS_PALADINE(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_RANGER) && IS_RANGER(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_SMITH) && IS_SMITH(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MERCHANT) && IS_MERCHANT(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_DRUID) && IS_DRUID(ch))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_KILLER) && PLR_FLAGGED(ch, PLR_KILLER))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_BD) && check_agrobd(ch))
		|| (!IS_SMITH(ch) && (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SHARPEN) || OBJ_FLAGGED(obj, EExtraFlag::ITEM_ARMORED)))
		|| (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_COLORED) && IS_COLORED(ch)))
	{
		return TRUE;
	}

	return FALSE;
}

void load_skills_definitions()
{
	char line1[256], line2[256], line3[256], line4[256], name[256];
	int i[15], j, sp_num, l;
	FILE* magic;

	if (!(magic = fopen(LIB_MISC "skills.lst", "r")))
	{
		log("Cann't open skills list file...");
		graceful_exit(1);
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %d %d %d %d %d", line1, line2, i, i + 1, i + 2, i + 3, i + 4) != 7)
		{
			log("Bad format for skill string!\r\n"
				"Format : <skill name (%%s %%s)>  <kin (%%d)> <class (%%d)> <remort (%%d)> <minlevel> <improove (%%d)> !");
			graceful_exit(1);
		}
		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*')
		{
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = fix_name_and_find_skill_num(name)) < 0)
		{
			log("Skill '%s' not found...", name);
			graceful_exit(1);
		}
        if (PlayerRace::GetKinNameByNum(i[0], ESex::SEX_MALE) == RACE_NAME_UNDEFINED)
		{
			log("Bad kin type for skill \"%s\"...", skill_info[sp_num].name);
			graceful_exit(1);
		}
		if (i[1] < 0 || i[1] >= NUM_PLAYER_CLASSES)
		{
			log("Bad class type for skill \"%s\"...", skill_info[sp_num].name);
			graceful_exit(1);
		}
		if (i[2] < 0 || i[2] >= MAX_REMORT)
		{
			log("Bad remort type for skill \"%s\"...", skill_info[sp_num].name);
			graceful_exit(1);
		}
		if (i[4])
		{
			skill_info[sp_num].k_improove[i[1]][i[0]] = MAX(1, i[4]);
			log("Improove set '%d' kin '%d' classes %d value %d", sp_num, i[0], i[1], i[4]);
		}
		if (i[3])
		{
			skill_info[sp_num].min_level[i[1]][i[0]] = i[3];
			log("Level set '%d' kin '%d' classes %d value %d", sp_num, i[0], i[1], i[3]);
		}
		skill_info[sp_num].min_remort[i[1]][i[0]] = i[2];
		log("Remort set '%d' kin '%d' classes %d value %d", sp_num, i[0], i[1], i[2]);
	}
	fclose(magic);
	if (!(magic = fopen(LIB_MISC "classskill.lst", "r")))
	{
		log("Cann't open classskill list file...");
		graceful_exit(1);
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %s %s", line1, line2, line3, line4) != 4)
		{
			log("Bad format for skill string!\r\n" "Format : <skill name (%%s %%s)> <kin (%%s)> <skills (%%s)> !");
			graceful_exit(1);
		}
		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*')
		{
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = fix_name_and_find_skill_num(name)) < 0)
		{
			log("Skill '%s' not found...", name);
			graceful_exit(1);
		}
		for (l = 0; line3[l] && l < NUM_KIN; l++)
		{
			if (!strchr("1xX!", line3[l]))
				continue;
			for (j = 0; line4[j] && j < NUM_PLAYER_CLASSES; j++)
			{
				if (!strchr("1xX!", line4[j]))
					continue;
				skill_info[sp_num].classknow[j][l] = KNOW_SKILL;
				log("Set skill '%s' kin %d classes %d is Know", skill_info[sp_num].name, l, j);
			}
		}
	}
	fclose(magic);
}

//Polud Читает данные из файла хранения параметров умений
void load_skills()
{
	const char *CLASS_SKILLS_FILE = LIB_MISC"classskills.xml";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CLASS_SKILLS_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node node_list = doc.child("skills");

	if (!node_list)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...classskills.xml read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node xNodeClass, xNodeSkill, race;
	int PCkin, PCclass, level_decrement;
	for (race = node_list.child("race"); race; race = race.next_sibling("race"))
	{
		PCkin = race.attribute("num_kin").as_int();
		for (xNodeClass = race.child("class"); xNodeClass; xNodeClass = xNodeClass.next_sibling("class"))
		{
			PCclass = xNodeClass.attribute("class_num").as_int();
			level_decrement = xNodeClass.attribute("level_decrement").as_int();
			for (xNodeSkill = xNodeClass.child("skill"); xNodeSkill; xNodeSkill = xNodeSkill.next_sibling("skill"))
			{
				int sk_num;
				std::string name = std::string(xNodeSkill.attribute("name").value());
				if ((sk_num = fix_name_and_find_skill_num(name)) < 0)
				{
					log("Skill '%s' not found...", name.c_str());
					graceful_exit(1);
				}
				skill_info[sk_num].classknow[PCclass][PCkin] = KNOW_SKILL;
				if ((level_decrement < 1 && level_decrement != -1) || level_decrement > MAX_REMORT)
				{
					log("ERROR: Недопустимый параметр level decrement класса %d.", PCclass);
					skill_info[sk_num].level_decrement[PCclass][PCkin] = -1;
				}
				else
				{
					skill_info[sk_num].level_decrement[PCclass][PCkin] = level_decrement;
				}
				//log("Умение '%s' для расы %d класса %d разрешено.", skill_info[sk_num].name, PCkin, PCclass);
				int value = xNodeSkill.attribute("improve").as_int();
				skill_info[sk_num].k_improove[PCclass][PCkin] = MAX(1, value);
				//log("Коэффициент улучшения умения '%s' расы %d класса %d установлен в %d", skill_info[sk_num].name, PCkin, PCclass, (int)skill_info[sk_num].k_improove[PCclass][PCkin]);
				value = xNodeSkill.attribute("level").as_int();
				if (value > 0 && value < LVL_IMMORT)
				{
					skill_info[sk_num].min_level[PCclass][PCkin] = value;
					//log("Минимальный уровень изучения умения '%s' расы %d класса %d установлен в %d", skill_info[sk_num].name, PCkin, PCclass, value);
				}
				else
				{
					log("ERROR: Недопустимый минимальный уровень изучения умения '%s' - %d", skill_info[sk_num].name, value);
					graceful_exit(1);
				}
				value = xNodeSkill.attribute("remort").as_int();
				if (value >= 0 && value < MAX_REMORT)
				{
					skill_info[sk_num].min_remort[PCclass][PCkin] = value;
					//log("Минимальное количество ремортов для изучения умения '%s' расы %d класса %d установлен в %d", skill_info[sk_num].name, PCkin, PCclass, skill_info[sk_num].min_remort[j][PCkin]);
				}
				else
				{
					log("ERROR: Недопустимое минимальное количество ремортов для умения '%s' - %d", skill_info[sk_num].name, value);
					graceful_exit(1);
				}
			}
		}
	}
}

/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void)
{
	FILE *magic;
	char line1[256], line2[256], line3[256], name[256];
	int i[15], j, sp_num;

	if (!(magic = fopen(LIB_MISC "magic.lst", "r")))
	{
		log("Can't open magic list file...");
		perror("fopen");
		graceful_exit(1);
	}

	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %d %d %d %d %d %d", line1, line2, i, i + 1, i + 2, i + 3, i + 4, i + 5) != 8)
		{
			log("Bad format for magic string!\r\n"
				"Format : <spell name (%%s %%s)> <kin (%%d)> <classes (%%d)> <remort (%%d)> <slot (%%d)> <level (%%d)>");
			graceful_exit(1);
		}

		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*')
		{
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}

		if ((sp_num = fix_name_and_find_spell_num(name)) < 0)
		{
			log("Spell '%s' not found...", name);
			graceful_exit(1);
		}

		if (i[0] < 0 || i[0] >= NUM_KIN)
		{
			log("Bad kin type for spell '%s' \"%d\"...", name, sp_num);
			graceful_exit(1);
		}
		if (i[1] < 0 || i[1] >= NUM_PLAYER_CLASSES)
		{
			log("Bad class type for spell '%s'  \"%d\"...", name, sp_num);
			graceful_exit(1);
		}
		if (i[2] < 0 || i[2] >= MAX_REMORT)
		{
			log("Bad remort type for spell '%s'  \"%d\"...", name, sp_num);
			graceful_exit(1);
		}
		mspell_remort(name, sp_num, i[0], i[1], i[2]);
		mspell_level(name, sp_num, i[0] , i[1], i[4]);
		mspell_slot(name, sp_num, i[0], i[1], i[3]);
		mspell_change(name, sp_num, i[0], i[1], i[5]);

	}
	fclose(magic);
	if (!(magic = fopen(LIB_MISC "items.lst", "r")))
	{
		log("Cann't open items list file...");
		graceful_exit(1);
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %s %d %d %d %d %d", line1, line2, line3, i, i + 1, i + 2, i + 3, i + 4) != 8)
		{
			log("Bad format for magic string!\r\n"
				"Format : <spell name (%%s %%s)> <type (%%s)> <items_vnum (%%d %%d %%d %%d)>");
			graceful_exit(1);
		}

		if (i[4] > 34)
			i[4] = 34;
		if (i[4] < 1)
			i[4] = 1;

		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*')
		{
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = fix_name_and_find_spell_num(name)) < 0)
		{
			log("Spell '%s' not found...", name);
			graceful_exit(1);
		}
		size_t c = strlen(line3);
		if (!strn_cmp(line3, "potion", c))
		{
			spell_create[sp_num].potion.items[0] = i[0];
			spell_create[sp_num].potion.items[1] = i[1];
			spell_create[sp_num].potion.items[2] = i[2];
			spell_create[sp_num].potion.rnumber = i[3];
			spell_create[sp_num].potion.min_caster_level = i[4];
			log("CREATE potion FOR MAGIC '%s'", spell_name(sp_num));
		}
		else if (!strn_cmp(line3, "wand", c))
		{
			spell_create[sp_num].wand.items[0] = i[0];
			spell_create[sp_num].wand.items[1] = i[1];
			spell_create[sp_num].wand.items[2] = i[2];
			spell_create[sp_num].wand.rnumber = i[3];
			spell_create[sp_num].wand.min_caster_level = i[4];
			log("CREATE wand FOR MAGIC '%s'", spell_name(sp_num));
		}
		else if (!strn_cmp(line3, "scroll", c))
		{
			spell_create[sp_num].scroll.items[0] = i[0];
			spell_create[sp_num].scroll.items[1] = i[1];
			spell_create[sp_num].scroll.items[2] = i[2];
			spell_create[sp_num].scroll.rnumber = i[3];
			spell_create[sp_num].scroll.min_caster_level = i[4];
			log("CREATE scroll FOR MAGIC '%s'", spell_name(sp_num));
		}
		else if (!strn_cmp(line3, "items", c))
		{
			spell_create[sp_num].items.items[0] = i[0];
			spell_create[sp_num].items.items[1] = i[1];
			spell_create[sp_num].items.items[2] = i[2];
			spell_create[sp_num].items.rnumber = i[3];
			spell_create[sp_num].items.min_caster_level = i[4];
			log("CREATE items FOR MAGIC '%s'", spell_name(sp_num));
		}
		else if (!strn_cmp(line3, "runes", c))
		{
			spell_create[sp_num].runes.items[0] = i[0];
			spell_create[sp_num].runes.items[1] = i[1];
			spell_create[sp_num].runes.items[2] = i[2];
			spell_create[sp_num].runes.rnumber = i[3];
			spell_create[sp_num].runes.min_caster_level = i[4];
			log("CREATE runes FOR MAGIC '%s'", spell_name(sp_num));
		}
		else
		{
			log("Unknown items option : %s", line3);
			graceful_exit(1);
		}
	}
	fclose(magic);
	// Load features variables - added by Gorrah
	if (!(magic = fopen(LIB_MISC "features.lst", "r")))
	{
		log("Cann't open features list file...");
		graceful_exit(1);
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %d %d %d %d %d %d %d",	line1, line2, i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6) != 9)
		{
			log("Bad format for feature string!\r\n"
				"Format : <feature name (%%s %%s)>  <kin (%%d %%d %%d)> <class (%%d)> <remort (%%d)> <level (%%d)> <naturalfeat (%%d)>!");
			graceful_exit(1);
		}
		name[0] = '\0';
		strcat(name, line1);
		if (*line2 != '*')
		{
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = find_feat_num(name)) <= 0)
		{
			log("Feat '%s' not found...", name);
			graceful_exit(1);
		}
		for (j = 0; j < NUM_KIN; j++)
			if (i[j] < 0 || i[j] > 1)
			{
				log("Bad race feat know type for feat \"%s\"... 0 or 1 expected", feat_info[sp_num].name);
				graceful_exit(1);
			}
		if (i[3] < 0 || i[3] >= NUM_PLAYER_CLASSES)
		{
			log("Bad class type for feat \"%s\"...", feat_info[sp_num].name);
			graceful_exit(1);
		}
		if (i[4] < 0 || i[4] >= MAX_REMORT)
		{
			log("Bad remort type for feat \"%s\"...", feat_info[sp_num].name);
			graceful_exit(1);
		}
		if (i[6] < 0 || i[6] > 1)
		{
			log("Bad natural classfeat type for feat \"%s\"... 0 or 1 expected", feat_info[sp_num].name);
			graceful_exit(1);
		}
		for (j = 0; j < NUM_KIN; j++)
			if (i[j] == 1)
			{
				//log("Setting up feat '%s'", feat_info[sp_num].name);
				feat_info[sp_num].classknow[i[3]][j] = TRUE;
				log("Classknow feat set '%s': %d kin: %d classes: %d Remort: %d Level: %d Natural: %d", feat_info[sp_num].name, sp_num, j, i[3], i[4], i[5], i[6]);

				feat_info[sp_num].min_remort[i[3]][j] = i[4];
				//log("Remort feat set '%d' kin '%d' classes %d value %d", sp_num, j, i[3], i[4]);

				feat_info[sp_num].slot[i[3]][j] = i[5];
				//log("Level feat set '%d' kin '%d' classes %d value %d", sp_num, j, i[3], i[5]);

				feat_info[sp_num].natural_classfeat[i[3]][j] = i[6] ? true : false;
				//log("Natural classfeature set '%d' kin '%d' classes %d", sp_num, j, i[3]);
			}
	}
	fclose(magic);
	// End of changed

//	Polud новый файл описания умений Skills.xml, если его нет - читаются старые
	load_skills();

	if (!(magic = fopen(LIB_MISC "skillvariables.lst", "r")))
	{
		log("Cann't open skillvariables list file...");
		graceful_exit(1);
	}

	// Загружаем переменные скилов из файла

	// ГОРНОЕ ДЕЛО
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

	// ЮВЕЛИР
	// Предварительно ставим значения по дефолту

	insgem_vars.lag = INSGEM_DFLT_LAG;
	insgem_vars.minus_for_affect = INSGEM_DFLT_MINUS_FOR_AFFECT;
	insgem_vars.prob_divide = INSGEM_DFLT_PROB_DIVIDE;
	insgem_vars.dikey_percent = INSGEM_DFLT_DIKEY_PERCENT;
	insgem_vars.timer_plus_percent = INSGEM_DFLT_TIMER_PLUS_PERCENT;
	insgem_vars.timer_minus_percent = INSGEM_DFLT_TIMER_MINUS_PERCENT;



	while (get_line(magic, name))
	{
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
			graceful_exit(1);
		}
		while (get_line(magic, name)) {
			if (!name[0] || name[0] == ';')
				continue;
			if (sscanf(name, "%d %s %s", i, line1, line2) != 3) {
				log("Bad format for magic string!\r\n"
				    "Format : <recipe number (%%d)> <races (%%s)> <classes (%%d)>");
				graceful_exit(1);
			}

			rcpt = im_get_recipe(i[0]);

			if (rcpt < 0) {
				log("Invalid recipe (%d)", i[0]);
				graceful_exit(1);
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
	if (!(magic = fopen(LIB_MISC "basic.lst", "r")))
	{
		log("Can't open basic values list file...");
		return;
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		i[0] = i[1] = i[2] = i[3] = i[4] = i[5] = 100000;
		if (sscanf(name, "%s %d %d %d %d %d %d", line, i, i + 1, i + 2, i + 3, i + 4, i + 5) < 1)
			continue;
		if (!str_cmp(line, "int"))
			mode = 1;
		else if (!str_cmp(line, "cha"))
			mode = 2;
		else if (!str_cmp(line, "size"))
			mode = 3;
		else if (!str_cmp(line, "weapon"))
			mode = 4;
		else if ((c = atoi(line)) > 0 && c <= 100 && mode > 0 && mode < 10)
		{
			int fields = 0;
			switch (mode)
			{			
			case 1:
				pointer = (int *) & (int_app[c].spell_aknowlege);
				fields = sizeof(int_app[c])/sizeof(int);
				break;

			case 2:
				pointer = (int *) & (cha_app[c].leadership);
				fields = sizeof(cha_app[c])/sizeof(int);
				break;

			case 3:
				pointer = (int *) & (size_app[c].ac);
				fields = sizeof(size_app[c])/sizeof(int);
				break;

			case 4:
				pointer = (int *) & (weapon_app[c].shocking);
				fields = sizeof(weapon_app[c])/sizeof(int);
				break;

			default:
				pointer = NULL;
			}

			if (pointer)  	//log("Mode %d - %d = %d %d %d %d %d %d",mode,c,
			{
				//    *i, *(i+1), *(i+2), *(i+3), *(i+4), *(i+5));
				for (j = 0; j < fields; j++)
				{
					if (i[j] != 100000)  	//log("[%d] %d <-> %d",j,*(pointer+j),*(i+j));
					{
						*(pointer + j) = *(i + j);
					}
				}
				//getchar();
			}
		}
	}
	fclose(magic);
	return;
}

/*
	Берет misc/grouping, первый столбик цифр считает номерами мортов,
	остальные столбики - значение макс. разрыва в уровнях для конкретного
	класса. На момент написания этого в конфиге присутствует 26 строк, макс.
	морт равен 50 - строки с мортами с 26 по 50 копируются с 25-мортовой строки.
*/
int GroupPenalties::init(void)
{
	char buf[MAX_INPUT_LENGTH];
	int clss = 0, remorts = 0, rows_assigned = 0, levels = 0, pos = 0, max_rows = MAX_REMORT+1;

	// пре-инициализация
	for (remorts = 0; remorts < max_rows; remorts++) //Строк в массиве должно быть на 1 больше, чем макс. морт
	{
		for (clss = 0; clss < NUM_PLAYER_CLASSES; clss++) //Столбцов в массиве должно быть ровно столько же, сколько есть классов
		{
			m_grouping[clss][remorts] = -1;
		}
	}

	FILE* f = fopen(LIB_MISC "grouping", "r");
	if (!f)
	{
		log("Невозможно открыть файл %s", LIB_MISC "grouping");
		return 1;
	}

	while (get_line(f, buf))
	{
		if (!buf[0] || buf[0] == ';' || buf[0] == '\n') //Строка пустая или строка-коммент
		{
			continue;
		}
		clss = 0;
		pos = 0;
		while (sscanf(&buf[pos], "%d", &levels) == 1)
		{
			while (buf[pos] == ' ' || buf[pos] == '\t')
			{
				pos++;
			}
			if (clss == 0) //Первый проход цикла по строке
			{
				remorts = levels; //Номера строк
				if (m_grouping[0][remorts] != -1)
				{
					log("Ошибка при чтении файла %s: дублирование параметров для %d ремортов",
						LIB_MISC "grouping", remorts);
					return 2;
				}
				if (remorts > MAX_REMORT || remorts < 0)
				{
					log("Ошибка при чтении файла %s: неверное значение количества ремортов: %d, "
						"должно быть в промежутке от 0 до %d",
						LIB_MISC "grouping", remorts, MAX_REMORT);
					return 3;
				}
			}
			else
			{
				m_grouping[clss - 1][remorts] = levels; // -1 потому что в массиве нет столбца с кол-вом мортов
			}
			clss++; //+Номер столбца массива
			while (buf[pos] != ' ' && buf[pos] != '\t' && buf[pos] != 0)
			{
				pos++; //Ищем следующее число в строке конфига
			}
		}
		if (clss != NUM_PLAYER_CLASSES+1)
		{
			log("Ошибка при чтении файла %s: неверный формат строки '%s', должно быть %d "
				"целых чисел, прочитали %d", LIB_MISC "grouping", buf, NUM_PLAYER_CLASSES+1, clss);
			return 4;
		}
		rows_assigned++;
	}

	if (rows_assigned < max_rows)
	{
		for (levels = remorts; levels < max_rows; levels++) //Берем свободную переменную
		{
			for (clss = 0; clss < NUM_PLAYER_CLASSES; clss++)
			{
				m_grouping[clss][levels] = m_grouping[clss][remorts]; //Копируем последнюю строку на все морты, для которых нет строк
			}
		}
	}
	fclose(f);
	return 0;
}

/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */

// Function to return the exp required for each class/level
int level_exp(CHAR_DATA * ch, int level)
{
	if (level > LVL_IMPL || level < 0)
	{
		log("SYSERR: Requesting exp for invalid level %d!", level);
		return 0;
	}

	/*
	 * Gods have exp close to EXP_MAX.  This statement should never have to
	 * changed, regardless of how many mortal or immortal levels exist.
	 */
	if (level > LVL_IMMORT)
	{
		return EXP_IMPL - ((LVL_IMPL - level) * 1000);
	}

	// Exp required for normal mortals is below
	float exp_modifier;
	if (GET_REMORT(ch) < MAX_EXP_COEFFICIENTS_USED)
		exp_modifier = exp_coefficients[GET_REMORT(ch)];
	else
		exp_modifier = exp_coefficients[MAX_EXP_COEFFICIENTS_USED];

	switch (GET_CLASS(ch))
	{

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (level)
		{
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
			// add new levels here
		case LVL_IMMORT:
			return int (exp_modifier * 94000000);
		}
		break;

	case CLASS_CLERIC:
	case CLASS_DRUID:
		switch (level)
		{
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
			// add new levels here
		case LVL_IMMORT:
			return int (exp_modifier * 87000000);
		}
		break;

	case CLASS_THIEF:
		switch (level)
		{
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return int (exp_modifier * 1000);
		case 3:
			return int (exp_modifier * 2000);
		case 4:
			return int (exp_modifier * 4000);
		case 5:
			return int (exp_modifier * 8000);
		case 6:
			return int (exp_modifier * 15000);
		case 7:
			return int (exp_modifier * 28000);
		case 8:
			return int (exp_modifier * 52000);
		case 9:
			return int (exp_modifier * 85000);
		case 10:
			return int (exp_modifier * 131000);
		case 11:
			return int (exp_modifier * 192000);
		case 12:
			return int (exp_modifier * 271000);
		case 13:
			return int (exp_modifier * 372000);
		case 14:
			return int (exp_modifier * 512000);
		case 15:
			return int (exp_modifier * 672000);
		case 16:
			return int (exp_modifier * 854000);
		case 17:
			return int (exp_modifier * 1047000);
		case 18:
			return int (exp_modifier * 1274000);
		case 19:
			return int (exp_modifier * 1534000);
		case 20:
			return int (exp_modifier * 1860000);
		case 21:
			return int (exp_modifier * 2520000);
		case 22:
			return int (exp_modifier * 3380000);
		case 23:
			return int (exp_modifier * 4374000);
		case 24:
			return int (exp_modifier * 5500000);
		case 25:
			return int (exp_modifier * 6827000);
		case 26:
			return int (exp_modifier * 8667000);
		case 27:
			return int (exp_modifier * 13334000);
		case 28:
			return int (exp_modifier * 20000000);
		case 29:
			return int (exp_modifier * 28667000);
		case 30:
			return int (exp_modifier * 40000000);
			// add new levels here
		case LVL_IMMORT:
			return int (exp_modifier * 53000000);
		}
		break;

	case CLASS_ASSASINE:
	case CLASS_MERCHANT:
		switch (level)
		{
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
			// add new levels here
		case LVL_IMMORT:
			return int (exp_modifier * 79000000);
		}
		break;

	case CLASS_WARRIOR:
	case CLASS_GUARD:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		switch (level)
		{
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
			// add new levels here
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

GroupPenalties grouping;	///< TODO: get rid of this global variable.

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
