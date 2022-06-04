//#include "feats.h"
//#include "feats_constants.h"

#include <boost/algorithm/string.hpp>

//#include "game_abilities/abilities_constants.h"
#include "action_targeting.h"
#include "handler.h"
#include "entities/player_races.h"
#include "color.h"
#include "game_fight/pk.h"
#include "game_magic/magic_utils.h"
#include "structs/global_objects.h"


//struct FeatureInfo feat_info[to_underlying(EFeat::kLastFeat) + 1];
std::unordered_map<EFeat, FeatureInfo> feat_info;

/* Служебные функции */
bool CheckVacantFeatSlot(CharData *ch, EFeat feat);

/* Функции для работы с переключаемыми способностями */
bool CheckAccessActivatedFeat(CharData *ch, EFeat feat_id);
void ActivateFeat(CharData *ch, EFeat feat_id);
void DeactivateFeature(CharData *ch, EFeat feat_id);

/* Ситуативные бонусы, пишутся для специфических способностей по потребности */
int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */);

/* Extern */
extern void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);

void InitFeat(EFeat feat, const char *name, EFeatType type = EFeatType::kNormal, int roll_bonus = abilities::kMaxRollBonus,
			  ESkill base_skill = ESkill::kUndefined, ESaving saving = ESaving::kStability) {
	feat_info[feat].id = feat;
	feat_info[feat].diceroll_bonus = roll_bonus;
	feat_info[feat].base_skill = base_skill;
	feat_info[feat].saving = saving;
	feat_info[feat].type = type;
}

FeatureInfo::FeatureInfo(EFeat feat_id) {
	id = feat_id;
	GetBaseParameter = &GET_REAL_INT;
	GetEffectParameter = &GET_REAL_STR;
}

