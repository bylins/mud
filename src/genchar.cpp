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

#include "genchar.h"

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "comm.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "game_magic/spells.h"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "db.h"
#include "structs/global_objects.h"

const char *genchar_help =
	"\r\n Сейчас вы должны выбрать себе характеристики. В зависимости от того, как\r\n"
	"вы их распределите, во многом будет зависеть жизнь вашего персонажа.\r\n"
	"В строке 'Можно добавить' вы видите число поинтов, которые можно распределить\r\n"
	"по характеристикам вашего персонажа. Уменьшая характеристику, вы добавляете\r\n"
	"туда один бал, увеличивая - убираете.\r\n"
	"Когда вы добьетесь, чтобы у вас в этой строке находилось число 0, вы сможете\r\n"
	"закончить генерацию и войти в игру.\r\n";

const char *default_race[] = {
	"Кривичи", //лекарь
	"Веляне", //колдун
	"Поляне", //вор
	"Северяне или Вятичи", //богатырь
	"Поляне", //наемник
	"Поляне или Вятичи", // дружинник
	"Веляне", // кудесник
	"Веляне", //волшебник
	"Веляне или Кривичи", //Чернокнижник
	"Поляне", //витязь
	"Поляне", //охотник
	"Северяне", //кузнец
	"Веляне или Поляне", // купец
	"Веляне" //волхв
};

int CalcBasseStatsSum(CharData *ch) {
	return ch->get_str() + ch->get_dex() + ch->get_int() + ch->get_wis() + ch->get_con() + ch->get_cha();
}

void genchar_disp_menu(CharData *ch) {
	char buf[kMaxStringLength];

	const auto &ch_class = MUD::Classes(ch->GetClass());
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
			"  П) Помощь (для доп. информации наберите 'справка характеристики')\r\n"
			"  О) &WАвтоматически сгенировать (рекомендуется для новичков)&n\r\n",
			ch->GetInbornStr(),
				ch_class.GetBaseStatGenMin(EBaseStat::kStr), ch_class.GetBaseStatGenMax(EBaseStat::kStr),
			ch->GetInbornDex(),
				ch_class.GetBaseStatGenMin(EBaseStat::kDex), ch_class.GetBaseStatGenMax(EBaseStat::kDex),
			ch->GetInbornInt(),
				ch_class.GetBaseStatGenMin(EBaseStat::kInt), ch_class.GetBaseStatGenMax(EBaseStat::kInt),
			ch->GetInbornWis(),
				ch_class.GetBaseStatGenMin(EBaseStat::kWis), ch_class.GetBaseStatGenMax(EBaseStat::kWis),
			ch->GetInbornCon(),
				ch_class.GetBaseStatGenMin(EBaseStat::kCon), ch_class.GetBaseStatGenMax(EBaseStat::kCon),
			ch->GetInbornCha(),
				ch_class.GetBaseStatGenMin(EBaseStat::kCha), ch_class.GetBaseStatGenMax(EBaseStat::kCha),
			kBaseStatsSum - CalcBasseStatsSum(ch));
	SendMsgToChar(buf, ch);
	if (kBaseStatsSum == CalcBasseStatsSum(ch))
		SendMsgToChar("  В) Закончить генерацию\r\n", ch);
	SendMsgToChar(" Ваш выбор: ", ch);
}

