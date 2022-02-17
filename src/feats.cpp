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

struct FeatureInfo feat_info[kMaxFeats];

/* Служебные функции */
void InitFeatByDefault(int feat_num);
bool CheckVacantFeatSlot(CharData *ch, int feat);

/* Функции для работы с переключаемыми способностями */
bool CheckAccessActivatedFeat(CharData *ch, int feat_num);
void ActivateFeat(CharData *ch, int feat_num);
void DeactivateFeature(CharData *ch, int feat_num);
int GetFeatureNum(char *feat_name);

/* Ситуативные бонусы, пишутся для специфических способностей по потребности */
int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */);

/* Активные способности */
void do_lightwalk(CharData *ch, char *argument, int cmd, int subcmd);

/* Extern */
extern void SetSkillCooldown(CharData *ch, ESkill skill, int cooldownInPulses);

///
/// Поиск номера способности по имени
/// \param alias = false
/// true для поиска при вводе имени способности игроком у учителей
///
int FindFeatNum(const char *name, bool alias) {
	for (int index = 1; index < kMaxFeats; index++) {
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
		if (flag)
			return index;
	}
	return (-1);
}

void InitFeat(int feat_num, const char *name, int type, bool can_up_slot, CFeatArray app,
			  int roll_bonus = abilities::MAX_ABILITY_DICEROLL_BONUS, ESkill base_skill = ESkill::kIncorrect,
			  ESaving saving = ESaving::kStability) {
	int i, j;
	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			feat_info[feat_num].min_remort[i][j] = 0;
			feat_info[feat_num].slot[i][j] = 0;
		}
	}
	if (name) {
		feat_info[feat_num].name = name;
		std::string alias(name);
		std::replace_if(alias.begin(), alias.end(), boost::is_any_of("_:"), ' ');
		boost::trim_all(alias);
		feat_info[feat_num].alias = alias;
	}
	feat_info[feat_num].id = feat_num;
	feat_info[feat_num].diceroll_bonus = roll_bonus;
	feat_info[feat_num].base_skill = base_skill;
	feat_info[feat_num].saving = saving;
	feat_info[feat_num].type = type;
	feat_info[feat_num].up_slot = can_up_slot;
	for (i = 0; i < kMaxFeatAffect; i++) {
		feat_info[feat_num].affected[i].location = app.affected[i].location;
		feat_info[feat_num].affected[i].modifier = app.affected[i].modifier;
	}
}

void InitFeatByDefault(int feat_num) {
	int i, j;

	for (i = 0; i < kNumPlayerClasses; i++) {
		for (j = 0; j < kNumKins; j++) {
			feat_info[feat_num].min_remort[i][j] = 0;
			feat_info[feat_num].slot[i][j] = 0;
			feat_info[feat_num].is_inborn[i][j] = false;
			feat_info[feat_num].is_known[i][j] = false;
		}
	}

	feat_info[feat_num].id = feat_num;
	feat_info[feat_num].name = unused_spellname;
	feat_info[feat_num].type = UNUSED_FTYPE;
	feat_info[feat_num].up_slot = false;
	feat_info[feat_num].always_available = false;
	feat_info[feat_num].uses_weapon_skill = false;
	feat_info[feat_num].damage_bonus = 0;
	feat_info[feat_num].success_degree_damage_bonus = 5;
	feat_info[feat_num].saving = ESaving::kStability;
	feat_info[feat_num].diceroll_bonus = abilities::MAX_ABILITY_DICEROLL_BONUS;
	feat_info[feat_num].base_skill = ESkill::kIncorrect;
	feat_info[feat_num].critfail_threshold = abilities::kDefaultCritfailThreshold;
	feat_info[feat_num].critsuccess_threshold = abilities::kDefaultCritsuccessThreshold;

	for (i = 0; i < kMaxFeatAffect; i++) {
		feat_info[feat_num].affected[i].location = APPLY_NONE;
		feat_info[feat_num].affected[i].modifier = 0;
	}

	feat_info[feat_num].GetBaseParameter = &GET_REAL_INT;
	feat_info[feat_num].GetEffectParameter = &GET_REAL_STR;
	feat_info[feat_num].CalcSituationalDamageFactor =
		([](CharData *) -> float {
			return 1.00;
		});
	feat_info[feat_num].CalcSituationalRollBonus =
		([](CharData *, CharData *) -> int {
			return 0;
		});
}

