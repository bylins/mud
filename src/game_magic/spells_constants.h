/**
\authors Created by Sventovit
\date 27.04.2022.
\brief Константы системы заклинаний.
*/

#ifndef BYLINS_SRC_GAME_MAGIC_SPELLS_CONSTANTS_H_
#define BYLINS_SRC_GAME_MAGIC_SPELLS_CONSTANTS_H_

#include "structs/structs.h"
#include <optional>
const int kMinMemoryCircle = 1;
const int kMaxMemoryCircle = 13;

enum class ESpell {
	kUndefined = 0,
	kArmor = 1,
	kTeleport = 2,
	kBless = 3,
	kBlindness = 4,
	kBurningHands = 5,
	kCallLighting = 6,
	kCharm = 7,
	kChillTouch = 8,
	kClone = 9,
	kIceBolts = 10,
	kControlWeather = 11,
	kCreateFood = 12,
	kCreateWater = 13,
	kCureBlind = 14,
	kCureCritic = 15,
	kCureLight = 16,
	kCurse = 17,
	kDetectAlign = 18,
	kDetectInvis = 19,
	kDetectMagic = 20,
	kDetectPoison = 21,
	kDispelEvil = 22,
	kEarthquake = 23,
	kEnchantWeapon = 24,
	kEnergyDrain = 25,
	kFireball = 26,
	kHarm = 27,
	kHeal = 28,
	kInvisible = 29,
	kLightingBolt = 30,
	kLocateObject = 31,
	kMagicMissile = 32,
	kPoison = 33,
	kProtectFromEvil = 34,
	kRemoveCurse = 35,
	kSanctuary = 36,
	kShockingGasp = 37,
	kSleep = 38,
	kStrength = 39,
	kSummon = 40,
	kPatronage = 41,
	kWorldOfRecall = 42,
	kRemovePoison = 43,
	kSenseLife = 44,
	kAnimateDead = 45,
	kDispelGood = 46,
	kGroupArmor = 47,
	kGroupHeal = 48,
	kGroupRecall = 49,
	kInfravision = 50,
	kWaterwalk = 51,
	kCureSerious = 52,
	kGroupStrength = 53,
	kHold = 54,
	kPowerHold = 55,
	kMassHold = 56,
	kFly = 57,
	kBrokenChains = 58,
	kNoflee = 59,
	kCreateLight = 60,
	kDarkness = 61,
	kStoneSkin = 62,
	kCloudly = 63,
	kSilence = 64,
	kLight = 65,
	kChainLighting = 66,
	kFireBlast = 67,
	kGodsWrath = 68,
	kWeaknes = 69,
	kGroupInvisible = 70,
	kShadowCloak = 71,
	kAcid = 72,
	kRepair = 73,
	kEnlarge = 74,
	kFear = 75,
	kSacrifice = 76,
	kWeb = 77,
	kBlink = 78,
	kRemoveHold = 79,
	kCamouflage = 80,
	kPowerBlindness = 81,
	kMassBlindness = 82,
	kPowerSilence = 83,
	kExtraHits = 84,
	kResurrection = 85,
	kMagicShield = 86,
	kForbidden = 87,
	kMassSilence = 88,
	kRemoveSilence = 89,
	kDamageLight = 90,
	kDamageSerious = 91,
	kDamageCritic = 92,
	kMassCurse = 93,
	kArmageddon = 94,
	kGroupFly = 95,
	kGroupBless = 96,
	kResfresh = 97,
	kStunning = 98,
	kHide = 99,
	kSneak = 100,
	kDrunked = 101,
	kAbstinent = 102,
	kFullFeed = 103,
	kColdWind = 104,
	kBattle = 105,
	kHaemorrhage = 106,
	kCourage = 107,
	kWaterbreath = 108,
	kSlowdown = 109,
	kHaste = 110,
	kMassSlow = 111,
	kGroupHaste = 112,
	kGodsShield = 113,
	kFever = 114,
	kCureFever = 115,
	kAwareness = 116,
	kReligion = 117,
	kAirShield = 118,
	kPortal = 119,
	kDispellMagic = 120,
	kSummonKeeper = 121,
	kFastRegeneration = 122,
	kCreateWeapon = 123,
	kFireShield = 124,
	kRelocate = 125,
	kSummonFirekeeper = 126,
	kIceShield = 127,
	kIceStorm = 128,
	kLessening = 129,
	kShineFlash = 130,
	kMadness = 131,
	kGroupMagicGlass = 132,
	kCloudOfArrows = 133,
	kVacuum = 134,
	kMeteorStorm = 135,
	kStoneHands = 136,
	kMindless = 137,
	kPrismaticAura = 138,
	kEviless = 139,
	kAirAura = 140,
	kFireAura = 141,
	kIceAura = 142,
	kShock = 143,
	kMagicGlass = 144,
	kGroupSanctuary = 145,
	kGroupPrismaticAura = 146,
	kDeafness = 147,
	kPowerDeafness = 148,
	kRemoveDeafness = 149,
	kMassDeafness = 150,
	kDustStorm = 151,
	kEarthfall = 152,
	kSonicWave = 153,
	kHolystrike = 154,
	kSumonAngel = 155,
	kMassFear = 156,
	kFascination = 157,
	kCrying = 158,
	kOblivion = 159,
	kBurdenOfTime = 160,
	kGroupRefresh = 161,
	kPeaceful = 162,
	kMagicBattle = 163,
	kBerserk = 164,
	kStoneBones = 165,
	kRoomLight = 166,
	kPoosinedFog = 167,
	kThunderstorm = 168,
	kLightWalk = 169,
	kFailure = 170,
	kClanPray = 171,
	kGlitterDust = 172,
	kScream = 173,
	kCatGrace = 174,
	kBullBody = 175,
	kSnakeWisdom = 176,
	kGimmicry = 177,
	kWarcryOfChallenge = 178,
	kWarcryOfMenace = 179,
	kWarcryOfRage = 180,
	kWarcryOfMadness = 181,
	kWarcryOfThunder = 182,
	kWarcryOfDefence = 183,
	kWarcryOfBattle = 184,
	kWarcryOfPower = 185,
	kWarcryOfBless = 186,
	kWarcryOfCourage = 187,
	kRuneLabel = 188,
	kAconitumPoison = 189,
	kScopolaPoison = 190,
	kBelenaPoison = 191,
	kDaturaPoison = 192,
	kTimerRestore = 193,
	kLucky = 194,
	kBandage = 195,
	kNoBandage = 196,
	kCapable = 197,
	kStrangle = 198,
	kRecallSpells = 199,
	kHypnoticPattern = 200,
	kSolobonus = 201,
	kVampirism = 202,
	kRestoration = 203,
	kDeathAura = 204,
	kRecovery = 205,
	kMassRecovery = 206,
	kAuraOfEvil = 207,
	kMentalShadow = 208,
	kBlackTentacles = 209,
	kWhirlwind = 210,
	kIndriksTeeth = 211,
	kAcidArrow = 212,
	kThunderStone = 213,
	kClod = 214,
	kExpedient = 215,
	kSightOfDarkness = 216,
	kGroupSincerity = 217,
	kMagicalGaze = 218,
	kAllSeeingEye = 219,
	kEyeOfGods = 220,
	kBreathingAtDepth = 221,
	kGeneralRecovery = 222,
	kCommonMeal = 223,
	kStoneWall = 224,
	kSnakeEyes = 225,
	kEarthAura = 226,
	kGroupProtectFromEvil = 227,
	kArrowsFire = 228,
	kArrowsWater = 229,
	kArrowsEarth = 230,
	kArrowsAir = 231,
	kArrowsDeath = 232,
	kPaladineInspiration = 233,
	kDexterity = 234,
	kGroupBlink = 235,
	kGroupCloudly = 236,
	kGroupAwareness = 237,
	kWarcryOfExperience = 238,
	kWarcryOfLuck = 239,
	kWarcryOfPhysdamage = 240,
	kMassFailure = 241,
	kSnare = 242,
	kFireBreath = 243,
	kGasBreath = 244,
	kFrostBreath = 245,
	kAcidBreath = 246,
	kLightingBreath = 247,
	kExpedientFail = 248,
	kIdentify = 351,
	kFullIdentify = 352,
	kQUest = 353,
	kFirst = kArmor,
	kLast = kQUest	// Не забываем менять
};

