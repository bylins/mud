#ifndef CLASS_CONSTANTS_HPP_
#define CLASS_CONSTANTS_HPP_

/*
	Константы классов персонажей
*/

#include "game_affects/affect_contants.h"

enum class ECharClass {
	kUndefined = -1,
	kSorcerer = 0,
	kConjurer,
	kThief,
	kWarrior,
	kAssasine,
	kGuard,
	kCharmer,
	kWizard,
	kNecromancer,
	kPaladine,
	kRanger,
	kVigilant,
	kMerchant,
	kMagus,
	kFirst = kSorcerer,
	kLast = kMagus, // Не забываем менять при изменении числа классов
	kMob = 20,
	kNpcBase = 100,
	kNpcLast = kNpcBase
};

constexpr int kNumPlayerClasses = to_underlying(ECharClass::kLast) + 1;
ECharClass& operator++(ECharClass &c);

template<>
const std::string &NAME_BY_ITEM<ECharClass>(ECharClass item);
template<>
ECharClass ITEM_BY_NAME<ECharClass>(const std::string &name);

#endif // CLASS_CONSTANTS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
