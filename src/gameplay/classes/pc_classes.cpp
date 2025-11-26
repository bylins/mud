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

#include "gameplay/magic/magic_utils.h"
#include "engine/core/handler.h"
#include "gameplay/fight/pk.h"
#include "gameplay/statistics/top.h"
#include "gameplay/communication/offtop.h"
#include "engine/ui/color.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/named_stuff.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/magic/spells_info.h"
#include "engine/db/global_objects.h"

const int kExpImpl = 2000000000;

extern int siteok_everyone;
extern double exp_coefficients[];

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
	if (is_newbie || (GetRealRemort(ch) >= 9 && GetRealRemort(ch) % 3 == 0)) {
		ch->set_skill(ESkill::kHangovering, 10);
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
		case ECharClass::kMagus: ch->set_skill(ESkill::kSideAttack, 10);
			break;
		case ECharClass::kSorcerer: ch->set_skill(ESkill::kSideAttack, 50);
			break;
		case ECharClass::kThief:
		case ECharClass::kAssasine: ch->set_skill(ESkill::kSideAttack, 75);
			break;
		case ECharClass::kMerchant: ch->set_skill(ESkill::kSideAttack, 85);
			break;
		case ECharClass::kGuard:
		case ECharClass::kPaladine:
		case ECharClass::kWarrior:
		case ECharClass::kRanger:
			if (ch->GetSkill(ESkill::kRiding) == 0)
				ch->set_skill(ESkill::kRiding, 10);
			ch->set_skill(ESkill::kSideAttack, 95);
			break;
		case ECharClass::kVigilant: ch->set_skill(ESkill::kSideAttack, 95);
			break;
		default: break;
	}

	advance_level(ch);
	sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GetRealLevel(ch));
	mudlog(buf, BRF, kLvlImplementator, SYSLOG, true);

	ch->set_hit(ch->get_real_max_hit());
	ch->set_move(ch->get_real_max_move());

	GET_COND(ch, THIRST) = 0;
	GET_COND(ch, FULL) = 0;
	GET_COND(ch, DRUNK) = 0;
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
void levelup_events(CharData *ch) {
	if (offtop_system::kMinOfftopLvl == GetRealLevel(ch)
		&& !ch->get_disposable_flag(DIS_OFFTOP_MESSAGE)) {
		ch->SetFlag(EPrf::kOfftopMode);
		ch->set_disposable_flag(DIS_OFFTOP_MESSAGE);
		SendMsgToChar(ch,
					  "%sТеперь вы можете пользоваться каналом оффтоп ('справка оффтоп').%s\r\n",
					  kColorBoldGrn, kColorNrm);
	}
	if (EXCHANGE_MIN_CHAR_LEV == GetRealLevel(ch)
		&& !ch->get_disposable_flag(DIS_EXCHANGE_MESSAGE)) {
		// по умолчанию базар у всех включен, поэтому не спамим даже однократно
		if (GetRealRemort(ch) <= 0) {
			SendMsgToChar(ch,
						  "%sТеперь вы можете покупать и продавать вещи на базаре ('справка базар!').%s\r\n",
						  kColorBoldGrn, kColorNrm);
		}
		ch->set_disposable_flag(DIS_EXCHANGE_MESSAGE);
	}
}

void advance_level(CharData *ch) {
	int add_move = 0, i;

	switch (ch->GetClass()) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: add_move = 2;
			break;

		case ECharClass::kThief:
		case ECharClass::kAssasine:
		case ECharClass::kMerchant:
		case ECharClass::kWarrior:
		case ECharClass::kGuard:
		case ECharClass::kRanger:
		case ECharClass::kPaladine: [[fallthrough]];
		case ECharClass::kVigilant: add_move = number(ch->GetInbornDex()/6 + 1, ch->GetInbornDex()/5 + 1);
			break;
		default: break;
	}

	check_max_hp(ch);
	ch->set_max_move(ch->get_max_move() + std::max(1, add_move));

	SetInbornAndRaceFeats(ch);

	if (IS_IMMORTAL(ch)) {
		for (i = 0; i < 3; i++) {
			GET_COND(ch, i) = (char) -1;
		}
		ch->SetFlag(EPrf::kHolylight);
	}

	TopPlayer::Refresh(ch);
	levelup_events(ch);
	ch->save_char();
}

