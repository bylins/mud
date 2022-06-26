#if !defined FIGHT_CONSTANTS_HPP_
#define FIGHT_CONSTANTS_HPP_

namespace fight {
enum AttackType {
	kMainHand = 1,	//Основная атака
	kOffHand = 2,	//Доп.атака
	kMobAdd = 3		//Доп.атака моба
};

const int kZeroDmg = 0;
const int kLethalDmg = 11;

enum DmgType {
	kUndefDmg,	// для совместимости, надо убирать
	kPhysDmg,	// физический урон
	kMagicDmg,	// магический урон, дальше смотрим тип
	kPoisonDmg,	// чтобы использовать вместо UNDEF
	kPureDmg	// например, чистый урон SLOW DT
};

enum {
	type_hit,
	type_skin,
	type_whip,
	type_slash,
	type_bite,
	type_bludgeon,
	type_crush,
	type_pound,
	type_claw,
	type_maul,
	type_thrash,
	type_pierce,
	type_blast,
	type_punch,
	type_stab,
	type_pick,
	type_sting
};

enum {
	kIgnoreSanct,			// игнор санки
	kIgnorePrism,			// игнор призмы
	kIgnoreArmor,			// игнор брони
	kHalfIgnoreArmor,		// ополовинивание учитываемой брони
	kIgnoreAbsorbe,			// игнор поглощения
	kNoFleeDmg,				// от этого урона не нужно убегать (яд и т.п.)
	kCritHit,				// крит удар
	kCritLuck,				// крит удача
	kIgnoreFireShield,		// игнор возврата дамаги от огненного щита
	kMagicReflect,			// дамаг идет от магического зеркала или звукового барьера
	kVictimFireShield,		// жертва имеет огненный щит
	kVictimAirShield,		// жертва имеет воздушный щит
	kVictimIceShield,		// жертва имеет ледяной щит
	kDrawBriefFireShield,	// было отражение от огненного щита в кратком режиме
	kDrawBriefAirShield,	// было отражение от воздушного щита в кратком режиме
	kDrawBriefIceShield,	// было отражение от ледяного щита в кратком режиме
	kDrawBriefMagMirror,	// была обратка от маг. зеркала

	kHitFlagsNum			// кол-во флагов
};

} // namespace fight

#endif // FIGHT_CONSTANTS_HPP_