void InitFeatures() {
	for (auto i = EFeat::kUndefined; i <= EFeat::kLast; ++i) {
		feat_info[i] = FeatureInfo(i);
	}

//1
	InitFeat(EFeat::kBerserker, "предсмертная ярость");
//2
	InitFeat(EFeat::kParryArrow, "отбить стрелу");
//3
	InitFeat(EFeat::kBlindFight, "слепой бой");
//4
	InitFeat(EFeat::kImpregnable, "непробиваемый");
//5
	InitFeat(EFeat::kApproachingAttack, "встречная атака");
//6
	InitFeat(EFeat::kDefender, "щитоносец");
//7
	InitFeat(EFeat::kDodger, "изворотливость");
//8
	InitFeat(EFeat::kLightWalk, "легкая поступь");
//9
	InitFeat(EFeat::kWriggler, "проныра");
//10
	InitFeat(EFeat::kSpellSubstitute, "подмена заклинания");
//11
	InitFeat(EFeat::kPowerAttack, "мощная атака", EFeatType::kActivated);
//12
	InitFeat(EFeat::kWoodenSkin, "деревянная кожа");
//13
	InitFeat(EFeat::kIronSkin, "железная кожа");
//14
	InitFeat(EFeat::kConnoiseur, "знаток");
//15
	InitFeat(EFeat::kExorcist, "изгоняющий нежить");
//16
	InitFeat(EFeat::kHealer, "целитель");
//17
	InitFeat(EFeat::kLightingReflex, "мгновенная реакция");
//18
	InitFeat(EFeat::kDrunkard, "пьяница");
//19
	InitFeat(EFeat::kPowerMagic, "мощь колдовства");
//20
	InitFeat(EFeat::kEndurance, "выносливость");
//21
	InitFeat(EFeat::kGreatFortitude, "сила духа");
//22
	InitFeat(EFeat::kFastRegen, "быстрое заживление");
//23
	InitFeat(EFeat::kStealthy, "незаметность");
//25
	InitFeat(EFeat::kSplendidHealth, "богатырское здоровье");
//26
	InitFeat(EFeat::kTracker, "следопыт");
//27
	InitFeat(EFeat::kWeaponFinesse, "ловкий удар");
//28
	InitFeat(EFeat::kCombatCasting, "боевое колдовство");
//29
	InitFeat(EFeat::kPunchMaster, "мастер кулачного боя");
//30
	InitFeat(EFeat::kClubsMaster, "мастер палицы");
//31
	InitFeat(EFeat::kAxesMaster, "мастер секиры");
//32
	InitFeat(EFeat::kLongsMaster, "мастер меча");
//33
	InitFeat(EFeat::kShortsMaster, "мастер ножа");
//34
	InitFeat(EFeat::kNonstandartsMaster, "мастер необычного оружия");
//35
	InitFeat(EFeat::kTwohandsMaster, "мастер двуручника");
//36
	InitFeat(EFeat::kPicksMaster, "мастер кинжала");
//37
	InitFeat(EFeat::kSpadesMaster, "мастер копья");
//38
	InitFeat(EFeat::kBowsMaster, "мастер лучник");
//39
	InitFeat(EFeat::kForestPath, "лесные тропы");
//40
	InitFeat(EFeat::kMountainPath, "горные тропы");
//41
	InitFeat(EFeat::kLuckyGuy, "счастливчик");
//42
	InitFeat(EFeat::kWarriorSpirit, "боевой дух");
//43
	InitFeat(EFeat::kReliableHealth, "крепкое здоровье");
//44
	InitFeat(EFeat::kExcellentMemory, "превосходная память");
//45
	InitFeat(EFeat::kAnimalDextery, "звериная прыть");
//46
	InitFeat(EFeat::kLegibleWritting, "чёткий почерк");
//47
	InitFeat(EFeat::kIronMuscles, "стальные мышцы");
//48
	InitFeat(EFeat::kMagicSign, "знак чародея");
//49
	InitFeat(EFeat::kGreatEndurance, "двужильность");
//50
	InitFeat(EFeat::kBestDestiny, "лучшая доля");
//51
	InitFeat(EFeat::kHerbalist, "травник");
//52
	InitFeat(EFeat::kJuggler, "жонглер");
//53
	InitFeat(EFeat::kNimbleFingers, "ловкач");
//54
	InitFeat(EFeat::kGreatPowerAttack, "улучшенная мощная атака", EFeatType::kActivated);
//55
	InitFeat(EFeat::kStrongImmunity, "привычка к яду");
//56
	InitFeat(EFeat::kMobility, "подвижность");
//57
	InitFeat(EFeat::kNaturalStr, "силач");
//58
	InitFeat(EFeat::kNaturalDex, "проворство");
//59
	InitFeat(EFeat::kNaturalInt, "природный ум");
//60
	InitFeat(EFeat::kNaturalWis, "мудрец");
//61
	InitFeat(EFeat::kNaturalCon, "здоровяк");
//62
	InitFeat(EFeat::kNaturalCha, "природное обаяние");
//63
	InitFeat(EFeat::kMnemonicEnhancer, "отличная память");
//64 -*
	InitFeat(EFeat::kMagneticPersonality, "предводитель");
//65
	InitFeat(EFeat::kDamrollBonus, "тяжел на руку");
//66
	InitFeat(EFeat::kHitrollBonus, "твердая рука");
//68
	InitFeat(EFeat::kPunchFocus, "любимое оружие: голые руки");
//69
	InitFeat(EFeat::kClubsFocus, "любимое оружие: палица");
//70
	InitFeat(EFeat::kAxesFocus, "любимое оружие: секира");
//71
	InitFeat(EFeat::kLongsFocus, "любимое оружие: меч");
//72
	InitFeat(EFeat::kShortsFocus, "любимое оружие: нож");
//73
	InitFeat(EFeat::kNonstandartsFocus, "любимое оружие: необычное");
//74
	InitFeat(EFeat::kTwohandsFocus, "любимое оружие: двуручник");
//75
	InitFeat(EFeat::kPicksFocus, "любимое оружие: кинжал");
//76
	InitFeat(EFeat::kSpadesFocus, "любимое оружие: копье");
//77
	InitFeat(EFeat::kBowsFocus, "любимое оружие: лук");
//78
	InitFeat(EFeat::kAimingAttack, "прицельная атака", EFeatType::kActivated);
//79
	InitFeat(EFeat::kGreatAimingAttack, "улучшенная прицельная атака", EFeatType::kActivated);
//80
	InitFeat(EFeat::kDoubleShot, "двойной выстрел");
//81
	InitFeat(EFeat::kPorter, "тяжеловоз");
//82
	InitFeat(EFeat::kSecretRunes, "тайные руны");
//86
	InitFeat(EFeat::kToFitItem, "переделать");
//87
	InitFeat(EFeat::kToFitClotches, "перешить");
//88
	InitFeat(EFeat::kStrengthConcentration, "концентрация силы");
//89
	InitFeat(EFeat::kDarkReading, "кошачий глаз");
//90
	InitFeat(EFeat::kSpellCapabler, "зачаровать");
//91
	InitFeat(EFeat::kWearingLightArmor, "легкие доспехи");
//92
	InitFeat(EFeat::kWearingMediumArmor, "средние доспехи");
//93
	InitFeat(EFeat::kWearingHeavyArmor, "тяжелые доспехи");
//94
	InitFeat(EFeat::kGemsInlay, "инкрустация");
//95
	InitFeat(EFeat::kWarriorStrength, "богатырская сила");
//96
	InitFeat(EFeat::kRelocate, "переместиться");
//97
	InitFeat(EFeat::kSilverTongue, "сладкоречие");
//98
	InitFeat(EFeat::kBully, "забияка");
//99
	InitFeat(EFeat::kThieveStrike, "воровской удар");
//100
	InitFeat(EFeat::kJeweller, "искусный ювелир");
//101
	InitFeat(EFeat::kSkilledTrader, "торговая сметка");
//102
	InitFeat(EFeat::kZombieDrover, "погонщик умертвий");
//103
	InitFeat(EFeat::kEmployer, "навык найма");
//104
	InitFeat(EFeat::kMagicUser, "использование амулетов");
//105
	InitFeat(EFeat::kGoldTongue, "златоуст");
//106
	InitFeat(EFeat::kCalmness, "хладнокровие");
//107
	InitFeat(EFeat::kRetreat, "отступление");
//108
	InitFeat(EFeat::kShadowStrike, "танцующая тень");
//109
	InitFeat(EFeat::kThrifty, "запасливость");
//110
	InitFeat(EFeat::kCynic, "циничность");
//111
	InitFeat(EFeat::kPartner, "напарник");
//112
	InitFeat(EFeat::kFavorOfDarkness, "помощь тьмы");
//113
	InitFeat(EFeat::kFuryOfDarkness, "ярость тьмы");
//114
	InitFeat(EFeat::kRegenOfDarkness, "темное восстановление");
//115
	InitFeat(EFeat::kSoulLink, "родство душ");
//116
	InitFeat(EFeat::kStrongClutch, "сильная хватка");
//117
	InitFeat(EFeat::kMagicArrows, "магические стрелы");
//118
	InitFeat(EFeat::kSoulsCollector, "колекционер душ");
//119
	InitFeat(EFeat::kDarkPact, "темная сделка");
//120
	InitFeat(EFeat::kCorruption, "порча");
//121
	InitFeat(EFeat::kHarvestOfLife, "жатва жизни");
//122
	InitFeat(EFeat::kLoyalAssist, "верный помощник");
//123
	InitFeat(EFeat::kHauntingSpirit, "блуждающий дух");
//124
	InitFeat(EFeat::kSnakeRage, "ярость змеи");
//126
	InitFeat(EFeat::kElderTaskmaster, "старший надсмотрщик");
//127
	InitFeat(EFeat::kLordOfUndeads, "повелитель нежити");
//128
	InitFeat(EFeat::kWarlock, "темный маг");
//129
	InitFeat(EFeat::kElderPriest, "старший жрец");
//130
	InitFeat(EFeat::kHighLich, "верховный лич");
//131
	InitFeat(EFeat::kDarkRitual, "темный ритуал");
//132
	InitFeat(EFeat::kTeamsterOfUndead, "погонщик нежити");
//133
	InitFeat(EFeat::kScirmisher, "держать строй", EFeatType::kActivated, 90, ESkill::kRescue, ESaving::kReflex);
	feat_info[EFeat::kScirmisher].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kScirmisher].CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
