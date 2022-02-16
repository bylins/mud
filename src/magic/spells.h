/* ************************************************************************
*   File: spells.h                                      Part of Bylins    *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef SPELLS_H_
#define SPELLS_H_

#include "entities/entity_constants.h"
#include "skills.h"
#include "structs/structs.h"    // there was defined type "byte" if it had been missing
#include "game_classes/classes_constants.h"

#include <optional>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

// *******************************
// * Spells type                 *
// *******************************

constexpr Bitvector kMtypeNeutral = 1 << 0;
constexpr Bitvector kMtypeAggressive = 1 << 1;

// *******************************
// * Spells class                *
// *******************************

enum EElement : int {
	kTypeNeutral = 0,
	kTypeAir,
	kTypeFire,
	kTypeWater,
	kTypeEarth,
	kTypeLight,
	kTypeDark,
	kTypeMind,
	kTypeLife
};

constexpr Bitvector kMagDamage = 1 << 0;
constexpr Bitvector kMagAffects = 1 << 1;
constexpr Bitvector kMagUnaffects = 1 << 2;
constexpr Bitvector kMagPoints = 1 << 3;
constexpr Bitvector kMagAlterObjs = 1 << 4;
constexpr Bitvector kMagGroups = 1 << 5;
constexpr Bitvector kMagMasses = 1 << 6;
constexpr Bitvector kMagAreas = 1 << 7;
constexpr Bitvector kMagSummons = 1 << 8;
constexpr Bitvector kMagCreations = 1 << 9;
constexpr Bitvector kMagManual = 1 << 10;
constexpr Bitvector kMagWarcry = 1 << 11;
constexpr Bitvector kMagNeedControl = 1 << 12;
// А чего это тут дырка Ж)
constexpr Bitvector kNpcDamagePc = 1 << 16;
constexpr Bitvector kNpcDamagePcMinhp = 1 << 17;
constexpr Bitvector kNpcAffectPc = 1 << 18;
constexpr Bitvector kNpcAffectPcCaster = 1 << 19;
constexpr Bitvector kNpcAffectNpc = 1 << 20;
constexpr Bitvector kNpcUnaffectNpc = 1 << 21;
constexpr Bitvector kNpcUnaffectNpcCaster = 1 << 22;
constexpr Bitvector kNpcDummy = 1 << 23;
constexpr Bitvector kMagRoom = 1 << 24;
constexpr Bitvector kMagCasterInroom = 1 << 25; // Аффект от этого спелла действует пока кастер в комнате //
constexpr Bitvector kMagCasterInworld = 1 << 26; // висит пока кастер в мире //
constexpr Bitvector kMagCasterAnywhere = 1 << 27; // висит пока не упадет сам //
constexpr Bitvector kMagCasterInworldDelay = 1 << 28; // висит пока кастер в мире, плюс таймер после ухода кастера//

#define NPC_CALCULATE 0xff << 16

// *** Extra attack bit flags //
constexpr Bitvector kEafParry = 1 << 0;
constexpr Bitvector kEafBlock = 1 << 1;
constexpr Bitvector kEafTouch = 1 << 2;
constexpr Bitvector kEafProtect = 1 << 3;
constexpr Bitvector kEafDodge = 1 << 4;
constexpr Bitvector kEafHammer = 1 << 5;
constexpr Bitvector kEafOverwhelm = 1 << 6;
constexpr Bitvector kEafSlow = 1 << 7;
constexpr Bitvector kEafPunctual = 1 << 8;
constexpr Bitvector kEafAwake = 1 << 9;
constexpr Bitvector kEafFirst = 1 << 10;
constexpr Bitvector kEafSecond = 1 << 11;
constexpr Bitvector kEafStand = 1 << 13;
constexpr Bitvector kEafUsedright = 1 << 14;
constexpr Bitvector kEafUsedleft = 1 << 15;
constexpr Bitvector kEafMultyparry = 1 << 16;
constexpr Bitvector kEafSleep = 1 << 17;
constexpr Bitvector kEafIronWind = 1 << 18;
constexpr Bitvector kEafAutoblock = 1 << 19;	// автоматический блок щитом в осторожном стиле
constexpr Bitvector kEafPoisoned = 1 << 20;		// отравление с пушек раз в раунд
constexpr Bitvector kEafFirstPoison = 1 << 21;	// отравление цели первый раз за бой

// PLAYER SPELLS TYPES //
constexpr Bitvector kSpellKnow = 1 << 0;
constexpr Bitvector kSpellTemp = 1 << 1;
constexpr Bitvector kSpellPotion = 1 << 2;
constexpr Bitvector kSpellWand = 1 << 3;
constexpr Bitvector kSpellScroll = 1 << 4;
constexpr Bitvector kSpellItems = 1 << 5;
constexpr Bitvector kSpellRunes = 1 << 6;

/// Flags for ingredient items (ITEM_INGREDIENT)
enum EIngredientFlag {
	kItemRunes = 1 << 0,
	kItemCheckUses = 1 << 1,
	kItemCheckLag = 1 << 2,
	kItemCheckLevel = 1 << 3,
	kItemDecayEmpty = 1 << 4
};

template<>
EIngredientFlag ITEM_BY_NAME<EIngredientFlag>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item);

constexpr Bitvector kMiLag1S = 1 << 0;
constexpr Bitvector kMiLag2S = 1 << 1;
constexpr Bitvector kMiLag4S = 1 << 2;
constexpr Bitvector kMiLag8S = 1 << 3;
constexpr Bitvector kMiLag16S = 1 << 4;
constexpr Bitvector kMiLag32S = 1 << 5;
constexpr Bitvector kMiLag64S = 1 << 6;
constexpr Bitvector kMiLag128S = 1 << 7;
constexpr Bitvector kMiLevel1 = 1 << 8;
constexpr Bitvector kMiLevel2 = 1 << 9;
constexpr Bitvector kMiLevel4 = 1 << 10;
constexpr Bitvector kMiLevel8 = 1 << 11;
constexpr Bitvector kMiLevel16 = 1 << 12;

// PLAYER SPELLS -- Numbered from 1 to SPELLS_COUNT //
enum ESpell : int {
	kSpellNoSpell = 0,
	kSpellArmor = 1,
	kSpellTeleport = 2,
	kSpellBless = 3,
	kSpellBlindness = 4,
	kSpellBurningHands = 5,
	kSpellCallLighting = 6,
	kSpellCharm = 7,
	kSpellChillTouch = 8,
	kSpellClone = 9,
	kSpellIceBolts = 10,
	kSpellControlWeather = 11,
	kSpellCreateFood = 12,
	kSpellCreateWater = 13,
	kSpellCureBlind = 14,
	kSpellCureCritic = 15,
	kSpellCureLight = 16,
	kSpellCurse = 17,
	kSpellDetectAlign = 18,
	kSpellDetectInvis = 19,
	kSpellDetectMagic = 20,
	kSpellDetectPoison = 21,
	kSpellDispelEvil = 22,
	kSpellEarthquake = 23,
	kSpellEnchantWeapon = 24,
	kSpellEnergyDrain = 25,
	kSpellFireball = 26,
	kSpellHarm = 27,
	kSpellHeal = 28,
	kSpellInvisible = 29,
	kSpellLightingBolt = 30,
	kSpellLocateObject = 31,
	kSpellMagicMissile = 32,
	kSpellPoison = 33,
	kSpellProtectFromEvil = 34,
	kSpellRemoveCurse = 35,
	kSpellSanctuary = 36,
	kSpellShockingGasp = 37,
	kSpellSleep = 38,
	kSpellStrength = 39,
	kSpellSummon = 40,
	kSpellPatronage = 41,
	kSpellWorldOfRecall = 42,
	kSpellRemovePoison = 43,
	kSpellSenseLife = 44,
	kSpellAnimateDead = 45,
	kSpellDispelGood = 46,
	kSpellGroupArmor = 47,
	kSpellGroupHeal = 48,
	kSpellGroupRecall = 49,
	kSpellInfravision = 50,
	kSpellWaterwalk = 51,
	kSpellCureSerious = 52,
	kSpellGroupStrength = 53,
	kSpellHold = 54,
	kSpellPowerHold = 55,
	kSpellMassHold = 56,
	kSpellFly = 57,
	kSpellBrokenChains = 58,
	kSpellNoflee = 59,
	kSpellCreateLight = 60,
	kSpellDarkness = 61,
	kSpellStoneSkin = 62,
	kSpellCloudly = 63,
	kSpellSllence = 64,
	kSpellLight = 65,
	kSpellChainLighting = 66,
	kSpellFireBlast = 67,
	kSpellGodsWrath = 68,
	kSpellWeaknes = 69,
	kSpellGroupInvisible = 70,
	kSpellShadowCloak = 71,
	kSpellAcid = 72,
	kSpellRepair = 73,
	kSpellEnlarge = 74,
	kSpellFear = 75,
	kSpellSacrifice = 76,
	kSpellWeb = 77,
	kSpellBlink = 78,
	kSpellRemoveHold = 79,
	kSpellCamouflage = 80,
	kSpellPowerBlindness = 81,
	kSpellMassBlindness = 82,
	kSpellPowerSilence = 83,
	kSpellExtraHits = 84,
	kSpellResurrection = 85,
	kSpellMagicShield = 86,
	kSpellForbidden = 87,
	kSpellMassSilence = 88,
	kSpellRemoveSilence = 89,
	kSpellDamageLight = 90,
	kSpellDamageSerious = 91,
	kSpellDamageCritic = 92,
	kSpellMassCurse = 93,
	kSpellArmageddon = 94,
	kSpellGroupFly = 95,
	kSpellGroupBless = 96,
	kSpellResfresh = 97,
	kSpellStunning = 98,
	kSpellHide = 99,
	kSpellSneak = 100,
	kSpellDrunked = 101,
	kSpellAbstinent = 102,
	kSpellFullFeed = 103,
	kSpellColdWind = 104,
	kSpellBattle = 105,
	kSpellHaemorragis = 106,
	kSpellCourage = 107,
	kSpellWaterbreath = 108,
	kSpellSlowdown = 109,
	kSpellHaste = 110,
	kSpellMassSlow = 111,
	kSpellGroupHaste = 112,
	kSpellGodsShield = 113,
	kSpellFever = 114,
	kSpellCureFever = 115,
	kSpellAwareness = 116,
	kSpellReligion = 117,
	kSpellAirShield = 118,
	kSpellPortal = 119,
	kSpellDispellMagic = 120,
	kSpellSummonKeeper = 121,
	kSpellFastRegeneration = 122,
	kSpellCreateWeapon = 123,
	kSpellFireShield = 124,
	kSpellRelocate = 125,
	kSpellSummonFirekeeper = 126,
	kSpellIceShield = 127,
	kSpellIceStorm = 128,
	kSpellLessening = 129,
	kSpellShineFlash = 130,
	kSpellMadness = 131,
	kSpellGroupMagicGlass = 132,
	kSpellCloudOfArrows = 133,
	kSpellVacuum = 134,
	kSpellMeteorStorm = 135,
	kSpellStoneHands = 136,
	kSpellMindless = 137,
	kSpellPrismaticAura = 138,
	kSpellEviless = 139,
	kSpellAirAura = 140,
	kSpellFireAura = 141,
	kSpellIceAura = 142,
	kSpellShock = 143,
	kSpellMagicGlass = 144,
	kSpellGroupSanctuary = 145,
	kSpellGroupPrismaticAura = 146,
	kSpellDeafness = 147,
	kSpellPowerDeafness = 148,
	kSpellRemoveDeafness = 149,
	kSpellMassDeafness = 150,
	kSpellDustStorm = 151,
	kSpellEarthfall = 152,
	kSpellSonicWave = 153,
	kSpellHolystrike = 154,
	kSpellSumonAngel = 155,
	kSpellMassFear = 156,
	kSpellFascination = 157,
	kSpellCrying = 158,
	kSpellOblivion = 159,
	kSpellBurdenOfTime = 160,
	kSpellGroupRefresh = 161,
	kSpellPeaceful = 162,
	kSpellMagicBattle = 163,
	kSpellBerserk = 164,
	kSpellStoneBones = 165,
	kSpellRoomLight = 166,
	kSpellPoosinedFog = 167,
	kSpellThunderstorm = 168,
	kSpellLightWalk = 169,
	kSpellFailure = 170,
	kSpellClanPray = 171,
	kSpellGlitterDust = 172,
	kSpellScream = 173,
	kSpellCatGrace = 174,
	kSpellBullBody = 175,
	kSpellSnakeWisdom = 176,
	kSpellGimmicry = 177,
	kSpellWarcryOfChallenge = 178,
	kSpellWarcryOfMenace = 179,
	kSpellWarcryOfRage = 180,
	kSpellWarcryOfMadness = 181,
	kSpellWarcryOfThunder = 182,
	kSpellWarcryOfDefence = 183,
	kSpellWarcryOfBattle = 184,
	kSpellWarcryOfPower = 185,
	kSpellWarcryOfBless = 186,
	kSpellWarcryOfCourage = 187,
	kSpellRuneLabel = 188,
	kSpellAconitumPoison = 189,
	kSpellScopolaPoison = 190,
	kSpellBelenaPoison = 191,
	kSpellDaturaPoison = 192,
	kSpellTimerRestore = 193,
	kSpellLucky = 194,
	kSpellBandage = 195,
	kSpellNoBandage = 196,
	kSpellCapable = 197,
	kSpellStrangle = 198,
	kSpellRecallSpells = 199,
	kSpellHypnoticPattern = 200,
	kSpellSolobonus = 201,
	kSpellVampirism = 202,
	kSpellRestoration = 203,
	kSpellDeathAura = 204,
	kSpellRecovery = 205,
	kSpellMassRecovery = 206,
	kSpellAuraOfEvil = 207,
	kSpellMentalShadow = 208,
	kSpellBlackTentacles = 209,
	kSpellWhirlwind = 210,
	kSpellIndriksTeeth = 211,
	kSpellAcidArrow = 212,
	kSpellThunderStone = 213,
	kSpellClod = 214,
	kSpellExpedient = 215,
	kSpellSightOfDarkness = 216,
	kSpellGroupSincerity = 217,
	kSpellMagicalGaze = 218,
	kSpellAllSeeingEye = 219,
	kSpellEyeOfGods = 220,
	kSpellBreathingAtDepth = 221,
	kSpellGeneralRecovery = 222,
	kSpellCommonMeal = 223,
	kSpellStoneWall = 224,
	kSpellSnakeEyes = 225,
	kSpellEarthAura = 226,
	kSpellGroupProtectFromEvil = 227,
	kSpellArrowsFire = 228,
	kSpellArrowsWater = 229,
	kSpellArrowsEarth = 230,
	kSpellArrowsAir = 231,
	kSpellArrowsDeath = 232,
	kSpellPaladineInspiration = 233,
	kSpellDexterity = 234,
	kSpellGroupBlink = 235,
	kSpellGroupCloudly = 236,
	kSpellGroupAwareness = 237,
	kSpellWatctyOfExpirence = 238,
	kSpellWarcryOfLuck = 239,
	kSpellWarcryOfPhysdamage = 240,
	kSpellMassFailure = 241,
	kSpellSnare = 242,
	kSpellFireBreath = 243,
	kSpellGasBreath = 244,
	kSpellFrostBreath = 245,
	kSpellAcidBreath = 246,
	kSpellLightingBreath = 247,
	kSpellIdentify = 351,
	kSpellFullIdentify = 352,
	kSpellQUest = 353,
	kSpellCount = kSpellQUest //last
};

template<>
ESpell ITEM_BY_NAME<ESpell>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESpell>(const ESpell spell);

const int kMaxSlot = 13;

const int kTypeHit = 400;
const int kTypeMagic = 420;
// new attack types can be added here - up to TYPE_SUFFERING
const int kTypeTriggerdeath = 495;
const int kTypeTunnerldeath = 496;
const int kTypeWaterdeath = 497;
const int kTypeRoomdeath = 498;
const int kTypeSuffering = 499;

constexpr Bitvector kTarIgnore = 1 << 0;
constexpr Bitvector kTarCharRoom = 1 << 1;
constexpr Bitvector kTarCharWorld = 1 << 2;	// не ищет мобов при касте чарами (призвать/переместиться/переход)
constexpr Bitvector kTarFightSelf = 1 << 3;
constexpr Bitvector kTarFightVict = 1 << 4;
constexpr Bitvector kTarSelfOnly = 1 << 5;	// Only a check, use with i.e. TAR_CHAR_ROOM //
constexpr Bitvector kTarNotSelf = 1 << 6;	// Only a check, use with i.e. TAR_CHAR_ROOM //
constexpr Bitvector kTarObjInv = 1 << 7;
constexpr Bitvector kTarObjRoom = 1 << 8;
constexpr Bitvector kTarObjWorld = 1 << 9;
constexpr Bitvector kTarObjEquip = 1 << 10;
constexpr Bitvector kTarRoomThis = 1 << 11;	// Цель комната в которой сидит чар//
constexpr Bitvector kTarRoomDir = 1 << 12;	// Цель комната в каком-то направлении от чара//
constexpr Bitvector kTarRoomWorld = 1 << 13;	// Цель какая-то комната в мире//

struct AttackHitType {
	const char *singular;
	const char *plural;
};

#define MANUAL_SPELL(spellname)    spellname(level, caster, cvict, ovict);

void SpellCreateWater(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellRecall(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /* obj*/);
void SpellTeleport(int /* level */, CharacterData *ch, CharacterData */*victim*/, ObjectData */*obj*/);
void SpellSummon(int /*level*/, CharacterData *ch, CharacterData *victim, ObjectData */*obj*/);
void SpellRelocate(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /* obj*/);
void SpellPortal(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /* obj*/);
void SpellLocateObject(int level, CharacterData *ch, CharacterData* /*victim*/, ObjectData *obj);
void SpellCharm(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /* obj*/);
void SpellInformation(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellIdentify(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellFullIdentify(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellEnchantWeapon(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellControlWeather(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellCreateWeapon(int/* level*/, CharacterData* /*ch*/, CharacterData* /*victim*/, ObjectData* /* obj*/);
void SpellEnergydrain(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /*obj*/);
void SpellFear(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /*obj*/);
void SpellSacrifice(int/* level*/, CharacterData *ch, CharacterData *victim, ObjectData* /*obj*/);
void SpellForbidden(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellHolystrike(int/* level*/, CharacterData *ch, CharacterData* /*victim*/, ObjectData* /*obj*/);
void SkillIdentify(int level, CharacterData *ch, CharacterData *victim, ObjectData *obj);
void SpellSummonAngel(int/* level*/, CharacterData *ch, CharacterData* /*victim*/, ObjectData* /*obj*/);
void SpellVampirism(int/* level*/, CharacterData* /*ch*/, CharacterData* /*victim*/, ObjectData* /*obj*/);
void SpellMentalShadow(int/* level*/, CharacterData *ch, CharacterData* /*victim*/, ObjectData* /*obj*/);

// возращает текст выводимый при спадении скила
std::string get_wear_off_text(ESpell spell);

// текст заклинания для игроков, произнесенные заклинателями разных религий
struct CastPhraseList {
	// заклинание произнесенное язычником
	std::string text_for_heathen;

	// заклинание произнесенное христианиным
	std::string text_for_christian;
};

// возвращает фразу для заклинания согласно религии
// std::nullopt если соответсвующего заклинания в таблице нет
std::optional<CastPhraseList> get_cast_phrase(int spell);

// basic magic calling functions

int FixNameAndFindSpellNum(char *name);

bool CatchBloodyCorpse(ObjectData *l);

// other prototypes //
void InitSpellLevels();
const char *GetSpellName(int num);
int CalculateSaving(CharacterData *killer, CharacterData *victim, ESaving saving, int ext_apply);
int CalcGeneralSaving(CharacterData *killer, CharacterData *victim, ESaving type, int ext_apply);
bool IsAbleToGetSpell(CharacterData *ch, int spellnum);
int CalcMinSpellLevel(CharacterData *ch, int spellnum, int req_lvl);
bool IsAbleToGetSpell(CharacterData *ch, int spellnum, int req_lvl);
ESkill GetMagicSkillId(int spellnum);
int CheckRecipeValues(CharacterData *ch, int spellnum, int spelltype, int showrecipe);
int CheckRecipeItems(CharacterData *ch, int spellnum, int spelltype, int extract, const CharacterData *targ = nullptr);

//Polud статистика использования заклинаний
typedef std::map<int, int> SpellCountType;

namespace SpellUsage {
	extern bool is_active;
	extern time_t start;
	void AddSpellStat(int char_class, int spellnum);
	void save();
	void clear();
};
//-Polud

#define CALC_SUCCESS(modi, perc)         ((modi)-100+(perc))

const int kHoursPerWarcry = 4;
const int kHoursPerTurnUndead = 8;

#endif // SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
