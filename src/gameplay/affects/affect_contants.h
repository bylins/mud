/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#ifndef BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_
#define BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

#include "gameplay/magic/spells_constants.h"
#include "gameplay/mechanics/condition.h"
#include "engine/structs/bitset_flags.h"

// Константа, определяющая скорость таймера аффектов
const int kSecsPerPlayerAffect = 2;

// Типы таймеров аффектов.
enum EAffFlag : Bitvector {
  kAfBattledec			= 1u << 0,
  kAfDeadkeep			= 1u << 1,
  kAfPulsedec			= 1u << 2,
  kAfSameTime			= 1u << 3,	// тикает раз в две секунды для PC, раз в минуту для NPC, или во время раунда в бою (чтобы не между раундами)
  kAfUpdateDuration		= 1u << 4,
  kAfAccumulateDuration	= 1u << 5,
  kAfUpdateMod			= 1u << 6,
  kAfDispellable		= 1u << 7,	// аффект можно снять магией (источник истины для CheckNodispel)
  kAfCurable			= 1u << 8,	// аффект можно вылечить (первая помощь и будущая механика лечения)
  kAfMustBeHandled		= 1u << 9,	// у аффекта есть периодический обработчик в коде (room-affect tick, см. HandleRoomAffect) -- бывший Affect::must_handled
  kAfUnique				= 1u << 10,	// перед наложением снять предыдущий аффект этого же типа от того же кастера (room-affect "только один в мире") -- бывший локальный only_one в CallMagicToRoom
  kAfFailed				= 1u << 11	// the affect is a failed-attempt marker: it carries the success affect_type but must NOT count as the real effect (the affect_total rebuild skips its flag bit; query via AffSuccessFlagged)
};

/**
 * Affect bits: used in char_data.char_specials.saved.affected_by //
 */
enum class EAffect : Bitvector {
	kUndefined = 0,
	// Alias of kUndefined: the generic "default" affect (a script/skill effect with no
	// specific identity). Displays via the shared kDefault sheaf's fallback kShortDesc.
	kDefault = kUndefined,
	kBlind = 1,
	kInvisible = 2,
	kDetectAlign = 3,
	kDetectInvisible = 4,
	kDetectMagic = 5,
	kDetectLife = 6,
	kWaterWalk = 7,
	kSanctuary = 8,
	kGroup = 9,
	kCurse = 10,
	kInfravision = 11,
	kPoisoned = 12,
	kProtectFromDark = 13,
	kProtectFromMind = 14,
	kSleep = 15,
	kNoTrack = 16,
	kTethered = 17,
	kBless = 18,
	kSneak = 19,
	kHide = 20,
	kCourage = 21,
	kCharmed = 22,
	kHold = 23,
	kFly = 24,
	kSilence = 25,
	kAwarness = 26,
	kBlink = 27,
	kHorse = 28,                    ///< NPC - is horse, PC - is horsed
	kNoFlee = 29,
	kSingleLight = 30,
	kHolyLight = 31,
	kHolyDark = 32,
	kDetectPoison = 33,
	kDrunked = 34,
	kAbstinent = 35,
	kStopRight = 36,
	kStopLeft = 37,
	kStopFight = 38,
	kHaemorrhage = 39,
	kDisguise = 40,
	kWaterBreath = 41,
	kSlow = 42,
	kHaste = 43,
	kGodsShield = 44,
	kAirShield = 45,
	kFireShield = 46,
	kIceShield = 47,
	kMagicGlass = 48,
	kStairs = 49,
	kStoneHands = 50,
	kPrismaticAura = 51,
	kHelper = 52,
	kForcesOfEvil = 53,
	kAirAura = 54,
	kFireAura = 55,
	kIceAura = 56,
	kDeafness = 57,
	kCrying = 58,
	kPeaceful = 59,
	kMagicStopFight = 60,
	kBerserk = 61,
	kLightWalk = 62,
	kBrokenChains = 63,
	kCloudOfArrows = 64,
	kShadowCloak = 65,
	kGlitterDust = 66,
	kAffright = 67,
	kScopolaPoison = 68,
	kDaturaPoison = 69,
	kSkillReduce = 70,
	kNoBattleSwitch = 71,
	kBelenaPoison = 72,
	kNoTeleport = 73,
	kCombatLuck = 74,
	kBandage = 75,
	kCannotBeBandaged = 76,
	kMorphing = 77,
	kStrangled = 78,
	kMemorizeSpells = 79,
	kNoobRegen = 80,
	kVampirism = 81,
	kLacerations = 82,
	kCommander = 83,
	kEarthAura = 84,
	kCloudly = 85,
	kConfused = 86,
	kNoCharge = 87,
	kInjuredLimb = 88,
	kFrenzy = 89,
	// issue.affect-migration: per-spell affects (each spell gets its own flag).
	kMagicArmor = 90,
	kShivering = 91,
	kDespodency = 92,
	kMagicStrength = 93,
	kPatronage = 94,
	kStoneSkin = 95,
	kMagicWeaknes = 96,
	kEnlarge = 97,
	kMagicShield = 98,
	kForbidden = 99,
	kFrostbite = 100,
	kSlowdown = 101,
	kFever = 102,
	kFastRegeneration = 103,
	kLessening = 104,
	kMadness = 105,
	kMindless = 106,
	kFascination = 107,
	kContusion = 108,
	kStoneBones = 109,
	kFailure = 110,
	kCatGrace = 111,
	kBullBody = 112,
	kSnakeWisdom = 113,
	kGimmicry = 114,
	kIndecision = 115,
	kInsanity = 116,
	kRousing = 117,
	kPassion = 118,
	kSurgeOfPower = 119,
	kSurgeOfValour = 120,
	kBattleCourage = 121,
	kInspiration = 122,
	kConcentration = 123,
	kBattleLuck = 124,
	kPhysdamageBonus = 125,
	kSuspiciousness = 126,
	kSevereWound = 127,
	kWellFed = 128,
	kPrayerful = 129,
	kPietas = 130,
};