//134
	InitFeat(EFeat::kTactician, "десяцкий", EFeatType::kActivated, 90, ESkill::kLeadership, ESaving::kReflex);
	feat_info[EFeat::kTactician].GetBaseParameter = &GET_REAL_CHA;
	feat_info[EFeat::kTactician].CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
//135
	InitFeat(EFeat::kLiveShield, "живой щит");
//137
	InitFeat(EFeat::kSerratedBlade, "воровская заточка", EFeatType::kActivated);
//138
	InitFeat(EFeat::kEvasion, "скользкий тип");
//139
	InitFeat(EFeat::kCutting, "порез", EFeatType::kTechnique, 110, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kCutting].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kCutting].GetEffectParameter = &GET_REAL_STR;
	feat_info[EFeat::kCutting].uses_weapon_skill = true;
	feat_info[EFeat::kCutting].always_available = false;
	feat_info[EFeat::kCutting].damage_bonus = 15;
	feat_info[EFeat::kCutting].success_degree_damage_bonus = 5;
	feat_info[EFeat::kCutting].CalcSituationalRollBonus =
		([](CharData */*ch*/, CharData * enemy) -> int {
			int bonus{0};
			if (AFF_FLAGGED(enemy, EAffect::kBlind)) {
				bonus += 40;
			}
			if (GET_POS(enemy) < EPosition::kFight) {
				bonus += 40;
			}
			return bonus;
		});

	feat_info[EFeat::kCutting].item_kits.reserve(4);

	auto item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(2);
	item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kShortBlades));
	item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kShortBlades));
	feat_info[EFeat::kCutting].item_kits.push_back(std::move(item_kit));

	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(2);
	item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kLongBlades));
	item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kLongBlades));
	feat_info[EFeat::kCutting].item_kits.push_back(std::move(item_kit));

	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(2);
	item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kSpades));
	item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kSpades));
	feat_info[EFeat::kCutting].item_kits.push_back(std::move(item_kit));

	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(2);
	item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kPicks));
	item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kPicks));
	feat_info[EFeat::kCutting].item_kits.push_back(std::move(item_kit));

//140
	InitFeat(EFeat::kFInesseShot, "ловкий выстрел");
//141
	InitFeat(EFeat::kObjectEnchanter, "наложение чар");
//142
	InitFeat(EFeat::kDeftShooter, "ловкий стрелок");
//143
	InitFeat(EFeat::kMagicShooter, "магический выстрел");
//144
	InitFeat(EFeat::kThrowWeapon, "метнуть", EFeatType::kTechnique, 100, ESkill::kThrow, ESaving::kReflex);
	feat_info[EFeat::kThrowWeapon].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kThrowWeapon].GetEffectParameter = &GET_REAL_STR;
	feat_info[EFeat::kThrowWeapon].uses_weapon_skill = false;
	feat_info[EFeat::kThrowWeapon].always_available = true;
	feat_info[EFeat::kThrowWeapon].damage_bonus = 20;
	feat_info[EFeat::kThrowWeapon].success_degree_damage_bonus = 5;
	feat_info[EFeat::kThrowWeapon].CalcSituationalDamageFactor =
		([](CharData *ch) -> int {
			return (CanUseFeat(ch, EFeat::kPowerThrow) + CanUseFeat(ch, EFeat::kDeadlyThrow));
		});
	feat_info[EFeat::kThrowWeapon].CalcSituationalRollBonus =
		([](CharData *ch, CharData * /* enemy */) -> int {
			if (AFF_FLAGGED(ch, EAffect::kBlind)) {
				return -60;
			}
			return 0;
		});
	// Это ужасно, понимаю, но не хочется возиться с написанием мутной функции с переменным числоа аргументов,
	// потому что при введении конфига, чем я планирую заняться в ближайшее время, все равно ее придется переписывать.
	//TODO: Не забыть переписать этот бордель
	feat_info[EFeat::kThrowWeapon].item_kits.reserve(2);

	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(1);
	item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon,
									  ESkill::kAny, EObjFlag::kThrowing));
	feat_info[EFeat::kThrowWeapon].item_kits.push_back(std::move(item_kit));

	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(1);
	item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon,
									  ESkill::kAny, EObjFlag::kThrowing));
	feat_info[EFeat::kThrowWeapon].item_kits.push_back(std::move(item_kit));