const ESpell &operator++(ESpell &s);
std::ostream& operator<<(std::ostream &os, const ESpell &s);

template<>
ESpell ITEM_BY_NAME<ESpell>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESpell>(const ESpell spell);

enum class EElement {
	kUndefined = 0,
	kAir,
	kFire,
	kWater,
	kEarth,
	kLight,
	kDark,
	kMind,
	kLife
};

template<>
EElement ITEM_BY_NAME<EElement>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<EElement>(const EElement element);

// PLAYER SPELLS TYPES //
enum ESpellType {
	kUnknowm = 0,
	kKnow = 1 << 0,
	kTemp = 1 << 1,
	kPotionCast = 1 << 2,
	kWandCast = 1 << 3,
	kScrollCast = 1 << 4,
	kItemCast = 1 << 5,
	kRunes = 1 << 6
};

template<>
ESpellType ITEM_BY_NAME<ESpellType>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESpellType>(const ESpellType item);

enum EMagic : Bitvector {
	kMagDamage = 1 << 0,
	kMagAffects = 1 << 1,
	kMagUnaffects = 1 << 2,
	kMagPoints = 1 << 3,
	kMagAlterObjs = 1 << 4,
	kMagGroups = 1 << 5,
	kMagMasses = 1 << 6,
	kMagAreas = 1 << 7,
	kMagSummons = 1 << 8,
	kMagCreations = 1 << 9,
	kMagManual = 1 << 10,
	kMagWarcry = 1 << 11,
	kMagNeedControl = 1 << 12,
	kMagNone = 1 << 13,
// А чего это тут дырка Ж)
	kNpcDamagePc = 1 << 16,
	kNpcDamagePcMinhp = 1 << 17,
	kNpcAffectPc = 1 << 18,
	kNpcAffectPcCaster = 1 << 19,
	kNpcAffectNpc = 1 << 20,
	kNpcUnaffectNpc = 1 << 21,
	kNpcUnaffectNpcCaster = 1 << 22,
	kNpcDummy = 1 << 23,
	kMagRoom = 1 << 24,
	kMagCasterInroom = 1 << 25, // Аффект от этого спелла действует пока кастер в комнате //
	kMagCasterInworld = 1 << 26, // висит пока кастер в мире //
	kMagCasterAnywhere = 1 << 27, // висит пока не упадет сам //
	kMagCasterInworldDelay = 1 << 28 // висит пока кастер в мире, плюс таймер после ухода кастера//
};

