/* ************************************************************************
*   File: genchar.cpp                                   Part of Bylins    *
*  Usage: functions for character generation                              *
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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "genchar.h"
#include "spells.h"

char *genchar_help =
    "\r\n Сейчас Вы должны выбрать себе характеристики. В зависимости от того, как\r\n"
    "вы их распределите, во многом будет зависеть жизнь Вашего персонажа.\r\n"
    "В строке 'Можно добавить' вы видите число поинтов, которые можно распредилить\r\n"
    "по характеристикам вашего персонажа. Уменьшая характеристику, вы добаляете\r\n"
    "туда один бал, увеличивая - убираете.\r\n"
    "Когда вы добьетесь, чтобы у Вас в этой строке находилось число 0, вы сможете\r\n"
    "закончить генерацию и войти в игру.\r\n";

int max_stats[][6] =
/* Str Dex Int Wis Con Cha */
{ {14, 13, 24, 25, 15, 10},	/* Лекарь */
  {14, 12, 25, 23, 13, 16},	/* Колдун */
  {19, 25, 12, 12, 17, 16},	/* Вор */
  {25, 11, 15, 15, 25, 10},	/* Богатырь */
  {22, 24, 14, 14, 17, 12},	/* Наемник */
  {23, 17, 14, 14, 23, 12},	/* Дружинник */
  {14, 12, 25, 23, 13, 16},	/* Кудесник */
  {14, 12, 25, 23, 13, 16},	/* Волшебник */
  {15, 13, 25, 23, 14, 12},	/* Чернокнижник */
  {22, 13, 16, 19, 18, 17},	/* Витязь */
  {25, 21, 16, 16, 18, 16},	/* Охотник */
  {25, 17, 13, 15, 20, 16},	/* Кузнец */
  {21, 17, 13, 13, 17, 16},	/* Купец */
  {18, 12, 24, 18, 15, 12}	/* Волхв */
};

int min_stats[][6] =
/* Str Dex Int Wis Con Cha */
{ {11, 10, 19, 20, 12, 10},	/* Лекарь */
  {10,  9, 20, 18, 10, 13},	/* Колдун */
  {16, 22,  9,  9, 14, 13},	/* Вор */
  {21,  8, 11, 11, 22, 10},	/* Богатырь */
  {17, 19, 11, 11, 14, 12},	/* Наемник */
  {20, 14, 10, 10, 17, 12},	/* Дружинник */
  {10,  9, 20, 18, 10, 13},	/* Кудесник */
  {10,  9, 20, 18, 10, 13},	/* Волшебник */
  { 9,  9, 20, 20, 11, 10},	/* Чернокнижник */
  {19, 10, 12, 15, 14, 13},	/* Витязь */
  {19, 15, 11, 11, 14, 11},	/* Охотник */
  {20, 14, 10, 11, 14, 12},	/* Кузнец */
  {18, 14, 10, 10, 14, 13},	/* Купец */
  {15, 10, 19, 15, 12, 12}	/* Волхв */
};

void genchar_disp_menu(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];

	sprintf(buf,
		"\r\n              -      +\r\n"
		"  Сила     : (А) %2d (З)        [%2d - %2d]\r\n"
		"  Ловкость : (Б) %2d (И)        [%2d - %2d]\r\n"
		"  Ум       : (Г) %2d (К)        [%2d - %2d]\r\n"
		"  Мудрость : (Д) %2d (Л)        [%2d - %2d]\r\n"
		"  Здоровье : (Е) %2d (М)        [%2d - %2d]\r\n"
		"  Обаяние  : (Ж) %2d (Н)        [%2d - %2d]\r\n"
		"\r\n"
		"  Можно добавить: %3d \r\n"
		"\r\n"
		"  П) Помощь\r\n",
		GET_STR(ch), MIN_STR(ch), MAX_STR(ch),
		GET_DEX(ch), MIN_DEX(ch), MAX_DEX(ch),
		GET_INT(ch), MIN_INT(ch), MAX_INT(ch),
		GET_WIS(ch), MIN_WIS(ch), MAX_WIS(ch),
		GET_CON(ch), MIN_CON(ch), MAX_CON(ch),
		GET_CHA(ch), MIN_CHA(ch), MAX_CHA(ch), SUM_ALL_STATS - SUM_STATS(ch));
	send_to_char(buf, ch);
	if (SUM_ALL_STATS == SUM_STATS(ch))
		send_to_char("  В) Закончить генерацию\r\n", ch);
	send_to_char(" Ваш выбор: ", ch);
}

