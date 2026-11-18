#ifndef CLASS_CONSTANTS_HPP_
#define CLASS_CONSTANTS_HPP_

/*
	Константы классов персонажей
*/

#include "gameplay/affects/affect_contants.h"
#include "utils/utils.h"

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
	kMob = 20, // не используется
	kNpcBase = 100,
	kNpcLast = kNpcBase
};

constexpr int kNumPlayerClasses = to_underlying(ECharClass::kLast) + 1;
ECharClass& operator++(ECharClass &c);

template<>
const std::string &NAME_BY_ITEM<ECharClass>(ECharClass item);
template<>
ECharClass ITEM_BY_NAME<ECharClass>(const std::string &name);

enum class EMobClass {
	kUndefined,
	kBoss,
	kTrash,
	kTank,
	kMelleeDmg,
	kArcher,
	kRogue,
	kMageDmg,
	kMageBuff,
	kHealer,
	kTotal
};

constexpr int MOB_ROLE_COUNT = static_cast<int>(EMobClass::kTotal) - 1;

// Роли моба лежат в битсете на MOB_ROLE_COUNT бит, и под kUndefined места в нём нет: бит 0 --
// это kBoss. Так их нумеруют и данные мира, и medit, и показ ролей (npc_role_types[0] = "босс").
// Значение самого перечисления на единицу больше номера бита, поэтому индекс берётся только
// через RoleBit: подстановка enum'а напрямую сдвигает проверку на одну роль
// (`get_role(EMobClass::kBoss)` спрашивал про "треш").
constexpr unsigned RoleBit(EMobClass role) {
	return static_cast<unsigned>(role) - 1;
}

EMobClass& operator++(EMobClass &c);

template<>
const std::string &NAME_BY_ITEM<EMobClass>(EMobClass item);
template<>
EMobClass ITEM_BY_NAME<EMobClass>(const std::string &name);

#endif // CLASS_CONSTANTS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
