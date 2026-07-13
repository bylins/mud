/**
\authors Created by Sventovit
\date 27.04.2022.
\brief Константы системы заклинаний.
*/

#ifndef BYLINS_SRC_GAME_MAGIC_SPELLS_CONSTANTS_H_
#define BYLINS_SRC_GAME_MAGIC_SPELLS_CONSTANTS_H_

#include "engine/structs/structs.h"
#include "gameplay/mechanics/condition.h"
#include "engine/structs/meta_enum.h"

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
	kEntangleEnemy = 59,
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
//	kDrunked = 101,	// UNUSED (issue.affect-migration): the "drunk" state is identified by its EAffect::kDrunked affect; af.type was redundant; value 101 retired.
//	kAbstinent = 102,	// UNUSED (issue.affect-migration): the "hangover" state is identified by its EAffect::kAbstinent affect; af.type was redundant; value 102 retired.
	kFullFeed = 103,
	kColdWind = 104,
//	kBattle = 105,	// UNUSED (issue.affect-migration): the generic "battle-applied affect" marker is gone; battle effects are identified by their EAffect affect_type + flags from affects.xml; value 105 retired.
//	kHaemorrhage = 106,	// UNUSED (issue.affect-migration): the bleeding state is identified by its EAffect::kHaemorrhage affect; af.type was redundant; value 106 retired.
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
//	kReligion = 117,	// UNUSED (issue.affect-migration): the pray/donate utility "spell" was removed;
//						// the prayer / sacrifice effects are now the affects kPrayerful / kPietas (value 117 retired).
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
//	kMagicBattle = 163,	// UNUSED (issue.affect-migration): the magic-stun marker is gone; the stun is identified by EAffect::kMagicStopFight; value 163 retired.
	kBerserk = 164,
	kStoneBones = 165,
	kRoomLight = 166,
	kDeadlyFog = 167,
	kThunderstorm = 168,
	kLightWalk = 169,
	kFailure = 170,
//	kClanPray = 171,	// UNUSED (issue.affect-migration): dead: no code references; value 171 retired.
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
	kCombatLuck = 194,
//	kBandage = 195,	// UNUSED (issue.affect-migration): the bandage state is identified by its EAffect::kBandage affect; af.type was redundant; value 195 retired.
//	kNoBandage = 196,	// UNUSED (issue.affect-migration): re-bandage already gated on AFF_FLAGGED(kCannotBeBandaged); value 196 retired.
//	kCapable = 197,	// UNUSED (issue.affect-migration): the "embedded spell" clone marker is the EAffect::kCapable affect; the kService spell was only an Affect::type marker; value 197 retired.
	kStrangle = 198,
//	kRecallSpells = 199,	// UNUSED (issue.affect-migration): the spell-recall buff is the EAffect::kMemorizeSpells affect; af.type was redundant; value 199 retired.
	kHypnoticPattern = 200,
//	kSolobonus = 201,	// UNUSED (issue.affect-migration): dead -- no apply site anywhere; the only consumers were two do_affects display checks for an affect nothing created; value 201 retired.
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
//	kExpedient = 215,	// UNUSED (issue.affect-migration): the chopoff Evasion buff is the EAffect::kEvade affect; af.type was redundant; value 215 retired.
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
//	kExpedientFail = 248,	// UNUSED (issue.affect-migration): was only an af.type marker, never queried; value 248 retired.
//	kLowerEffectiveness = 249,	// UNUSED (issue.affect-migration): disarm injure debuff identified by its kInjuredLimb affect; af.type was unused; value 249 retired.
//	kNoInjure = 250,	// UNUSED (issue.affect-migration): the disarm re-injure cooldown is now the
//						// kSuspiciousness affect (checked via IsAffectedWithCasterId); value 250 retired.
//	kConfuse = 251,	// UNUSED (issue.affect-migration): the shield-bash stun is the EAffect::kConfused affect; af.type was redundant; value 251 retired.
	// Internal proc spell: the per-hit bolt of the kCloudOfArrows affect. Cast via
	// CallMagic from fight_hit.cpp (no verbal, weave-only) -- not player-castable.
	kCloudOfArrowsBolt = 252,
	kIdentify = 351,
	kFullIdentify = 352,
//	kQUest = 353,	// UNUSED (issue.affect-migration): dead: no code references; value 353 retired.
//	kPortalTimer = 354,	// UNUSED (issue.affect-migration): the portal pair is identified by its ERoomAffect (kPortalTimer two-way / kNoPortalExit one-way); the room af.type was migrated off ESpell; value 354 retired.
//	kNoCharge = 355,	// UNUSED (issue.affect-migration): effect now checked via the kNoCharge affect (IsAffectedWithCasterId); value 355 retired.
	kDazzle = 356,
	kGreatHeal = 357,
	kFrenzy = 358,
	// Placeholder slots for spell prototyping in test mode (mode="kTesting"
	// in spells.xml). Pick the next free slot, write the config under that
	// id, and rename here when the spell graduates to its real name.
	kFireTrap = 359,	// issue.room-affect-trigger-improve: door-affect test spell (kTarDirection; was kTestOne)
	kTestTwo = 360,
	kTestThree = 361,
	kTestFour = 362,
	kTestFive = 363,
