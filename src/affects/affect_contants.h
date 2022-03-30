/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#ifndef BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_
#define BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

#include "structs/structs.h"

// Типы таймеров аффектов.
constexpr Bitvector kAfBattledec = 1 << 0;
constexpr Bitvector kAfDeadkeep = 1 << 1;
constexpr Bitvector kAfPulsedec = 1 << 2;
constexpr Bitvector kAfSameTime = 1 << 3; // тикает раз в две секунды или во время раунда в бою (чтобы не между раундами)

/**
 * Affect bits: used in char_data.char_specials.saved.affected_by //
 */
enum class EAffect : Bitvector {
	kBlind = 1u << 0,                    ///< (R) Char is blind
	kInvisible = 1u << 1,                ///< Char is invisible
	kDetectAlign = 1u << 2,                ///< Char is sensitive to align
	kDetectInvisible = 1u << 3,                ///< Char can see invis entities
	kDetectMagic = 1u << 4,                ///< Char is sensitive to magic
	kDetectLife = 1u << 5,                ///< Char can sense hidden life
	kWaterWalk = 1u << 6,                ///< Char can walk on water
	kSanctuary = 1u << 7,                ///< Char protected by sanct.
	kGroup = 1u << 8,                    ///< (R) Char is grouped
	kCurse = 1u << 9,                    ///< Char is cursed
	kInfravision = 1u << 10,                ///< Char can see in dark
	kPoisoned = 1u << 11,                    ///< (R) Char is poisoned
	kProtectedFromEvil = 1u << 12,            ///< Char protected from evil
	kProtectedFromGood = 1u << 13,            ///< Char protected from good
	kSleep = 1u << 14,                    ///< (R) Char magically asleep
	kNoTrack = 1u << 15,                    ///< Char can't be tracked
	kTethered = 1u << 16,                ///< Room for future expansion
	kBless = 1u << 17,                    ///< Room for future expansion
	kSneak = 1u << 18,                    ///< Char can move quietly
	kHide = 1u << 19,                    ///< Char is hidden
	kCourage = 1u << 20,                    ///< Room for future expansion
	kCharmed = 1u << 21,                    ///< Char is charmed
	kHold = 1u << 22,
	kFly = 1u << 23,
	kSilence = 1u << 24,
	kAwarness = 1u << 25,
	kBlink = 1u << 26,
	kHorse = 1u << 27,                    ///< NPC - is horse, PC - is horsed
	kNoFlee = 1u << 28,
	kSingleLight = 1u << 29,
	kHolyLight = kIntOne | (1u << 0),
	kHolyDark = kIntOne | (1u << 1),
	kDetectPoison = kIntOne | (1u << 2),
	kDrunked = kIntOne | (1u << 3),
	kAbstinent = kIntOne | (1u << 4),
	kStopRight = kIntOne | (1u << 5),
	kStopLeft = kIntOne | (1u << 6),
	kStopFight = kIntOne | (1u << 7),
	kHaemorrhage = kIntOne | (1u << 8),
	kDisguise = kIntOne | (1u << 9),
	kWaterBreath = kIntOne | (1u << 10),
	kSlow = kIntOne | (1u << 11),
	kHaste = kIntOne | (1u << 12),
	kShield = kIntOne | (1u << 13),
	kAirShield = kIntOne | (1u << 14),
	kFireShield = kIntOne | (1u << 15),
	kIceShield = kIntOne | (1u << 16),
	kMagicGlass = kIntOne | (1u << 17),
	kStairs = kIntOne | (1u << 18),
	kStoneHands = kIntOne | (1u << 19),
	kPrismaticAura = kIntOne | (1u << 20),
	kHelper = kIntOne | (1u << 21),
	kForcesOfEvil = kIntOne | (1u << 22),
	kAitAura = kIntOne | (1u << 23),
	kFireAura = kIntOne | (1u << 24),
	kIceAura = kIntOne | (1u << 25),
	kDeafness = kIntOne | (1u << 26),
	kCrying = kIntOne | (1u << 27),
	kPeaceful = kIntOne | (1u << 28),
	kMagicStopFight = kIntOne | (1u << 29),
	kBerserk = kIntTwo | (1u << 0),
	kLightWalk = kIntTwo | (1u << 1),
	kBrokenChains = kIntTwo | (1u << 2),
	kCloudOfArrows = kIntTwo | (1u << 3),
	kShadowCloak = kIntTwo | (1u << 4),
	kGlitterDust = kIntTwo | (1u << 5),
	kAffright = kIntTwo | (1u << 6),
	kScopolaPoison = kIntTwo | (1u << 7),
	kDaturaPoison = kIntTwo | (1u << 8),
	kSkillReduce = kIntTwo | (1u << 9),
	kNoBattleSwitch = kIntTwo | (1u << 10),
	kBelenaPoison = kIntTwo | (1u << 11),
	kNoTeleport = kIntTwo | (1u << 12),
	kLacky = kIntTwo | (1u << 13),
	kBandage = kIntTwo | (1u << 14),
	kCannotBeBandaged = kIntTwo | (1u << 15),
	kMorphing = kIntTwo | (1u << 16),
	kStrangled = kIntTwo | (1u << 17),
	kMemorizeSpells = kIntTwo | (1u << 18),
	kNoobRegen = kIntTwo | (1u << 19),
	kVampirism = kIntTwo | (1u << 20),
	kExpediend = kIntTwo | (1u << 21), // не используется, можно переименовать и использовать
	kCommander = kIntTwo | (1u << 22),
	kEarthAura = kIntTwo | (1u << 23),
};