// --- BitsetFlags integration for EAffect ----------------------------------------------------------
// EAffect values are plain integers now: kUndefined=0 (the "no flag" sentinel, used by affect_type{}
// defaults and static_cast<EAffect>(0)), real affects 1-based kBlind=1..kFrenzy=89. The flat bit index
// is value-1, which equals the legacy plane*30+bit position -- so CharData::affected_by as
// BitsetFlags<EAffect> stays byte-identical to the old FlagData on disk. count = 89 distinct bits.
template<>
struct flag_traits<EAffect> {
	static constexpr std::size_t count = 130;
};
template<>
struct flag_index_mapping<EAffect> {
	static constexpr std::size_t to_index(EAffect f) {
		// kUndefined=0 is reserved as "no flag"; real affects are 1-based (kBlind=1..kFrenzy=89),
		// so the flat bit index (= legacy plane*30+bit, preserving on-disk bytes) is value-1.
		return static_cast<std::size_t>(f) - 1;
	}
};

template<>
const std::string &NAME_BY_ITEM<EAffect>(EAffect item);
template<>
EAffect ITEM_BY_NAME<EAffect>(const std::string &name);
template<>
const std::map<EAffect, std::string> &NAMES_OF<EAffect>();  // issue.vedun-editor

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
template<>
const std::map<EApply, std::string> &NAMES_OF<EApply>();  // issue.vedun-editor

template<>
const std::string &NAME_BY_ITEM<EAffFlag>(EAffFlag item);
template<>
EAffFlag ITEM_BY_NAME<EAffFlag>(const std::string &name);
template<>
const std::map<EAffFlag, std::string> &NAMES_OF<EAffFlag>();  // issue.vedun-editor: editor enum pick-list

using WeaponAffectArray = std::array<WeaponAffect, kWeaponAffectCount>;
extern WeaponAffectArray weapon_affect;
// issue.ext-affects: built at boot from cfg/affects.xml (see affects_loader.h), indexed positionally by
// the EAffect bit so sprintbits keeps working unchanged; null until the "affects" cfg is loaded.
extern const char *apply_types[];

#endif //BYLINS_SRC_AFFECTS_AFFECT_CONTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
