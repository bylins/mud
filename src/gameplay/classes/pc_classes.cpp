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
#include "pc_classes.h"
#include "gameplay/core/experience.h"
#include "administration/privilege.h"
#include "gameplay/core/remort.h"
#include "gameplay/mechanics/condition.h"
#include "gameplay/mechanics/minions.h"

#include "gameplay/magic/magic_utils.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/fight/pk.h"
#include "gameplay/statistics/top.h"
#include "gameplay/communication/offtop.h"
#include "engine/ui/color.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/named_stuff.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/magic/spells_info.h"
#include "gameplay/core/remort.h"
#include "engine/db/global_objects.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"

#include <algorithm>


extern int siteok_everyone;

// local functions
byte saving_throws(int class_num, int type, int level);
int invalid_anti_class(CharData *ch, const ObjData *obj);
byte GetExtendSavingThrows(ECharClass class_id, ESaving save, int level);
int invalid_unique(CharData *ch, const ObjData *obj);
extern bool char_to_pk_clan(CharData *ch);
// Names first

// The menu for choosing a religion in interpreter.c:
const char *religion_menu =
	"\r\n" "Какой религии вы отдаете предпочтение :\r\n" "  Я[з]ычество\r\n" "  [Х]ристианство\r\n";

const int kReligionAny = 100;

/* Соответствие классов и религий. kReligionPoly-класс не может быть христианином
                                   kReligionMono-класс не может быть язычником
				   RELIGION_ANY - класс может быть кем угодно */
const int class_religion[] = {kReligionAny,        //Лекарь
							  kReligionAny,        //Колдун
							  kReligionAny,        //Тать
							  kReligionAny,        //Богатырь
							  kReligionAny,        //Наемник
							  kReligionAny,        //Дружинник
							  kReligionAny,        //Кудесник
							  kReligionAny,        //Волшебник
							  kReligionAny,        //Чернокнижник
							  kReligionAny,        //Витязь
							  kReligionAny,        //Охотник
							  kReligionAny,        //Кузнец
							  kReligionAny,        //Купец
							  kReligionPoly        //Волхв
};

/* Вообще то такие вещи должен сам контейнер делать, но пока не реализован
 * какой-нибудь нормальный быстрый поиск по имени, а привинчивать костыль,
 * чтобы потом его убирать, не хочется. Не забыть переделать - ABYRVALG */
ECharClass FindAvailableCharClassId(const std::string &class_name) {
	for (const auto &it: MUD::Classes()) {
		if (it.IsAvailable()) {
		 	if (CompareParam(class_name, it.GetPluralName()) || CompareParam(class_name, it.GetName())) {
				return it.GetId();
		 	}
		}
	}
	return ECharClass::kUndefined;
}

// Таблицы бызовых спасбросков