int genchar_parse(CharData *ch, char *arg) {
	const auto &ch_class = MUD::Classes(ch->GetClass());
	switch (*arg) {
		case 'А':
		case 'а': ch->set_str(std::max(ch->GetInbornStr() - 1, ch_class.GetBaseStatGenMin(EBaseStat::kStr)));
			break;
		case 'Б':
		case 'б': ch->set_dex(std::max(ch->GetInbornDex() - 1, ch_class.GetBaseStatGenMin(EBaseStat::kDex)));
			break;
		case 'Г':
		case 'г': ch->set_int(std::max(ch->GetInbornInt() - 1, ch_class.GetBaseStatGenMin(EBaseStat::kInt)));
			break;
		case 'Д':
		case 'д': ch->set_wis(std::max(ch->GetInbornWis() - 1, ch_class.GetBaseStatGenMin(EBaseStat::kWis)));
			break;
		case 'Е':
		case 'е': ch->set_con(std::max(ch->GetInbornCon() - 1, ch_class.GetBaseStatGenMin(EBaseStat::kCon)));
			break;
		case 'Ж':
		case 'ж': ch->set_cha(std::max(ch->GetInbornCha() - 1, ch_class.GetBaseStatGenMin(EBaseStat::kCha)));
			break;
		case 'З':
		case 'з': ch->set_str(std::min(ch->GetInbornStr() + 1, ch_class.GetBaseStatGenMax(EBaseStat::kStr)));
			break;
		case 'И':
		case 'и': ch->set_dex(std::min(ch->GetInbornDex() + 1, ch_class.GetBaseStatGenMax(EBaseStat::kDex)));
			break;
		case 'К':
		case 'к': ch->set_int(std::min(ch->GetInbornInt() + 1, ch_class.GetBaseStatGenMax(EBaseStat::kInt)));
			break;
		case 'Л':
		case 'л': ch->set_wis(std::min(ch->GetInbornWis() + 1, ch_class.GetBaseStatGenMax(EBaseStat::kWis)));
			break;
		case 'М':
		case 'м': ch->set_con(std::min(ch->GetInbornCon() + 1, ch_class.GetBaseStatGenMax(EBaseStat::kCon)));
			break;
		case 'Н':
		case 'н': ch->set_cha(std::min(ch->GetInbornCha() + 1, ch_class.GetBaseStatGenMax(EBaseStat::kCha)));
			break;
		case 'П':
		case 'п': SendMsgToChar(genchar_help, ch);
			break;
		case 'В':
		case 'в':
			if (CalcBasseStatsSum(ch) != kBaseStatsSum)
				break;
			// по случаю успешной генерации сохраняем стартовые статы
			ch->set_start_stat(G_STR, ch->GetInbornStr());
			ch->set_start_stat(G_DEX, ch->GetInbornDex());
			ch->set_start_stat(G_INT, ch->GetInbornInt());
			ch->set_start_stat(G_WIS, ch->GetInbornWis());
			ch->set_start_stat(G_CON, ch->GetInbornCon());
			ch->set_start_stat(G_CHA, ch->GetInbornCha());
			return kGencharExit;
		case 'О':
		case 'о': {
			const auto &tmp_class = MUD::Classes(ch->GetClass());
			ch->set_str(tmp_class.GetBaseStatGenAuto(EBaseStat::kStr));
			ch->set_dex(tmp_class.GetBaseStatGenAuto(EBaseStat::kDex));
			ch->set_int(tmp_class.GetBaseStatGenAuto(EBaseStat::kInt));
			ch->set_wis(tmp_class.GetBaseStatGenAuto(EBaseStat::kWis));
			ch->set_con(tmp_class.GetBaseStatGenAuto(EBaseStat::kCon));
			ch->set_cha(tmp_class.GetBaseStatGenAuto(EBaseStat::kCha));
			ch->set_start_stat(G_STR, ch->GetInbornStr());
			ch->set_start_stat(G_DEX, ch->GetInbornDex());
			ch->set_start_stat(G_INT, ch->GetInbornInt());
			ch->set_start_stat(G_WIS, ch->GetInbornWis());
			ch->set_start_stat(G_CON, ch->GetInbornCon());
			ch->set_start_stat(G_CHA, ch->GetInbornCha());
			return kGencharExit;
		}
		default: break;
	}
	return kGencharContinue;
}

