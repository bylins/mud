/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#ifndef BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_
#define BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

#include "gameplay/magic/spells_constants.h"

// Константа, определяющая скорость таймера аффектов
const int kSecsPerPlayerAffect = 2;

// Типы таймеров аффектов.
constexpr Bitvector kAfBattledec = 1u << 0;
constexpr Bitvector kAfDeadkeep = 1u << 1;
constexpr Bitvector kAfPulsedec = 1u << 2;
constexpr Bitvector kAfSameTime = 1u << 3; // тикает раз в две секунды для PC, раз в минуту для NPC, или во время раунда в бою (чтобы не между раундами)

/**
 * Affect bits: used in char_data.char_specials.saved.affected_by //
 */
enum class EAffect : Bitvector {
	kUndefinded = 0u,
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
	kProtectFromDark = 1u << 12,            ///
	kProtectFromMind = 1u << 13,            ///
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
	kGodsShield = kIntOne | (1u << 13),
	kAirShield = kIntOne | (1u << 14),
	kFireShield = kIntOne | (1u << 15),
	kIceShield = kIntOne | (1u << 16),
	kMagicGlass = kIntOne | (1u << 17),
	kStairs = kIntOne | (1u << 18),
	kStoneHands = kIntOne | (1u << 19),
	kPrismaticAura = kIntOne | (1u << 20),
	kHelper = kIntOne | (1u << 21),
	kForcesOfEvil = kIntOne | (1u << 22),
	kAirAura = kIntOne | (1u << 23),
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
	kCombatLuck = kIntTwo | (1u << 13),
	kBandage = kIntTwo | (1u << 14),
	kCannotBeBandaged = kIntTwo | (1u << 15),
	kMorphing = kIntTwo | (1u << 16),
	kStrangled = kIntTwo | (1u << 17),
	kMemorizeSpells = kIntTwo | (1u << 18),
	kNoobRegen = kIntTwo | (1u << 19),
	kVampirism = kIntTwo | (1u << 20),
	kLacerations = kIntTwo | (1u << 21),
	kCommander = kIntTwo | (1u << 22),
	kEarthAura = kIntTwo | (1u << 23),
	kCloudly = kIntTwo | (1u << 24),
	kConfused = kIntTwo | (1u << 25),
	kNoCharge = kIntTwo | (1u << 26),
	kInjured = kIntTwo | (1u << 27),
	kFrenzy = kIntTwo | (1u << 28),
};

template<>
const std::string &NAME_BY_ITEM<EAffect>(EAffect item);
template<>
EAffect ITEM_BY_NAME<EAffect>(const std::string &name);

typedef std::list<EAffect> affects_list_t;

enum class EWeaponAffect : Bitvector {
	kBlindness = (1 << 0),			//0
	kInvisibility = (1 << 1),
	kDetectAlign = (1 << 2),
	kDetectInvisibility = (1 << 3),
	kDetectMagic = (1 << 4),
	kDetectLife = (1 << 5),
	kWaterWalk = (1 << 6),
	kSanctuary = (1 << 7),
	kCurse = (1 << 8),
	kInfravision = (1 << 9),
	kPoison = (1 << 10),			//10
	kProtectFromDark = (1 << 11),
	kProtectFromMind = (1 << 12),
	kSleep = (1 << 13),
	kNoTrack = (1 << 14),
	kBless = (1 << 15),
	kSneak = (1 << 16),
	kHide = (1 << 17),
	kHold = (1 << 18),
	kFly = (1 << 19),
	kSilence = (1 << 20),			//20
	kAwareness = (1 << 21),
	kBlink = (1 << 22),
	kNoFlee = (1 << 23),
	kSingleLight = (1 << 24),
	kHolyLight = (1 << 25),
	kHolyDark = (1 << 26),
	kDetectPoison = (1 << 27),
	kSlow = (1 << 28),
	kHaste = (1 << 29),
	kWaterBreath = kIntOne | (1 << 0),//30
	kHaemorrhage = kIntOne | (1 << 1),
	kDisguising = kIntOne | (1 << 2),
	kShield = kIntOne | (1 << 3),
	kAirShield = kIntOne | (1 << 4),
	kFireShield = kIntOne | (1 << 5),
	kIceShield = kIntOne | (1 << 6),
	kMagicGlass = kIntOne | (1 << 7),
	kStoneHand = kIntOne | (1 << 8),
	kPrismaticAura = kIntOne | (1 << 9),
	kAirAura = kIntOne | (1 << 10),		//40
	kFireAura = kIntOne | (1 << 11),
	kIceAura = kIntOne | (1 << 12),
	kDeafness = kIntOne | (1 << 13),
	kComamnder = kIntOne | (1 << 14),
	kEarthAura = kIntOne | (1 << 15),	//45
	kCloudly = kIntOne | (1 << 16)
// не забудьте поправить kWeaponAffectCount
};