int genchar_parse(CHAR_DATA * ch, char *arg)
{
	switch (*arg) {
	case 'А':
	case 'а':
		GET_STR(ch) = MAX(GET_STR(ch) - 1, MIN_STR(ch));
		break;
	case 'Б':
	case 'б':
		GET_DEX(ch) = MAX(GET_DEX(ch) - 1, MIN_DEX(ch));
		break;
	case 'Г':
	case 'г':
		GET_INT(ch) = MAX(GET_INT(ch) - 1, MIN_INT(ch));
		break;
	case 'Д':
	case 'д':
		GET_WIS(ch) = MAX(GET_WIS(ch) - 1, MIN_WIS(ch));
		break;
	case 'Е':
	case 'е':
		GET_CON(ch) = MAX(GET_CON(ch) - 1, MIN_CON(ch));
		break;
	case 'Ж':
	case 'ж':
		GET_CHA(ch) = MAX(GET_CHA(ch) - 1, MIN_CHA(ch));
		break;
	case 'З':
	case 'з':
		GET_STR(ch) = MIN(GET_STR(ch) + 1, MAX_STR(ch));
		break;
	case 'И':
	case 'и':
		GET_DEX(ch) = MIN(GET_DEX(ch) + 1, MAX_DEX(ch));
		break;
	case 'К':
	case 'к':
		GET_INT(ch) = MIN(GET_INT(ch) + 1, MAX_INT(ch));
		break;
	case 'Л':
	case 'л':
		GET_WIS(ch) = MIN(GET_WIS(ch) + 1, MAX_WIS(ch));
		break;
	case 'М':
	case 'м':
		GET_CON(ch) = MIN(GET_CON(ch) + 1, MAX_CON(ch));
		break;
	case 'Н':
	case 'н':
		GET_CHA(ch) = MIN(GET_CHA(ch) + 1, MAX_CHA(ch));
		break;
	case 'П':
	case 'п':
		send_to_char(genchar_help, ch);
		break;
	case 'В':
	case 'в':
		if (SUM_STATS(ch) != SUM_ALL_STATS)
			break;
		// по случаю успешной генерации сохраняем стартовые статы
		GET_START_STAT(ch, G_STR) = GET_STR(ch);
		GET_START_STAT(ch, G_DEX) = GET_DEX(ch);
		GET_START_STAT(ch, G_INT) = GET_INT(ch);
		GET_START_STAT(ch, G_WIS) = GET_WIS(ch);
		GET_START_STAT(ch, G_CON) = GET_CON(ch);
		GET_START_STAT(ch, G_CHA) = GET_CHA(ch);
		return GENCHAR_EXIT;
	default:
		break;
	}
	return GENCHAR_CONTINUE;
}