template<>
const std::string &NAME_BY_ITEM<EAffect>(EAffect item);
template<>
EAffect ITEM_BY_NAME<EAffect>(const std::string &name);

typedef std::list<EAffect> affects_list_t;


enum class EWeaponAffect : Bitvector {
	kBlindness = (1 << 0),
	kInvisibility = (1 << 1),
	kDetectAlign = (1 << 2),
	kDetectInvisibility = (1 << 3),
	kDetectMagic = (1 << 4),
	kDetectLife = (1 << 5),
	kWaterWalk = (1 << 6),
	kSanctuary = (1 << 7),
	kCurse = (1 << 8),
	kInfravision = (1 << 9),
	kPoison = (1 << 10),
	kProtectedFromEvil = (1 << 11),
	kProtectedFromGood = (1 << 12),
	kSleep = (1 << 13),
	kNoTrack = (1 << 14),
	kBless = (1 << 15),
	kSneak = (1 << 16),
	kHide = (1 << 17),
	kHold = (1 << 18),
	kFly = (1 << 19),
	kSilence = (1 << 20),
	kAwareness = (1 << 21),
	kBlink = (1 << 22),
	kNoFlee = (1 << 23),
	kSingleLight = (1 << 24),
	kHolyLight = (1 << 25),
	kHolyDark = (1 << 26),
	kDetectPoison = (1 << 27),
	kSlow = (1 << 28),
	kHaste = (1 << 29),
	kWaterBreath = kIntOne | (1 << 0),
	kHaemorrhage = kIntOne | (1 << 1),
	kDisguising = kIntOne | (1 << 2),
	kShield = kIntOne | (1 << 3),
	kAirShield = kIntOne | (1 << 4),
	kFireShield = kIntOne | (1 << 5),
	kIceShield = kIntOne | (1 << 6),
	kMagicGlass = kIntOne | (1 << 7),
	kStoneHand = kIntOne | (1 << 8),
	kPrismaticAura = kIntOne | (1 << 9),
	kAirAura = kIntOne | (1 << 10),
	kFireAura = kIntOne | (1 << 11),
	kIceAura = kIntOne | (1 << 12),
	kDeafness = kIntOne | (1 << 13),
	kComamnder = kIntOne | (1 << 14),
	kEarthAura = kIntOne | (1 << 15),
// не забудьте поправить WAFF_COUNT
};

constexpr size_t kWeaponAffectCount = 47;

template<>
EWeaponAffect ITEM_BY_NAME<EWeaponAffect>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(EWeaponAffect item);

struct WeaponAffect {
	EWeaponAffect aff_pos;
	Bitvector aff_bitvector;
	int aff_spell;
};