//	kDeadlyFogTick = 364,	// UNUSED (issue.affect-migration): the deadly-fog tick is now the kDeadlyFog room affect's own <actions> in room_affects.xml; value 364 retired.
	kCreateArmor = 365,  // issue.obj-casting: new data-driven <obj_creation> spell (plumbing)
	// issue.new-unaffect-spells: skill-based dispels (see spells.xml <unaffect>).
	kUnweave = 366,      // "unweave" -- peaceful: strip dispellable debuffs from a char
	kSuppress = 367,     // "suppress" -- strip the kAfBoon buff class from an enemy
	kErode = 368,        // "erode" -- strip the kAfWarding buff class from an enemy
	kAegisRift = 369,    // "aegis rift" -- strip the kAfAegis buff class from an enemy
	kToils = 370,        // "toils" -- strip fly/waterwalk/waterbreath from an enemy
	kCleanseArea = 371,  // "cleanse area" -- strip dispellable room affects
	kFirst = kArmor,
	kLast = 371	// Не забываем менять
};

const ESpell &operator++(ESpell &s);
std::ostream& operator<<(std::ostream &os, const ESpell &s);

template<>
ESpell ITEM_BY_NAME<ESpell>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESpell>(const ESpell spell);
template<>
const std::map<ESpell, std::string> &NAMES_OF<ESpell>();

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
template<>
const std::map<EElement, std::string> &NAMES_OF<EElement>();

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
template<>
const std::map<ESpellType, std::string> &NAMES_OF<ESpellType>();

enum EMagic : Bitvector {
	kMagDamage = 1 << 0,
	kMagAffects = 1 << 1,
	kMagUnaffects = 1 << 2,
	kMagPoints = 1 << 3,
	// 1 << 4 free (was kMagAlterObjs; now the data-driven <alter_obj> action)
	kMagGroups = 1 << 5,
	kMagMasses = 1 << 6,
	kMagAreas = 1 << 7,
	// 1 << 8 free (was kMagSummons; summon is now the data-driven <summon> action)
	// 1 << 9 free (was kMagCreations; now the data-driven <obj_creation> action)
	kMagManual = 1 << 10,
	kMagWarcry = 1 << 11,
	kMagNeedControl = 1 << 12,
	// Bit 13 used to be kMagCharRelocate; folded into kMagManual.
// А чего это тут дырка Ж)
	kNpcDamagePc = 1 << 16,
	kNpcDamagePcMinhp = 1 << 17,
	kNpcAffectPc = 1 << 18,
	kNpcAffectPcCaster = 1 << 19,
	kNpcAffectNpc = 1 << 20,
	kNpcUnaffectNpc = 1 << 21,
	kNpcUnaffectNpcCaster = 1 << 22,
	kNpcDummy = 1 << 23,
	// issue.mob-ai-improve: dispel an enemy PLAYER's buffs (offensive unaffect). The NPC
	// block (bits 16-23) is full, so this reuses free bit 13 (ex-kMagCharRelocate) and is
	// folded into NPC_CALCULATE below so mob_casting still treats it as an AI-castable spell.
	kNpcUnaffectPc = 1 << 13,
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
template<>
const std::map<EMagic, std::string> &NAMES_OF<EMagic>();

enum ETarget : Bitvector {
	kTarNone = 0,
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
	kTarRoomWorld = 1 << 13,	// Цель какая-то комната в мире//
	kTarAllyOnly = 1 << 14,	// Only a check: PC may target only self or a groupmate. Use with kTarCharRoom //
	kTarMinionsOnly = 1 << 15,	// Only a check: target must be one of the caster's own NPC followers (master == caster) //
	kTarDirection = 1 << 16	// issue.room-affect-trigger-improve: target a DIRECTION/exit from the caster (door affects). Arg = a RU/EN direction name; resolves to EXIT(caster, dir).
};

template<>
ETarget ITEM_BY_NAME<ETarget>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ETarget>(const ETarget item);
template<>
const std::map<ETarget, std::string> &NAMES_OF<ETarget>();

constexpr Bitvector kMtypeNeutral = 1 << 0;
constexpr Bitvector kMtypeAggressive = 1 << 1;

// возращает текст выводимый при спадении скила
std::string GetAffExpiredText(ESpell spell_id);


// Extra-attack bit flags (combat; stored in CharData::battle_affects).
enum EExtraAttackFlag : Bitvector {
	kEafParry = 1 << 0,
	kEafBlock = 1 << 1,
	kEafTouch = 1 << 2,
	// kEafProtect = 1 << 3,  // unused
	kEafDodge = 1 << 4,
	kEafHammer = 1 << 5,
	kEafOverwhelm = 1 << 6,
	kEafSlow = 1 << 7,
	kEafPunctual = 1 << 8,
	kEafAwake = 1 << 9,
	kEafFirst = 1 << 10,
	kEafSecond = 1 << 11,
	kEafStand = 1 << 13,
	kEafUsedright = 1 << 14,
	kEafUsedleft = 1 << 15,
	kEafMultyparry = 1 << 16,
	kEafSleep = 1 << 17,
	kEafIronWind = 1 << 18,
	kEafAutoblock = 1 << 19,	// автоматический блок щитом в осторожном стиле
	kEafPoisoned = 1 << 20,		// отравление с пушек раз в раунд
	kEafFirstPoison = 1 << 21,	// отравление цели первый раз за бой
	kEafInvisible = 1 << 22,	// одет автоинвиз
};

// Spell-flag mask: the "NPC casting calculation" bits -- the contiguous NPC block (16-23)
// plus the out-of-band kNpcUnaffectPc at bit 13 (issue.mob-ai-improve; the 16-23 block was
// full). Was `0xff << 16`.
constexpr Bitvector NPC_CALCULATE = (0xff << 16) | (1 << 13);

#endif //BYLINS_SRC_GAME_MAGIC_SPELLS_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