/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(CHAR_DATA * ch)
{
	int i;

	switch (ch->player.chclass) {
	case CLASS_CLERIC:
		ch->real_abils.cha = 10;
		do {
			ch->real_abils.con = 12 + number(0, 3);
			ch->real_abils.wis = 18 + number(0, 5);
			ch->real_abils.intel = 18 + number(0, 5);
		}		// 57/48 roll 13/9
		while (ch->real_abils.con + ch->real_abils.wis + ch->real_abils.intel != 57);
		do {
			ch->real_abils.str = 11 + number(0, 3);
			ch->real_abils.dex = 10 + number(0, 3);
		}		// 92/88 roll 6/4
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.wis += 2;
		ch->real_abils.intel += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		break;

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
		do {
			ch->real_abils.str = 10 + number(0, 4);
			ch->real_abils.wis = 17 + number(0, 5);
			ch->real_abils.intel = 18 + number(0, 5);
		}		// 55/45 roll 14/10
		while (ch->real_abils.str + ch->real_abils.wis + ch->real_abils.intel != 55);
		do {
			ch->real_abils.con = 10 + number(0, 3);
			ch->real_abils.dex = 9 + number(0, 3);
			ch->real_abils.cha = 13 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.wis += 1;
		ch->real_abils.intel += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
		break;

	case CLASS_NECROMANCER:
		do {
			ch->real_abils.cha = 10 + number(0, 2);
			ch->real_abils.wis = 20 + number(0, 3);
			ch->real_abils.intel = 18 + number(0, 5);
		}		// 55/45 roll 14/10
		while (ch->real_abils.cha + ch->real_abils.wis + ch->real_abils.intel != 55);
		do {
			ch->real_abils.str = 9 + number(0, 6);
			ch->real_abils.dex = 9 + number(0, 4);
			ch->real_abils.con = 11 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.con += 1;
		ch->real_abils.intel += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
		break;

	case CLASS_THIEF:
		do {
			ch->real_abils.str = 16 + number(0, 3);
			ch->real_abils.con = 14 + number(0, 3);
			ch->real_abils.dex = 20 + number(0, 3);
		}		// 57/50 roll 9/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 57);
		do {
			ch->real_abils.wis = 9 + number(0, 3);
			ch->real_abils.cha = 12 + number(0, 3);
			ch->real_abils.intel = 9 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.dex += 2;
		ch->real_abils.cha += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		break;

	case CLASS_WARRIOR:
		ch->real_abils.cha = 10;
		do {
			ch->real_abils.str = 20 + number(0, 4);
			ch->real_abils.dex = 8 + number(0, 3);
			ch->real_abils.con = 20 + number(0, 3);
		}		// 55/48 roll 10/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.intel = 11 + number(0, 4);
			ch->real_abils.wis = 11 + number(0, 4);
		}		// 92/87 roll 8/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(165, 180) : number(170, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
		break;

	case CLASS_ASSASINE:
		ch->real_abils.cha = 12;
		do {
			ch->real_abils.str = 16 + number(0, 5);
			ch->real_abils.dex = 18 + number(0, 5);
			ch->real_abils.con = 14 + number(0, 5);
		}		// 55/48 roll 15/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.intel = 11 + number(0, 3);
			ch->real_abils.wis = 11 + number(0, 3);
		}		// 92/87 roll 8/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 1;
		ch->real_abils.dex += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(150, 200);
		break;

	case CLASS_GUARD:
		ch->real_abils.cha = 12;
		do {
			ch->real_abils.str = 19 + number(0, 3);
			ch->real_abils.dex = 13 + number(0, 3);
			ch->real_abils.con = 16 + number(0, 5);
		}		// 55/48 roll 11/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.intel = 10 + number(0, 4);
			ch->real_abils.wis = 10 + number(0, 4);
		}		// 92/87 roll 8/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 1;
		ch->real_abils.dex += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 170) : number(160, 200);
		break;

	case CLASS_PALADINE:
		do {
			ch->real_abils.str = 18 + number(0, 3);
			ch->real_abils.wis = 14 + number(0, 4);
			ch->real_abils.con = 14 + number(0, 4);
		}		// 53/46 roll 11/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.wis != 53);
		do {
			ch->real_abils.intel = 12 + number(0, 4);
			ch->real_abils.dex = 10 + number(0, 3);
			ch->real_abils.cha = 12 + number(0, 4);
		}		// 92/87 roll 11/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.cha += 1;
		ch->real_abils.wis += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 175) : number(140, 190);
		break;

	case CLASS_RANGER:

		do {
			ch->real_abils.str = 18 + number(0, 6);
			ch->real_abils.dex = 14 + number(0, 6);
			ch->real_abils.con = 16 + number(0, 3);
		}		// 53/46 roll 12/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 53);
		do {
			ch->real_abils.intel = 11 + number(0, 5);
			ch->real_abils.wis = 11 + number(0, 5);
			ch->real_abils.cha = 11 + number(0, 5);
		}		// 92/85 roll 10/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 1;
		ch->real_abils.dex += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
		break;

	case CLASS_SMITH:
		do {
			ch->real_abils.str = 18 + number(0, 5);
			ch->real_abils.dex = 14 + number(0, 3);
			ch->real_abils.con = 14 + number(0, 3);
		}		// 53/46 roll 11/7
		while (ch->real_abils.str + ch->real_abils.dex + ch->real_abils.con != 53);
		do {
			ch->real_abils.intel = 10 + number(0, 3);
			ch->real_abils.wis = 11 + number(0, 4);
			ch->real_abils.cha = 11 + number(0, 4);
		}		// 92/85 roll 11/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 2;
		ch->real_abils.cha += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(170, 200);
		break;

	case CLASS_MERCHANT:
		do {
			ch->real_abils.str = 18 + number(0, 3);
			ch->real_abils.con = 16 + number(0, 3);
			ch->real_abils.dex = 14 + number(0, 3);
		}		// 55/48 roll 9/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.wis = 10 + number(0, 3);
			ch->real_abils.cha = 12 + number(0, 3);
			ch->real_abils.intel = 10 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.con += 2;
		ch->real_abils.cha += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
		break;

	case CLASS_DRUID:
		ch->real_abils.cha = 12;
		do {
			ch->real_abils.con = 13 + number(0, 4);
			ch->real_abils.wis = 15 + number(0, 3);
			ch->real_abils.intel = 17 + number(0, 5);
		}		// 53/45 roll 12/8
		while (ch->real_abils.con + ch->real_abils.wis + ch->real_abils.intel != 53);
		do {
			ch->real_abils.str = 14 + number(0, 3);
			ch->real_abils.dex = 10 + number(0, 2);
		}		// 92/89 roll 5/3
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.intel += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		for (i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(ch, i) = SPELL_RUNES;
		break;

	default:
		log("SYSERROR : ATTEMPT STORE ABILITIES FOR UNKNOWN CLASS (Player %s)", GET_NAME(ch));
	};
}