template<>
EMagic ITEM_BY_NAME<EMagic>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<EMagic>(const EMagic item);

enum ETarget : Bitvector {
	kTarIgnore = 1 << 0,
	kTarCharRoom = 1 << 1,
	kTarCharWorld = 1 << 2,	// не ищет мобов при касте чарами (призвать/переместиться/переход)
	kTarFightSelf = 1 << 3,
	kTarFightVict = 1 << 4,
	kTarSelfOnly = 1 << 5,	// Only a check, use with i.e. TAR_CHAR_ROOM //
	kTarNotSelf = 1 << 6,	// Only a check, use with i.e. TAR_CHAR_ROOM //
	kTarObjInv = 1 << 7,
	kTarObjRoom = 1 << 8,
	kTarObjWorld = 1 << 9,
	kTarObjEquip = 1 << 10,
	kTarRoomThis = 1 << 11,	// Цель комната в которой сидит чар//
	kTarRoomDir = 1 << 12,	// Цель комната в каком-то направлении от чара//
	kTarRoomWorld = 1 << 13	// Цель какая-то комната в мире//
};

template<>
ETarget ITEM_BY_NAME<ETarget>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ETarget>(const ETarget item);

constexpr Bitvector kMtypeNeutral = 1 << 0;
constexpr Bitvector kMtypeAggressive = 1 << 1;

// текст заклинания для игроков, произнесенные заклинателями разных религий
struct CastPhraseList {
	std::string text_for_heathen;	// заклинание произнесенное язычником
	std::string text_for_christian;	// заклинание произнесенное христианиным
};

// возращает текст выводимый при спадении скила
std::string GetAffExpiredText(ESpell spell_id);
// возвращает фразу для заклинания согласно религии
// std::nullopt если соответсвующего заклинания в таблице нет
std::optional<CastPhraseList> GetCastPhrase(ESpell spell_id);

#endif //BYLINS_SRC_GAME_MAGIC_SPELLS_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
