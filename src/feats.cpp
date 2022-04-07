/*************************************************************************
*   File: features.cpp                                 Part of Bylins    *
*   Features code                                                        *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                	                                 *
************************************************************************ */

//#include "feats.h"

#include "abilities/abilities_constants.h"
#include "action_targeting.h"
#include "handler.h"
#include "entities/player_races.h"
#include "color.h"
#include "fightsystem/pk.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>

//using namespace abilities;

extern const char *unused_spellname;

struct FeatureInfo feat_info[EFeat::kLastFeat + 1];

/* Служебные функции */
void InitFeatByDefault(EFeat feat_id);
bool CheckVacantFeatSlot(CharData *ch, EFeat feat);

/* Функции для работы с переключаемыми способностями */
bool CheckAccessActivatedFeat(CharData *ch, EFeat feat_id);
void ActivateFeat(CharData *ch, EFeat feat_id);
void DeactivateFeature(CharData *ch, EFeat feat_id);
EFeat GetFeatureNum(char *feat_name);

/* Ситуативные бонусы, пишутся для специфических способностей по потребности */
int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */);

/* Активные способности */
void do_lightwalk(CharData *ch, char *argument, int cmd, int subcmd);

/* Extern */
extern void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);

EFeat& operator++(EFeat &f) {
	f =  static_cast<EFeat>(to_underlying(f) + 1);
	return f;
};

///
/// Поиск номера способности по имени
/// \param alias = false
/// true для поиска при вводе имени способности игроком у учителей
///
EFeat FindFeatNum(const char *name, bool alias) {
	for (auto index = EFeat::kFirstFeat; index <= EFeat::kLastFeat; ++index) {
		bool flag = true;
		std::string name_feat(alias ? feat_info[index].alias.c_str() : feat_info[index].name);
		std::vector<std::string> strs_feat, strs_args;
		boost::split(strs_feat, name_feat, boost::is_any_of(" "));
		boost::split(strs_args, name, boost::is_any_of(" "));
		const int bound = static_cast<int>(strs_feat.size() >= strs_args.size()
										   ? strs_args.size()
										   : strs_feat.size());
		for (int i = 0; i < bound; i++) {
			if (!boost::starts_with(strs_feat[i], strs_args[i])) {
				flag = false;
			}
		}
		if (flag) {
			return index;
		}
	}
	return EFeat::kIncorrectFeat;
}

void InitFeat(EFeat feat, const char *name, EFeatType type, bool can_up_slot, CFeatArray app,
			  int roll_bonus = abilities::kMaxRollBonus, ESkill base_skill = ESkill::kIncorrect,
			  ESaving saving = ESaving::kStability) {
	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			feat_info[feat].min_remort[i][j] = 0;
			feat_info[feat].slot[i][j] = 0;
		}
	}
	if (name) {
		feat_info[feat].name = name;
		std::string alias(name);
		std::replace_if(alias.begin(), alias.end(), boost::is_any_of("_:"), ' ');
		boost::trim_all(alias);
		feat_info[feat].alias = alias;
	}
	feat_info[feat].id = feat;
	feat_info[feat].diceroll_bonus = roll_bonus;
	feat_info[feat].base_skill = base_skill;
	feat_info[feat].saving = saving;
	feat_info[feat].type = type;
	feat_info[feat].up_slot = can_up_slot;
	for (i = 0; i < kMaxFeatAffect; i++) {
		feat_info[feat].affected[i].location = app.affected[i].location;
		feat_info[feat].affected[i].modifier = app.affected[i].modifier;
	}
}

void InitFeatByDefault(EFeat feat_id) {
	int i, j;

	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			feat_info[feat_id].min_remort[i][j] = 0;
			feat_info[feat_id].slot[i][j] = 0;
			feat_info[feat_id].is_inborn[i][j] = false;
			feat_info[feat_id].is_known[i][j] = false;
		}
	}

	feat_info[feat_id].id = feat_id;
	feat_info[feat_id].name = unused_spellname;
	feat_info[feat_id].type = EFeatType::kUnused;
	feat_info[feat_id].up_slot = false;
	feat_info[feat_id].always_available = false;
	feat_info[feat_id].uses_weapon_skill = false;
	feat_info[feat_id].damage_bonus = 0;
	feat_info[feat_id].success_degree_damage_bonus = 5;
	feat_info[feat_id].saving = ESaving::kStability;
	feat_info[feat_id].diceroll_bonus = abilities::kMaxRollBonus;
	feat_info[feat_id].base_skill = ESkill::kIncorrect;
	feat_info[feat_id].critfail_threshold = abilities::kDefaultCritfailThreshold;
	feat_info[feat_id].critsuccess_threshold = abilities::kDefaultCritsuccessThreshold;

	for (i = 0; i < kMaxFeatAffect; i++) {
		feat_info[feat_id].affected[i].location = EApply::kNone;
		feat_info[feat_id].affected[i].modifier = 0;
	}

	feat_info[feat_id].GetBaseParameter = &GET_REAL_INT;
	feat_info[feat_id].GetEffectParameter = &GET_REAL_STR;
	feat_info[feat_id].CalcSituationalDamageFactor =
		([](CharData *) -> float {
			return 1.00;
		});
	feat_info[feat_id].CalcSituationalRollBonus =
		([](CharData *, CharData *) -> int {
			return 0;
		});
}