// Функция для склонения имени по падежам.
// Буквы должны быть заранее переведены в нижний регистр.
// name - имя в именительном падеже
// sex - пол (SEX_MALE или SEX_FEMALE)
// caseNum - номер падежа (0 - 5)
//  0 - именительный (кто? что?)
//  1 - родительный (кого? чего?)
//  2 - дательный (кому? чему?)
//  3 - винительный (кого? что?)
//  4 - творительный (кем? чем?)
//  5 - предложный (о ком? о чем?)
// result - результат
void GetCase(char *name, int sex, int caseNum, char *result)
{
	int len = strlen(name);
	name[0] = UPPER(name[0]);

	if (strchr("цкнгшщзхфвпрлджчсмтб", name[len - 1]) != NULL && sex == SEX_MALE)
	{
		strcpy(result, name);
		if (caseNum == 1)
			strcat(result, "а"); // Ивана
		else if (caseNum == 2)
			strcat(result, "у"); // Ивану
		else if (caseNum == 3)
			strcat(result, "а"); // Ивана
		else if (caseNum == 4)
			strcat(result, "ом"); // Иваном, Ретичем
		else if (caseNum == 5)
			strcat(result, "е"); // Иване
		return;
	}

	if (name[len - 1] == 'я')
	{
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1)
			strcat(result, "и"); // Ани, Вани
		else if (caseNum == 2)
			strcat(result, "е"); // Ане, Ване
		else if (caseNum == 3)
			strcat(result, "ю"); // Аню, Ваню
		else if (caseNum == 4)
			strcat(result, "ей"); // Аней, Ваней
		else if (caseNum == 5)
			strcat(result, "е"); // Ане, Ване
		else
			strcat(result, "я"); // Аня, Ваня
		return;
	}

	if (name[len - 1] == 'й' && sex == SEX_MALE)
	{
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1)
			strcat(result, "я"); // Дрегвия
		else if (caseNum == 2)
			strcat(result, "ю"); // Дрегвию
		else if (caseNum == 3)
			strcat(result, "я"); // Дрегвия
		else if (caseNum == 4)
			strcat(result, "ем"); // Дрегвием
		else if (caseNum == 5)
			strcat(result, "е"); // Дрегвие
		else
			strcat(result, "й"); // Дрегвий
		return;
	}

	if (name[len - 1] == 'а')
	{
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1)
		{
			if (strchr("шщжч", name[len - 2]) != NULL)
				strcat(result, "и"); // Маши, Паши
			else
				strcat(result, "ы"); // Анны
		}
		else if (caseNum == 2)
			strcat(result, "е"); // Паше, Анне
		else if (caseNum == 3)
			strcat(result, "у"); // Пашу, Анну
		else if (caseNum == 4)
		{
			if (strchr("шщч", name[len - 2]) != NULL)
				strcat(result, "ей"); // Машей, Пашей
			else
				strcat(result, "ой"); // Анной, Ханжой
		}
		else if (caseNum == 5)
			strcat(result, "е"); // Паше, Анне
		else
			strcat(result, "а"); // Паша, Анна
		return;
	}

	// остальные варианты либо не склоняются, либо редки (например, оканчиваются на ь)
	strcpy(result, name);
	return;
}