// Modifier constants used with obj affects ('A' fields) //
// Applies используются как в предметах, так и в аффектах. Разумней разместить их тут, т.к. по сути
// applies на предметах - это урезанные аффекты.
enum EApplyLocation {
	APPLY_NONE = 0,    // No effect         //
	APPLY_STR = 1,    // Apply to strength    //
	APPLY_DEX = 2,    // Apply to dexterity      //
	APPLY_INT = 3,    // Apply to constitution   //
	APPLY_WIS = 4,    // Apply to wisdom      //
	APPLY_CON = 5,    // Apply to constitution   //
	APPLY_CHA = 6,    // Apply to charisma    //
	APPLY_CLASS = 7,    // Reserved       //
	APPLY_LEVEL = 8,    // Reserved       //
	APPLY_AGE = 9,    // Apply to age         //
	APPLY_CHAR_WEIGHT = 10,    // Apply to weight      //
	APPLY_CHAR_HEIGHT = 11,    // Apply to height      //
	APPLY_MANAREG = 12,    // Apply to max mana    //
	APPLY_HIT = 13,    // Apply to max hit points //
	APPLY_MOVE = 14,    // Apply to max move points   //
	APPLY_GOLD = 15,    // Reserved       //
	APPLY_EXP = 16,    // Reserved       //
	APPLY_AC = 17,    // Apply to Armor Class    //
	APPLY_HITROLL = 18,    // Apply to hitroll     //
	APPLY_DAMROLL = 19,    // Apply to damage roll    //
	APPLY_SAVING_WILL = 20,    // Apply to save throw: paralz   //
	APPLY_RESIST_FIRE = 21,    // Apply to RESIST throw: fire  //
	APPLY_RESIST_AIR = 22,    // Apply to RESIST throw: air   //
	APPLY_SAVING_CRITICAL = 23,    // Apply to save throw: breath   //
	APPLY_SAVING_STABILITY = 24,    // Apply to save throw: spells   //
	APPLY_HITREG = 25,
	APPLY_MOVEREG = 26,
	APPLY_C1 = 27,
	APPLY_C2 = 28,
	APPLY_C3 = 29,
	APPLY_C4 = 30,
	APPLY_C5 = 31,
	APPLY_C6 = 32,
	APPLY_C7 = 33,
	APPLY_C8 = 34,
	APPLY_C9 = 35,
	APPLY_SIZE = 36,
	APPLY_ARMOUR = 37,
	APPLY_POISON = 38,
	APPLY_SAVING_REFLEX = 39,
	APPLY_CAST_SUCCESS = 40,
	APPLY_MORALE = 41,
	APPLY_INITIATIVE = 42,
	APPLY_RELIGION = 43,
	APPLY_ABSORBE = 44,
	APPLY_LIKES = 45,
	APPLY_RESIST_WATER = 46,    // Apply to RESIST throw: water  //
	APPLY_RESIST_EARTH = 47,    // Apply to RESIST throw: earth  //
	APPLY_RESIST_VITALITY = 48,    // Apply to RESIST throw: light, dark, critical damage  //
	APPLY_RESIST_MIND = 49,    // Apply to RESIST throw: mind magic  //
	APPLY_RESIST_IMMUNITY = 50,    // Apply to RESIST throw: poison, disease etc.  //
	APPLY_AR = 51,    // Apply to Magic affect resist //
	APPLY_MR = 52,    // Apply to Magic damage resist //
	APPLY_ACONITUM_POISON = 53,
	APPLY_SCOPOLIA_POISON = 54,
	APPLY_BELENA_POISON = 55,
	APPLY_DATURA_POISON = 56,
	APPLY_FREE_FOR_USE = 57, // занимайте
	APPLY_BONUS_EXP = 58,
	APPLY_BONUS_SKILLS = 59,
	APPLY_PLAQUE = 60,
	APPLY_MADNESS = 61,
	APPLY_PR = 62,
	APPLY_RESIST_DARK = 63,
	APPLY_VIEW_DT = 64,
	APPLY_PERCENT_EXP = 65, //бонус +экспа
	APPLY_PERCENT_PHYSDAM = 66, // бонус + физповреждение
	APPLY_SPELL_BLINK = 67, // мигание заклом
	APPLY_PERCENT_MAGDAM = 68,
	NUM_APPLIES
};

template<>
const std::string &NAME_BY_ITEM<EApplyLocation>(EApplyLocation item);
template<>
EApplyLocation ITEM_BY_NAME<EApplyLocation>(const std::string &name);

using weapon_affect_t = std::array<WeaponAffect, kWeaponAffectCount>;
extern weapon_affect_t weapon_affect;
extern const char *affected_bits[];
extern const char *apply_types[];

#endif //BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
