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

// Weapon attack types (issue #3311). Underlying values match the former
// anonymous enum (type_hit==0 ... type_sting==16): used as the offset into
// attack_hit_text[] and as the (kTypeHit-based) key of fight messages.
enum class EAttackType {
	kUndefined = -1,
	kHit,		// was type_hit
	kSkin,		// was type_skin
	kWhip,		// was type_whip
	kSlash,		// was type_slash
	kBite,		// was type_bite
	kBludgeon,	// was type_bludgeon
	kCrush,		// was type_crush
	kPound,		// was type_pound
	kClaw,		// was type_claw
	kMaul,		// was type_maul
	kThrash,	// was type_thrash
	kPierce,	// was type_pierce
	kBlast,		// was type_blast
	kPunch,		// was type_punch
	kStab,		// was type_stab
	kPick,		// was type_pick
	kSting,		// was type_sting
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
	kIgnoreBlink,			// игнор мигания

	kHitFlagsNum			// кол-во флагов
};

} // namespace fight

#endif // FIGHT_CONSTANTS_HPP_
