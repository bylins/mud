#ifndef __CLASS_CONSTANTS_HPP__
#define __CLASS_CONSTANTS_HPP__

/*
	Константы классов персонажей
*/

#include "affects/affect_contants.h"

enum ECharClass : int {
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
	kNpcBase = 100, // От этого маразма надо избавиться
	kNpcLast = 107
};

constexpr int kNumPlayerClasses = ECharClass::kLast + 1;
ECharClass& operator++(ECharClass &c);

template<>
const std::string &NAME_BY_ITEM<ECharClass>(ECharClass item);
template<>
ECharClass ITEM_BY_NAME<ECharClass>(const std::string &name);

constexpr Bitvector kMaskSorcerer = 1 << ECharClass::kSorcerer;
constexpr Bitvector kMaskConjurer = 1 << ECharClass::kConjurer;
constexpr Bitvector kMaskThief = 1 << ECharClass::kThief;
constexpr Bitvector kMaskWarrior = 1 << ECharClass::kWarrior;
constexpr Bitvector kMaskAssasine = 1 << ECharClass::kAssasine;
constexpr Bitvector kMaskGuard = 1 << ECharClass::kGuard;
constexpr Bitvector kMaskWizard = 1 << ECharClass::kWizard;
constexpr Bitvector kMaskCharmer = 1 << ECharClass::kCharmer;
constexpr Bitvector kMaskNecromancer = 1 << ECharClass::kNecromancer;
constexpr Bitvector kMaskPaladine = 1 << ECharClass::kPaladine;
constexpr Bitvector kMaskRanger = 1 << ECharClass::kRanger;
constexpr Bitvector kMaskVigilant = 1 << ECharClass::kVigilant;
constexpr Bitvector kMaskMerchant = 1 << ECharClass::kMerchant;
constexpr Bitvector kMaskMagus = 1 << ECharClass::kMagus;

constexpr Bitvector kMaskMage = kMaskConjurer | kMaskWizard | kMaskCharmer | kMaskNecromancer;
constexpr Bitvector kMaskCaster = kMaskConjurer | kMaskWizard | kMaskCharmer | kMaskNecromancer | kMaskSorcerer | kMaskMagus;
constexpr Bitvector kMaskFighter = kMaskThief | kMaskWarrior | kMaskAssasine | kMaskGuard | kMaskPaladine | kMaskRanger | kMaskVigilant;

struct CLassExtraAffects {
	EAffectFlag affect;
	bool set_or_clear;
};

struct ClassApplies {
	using ExtraAffectsVector = std::vector<CLassExtraAffects>;

	int unknown_weapon_fault;
	int koef_con;
	int base_con;
	int min_con;
	int max_con;

	const ExtraAffectsVector *extra_affects;
};

extern ClassApplies class_app[];

#endif // __CLASS_CONSTANTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