const byte sav_01[50] =
	{
		90, 90, 90, 90, 90, 89, 89, 88, 88, 87,    // 00-09
		86, 85, 84, 83, 81, 79, 78, 75, 73, 71,    // 10-19
		68, 65, 62, 59, 56, 52, 48, 44, 40, 35,    // 20-29
		30, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_02[50] =
	{
		90, 90, 90, 90, 90, 89, 89, 88, 87, 87,    // 00-09
		86, 84, 83, 81, 80, 78, 75, 73, 70, 68,    // 10-19
		65, 61, 58, 54, 50, 46, 41, 36, 31, 26,    // 20-29
		20, 15, 10, 9, 8, 7, 6, 5, 4, 3,    // 30-39
		2, 1, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_03[50] =
	{
		90, 90, 90, 90, 90, 89, 89, 88, 88, 87,    // 00-09
		86, 85, 83, 82, 80, 79, 76, 74, 72, 69,    // 10-19
		66, 63, 60, 57, 53, 49, 45, 40, 35, 30,    // 20-29
		25, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_04[50] =
	{
		90, 90, 90, 90, 90, 90, 89, 89, 89, 88,    // 00-09
		87, 87, 86, 85, 84, 83, 82, 80, 79, 77,    // 10-19
		75, 74, 72, 69, 67, 65, 62, 59, 56, 53,    // 20-29
		50, 45, 40, 35, 30, 25, 20, 15, 10, 5,    // 30-39
		4, 3, 2, 1, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_05[50] =
	{
		90, 90, 90, 90, 90, 89, 89, 89, 88, 87,    // 00-09
		86, 86, 84, 83, 82, 80, 79, 77, 75, 72,    // 10-19
		70, 67, 65, 62, 59, 55, 52, 48, 44, 39,    // 20-29
		35, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
//kClassMob
const byte sav_06[100] =
	{
		90, 90, 90, 90, 90, 90, 89, 89, 88, 88,    // 00-09
		87, 86, 85, 84, 82, 80, 78, 76, 74, 72,    // 10-19
		70, 67, 64, 61, 58, 55, 52, 49, 46, 43,    // 20-29
		41, 39, 38, 37, 33, 25, 20, 15, 10, 5,    // 30-39
		4, 3, 2, 1, 0, 0, 0, 0, 0, 0,            // 40-49
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 50-59
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 60-69
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 70-79
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 80-89
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0            // 90-99
	};
const byte sav_08[50] =
	{
		90, 90, 90, 90, 90, 89, 89, 89, 88, 88,    // 00-09
		87, 86, 85, 84, 83, 81, 80, 78, 76, 74,    // 10-19
		72, 70, 67, 64, 61, 58, 55, 52, 48, 44,    // 20-29
		40, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_09[50] =
	{
		90, 75, 73, 71, 69, 67, 65, 63, 61, 60,    // 00-09
		59, 57, 55, 53, 51, 50, 49, 47, 45, 43,    // 10-19
		41, 40, 39, 37, 35, 33, 31, 29, 27, 25,    // 20-29
		23, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_10[50] =
	{
		90, 80, 79, 78, 76, 75, 73, 70, 67, 65,    // 00-09
		64, 63, 61, 60, 59, 57, 56, 55, 54, 53,    // 10-19
		52, 51, 50, 48, 45, 44, 42, 40, 39, 38,    // 20-29
		37, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_11[50] =
	{
		90, 80, 79, 78, 77, 76, 75, 74, 73, 72,    // 00-09
		71, 70, 69, 68, 67, 66, 65, 64, 63, 62,    // 10-19
		61, 60, 59, 58, 57, 56, 55, 54, 53, 52,    // 20-29
		51, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
const byte sav_12[50] =
	{
		90, 85, 83, 82, 80, 75, 70, 65, 63, 62,    // 00-09
		60, 55, 50, 45, 43, 42, 40, 37, 33, 30,    // 10-19
		29, 28, 26, 25, 24, 23, 21, 20, 19, 18,    // 20-29
		17, 16, 15, 14, 13, 12, 11, 10, 9, 8,    // 30-39
		7, 6, 5, 4, 3, 2, 1, 0, 0, 0    // 40-49
	};
//kClassMob
const byte sav_13[100] =
	{
		90, 83, 81, 79, 77, 75, 72, 68, 65, 63,    // 00-09
		61, 58, 56, 53, 50, 47, 45, 43, 42, 41,    // 10-19
		40, 38, 37, 36, 34, 33, 32, 31, 30, 29,    // 20-29
		27, 26, 25, 24, 23, 22, 21, 20, 18, 16,    // 30-39
		12, 10, 8, 6, 4, 2, 1, 0, 0, 0,            // 40-49
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 50-59
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 60-69
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 70-79
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,            // 80-89
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0            // 90-99
	};
const byte sav_14[50] =
	{
		100, 100, 100, 100, 100, 100, 100, 100, 100, 100,    // 00-09
		100, 100, 100, 100, 100, 100, 100, 100, 100, 100,    // 10-19
		100, 100, 100, 100, 100, 100, 100, 100, 100, 100,    // 20-29
		100, 70, 70, 70, 70, 70, 70, 70, 70, 70,    // 30-39
		70, 70, 70, 70, 70, 70, 70, 70, 70, 70    // 40-49
	};
const byte sav_15[50] =
	{
		100, 99, 98, 97, 96, 95, 94, 93, 92, 91,    // 00-09
		90, 89, 88, 87, 86, 85, 84, 83, 82, 81,    // 10-19
		80, 79, 78, 77, 76, 75, 74, 73, 72, 71,    // 20-29
		70, 50, 50, 50, 50, 50, 50, 50, 50, 50,    // 30-39
		50, 50, 50, 50, 50, 50, 50, 50, 50, 50    // 40-49
	};
const byte sav_16[50] =
	{
		100, 99, 97, 96, 95, 94, 92, 91, 89, 88,    // 00-09
		86, 85, 84, 83, 81, 80, 79, 77, 76, 75,    // 10-19
		74, 72, 71, 70, 68, 65, 64, 63, 62, 61,    // 20-29
		60, 58, 57, 56, 54, 52, 51, 49, 47, 46,    // 30-39
		45, 43, 42, 41, 39, 37, 35, 34, 32, 31    // 40-49
	};
const byte sav_17[50] =
	{
		100, 99, 97, 96, 95, 94, 92, 91, 89, 88,    // 00-09
		86, 84, 82, 80, 78, 76, 74, 72, 70, 68,    // 10-19
		64, 62, 60, 58, 56, 54, 52, 50, 49, 47,    // 20-29
		45, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 30-39
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 40-49
	};
//kClassMob
const byte sav_18[100] =
	{
		100, 100, 100, 100, 100, 99, 99, 99, 99, 99,    // 00-09
		98, 98, 98, 98, 98, 97, 97, 97, 97, 97,            // 10-19
		96, 96, 96, 96, 96, 95, 95, 95, 95, 95,            // 20-29
		94, 94, 94, 94, 94, 93, 93, 93, 93, 93,            // 30-39
		92, 92, 92, 92, 92, 91, 91, 91, 91, 91,            // 40-49
		91, 91, 91, 91, 91, 90, 90, 90, 90, 90,            // 50-59
		89, 89, 89, 89, 89, 88, 88, 88, 88, 88,            // 60-69
		87, 87, 87, 87, 87, 86, 86, 86, 86, 86,            // 70-79
		85, 85, 85, 85, 85, 84, 84, 84, 84, 84,            // 80-89
		83, 83, 83, 83, 83, 82, 82, 82, 82, 82            // 90-99
	};

// {CLASS,{PARA,ROD,AFFECT,BREATH,SPELL,BASIC}}
struct ClassSavings {
	ECharClass chclass;
	const byte *saves[to_underlying(ESaving::kLast) + 1];
};

const ClassSavings std_saving[] = {
	{ECharClass::kSorcerer, {sav_01, sav_10, sav_08, sav_14}},
	{ECharClass::kConjurer, {sav_01, sav_09, sav_02, sav_14}},
	{ECharClass::kCharmer, {sav_02, sav_09, sav_02, sav_14}},
	{ECharClass::kWizard, {sav_01, sav_09, sav_01, sav_14}},
	{ECharClass::kNecromancer, {sav_01, sav_09, sav_01, sav_14}},
	{ECharClass::kMagus, {sav_03, sav_10, sav_03, sav_14}},
	{ECharClass::kThief, {sav_08, sav_11, sav_08, sav_15}},
	{ECharClass::kAssasine, {sav_08, sav_11, sav_08, sav_15}},
	{ECharClass::kMerchant, {sav_08, sav_11, sav_08, sav_15}},
	{ECharClass::kWarrior, {sav_04, sav_12, sav_04, sav_16}},
	{ECharClass::kGuard, {sav_04, sav_12, sav_04, sav_16}},
	{ECharClass::kVigilant, {sav_04, sav_12, sav_04, sav_16}},
	{ECharClass::kPaladine, {sav_05, sav_12, sav_05, sav_17}},
	{ECharClass::kRanger, {sav_05, sav_12, sav_05, sav_17}},
	{ECharClass::kNpcBase, {sav_06, sav_13, sav_06, sav_18}},
	{ECharClass::kUndefined, {sav_02, sav_12, sav_02, sav_16}}
};

byte GetSavingThrows(ECharClass class_id, ESaving type, int level) {
	return GetExtendSavingThrows(class_id, type, level);
}

byte GetExtendSavingThrows(ECharClass class_id, ESaving save, int level) {
	int i;
	if (save < ESaving::kFirst || save > ESaving::kLast) {
		return 100; // Что за 100? Почему 100? kMaxSaving равен 400. Идиотизм.
	}
	if (level <= 0 || level > kMaxMobLevel) {
		return 100;
	}
	--level;
	for (i = 0; std_saving[i].chclass != ECharClass::kUndefined && std_saving[i].chclass != class_id; ++i);

	return std_saving[i].saves[to_underlying(save)][level];
}

// THAC0 for classes and levels.  (To Hit Armor Class 0)
int GetThac0(ECharClass class_id, int level) {
	switch (class_id) {
		case ECharClass::kConjurer: [[fallthrough]];
		case ECharClass::kWizard: [[fallthrough]];
		case ECharClass::kCharmer: [[fallthrough]];
		case ECharClass::kNecromancer: {
			switch (level) {
				case 0: return 100; break;
				case 1: return 20;
				case 2: return 20;
				case 3: return 20;
				case 4: return 19;
				case 5: return 19;
				case 6: return 19;
				case 7: return 18;
				case 8: return 18;
				case 9: return 18;
				case 10: return 17;
				case 11: return 17;
				case 12: return 17;
				case 13: return 16;
				case 14: return 16;
				case 15: return 16;
				case 16: return 15;
				case 17: return 15;
				case 18: return 15;
				case 19: return 14;
				case 20: return 14;
				case 21: return 14;
				case 22: return 13;
				case 23: return 13;
				case 24: return 13;
				case 25: return 12;
				case 26: return 12;
				case 27: return 12;
				case 28: return 11;
				case 29: return 11;
				case 30: return 10;
				case 31: return 0;
				case 32: return 0;
				case 33: return 0;
				case 34: return 0;
				default: log("SYSERR: Missing level for mage thac0.");
					break;
			}
			 break;
		}
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: {
			switch (level) {
				case 0: return 100;
				case 1: return 20;
				case 2: return 20;
				case 3: return 20;
				case 4: return 18;
				case 5: return 18;
				case 6: return 18;
				case 7: return 16;
				case 8: return 16;
				case 9: return 16;
				case 10: return 14;
				case 11: return 14;
				case 12: return 14;
				case 13: return 12;
				case 14: return 12;
				case 15: return 12;
				case 16: return 10;
				case 17: return 10;
				case 18: return 10;
				case 19: return 8;
				case 20: return 8;
				case 21: return 8;
				case 22: return 6;
				case 23: return 6;
				case 24: return 6;
				case 25: return 4;
				case 26: return 4;
				case 27: return 4;
				case 28: return 2;
				case 29: return 2;
				case 30: return 1;
				case 31: return 0;
				case 32: return 0;
				case 33: return 0;
				case 34: return 0;
				default: log("SYSERR: Missing level for sorcerer thac0.");
					break;
			}
			 break;
		}
		case ECharClass::kAssasine:
		case ECharClass::kThief: [[fallthrough]];
		case ECharClass::kMerchant: {
			switch (level) {
				case 0: return 100;
				case 1: return 20;
				case 2: return 20;
				case 3: return 19;
				case 4: return 19;
				case 5: return 18;
				case 6: return 18;
				case 7: return 17;
				case 8: return 17;
				case 9: return 16;
				case 10: return 16;
				case 11: return 15;
				case 12: return 15;
				case 13: return 14;
				case 14: return 14;
				case 15: return 13;
				case 16: return 13;
				case 17: return 12;
				case 18: return 12;
				case 19: return 11;
				case 20: return 11;
				case 21: return 10;
				case 22: return 10;
				case 23: return 9;
				case 24: return 9;
				case 25: return 8;
				case 26: return 8;
				case 27: return 7;
				case 28: return 7;
				case 29: return 6;
				case 30: return 5;
				case 31: return 0;
				case 32: return 0;
				case 33: return 0;
				case 34: return 0;
				default: log("SYSERR: Missing level for thief thac0.");
					break;
			}
			 break;
		}
		case ECharClass::kWarrior: [[fallthrough]];
		case ECharClass::kGuard: {
			switch (level) {
				case 0: return 100;
				case 1: return 20;
				case 2: return 19;
				case 3: return 18;
				case 4: return 17;
				case 5: return 16;
				case 6: return 15;
				case 7: return 14;
				case 8: return 14;
				case 9: return 13;
				case 10: return 12;
				case 11: return 11;
				case 12: return 10;
				case 13: return 9;
				case 14: return 8;
				case 15: return 7;
				case 16: return 6;
				case 17: return 5;
				case 18: return 4;
				case 19: return 3;
				case 20: return 2;
				case 21: return 1;
				case 22: return 1;
				case 23: return 1;
				case 24: return 1;
				case 25: return 1;
				case 26: return 1;
				case 27: return 1;
				case 28: return 1;
				case 29: return 1;
				case 30: return 0;
				case 31: return 0;
				case 32: return 0;
				case 33: return 0;
				case 34: return 0;
				default: log("SYSERR: Missing level for warrior thac0.");
					break;
			}
			 break;
		}
		case ECharClass::kPaladine:
		case ECharClass::kRanger: [[fallthrough]];
		case ECharClass::kVigilant: {
			switch (level) {
				case 0: return 100;
				case 1: return 20;
				case 2: return 19;
				case 3: return 18;
				case 4: return 18;
				case 5: return 17;
				case 6: return 16;
				case 7: return 16;
				case 8: return 15;
				case 9: return 14;
				case 10: return 14;
				case 11: return 13;
				case 12: return 12;
				case 13: return 11;
				case 14: return 11;
				case 15: return 10;
				case 16: return 10;
				case 17: return 9;
				case 18: return 8;
				case 19: return 7;
				case 20: return 6;
				case 21: return 6;
				case 22: return 5;
				case 23: return 5;
				case 24: return 4;
				case 25: return 4;
				case 26: return 3;
				case 27: return 3;
				case 28: return 2;
				case 29: return 1;
				case 30: return 0;
				case 31: return 0;
				case 32: return 0;
				case 33: return 0;
				case 34: return 0;
				default: log("SYSERR: Missing level for warrior thac0.");
					break;
			}
			 break;
		}
		default: log("SYSERR: Unknown class in thac0 chart.");
			break;
	}

	// Will not get there unless something is wrong.
	return 100;
}

// AC0 for classes and levels.
int GetExtraAc0(ECharClass class_id, int level) {
	switch (class_id) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer: [[fallthrough]];
		case ECharClass::kNecromancer: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return 0;
				case 9: return 0;
				case 10: return 0;
				case 11: return 0;
				case 12: return 0;
				case 13: return 0;
				case 14: return 0;
				case 15: return 0;
				case 16: return -1;
				case 17: return -1;
				case 18: return -1;
				case 19: return -1;
				case 20: return -1;
				case 21: return -1;
				case 22: return -1;
				case 23: return -1;
				case 24: return -1;
				case 25: return -1;
				case 26: return -1;
				case 27: return -1;
				case 28: return -1;
				case 29: return -1;
				case 30: return -1;
				case 31: return -5;
				case 32: return -10;
				case 33: return -15;
				case 34: return -20;
				default: return 0;
			}
			 break;
		}
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return 0;
				case 9: return 0;
				case 10: return 0;
				case 11: return -1;
				case 12: return -1;
				case 13: return -1;
				case 14: return -1;
				case 15: return -1;
				case 16: return -1;
				case 17: return -1;
				case 18: return -1;
				case 19: return -1;
				case 20: return -1;
				case 21: return -2;
				case 22: return -2;
				case 23: return -2;
				case 24: return -2;
				case 25: return -2;
				case 26: return -2;
				case 27: return -2;
				case 28: return -2;
				case 29: return -2;
				case 30: return -2;
				case 31: return -5;
				case 32: return -10;
				case 33: return -15;
				case 34: return -20;
				default: return 0;
			}
			break;
		}
		case ECharClass::kAssasine:
		case ECharClass::kThief: [[fallthrough]];
		case ECharClass::kMerchant: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return 0;
				case 9: return -1;
				case 10: return -1;
				case 11: return -1;
				case 12: return -1;
				case 13: return -1;
				case 14: return -1;
				case 15: return -1;
				case 16: return -1;
				case 17: return -2;
				case 18: return -2;
				case 19: return -2;
				case 20: return -2;
				case 21: return -2;
				case 22: return -2;
				case 23: return -2;
				case 24: return -2;
				case 25: return -3;
				case 26: return -3;
				case 27: return -3;
				case 28: return -3;
				case 29: return -3;
				case 30: return -3;
				case 31: return -5;
				case 32: return -10;
				case 33: return -15;
				case 34: return -20;
				default: return 0;
			}
			 break;
		}
		case ECharClass::kWarrior: [[fallthrough]];
		case ECharClass::kGuard: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return -1;
				case 7: return -1;
				case 8: return -1;
				case 9: return -1;
				case 10: return -1;
				case 11: return -2;
				case 12: return -2;
				case 13: return -2;
				case 14: return -2;
				case 15: return -2;
				case 16: return -3;
				case 17: return -3;
				case 18: return -3;
				case 19: return -3;
				case 20: return -3;
				case 21: return -4;
				case 22: return -4;
				case 23: return -4;
				case 24: return -4;
				case 25: return -4;
				case 26: return -5;
				case 27: return -5;
				case 28: return -5;
				case 29: return -5;
				case 30: return -5;
				case 31: return -10;
				case 32: return -15;
				case 33: return -20;
				case 34: return -25;
				default: return 0;
			}
			 break;
			}
		case ECharClass::kPaladine:
		case ECharClass::kRanger: [[fallthrough]];
		case ECharClass::kVigilant: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return -1;
				case 9: return -1;
				case 10: return -1;
				case 11: return -1;
				case 12: return -1;
				case 13: return -1;
				case 14: return -1;
				case 15: return -2;
				case 16: return -2;
				case 17: return -2;
				case 18: return -2;
				case 19: return -2;
				case 20: return -2;
				case 21: return -2;
				case 22: return -3;
				case 23: return -3;
				case 24: return -3;
				case 25: return -3;
				case 26: return -3;
				case 27: return -3;
				case 28: return -3;
				case 29: return -4;
				case 30: return -4;
				case 31: return -5;
				case 32: return -10;
				case 33: return -15;
				case 34: return -20;
				default: return 0;
			}
			 break;
			}
		default: return 0;
	}
	return 0;
}

