/*************************************************************************
*   File: features.hpp                                 Part of Bylins    *
*   Features code header                                                 *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                                                  *
**************************************************************************/
#ifndef FILE_FEATURES_H_INCLUDED
#define FILE_FEATURES_H_INCLUDED

#include "abilities/abilities_items_set.h"
#include "game_skills/skills.h"
#include "structs/structs.h"
#include "conf.h"
#include "game_classes/classes_constants.h"

#include <array>
#include <bitset>

enum EFeat : int {
	kIncorrectFeat = 0,			//DO NOT USE
	kBerserker = 1,				//предсмертная ярость
	kParryArrow = 2,			//отбить стрелу
	kBlindFight = 3,			//слепой бой
	kImpregnable = 4,			//непробиваемый
	kApproachingAttack = 5,		//встречная атака
	kDefender = 6,				//щитоносец
	kDodger = 7,				//изворотливость
	kLightWalk = 8,				//легкая поступь
	kWriggler = 9,				//проныра
	kSpellSubstitute = 10,		//подмена заклинания
	kPowerAttack = 11,			//мощная атака
	kWoodenSkin = 12,			//деревянная кожа
	kIronSkin = 13,				//железная кожа
	kConnoiseur = 14,			//знаток
	kExorcist = 15,				//изгоняющий нежить
	kHealer = 16,				//целитель
	kLightingReflex = 17,		//мгновенная реакция
	kDrunkard = 18,				//пьяница
	kPowerMagic = 19,			//мощь колдовства
	kEndurance = 20,			//выносливость
	kGreatFortitude = 21,		//сила духа
	kFastRegen = 22,			//быстрое заживление
	kStealthy = 23,				//незаметность
	kRelatedToMagic = 24,		//магическое родство
	kSplendidHealth = 25,		//богатырское здоровье
	kTracker = 26,				//следопыт
	kWeaponFinesse = 27,		//ловкий удар
	kCombatCasting = 28,		//боевое чародейство
	kPunchMaster = 29,			//кулачный боец
	kClubsMaster = 30,			//мастер палицы
	kAxesMaster = 31,			//мастер секиры
	kLongsMaster = 32,			//мастер меча
	kShortsMaster = 33,			//мастер кинжала
	kNonstandartsMaster = 34,	//необычное оружие
	kTwohandsMaster = 35,		//мастер двуручника
	kPicksMaster = 36,			//мастер ножа
	kSpadesMaster = 37,			//мастер копья
	kBowsMaster = 38,			//мастер стрельбы
	kForestPath = 39,			//лесные тропы
	kMountainPath = 40,			//горные тропы
	kLuckyGuy = 41,				//счастливчик
	kWarriorSpirit = 42,		//боевой дух
	kReliableHealth = 43,		//крепкое здоровье
	kExcellentMemory = 44,		//превосходная память
	kAnimalDextery = 45,		//звериная прыть
	kLegibleWritting = 46,		//четкий почерк
	kIronMuscles = 47,			//стальные мышцы
	kMagicSign = 48,			//знак чародея
	kGreatEndurance = 49,		//двужильность
	kBestDestiny = 50,			//лучшая доля
	kHerbalist = 51,			//травник
	kJuggler = 52,				//жонглер
	kNimbleFingers = 53,		//ловкач
	kGreatPowerAttack = 54,		//улучшенная мощная атака
	kStrongImmunity = 55,		//привычка к яду
	kMobility = 56,				//подвижность
	kNaturalStr = 57,			//силач
	kNaturalDex = 58,			//проворство
	kNaturalInt = 59,			//природный ум
	kNaturalWis = 60,			//мудрец
	kNaturalCon = 61,			//здоровяк
	kNaturalCha = 62,			//природное обаяние
	kMnemonicEnhancer = 63,		//отличная память
	kMagneticPersonality = 64,	//предводитель
	kDamrollBonus = 65,			//тяжел на руку
	kHitrollBonus = 66,			//твердая рука
	kMagicalInstinct = 67,		//магическое чутье
	kPunchFocus = 68,			//любимое оружие: голые руки
	kClubsFocus = 69,			//любимое оружие: палица
	kAxesFocus = 70,			//любимое оружие: секира
	kLongsFocus = 71,			//любимое оружие: меч
	kShortsFocus = 72,			//любимое оружие: нож
	kNonstandartsFocus = 73,	//любимое оружие: необычное
	kTwohandsFocus = 74,		//любимое оружие: двуручник
	kPicksFocus = 75,			//любимое оружие: кинжал
	kSpadesFocus = 76,			//любимое оружие: копье
	kBowsFocus = 77,			//любимое оружие: лук
	kAimingAttack = 78,			//прицельная атака
	kGreatAimingAttack = 79,	//улучшенная прицельная атака
	kDoubleshot = 80,			//двойной выстрел
	kPorter = 81,				//тяжеловоз
	kSecretRunes = 82,			//тайные руны
/*
Удалены, номера можно использовать.
	UNUSED = 83,
	UNUSED = 84,
	UNUSED = 85,
*/
	kToFitItem = 86,			//подгонка
	kToFitClotches = 87,		//перешить
	kStrengthConcentration = 88,// концентрация силы
	kDarkReading = 89,			// кошачий глаз
	kSpellCapabler = 90,		// зачаровать
	kWearingLightArmor = 91,	// ношение легкого типа доспехов
	kWearingMediumArmor = 92,	// ношение среднего типа доспехов
	kWearingHeavyArmor = 93,	// ношение тяжелого типа доспехов
	kGemsInlay = 94,			// инкрустация камнями (TODO: не используется)
	kWarriorStrength = 95,		// богатырская сила
	kRelocate = 96,				// переместиться
	kSilverTongue = 97,			//сладкоречие
	kBully = 98,				//забияка
	kThieveStrike = 99,			//воровской удар
	kJeweller = 100,			//искусный ювелир
	kSkilledTrader = 101,		//торговая сметка
	kZombieDrover = 102,		//погонщик умертвий
	kEmployer = 103,			//навык найма
	kMagicUser = 104,			//использование амулетов
	kGoldTongue = 105,			//златоуст
	kCalmness = 106,			//хладнокровие
	kRetreat = 107,				//отступление
	kShadowStrike = 108,		//танцующая тень
	kThrifty = 109,				//запасливость
	kCynic = 110,				//циничность
	kPartner = 111,				//напарник
	kFavorOfDarkness = 112,		//помощь тьмы
	kFuryOfDarkness = 113,		//ярость тьмы
	kRegenOfDarkness = 114,		//темное восстановление
	kSoulLink = 115,			//родство душ
	kStrongClutch = 116,		//сильная хватка
	kMagicArrows = 117,			//магические стрелы
	kSoulsCollector = 118,		//коллекционер душ
	kDarkPact = 119,			//темная сделка
	kCorruption = 120,			//порча
	kHarvestOfLife = 121,		//жатва жизни
	kLoyalAssist = 122,			//верный помощник
	kHauntingSpirit = 123,		//блуждающий дух
	kSnakeRage = 124,			//ярость змеи
//	UNUSED = 125,
	kElderTaskmaster = 126,		//Старший надсмотрщик
	kLordOfUndeads = 127,		//Повелитель нежити
	kWarlock = 128,				//Темный маг
	kElderPriest = 129,			//Старший жрец
	kHighLich = 130,			//Верховный лич
	kDarkRitual = 131,			//Темный ритуал
	kTeamsterOfUndead = 132,	//Погонщик нежити
	kScirmisher = 133,			//Держать строй
	kTactician = 134,			//Атаман
	kLiveShield = 135,			//мультиреск

/*
новвоведения для татя

	RESERVED = 132,		//
	RESERVED = 133,		//
	RESERVED = 134,		//
	RESERVED = 135,		//
	RESERVED = 136,		//
*/
	kSerratedBlade = 137,		//воровская заточка
	kEvasion = 138,				//скользский тип
	kCutting = 139,				//порез
	kFInesseShot = 140,			//ловкий выстрел
	kObjectEnchanter = 141,		//зачаровывание предметов
	kDeftShooter = 142,			//ловкий стрелок
	kMagicShooter = 143,		//магический выстрел
	kThrowWeapon = 144,			//метнуть
	kShadowThrower = 145,		//змеево оружие
	kShadowDagger = 146,		//змеев кинжал
	kShadowSpear = 147,			//змеево копье
	kShadowClub = 148,			//змеева палица
	kDoubleThrower = 149,		//двойной бросок
	kTripleThrower = 150,		//тройной бросок
	kPowerThrow = 151,			//размах
	kDeadlyThrow = 152,			//широкий размах
	kUndeadsTurn = 153,			//затычка для "изгнать нежить"
	kMultipleCast = 154,		//уменьшение штрафа за число циелей для масскастов
	kMagicalShield = 155,		//заговоренный щит" - фит для витязей, защита от директ спеллов
	kAnimalMaster = 156,		//хозяин животных
	kFirstFeat = kBerserker,
	kLastFeat = kAnimalMaster	// !!! Не забываем менять !!!
};