void decrease_level(CharData *ch) {
	int add_move = 0;

	switch (ch->GetClass()) {
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
		case ECharClass::kSorcerer: [[fallthrough]];
		case ECharClass::kMagus: add_move = 2;
			break;

		case ECharClass::kThief:
		case ECharClass::kAssasine:
		case ECharClass::kMerchant:
		case ECharClass::kWarrior:
		case ECharClass::kGuard:
		case ECharClass::kPaladine:
		case ECharClass::kRanger: [[fallthrough]];
		case ECharClass::kVigilant: add_move = ch->GetInbornDex() / 5 + 1;
			break;
		default: break;
	}

	check_max_hp(ch);
	ch->set_max_move(ch->get_max_move() - std::clamp(add_move, 1, ch->get_max_move()));

	GET_WIMP_LEV(ch) = std::clamp(GET_WIMP_LEV(ch), 0, ch->get_real_max_hit()/2);
	if (!IS_IMMORTAL(ch)) {
		ch->UnsetFlag(EPrf::kHolylight);
	}

	TopPlayer::Refresh(ch);
	ch->save_char();
}

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
		|| IS_IMMORTAL(ch)
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
	if ((ch->IsNpc() || IS_IMMORTAL(ch)) && !IS_CHARMICE(ch)) {
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
		|| (obj->has_anti_flag(EAntiFlag::kMale) && IS_MALE(ch))
		|| (obj->has_anti_flag(EAntiFlag::kFemale) && IS_FEMALE(ch))
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

	if (!IS_CHARMICE(ch)
		&& (ch->IsNpc()
			|| IS_IMMORTAL(ch))) {
		return false;
	}

	if ((obj->has_no_flag(ENoFlag::kMono) && GET_RELIGION(ch) == kReligionMono)
		|| (obj->has_no_flag(ENoFlag::kPoly) && GET_RELIGION(ch) == kReligionPoly)
		|| (obj->has_no_flag(ENoFlag::kMage) && IsMage(ch))
		|| (obj->has_no_flag(ENoFlag::kConjurer) && IS_CONJURER(ch))
		|| (obj->has_no_flag(ENoFlag::kCharmer) && IS_CHARMER(ch))
		|| (obj->has_no_flag(ENoFlag::kWIzard) && IS_WIZARD(ch))
		|| (obj->has_no_flag(ENoFlag::kNecromancer) && IS_NECROMANCER(ch))
		|| (obj->has_no_flag(ENoFlag::kFighter) && IsFighter(ch))
		|| (obj->has_no_flag(ENoFlag::kMale) && IS_MALE(ch))
		|| (obj->has_no_flag(ENoFlag::kFemale) && IS_FEMALE(ch))
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
void InitSpellLevels() {
	FILE *magic;
	char line1[256], line2[256], line3[256], name[256];
	int i[15];

	if (!(magic = fopen(LIB_MISC "runes.lst", "r"))) {
		log("Cann't open items list file...");
		graceful_exit(1);
	}

	auto sp_num{ESpell::kUndefined};;
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %s %s %d %d %d %d %d", line1, line2, line3, i, i + 1, i + 2, i + 3, i + 4) != 8) {
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
		if (*line2 != '*') {
			*(name + strlen(name) + 1) = '\0';
			*(name + strlen(name) + 0) = ' ';
			strcat(name, line2);
		}
		if ((sp_num = FixNameAndFindSpellId(name)) < ESpell::kFirst) {
			log("Spell '%s' not found...", name);
			graceful_exit(1);
		}
		size_t c = strlen(line3);
/*		if (!strn_cmp(line3, "potion", c)) {
			spell_create[sp_num].potion.items[0] = i[0];
			spell_create[sp_num].potion.items[1] = i[1];
			spell_create[sp_num].potion.items[2] = i[2];
			spell_create[sp_num].potion.rnumber = i[3];
			spell_create[sp_num].potion.min_caster_level = i[4];
		} else if (!strn_cmp(line3, "wand", c)) {
			spell_create[sp_num].wand.items[0] = i[0];
			spell_create[sp_num].wand.items[1] = i[1];
			spell_create[sp_num].wand.items[2] = i[2];
			spell_create[sp_num].wand.rnumber = i[3];
			spell_create[sp_num].wand.min_caster_level = i[4];
		} else if (!strn_cmp(line3, "scroll", c)) {
			spell_create[sp_num].scroll.items[0] = i[0];
			spell_create[sp_num].scroll.items[1] = i[1];
			spell_create[sp_num].scroll.items[2] = i[2];
			spell_create[sp_num].scroll.rnumber = i[3];
			spell_create[sp_num].scroll.min_caster_level = i[4];
		} else if (!strn_cmp(line3, "items", c)) {
			spell_create[sp_num].items.items[0] = i[0];
			spell_create[sp_num].items.items[1] = i[1];
			spell_create[sp_num].items.items[2] = i[2];
			spell_create[sp_num].items.rnumber = i[3];
			spell_create[sp_num].items.min_caster_level = i[4];
		} else 
*/
		if (!strn_cmp(line3, "runes", c)) {
			spell_create[sp_num].runes.items[0] = i[0];
			spell_create[sp_num].runes.items[1] = i[1];
			spell_create[sp_num].runes.items[2] = i[2];
			spell_create[sp_num].runes.rnumber = i[3];
			spell_create[sp_num].runes.min_caster_level = i[4];
		} else {
			log("Unknown items option : %s", line3);
			graceful_exit(1);
		}
	}
	fclose(magic);
}

void InitBasicValues() {
	FILE *magic;
	char line[256], name[kMaxInputLength];
	int i[10], c, j, mode = 0, *pointer;
	if (!(magic = fopen(LIB_MISC "basic.lst", "r"))) {
		log("Can't open basic values list file...");
		return;
	}
	while (get_line(magic, name)) {
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
		else if ((c = atoi(line)) > 0 && c <= 100 && mode > 0 && mode < 10) {
			int fields = 0;
			switch (mode) {
				case 1: pointer = (int *) &(int_app[c].spell_aknowlege);
					fields = sizeof(int_app[c]) / sizeof(int);
					break;

				case 2: pointer = (int *) &(cha_app[c].leadership);
					fields = sizeof(cha_app[c]) / sizeof(int);
					break;

				case 3: pointer = (int *) &(size_app[c].ac);
					fields = sizeof(size_app[c]) / sizeof(int);
					break;

				case 4: pointer = (int *) &(weapon_app[c].shocking);
					fields = sizeof(weapon_app[c]) / sizeof(int);
					break;

				default: pointer = nullptr;
			}

			if (pointer)    //log("Mode %d - %d = %d %d %d %d %d %d",mode,c,
			{
				//    *i, *(i+1), *(i+2), *(i+3), *(i+4), *(i+5));
				for (j = 0; j < fields; j++) {
					if (i[j] != 100000)    //log("[%d] %d <-> %d",j,*(pointer+j),*(i+j));
					{
						*(pointer + j) = *(i + j);
					}
				}
				//getchar();
			}
		}
	}
	fclose(magic);
}

/*
	Берет misc/grouping, первый столбик цифр считает номерами мортов,
	остальные столбики - значение макс. разрыва в уровнях для конкретного
	класса. На момент написания этого в конфиге присутствует 26 строк, макс.
	морт равен 50 - строки с мортами с 26 по 50 копируются с 25-мортовой строки.
*/
int GroupPenalties::init() {
	char buf[kMaxInputLength];
	int remorts = 0, rows_assigned = 0, levels = 0, pos = 0, max_rows = kMaxRemort + 1;

	// пре-инициализация
	//Строк в массиве должно быть на 1 больше, чем макс. морт
	//Столбцов в массиве должно быть ровно столько же, сколько есть классов
	for (auto clss = ECharClass::kFirst; clss <= ECharClass::kLast; ++clss) {
		for (remorts = 0; remorts < max_rows; remorts++) {
			grouping_[clss][remorts] = -1;
		}
	}

	FILE *f = fopen(LIB_MISC "grouping", "r");
	if (!f) {
		log("Невозможно открыть файл %s", LIB_MISC "grouping");
		return 1;
	}

	while (get_line(f, buf)) {
		//Строка пустая или строка-коммент
		if (!buf[0] || buf[0] == ';' || buf[0] == '\n') {
			continue;
		}
		auto clss{ECharClass::kUndefined};
		pos = 0;
		while (sscanf(&buf[pos], "%d", &levels) == 1) {
			while (buf[pos] == ' ' || buf[pos] == '\t') {
				++pos;
			}
			//Первый проход цикла по строке
			if (clss == ECharClass::kUndefined) {
				remorts = levels; //Номера строк
				if (grouping_[ECharClass::kFirst][remorts] != -1) {
					log("Ошибка при чтении файла %s: дублирование параметров для %d ремортов",
						LIB_MISC "grouping", remorts);
					return 2;
				}
				if (remorts > kMaxRemort || remorts < 0) {
					log("Ошибка при чтении файла %s: неверное значение количества ремортов: %d, "
						"должно быть в промежутке от 0 до %d",
						LIB_MISC "grouping", remorts, kMaxRemort);
					return 3;
				}
			} else {
				grouping_[clss][remorts] = levels;
			}
			++clss;
			while (buf[pos] != ' ' && buf[pos] != '\t' && buf[pos] != 0) {
				++pos;
			}
		}

		if (clss < ECharClass::kLast) {
			log("Ошибка при чтении файла %s: неверный формат строки '%s', должно быть %d "
				"целых чисел, прочитали %d",
				LIB_MISC "grouping", buf, to_underlying(ECharClass::kLast) + 2, to_underlying(clss) + 1);
			return 4;
		}
		++rows_assigned;
	}

	if (rows_assigned < max_rows) {
		//Берем свободную переменную
		//Копируем последнюю строку на все морты, для которых нет строк
		for (levels = remorts; levels < max_rows; levels++) {
			for (auto clss = ECharClass::kFirst; clss <= ECharClass::kLast; ++clss) {
				grouping_[clss][levels] = grouping_[clss][remorts];
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
long GetExpUntilNextLvl(CharData *ch, int level) {
	if (level > kLvlImplementator || level < 0) {
		log("SYSERR: Requesting exp for invalid level %d!", level);
		return 0;
	}

	/*
	 * Gods have exp close to EXP_MAX.  This statement should never have to
	 * changed, regardless of how many mortal or immortal levels exist.
	 */
	if (level > kLvlImmortal) {
		return kExpImpl - ((kLvlImplementator - level) * 1000);
	}

	// Exp required for normal mortals is below
	float exp_modifier;
	if (GetRealRemort(ch) < kMaxExpCoefficientsUsed) {
		exp_modifier = static_cast<float>(exp_coefficients[GetRealRemort(ch)]);
	} else {
		exp_modifier = static_cast<float>(exp_coefficients[kMaxExpCoefficientsUsed]);
	}

	switch (ch->GetClass()) {

		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
		case ECharClass::kNecromancer:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 2500);
				case 3: return int(exp_modifier * 5000);
				case 4: return int(exp_modifier * 9000);
				case 5: return int(exp_modifier * 17000);
				case 6: return int(exp_modifier * 27000);
				case 7: return int(exp_modifier * 47000);
				case 8: return int(exp_modifier * 77000);
				case 9: return int(exp_modifier * 127000);
				case 10: return int(exp_modifier * 197000);
				case 11: return int(exp_modifier * 297000);
				case 12: return int(exp_modifier * 427000);
				case 13: return int(exp_modifier * 587000);
				case 14: return int(exp_modifier * 817000);
				case 15: return int(exp_modifier * 1107000);
				case 16: return int(exp_modifier * 1447000);
				case 17: return int(exp_modifier * 1847000);
				case 18: return int(exp_modifier * 2310000);
				case 19: return int(exp_modifier * 2830000);
				case 20: return int(exp_modifier * 3580000);
				case 21: return int(exp_modifier * 4580000);
				case 22: return int(exp_modifier * 5830000);
				case 23: return int(exp_modifier * 7330000);
				case 24: return int(exp_modifier * 9080000);
				case 25: return int(exp_modifier * 11080000);
				case 26: return int(exp_modifier * 15000000);
				case 27: return int(exp_modifier * 22000000);
				case 28: return int(exp_modifier * 33000000);
				case 29: return int(exp_modifier * 47000000);
				case 30: return int(exp_modifier * 64000000);
					// add new levels here
				default: return int(exp_modifier * 94000000);
			}
			break;

		case ECharClass::kSorcerer:
		case ECharClass::kMagus:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 2500);
				case 3: return int(exp_modifier * 5000);
				case 4: return int(exp_modifier * 9000);
				case 5: return int(exp_modifier * 17000);
				case 6: return int(exp_modifier * 27000);
				case 7: return int(exp_modifier * 47000);
				case 8: return int(exp_modifier * 77000);
				case 9: return int(exp_modifier * 127000);
				case 10: return int(exp_modifier * 197000);
				case 11: return int(exp_modifier * 297000);
				case 12: return int(exp_modifier * 427000);
				case 13: return int(exp_modifier * 587000);
				case 14: return int(exp_modifier * 817000);
				case 15: return int(exp_modifier * 1107000);
				case 16: return int(exp_modifier * 1447000);
				case 17: return int(exp_modifier * 1847000);
				case 18: return int(exp_modifier * 2310000);
				case 19: return int(exp_modifier * 2830000);
				case 20: return int(exp_modifier * 3580000);
				case 21: return int(exp_modifier * 4580000);
				case 22: return int(exp_modifier * 5830000);
				case 23: return int(exp_modifier * 7330000);
				case 24: return int(exp_modifier * 9080000);
				case 25: return int(exp_modifier * 11080000);
				case 26: return int(exp_modifier * 15000000);
				case 27: return int(exp_modifier * 22000000);
				case 28: return int(exp_modifier * 33000000);
				case 29: return int(exp_modifier * 47000000);
				case 30: return int(exp_modifier * 64000000);
					// add new levels here
				default: return int(exp_modifier * 87000000);
			}
			break;

		case ECharClass::kThief:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 1000);
				case 3: return int(exp_modifier * 2000);
				case 4: return int(exp_modifier * 4000);
				case 5: return int(exp_modifier * 8000);
				case 6: return int(exp_modifier * 15000);
				case 7: return int(exp_modifier * 28000);
				case 8: return int(exp_modifier * 52000);
				case 9: return int(exp_modifier * 85000);
				case 10: return int(exp_modifier * 131000);
				case 11: return int(exp_modifier * 192000);
				case 12: return int(exp_modifier * 271000);
				case 13: return int(exp_modifier * 372000);
				case 14: return int(exp_modifier * 512000);
				case 15: return int(exp_modifier * 672000);
				case 16: return int(exp_modifier * 854000);
				case 17: return int(exp_modifier * 1047000);
				case 18: return int(exp_modifier * 1274000);
				case 19: return int(exp_modifier * 1534000);
				case 20: return int(exp_modifier * 1860000);
				case 21: return int(exp_modifier * 2520000);
				case 22: return int(exp_modifier * 3380000);
				case 23: return int(exp_modifier * 4374000);
				case 24: return int(exp_modifier * 5500000);
				case 25: return int(exp_modifier * 6827000);
				case 26: return int(exp_modifier * 8667000);
				case 27: return int(exp_modifier * 13334000);
				case 28: return int(exp_modifier * 20000000);
				case 29: return int(exp_modifier * 28667000);
				case 30: return int(exp_modifier * 40000000);
					// add new levels here
				default: return int(exp_modifier * 53000000);
			}
			break;

		case ECharClass::kAssasine:
		case ECharClass::kMerchant:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 1500);
				case 3: return int(exp_modifier * 3000);
				case 4: return int(exp_modifier * 6000);
				case 5: return int(exp_modifier * 12000);
				case 6: return int(exp_modifier * 22000);
				case 7: return int(exp_modifier * 42000);
				case 8: return int(exp_modifier * 77000);
				case 9: return int(exp_modifier * 127000);
				case 10: return int(exp_modifier * 197000);
				case 11: return int(exp_modifier * 287000);
				case 12: return int(exp_modifier * 407000);
				case 13: return int(exp_modifier * 557000);
				case 14: return int(exp_modifier * 767000);
				case 15: return int(exp_modifier * 1007000);
				case 16: return int(exp_modifier * 1280000);
				case 17: return int(exp_modifier * 1570000);
				case 18: return int(exp_modifier * 1910000);
				case 19: return int(exp_modifier * 2300000);
				case 20: return int(exp_modifier * 2790000);
				case 21: return int(exp_modifier * 3780000);
				case 22: return int(exp_modifier * 5070000);
				case 23: return int(exp_modifier * 6560000);
				case 24: return int(exp_modifier * 8250000);
				case 25: return int(exp_modifier * 10240000);
				case 26: return int(exp_modifier * 13000000);
				case 27: return int(exp_modifier * 20000000);
				case 28: return int(exp_modifier * 30000000);
				case 29: return int(exp_modifier * 43000000);
				case 30: return int(exp_modifier * 59000000);
					// add new levels here
				default: return int(exp_modifier * 79000000);
			}
			break;

		case ECharClass::kWarrior:
		case ECharClass::kGuard:
		case ECharClass::kPaladine:
		case ECharClass::kRanger:
		case ECharClass::kVigilant:
			switch (level) {
				case 0: return 0;
				case 1: return 1;
				case 2: return int(exp_modifier * 2000);
				case 3: return int(exp_modifier * 4000);
				case 4: return int(exp_modifier * 8000);
				case 5: return int(exp_modifier * 14000);
				case 6: return int(exp_modifier * 24000);
				case 7: return int(exp_modifier * 39000);
				case 8: return int(exp_modifier * 69000);
				case 9: return int(exp_modifier * 119000);
				case 10: return int(exp_modifier * 189000);
				case 11: return int(exp_modifier * 289000);
				case 12: return int(exp_modifier * 419000);
				case 13: return int(exp_modifier * 579000);
				case 14: return int(exp_modifier * 800000);
				case 15: return int(exp_modifier * 1070000);
				case 16: return int(exp_modifier * 1340000);
				case 17: return int(exp_modifier * 1660000);
				case 18: return int(exp_modifier * 2030000);
				case 19: return int(exp_modifier * 2450000);
				case 20: return int(exp_modifier * 2950000);
				case 21: return int(exp_modifier * 3950000);
				case 22: return int(exp_modifier * 5250000);
				case 23: return int(exp_modifier * 6750000);
				case 24: return int(exp_modifier * 8450000);
				case 25: return int(exp_modifier * 10350000);
				case 26: return int(exp_modifier * 14000000);
				case 27: return int(exp_modifier * 21000000);
				case 28: return int(exp_modifier * 31000000);
				case 29: return int(exp_modifier * 44000000);
				case 30: return int(exp_modifier * 64000000);
					// add new levels here
				default: return int(exp_modifier * 79000000);
			}
			break;
		default: break;
	}

	/*
	 * This statement should never be reached if the exp tables in this function
	 * are set up properly.  If you see exp of 123456 then the tables above are
	 * incomplete -- so, complete them!
	 */
	log("SYSERR: XP tables not set up correctly in class.c!");
	return 123456;
}

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