// DAMROLL for classes and levels.
int GetExtraDamroll(ECharClass class_id, int level) {
	switch (class_id) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer: [[fallthrough]];
		case ECharClass::kNecromancer: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return 0;
				case 9: return 0;
				case 10: return 0;
				case 11: return 1;
				case 12: return 1;
				case 13: return 1;
				case 14: return 1;
				case 15: return 1;
				case 16: return 1;
				case 17: return 1;
				case 18: return 1;
				case 19: return 1;
				case 20: return 1;
				case 21: return 2;
				case 22: return 2;
				case 23: return 2;
				case 24: return 2;
				case 25: return 2;
				case 26: return 2;
				case 27: return 2;
				case 28: return 2;
				case 29: return 2;
				case 30: return 2;
				case 31: return 4;
				case 32: return 6;
				case 33: return 8;
				case 34: return 10;
				default: return 0;
			}
			 break;
			}
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return 0;
				case 9: return 1;
				case 10: return 1;
				case 11: return 1;
				case 12: return 1;
				case 13: return 1;
				case 14: return 1;
				case 15: return 1;
				case 16: return 1;
				case 17: return 2;
				case 18: return 2;
				case 19: return 2;
				case 20: return 2;
				case 21: return 2;
				case 22: return 2;
				case 23: return 2;
				case 24: return 2;
				case 25: return 3;
				case 26: return 3;
				case 27: return 3;
				case 28: return 3;
				case 29: return 3;
				case 30: return 3;
				case 31: return 5;
				case 32: return 7;
				case 33: return 9;
				case 34: return 10;
				default: return 0;
			}
			 break;
			}
		case ECharClass::kAssasine:
		case ECharClass::kThief: [[fallthrough]];
		case ECharClass::kMerchant: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 0;
				case 8: return 1;
				case 9: return 1;
				case 10: return 1;
				case 11: return 1;
				case 12: return 1;
				case 13: return 1;
				case 14: return 1;
				case 15: return 2;
				case 16: return 2;
				case 17: return 2;
				case 18: return 2;
				case 19: return 2;
				case 20: return 2;
				case 21: return 2;
				case 22: return 3;
				case 23: return 3;
				case 24: return 3;
				case 25: return 3;
				case 26: return 3;
				case 27: return 3;
				case 28: return 3;
				case 29: return 4;
				case 30: return 4;
				case 31: return 5;
				case 32: return 7;
				case 33: return 9;
				case 34: return 11;
				default: return 0;
			}
			 break;
			}
		case ECharClass::kWarrior: [[fallthrough]];
		case ECharClass::kGuard: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 1;
				case 7: return 1;
				case 8: return 1;
				case 9: return 1;
				case 10: return 1;
				case 11: return 2;
				case 12: return 2;
				case 13: return 2;
				case 14: return 2;
				case 15: return 2;
				case 16: return 3;
				case 17: return 3;
				case 18: return 3;
				case 19: return 3;
				case 20: return 3;
				case 21: return 4;
				case 22: return 4;
				case 23: return 4;
				case 24: return 4;
				case 25: return 4;
				case 26: return 5;
				case 27: return 5;
				case 28: return 5;
				case 29: return 5;
				case 30: return 5;
				case 31: return 10;
				case 32: return 15;
				case 33: return 20;
				case 34: return 25;
				default: return 0;
			}
			 break;
			}
		case ECharClass::kPaladine:
		case ECharClass::kRanger: [[fallthrough]];
		case ECharClass::kVigilant: {
			switch (level) {
				case 0: return 0;
				case 1: return 0;
				case 2: return 0;
				case 3: return 0;
				case 4: return 0;
				case 5: return 0;
				case 6: return 0;
				case 7: return 1;
				case 8: return 1;
				case 9: return 1;
				case 10: return 1;
				case 11: return 1;
				case 12: return 1;
				case 13: return 2;
				case 14: return 2;
				case 15: return 2;
				case 16: return 2;
				case 17: return 2;
				case 18: return 2;
				case 19: return 3;
				case 20: return 3;
				case 21: return 3;
				case 22: return 3;
				case 23: return 3;
				case 24: return 3;
				case 25: return 4;
				case 26: return 4;
				case 27: return 4;
				case 28: return 4;
				case 29: return 4;
				case 30: return 4;
				case 31: return 5;
				case 32: return 10;
				case 33: return 15;
				case 34: return 20;
				default: return 0;
			}
			 break;
			}
		default: return 0;
	}
	return 0;
}