//145
	InitFeat(EFeat::kShadowThrower, "змеево оружие", EFeatType::kTechnique, 100, ESkill::kDarkMagic, ESaving::kWill);
	feat_info[EFeat::kShadowThrower].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kShadowThrower].GetEffectParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowThrower].damage_bonus = -30;
	feat_info[EFeat::kShadowThrower].success_degree_damage_bonus = 1;
	feat_info[EFeat::kShadowThrower].uses_weapon_skill = false;
	feat_info[EFeat::kShadowThrower].CalcSituationalRollBonus =
		([](CharData *ch, CharData * /* enemy */) -> int {
			if (AFF_FLAGGED(ch, EAffect::kBlind)) {
				return -60;
			}
			return 0;
		});

	feat_info[EFeat::kShadowThrower].item_kits.reserve(2);
	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(1);
	item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon,
									  ESkill::kAny, EObjFlag::kThrowing));
	feat_info[EFeat::kShadowThrower].item_kits.push_back(std::move(item_kit));
	item_kit = std::make_unique<TechniqueItemKit>();
	item_kit->reserve(1);
	item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon,
									  ESkill::kAny, EObjFlag::kThrowing));
	feat_info[EFeat::kShadowThrower].item_kits.push_back(std::move(item_kit));
//146
	InitFeat(EFeat::kShadowDagger, "змеев кинжал", EFeatType::kNormal, 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[EFeat::kShadowDagger].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowDagger].uses_weapon_skill = false;
//147
	InitFeat(EFeat::kShadowSpear, "змеево копьё", EFeatType::kNormal, 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[EFeat::kShadowSpear].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowSpear].uses_weapon_skill = false;
//148
	InitFeat(EFeat::kShadowClub, "змеева палица", EFeatType::kNormal, 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[EFeat::kShadowClub].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowClub].uses_weapon_skill = false;
//149
	InitFeat(EFeat::kDoubleThrower, "двойной бросок", EFeatType::kActivated, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kDoubleThrower].GetBaseParameter = &GET_REAL_DEX;
//150
	InitFeat(EFeat::kTripleThrower, "тройной бросок", EFeatType::kActivated, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kTripleThrower].GetBaseParameter = &GET_REAL_DEX;
//1151
	InitFeat(EFeat::kPowerThrow, "размах", EFeatType::kNormal, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kPowerThrow].GetBaseParameter = &GET_REAL_STR;
//152
	InitFeat(EFeat::kDeadlyThrow, "широкий размах", EFeatType::kNormal, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kDeadlyThrow].GetBaseParameter = &GET_REAL_STR;
//153
	InitFeat(EFeat::kUndeadsTurn, "turn undead", EFeatType::kTechnique, 70, ESkill::kTurnUndead, ESaving::kStability);
	feat_info[EFeat::kUndeadsTurn].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kUndeadsTurn].GetEffectParameter = &GET_REAL_WIS;
	feat_info[EFeat::kUndeadsTurn].uses_weapon_skill = false;
	feat_info[EFeat::kUndeadsTurn].always_available = true;
	feat_info[EFeat::kUndeadsTurn].damage_bonus = -30;
	feat_info[EFeat::kUndeadsTurn].success_degree_damage_bonus = 2;
//154
	InitFeat(EFeat::kMultipleCast, "изощренные чары");
//155	
	InitFeat(EFeat::kMagicalShield, "заговоренный щит");
//156
	InitFeat(EFeat::kAnimalMaster, "хозяин животных");
//157
	InitFeat(EFeat::kSlashMaster, "рубака");
//158
	InitFeat(EFeat::kPhysicians, "врачеватель");

// Не забудьде добавит фит в void init_EFeat_ITEM_NAMES()
}

bool CanUseFeat(const CharData *ch, EFeat feat) {
	if (MUD::Feat(feat).IsInvalid()) {
		return false;
	};
	if (!ch->HaveFeat(feat) && !feat_info[feat].always_available) {
		return false;
	};
	if (ch->IsNpc()) {
		return true;
	};
	if (CalcMaxFeatSlotPerLvl(ch) < MUD::Class(ch->GetClass()).feats[feat].GetSlot()) {
		return false;
	};
	if (GET_REAL_REMORT(ch) < MUD::Class(ch->GetClass()).feats[feat].GetMinRemort()) {
		return false;
	};

	switch (feat) {
		case EFeat::kWeaponFinesse:
		case EFeat::kFInesseShot: return (GET_REAL_DEX(ch) > GET_REAL_STR(ch) && GET_REAL_DEX(ch) > 17);
			break;
		case EFeat::kParryArrow: return (GET_REAL_DEX(ch) > 15);
			break;
		case EFeat::kPowerAttack: return (GET_REAL_STR(ch) > 19);
			break;
		case EFeat::kGreatPowerAttack: return (GET_REAL_STR(ch) > 21);
			break;
		case EFeat::kAimingAttack: return (GET_REAL_DEX(ch) > 15);
			break;
		case EFeat::kGreatAimingAttack: return (GET_REAL_DEX(ch) > 17);
			break;
		case EFeat::kDoubleShot: return (ch->GetSkill(ESkill::kBows) > 39);
			break;
		case EFeat::kJeweller: return (ch->GetSkill(ESkill::kJewelry) > 59);
			break;
		case EFeat::kSkilledTrader: return ((GetRealLevel(ch) + GET_REAL_REMORT(ch) / 3) > 19);
			break;
		case EFeat::kMagicUser: return (GetRealLevel(ch) < 25);
			break;
		case EFeat::kLiveShield: return (ch->GetSkill(ESkill::kRescue) > 124);
			break;
		case EFeat::kShadowThrower: return (ch->GetSkill(ESkill::kDarkMagic) > 120);
			break;
		case EFeat::kAnimalMaster: return (ch->GetSkill(ESkill::kMindMagic) > 79);
			break;
		case EFeat::kScirmisher:
			return !(AFF_FLAGGED(ch, EAffect::kStopFight)
				|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)
				|| GET_POS(ch) < EPosition::kFight);
			break;
		default: return true;
			break;
	}
	return true;
}