void InitFeatures() {
	CFeatArray feat_app;
	for (int i = 1; i < kMaxFeats; i++) {
		InitFeatByDefault(i);
	}
//1
	InitFeat(BERSERK_FEAT, "предсмертная ярость", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//2
	InitFeat(PARRY_ARROW_FEAT, "отбить стрелу", NORMAL_FTYPE, true, feat_app);
//3
	InitFeat(BLIND_FIGHT_FEAT, "слепой бой", NORMAL_FTYPE, true, feat_app);
//4
	feat_app.insert(APPLY_MR, 1);
	feat_app.insert(APPLY_AR, 1);
	InitFeat(IMPREGNABLE_FEAT, "непробиваемый", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//5-*
	InitFeat(APPROACHING_ATTACK_FEAT, "встречная атака", NORMAL_FTYPE, true, feat_app);
//6
	InitFeat(DEFENDER_FEAT, "щитоносец", NORMAL_FTYPE, true, feat_app);
//7
	InitFeat(DODGER_FEAT, "изворотливость", AFFECT_FTYPE, true, feat_app);
//8
	InitFeat(LIGHT_WALK_FEAT, "легкая поступь", NORMAL_FTYPE, true, feat_app);
//9
	InitFeat(WRIGGLER_FEAT, "проныра", NORMAL_FTYPE, true, feat_app);
//10
	InitFeat(SPELL_SUBSTITUTE_FEAT, "подмена заклинания", NORMAL_FTYPE, true, feat_app);
//11
	InitFeat(POWER_ATTACK_FEAT, "мощная атака", ACTIVATED_FTYPE, true, feat_app);
//12
	feat_app.insert(APPLY_RESIST_FIRE, 5);
	feat_app.insert(APPLY_RESIST_AIR, 5);
	feat_app.insert(APPLY_RESIST_WATER, 5);
	feat_app.insert(APPLY_RESIST_EARTH, 5);
	feat_app.insert(APPLY_RESIST_DARK, 5);
	InitFeat(WOODEN_SKIN_FEAT, "деревянная кожа", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//13
	feat_app.insert(APPLY_RESIST_FIRE, 10);
	feat_app.insert(APPLY_RESIST_AIR, 10);
	feat_app.insert(APPLY_RESIST_WATER, 10);
	feat_app.insert(APPLY_RESIST_EARTH, 10);
	feat_app.insert(APPLY_RESIST_DARK, 10);
	feat_app.insert(APPLY_ABSORBE, 5);
	InitFeat(IRON_SKIN_FEAT, "железная кожа", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//14
	feat_app.insert(FEAT_TIMER, 8);
	InitFeat(CONNOISEUR_FEAT, "знаток", SKILL_MOD_FTYPE, true, feat_app);
	feat_app.clear();
//15
	InitFeat(EXORCIST_FEAT, "изгоняющий нежить", SKILL_MOD_FTYPE, true, feat_app);
//16
	InitFeat(HEALER_FEAT, "целитель", NORMAL_FTYPE, true, feat_app);
//17
	feat_app.insert(APPLY_SAVING_REFLEX, -10);
	InitFeat(LIGHTING_REFLEX_FEAT, "мгновенная реакция", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//18
	feat_app.insert(FEAT_TIMER, 8);
	InitFeat(DRUNKARD_FEAT, "пьяница", SKILL_MOD_FTYPE, true, feat_app);
	feat_app.clear();
//19
	InitFeat(POWER_MAGIC_FEAT, "мощь колдовства", NORMAL_FTYPE, true, feat_app);
//20
	feat_app.insert(APPLY_MOVEREG, 40);
	InitFeat(ENDURANCE_FEAT, "выносливость", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//21
	feat_app.insert(APPLY_SAVING_WILL, -10);
	feat_app.insert(APPLY_SAVING_STABILITY, -10);
	InitFeat(GREAT_FORTITUDE_FEAT, "сила духа", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//22
	feat_app.insert(APPLY_HITREG, 35);
	InitFeat(FAST_REGENERATION_FEAT, "быстрое заживление", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//23
	InitFeat(STEALTHY_FEAT, "незаметность", SKILL_MOD_FTYPE, true, feat_app);
//24
	feat_app.insert(APPLY_CAST_SUCCESS, 80);
	InitFeat(RELATED_TO_MAGIC_FEAT, "магическое родство", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//25
	feat_app.insert(APPLY_HITREG, 10);
	feat_app.insert(APPLY_SAVING_CRITICAL, -4);
	InitFeat(SPLENDID_HEALTH_FEAT, "богатырское здоровье", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//26
	InitFeat(TRACKER_FEAT, "следопыт", SKILL_MOD_FTYPE, true, feat_app);
	feat_app.clear();
//27
	InitFeat(WEAPON_FINESSE_FEAT, "ловкий удар", NORMAL_FTYPE, true, feat_app);
//28
	InitFeat(COMBAT_CASTING_FEAT, "боевое колдовство", NORMAL_FTYPE, true, feat_app);
//29
	feat_app.insert(PUNCH_FOCUS_FEAT, 1);
	InitFeat(PUNCH_MASTER_FEAT, "мастер кулачного боя", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//30
	feat_app.insert(CLUB_FOCUS_FEAT, 1);
	InitFeat(CLUBS_MASTER_FEAT, "мастер палицы", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//31
	feat_app.insert(AXES_FOCUS_FEAT, 1);
	InitFeat(AXES_MASTER_FEAT, "мастер секиры", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//32
	feat_app.insert(LONGS_FOCUS_FEAT, 1);
	InitFeat(LONGS_MASTER_FEAT, "мастер меча", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//33
	feat_app.insert(SHORTS_FOCUS_FEAT, 1);
	InitFeat(SHORTS_MASTER_FEAT, "мастер ножа", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//34
	feat_app.insert(NONSTANDART_FOCUS_FEAT, 1);
	InitFeat(NONSTANDART_MASTER_FEAT, "мастер необычного оружия", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//35
	feat_app.insert(BOTHHANDS_FOCUS_FEAT, 1);
	InitFeat(BOTHHANDS_MASTER_FEAT, "мастер двуручника", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//36
	feat_app.insert(PICK_FOCUS_FEAT, 1);
	InitFeat(PICK_MASTER_FEAT, "мастер кинжала", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//37
	feat_app.insert(SPADES_FOCUS_FEAT, 1);
	InitFeat(SPADES_MASTER_FEAT, "мастер копья", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//38
	feat_app.insert(BOWS_FOCUS_FEAT, 1);
	InitFeat(BOWS_MASTER_FEAT, "мастер лучник", NORMAL_FTYPE, true, feat_app);
	feat_app.clear();
//39
	InitFeat(FOREST_PATHS_FEAT, "лесные тропы", NORMAL_FTYPE, true, feat_app);
//40
	InitFeat(MOUNTAIN_PATHS_FEAT, "горные тропы", NORMAL_FTYPE, true, feat_app);
//41
	feat_app.insert(APPLY_MORALE, 5);
	InitFeat(LUCKY_FEAT, "счастливчик", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//42
	InitFeat(SPIRIT_WARRIOR_FEAT, "боевой дух", NORMAL_FTYPE, true, feat_app);
//43
	feat_app.insert(APPLY_HITREG, 50);
	InitFeat(RELIABLE_HEALTH_FEAT, "крепкое здоровье", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//44
	feat_app.insert(APPLY_MANAREG, 100);
	InitFeat(EXCELLENT_MEMORY_FEAT, "превосходная память", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//45
	feat_app.insert(APPLY_DEX, 1);
	InitFeat(ANIMAL_DEXTERY_FEAT, "звериная прыть", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//46
	feat_app.insert(APPLY_MANAREG, 25);
	InitFeat(LEGIBLE_WRITTING_FEAT, "чёткий почерк", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//47
	feat_app.insert(APPLY_DAMROLL, 2);
	InitFeat(IRON_MUSCLES_FEAT, "стальные мышцы", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//48
	feat_app.insert(APPLY_CAST_SUCCESS, 5);
	InitFeat(MAGIC_SIGN_FEAT, "знак чародея", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//49
	feat_app.insert(APPLY_MOVEREG, 75);
	InitFeat(GREAT_ENDURANCE_FEAT, "двужильность", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//50
	feat_app.insert(APPLY_MORALE, 5);
	InitFeat(BEST_DESTINY_FEAT, "лучшая доля", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//51
	InitFeat(BREW_POTION_FEAT, "травник", NORMAL_FTYPE, true, feat_app);
//52
	InitFeat(JUGGLER_FEAT, "жонглер", NORMAL_FTYPE, true, feat_app);
//53
	InitFeat(NIMBLE_FINGERS_FEAT, "ловкач", SKILL_MOD_FTYPE, true, feat_app);
//54
	InitFeat(GREAT_POWER_ATTACK_FEAT, "улучшенная мощная атака", ACTIVATED_FTYPE, true, feat_app);
//55
	feat_app.insert(APPLY_RESIST_IMMUNITY, 15);
	InitFeat(IMMUNITY_FEAT, "привычка к яду", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//56
	feat_app.insert(APPLY_AC, -40);
	InitFeat(MOBILITY_FEAT, "подвижность", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//57
	feat_app.insert(APPLY_STR, 1);
	InitFeat(NATURAL_STRENGTH_FEAT, "силач", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//58
	feat_app.insert(APPLY_DEX, 1);
	InitFeat(NATURAL_DEXTERY_FEAT, "проворство", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//59
	feat_app.insert(APPLY_INT, 1);
	InitFeat(NATURAL_INTELLECT_FEAT, "природный ум", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//60
	feat_app.insert(APPLY_WIS, 1);
	InitFeat(NATURAL_WISDOM_FEAT, "мудрец", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//61
	feat_app.insert(APPLY_CON, 1);
	InitFeat(NATURAL_CONSTITUTION_FEAT, "здоровяк", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//62
	feat_app.insert(APPLY_CHA, 1);
	InitFeat(NATURAL_CHARISMA_FEAT, "природное обаяние", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//63
	feat_app.insert(APPLY_MANAREG, 25);
	InitFeat(MNEMONIC_ENHANCER_FEAT, "отличная память", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//64 -*
	InitFeat(MAGNETIC_PERSONALITY_FEAT, "предводитель", SKILL_MOD_FTYPE, true, feat_app);
//65
	feat_app.insert(APPLY_DAMROLL, 2);
	InitFeat(DAMROLL_BONUS_FEAT, "тяжел на руку", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//66
	feat_app.insert(APPLY_HITROLL, 1);
	InitFeat(HITROLL_BONUS_FEAT, "твердая рука", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//67
	feat_app.insert(APPLY_CAST_SUCCESS, 30);
	InitFeat(MAGICAL_INSTINCT_FEAT, "магическое чутье", AFFECT_FTYPE, true, feat_app);
	feat_app.clear();
//68
	InitFeat(PUNCH_FOCUS_FEAT, "любимое_оружие: голые руки", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kPunch);
//69
	InitFeat(CLUB_FOCUS_FEAT, "любимое_оружие: палица", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kClubs);
//70
	InitFeat(AXES_FOCUS_FEAT, "любимое_оружие: секира", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kAxes);
//71
	InitFeat(LONGS_FOCUS_FEAT, "любимое_оружие: меч", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kLongBlades);
//72
	InitFeat(SHORTS_FOCUS_FEAT, "любимое_оружие: нож", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kShortBlades);
//73
	InitFeat(NONSTANDART_FOCUS_FEAT, "любимое_оружие: необычное", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kNonstandart);
//74
	InitFeat(BOTHHANDS_FOCUS_FEAT, "любимое_оружие: двуручник", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kTwohands);
//75
	InitFeat(PICK_FOCUS_FEAT, "любимое_оружие: кинжал", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kPicks);
//76
	InitFeat(SPADES_FOCUS_FEAT, "любимое_оружие: копье", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kSpades);
//77
	InitFeat(BOWS_FOCUS_FEAT, "любимое_оружие: лук", SKILL_MOD_FTYPE, true,
			 feat_app, 0, ESkill::kBows);
//78
	InitFeat(AIMING_ATTACK_FEAT, "прицельная атака", ACTIVATED_FTYPE, true, feat_app);
//79
	InitFeat(GREAT_AIMING_ATTACK_FEAT, "улучшенная прицельная атака", ACTIVATED_FTYPE, true, feat_app);
//80
	InitFeat(DOUBLESHOT_FEAT, "двойной выстрел", NORMAL_FTYPE, true, feat_app);
//81
	InitFeat(PORTER_FEAT, "тяжеловоз", NORMAL_FTYPE, true, feat_app);
//82
	InitFeat(SECRET_RUNES_FEAT, "тайные руны", NORMAL_FTYPE, true, feat_app);
/*
//83
	initializeFeature(RUNE_USER_FEAT, "тайные руны", NORMAL_FTYPE, true, feat_app);
//84
	initializeFeature(RUNE_MASTER_FEAT, "заветные руны", NORMAL_FTYPE, true, feat_app);
//85
	initializeFeature(RUNE_ULTIMATE_FEAT, "руны богов", NORMAL_FTYPE, true, feat_app);
	*/
//86
	InitFeat(TO_FIT_ITEM_FEAT, "переделать", NORMAL_FTYPE, true, feat_app);
//87
	InitFeat(TO_FIT_CLOTHCES_FEAT, "перешить", NORMAL_FTYPE, true, feat_app);
//88
	InitFeat(STRENGTH_CONCETRATION_FEAT, "концентрация силы", NORMAL_FTYPE, true, feat_app);
//89
	InitFeat(DARK_READING_FEAT, "кошачий глаз", NORMAL_FTYPE, true, feat_app);
//90
	InitFeat(SPELL_CAPABLE_FEAT, "зачаровать", NORMAL_FTYPE, true, feat_app);
//91
	InitFeat(ARMOR_LIGHT_FEAT, "легкие доспехи", NORMAL_FTYPE, true, feat_app);
//92
	InitFeat(ARMOR_MEDIAN_FEAT, "средние доспехи", NORMAL_FTYPE, true, feat_app);
//93
	InitFeat(ARMOR_HEAVY_FEAT, "тяжелые доспехи", NORMAL_FTYPE, true, feat_app);
//94
	InitFeat(GEMS_INLAY_FEAT, "инкрустация", NORMAL_FTYPE, true, feat_app);
//95
	InitFeat(WARRIOR_STR_FEAT, "богатырская сила", NORMAL_FTYPE, true, feat_app);
//96
	InitFeat(RELOCATE_FEAT, "переместиться", NORMAL_FTYPE, true, feat_app);
//97
	InitFeat(SILVER_TONGUED_FEAT, "сладкоречие", NORMAL_FTYPE, true, feat_app);
//98
	InitFeat(BULLY_FEAT, "забияка", NORMAL_FTYPE, true, feat_app);
//99
	InitFeat(THIEVES_STRIKE_FEAT, "воровской удар", NORMAL_FTYPE, true, feat_app);
//100
	InitFeat(MASTER_JEWELER_FEAT, "искусный ювелир", NORMAL_FTYPE, true, feat_app);
//101
	InitFeat(SKILLED_TRADER_FEAT, "торговая сметка", NORMAL_FTYPE, true, feat_app);
//102
	InitFeat(ZOMBIE_DROVER_FEAT, "погонщик умертвий", NORMAL_FTYPE, true, feat_app);
//103
	InitFeat(EMPLOYER_FEAT, "навык найма", NORMAL_FTYPE, true, feat_app);
//104
	InitFeat(MAGIC_USER_FEAT, "использование амулетов", NORMAL_FTYPE, true, feat_app);
//105
	InitFeat(GOLD_TONGUE_FEAT, "златоуст", NORMAL_FTYPE, true, feat_app);
//106
	InitFeat(CALMNESS_FEAT, "хладнокровие", NORMAL_FTYPE, true, feat_app);
//107
	InitFeat(RETREAT_FEAT, "отступление", NORMAL_FTYPE, true, feat_app);
//108
	InitFeat(SHADOW_STRIKE_FEAT, "танцующая тень", NORMAL_FTYPE, true, feat_app);
//109
	InitFeat(THRIFTY_FEAT, "запасливость", NORMAL_FTYPE, true, feat_app);
//110
	InitFeat(CYNIC_FEAT, "циничность", NORMAL_FTYPE, true, feat_app);
//111
	InitFeat(PARTNER_FEAT, "напарник", NORMAL_FTYPE, true, feat_app);
//112
	InitFeat(HELPDARK_FEAT, "помощь тьмы", NORMAL_FTYPE, true, feat_app);
//113
	InitFeat(FURYDARK_FEAT, "ярость тьмы", NORMAL_FTYPE, true, feat_app);
//114
	InitFeat(DARKREGEN_FEAT, "темное восстановление", NORMAL_FTYPE, true, feat_app);
//115
	InitFeat(SOULLINK_FEAT, "родство душ", NORMAL_FTYPE, true, feat_app);
//116
	InitFeat(STRONGCLUTCH_FEAT, "сильная хватка", NORMAL_FTYPE, true, feat_app);
//117
	InitFeat(MAGICARROWS_FEAT, "магические стрелы", NORMAL_FTYPE, true, feat_app);
//118
	InitFeat(COLLECTORSOULS_FEAT, "колекционер душ", NORMAL_FTYPE, true, feat_app);
//119
	InitFeat(DARKDEAL_FEAT, "темная сделка", NORMAL_FTYPE, true, feat_app);
//120
	InitFeat(DECLINE_FEAT, "порча", NORMAL_FTYPE, true, feat_app);
//121
	InitFeat(HARVESTLIFE_FEAT, "жатва жизни", NORMAL_FTYPE, true, feat_app);
//122
	InitFeat(LOYALASSIST_FEAT, "верный помощник", NORMAL_FTYPE, true, feat_app);
//123
	InitFeat(HAUNTINGSPIRIT_FEAT, "блуждающий дух", NORMAL_FTYPE, true, feat_app);
//124
	InitFeat(SNEAKRAGE_FEAT, "ярость змеи", NORMAL_FTYPE, true, feat_app);
//126
	InitFeat(ELDER_TASKMASTER_FEAT, "старший надсмотрщик", NORMAL_FTYPE, true, feat_app);
//127
	InitFeat(LORD_UNDEAD_FEAT, "повелитель нежити", NORMAL_FTYPE, true, feat_app);
//128
	InitFeat(DARK_WIZARD_FEAT, "темный маг", NORMAL_FTYPE, true, feat_app);
//129
	InitFeat(ELDER_PRIEST_FEAT, "старший жрец", NORMAL_FTYPE, true, feat_app);
//130
	InitFeat(HIGH_LICH_FEAT, "верховный лич", NORMAL_FTYPE, true, feat_app);
//131
	InitFeat(BLACK_RITUAL_FEAT, "темный ритуал", NORMAL_FTYPE, true, feat_app);
//132
	InitFeat(TEAMSTER_UNDEAD_FEAT, "погонщик нежити", NORMAL_FTYPE, true, feat_app);
//133
	InitFeat(SKIRMISHER_FEAT, "держать строй", ACTIVATED_FTYPE, true, feat_app,
			 90, ESkill::kRescue, ESaving::kReflex);
	feat_info[SKIRMISHER_FEAT].GetBaseParameter = &GET_REAL_DEX;
	feat_info[SKIRMISHER_FEAT].CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
//134
	InitFeat(TACTICIAN_FEAT, "десяцкий", ACTIVATED_FTYPE, true, feat_app, 90, ESkill::kLeadership, ESaving::kReflex);
	feat_info[TACTICIAN_FEAT].GetBaseParameter = &GET_REAL_CHA;
	feat_info[TACTICIAN_FEAT].CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
//135
	InitFeat(LIVE_SHIELD_FEAT, "живой щит", NORMAL_FTYPE, true, feat_app);
// === Проскок номеров (типа резерв под татей) ===
//138
	InitFeat(EVASION_FEAT, "скользкий тип", NORMAL_FTYPE, true, feat_app);
//139
	InitFeat(EXPEDIENT_CUT_FEAT, "порез", TECHNIQUE_FTYPE, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[EXPEDIENT_CUT_FEAT].GetBaseParameter = &GET_REAL_DEX;
	feat_info[EXPEDIENT_CUT_FEAT].GetEffectParameter = &GET_REAL_STR;
	feat_info[EXPEDIENT_CUT_FEAT].uses_weapon_skill = true;
	feat_info[EXPEDIENT_CUT_FEAT].always_available = false;
	feat_info[EXPEDIENT_CUT_FEAT].damage_bonus = 7;
	feat_info[EXPEDIENT_CUT_FEAT].success_degree_damage_bonus = 10;
	feat_info[EXPEDIENT_CUT_FEAT].CalcSituationalDamageFactor = ([](CharData */*ch*/) -> float { return 1.0; });
	feat_info[EXPEDIENT_CUT_FEAT].CalcSituationalRollBonus =
		([](CharData */*ch*/, CharData * enemy) -> int {
			if (AFF_FLAGGED(enemy, EAffectFlag::AFF_BLIND)) {
				return 60;
			}
			return 0;
		});

	feat_info[EXPEDIENT_CUT_FEAT].item_kits.reserve(3);

	auto techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(2);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjData::ITEM_WEAPON, ESkill::kShortBlades));
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjData::ITEM_WEAPON, ESkill::kShortBlades));
	feat_info[EXPEDIENT_CUT_FEAT].item_kits.push_back(std::move(techniqueItemKit));

	techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(2);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjData::ITEM_WEAPON, ESkill::kSpades));
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjData::ITEM_WEAPON, ESkill::kSpades));
	feat_info[EXPEDIENT_CUT_FEAT].item_kits.push_back(std::move(techniqueItemKit));

	techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(2);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjData::ITEM_WEAPON, ESkill::kPicks));
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjData::ITEM_WEAPON, ESkill::kPicks));
	feat_info[EXPEDIENT_CUT_FEAT].item_kits.push_back(std::move(techniqueItemKit));

//140
	InitFeat(SHOT_FINESSE_FEAT, "ловкий выстрел", NORMAL_FTYPE, true, feat_app);
//141
	InitFeat(OBJECT_ENCHANTER_FEAT, "наложение чар", NORMAL_FTYPE, true, feat_app);
//142
	InitFeat(DEFT_SHOOTER_FEAT, "ловкий стрелок", NORMAL_FTYPE, true, feat_app);
//143
	InitFeat(MAGIC_SHOOTER_FEAT, "магический выстрел", NORMAL_FTYPE, true, feat_app);
//144
	InitFeat(THROW_WEAPON_FEAT, "метнуть", TECHNIQUE_FTYPE, true, feat_app,
			 100, ESkill::kThrow, ESaving::kReflex);
	feat_info[THROW_WEAPON_FEAT].GetBaseParameter = &GET_REAL_DEX;
	feat_info[THROW_WEAPON_FEAT].GetEffectParameter = &GET_REAL_STR;
	feat_info[THROW_WEAPON_FEAT].uses_weapon_skill = false;
	feat_info[THROW_WEAPON_FEAT].always_available = true;
	feat_info[THROW_WEAPON_FEAT].damage_bonus = 5;
	feat_info[THROW_WEAPON_FEAT].success_degree_damage_bonus = 5;
	feat_info[THROW_WEAPON_FEAT].CalcSituationalDamageFactor =
		([](CharData *ch) -> float {
			return (0.1 * can_use_feat(ch, POWER_THROW_FEAT) + 0.1 * can_use_feat(ch, DEADLY_THROW_FEAT));
		});
	feat_info[THROW_WEAPON_FEAT].CalcSituationalRollBonus =
		([](CharData *ch, CharData * /* enemy */) -> int {
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
				return -60;
			}
			return 0;
		});
	// Это ужасно, понимаю, но не хочется возиться с написанием мутной функции с переменным числоа аргументов,
	// потому что при введении конфига, чем я планирую заняться в ближайшее время, все равно ее придется переписывать.
	//TODO: Не забыть переписать этот бордель
	feat_info[THROW_WEAPON_FEAT].item_kits.reserve(2);

	techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[THROW_WEAPON_FEAT].item_kits.push_back(std::move(techniqueItemKit));

	techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[THROW_WEAPON_FEAT].item_kits.push_back(std::move(techniqueItemKit));
//145
	InitFeat(SHADOW_THROW_FEAT, "змеево оружие", TECHNIQUE_FTYPE, true, feat_app,
			 100, ESkill::kDarkMagic, ESaving::kWill);
	feat_info[SHADOW_THROW_FEAT].GetBaseParameter = &GET_REAL_DEX;
	feat_info[SHADOW_THROW_FEAT].GetEffectParameter = &GET_REAL_INT;
	feat_info[SHADOW_THROW_FEAT].damage_bonus = -30;
	feat_info[SHADOW_THROW_FEAT].success_degree_damage_bonus = 1;
	feat_info[SHADOW_THROW_FEAT].uses_weapon_skill = false;
	feat_info[SHADOW_THROW_FEAT].CalcSituationalRollBonus =
		([](CharData *ch, CharData * /* enemy */) -> int {
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
				return -60;
			}
			return 0;
		});

	feat_info[SHADOW_THROW_FEAT].item_kits.reserve(2);
	techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_WIELD, ObjData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[SHADOW_THROW_FEAT].item_kits.push_back(std::move(techniqueItemKit));
	techniqueItemKit = std::make_unique<TechniqueItemKit>();
	techniqueItemKit->reserve(1);
	techniqueItemKit->push_back(TechniqueItem(WEAR_HOLD, ObjData::ITEM_WEAPON,
											  ESkill::kAny, EExtraFlag::ITEM_THROWING));
	feat_info[SHADOW_THROW_FEAT].item_kits.push_back(std::move(techniqueItemKit));
//146
	InitFeat(SHADOW_DAGGER_FEAT, "змеев кинжал", NORMAL_FTYPE, true, feat_app,
			 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[SHADOW_DAGGER_FEAT].GetBaseParameter = &GET_REAL_INT;
	feat_info[SHADOW_DAGGER_FEAT].uses_weapon_skill = false;
//147
	InitFeat(SHADOW_SPEAR_FEAT, "змеево копьё", NORMAL_FTYPE, true,
			 feat_app, 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[SHADOW_SPEAR_FEAT].GetBaseParameter = &GET_REAL_INT;
	feat_info[SHADOW_SPEAR_FEAT].uses_weapon_skill = false;
//148
	InitFeat(SHADOW_CLUB_FEAT, "змеева палица", NORMAL_FTYPE, true, feat_app,
			 80, ESkill::kDarkMagic, ESaving::kStability);
	feat_info[SHADOW_CLUB_FEAT].GetBaseParameter = &GET_REAL_INT;
	feat_info[SHADOW_CLUB_FEAT].uses_weapon_skill = false;
//149
	InitFeat(DOUBLE_THROW_FEAT, "двойной бросок", ACTIVATED_FTYPE, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[DOUBLE_THROW_FEAT].GetBaseParameter = &GET_REAL_DEX;
//150
	InitFeat(TRIPLE_THROW_FEAT, "тройной бросок", ACTIVATED_FTYPE, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[TRIPLE_THROW_FEAT].GetBaseParameter = &GET_REAL_DEX;
//1151
	InitFeat(POWER_THROW_FEAT, "размах", NORMAL_FTYPE, true, feat_app, 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[POWER_THROW_FEAT].GetBaseParameter = &GET_REAL_STR;
//152
	InitFeat(DEADLY_THROW_FEAT, "широкий размах", NORMAL_FTYPE, true, feat_app,
			 100, ESkill::kPunch, ESaving::kReflex);
	feat_info[DEADLY_THROW_FEAT].GetBaseParameter = &GET_REAL_STR;
//153
	InitFeat(TURN_UNDEAD_FEAT, "turn undead", TECHNIQUE_FTYPE, true, feat_app,
			 70, ESkill::kTurnUndead, ESaving::kStability);
	feat_info[TURN_UNDEAD_FEAT].GetBaseParameter = &GET_REAL_INT;
	feat_info[TURN_UNDEAD_FEAT].GetEffectParameter = &GET_REAL_WIS;
	feat_info[TURN_UNDEAD_FEAT].uses_weapon_skill = false;
	feat_info[TURN_UNDEAD_FEAT].always_available = true;
	feat_info[TURN_UNDEAD_FEAT].damage_bonus = -30;
	feat_info[TURN_UNDEAD_FEAT].success_degree_damage_bonus = 2;
//154
	InitFeat(MULTI_CAST_FEAT, "изощренные чары", NORMAL_FTYPE, true, feat_app);
//155	
	InitFeat(MAGICAL_SHIELD_FEAT, "заговоренный щит", NORMAL_FTYPE, true, feat_app);
// 156
	InitFeat(ANIMAL_MASTER_FEAT, "хозяин животных", NORMAL_FTYPE, true, feat_app);
}

const char *GetFeatName(int num) {
	if (num > 0 && num < kMaxFeats) {
		return (feat_info[num].name);
	} else {
		if (num == -1) {
			return "UNUSED";
		} else {
			return "UNDEFINED";
		}
	}
}

bool can_use_feat(const CharData *ch, int feat) {
	if (feat_info[feat].always_available) {
		return true;
	};
	if ((feat == INCORRECT_FEAT) || !HAVE_FEAT(ch, feat)) {
		return false;
	};
	if (IS_NPC(ch)) {
		return true;
	};
	if (NUM_LEV_FEAT(ch) < feat_info[feat].slot[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
		return false;
	};
	if (GET_REAL_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
		return false;
	};

	switch (feat) {
		case WEAPON_FINESSE_FEAT:
		case SHOT_FINESSE_FEAT: return (GET_REAL_DEX(ch) > GET_REAL_STR(ch) && GET_REAL_DEX(ch) > 17);
			break;
		case PARRY_ARROW_FEAT: return (GET_REAL_DEX(ch) > 15);
			break;
		case POWER_ATTACK_FEAT: return (GET_REAL_STR(ch) > 19);
			break;
		case GREAT_POWER_ATTACK_FEAT: return (GET_REAL_STR(ch) > 21);
			break;
		case AIMING_ATTACK_FEAT: return (GET_REAL_DEX(ch) > 15);
			break;
		case GREAT_AIMING_ATTACK_FEAT: return (GET_REAL_DEX(ch) > 17);
			break;
		case DOUBLESHOT_FEAT: return (ch->get_skill(ESkill::kBows) > 39);
			break;
		case MASTER_JEWELER_FEAT: return (ch->get_skill(ESkill::kJewelry) > 59);
			break;
		case SKILLED_TRADER_FEAT: return ((ch->get_level() + GET_REAL_REMORT(ch) / 3) > 19);
			break;
		case MAGIC_USER_FEAT: return (GET_REAL_LEVEL(ch) < 25);
			break;
		case LIVE_SHIELD_FEAT: return (ch->get_skill(ESkill::kRescue) > 124);
			break;
		case SHADOW_THROW_FEAT: return (ch->get_skill(ESkill::kDarkMagic) > 120);
			break;
		case ANIMAL_MASTER_FEAT: return (ch->get_skill(ESkill::kMindMagic) > 80);
			break;
			// Костыльный блок работы скирмишера где не нужно
			// Svent TODO Для абилок не забыть реализовать провкрку состояния персонажа
		case SKIRMISHER_FEAT:
			return !(AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)
				|| GET_POS(ch) < EPosition::kFight);
			break;
		default: return true;
			break;
	}
	return true;
}

bool can_get_feat(CharData *ch, int feat) {
	int i, count = 0;
	if (feat <= 0 || feat >= kMaxFeats) {
		sprintf(buf, "Неверный номер способности (feat=%d, ch=%s) передан в features::can_get_feat!",
				feat, ch->get_name().c_str());
		mudlog(buf, BRF, kLevelImmortal, SYSLOG, true);
		return false;
	}
	// Если фит доступен всем и всегда - неачем его куда-то "заучиввать".
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
		case PARRY_ARROW_FEAT: return (ch->get_skill(ESkill::kMultiparry) || ch->get_skill(ESkill::kParry));
			break;
		case CONNOISEUR_FEAT: return (ch->get_skill(ESkill::kIdentify));
			break;
		case EXORCIST_FEAT: return (ch->get_skill(ESkill::kTurnUndead));
			break;
		case HEALER_FEAT: return (ch->get_skill(ESkill::kFirstAid));
			break;
		case STEALTHY_FEAT:
			return (ch->get_skill(ESkill::kHide) || ch->get_skill(ESkill::kSneak) || ch->get_skill(ESkill::kDisguise));
			break;
		case TRACKER_FEAT: return (ch->get_skill(ESkill::kTrack) || ch->get_skill(ESkill::kSense));
			break;
		case PUNCH_MASTER_FEAT:
		case CLUBS_MASTER_FEAT:
		case AXES_MASTER_FEAT:
		case LONGS_MASTER_FEAT:
		case SHORTS_MASTER_FEAT:
		case NONSTANDART_MASTER_FEAT:
		case BOTHHANDS_MASTER_FEAT:
		case PICK_MASTER_FEAT:
		case SPADES_MASTER_FEAT:
		case BOWS_MASTER_FEAT:
			if (!HAVE_FEAT(ch, (ubyte) feat_info[feat].affected[0].location)) {
				return false;
			}
			for (i = PUNCH_MASTER_FEAT; i <= BOWS_MASTER_FEAT; i++) {
				if (HAVE_FEAT(ch, i)) {
					count++;
				}
			}
			if (count >= 1 + GET_REAL_REMORT(ch) / 7) {
				return false;
			}
			break;
		case SPIRIT_WARRIOR_FEAT: return (HAVE_FEAT(ch, GREAT_FORTITUDE_FEAT));
			break;
		case NIMBLE_FINGERS_FEAT: return (ch->get_skill(ESkill::kSteal) || ch->get_skill(ESkill::kPickLock));
			break;
		case GREAT_POWER_ATTACK_FEAT: return (HAVE_FEAT(ch, POWER_ATTACK_FEAT));
			break;
		case PUNCH_FOCUS_FEAT:
		case CLUB_FOCUS_FEAT:
		case AXES_FOCUS_FEAT:
		case LONGS_FOCUS_FEAT:
		case SHORTS_FOCUS_FEAT:
		case NONSTANDART_FOCUS_FEAT:
		case BOTHHANDS_FOCUS_FEAT:
		case PICK_FOCUS_FEAT:
		case SPADES_FOCUS_FEAT:
		case BOWS_FOCUS_FEAT:
			if (!ch->get_skill(feat_info[feat].base_skill)) {
				return false;
			}

			for (i = PUNCH_FOCUS_FEAT; i <= BOWS_FOCUS_FEAT; i++) {
				if (HAVE_FEAT(ch, i)) {
					count++;
				}
			}

			if (count >= 2 + GET_REAL_REMORT(ch) / 6) {
				return false;
			}
			break;
		case GREAT_AIMING_ATTACK_FEAT: return (HAVE_FEAT(ch, AIMING_ATTACK_FEAT));
			break;
		case DOUBLESHOT_FEAT: return (HAVE_FEAT(ch, BOWS_FOCUS_FEAT) && ch->get_skill(ESkill::kBows) > 39);
			break;
		case MASTER_JEWELER_FEAT: return (ch->get_skill(ESkill::kJewelry) > 59);
			break;
		case EXPEDIENT_CUT_FEAT:
			return (HAVE_FEAT(ch, SHORTS_MASTER_FEAT)
				|| HAVE_FEAT(ch, PICK_MASTER_FEAT)
				|| HAVE_FEAT(ch, LONGS_MASTER_FEAT)
				|| HAVE_FEAT(ch, SPADES_MASTER_FEAT)
				|| HAVE_FEAT(ch, BOTHHANDS_MASTER_FEAT));
			break;
		case SKIRMISHER_FEAT: return (ch->get_skill(ESkill::kRescue));
			break;
		case TACTICIAN_FEAT: return (ch->get_skill(ESkill::kLeadership) > 99);
			break;
		case SHADOW_THROW_FEAT: return (HAVE_FEAT(ch, POWER_THROW_FEAT) && (ch->get_skill(ESkill::kDarkMagic) > 120));
			break;
		case SHADOW_DAGGER_FEAT:
		case SHADOW_SPEAR_FEAT: [[fallthrough]];
		case SHADOW_CLUB_FEAT: return (HAVE_FEAT(ch, SHADOW_THROW_FEAT) && (ch->get_skill(ESkill::kDarkMagic) > 130));
			break;
		case DOUBLE_THROW_FEAT: return (HAVE_FEAT(ch, POWER_THROW_FEAT) && (ch->get_skill(ESkill::kThrow) > 100));
			break;
		case TRIPLE_THROW_FEAT: return (HAVE_FEAT(ch, DEADLY_THROW_FEAT) && (ch->get_skill(ESkill::kThrow) > 130));
			break;
		case POWER_THROW_FEAT: return (ch->get_skill(ESkill::kThrow) > 90);
			break;
		case DEADLY_THROW_FEAT: return (HAVE_FEAT(ch, POWER_THROW_FEAT) && (ch->get_skill(ESkill::kThrow) > 110));
			break;
		default: return true;
			break;
	}

	return true;
}

bool CheckVacantFeatSlot(CharData *ch, int feat) {
	int i, lowfeat, hifeat;

	if (feat_info[feat].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
		|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), feat))
		return true;

	//сколько у нас вообще способностей, у которых слот меньше требуемого, и сколько - тех, у которых больше или равно?
	lowfeat = 0;
	hifeat = 0;

	//Мы не можем просто учесть кол-во способностей меньше требуемого и больше требуемого,
	//т.к. возможны свободные слоты меньше требуемого, и при этом верхние заняты все
	auto slot_list = std::vector<int>();
	for (i = 1; i < kMaxFeats; ++i) {
		if (feat_info[i].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
			|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), i))
			continue;

		if (HAVE_FEAT(ch, i)) {
			if (FEAT_SLOT(ch, i) >= FEAT_SLOT(ch, feat)) {
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
	if (NUM_LEV_FEAT(ch) - FEAT_SLOT(ch, feat) - hifeat - MAX(0, lowfeat - FEAT_SLOT(ch, feat)) > 0)
		return true;

	//oops.. слотов нет
	return false;
}

int GetModifier(int feat, int location) {
	for (int i = 0; i < kMaxFeatAffect; i++) {
		if (feat_info[feat].affected[i].location == location) {
			return (int) feat_info[feat].affected[i].modifier;
		}
	}
	return 0;
}

void CheckBerserk(CharData *ch) {
	struct TimedFeat timed;
	int prob;

	if (affected_by_spell(ch, kSpellBerserk) &&
		(GET_HIT(ch) > GET_REAL_MAX_HIT(ch) / 2)) {
		affect_from_char(ch, kSpellBerserk);
		send_to_char("Предсмертное исступление оставило вас.\r\n", ch);
	}

	if (can_use_feat(ch, BERSERK_FEAT) && ch->get_fighting() &&
		!IsTimed(ch, BERSERK_FEAT) && !AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK)
		&& (GET_HIT(ch) < GET_REAL_MAX_HIT(ch) / 4)) {
		CharData *vict = ch->get_fighting();
		timed.feat = BERSERK_FEAT;
		timed.time = 4;
		ImposeTimedFeat(ch, &timed);

		Affect<EApplyLocation> af;
		af.type = kSpellBerserk;
		af.duration = CalcDuration(ch, 1, 60, 30, 0, 0);
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.battleflag = 0;

		prob = IS_NPC(ch) ? 601 : (751 - GET_REAL_LEVEL(ch) * 5);
		if (number(1, 1000) < prob) {
			af.bitvector = to_underlying(EAffectFlag::AFF_BERSERK);
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

	if (IS_NPC(ch) || !can_use_feat(ch, LIGHT_WALK_FEAT)) {
		send_to_char("Вы не можете этого.\r\n", ch);
		return;
	}

	if (ch->ahorse()) {
		act("Позаботьтесь сперва о мягких тапочках для $N3...", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}

	if (affected_by_spell(ch, kSpellLightWalk)) {
		send_to_char("Вы уже двигаетесь легким шагом.\r\n", ch);
		return;
	}
	if (IsTimed(ch, LIGHT_WALK_FEAT)) {
		send_to_char("Вы слишком утомлены для этого.\r\n", ch);
		return;
	}

	affect_from_char(ch, kSpellLightWalk);

	timed.feat = LIGHT_WALK_FEAT;
	timed.time = 24;
	ImposeTimedFeat(ch, &timed);

	send_to_char("Хорошо, вы попытаетесь идти, не оставляя лишних следов.\r\n", ch);
	Affect<EApplyLocation> af;
	af.type = kSpellLightWalk;
	af.duration = CalcDuration(ch, 2, GET_REAL_LEVEL(ch), 5, 2, 8);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.battleflag = 0;
	if (number(1, 1000) > number(1, GET_REAL_DEX(ch) * 50)) {
		af.bitvector = 0;
		send_to_char("Вам не хватает ловкости...\r\n", ch);
	} else {
		af.bitvector = to_underlying(EAffectFlag::AFF_LIGHT_WALK);
		send_to_char("Ваши шаги стали легче перышка.\r\n", ch);
	}
	affect_to_char(ch, af);
}

void do_fit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	ObjData *obj;
	CharData *vict;
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	//отключено пока для не-иммов
	if (GET_REAL_LEVEL(ch) < kLevelImmortal) {
		send_to_char("Вы не можете этого.", ch);
		return;
	};

	//Может ли игрок использовать эту способность?
	if ((subcmd == SCMD_DO_ADAPT) && !can_use_feat(ch, TO_FIT_ITEM_FEAT)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	};
	if ((subcmd == SCMD_MAKE_OVER) && !can_use_feat(ch, TO_FIT_CLOTHCES_FEAT)) {
		send_to_char("Вы не умеете этого.", ch);
		return;
	};

	//Есть у нас предмет, который хотят переделать?
	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char("Что вы хотите переделать?\r\n", ch);
		return;
	};

	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
		sprintf(buf, "У вас нет \'%s\'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	};

	//На кого переделываем?
	argument = one_argument(argument, arg2);
	if (!(vict = get_char_vis(ch, arg2, FIND_CHAR_ROOM))) {
		send_to_char("Под кого вы хотите переделать эту вещь?\r\n Нет такого создания в округе!\r\n", ch);
		return;
	};

	//Предмет уже имеет владельца
	if (GET_OBJ_OWNER(obj) != 0) {
		send_to_char("У этой вещи уже есть владелец.\r\n", ch);
		return;

	};

	if ((GET_OBJ_WEAR(obj) <= 1) || OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF)) {
		send_to_char("Этот предмет невозможно переделать.\r\n", ch);
		return;
	}

	switch (subcmd) {
		case SCMD_DO_ADAPT:
			if (GET_OBJ_MATER(obj) != ObjData::MAT_NONE
				&& GET_OBJ_MATER(obj) != ObjData::MAT_BULAT
				&& GET_OBJ_MATER(obj) != ObjData::MAT_BRONZE
				&& GET_OBJ_MATER(obj) != ObjData::MAT_IRON
				&& GET_OBJ_MATER(obj) != ObjData::MAT_STEEL
				&& GET_OBJ_MATER(obj) != ObjData::MAT_SWORDSSTEEL
				&& GET_OBJ_MATER(obj) != ObjData::MAT_COLOR
				&& GET_OBJ_MATER(obj) != ObjData::MAT_WOOD
				&& GET_OBJ_MATER(obj) != ObjData::MAT_SUPERWOOD
				&& GET_OBJ_MATER(obj) != ObjData::MAT_GLASS) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				send_to_char(buf, ch);
				return;
			}
			break;
		case SCMD_MAKE_OVER:
			if (GET_OBJ_MATER(obj) != ObjData::MAT_BONE
				&& GET_OBJ_MATER(obj) != ObjData::MAT_MATERIA
				&& GET_OBJ_MATER(obj) != ObjData::MAT_SKIN
				&& GET_OBJ_MATER(obj) != ObjData::MAT_ORGANIC) {
				sprintf(buf, "К сожалению %s сделан%s из неподходящего материала.\r\n",
						GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_SUF_6(obj));
				send_to_char(buf, ch);
				return;
			}
			break;
		default:
			send_to_char("Это какая-то ошибка...\r\n", ch);
			return;
			break;
	};
	obj->set_owner(GET_UNIQUE(vict));
	sprintf(buf, "Вы долго пыхтели и сопели, переделывая работу по десять раз.\r\n");
	sprintf(buf + strlen(buf), "Вы извели кучу времени и 10000 кун золотом.\r\n");
	sprintf(buf + strlen(buf), "В конце-концов подогнали %s точно по мерке %s.\r\n",
			GET_OBJ_PNAME(obj, 3).c_str(), GET_PAD(vict, 1));

	send_to_char(buf, ch);

}

#include "game_classes/classes_spell_slots.h" // удалить после вырезания do_spell_capable
#include "magic/spells_info.h"
#define SpINFO spell_info[spellnum]
// Вложить закл в клона
void do_spell_capable(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	using PlayerClass::slot_for_char;

	struct TimedFeat timed;

	if (!IS_IMPL(ch) && (IS_NPC(ch) || !can_use_feat(ch, SPELL_CAPABLE_FEAT))) {
		send_to_char("Вы не столь могущественны.\r\n", ch);
		return;
	}

	if (IsTimed(ch, SPELL_CAPABLE_FEAT) && !IS_IMPL(ch)) {
		send_to_char("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	char *s;
	int spellnum;

	if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
		send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
		return;
	}

	s = strtok(argument, "'*!");
	if (s == nullptr) {
		send_to_char("ЧТО вы хотите колдовать?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (s == nullptr) {
		send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	spellnum = FixNameAndFindSpellNum(s);
	if (spellnum < 1 || spellnum > kSpellCount) {
		send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	if ((!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellTemp | kSpellKnow) ||
		GET_REAL_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)) &&
		(GET_REAL_LEVEL(ch) < kLevelGreatGod) && (!IS_NPC(ch))) {
		if (GET_REAL_LEVEL(ch) < MIN_CAST_LEV(SpINFO, ch)
			|| GET_REAL_REMORT(ch) < MIN_CAST_REM(SpINFO, ch)
			|| slot_for_char(ch, SpINFO.slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0) {
			send_to_char("Рано еще вам бросаться такими словами!\r\n", ch);
			return;
		} else {
			send_to_char("Было бы неплохо изучить, для начала, это заклинание...\r\n", ch);
			return;
		}
	}

	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	Follower *k;
	CharData *follower = nullptr;
	for (k = ch->followers; k; k = k->next) {
		if (AFF_FLAGGED(k->ch, EAffectFlag::AFF_CHARM)
			&& k->ch->get_master() == ch
			&& MOB_FLAGGED(k->ch, MOB_CLONE)
			&& !affected_by_spell(k->ch, kSpellCapable)
			&& ch->in_room == IN_ROOM(k->ch)) {
			follower = k->ch;
			break;
		}
	}
	if (!GET_SPELL_MEM(ch, spellnum) && !IS_IMMORTAL(ch)) {
		send_to_char("Вы совершенно не помните, как произносится это заклинание...\r\n", ch);
		return;
	}

	if (!follower) {
		send_to_char("Хорошо бы найти подходящую цель для этого.\r\n", ch);
		return;
	}

	act("Вы принялись зачаровывать $N3.", false, ch, nullptr, follower, kToChar);
	act("$n принял$u делать какие-то пассы и что-то бормотать в сторону $N3.", false, ch, nullptr, follower, kToRoom);

	GET_SPELL_MEM(ch, spellnum)--;
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
		MemQ_remember(ch, spellnum);

	if (!IS_SET(SpINFO.routines, kMagDamage) || !SpINFO.violent ||
		IS_SET(SpINFO.routines, kMagMasses) || IS_SET(SpINFO.routines, kMagGroups) ||
		IS_SET(SpINFO.routines, kMagAreas)) {
		send_to_char("Вы конечно мастер, но не такой магии.\r\n", ch);
		return;
	}
	affect_from_char(ch, SPELL_CAPABLE_FEAT);

	timed.feat = SPELL_CAPABLE_FEAT;

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
	Affect<EApplyLocation> af;
	af.type = kSpellCapable;
	af.duration = 48;
	if (GET_REAL_REMORT(ch) > 0) {
		af.modifier = GET_REAL_REMORT(ch) * 4;//вешаецо аффект который дает +морт*4 касту
		af.location = APPLY_CAST_SUCCESS;
	} else {
		af.modifier = 0;
		af.location = APPLY_NONE;
	}
	af.battleflag = 0;
	af.bitvector = 0;
	affect_to_char(follower, af);
	follower->mob_specials.capable_spell = spellnum;
}

void SetRaceFeats(CharData *ch) {
	std::vector<int> feat_list = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int &i: feat_list) {
		if (can_get_feat(ch, i)) {
			SET_FEAT(ch, i);
		}
	}
}

void UnsetRaceFeats(CharData *ch) {
	std::vector<int> feat_list = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int &i: feat_list) {
		UNSET_FEAT(ch, i);
	}
}

void setInbornFeaturesOfClass(CharData *ch) {
	for (int i = 1; i < kMaxFeats; ++i) {
		if (can_get_feat(ch, i) && feat_info[i].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) {
			SET_FEAT(ch, i);
		}
	}
}

void SetInbornFeats(CharData *ch) {
	setInbornFeaturesOfClass(ch);
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
	mudlog(buf, BRF, kLevelGod, SYSLOG, true);
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
		i.location = APPLY_NONE;
		i.modifier = 0;
	}
}

bool TryFlipActivatedFeature(CharData *ch, char *argument) {
	int featureNum = GetFeatureNum(argument);
	if (featureNum <= INCORRECT_FEAT) {
		return false;
	}
	if (!CheckAccessActivatedFeat(ch, featureNum)) {
		return true;
	};
	if (PRF_FLAGGED(ch, GetPrfWithFeatNumber(featureNum))) {
		DeactivateFeature(ch, featureNum);
	} else {
		ActivateFeat(ch, featureNum);
	}

	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	return true;
}

void ActivateFeat(CharData *ch, int feat_num) {
	switch (feat_num) {
		case POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			PRF_FLAGS(ch).set(PRF_POWERATTACK);
			break;
		case GREAT_POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			PRF_FLAGS(ch).set(PRF_GREATPOWERATTACK);
			break;
		case AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			PRF_FLAGS(ch).set(PRF_AIMINGATTACK);
			break;
		case GREAT_AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			PRF_FLAGS(ch).set(PRF_GREATAIMINGATTACK);
			break;
		case SKIRMISHER_FEAT:
			if (!AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)) {
				send_to_char(ch,
							 "Голос десятника Никифора вдруг рявкнул: \"%s, тюрюхайло! 'В шеренгу по одному' иначе сполняется!\"\r\n",
							 ch->get_name().c_str());
				return;
			}
			if (PRF_FLAGGED(ch, PRF_SKIRMISHER)) {
				send_to_char("Вы уже стоите в передовом строю.\r\n", ch);
				return;
			}
			PRF_FLAGS(ch).set(PRF_SKIRMISHER);
			send_to_char("Вы протиснулись вперед и встали в строй.\r\n", ch);
			act("$n0 протиснул$u вперед и встал$g в строй.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			break;
		case DOUBLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_TRIPLE_THROW);
			PRF_FLAGS(ch).set(PRF_DOUBLE_THROW);
			break;
		case TRIPLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_DOUBLE_THROW);
			PRF_FLAGS(ch).set(PRF_TRIPLE_THROW);
			break;
		default: break;
	}
	send_to_char(ch,
				 "%sВы решили использовать способность '%s'.%s\r\n",
				 CCIGRN(ch, C_SPR),
				 feat_info[feat_num].name,
				 CCNRM(ch, C_OFF));
}

void DeactivateFeature(CharData *ch, int feat_num) {
	switch (feat_num) {
		case POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_POWERATTACK);
			break;
		case GREAT_POWER_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
			break;
		case AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_AIMINGATTACK);
			break;
		case GREAT_AIMING_ATTACK_FEAT: PRF_FLAGS(ch).unset(PRF_GREATAIMINGATTACK);
			break;
		case SKIRMISHER_FEAT: PRF_FLAGS(ch).unset(PRF_SKIRMISHER);
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP)) {
				send_to_char("Вы решили, что в обозе вам будет спокойней.\r\n", ch);
				act("$n0 тактически отступил$g в тыл отряда.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			}
			break;
		case DOUBLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_DOUBLE_THROW);
			break;
		case TRIPLE_THROW_FEAT: PRF_FLAGS(ch).unset(PRF_TRIPLE_THROW);
			break;
		default: break;
	}
	send_to_char(ch,
				 "%sВы прекратили использовать способность '%s'.%s\r\n",
				 CCIGRN(ch, C_SPR),
				 feat_info[feat_num].name,
				 CCNRM(ch, C_OFF));
}

bool CheckAccessActivatedFeat(CharData *ch, int feat_num) {
	if (!can_use_feat(ch, feat_num)) {
		send_to_char("Вы не в состоянии использовать эту способность.\r\n", ch);
		return false;
	}
	if (feat_info[feat_num].type != ACTIVATED_FTYPE) {
		send_to_char("Эту способность невозможно применить таким образом.\r\n", ch);
		return false;
	}

	return true;
}

int GetFeatureNum(char *feat_name) {
	skip_spaces(&feat_name);
	return FindFeatNum(feat_name);
}

/*
 TODO: при переписывании способностей переделать на композицию или интерфейс
*/
Bitvector GetPrfWithFeatNumber(int feat_num) {
	switch (feat_num) {
		case POWER_ATTACK_FEAT: return PRF_POWERATTACK;
			break;
		case GREAT_POWER_ATTACK_FEAT: return PRF_GREATPOWERATTACK;
			break;
		case AIMING_ATTACK_FEAT: return PRF_AIMINGATTACK;
			break;
		case GREAT_AIMING_ATTACK_FEAT: return PRF_GREATAIMINGATTACK;
			break;
		case SKIRMISHER_FEAT: return PRF_SKIRMISHER;
			break;
		case DOUBLE_THROW_FEAT: return PRF_DOUBLE_THROW;
			break;
		case TRIPLE_THROW_FEAT: return PRF_TRIPLE_THROW;
			break;
		default: break;
	}

	return PRF_POWERATTACK;
}

/*
* Ситуативный бонус броска для "tactician feat" и "skirmisher feat":
* Каждый персонаж в строю прикрывает двух, третий дает штраф.
* Избыток "строевиков" повышает шанс на удачное срабатывание.
* Svent TODO: Придумать более универсальный механизм бонусов/штрафов в зависимости от данных абилки
*/
int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */) {
	ActionTargeting::FriendsRosterType roster{ch};
	int skirmishers = roster.count([](CharData *ch) { return PRF_FLAGGED(ch, PRF_SKIRMISHER); });
	int uncoveredSquadMembers = roster.amount() - skirmishers;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
		return (skirmishers * 2 - uncoveredSquadMembers) * abilities::kCircumstanceFactor - 40;
	};
	return (skirmishers * 2 - uncoveredSquadMembers) * abilities::kCircumstanceFactor;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