/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(CharData *ch) {
	switch (ch->GetClass()) {
		case ECharClass::kSorcerer: ch->set_cha(10);
			do {
				ch->set_con(12 + number(0, 3));
				ch->set_wis(18 + number(0, 5));
				ch->set_int(18 + number(0, 5));
			}        // 57/48 roll 13/9
			while (ch->get_con() + ch->get_wis() + ch->get_int() != 57);
			do {
				ch->set_str(11 + number(0, 3));
				ch->set_dex(10 + number(0, 3));
			}        // 92/88 roll 6/4
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_wis(ch->get_wis() + 2);
			ch->set_int(ch->get_int() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
			break;

		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
			do {
				ch->set_str(10 + number(0, 4));
				ch->set_wis(17 + number(0, 5));
				ch->set_int(18 + number(0, 5));
			}        // 55/45 roll 14/10
			while (ch->get_str() + ch->get_wis() + ch->get_int() != 55);
			do {
				ch->set_con(10 + number(0, 3));
				ch->set_dex(9 + number(0, 3));
				ch->set_cha(13 + number(0, 3));
			}        // 92/87 roll 9/5
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_wis(ch->get_wis() + 1);
			ch->set_int(ch->get_int() + 2);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
			break;

		case ECharClass::kNecromancer:
			do {
				ch->set_cha(10 + number(0, 2));
				ch->set_wis(20 + number(0, 3));
				ch->set_int(18 + number(0, 5));
			}    // 58/48
			while (ch->get_cha() + ch->get_wis() + ch->get_int() != 55);
			do {
				ch->set_str(9 + number(0, 6));
				ch->set_dex(9 + number(0, 4));
				ch->set_con(11 + number(0, 2));
			}    // 96/84
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_con(ch->get_con() + 1);
			ch->set_int(ch->get_int() + 2);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
			break;

		case ECharClass::kThief:
			do {
				ch->set_str(16 + number(0, 3));
				ch->set_con(14 + number(0, 3));
				ch->set_dex(20 + number(0, 3));
			}    // 59/50
			while (ch->get_str() + ch->get_con() + ch->get_dex() != 57);
			do {
				ch->set_wis(9 + number(0, 3));
				ch->set_cha(13 + number(0, 2));
				ch->set_int(9 + number(0, 3));
			}    // 96/88
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_dex(ch->get_dex() + 2);
			ch->set_cha(ch->get_cha() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
			break;

		case ECharClass::kWarrior: ch->set_cha(10);
			do {
				ch->set_str(20 + number(0, 4));
				ch->set_dex(8 + number(0, 3));
				ch->set_con(20 + number(0, 3));
			}        // 55/48 roll 10/7
			while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
			do {
				ch->set_int(11 + number(0, 4));
				ch->set_wis(11 + number(0, 4));
			}        // 92/87 roll 8/5
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 1);
			ch->set_con(ch->get_con() + 2);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(165, 180) : number(170, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
			break;

		case ECharClass::kAssasine: ch->set_cha(12);
			do {
				ch->set_str(16 + number(0, 5));
				ch->set_dex(18 + number(0, 5));
				ch->set_con(14 + number(0, 2));
			}    // 60/48
			while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
			do {
				ch->set_int(11 + number(0, 3));
				ch->set_wis(11 + number(0, 3));
			}    // 95/89
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 1);
			ch->set_con(ch->get_con() + 1);
			ch->set_dex(ch->get_dex() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(150, 200);
			break;

		case ECharClass::kGuard: ch->set_cha(12);
			do {
				ch->set_str(19 + number(0, 3));
				ch->set_dex(13 + number(0, 3));
				ch->set_con(16 + number(0, 5));
			}        // 55/48 roll 11/7
			while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
			do {
				ch->set_int(10 + number(0, 4));
				ch->set_wis(10 + number(0, 4));
			}        // 92/87 roll 8/5
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 1);
			ch->set_dex(ch->get_dex() + 1);
			ch->set_con(ch->get_con() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 170) : number(160, 200);
			break;

		case ECharClass::kPaladine:
			do {
				ch->set_str(18 + number(0, 3));
				ch->set_wis(14 + number(0, 4));
				ch->set_con(14 + number(0, 4));
			}        // 53/46 roll 11/7
			while (ch->get_str() + ch->get_con() + ch->get_wis() != 53);
			do {
				ch->set_int(12 + number(0, 4));
				ch->set_dex(10 + number(0, 3));
				ch->set_cha(12 + number(0, 4));
			}        // 92/87 roll 11/5
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 1);
			ch->set_cha(ch->get_cha() + 1);
			ch->set_wis(ch->get_wis() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 175) : number(140, 190);
			break;

		case ECharClass::kRanger:

			do {
				ch->set_str(18 + number(0, 6));
				ch->set_dex(13 + number(0, 6));
				ch->set_con(14 + number(0, 4));
			}        // 53/46 roll 12/7
			while (ch->get_str() + ch->get_con() + ch->get_dex() != 53);
			do {
				ch->set_int(11 + number(0, 5));
				ch->set_wis(11 + number(0, 5));
				ch->set_cha(11 + number(0, 5));
			}        // 92/85 roll 10/7
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 1);
			ch->set_dex(ch->get_dex() + 2);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
			break;

		case ECharClass::kVigilant:
			do {
				ch->set_str(18 + number(0, 5));
				ch->set_dex(14 + number(0, 3));
				ch->set_con(14 + number(0, 6));
			}        // 53/46 roll 11/7
			while (ch->get_str() + ch->get_dex() + ch->get_con() != 55);
			do {
				ch->set_int(10 + number(0, 3));
				ch->set_wis(11 + number(0, 4));
				ch->set_cha(11 + number(0, 4));
			}        // 92/85 roll 11/7
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 2);
			ch->set_cha(ch->get_cha() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(170, 200);
			break;

		case ECharClass::kMerchant:
			do {
				ch->set_str(18 + number(0, 3));
				ch->set_con(12 + number(0, 6));
				ch->set_dex(14 + number(0, 3));
			}        // 55/48 roll 9/7
			while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
			do {
				ch->set_wis(10 + number(0, 3));
				ch->set_cha(12 + number(0, 4));
				ch->set_int(10 + number(0, 4));
			}        // 92/87 roll 9/5
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_con(ch->get_con() + 2);
			ch->set_cha(ch->get_cha() + 1);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 190);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
			break;

		case ECharClass::kMagus: ch->set_cha(12);
			do {
				ch->set_con(12 + number(0, 3));
				ch->set_wis(15 + number(0, 3));
				ch->set_int(17 + number(0, 5));
			}        // 53/45 roll 12/8
			while (ch->get_con() + ch->get_wis() + ch->get_int() != 53);
			do {
				ch->set_str(14 + number(0, 3));
				ch->set_dex(10 + number(0, 2));
			}        // 92/89 roll 5/3
			while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
			// ADD SPECIFIC STATS
			ch->set_str(ch->get_str() + 1);
			ch->set_int(ch->get_int() + 2);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
			for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kRunes;
			}
			break;

		default: log("SYSERROR : ATTEMPT STORE ABILITIES FOR UNKNOWN CLASS (Player %s)", GET_NAME(ch));
	};
}