bool CanGetFocusFeat(const CharData *ch, const EFeat feat_id) {
	int count{0};
	for (auto focus_feat_id = EFeat::kPunchFocus; focus_feat_id <= EFeat::kBowsFocus; ++focus_feat_id) {
		if (ch->HaveFeat(focus_feat_id)) {
			count++;
		}
	}

	if (count >= 2 + GET_REAL_REMORT(ch) / 6) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchFocus:
			return ch->GetSkillWithoutEquip(ESkill::kPunch);
			break;
		case EFeat::kClubsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kClubs);
			break;
		case EFeat::kAxesFocus:
			return ch->GetSkillWithoutEquip(ESkill::kAxes);
			break;
		case EFeat::kLongsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kLongBlades);
			break;
		case EFeat::kShortsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kShortBlades);
			break;
		case EFeat::kNonstandartsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kNonstandart);
			break;
		case EFeat::kTwohandsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kTwohands);
			break;
		case EFeat::kPicksFocus:
			return ch->GetSkillWithoutEquip(ESkill::kPicks);
			break;
		case EFeat::kSpadesFocus:
			return ch->GetSkillWithoutEquip(ESkill::kSpades);
			break;
		case EFeat::kBowsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kBows);
			break;
		default:
			return false;
			break;
	}
}

bool CanGetMasterFeat(const CharData *ch, const EFeat feat_id) {
	int count{0};
	for (auto master_feat = EFeat::kPunchMaster; master_feat <= EFeat::kBowsMaster; ++master_feat) {
		if (ch->HaveFeat(master_feat)) {
			count++;
		}
	}
	if (count >= 1 + GET_REAL_REMORT(ch) / 7) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchMaster:
			return ch->HaveFeat(EFeat::kPunchFocus) && ch->GetSkillWithoutEquip(ESkill::kPunch);
			break;
		case EFeat::kClubsMaster:
			return ch->HaveFeat(EFeat::kClubsFocus) && ch->GetSkillWithoutEquip(ESkill::kClubs);
			break;
		case EFeat::kAxesMaster:
			return ch->HaveFeat(EFeat::kAxesFocus) && ch->GetSkillWithoutEquip(ESkill::kAxes);
			break;
		case EFeat::kLongsMaster:
			return ch->HaveFeat(EFeat::kLongsFocus) && ch->GetSkillWithoutEquip(ESkill::kLongBlades);
			break;
		case EFeat::kShortsMaster:
			return ch->HaveFeat(EFeat::kShortsFocus) && ch->GetSkillWithoutEquip(ESkill::kShortBlades);
			break;
		case EFeat::kNonstandartsMaster:
			return ch->HaveFeat(EFeat::kNonstandartsFocus) && ch->GetSkillWithoutEquip(ESkill::kNonstandart);
			break;
		case EFeat::kTwohandsMaster:
			return ch->HaveFeat(EFeat::kTwohandsFocus) && ch->GetSkillWithoutEquip(ESkill::kTwohands);
			break;
		case EFeat::kPicksMaster:
			return ch->HaveFeat(EFeat::kPicksFocus) && ch->GetSkillWithoutEquip(ESkill::kPicks);
			break;
		case EFeat::kSpadesMaster:
			return ch->HaveFeat(EFeat::kSpadesFocus) && ch->GetSkillWithoutEquip(ESkill::kSpades);
			break;
		case EFeat::kBowsMaster:
			return ch->HaveFeat(EFeat::kBowsFocus) && ch->GetSkillWithoutEquip(ESkill::kBows);
			break;
		default:
			return false;
			break;
	}

	return false;
}