constexpr size_t kWeaponAffectCount = 47;

template<>
EWeaponAffect ITEM_BY_NAME<EWeaponAffect>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(EWeaponAffect item);

struct WeaponAffect {
	EWeaponAffect aff_pos;
	Bitvector aff_bitvector;
	ESpell aff_spell;
};

// Applies используются как в предметах, так и в аффектах. Разумней разместить их тут, т.к. по сути
// applies на предметах - это урезанные аффекты.
/**
 * Modifier constants used with obj affects ('A' fields) and character affects
 */

enum EApply {
	kNone = 0,    // No effect         //
	kStr = 1,    // Apply to strength    //
	kDex = 2,    // Apply to dexterity      //
	kInt = 3,    // Apply to constitution   //
	kWis = 4,    // Apply to wisdom      //
	kCon = 5,    // Apply to constitution   //
	kCha = 6,    // Apply to charisma    //
	kClass = 7,    // Reserved       //
	kLvl = 8,    // Reserved       //
	kAge = 9,    // Apply to age         //
	kWeight = 10,    // Apply to weight      //
	kHeight = 11,    // Apply to height      //
	kManaRegen = 12,    // Apply to max mana    //
	kHp = 13,    // Apply to max hit points //
	kMove = 14,    // Apply to max move points   //
	kGold = 15,    // Reserved       //
	kExp = 16,    // ---- свободно. занимайте
	kAc = 17,    // Apply to Armor Class    //
	kHitroll = 18,    // Apply to hitroll     //
	kDamroll = 19,    // Apply to damage roll    //
	kSavingWill = 20,    // Apply to save throw: paralz   //
	kResistFire = 21,    // Apply to RESIST throw: fire  //
	kResistAir = 22,    // Apply to RESIST throw: air   //
	kSavingCritical = 23,    // Apply to save throw: breath   //
	kSavingStability = 24,    // Apply to save throw: spells   //
	kHpRegen = 25,
	kMoveRegen = 26,
	kFirstCircle = 27,
	kSecondCircle = 28,
	kThirdCircle = 29,
	kFourthCircle = 30,
	kFifthCircle = 31,
	kSixthCircle = 32,
	kSeventhCircle = 33,
	kEighthCircle = 34,
	kNinthCircle = 35,
	kSize = 36,
	kArmour = 37,
	kPoison = 38,
	kSavingReflex = 39,
	kCastSuccess = 40,
	kMorale = 41,
	kInitiative = 42,
	kReligion = 43,
	kAbsorbe = 44,
	kLikes = 45,
	kResistWater = 46,    // Apply to RESIST throw: water  //
	kResistEarth = 47,    // Apply to RESIST throw: earth  //
	kResistVitality = 48,    // Apply to RESIST throw: light, dark, critical damage  //
	kResistMind = 49,    // Apply to RESIST throw: mind magic  //
	kResistImmunity = 50,    // Apply to RESIST throw: poison, disease etc.  //
	kAffectResist = 51,    // Apply to Magic affect resist //
	kMagicResist = 52,    // Apply to Magic damage resist //
	kAconitumPoison = 53,
	kScopolaPoison = 54,
	kBelenaPoison = 55,
	kDaturaPoison = 56,
	kFreeForUse1 = 57, // занимайте
	kExpBonus = 58,
	kSkillsBonus = 59,
	kPlague = 60,
	kMadness = 61,
	kPhysicResist = 62,
	kResistDark = 63,
	kViewDeathTraps = 64,
	kExpPercent = 65, //бонус +экспа
	kPhysicDamagePercent = 66, // бонус + физповреждение
	kSpelledBlinkPhys = 67, // мигание от физурона
	kMagicDamagePercent = 68, //бонус + маг повреждение
	kSpelledBlinkMag = 69, //мигание от магурона
	kNumberApplies
};

template<>
const std::string &NAME_BY_ITEM<EApply>(EApply item);
template<>
EApply ITEM_BY_NAME<EApply>(const std::string &name);

using WeaponAffectArray = std::array<WeaponAffect, kWeaponAffectCount>;
extern WeaponAffectArray weapon_affect;
extern const char *affected_bits[];
extern const char *apply_types[];

#endif //BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
