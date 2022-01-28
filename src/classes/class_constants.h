#ifndef __CLASS_CONSTANTS_HPP__
#define __CLASS_CONSTANTS_HPP__

/*
	Константы классов персонажей
*/

#include "affects/affect_contants.h"

enum ECharClass : int {
	CLASS_UNDEFINED = -1,
	CLASS_CLERIC = 0,
	CLASS_BATTLEMAGE,
	CLASS_THIEF,
	CLASS_WARRIOR,
	CLASS_ASSASINE,
	CLASS_GUARD,
	CLASS_CHARMMAGE,
	CLASS_DEFENDERMAGE,
	CLASS_NECROMANCER,
	CLASS_PALADINE,
	CLASS_RANGER,
	CLASS_SMITH,
	CLASS_MERCHANT,
	CLASS_DRUID,
	PLAYER_CLASS_FIRST = CLASS_CLERIC,
	PLAYER_CLASS_LAST = CLASS_DRUID, // Не забываем менять при изменении числа классов
	CLASS_MOB = 20,
	NPC_CLASS_BASE = 100,
	NPC_CLASS_LAST = 107
};

constexpr int NUM_PLAYER_CLASSES = PLAYER_CLASS_LAST + 1;

ECharClass& operator++(ECharClass &c);

#define MASK_BATTLEMAGE   (1 << CLASS_BATTLEMAGE)
#define MASK_CLERIC       (1 << CLASS_CLERIC)
#define MASK_THIEF        (1 << CLASS_THIEF)
#define MASK_WARRIOR      (1 << CLASS_WARRIOR)
#define MASK_ASSASINE     (1 << CLASS_ASSASINE)
#define MASK_GUARD        (1 << CLASS_GUARD)
#define MASK_DEFENDERMAGE (1 << CLASS_DEFENDERMAGE)
#define MASK_CHARMMAGE    (1 << CLASS_CHARMMAGE)
#define MASK_NECROMANCER  (1 << CLASS_NECROMANCER)
#define MASK_PALADINE     (1 << CLASS_PALADINE)
#define MASK_RANGER       (1 << CLASS_RANGER)
#define MASK_SMITH        (1 << CLASS_SMITH)
#define MASK_MERCHANT     (1 << CLASS_MERCHANT)
#define MASK_DRUID        (1 << CLASS_DRUID)

#define MASK_MAGES        (MASK_BATTLEMAGE | MASK_DEFENDERMAGE | MASK_CHARMMAGE | MASK_NECROMANCER)
#define MASK_CASTER       (MASK_BATTLEMAGE | MASK_DEFENDERMAGE | MASK_CHARMMAGE | MASK_NECROMANCER | MASK_CLERIC | MASK_DRUID)
#define MASK_FIGHTERS     (MASK_THIEF | MASK_WARRIOR | MASK_ASSASINE | MASK_GUARD | MASK_PALADINE | MASK_RANGER | MASK_SMITH)

struct extra_affects_type {
	EAffectFlag affect;
	bool set_or_clear;
};

struct class_app_type {
	using extra_affects_list_t = std::vector<extra_affects_type>;

	int unknown_weapon_fault;
	int koef_con;
	int base_con;
	int min_con;
	int max_con;

	const extra_affects_list_t *extra_affects;
};

extern class_app_type class_app[];

#endif // __CLASS_CONSTANTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