bool CanGetFeat(CharData *ch, EFeat feat) {
	if (MUD::Feat(feat).IsInvalid()) {
		return false;
	}
	if (feat_info[feat].always_available) {
		return false;
	};

	if ((MUD::Class(ch->GetClass()).feats.IsUnavailable(feat) &&
		!PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat))) ||
		(GET_REAL_REMORT(ch) < MUD::Class(ch->GetClass()).feats[feat].GetMinRemort())) {
		return false;
	}

	if (!CheckVacantFeatSlot(ch, feat)) {
		return false;
	}

	switch (feat) {
		case EFeat::kParryArrow:
			return ch->GetSkillWithoutEquip(ESkill::kMultiparry) ||
				ch->GetSkillWithoutEquip(ESkill::kParry);
			break;
		case EFeat::kConnoiseur:
			return ch->GetSkillWithoutEquip(ESkill::kIdentify);
			break;
		case EFeat::kExorcist:
			return ch->GetSkillWithoutEquip(ESkill::kTurnUndead);
			break;
		case EFeat::kHealer:
			return ch->GetSkillWithoutEquip(ESkill::kFirstAid);
			break;
		case EFeat::kStealthy:
			return ch->GetSkillWithoutEquip(ESkill::kHide) ||
				ch->GetSkillWithoutEquip(ESkill::kSneak) ||
				ch->GetSkillWithoutEquip(ESkill::kDisguise);
			break;
		case EFeat::kTracker:
			return (ch->GetSkillWithoutEquip(ESkill::kTrack) ||
				ch->GetSkillWithoutEquip(ESkill::kSense));
			break;
		case EFeat::kPunchMaster:
		case EFeat::kClubsMaster:
		case EFeat::kAxesMaster:
		case EFeat::kLongsMaster:
		case EFeat::kShortsMaster:
		case EFeat::kNonstandartsMaster:
		case EFeat::kTwohandsMaster:
		case EFeat::kPicksMaster:
		case EFeat::kSpadesMaster: [[fallthrough]];
		case EFeat::kBowsMaster:
			return CanGetMasterFeat(ch, feat);
			break;
		case EFeat::kWarriorSpirit:
			return ch->HaveFeat(EFeat::kGreatFortitude);
			break;
		case EFeat::kNimbleFingers:
			return ch->GetSkillWithoutEquip(ESkill::kSteal) ||
				ch->GetSkillWithoutEquip(ESkill::kPickLock);
			break;
		case EFeat::kGreatPowerAttack:
			return ch->HaveFeat(EFeat::kPowerAttack);
			break;
		case EFeat::kPunchFocus:
		case EFeat::kClubsFocus:
		case EFeat::kAxesFocus:
		case EFeat::kLongsFocus:
		case EFeat::kShortsFocus:
		case EFeat::kNonstandartsFocus:
		case EFeat::kTwohandsFocus:
		case EFeat::kPicksFocus:
		case EFeat::kSpadesFocus: [[fallthrough]];
		case EFeat::kBowsFocus:
			return CanGetFocusFeat(ch, feat);
			break;
		case EFeat::kGreatAimingAttack:
			return ch->HaveFeat(EFeat::kAimingAttack);
			break;
		case EFeat::kDoubleShot:
			return ch->HaveFeat(EFeat::kBowsFocus) && ch->GetSkillWithoutEquip(ESkill::kBows) > 39;
			break;
		case EFeat::kJeweller:
			return ch->GetSkillWithoutEquip(ESkill::kJewelry) > 59;
			break;
		case EFeat::kCutting:
			return ch->HaveFeat(EFeat::kShortsMaster) ||
				ch->HaveFeat(EFeat::kPicksMaster) ||
				ch->HaveFeat(EFeat::kLongsMaster) ||
				ch->HaveFeat(EFeat::kSpadesMaster);
			break;
		case EFeat::kScirmisher:
			return ch->GetSkillWithoutEquip(ESkill::kRescue);
			break;
		case EFeat::kTactician:
			return ch->GetSkillWithoutEquip(ESkill::kLeadership) > 99;
			break;
		case EFeat::kShadowThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kDarkMagic) > 120;
			break;
		case EFeat::kShadowDagger:
		case EFeat::kShadowSpear: [[fallthrough]];
		case EFeat::kShadowClub:
			return ch->HaveFeat(EFeat::kShadowThrower) &&
				ch->GetSkillWithoutEquip(ESkill::kDarkMagic) > 130;
			break;
		case EFeat::kDoubleThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 100;
			break;
		case EFeat::kTripleThrower:
			return ch->HaveFeat(EFeat::kDeadlyThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 130;
			break;
		case EFeat::kPowerThrow:
			return ch->GetSkillWithoutEquip(ESkill::kThrow) > 90;
			break;
		case EFeat::kDeadlyThrow:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 110;
			break;
		case EFeat::kSerratedBlade:
			return ch->HaveFeat(EFeat::kCutting);
			break;
		default: return true;
			break;
	}

	return true;
}

bool CheckVacantFeatSlot(CharData *ch, EFeat feat_id) {
	if (MUD::Class(ch->GetClass()).feats[feat_id].IsInborn()
		|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat_id))) {
		return true;
	}

	//сколько у нас вообще способностей, у которых слот меньше требуемого, и сколько - тех, у которых больше или равно?
	int lowfeat = 0;
	int hifeat = 0;

	//Мы не можем просто учесть кол-во способностей меньше требуемого и больше требуемого,
	//т.к. возможны свободные слоты меньше требуемого, и при этом верхние заняты все
	auto slot_list = std::vector<int>();
	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (feat.IsInborn() || PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat.GetId()))) {
			continue;
		}

		if (ch->HaveFeat(feat.GetId())) {
			if (feat.GetSlot() >= MUD::Class(ch->GetClass()).feats[feat_id].GetSlot()) {
				++hifeat;
			} else {
				slot_list.push_back(feat.GetSlot());
			}
		}
	}

	std::sort(slot_list.begin(), slot_list.end());

	//Посчитаем сколько действительно нижние способности занимают слотов (с учетом пропусков)
	for (const auto &slot: slot_list) {
		if (lowfeat < slot) {
			lowfeat = slot + 1;
		} else {
			++lowfeat;
		}
	}

	//из имеющегося количества слотов нужно вычесть:
	//число высоких слотов, занятых низкоуровневыми способностями,
	//с учетом, что низкоуровневые могут и не занимать слотов выше им положенных,
	//а также собственно число слотов, занятых высокоуровневыми способностями
	if (CalcMaxFeatSlotPerLvl(ch) - MUD::Class(ch->GetClass()).feats[feat_id].GetSlot() -
		hifeat - std::max(0, lowfeat - MUD::Class(ch->GetClass()).feats[feat_id].GetSlot()) > 0) {
		return true;
	}

	//oops.. слотов нет
	return false;
}

