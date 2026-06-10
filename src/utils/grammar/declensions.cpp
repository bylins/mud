/**
\file declensions.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Numeral-agreement (declension) of Russian nouns by count.
\detail issue.utils-cleaning: moved out of utils.cpp.
*/

#include "declensions.h"

#include <array>
#include <string>
#include <unordered_map>

using SomePads = std::array<std::string, 3>;
using SomePadsMap = std::unordered_map<EWhat, SomePads>;

const char *GetDeclensionInNumber(long amount, EWhat of_what) {
	static const SomePadsMap things_cases = {
		{EWhat::kDay, {"дней", "день", "дня"}},
		{EWhat::kHour, {"часов", "час", "часа"}},
		{EWhat::kYear, {"лет", "год", "года"}},
		{EWhat::kPoint, {"очков", "очко", "очка"}},
		{EWhat::kMinA, {"минут", "минута", "минуты"}},
		{EWhat::kMinU, {"минут", "минуту", "минуты"}},
		{EWhat::kMoneyA, {"кун", "куна", "куны"}},
		{EWhat::kMoneyU, {"кун", "куну", "куны"}},
		{EWhat::kThingA, {"штук", "штука", "штуки"}},
		{EWhat::kThingU, {"штук", "штуку", "штуки"}},
		{EWhat::kLvl, {"уровней", "уровень", "уровня"}},
		{EWhat::kMoveA, {"верст", "верста", "версты"}},
		{EWhat::kMoveU, {"верст", "версту", "версты"}},
		{EWhat::kOneA, {"единиц", "единица", "единицы"}},
		{EWhat::kOneU, {"единиц", "единицу", "единицы"}},
		{EWhat::kSec, {"секунд", "секунду", "секунды"}},
		{EWhat::kDegree, {"градусов", "градус", "градуса"}},
		{EWhat::kRow, {"строк", "строка", "строки"}},
		{EWhat::kObject, {"предметов", "предмет", "предмета"}},
		{EWhat::kObjU, {"предметов", "предмета", "предметов"}},
		{EWhat::kRemort, {"перевоплощений", "перевоплощение", "перевоплощения"}},
		{EWhat::kWeek, {"недель", "неделя", "недели"}},
		{EWhat::kMonth, {"месяцев", "месяц", "месяца"}},
		{EWhat::kWeekU, {"недель", "неделю", "недели"}},
		{EWhat::kGlory, {"славы", "слава", "славы"}},
		{EWhat::kGloryU, {"славы", "славу", "славы"}},
		{EWhat::kPeople, {"человек", "человек", "человека"}},
		{EWhat::kStr, {"силы", "сила", "силы"}},
		{EWhat::kGulp, {"глотков", "глоток", "глотка"}},
		{EWhat::kTorc, {"гривен", "гривна", "гривны"}},
		{EWhat::kGoldTorc, {"золотых", "золотая", "золотые"}},
		{EWhat::kSilverTorc, {"серебряных", "серебряная", "серебряные"}},
		{EWhat::kBronzeTorc, {"бронзовых", "бронзовая", "бронзовые"}},
		{EWhat::kTorcU, {"гривен", "гривну", "гривны"}},
		{EWhat::kGoldTorcU, {"золотых", "золотую", "золотые"}},
		{EWhat::kSilverTorcU, {"серебряных", "серебряную", "серебряные"}},
		{EWhat::kBronzeTorcU, {"бронзовых", "бронзовую", "бронзовые"}},
		{EWhat::kIceU, {"искристых снежинок", "искристую снежинку", "искристые снежинки"}},
		{EWhat::kNogataU, {"ногат", "ногату", "ногаты"}}
	};

	if (amount < 0) {
		amount = -amount;
	}

	if ((amount % 100 >= 11 && amount % 100 <= 14) || amount % 10 >= 5 || amount % 10 == 0) {
		return things_cases.at(of_what)[0].c_str();
	}

	if (amount % 10 == 1) {
		return things_cases.at(of_what)[1].c_str();
	} else {
		return things_cases.at(of_what)[2].c_str();
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