// Some initializations for characters, including initial skills
void init_warcry(CharData *ch) // проставление кличей в обход античита
{
	if (ch->GetClass() == ECharClass::kGuard)
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfDefence), ESpellType::kKnow); // клич призыв к обороне

	if (ch->GetClass() == ECharClass::kRanger) {
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfExperience), ESpellType::kKnow); // клич опыта
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfLuck), ESpellType::kKnow); // клич удачи
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfPhysdamage), ESpellType::kKnow); // клич +дамага
	}
	if (ch->GetClass() == ECharClass::kWarrior) {
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfBattle), ESpellType::kKnow); // клич призыв битвы
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfPower), ESpellType::kKnow); // клич призыв мощи
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfBless), ESpellType::kKnow); // клич призывы доблести
		SET_BIT(GET_SPELL_TYPE(ch, ESpell::kWarcryOfCourage), ESpellType::kKnow); // клич призыв отваги
	}

}

void DoPcInit(CharData *ch, bool is_newbie) {
	ch->set_level(1);
	ch->set_exp(1);
	ch->set_max_hit(10);
	if (is_newbie || (remort::GetRealRemort(ch) >= 9 && remort::GetRealRemort(ch) % 3 == 0)) {
		SetSkill(ch, ESkill::kHangovering, 10);
	}

	if (is_newbie && IS_MANA_CASTER(ch)) {
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(ch, spell_id) = ESpellType::kRunes;
		}
	}

	if (is_newbie) {
		log("Create new player %s", GET_NAME(ch));
		std::vector<int> outfit_list(Noob::get_start_outfit(ch));
		for (int & i : outfit_list) {
			const ObjData::shared_ptr obj = world_objects.create_from_prototype_by_vnum(i);
			if (obj) {
				obj->set_extra_flag(EObjFlag::kNosell);
				obj->set_extra_flag(EObjFlag::kDecay);
				obj->set_cost(0);
				obj->set_rent_off(0);
				obj->set_rent_on(0);
				PlaceObjToInventory(obj.get(), ch);
				Noob::equip_start_outfit(ch, obj.get());
			}
		}
	}

	switch (ch->GetClass()) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
		case ECharClass::kMagus: SetSkill(ch, ESkill::kSideAttack, 10);
			break;
		case ECharClass::kSorcerer: SetSkill(ch, ESkill::kSideAttack, 50);
			break;
		case ECharClass::kThief:
		case ECharClass::kAssasine: SetSkill(ch, ESkill::kSideAttack, 75);
			break;
		case ECharClass::kMerchant: SetSkill(ch, ESkill::kSideAttack, 85);
			break;
		case ECharClass::kGuard:
		case ECharClass::kPaladine:
		case ECharClass::kWarrior:
		case ECharClass::kRanger:
			if (GetSkill(ch, ESkill::kRiding) == 0)
				SetSkill(ch, ESkill::kRiding, 10);
			SetSkill(ch, ESkill::kSideAttack, 95);
			break;
		case ECharClass::kVigilant: SetSkill(ch, ESkill::kSideAttack, 95);
			break;
		default: break;
	}

	experience::advance_level(ch);
	sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GetRealLevel(ch));
	mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);

	ch->set_hit(ch->get_real_max_hit());
	ch->set_move(ch->get_real_max_move());

	GET_COND(ch, condition::kThirst) = 0;
	GET_COND(ch, condition::kFull) = 0;
	GET_COND(ch, condition::kDrunk) = 0;
	// проставим кличи
	init_warcry(ch);
	if (siteok_everyone) {
		ch->SetFlag(EPlrFlag::kSiteOk);
	}
}

