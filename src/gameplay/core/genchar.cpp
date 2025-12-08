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

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"
#include "engine/core/comm.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "gameplay/magic/spells.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"

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

	const auto &ch_class = MUD::Class(ch->GetClass());
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
	const auto &ch_class = MUD::Class(ch->GetClass());
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
			const auto &tmp_class = MUD::Class(ch->GetClass());
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

void SetStartAbils(CharData *ch) {
	const auto &tmp_class = MUD::Class(ch->GetClass());
	ch->set_str(tmp_class.GetBaseStatGenMin(EBaseStat::kStr));
	ch->set_dex(tmp_class.GetBaseStatGenMin(EBaseStat::kDex));
	ch->set_con(tmp_class.GetBaseStatGenMin(EBaseStat::kCon));
	ch->set_int(tmp_class.GetBaseStatGenMin(EBaseStat::kInt));
	ch->set_wis(tmp_class.GetBaseStatGenMin(EBaseStat::kWis));
	ch->set_cha(tmp_class.GetBaseStatGenMin(EBaseStat::kCha));
	switch (ch->GetClass()) {
		case ECharClass::kSorcerer: ch->set_cha(10);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
			break;
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
			break;
		case ECharClass::kNecromancer:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
			break;
		case ECharClass::kThief:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
			break;
		case ECharClass::kWarrior: ch->set_cha(10);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(165, 180) : number(170, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
			break;
		case ECharClass::kAssasine: ch->set_cha(12);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(150, 200);
			break;
		case ECharClass::kGuard: ch->set_cha(12);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 170) : number(160, 200);
			break;
		case ECharClass::kPaladine:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 175) : number(140, 190);
			break;
		case ECharClass::kRanger:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
			break;
		case ECharClass::kVigilant:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(170, 200);
			break;
		case ECharClass::kMerchant:
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 190);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
			break;
		case ECharClass::kMagus: ch->set_cha(12);
			GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
			GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
			for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kRunes;
			}
			break;
		default: 
			log("SYSERROR : ATTEMPT STORE ABILITIES FOR UNKNOWN CLASS (Player %s)", GET_NAME(ch));
			break;
	}
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
void GetCase(std::string name, const EGender sex, int caseNum, char *data) {
	size_t len = name.size();
	std::string result = data;

	if (strchr("цкнгшщзхфвпрлджчсмтб", name[len - 1]) != nullptr
		&& sex == EGender::kMale) {
		result = name;
		if (caseNum == 1)
			result += "а"; // Ивана
		else if (caseNum == 2)
			result += "у"; // Ивану
		else if (caseNum == 3)
			result += "а"; // Ивана
		else if (caseNum == 4)
			result += "ом"; // Иваном, Ретичем
		else if (caseNum == 5)
			result += "е"; // Иване
	} else if (name[len - 1] == 'я') {
		result = name.substr(0, len - 1);
		if (caseNum == 1)
			result += "и"; // Ани, Вани
		else if (caseNum == 2)
			result += "е"; // Ане, Ване
		else if (caseNum == 3)
			result += "ю"; // Аню, Ваню
		else if (caseNum == 4)
			result += "ей"; // Аней, Ваней
		else if (caseNum == 5)
			result += "е"; // Ане, Ване
		else
			result += "я"; // Аня, Ваня
	} else if (name[len - 1] == 'й'
		&& sex == EGender::kMale) {
		result = name.substr(0, len - 1);
		if (caseNum == 1)
			result += "я"; // Дрегвия
		else if (caseNum == 2)
			result += "ю"; // Дрегвию
		else if (caseNum == 3)
			result += "я"; // Дрегвия
		else if (caseNum == 4)
			result += "ем"; // Дрегвием
		else if (caseNum == 5)
			result += "и"; // Дрегвии
		else
			result += "й"; // Дрегвий
	} else if (name[len - 1] == 'а') {
		result = name.substr(0, len - 1);
		if (caseNum == 1) {
			if (strchr("шщжч", name[len - 2]) != nullptr)
				result += "и"; // Маши, Паши
			else
				result += "ы"; // Анны
		} else if (caseNum == 2)
			result += "е"; // Паше, Анне
		else if (caseNum == 3)
			result += "у"; // Пашу, Анну
		else if (caseNum == 4) {
			if (strchr("шщч", name[len - 2]) != nullptr)
				result += "ей"; // Машей, Пашей
			else
				result += "ой"; // Анной, Ханжой
		} else if (caseNum == 5)
			result += "е"; // Паше, Анне
		else
			result += "а"; // Паша, Анна
	} else {
		// остальные варианты либо не склоняются, либо редки (например, оканчиваются на ь)
		result = name;
	}
	strcpy(data, result.c_str());
	utils::CAP(data);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
