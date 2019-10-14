#if !defined __FIGHT_CONSTANTS_HPP__
#define __FIGHT_CONSTANTS_HPP__

namespace FightSystem
{
	const int RIGHT_WEAPON = 1;
	const int LEFT_WEAPON = 2;

	const int ZERO_DMG = 0;

	enum DmgType { UNDEF_DMG, PHYS_DMG, MAGE_DMG };

	enum
	{
		type_hit, type_skin, type_whip, type_slash, type_bite,
		type_bludgeon, type_crush, type_pound, type_claw, type_maul,
		type_thrash, type_pierce, type_blast, type_punch, type_stab,
		type_pick, type_sting
	};

	enum
	{
		// игнор санки
		IGNORE_SANCT,
		// игнор призмы
		IGNORE_PRISM,
		// игнор брони
		IGNORE_ARMOR,
		// ополовинивание учитываемой брони
		HALF_IGNORE_ARMOR,
		// игнор поглощения
		IGNORE_ABSORBE,
		// от этого урона не нужно убегать (яд и т.п.)
		NO_FLEE_DMG,
		// крит удар
		CRIT_HIT,
		// игнор возврата дамаги от огненного щита
		IGNORE_FSHIELD,
		// дамаг идет от магического зеркала или звукового барьера
		MAGIC_REFLECT,
		// жертва имеет огненный щит
		VICTIM_FIRE_SHIELD,
		// жертва имеет воздушный щит
		VICTIM_AIR_SHIELD,
		// жертва имеет ледяной щит
		VICTIM_ICE_SHIELD,
		// было отражение от огненного щита в кратком режиме
		DRAW_BRIEF_FIRE_SHIELD,
		// было отражение от воздушного щита в кратком режиме
		DRAW_BRIEF_AIR_SHIELD,
		// было отражение от ледяного щита в кратком режиме
		DRAW_BRIEF_ICE_SHIELD,
		// была обратка от маг. зеркала
		DRAW_BRIEF_MAG_MIRROR,

		// кол-во флагов
		HIT_TYPE_FLAGS_NUM
	};
} // namespace FightSystem

#endif //__FIGHT_CONSTANTS_HPP__