// \todo Надо как-то переделать загрузку родовых способностей, чтобы там было не int, а сразу EFeat
void SetRaceFeats(CharData *ch) {
	auto race_features = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int i : race_features) {
		auto feat_id = static_cast<EFeat>(i);
		if (CanGetFeat(ch, feat_id)) {
			ch->SetFeat(feat_id);
		}
	}
}

void UnsetRaceFeats(CharData *ch) {
	auto race_features = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int i : race_features) {
		ch->UnsetFeat(static_cast<EFeat>(i));
	}
}

void SetInbornFeats(CharData *ch) {
	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (feat.IsInborn() && !ch->HaveFeat(feat.GetId()) && CanGetFeat(ch, feat.GetId())) {
			ch->SetFeat(feat.GetId());
		}
	}
}

void SetInbornAndRaceFeats(CharData *ch) {
	SetInbornFeats(ch);
	SetRaceFeats(ch);
}

void UnsetInaccessibleFeats(CharData *ch) {
	for (auto feat_id = EFeat::kFirst; feat_id <= EFeat::kLast; ++feat_id) {
		if (ch->HaveFeat(feat_id)) {
			if (MUD::Class(ch->GetClass()).feats.IsUnavailable(feat_id)) {
				ch->UnsetFeat(feat_id);
			}
		}
	}
}

bool TryFlipActivatedFeature(CharData *ch, char *argument) {
	auto feat_id = FindFeatId(argument);
	if (feat_id <= EFeat::kUndefined) {
		return false;
	}
	if (!CheckAccessActivatedFeat(ch, feat_id)) {
		return true;
	};
	if (PRF_FLAGGED(ch, GetPrfWithFeatNumber(feat_id))) {
		DeactivateFeature(ch, feat_id);
	} else {
		ActivateFeat(ch, feat_id);
	}

	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	return true;
}

void ActivateFeat(CharData *ch, EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformPowerAttack);
			break;
		case EFeat::kGreatPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformGreatPowerAttack);
			break;
		case EFeat::kAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformAimingAttack);
			break;
		case EFeat::kGreatAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformGreatAimingAttack);
			break;
		case EFeat::kSerratedBlade: SendMsgToChar("Вы перехватили свои клинки особым хватом.\r\n", ch);
			act("$n0 ловко прокрутил$g между пальцев свои клинки.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			PRF_FLAGS(ch).set(EPrf::kPerformSerratedBlade);
			break;
		case EFeat::kScirmisher:
			if (!AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar(ch,
							  "Голос десятника Никифора вдруг рявкнул: \"%s, тюрюхайло! 'В шеренгу по одному' иначе сполняется!\"\r\n",
							  ch->get_name().c_str());
				return;
			}
			if (PRF_FLAGGED(ch, EPrf::kSkirmisher)) {
				SendMsgToChar("Вы уже стоите в передовом строю.\r\n", ch);
				return;
			}
			PRF_FLAGS(ch).set(EPrf::kSkirmisher);
			SendMsgToChar("Вы протиснулись вперед и встали в строй.\r\n", ch);
			act("$n0 протиснул$u вперед и встал$g в строй.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			break;
		case EFeat::kDoubleThrower: PRF_FLAGS(ch).unset(EPrf::kTripleThrow);
			PRF_FLAGS(ch).set(EPrf::kDoubleThrow);
			break;
		case EFeat::kTripleThrower: PRF_FLAGS(ch).unset(EPrf::kDoubleThrow);
			PRF_FLAGS(ch).set(EPrf::kTripleThrow);
			break;
		default: break;
	}
	SendMsgToChar(ch,
				  "%sВы решили использовать способность '%s'.%s\r\n",
				  CCIGRN(ch, C_SPR),
				  MUD::Feat(feat_id).GetCName(),
				  CCNRM(ch, C_OFF));
}