EFeat& operator++(EFeat &f);

enum class EFeatType {
	kUnused = -1,
	kNormal = 0,
	kAffect = 1,
	kSkillMod = 2,
	kActivated = 3,
	kTechnique = 4,
};

/* Константы и формулы, определяющие число способностей у персонажа
   Скорость появления новых слотов можно задавать для каждого класса
      индивидуально, но последний слот персонаж всегда получает на 28 уровне */

//Раз в сколько ремортов появляется новый слот под способность

const int feat_slot_for_remort[kNumPlayerClasses] = {5, 6, 4, 4, 4, 4, 6, 6, 6, 4, 4, 4, 4, 5};
// Количество пар "параметр-значение" у способности
const int kMaxFeatAffect = 5;
// Максимально доступное на морте количество не-врожденных способностей
#define MAX_ACC_FEAT(ch)    ((int) 1+(kLvlImmortal-1)*(5+GET_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)])/28)

// Поля изменений для способностей (кроме EFeatType::kAffect, для них используются стардартные поля APPLY)
const int kFeatTimer = 1;
//const int FEAT_SKILL = 2;

struct TimedFeat {
	int feat{kIncorrectFeat};	// Used feature //
	ubyte time{0};				// Time for next using //
	struct TimedFeat *next{nullptr};
};