// * Перерасчет максимальных родных хп персонажа.
// * При входе в игру, левеле/делевеле, добавлении/удалении славы.
void check_max_hp(CharData *ch) {
	ch->set_max_hit(PlayerSystem::con_natural_hp(ch));
}

// * Обработка событий при левел-апе.

/*
 * invalid_class is used by handler.cpp to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_unique(CharData *ch, const ObjData *obj) {
	ObjData *object;
	if (!IS_CORPSE(obj)) {
		for (object = obj->get_contains(); object; object = object->get_next_content()) {
			if (invalid_unique(ch, object)) {
				return (true);
			}
		}
	}
	if (!ch
		|| !obj
		|| (ch->IsNpc()
			&& !AFF_FLAGGED(ch, EAffect::kCharmed))
		|| privilege::IsImmortal(ch)
		|| obj->get_owner() == 0
		|| obj->get_owner() == ch->get_uid()) {
		return (false);
	}
	return (true);
}

int invalid_anti_class(CharData *ch, const ObjData *obj) {
	if (!IS_CORPSE(obj)) {
		for (const ObjData *object = obj->get_contains(); object; object = object->get_next_content()) {
			if (invalid_anti_class(ch, object) || NamedStuff::check_named(ch, object, false)) {
				return (true);
			}
		}
	}
	if (obj->has_anti_flag(EAntiFlag::kCharmice)
		&& AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return (true);
	}
	if ((ch->IsNpc() || privilege::IsImmortal(ch)) && !IsCharmice(ch)) {
		return (false);
	}
	if ((obj->has_anti_flag(EAntiFlag::kNoPkClan) && char_to_pk_clan(ch))) {
		return (true);
	}

	if ((obj->has_anti_flag(EAntiFlag::kMono) && GET_RELIGION(ch) == kReligionMono)
		|| (obj->has_anti_flag(EAntiFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly)
		|| (obj->has_anti_flag(EAntiFlag::kMage) && IsMage(ch))
		|| (obj->has_anti_flag(EAntiFlag::kConjurer) && IS_CONJURER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kCharmer) && IS_CHARMER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kWizard) && IS_WIZARD(ch))
		|| (obj->has_anti_flag(EAntiFlag::kNecromancer) && IS_NECROMANCER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kFighter) && IsFighter(ch))
		|| (obj->has_anti_flag(EAntiFlag::kMale) && IsMale(ch))
		|| (obj->has_anti_flag(EAntiFlag::kFemale) && IsFemale(ch))
		|| (obj->has_anti_flag(EAntiFlag::kSorcerer) && IS_SORCERER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kWarrior) && IS_WARRIOR(ch))
		|| (obj->has_anti_flag(EAntiFlag::kGuard) && IS_GUARD(ch))
		|| (obj->has_anti_flag(EAntiFlag::kThief) && IS_THIEF(ch))
		|| (obj->has_anti_flag(EAntiFlag::kAssasine) && IS_ASSASINE(ch))
		|| (obj->has_anti_flag(EAntiFlag::kPaladine) && IS_PALADINE(ch))
		|| (obj->has_anti_flag(EAntiFlag::kRanger) && IS_RANGER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kVigilant) && IS_VIGILANT(ch))
		|| (obj->has_anti_flag(EAntiFlag::kMerchant) && IS_MERCHANT(ch))
		|| (obj->has_anti_flag(EAntiFlag::kMagus) && IS_MAGUS(ch))
		|| (obj->has_anti_flag(EAntiFlag::kKiller) && ch->IsFlagged(EPlrFlag::kKiller))
		|| (obj->has_anti_flag(EAntiFlag::kBattle) && check_agrobd(ch))
		|| (obj->has_anti_flag(EAntiFlag::kColored) && pk_count(ch))) {
		return (true);
	}
	return (false);
}

int invalid_no_class(CharData *ch, const ObjData *obj) {
	if (obj->has_no_flag(ENoFlag::kCharmice)
		&& AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return true;
	}

	if (!IsCharmice(ch)
		&& (ch->IsNpc()
			|| privilege::IsImmortal(ch))) {
		return false;
	}

	if ((obj->has_no_flag(ENoFlag::kMono) && GET_RELIGION(ch) == kReligionMono)
		|| (obj->has_no_flag(ENoFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly)
		|| (obj->has_no_flag(ENoFlag::kMage) && IsMage(ch))
		|| (obj->has_no_flag(ENoFlag::kConjurer) && IS_CONJURER(ch))
		|| (obj->has_no_flag(ENoFlag::kCharmer) && IS_CHARMER(ch))
		|| (obj->has_no_flag(ENoFlag::kWizard) && IS_WIZARD(ch))
		|| (obj->has_no_flag(ENoFlag::kNecromancer) && IS_NECROMANCER(ch))
		|| (obj->has_no_flag(ENoFlag::kFighter) && IsFighter(ch))
		|| (obj->has_no_flag(ENoFlag::kMale) && IsMale(ch))
		|| (obj->has_no_flag(ENoFlag::kFemale) && IsFemale(ch))
		|| (obj->has_no_flag(ENoFlag::kSorcerer) && IS_SORCERER(ch))
		|| (obj->has_no_flag(ENoFlag::kWarrior) && IS_WARRIOR(ch))
		|| (obj->has_no_flag(ENoFlag::kGuard) && IS_GUARD(ch))
		|| (obj->has_no_flag(ENoFlag::kThief) && IS_THIEF(ch))
		|| (obj->has_no_flag(ENoFlag::kAssasine) && IS_ASSASINE(ch))
		|| (obj->has_no_flag(ENoFlag::kPaladine) && IS_PALADINE(ch))
		|| (obj->has_no_flag(ENoFlag::kRanger) && IS_RANGER(ch))
		|| (obj->has_no_flag(ENoFlag::kVigilant) && IS_VIGILANT(ch))
		|| (obj->has_no_flag(ENoFlag::kMerchant) && IS_MERCHANT(ch))
		|| (obj->has_no_flag(ENoFlag::kMagus) && IS_MAGUS(ch))
		|| (obj->has_no_flag(ENoFlag::kKiller) && ch->IsFlagged(EPlrFlag::kKiller))
		|| (obj->has_no_flag(ENoFlag::kBattle) && check_agrobd(ch))
		|| (!IS_VIGILANT(ch) && (obj->has_flag(EObjFlag::kSharpen) || obj->has_flag(EObjFlag::kArmored)))
		|| (obj->has_no_flag(ENoFlag::kColored) && pk_count(ch))) {
		return true;
	}

	return false;
}

int invalid_anti_class_proto(CharData *ch, const CObjectPrototype *obj) {
	if (obj->has_anti_flag(EAntiFlag::kCharmice)
		&& AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return (true);
	}
	if ((ch->IsNpc() || privilege::IsImmortal(ch)) && !IsCharmice(ch)) {
		return (false);
	}
	if ((obj->has_anti_flag(EAntiFlag::kNoPkClan) && char_to_pk_clan(ch))) {
		return (true);
	}
	if ((obj->has_anti_flag(EAntiFlag::kMono) && GET_RELIGION(ch) == kReligionMono)
		|| (obj->has_anti_flag(EAntiFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly)
		|| (obj->has_anti_flag(EAntiFlag::kMage) && IsMage(ch))
		|| (obj->has_anti_flag(EAntiFlag::kConjurer) && IS_CONJURER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kCharmer) && IS_CHARMER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kWizard) && IS_WIZARD(ch))
		|| (obj->has_anti_flag(EAntiFlag::kNecromancer) && IS_NECROMANCER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kFighter) && IsFighter(ch))
		|| (obj->has_anti_flag(EAntiFlag::kMale) && IsMale(ch))
		|| (obj->has_anti_flag(EAntiFlag::kFemale) && IsFemale(ch))
		|| (obj->has_anti_flag(EAntiFlag::kSorcerer) && IS_SORCERER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kWarrior) && IS_WARRIOR(ch))
		|| (obj->has_anti_flag(EAntiFlag::kGuard) && IS_GUARD(ch))
		|| (obj->has_anti_flag(EAntiFlag::kThief) && IS_THIEF(ch))
		|| (obj->has_anti_flag(EAntiFlag::kAssasine) && IS_ASSASINE(ch))
		|| (obj->has_anti_flag(EAntiFlag::kPaladine) && IS_PALADINE(ch))
		|| (obj->has_anti_flag(EAntiFlag::kRanger) && IS_RANGER(ch))
		|| (obj->has_anti_flag(EAntiFlag::kVigilant) && IS_VIGILANT(ch))
		|| (obj->has_anti_flag(EAntiFlag::kMerchant) && IS_MERCHANT(ch))
		|| (obj->has_anti_flag(EAntiFlag::kMagus) && IS_MAGUS(ch))
		|| (obj->has_anti_flag(EAntiFlag::kKiller) && ch->IsFlagged(EPlrFlag::kKiller))
		|| (obj->has_anti_flag(EAntiFlag::kBattle) && check_agrobd(ch))
		|| (obj->has_anti_flag(EAntiFlag::kColored) && pk_count(ch))) {
		return (true);
	}
	return (false);
}

int invalid_no_class_proto(CharData *ch, const CObjectPrototype *obj) {
	if (obj->has_no_flag(ENoFlag::kCharmice)
		&& AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return true;
	}
	if (!IsCharmice(ch)
		&& (ch->IsNpc()
			|| privilege::IsImmortal(ch))) {
		return false;
	}
	if ((obj->has_no_flag(ENoFlag::kMono) && GET_RELIGION(ch) == kReligionMono)
		|| (obj->has_no_flag(ENoFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly)
		|| (obj->has_no_flag(ENoFlag::kMage) && IsMage(ch))
		|| (obj->has_no_flag(ENoFlag::kConjurer) && IS_CONJURER(ch))
		|| (obj->has_no_flag(ENoFlag::kCharmer) && IS_CHARMER(ch))
		|| (obj->has_no_flag(ENoFlag::kWizard) && IS_WIZARD(ch))
		|| (obj->has_no_flag(ENoFlag::kNecromancer) && IS_NECROMANCER(ch))
		|| (obj->has_no_flag(ENoFlag::kFighter) && IsFighter(ch))
		|| (obj->has_no_flag(ENoFlag::kMale) && IsMale(ch))
		|| (obj->has_no_flag(ENoFlag::kFemale) && IsFemale(ch))
		|| (obj->has_no_flag(ENoFlag::kSorcerer) && IS_SORCERER(ch))
		|| (obj->has_no_flag(ENoFlag::kWarrior) && IS_WARRIOR(ch))
		|| (obj->has_no_flag(ENoFlag::kGuard) && IS_GUARD(ch))
		|| (obj->has_no_flag(ENoFlag::kThief) && IS_THIEF(ch))
		|| (obj->has_no_flag(ENoFlag::kAssasine) && IS_ASSASINE(ch))
		|| (obj->has_no_flag(ENoFlag::kPaladine) && IS_PALADINE(ch))
		|| (obj->has_no_flag(ENoFlag::kRanger) && IS_RANGER(ch))
		|| (obj->has_no_flag(ENoFlag::kVigilant) && IS_VIGILANT(ch))
		|| (obj->has_no_flag(ENoFlag::kMerchant) && IS_MERCHANT(ch))
		|| (obj->has_no_flag(ENoFlag::kMagus) && IS_MAGUS(ch))
		|| (obj->has_no_flag(ENoFlag::kKiller) && ch->IsFlagged(EPlrFlag::kKiller))
		|| (obj->has_no_flag(ENoFlag::kBattle) && check_agrobd(ch))
		|| (!IS_VIGILANT(ch) && (obj->has_flag(EObjFlag::kSharpen) || obj->has_flag(EObjFlag::kArmored)))
		|| (obj->has_no_flag(ENoFlag::kColored) && pk_count(ch))) {
		return true;
	}
	return false;
}

/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
#include "classes_spell_slots.h"
// (issue.runes-migrate) InitSpellLevels() retired in favour of the
// cfg_manager-driven rune_spells loader (cfg/mechanics/rune_spells.xml).
// The boot call moved to MUD::CfgManager().LoadCfg("rune_spells") at the
// same boot-step position; `reload runes` goes through ReloadCfg.

namespace {
// Целочисленный атрибут DataNode; def при отсутствии/некорректном значении.
int HandicapAttrInt(parser_wrapper::DataNode &node, const char *key, int def) {
	const char *v = node.GetValue(key);
	if (!v || !*v) {
		return def;
	}
	try {
		return parse::ReadAsInt(v);
	} catch (const std::exception &) {
		return def;
	}
}
} // namespace

/*
	Заполняет таблицу гандикапа группового опыта из <group_exp_handicap>:
	<remort val="N"> с детьми <class id="ECharClass" level_greater="L"/>.
	Реморты, для которых строк нет, копируются с максимального заданного реморта
	(в конфиге обычно ~75 строк, kMaxRemort больше).
*/
void GroupPenalties::Load(parser_wrapper::DataNode data) {
	// пре-инициализация всех классов всеми мортами в -1
	for (auto clss = ECharClass::kFirst; clss <= ECharClass::kLast; ++clss) {
		grouping_[clss].fill(-1);
	}

	int max_remort = -1;
	for (auto &remort_node : data.Children("remort")) {
		const int remort = HandicapAttrInt(remort_node, "val", -1);
		if (remort < 0 || remort > kMaxRemort) {
			log("group_exp_handicap: неверное число ремортов: %d (0..%d)", remort, kMaxRemort);
			continue;
		}
		for (auto &class_node : remort_node.Children("class")) {
			const char *id = class_node.GetValue("id");
			ECharClass clss;
			try {
				clss = parse::ReadAsConstant<ECharClass>(id);
			} catch (const std::exception &) {
				log("group_exp_handicap: неизвестный класс '%s' (реморт %d)", id ? id : "", remort);
				continue;
			}
			grouping_[clss][remort] = HandicapAttrInt(class_node, "level_greater", -1);
		}
		max_remort = std::max(max_remort, remort);
	}

	// копируем последний заданный реморт на все большие морты, для которых строк нет
	if (max_remort >= 0 && max_remort < kMaxRemort) {
		for (auto clss = ECharClass::kFirst; clss <= ECharClass::kLast; ++clss) {
			for (int r = max_remort + 1; r <= kMaxRemort; ++r) {
				grouping_[clss][r] = grouping_[clss][max_remort];
			}
		}
	}
	loaded_ = (max_remort >= 0);
}