void DeactivateFeature(CharData *ch, EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			break;
		case EFeat::kGreatPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			break;
		case EFeat::kAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			break;
		case EFeat::kGreatAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			break;
		case EFeat::kSerratedBlade: SendMsgToChar("Вы ловко прокрутили свои клинки в обычный прямой хват.\r\n", ch);
			act("$n0 ловко прокрутил$g между пальцев свои клинки.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			PRF_FLAGS(ch).unset(EPrf::kPerformSerratedBlade);
			break;
		case EFeat::kScirmisher: PRF_FLAGS(ch).unset(EPrf::kSkirmisher);
			if (AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar("Вы решили, что в обозе вам будет спокойней.\r\n", ch);
				act("$n0 тактически отступил$g в тыл отряда.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			}
			break;
		case EFeat::kDoubleThrower: PRF_FLAGS(ch).unset(EPrf::kDoubleThrow);
			break;
		case EFeat::kTripleThrower: PRF_FLAGS(ch).unset(EPrf::kTripleThrow);
			break;
		default: break;
	}
	SendMsgToChar(ch, "%sВы прекратили использовать способность '%s'.%s\r\n",
				  CCIGRN(ch, C_SPR), MUD::Feat(feat_id).GetCName(), CCNRM(ch, C_OFF));
}

bool CheckAccessActivatedFeat(CharData *ch, EFeat feat_id) {
	if (!CanUseFeat(ch, feat_id)) {
		SendMsgToChar("Вы не в состоянии использовать эту способность.\r\n", ch);
		return false;
	}
	if (feat_info[feat_id].type != EFeatType::kActivated) {
		SendMsgToChar("Эту способность невозможно применить таким образом.\r\n", ch);
		return false;
	}

	return true;
}

EFeat FindWeaponMasterFeat(ESkill skill) {
	switch (skill) {
		case ESkill::kPunch: return EFeat::kPunchMaster;
			break;
		case ESkill::kClubs: return EFeat::kClubsMaster;
			break;
		case ESkill::kAxes: return EFeat::kAxesMaster;
			break;
		case ESkill::kLongBlades: return EFeat::kLongsMaster;
			break;
		case ESkill::kShortBlades: return EFeat::kShortsMaster;
			break;
		case ESkill::kNonstandart: return EFeat::kNonstandartsMaster;
			break;
		case ESkill::kTwohands: return EFeat::kTwohandsMaster;
			break;
		case ESkill::kPicks: return EFeat::kPicksMaster;
			break;
		case ESkill::kSpades: return EFeat::kSpadesMaster;
			break;
		case ESkill::kBows: return EFeat::kBowsMaster;
			break;
		default: return EFeat::kUndefined;
	}
}

/*
 TODO: при переписывании способностей переделать на композицию или интерфейс
*/
Bitvector GetPrfWithFeatNumber(EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: return EPrf::kPerformPowerAttack;
			break;
		case EFeat::kGreatPowerAttack: return EPrf::kPerformGreatPowerAttack;
			break;
		case EFeat::kAimingAttack: return EPrf::kPerformAimingAttack;
			break;
		case EFeat::kGreatAimingAttack: return EPrf::kPerformGreatAimingAttack;
			break;
		case EFeat::kSerratedBlade: return EPrf::kPerformSerratedBlade;
			break;
		case EFeat::kScirmisher: return EPrf::kSkirmisher;
			break;
		case EFeat::kDoubleThrower: return EPrf::kDoubleThrow;
			break;
		case EFeat::kTripleThrower: return EPrf::kTripleThrow;
			break;
		default: break;
	}

	return EPrf::kPerformPowerAttack;
}

/*
* Ситуативный бонус броска для "tactician feat" и "skirmisher feat":
* Каждый персонаж в строю прикрывает двух, третий дает штраф.
* Избыток "строевиков" повышает шанс на удачное срабатывание.
* Svent TODO: Придумать более универсальный механизм бонусов/штрафов в зависимости от данных абилки
*/
int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */) {
	ActionTargeting::FriendsRosterType roster{ch};
	int skirmishers = roster.count([](CharData *ch) { return PRF_FLAGGED(ch, EPrf::kSkirmisher); });
	int uncoveredSquadMembers = roster.amount() - skirmishers;
	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		return (skirmishers*2 - uncoveredSquadMembers)*abilities::kCircumstanceFactor - 40;
	};
	return (skirmishers*2 - uncoveredSquadMembers)*abilities::kCircumstanceFactor;
};

int CalcMaxFeatSlotPerLvl(const CharData *ch) {
	return (kMinBaseFeatsSlotsAmount + GetRealLevel(ch)*(kMaxBaseFeatsSlotsAmount - 1 +
		GET_REAL_REMORT(ch)/ MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot())/kLastFeatSlotLvl);
}

int CalcFeatSlotsAmountPerRemort(CharData *ch) {
	auto result = kMinBaseFeatsSlotsAmount + kMaxPlayerLevel*(kMaxBaseFeatsSlotsAmount - 1 +
		ch->get_remort()/ MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot())/kLastFeatSlotLvl;
	return result;
}

namespace feats {

using ItemPtr = FeatInfoBuilder::ItemPtr;

void FeatsLoader::Load(DataNode data) {
	MUD::Feats().Init(data.Children());
}

void FeatsLoader::Reload(DataNode data) {
	MUD::Feats().Reload(data.Children());
}

ItemPtr FeatInfoBuilder::Build(DataNode &node) {
	try {
		return ParseFeat(node);
	} catch (std::exception &e) {
		err_log("Feat parsing error (incorrect value '%s')", e.what());
		return nullptr;
	}
}

ItemPtr FeatInfoBuilder::ParseFeat(DataNode &node) {
	auto info = ParseHeader(node);

	ParseEffects(info, node);
	return info;
}

ItemPtr FeatInfoBuilder::ParseHeader(DataNode &node) {
	auto id{EFeat::kUndefined};
	try {
		id = parse::ReadAsConstant<EFeat>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect feat id (%s).", e.what());
		throw;
	}
	auto mode = FeatInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	auto info = std::make_shared<FeatInfo>(id, mode);
	try {
		info->name_ = parse::ReadAsStr(node.GetValue("name"));
	} catch (std::exception &) {
	}

	return info;
}

void FeatInfoBuilder::ParseEffects(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("effects")) {
		info->effects.Build(node);
		node.GoToParent();
	}
}

void FeatInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print feat:" << std::endl
		   << " Id: " << KGRN << NAME_BY_ITEM<EFeat>(GetId()) << KNRM << std::endl
		   << " Name: " << KGRN << GetName() << KNRM << std::endl
		   << " Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(GetMode()) << KNRM << std::endl;

	effects.Print(ch, buffer);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