void InitFeatures() {
	CFeatArray feat_app;
	for (auto i = EFeat::kBerserker; i <= EFeat::kLastFeat; ++i) {
		InitFeatByDefault(i);
	}
//1
	InitFeat(EFeat::kBerserker, "предсмертная ярость", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//2
	InitFeat(EFeat::kParryArrow, "отбить стрелу", EFeatType::kNormal, true, feat_app);
//3
	InitFeat(EFeat::kBlindFight, "слепой бой", EFeatType::kNormal, true, feat_app);
//4
	feat_app.insert(EApply::kMagicResist, 1);
	feat_app.insert(EApply::kAffectResist, 1);
	InitFeat(EFeat::kImpregnable, "непробиваемый", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//5-*
	InitFeat(EFeat::kApproachingAttack, "встречная атака", EFeatType::kNormal, true, feat_app);
//6
	InitFeat(EFeat::kDefender, "щитоносец", EFeatType::kNormal, true, feat_app);
//7
	InitFeat(EFeat::kDodger, "изворотливость", EFeatType::kAffect, true, feat_app);
//8
	InitFeat(EFeat::kLightWalk, "легкая поступь", EFeatType::kNormal, true, feat_app);
//9
	InitFeat(EFeat::kWriggler, "проныра", EFeatType::kNormal, true, feat_app);
//10
	InitFeat(EFeat::kSpellSubstitute, "подмена заклинания", EFeatType::kNormal, true, feat_app);
//11
	InitFeat(EFeat::kPowerAttack, "мощная атака", EFeatType::kActivated, true, feat_app);
//12
	feat_app.insert(EApply::kResistFire, 5);
	feat_app.insert(EApply::kResistAir, 5);
	feat_app.insert(EApply::kResistWater, 5);
	feat_app.insert(EApply::kResistEarth, 5);
	feat_app.insert(EApply::kResistDark, 5);
	InitFeat(EFeat::kWoodenSkin, "деревянная кожа", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//13
	feat_app.insert(EApply::kResistFire, 10);
	feat_app.insert(EApply::kResistAir, 10);
	feat_app.insert(EApply::kResistWater, 10);
	feat_app.insert(EApply::kResistEarth, 10);
	feat_app.insert(EApply::kResistDark, 10);
	feat_app.insert(EApply::kAbsorbe, 5);
	InitFeat(EFeat::kIronSkin, "железная кожа", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//14
	feat_app.insert(kFeatTimer, 8);
	InitFeat(EFeat::kConnoiseur, "знаток", EFeatType::kSkillMod, true, feat_app);
	feat_app.clear();
//15
	InitFeat(EFeat::kExorcist, "изгоняющий нежить", EFeatType::kSkillMod, true, feat_app);
//16
	InitFeat(EFeat::kHealer, "целитель", EFeatType::kNormal, true, feat_app);
//17
	feat_app.insert(EApply::kSavingReflex, -10);
	InitFeat(EFeat::kLightingReflex, "мгновенная реакция", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//18
	feat_app.insert(kFeatTimer, 8);
	InitFeat(EFeat::kDrunkard, "пьяница", EFeatType::kSkillMod, true, feat_app);
	feat_app.clear();
//19
	InitFeat(EFeat::kPowerMagic, "мощь колдовства", EFeatType::kNormal, true, feat_app);
//20
	feat_app.insert(EApply::kMoveRegen, 40);
	InitFeat(EFeat::kEndurance, "выносливость", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//21
	feat_app.insert(EApply::kSavingWill, -10);
	feat_app.insert(EApply::kSavingStability, -10);
	InitFeat(EFeat::kGreatFortitude, "сила духа", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//22
	feat_app.insert(EApply::kHpRegen, 35);
	InitFeat(EFeat::kFastRegen, "быстрое заживление", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//23
	InitFeat(EFeat::kStealthy, "незаметность", EFeatType::kSkillMod, true, feat_app);
//24
	feat_app.insert(EApply::kCastSuccess, 80);
	InitFeat(EFeat::kRelatedToMagic, "магическое родство", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//25
	feat_app.insert(EApply::kHpRegen, 10);
	feat_app.insert(EApply::kSavingCritical, -4);
	InitFeat(EFeat::kSplendidHealth, "богатырское здоровье", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//26
	InitFeat(EFeat::kTracker, "следопыт", EFeatType::kSkillMod, true, feat_app);
	feat_app.clear();
//27
	InitFeat(EFeat::kWeaponFinesse, "ловкий удар", EFeatType::kNormal, true, feat_app);
//28
	InitFeat(EFeat::kCombatCasting, "боевое колдовство", EFeatType::kNormal, true, feat_app);
//29
	feat_app.insert(EFeat::kPunchFocus, 1);
	InitFeat(EFeat::kPunchMaster, "мастер кулачного боя", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//30
	feat_app.insert(EFeat::kClubsFocus, 1);
	InitFeat(EFeat::kClubsMaster, "мастер палицы", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//31
	feat_app.insert(EFeat::kAxesFocus, 1);
	InitFeat(EFeat::kAxesMaster, "мастер секиры", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//32
	feat_app.insert(EFeat::kLongsFocus, 1);
	InitFeat(EFeat::kLongsMaster, "мастер меча", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//33
	feat_app.insert(EFeat::kShortsFocus, 1);
	InitFeat(EFeat::kShortsMaster, "мастер ножа", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//34
	feat_app.insert(EFeat::kNonstandartsFocus, 1);
	InitFeat(EFeat::kNonstandartsMaster, "мастер необычного оружия", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//35
	feat_app.insert(EFeat::kTwohandsFocus, 1);
	InitFeat(EFeat::kTwohandsMaster, "мастер двуручника", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//36
	feat_app.insert(EFeat::kPicksFocus, 1);
	InitFeat(EFeat::kPicksMaster, "мастер кинжала", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//37
	feat_app.insert(EFeat::kSpadesFocus, 1);
	InitFeat(EFeat::kSpadesMaster, "мастер копья", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//38
	feat_app.insert(EFeat::kBowsFocus, 1);
	InitFeat(EFeat::kBowsMaster, "мастер лучник", EFeatType::kNormal, true, feat_app);
	feat_app.clear();
//39
	InitFeat(EFeat::kForestPath, "лесные тропы", EFeatType::kNormal, true, feat_app);
//40
	InitFeat(EFeat::kMountainPath, "горные тропы", EFeatType::kNormal, true, feat_app);
//41
	feat_app.insert(EApply::kMorale, 5);
	InitFeat(EFeat::kLuckyGuy, "счастливчик", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//42
	InitFeat(EFeat::kWarriorSpirit, "боевой дух", EFeatType::kNormal, true, feat_app);
//43
	feat_app.insert(EApply::kHpRegen, 50);
	InitFeat(EFeat::kReliableHealth, "крепкое здоровье", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//44
	feat_app.insert(EApply::kMamaRegen, 100);
	InitFeat(EFeat::kExcellentMemory, "превосходная память", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//45
	feat_app.insert(EApply::kDex, 1);
	InitFeat(EFeat::kAnimalDextery, "звериная прыть", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//46
	feat_app.insert(EApply::kMamaRegen, 25);
	InitFeat(EFeat::kLegibleWritting, "чёткий почерк", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//47
	feat_app.insert(EApply::kDamroll, 2);
	InitFeat(EFeat::kIronMuscles, "стальные мышцы", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//48
	feat_app.insert(EApply::kCastSuccess, 5);
	InitFeat(EFeat::kMagicSign, "знак чародея", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//49
	feat_app.insert(EApply::kMoveRegen, 75);
	InitFeat(EFeat::kGreatEndurance, "двужильность", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//50
	feat_app.insert(EApply::kMorale, 5);
	InitFeat(EFeat::kBestDestiny, "лучшая доля", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//51
	InitFeat(EFeat::kHerbalist, "травник", EFeatType::kNormal, true, feat_app);
//52
	InitFeat(EFeat::kJuggler, "жонглер", EFeatType::kNormal, true, feat_app);
//53
	InitFeat(EFeat::kNimbleFingers, "ловкач", EFeatType::kSkillMod, true, feat_app);
//54
	InitFeat(EFeat::kGreatPowerAttack, "улучшенная мощная атака", EFeatType::kActivated, true, feat_app);
//55
	feat_app.insert(EApply::kResistImmunity, 15);
	InitFeat(EFeat::kStrongImmunity, "привычка к яду", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//56
	feat_app.insert(EApply::kAc, -40);
	InitFeat(EFeat::kMobility, "подвижность", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//57
	feat_app.insert(EApply::kStr, 1);
	InitFeat(EFeat::kNaturalStr, "силач", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//58
	feat_app.insert(EApply::kDex, 1);
	InitFeat(EFeat::kNaturalDex, "проворство", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//59
	feat_app.insert(EApply::kInt, 1);
	InitFeat(EFeat::kNaturalInt, "природный ум", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//60
	feat_app.insert(EApply::kWis, 1);
	InitFeat(EFeat::kNaturalWis, "мудрец", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//61
	feat_app.insert(EApply::kCon, 1);
	InitFeat(EFeat::kNaturalCon, "здоровяк", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//62
	feat_app.insert(EApply::kCha, 1);
	InitFeat(EFeat::kNaturalCha, "природное обаяние", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//63
	feat_app.insert(EApply::kMamaRegen, 25);
	InitFeat(EFeat::kMnemonicEnhancer, "отличная память", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//64 -*
	InitFeat(EFeat::kMagneticPersonality, "предводитель", EFeatType::kSkillMod, true, feat_app);
//65
	feat_app.insert(EApply::kDamroll, 2);
	InitFeat(EFeat::kDamrollBonus, "тяжел на руку", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//66
	feat_app.insert(EApply::kHitroll, 1);
	InitFeat(EFeat::kHitrollBonus, "твердая рука", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//67
	feat_app.insert(EApply::kCastSuccess, 30);
	InitFeat(EFeat::kMagicalInstinct, "магическое чутье", EFeatType::kAffect, true, feat_app);
	feat_app.clear();
//68
	InitFeat(EFeat::kPunchFocus, "любимое_оружие: голые руки", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kPunch);
//69
	InitFeat(EFeat::kClubsFocus, "любимое_оружие: палица", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kClubs);
//70
	InitFeat(EFeat::kAxesFocus, "любимое_оружие: секира", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kAxes);
//71
	InitFeat(EFeat::kLongsFocus, "любимое_оружие: меч", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kLongBlades);
//72
	InitFeat(EFeat::kShortsFocus, "любимое_оружие: нож", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kShortBlades);
//73
	InitFeat(EFeat::kNonstandartsFocus, "любимое_оружие: необычное", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kNonstandart);
//74
	InitFeat(EFeat::kTwohandsFocus, "любимое_оружие: двуручник", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kTwohands);
//75
	InitFeat(EFeat::kPicksFocus, "любимое_оружие: кинжал", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kPicks);
//76
	InitFeat(EFeat::kSpadesFocus, "любимое_оружие: копье", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kSpades);
//77
	InitFeat(EFeat::kBowsFocus, "любимое_оружие: лук", EFeatType::kSkillMod, true,
			 feat_app, 0, ESkill::kBows);
//78
	InitFeat(EFeat::kAimingAttack, "прицельная атака", EFeatType::kActivated, true, feat_app);
//79
	InitFeat(EFeat::kGreatAimingAttack, "улучшенная прицельная атака", EFeatType::kActivated, true, feat_app);
//80
	InitFeat(EFeat::kDoubleshot, "двойной выстрел", EFeatType::kNormal, true, feat_app);
//81
	InitFeat(EFeat::kPorter, "тяжеловоз", EFeatType::kNormal, true, feat_app);
//82
	InitFeat(EFeat::kSecretRunes, "тайные руны", EFeatType::kNormal, true, feat_app);
/*
//83
	UNUSED
//84
	UNUSED
//85
	UNUSED
	*/
//86
	InitFeat(EFeat::kToFitItem, "переделать", EFeatType::kNormal, true, feat_app);
//87
	InitFeat(EFeat::kToFitClotches, "перешить", EFeatType::kNormal, true, feat_app);
//88
	InitFeat(EFeat::kStrengthConcentration, "концентрация силы", EFeatType::kNormal, true, feat_app);
//89
	InitFeat(EFeat::kDarkReading, "кошачий глаз", EFeatType::kNormal, true, feat_app);
//90
	InitFeat(EFeat::kSpellCapabler, "зачаровать", EFeatType::kNormal, true, feat_app);
//91
	InitFeat(EFeat::kWearingLightArmor, "легкие доспехи", EFeatType::kNormal, true, feat_app);
//92
	InitFeat(EFeat::kWearingMediumArmor, "средние доспехи", EFeatType::kNormal, true, feat_app);
//93
	InitFeat(EFeat::kWearingHeavyArmor, "тяжелые доспехи", EFeatType::kNormal, true, feat_app);
//94
	InitFeat(EFeat::kGemsInlay, "инкрустация", EFeatType::kNormal, true, feat_app);
//95
	InitFeat(EFeat::kWarriorStrength, "богатырская сила", EFeatType::kNormal, true, feat_app);
//96
	InitFeat(EFeat::kRelocate, "переместиться", EFeatType::kNormal, true, feat_app);
//97
	InitFeat(EFeat::kSilverTongue, "сладкоречие", EFeatType::kNormal, true, feat_app);
//98
	InitFeat(EFeat::kBully, "забияка", EFeatType::kNormal, true, feat_app);
//99
	InitFeat(EFeat::kThieveStrike, "воровской удар", EFeatType::kNormal, true, feat_app);
//100
	InitFeat(EFeat::kJeweller, "искусный ювелир", EFeatType::kNormal, true, feat_app);
//101
	InitFeat(EFeat::kSkilledTrader, "торговая сметка", EFeatType::kNormal, true, feat_app);
//102
	InitFeat(EFeat::kZombieDrover, "погонщик умертвий", EFeatType::kNormal, true, feat_app);
//103
	InitFeat(EFeat::kEmployer, "навык найма", EFeatType::kNormal, true, feat_app);
//104
	InitFeat(EFeat::kMagicUser, "использование амулетов", EFeatType::kNormal, true, feat_app);
//105
	InitFeat(EFeat::kGoldTongue, "златоуст", EFeatType::kNormal, true, feat_app);
//106
	InitFeat(EFeat::kCalmness, "хладнокровие", EFeatType::kNormal, true, feat_app);
//107
	InitFeat(EFeat::kRetreat, "отступление", EFeatType::kNormal, true, feat_app);
//108
	InitFeat(EFeat::kShadowStrike, "танцующая тень", EFeatType::kNormal, true, feat_app);
//109
	InitFeat(EFeat::kThrifty, "запасливость", EFeatType::kNormal, true, feat_app);
//110
	InitFeat(EFeat::kCynic, "циничность", EFeatType::kNormal, true, feat_app);
//111
	InitFeat(EFeat::kPartner, "напарник", EFeatType::kNormal, true, feat_app);
//112
	InitFeat(EFeat::kFavorOfDarkness, "помощь тьмы", EFeatType::kNormal, true, feat_app);
//113
	InitFeat(EFeat::kFuryOfDarkness, "ярость тьмы", EFeatType::kNormal, true, feat_app);
//114
	InitFeat(EFeat::kRegenOfDarkness, "темное восстановление", EFeatType::kNormal, true, feat_app);
//115
	InitFeat(EFeat::kSoulLink, "родство душ", EFeatType::kNormal, true, feat_app);
//116
	InitFeat(EFeat::kStrongClutch, "сильная хватка", EFeatType::kNormal, true, feat_app);
//117
	InitFeat(EFeat::kMagicArrows, "магические стрелы", EFeatType::kNormal, true, feat_app);
//118
	InitFeat(EFeat::kSoulsCollector, "колекционер душ", EFeatType::kNormal, true, feat_app);
//119
	InitFeat(EFeat::kDarkPact, "темная сделка", EFeatType::kNormal, true, feat_app);
//120
	InitFeat(EFeat::kCorruption, "порча", EFeatType::kNormal, true, feat_app);
//121
	InitFeat(EFeat::kHarvestOfLife, "жатва жизни", EFeatType::kNormal, true, feat_app);
//122
	InitFeat(EFeat::kLoyalAssist, "верный помощник", EFeatType::kNormal, true, feat_app);
//123
	InitFeat(EFeat::kHauntingSpirit, "блуждающий дух", EFeatType::kNormal, true, feat_app);
//124
	InitFeat(EFeat::kSnakeRage, "ярость змеи", EFeatType::kNormal, true, feat_app);
//126
	InitFeat(EFeat::kElderTaskmaster, "старший надсмотрщик", EFeatType::kNormal, true, feat_app);
//127
	InitFeat(EFeat::kLordOfUndeads, "повелитель нежити", EFeatType::kNormal, true, feat_app);
//128
	InitFeat(EFeat::kWarlock, "темный маг", EFeatType::kNormal, true, feat_app);
//129
	InitFeat(EFeat::kElderPriest, "старший жрец", EFeatType::kNormal, true, feat_app);
//130
	InitFeat(EFeat::kHighLich, "верховный лич", EFeatType::kNormal, true, feat_app);
//131
	InitFeat(EFeat::kDarkRitual, "темный ритуал", EFeatType::kNormal, true, feat_app);
//132
	InitFeat(EFeat::kTeamsterOfUndead, "погонщик нежити", EFeatType::kNormal, true, feat_app);
//133
	InitFeat(EFeat::kScirmisher, "держать строй", EFeatType::kActivated, true, feat_app,
			 90, ESkill::kRescue, ESaving::kReflex);
	feat_info[EFeat::kScirmisher].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kScirmisher].CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
//134
	InitFeat(EFeat::kTactician, "десяцкий", EFeatType::kActivated, true, feat_app, 90, ESkill::kLeadership, ESaving::kReflex);
	feat_info[EFeat::kTactician].GetBaseParameter = &GET_REAL_CHA;
	feat_info[EFeat::kTactician].CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
//135
	InitFeat(EFeat::kLiveShield, "живой щит", EFeatType::kNormal, true, feat_app);
// === Проскок номеров (типа резерв под татей) ===
//138
	InitFeat(EFeat::kEvasion, "скользкий тип", EFeatType::kNormal, true, feat_app);
//139
	InitFeat(EFeat::kCutting, "порез", EFeatType::kTechnique, true, feat_app,
			 90, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kCutting].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kCutting].GetEffectParameter = &GET_REAL_STR;
	feat_info[EFeat::kCutting].uses_weapon_skill = true;
	feat_info[EFeat::kCutting].always_available = false;
	feat_info[EFeat::kCutting].damage_bonus = 3;
	feat_info[EFeat::kCutting].success_degree_damage_bonus = 7;
	feat_info[EFeat::kCutting].CalcSituationalDamageFactor = ([](CharData */*ch*/) -> float { return 1.0; });
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
	InitFeat(EFeat::kFInesseShot, "ловкий выстрел", EFeatType::kNormal, true, feat_app);
//141
	InitFeat(EFeat::kObjectEnchanter, "наложение чар", EFeatType::kNormal, true, feat_app);
//142
	InitFeat(EFeat::kDeftShooter, "ловкий стрелок", EFeatType::kNormal, true, feat_app);
//143
	InitFeat(EFeat::kMagicShooter, "магический выстрел", EFeatType::kNormal, true, feat_app);
//144
	InitFeat(EFeat::kThrowWeapon, "метнуть", EFeatType::kTechnique, true, feat_app,
			 100, ESkill::kThrow, ESaving::kReflex);
	feat_info[EFeat::kThrowWeapon].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EFeat::kThrowWeapon].GetEffectParameter = &GET_REAL_STR;
	feat_info[EFeat::kThrowWeapon].uses_weapon_skill = false;
	feat_info[EFeat::kThrowWeapon].always_available = true;
	feat_info[EFeat::kThrowWeapon].damage_bonus = 5;
	feat_info[EFeat::kThrowWeapon].success_degree_damage_bonus = 5;
	feat_info[EFeat::kThrowWeapon].CalcSituationalDamageFactor =
		([](CharData *ch) -> float {
			return static_cast<float>(0.1* IsAbleToUseFeat(ch, EFeat::kPowerThrow) +
				0.1* IsAbleToUseFeat(ch, EFeat::kDeadlyThrow));
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
	InitFeat(EFeat::kShadowThrower, "змеево оружие", EFeatType::kTechnique, true, feat_app,
			 100, ESkill::kDarkMagic, ESaving::kWill);
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
	InitFeat(EFeat::kShadowDagger, "змеев кинжал", EFeatType::kNormal, true, feat_app,
			 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[EFeat::kShadowDagger].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowDagger].uses_weapon_skill = false;
//147
	InitFeat(EFeat::kShadowSpear, "змеево копьё", EFeatType::kNormal, true,
			 feat_app, 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[EFeat::kShadowSpear].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowSpear].uses_weapon_skill = false;
//148
	InitFeat(EFeat::kShadowClub, "змеева палица", EFeatType::kNormal, true, feat_app,
			 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[EFeat::kShadowClub].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kShadowClub].uses_weapon_skill = false;
//149
	InitFeat(EFeat::kDoubleThrower, "двойной бросок", EFeatType::kActivated, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kDoubleThrower].GetBaseParameter = &GET_REAL_DEX;
//150
	InitFeat(EFeat::kTripleThrower, "тройной бросок", EFeatType::kActivated, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kTripleThrower].GetBaseParameter = &GET_REAL_DEX;
//1151
	InitFeat(EFeat::kPowerThrow, "размах", EFeatType::kNormal, true, feat_app, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kPowerThrow].GetBaseParameter = &GET_REAL_STR;
//152
	InitFeat(EFeat::kDeadlyThrow, "широкий размах", EFeatType::kNormal, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EFeat::kDeadlyThrow].GetBaseParameter = &GET_REAL_STR;
//153
	InitFeat(EFeat::kUndeadsTurn, "turn undead", EFeatType::kTechnique, true, feat_app,
			 70, ESkill::kTurnUndead, ESaving::kStability);
	feat_info[EFeat::kUndeadsTurn].GetBaseParameter = &GET_REAL_INT;
	feat_info[EFeat::kUndeadsTurn].GetEffectParameter = &GET_REAL_WIS;
	feat_info[EFeat::kUndeadsTurn].uses_weapon_skill = false;
	feat_info[EFeat::kUndeadsTurn].always_available = true;
	feat_info[EFeat::kUndeadsTurn].damage_bonus = -30;
	feat_info[EFeat::kUndeadsTurn].success_degree_damage_bonus = 2;
//154
	InitFeat(EFeat::kMultipleCast, "изощренные чары", EFeatType::kNormal, true, feat_app);
//155	
	InitFeat(EFeat::kMagicalShield, "заговоренный щит", EFeatType::kNormal, true, feat_app);
// 156
	InitFeat(EFeat::kAnimalMaster, "хозяин животных", EFeatType::kNormal, true, feat_app);
}

const char *GetFeatName(EFeat id) {
	if (id >= EFeat::kFirstFeat && id <= EFeat::kLastFeat) {
		return (feat_info[id].name);
	} else {
		if (id == EFeat::kIncorrectFeat) {
			return "UNUSED";
		} else {
			return "UNDEFINED";
		}
	}
}

bool IsAbleToUseFeat(const CharData *ch, EFeat feat) {
	if (feat_info[feat].always_available) {
		return true;
	};
	if ((feat == EFeat::kIncorrectFeat) || !HAVE_FEAT(ch, feat)) {
		return false;
	};
	if (ch->is_npc()) {
		return true;
	};
	if (NUM_LEV_FEAT(ch) < feat_info[feat].slot[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
		return false;
	};
	if (GET_REAL_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
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
		case EFeat::kDoubleshot: return (ch->get_skill(ESkill::kBows) > 39);
			break;
		case EFeat::kJeweller: return (ch->get_skill(ESkill::kJewelry) > 59);
			break;
		case EFeat::kSkilledTrader: return ((GetRealLevel(ch) + GET_REAL_REMORT(ch) / 3) > 19);
			break;
		case EFeat::kMagicUser: return (GetRealLevel(ch) < 25);
			break;
		case EFeat::kLiveShield: return (ch->get_skill(ESkill::kRescue) > 124);
			break;
		case EFeat::kShadowThrower: return (ch->get_skill(ESkill::kDarkMagic) > 120);
			break;
		case EFeat::kAnimalMaster: return (ch->get_skill(ESkill::kMindMagic) > 79);
			break;
			// Костыльный блок работы скирмишера где не нужно
			// Svent TODO Для абилок не забыть реализовать провкрку состояния персонажа
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

bool IsAbleToGetFeat(CharData *ch, EFeat feat) {
	int i, count = 0;
	if (feat < EFeat::kFirstFeat || feat > EFeat::kLastFeat) {
		sprintf(buf, "Неверный номер способности (feat=%d, ch=%s) передан в features::can_get_feat!",
				feat, ch->get_name().c_str());
		mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
		return false;
	}
	// Фит доступен всем и всегда, незачем его куда-то заучиввать.
	if (feat_info[feat].always_available) {
		return false;
	};
	if ((!feat_info[feat].is_known[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
		&& !PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), feat))
		|| (GET_REAL_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])) {
		return false;
	}

	if (!CheckVacantFeatSlot(ch, feat)) {
		return false;
	}

	switch (feat) {
		case EFeat::kParryArrow:
			return (ch->get_skill(ESkill::kMultiparry) || ch->get_skill(ESkill::kParry));
			break;
		case EFeat::kConnoiseur:
			return (ch->get_skill(ESkill::kIdentify));
			break;
		case EFeat::kExorcist:
			return (ch->get_skill(ESkill::kTurnUndead));
			break;
		case EFeat::kHealer:
			return (ch->get_skill(ESkill::kFirstAid));
			break;
		case EFeat::kStealthy:
			return (ch->get_skill(ESkill::kHide) ||
				ch->get_skill(ESkill::kSneak) ||
				ch->get_skill(ESkill::kDisguise));
			break;
		case EFeat::kTracker:
			return (ch->get_skill(ESkill::kTrack) || ch->get_skill(ESkill::kSense));
			break;
		case EFeat::kPunchMaster:
		case EFeat::kClubsMaster:
		case EFeat::kAxesMaster:
		case EFeat::kLongsMaster:
		case EFeat::kShortsMaster:
		case EFeat::kNonstandartsMaster:
		case EFeat::kTwohandsMaster:
		case EFeat::kPicksMaster:
		case EFeat::kSpadesMaster:
		case EFeat::kBowsMaster:
			if (!HAVE_FEAT(ch, (ubyte) feat_info[feat].affected[0].location)) {
				return false;
			}
			for (i = EFeat::kPunchMaster; i <= EFeat::kBowsMaster; i++) {
				if (HAVE_FEAT(ch, i)) {
					count++;
				}
			}
			if (count >= 1 + GET_REAL_REMORT(ch) / 7) {
				return false;
			}
			break;
		case EFeat::kWarriorSpirit: return (HAVE_FEAT(ch, EFeat::kGreatFortitude));
			break;
		case EFeat::kNimbleFingers:
			return (ch->get_skill(ESkill::kSteal) || ch->get_skill(ESkill::kPickLock));
			break;
		case EFeat::kGreatPowerAttack: return (HAVE_FEAT(ch, EFeat::kPowerAttack));
			break;
		case EFeat::kPunchFocus:
		case EFeat::kClubsFocus:
		case EFeat::kAxesFocus:
		case EFeat::kLongsFocus:
		case EFeat::kShortsFocus:
		case EFeat::kNonstandartsFocus:
		case EFeat::kTwohandsFocus:
		case EFeat::kPicksFocus:
		case EFeat::kSpadesFocus:
		case EFeat::kBowsFocus:
			if (!ch->get_skill(feat_info[feat].base_skill)) {
				return false;
			}

			for (i = EFeat::kPunchFocus; i <= EFeat::kBowsFocus; i++) {
				if (HAVE_FEAT(ch, i)) {
					count++;
				}
			}

			if (count >= 2 + GET_REAL_REMORT(ch) / 6) {
				return false;
			}
			break;
		case EFeat::kGreatAimingAttack:
			return (HAVE_FEAT(ch, EFeat::kAimingAttack));
			break;
		case EFeat::kDoubleshot:
			return (HAVE_FEAT(ch, EFeat::kBowsFocus) &&
				ch->get_skill(ESkill::kBows) > 39);
			break;
		case EFeat::kJeweller:
			return (ch->get_skill(ESkill::kJewelry) > 59);
			break;
		case EFeat::kCutting:
			return (HAVE_FEAT(ch, EFeat::kShortsMaster) ||
				HAVE_FEAT(ch, EFeat::kPicksMaster) ||
				HAVE_FEAT(ch, EFeat::kLongsMaster) ||
				HAVE_FEAT(ch, EFeat::kSpadesMaster) ||
				HAVE_FEAT(ch, EFeat::kTwohandsMaster));
			break;
		case EFeat::kScirmisher:
			return (ch->get_skill(ESkill::kRescue));
			break;
		case EFeat::kTactician:
			return (ch->get_skill(ESkill::kLeadership) > 99);
			break;
		case EFeat::kShadowThrower:
			return (HAVE_FEAT(ch, EFeat::kPowerThrow) &&
				(ch->get_skill(ESkill::kDarkMagic) > 120));
			break;
		case EFeat::kShadowDagger:
		case EFeat::kShadowSpear: [[fallthrough]];
		case EFeat::kShadowClub:
			return (HAVE_FEAT(ch, EFeat::kShadowThrower) &&
				(ch->get_skill(ESkill::kDarkMagic) > 130));
			break;
		case EFeat::kDoubleThrower:
			return (HAVE_FEAT(ch, EFeat::kPowerThrow) &&
				(ch->get_skill(ESkill::kThrow) > 100));
			break;
		case EFeat::kTripleThrower:
			return (HAVE_FEAT(ch, EFeat::kDeadlyThrow) &&
				(ch->get_skill(ESkill::kThrow) > 130));
			break;
		case EFeat::kPowerThrow:
			return (ch->get_skill(ESkill::kThrow) > 90);
			break;
		case EFeat::kDeadlyThrow:
			return (HAVE_FEAT(ch, EFeat::kPowerThrow) &&
				(ch->get_skill(ESkill::kThrow) > 110));
			break;
		default: return true;
			break;
	}

	return true;
}

bool CheckVacantFeatSlot(CharData *ch, EFeat feat_id) {
	int i, lowfeat, hifeat;

	if (feat_info[feat_id].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
		|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), feat_id))
		return true;

	//сколько у нас вообще способностей, у которых слот меньше требуемого, и сколько - тех, у которых больше или равно?
	lowfeat = 0;
	hifeat = 0;

	//Мы не можем просто учесть кол-во способностей меньше требуемого и больше требуемого,
	//т.к. возможны свободные слоты меньше требуемого, и при этом верхние заняты все
	auto slot_list = std::vector<int>();
	for (i = EFeat::kFirstFeat; i <= EFeat::kLastFeat; ++i) {
		if (feat_info[i].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] ||
			PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), i)) {
			continue;
		}

		if (HAVE_FEAT(ch, i)) {
			if (FEAT_SLOT(ch, i) >= FEAT_SLOT(ch, feat_id)) {
				++hifeat;
			} else {
				slot_list.push_back(FEAT_SLOT(ch, i));
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
	if (NUM_LEV_FEAT(ch) - FEAT_SLOT(ch, feat_id) - hifeat - MAX(0, lowfeat - FEAT_SLOT(ch, feat_id)) > 0)
		return true;

	//oops.. слотов нет
	return false;
}

int GetModifier(EFeat feat_id, int location) {
	for (int i = 0; i < kMaxFeatAffect; i++) {
		if (feat_info[feat_id].affected[i].location == location) {
			return (int) feat_info[feat_id].affected[i].modifier;
		}
	}
	return 0;
}

void CheckBerserk(CharData *ch) {
	struct TimedFeat timed;
	int prob;

	if (IsAffectedBySpell(ch, kSpellBerserk) &&
		(GET_HIT(ch) > GET_REAL_MAX_HIT(ch) / 2)) {
		affect_from_char(ch, kSpellBerserk);
		SendMsgToChar("Предсмертное исступление оставило вас.\r\n", ch);
	}

	if (IsAbleToUseFeat(ch, EFeat::kBerserker) && ch->get_fighting() &&
		!IsTimed(ch, EFeat::kBerserker) && !AFF_FLAGGED(ch, EAffect::kBerserk)
		&& (GET_HIT(ch) < GET_REAL_MAX_HIT(ch) / 4)) {
		CharData *vict = ch->get_fighting();
		timed.feat = EFeat::kBerserker;
		timed.time = 4;
		ImposeTimedFeat(ch, &timed);

		Affect<EApply> af;
		af.type = kSpellBerserk;
		af.duration = CalcDuration(ch, 1, 60, 30, 0, 0);
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = 0;

		prob = ch->is_npc() ? 601 : (751 - GetRealLevel(ch) * 5);
		if (number(1, 1000) < prob) {
			af.bitvector = to_underlying(EAffect::kBerserk);
			act("Вас обуяла предсмертная ярость!", false, ch, nullptr, nullptr, kToChar);
			act("$n0 исступленно взвыл$g и бросил$u на противника!", false, ch, nullptr, vict, kToNotVict);
			act("$n0 исступленно взвыл$g и бросил$u на вас!", false, ch, nullptr, vict, kToVict);
		} else {
			af.bitvector = 0;
			act("Вы истошно завопили, пытаясь напугать противника. Без толку.", false, ch, nullptr, nullptr, kToChar);
			act("$n0 истошно завопил$g, пытаясь напугать противника. Забавно...", false, ch, nullptr, vict, kToNotVict);
			act("$n0 истошно завопил$g, пытаясь напугать вас. Забавно...", false, ch, nullptr, vict, kToVict);
		}
		affect_join(ch, af, true, false, true, false);
	}
}

void do_lightwalk(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedFeat timed;

	if (ch->is_npc() || !IsAbleToUseFeat(ch, EFeat::kLightWalk)) {
		SendMsgToChar("Вы не можете этого.\r\n", ch);
		return;
	}

	if (ch->ahorse()) {
		act("Позаботьтесь сперва о мягких тапочках для $N3...", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}

	if (IsAffectedBySpell(ch, kSpellLightWalk)) {
		SendMsgToChar("Вы уже двигаетесь легким шагом.\r\n", ch);
		return;
	}
	if (IsTimed(ch, EFeat::kLightWalk)) {
		SendMsgToChar("Вы слишком утомлены для этого.\r\n", ch);
		return;
	}

	affect_from_char(ch, kSpellLightWalk);

	timed.feat = EFeat::kLightWalk;
	timed.time = 24;
	ImposeTimedFeat(ch, &timed);

	SendMsgToChar("Хорошо, вы попытаетесь идти, не оставляя лишних следов.\r\n", ch);
	Affect<EApply> af;
	af.type = kSpellLightWalk;
	af.duration = CalcDuration(ch, 2, GetRealLevel(ch), 5, 2, 8);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;
	if (number(1, 1000) > number(1, GET_REAL_DEX(ch) * 50)) {
		af.bitvector = 0;
		SendMsgToChar("Вам не хватает ловкости...\r\n", ch);
	} else {
		af.bitvector = to_underlying(EAffect::kLightWalk);
		SendMsgToChar("Ваши шаги стали легче перышка.\r\n", ch);
	}
	affect_to_char(ch, af);
}

void do_fit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *obj;
	CharData *vict;
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	//отключено пока для не-иммов
	if (GetRealLevel(ch) < kLvlImmortal) {
		SendMsgToChar("Вы не можете этого.", ch);
		return;
	};

	//Может ли игрок использовать эту способность?
	if ((subcmd == SCMD_DO_ADAPT) && !IsAbleToUseFeat(ch, EFeat::kToFitItem)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	};
	if ((subcmd == SCMD_MAKE_OVER) && !IsAbleToUseFeat(ch, EFeat::kToFitClotches)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	};

	//Есть у нас предмет, который хотят переделать?
	argument = one_argument(argument, arg1);

	if (!*arg1) {
		SendMsgToChar("Что вы хотите переделать?\r\n", ch);
		return;
	};

	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		sprintf(buf, "У вас нет \'%s\'.\r\n", arg1);
		SendMsgToChar(buf, ch);
		return;
	};

	//На кого переделываем?
	argument = one_argument(argument, arg2);
	if (!(vict = get_char_vis(ch, arg2, EFind::kCharInRoom))) {
		SendMsgToChar("Под кого вы хотите переделать эту вещь?\r\n Нет такого создания в округе!\r\n", ch);
		return;
	};

	//Предмет уже имеет владельца
	if (GET_OBJ_OWNER(obj) != 0) {
		SendMsgToChar("У этой вещи уже есть владелец.\r\n", ch);
		return;

	};

	if ((GET_OBJ_WEAR(obj) <= 1) || obj->has_flag(EObjFlag::KSetItem)) {
		SendMsgToChar("Этот предмет невозможно переделать.\r\n", ch);
		return;
	}

	switch (subcmd) {
		case SCMD_DO_ADAPT:
			if (GET_OBJ_MATER(obj) != EObjMaterial::kMaterialUndefined
				&& GET_OBJ_MATER(obj) != EObjMaterial::kBulat
				&& GET_OBJ_MATER(obj) != EObjMaterial::kBronze
				&& GET_OBJ_MATER(obj) != EObjMaterial::kIron
				&& GET_OBJ_MATER(obj) != EObjMaterial::kSteel
				&& GET_OBJ_MATER(obj) != EObjMaterial::kForgedSteel
				&& GET_OBJ_MATER(obj) != EObjMaterial::kPreciousMetel
				&& GET_OBJ_MATER(obj) != EObjMaterial::kWood
				&& GET_OBJ_MATER(obj) != EObjMaterial::kHardWood
				&& GET_OBJ_MATER(obj) != EObjMaterial::kGlass) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				SendMsgToChar(buf, ch);
				return;
			}
			break;
		case SCMD_MAKE_OVER:
			if (GET_OBJ_MATER(obj) != EObjMaterial::kBone
				&& GET_OBJ_MATER(obj) != EObjMaterial::kCloth
				&& GET_OBJ_MATER(obj) != EObjMaterial::kSkin
				&& GET_OBJ_MATER(obj) != EObjMaterial::kOrganic) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				SendMsgToChar(buf, ch);
				return;
			}
			break;
		default:
			SendMsgToChar("Это какая-то ошибка...\r\n", ch);
			return;
			break;
	};
	obj->set_owner(GET_UNIQUE(vict));
	sprintf(buf, "Вы долго пыхтели и сопели, переделывая работу по десять раз.\r\n");
	sprintf(buf + strlen(buf), "Вы извели кучу времени и 10000 кун золотом.\r\n");
	sprintf(buf + strlen(buf), "В конце-концов подогнали %s точно по мерке %s.\r\n",
			GET_OBJ_PNAME(obj, 3).c_str(), GET_PAD(vict, 1));

	SendMsgToChar(buf, ch);

}

#include "game_classes/classes_spell_slots.h" // удалить после вырезания do_spell_capable
#include "game_magic/spells_info.h"
#define SpINFO spell_info[spellnum]
// Вложить закл в клона
void do_spell_capable(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	using PlayerClass::CalcCircleSlotsAmount;

	struct TimedFeat timed;

	if (!IS_IMPL(ch) && (ch->is_npc() || !IsAbleToUseFeat(ch, EFeat::kSpellCapabler))) {
		SendMsgToChar("Вы не столь могущественны.\r\n", ch);
		return;
	}

	if (IsTimed(ch, EFeat::kSpellCapabler) && !IS_IMPL(ch)) {
		SendMsgToChar("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	char *s;
	int spellnum;

	if (ch->is_npc() && AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
		SendMsgToChar("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}

	s = strtok(argument, "'*!");
	if (s == nullptr) {
		SendMsgToChar("ЧТО вы хотите колдовать?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (s == nullptr) {
		SendMsgToChar("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	spellnum = FixNameAndFindSpellNum(s);
	if (spellnum < 1 || spellnum > kSpellCount) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	if ((!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellTemp | kSpellKnow) ||
		GET_REAL_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)) &&
		(GetRealLevel(ch) < kLvlGreatGod) && (!ch->is_npc())) {
		if (GetRealLevel(ch) < MIN_CAST_LEV(SpINFO, ch)
			|| GET_REAL_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)
			|| CalcCircleSlotsAmount(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0) {
			SendMsgToChar("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		} else {
			SendMsgToChar("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	Follower *k;
	CharData *follower = nullptr;
	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->ch, EAffect::kCharmed)
			&& k->ch->get_master() == ch
			&& MOB_FLAGGED(k->ch, EMobFlag::kClone)
			&& !IsAffectedBySpell(k->ch, kSpellCapable)
			&& ch->in_room == IN_ROOM(k->ch)) {
			follower = k->ch;
			break;
		}
	}
	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	if (!follower) {
		SendMsgToChar("Хорошо бы найти подходящую цель для этого.\r\n", ch);
		return;
	}

	act("Вы принялись зачаровывать $N3.", false, ch, nullptr, follower, kToChar);
	act("$n принял$u делать какие-то пассы и что-то бормотать в сторону $N3.", false, ch, nullptr, follower, kToRoom);

	GET_SPELL_MEM(ch, spellnum)--;
	if (!ch->is_npc() && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, EPrf::kAutomem))
		MemQ_remember(ch, spellnum);

	if (!IS_SET(SpINFO.routines, kMagDamage) || !SpINFO.violent ||
		IS_SET(SpINFO.routines, kMagMasses) || IS_SET(SpINFO.routines, kMagGroups) ||
		IS_SET(SpINFO.routines, kMagAreas)) {
		SendMsgToChar("Вы конечно мастер, но не такой магии.\r\n", ch);
		return;
	}
	affect_from_char(ch, EFeat::kSpellCapabler);

	timed.feat = EFeat::kSpellCapabler;

	switch (SpINFO.slot_forc[GET_CLASS(ch)][GET_KIN(ch)]) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5://1-5 слоты кд 4 тика
			timed.time = 4;
			break;
		case 6:
		case 7://6-7 слоты кд 6 тиков
			timed.time = 6;
			break;
		case 8://8 слот кд 10 тиков
			timed.time = 10;
			break;
		case 9://9 слот кд 12 тиков
			timed.time = 12;
			break;
		default://10 слот или тп
			timed.time = 24;
	}
	ImposeTimedFeat(ch, &timed);

	GET_CAST_SUCCESS(follower) = GET_REAL_REMORT(ch) * 4;
	Affect<EApply> af;
	af.type = kSpellCapable;
	af.duration = 48;
	if (GET_REAL_REMORT(ch) > 0) {
		af.modifier = GET_REAL_REMORT(ch) * 4;//вешаецо аффект который дает +морт*4 касту
		af.location = EApply::kCastSuccess;
	} else {
		af.modifier = 0;
		af.location = EApply::kNone;
	}
	af.battleflag = 0;
	af.bitvector = 0;
	affect_to_char(follower, af);
	follower->mob_specials.capable_spell = spellnum;
}

// \todo Надо как-то переделать загрузку родовых способностей, чтобы там было не int, а сразу EFeat
void SetRaceFeats(CharData *ch) {
	auto race_features = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int &i: race_features) {
		if (IsAbleToGetFeat(ch, static_cast<EFeat>(i))) {
			SET_FEAT(ch, i);
		}
	}
}

void UnsetRaceFeats(CharData *ch) {
	auto race_features = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int &i: race_features) {
		UNSET_FEAT(ch, i);
	}
}

void SetInbornClassFeats(CharData *ch) {
	for (auto i = EFeat::kBerserker; i <= EFeat::kLastFeat; ++i) {
		if (IsAbleToGetFeat(ch, i) && feat_info[i].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
			SET_FEAT(ch, i);
		}
	}
}

void SetInbornFeats(CharData *ch) {
	SetInbornClassFeats(ch);
	SetRaceFeats(ch);
}

int CFeatArray::pos(int pos /*= -1*/) {
	if (pos == -1) {
		return pos_;
	} else if (pos >= 0 && pos < kMaxFeatAffect) {
		pos_ = pos;
		return pos_;
	}
	sprintf(buf, "SYSERR: invalid argument (%d) was sended to features::aff_aray.pos!", pos);
	mudlog(buf, BRF, kLvlGod, SYSLOG, true);
	return pos_;
}

void CFeatArray::insert(const int location, int modifier) {
	affected[pos_].location = location;
	affected[pos_].modifier = modifier;
	pos_++;
	if (pos_ >= kMaxFeatAffect) {
		pos_ = 0;
	}
}

void CFeatArray::clear() {
	pos_ = 0;
	for (auto & i : affected) {
		i.location = EApply::kNone;
		i.modifier = 0;
	}
}

bool TryFlipActivatedFeature(CharData *ch, char *argument) {
	auto feat_id = GetFeatureNum(argument);
	if (feat_id <= EFeat::kIncorrectFeat) {
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
				 feat_info[feat_id].name,
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
				 CCIGRN(ch, C_SPR), feat_info[feat_id].name, CCNRM(ch, C_OFF));
}

bool CheckAccessActivatedFeat(CharData *ch, EFeat feat_id) {
	if (!IsAbleToUseFeat(ch, feat_id)) {
		SendMsgToChar("Вы не в состоянии использовать эту способность.\r\n", ch);
		return false;
	}
	if (feat_info[feat_id].type != EFeatType::kActivated) {
		SendMsgToChar("Эту способность невозможно применить таким образом.\r\n", ch);
		return false;
	}

	return true;
}

EFeat GetFeatureNum(char *feat_name) {
	skip_spaces(&feat_name);
	return FindFeatNum(feat_name);
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
		default: return EFeat::kIncorrectFeat;
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
		return (skirmishers * 2 - uncoveredSquadMembers) * abilities::kCircumstanceFactor - 40;
	};
	return (skirmishers * 2 - uncoveredSquadMembers) * abilities::kCircumstanceFactor;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