// Прямая загрузка из файла (для тестов и автономного использования). 0 при успехе.
int GroupPenalties::init() {
	Load(parser_wrapper::DataNode(LIB_CFG "mechanics/group_exp_handicap.xml"));
	return loaded_ ? 0 : 1;
}

void GroupPenaltiesLoader::Load(parser_wrapper::DataNode data) {
	grouping.Load(std::move(data));
}

void GroupPenaltiesLoader::Reload(parser_wrapper::DataNode data) {
	grouping.Load(std::move(data));
}

/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */

// Function to return the exp required for each class/level

GroupPenalties grouping;    ///< TODO: get rid of this global variable.

bool IsMage(const CharData *ch) {
	static const std::set<ECharClass> magic_classes{
		ECharClass::kConjurer,
		ECharClass::kWizard,
		ECharClass::kCharmer,
		ECharClass::kNecromancer};

	return magic_classes.contains(ch->GetClass());
}

bool IsCaster(const CharData *ch) {
	static const std::set<ECharClass> caster_classes{
		ECharClass::kSorcerer,
		ECharClass::kConjurer,
		ECharClass::kWizard,
		ECharClass::kCharmer,
		ECharClass::kNecromancer,
		ECharClass::kMagus};

	return caster_classes.contains(ch->GetClass());
}

bool IsFighter(const CharData *ch) {
	static const std::set<ECharClass> fight_classes{
		ECharClass::kThief,
		ECharClass::kWarrior,
		ECharClass::kAssasine,
		ECharClass::kGuard,
		ECharClass::kPaladine,
		ECharClass::kRanger,
		ECharClass::kVigilant};

	return fight_classes.contains(ch->GetClass());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