// Функция для склонения имени по падежам.
// Буквы должны быть заранее переведены в нижний регистр.
// name - имя в именительном падеже
// sex - пол (kMale или kFemale)
// caseNum - номер падежа (0 - 5)
//  0 - именительный (кто? что?)
//  1 - родительный (кого? чего?)
//  2 - дательный (кому? чему?)
//  3 - винительный (кого? что?)
//  4 - творительный (кем? чем?)
//  5 - предложный (о ком? о чем?)
// result - результат
void GetCase(const char *name, const ESex sex, int caseNum, char *result) {
	size_t len = strlen(name);

	if (strchr("цкнгшщзхфвпрлджчсмтб", name[len - 1]) != nullptr
		&& sex == ESex::kMale) {
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
	} else if (name[len - 1] == 'я') {
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
	} else if (name[len - 1] == 'й'
		&& sex == ESex::kMale) {
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
			strcat(result, "и"); // Дрегвии
		else
			strcat(result, "й"); // Дрегвий
	} else if (name[len - 1] == 'а') {
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1) {
			if (strchr("шщжч", name[len - 2]) != nullptr)
				strcat(result, "и"); // Маши, Паши
			else
				strcat(result, "ы"); // Анны
		} else if (caseNum == 2)
			strcat(result, "е"); // Паше, Анне
		else if (caseNum == 3)
			strcat(result, "у"); // Пашу, Анну
		else if (caseNum == 4) {
			if (strchr("шщч", name[len - 2]) != nullptr)
				strcat(result, "ей"); // Машей, Пашей
			else
				strcat(result, "ой"); // Анной, Ханжой
		} else if (caseNum == 5)
			strcat(result, "е"); // Паше, Анне
		else
			strcat(result, "а"); // Паша, Анна
	} else {
		// остальные варианты либо не склоняются, либо редки (например, оканчиваются на ь)
		strcpy(result, name);
	}
	CAP(result);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
