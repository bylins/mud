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

namespace grammar {

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
		{EWhat::kPeople, {"человек", "человек", "человека"}},
		{EWhat::kStr, {"силы", "сила", "силы"}},
		{EWhat::kGulp, {"глотков", "глоток", "глотка"}},
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

const char *CountedFormEnding(ECase gram_case, int slot) {
	static const char *kEndings[6][3] = {
		{"ая", "а", "а"},
		{"ой", "и", "ы"},
		{"ой", "е", "е"},
		{"ую", "у", "у"},
		{"ой", "ой", "ой"},
		{"ой", "е", "е"}
	};
	const int c = static_cast<int>(gram_case);
	if (c < 0 || c > 5 || slot < 0 || slot > 2) {
		return "";
	}
	return kEndings[c][slot];
}

const char *OneNumeralEnding(EGender gender, ECase gram_case) {
	using Cases = std::unordered_map<ECase, std::string>;
	using ByGender = std::unordered_map<EGender, Cases>;
	static const ByGender kOne {
		{EGender::kMale, {
			{ECase::kNom, "ин"}, {ECase::kGen, "ного"}, {ECase::kDat, "ному"},
			{ECase::kAcc, "нин"}, {ECase::kIns, "ним"}, {ECase::kPre, "ном"}}},
		{EGender::kFemale, {
			{ECase::kNom, "на"}, {ECase::kGen, "ной"}, {ECase::kDat, "ной"},
			{ECase::kAcc, "ну"}, {ECase::kIns, "ной"}, {ECase::kPre, "ной"}}},
		{EGender::kNeutral, {
			{ECase::kNom, "но"}, {ECase::kGen, "ного"}, {ECase::kDat, "ному"},
			{ECase::kAcc, "но"}, {ECase::kIns, "ним"}, {ECase::kPre, "ном"}}},
		{EGender::kPoly, {
			{ECase::kNom, "ни"}, {ECase::kGen, "них"}, {ECase::kDat, "ним"},
			{ECase::kAcc, "ни"}, {ECase::kIns, "ними"}, {ECase::kPre, "них"}}}
	};
	const auto g = kOne.find(gender);
	if (g == kOne.end()) {
		return "";
	}
	const auto it = g->second.find(gram_case);
	return it == g->second.end() ? "" : it->second.c_str();
}

}  // namespace grammar

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