extern struct FeatureInfo feat_info[EFeat::kLastFeat + 1];

const char *GetFeatName(EFeat id);
int GetModifier(EFeat feat_id, int location);
EFeat FindFeatNum(const char *name, bool alias = false);
EFeat FindWeaponMasterFeat(ESkill skill);
void InitFeatures();
void CheckBerserk(CharData *ch);
void SetRaceFeats(CharData *ch);
void UnsetRaceFeats(CharData *ch);
void SetInbornFeats(CharData *ch);
bool IsAbleToUseFeat(const CharData *ch, EFeat feat);
bool IsAbleToGetFeat(CharData *ch, EFeat feat);
bool TryFlipActivatedFeature(CharData *ch, char *argument);
Bitvector GetPrfWithFeatNumber(EFeat feat_id);

/*
	Класс для удобства вбивания значений в массив affected структуры способности
*/
class CFeatArray {
 public:
	explicit CFeatArray() :
		pos_{0} {}

	int pos(int pos = -1);
	void insert(int location, int modifier);
	void clear();

	struct CFeatAffect {
		int location;
		int modifier;

		CFeatAffect() :
			location{0}, modifier{0} {}

		CFeatAffect(int location, int modifier) :
			location(location), modifier(modifier) {}

		bool operator!=(const CFeatAffect &r) const {
			return (location != r.location || modifier != r.modifier);
		}
		bool operator==(const CFeatAffect &r) const {
			return !(*this != r);
		}
	};

	struct CFeatAffect affected[kMaxFeatAffect];

 private:
	int pos_;
};

struct FeatureInfo {
	EFeat id;
	EFeatType type;
	int min_remort[kNumPlayerClasses][kNumKins];
	int slot[kNumPlayerClasses][kNumKins];
	bool is_known[kNumPlayerClasses][kNumKins];
	bool is_inborn[kNumPlayerClasses][kNumKins];
	bool up_slot;
	bool uses_weapon_skill;
	bool always_available;
	const char *name;
	std::array<CFeatArray::CFeatAffect, kMaxFeatAffect> affected;
	std::string alias;
	// Параметры для нового базового броска на способность
	// Пока тут, до переписывания системы способностей
	int damage_bonus;
	int success_degree_damage_bonus;
	int diceroll_bonus;
	int critfail_threshold;
	int critsuccess_threshold;
	ESaving saving;
	ESkill base_skill;

	TechniqueItemKitsGroup item_kits;

	int (*GetBaseParameter)(const CharData *ch);
	int (*GetEffectParameter)(const CharData *ch);
	float (*CalcSituationalDamageFactor)(CharData * /* ch */);
	int (*CalcSituationalRollBonus)(CharData * /* ch */, CharData * /* enemy */);
};

#endif // __FEATURES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
