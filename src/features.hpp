/*************************************************************************
*   File: features.hpp                                 Part of Bylins    *
*   Features code header                                                 *
*                                                                        *
*  $Author$                                                     * 
*  $Date$                                          *
*  $Revision$                                                  *
**************************************************************************/
#ifndef __FEATURES_HPP__
#define __FEATURES_HPP__
#include <bitset>
using std::bitset;

#define THAC0_FEAT			0   //DO NOT USED
#define BERSERK_FEAT			1   //предсмертная ярость
#define PARRY_ARROW_FEAT		2   //отбить стрелу
#define BLIND_FIGHT_FEAT		3   //слепой боец
#define CLEAVE_FEAT			4   //мгновенная атака
#define APPROACHING_ATTACK_FEAT		5   //встречная атака
#define DODGE_FEAT			6   //увертка
#define TWO_WEAPON_FIGHTING_FEAT	7   //двуручный бой
#define LIGHT_WALK_FEAT			8   //легкая поступь
#define DEPOSIT_FINDING_FEAT		9   //лозоходство
#define SPELL_SUBSTITUTE_FEAT		10   //подмена заклинания
#define POWER_ATTACK_FEAT		11  //мощная атака
#define WOODEN_SKIN_FEAT		12  //деревянная кожа
#define IRON_SKIN_FEAT			13  //железная кожа
#define CONNOISEUR_FEAT			14  //знаток
#define EXORCIST_FEAT			15  //изгоняющий нежить
#define HEALER_FEAT			16  //целитель
#define LIGHTING_REFLEX_FEAT		17  //мгновенная реакция
#define DRUNKARD_FEAT			18  //пьяница
#define DIEHARD_FEAT			19  //второе дыхание
#define ENDURANCE_FEAT			20  //выносливость
#define GREAT_FORTITUDE_FEAT		21  //сила духа
#define SKILL_FOCUS_FEAT		22  //любимое умение
#define STEALTHY_FEAT			23  //незаметность
#define WEAPON_FOCUS_FEAT		24  //излюбленное оружие
#define SPLENDID_HEALTH_FEAT		25  //богатырское здоровье
#define TRACKER_FEAT			26  //следопыт
#define WEAPON_FINESSE_FEAT		27  //проворство
#define COMBAT_CASTING_FEAT		28  //боевое чародейство
#define PUNCH_MASTER_FEAT		29  //кулачный боец
#define CLUBS_MASTER_FEAT		30  //мастер палицы
#define AXES_MASTER_FEAT		31  //мастер секиры
#define LONGS_MASTER_FEAT		32  //мастер меча
#define SHORTS_MASTER_FEAT		33  //мастер кинжала
#define NOSTANDART_MASTER_FEAT		34  //необычное оружие
#define BOTHHANDS_MASTER_FEAT		35  //мастер двуручника
#define PICK_MASTER_FEAT		36  //мастер ножа
#define SPADES_MASTER_FEAT		37  //мастер копья
#define BOWS_MASTER_FEAT		38  //мастер стрельбы
#define FOREST_PATHS_FEAT		39  //лесные тропы
#define MOUNTAIN_PATHS_FEAT		40  //горные тропы
#define LUCKY_FEAT			41  //счастливчик
#define SPIRIT_WARRIOR_FEAT		42  //боевой дух
#define FIGHTING_TRICK_FEAT		43  //боевая уловка
#define SPELL_FOCUS_FEAT		44  //любимое заклинание
#define THIRD_RING_SPELL_FEAT		45  //
#define FOURTH_RING_SPELL_FEAT		46  //
#define FIFTH_RING_SPELL_FEAT		47  //
#define SIXTH_RING_SPELL_FEAT		48  //
#define SEVENTH_RING_SPELL_FEAT		49  //
#define LEGIBLE_WRITTING_FEAT		50  //четкий почерк
#define BREW_POTION_FEAT		51  //варка зелья

/* MAX_FEATS определяется в structs.h */

#define UNUSED_FTYPE	-1
#define NORMAL_FTYPE	0
#define AFFECT_FTYPE	1
#define SKILL_MOD_FTYPE	2

#define MAX_FEAT_AFFECT	5 /* Количество пар "параметр-значение" */
#define MAX_ACC_FEAT	6 /* Максимально доступное количество не-врожденных способностей. */
#define LEV_ACC_FEAT	5 /* Раз в сколько уровней появляется новая способность */

/* Поля изменений для способностей (кроме AFFECT_FEAT_TYPE, для них сипользуются стардартные поля APPLY) */
#define FEAT_TIMER 1

extern struct feat_info_type feat_info[MAX_FEATS];

SPECIAL(feat_teacher);
int find_feat_num(char *name);
void assign_feats(void);
bool can_use_feat(CHAR_DATA *ch, int feat);
bool can_get_feat(CHAR_DATA *ch, int feat);
int number_of_features(CHAR_DATA *ch);

#endif

struct feat_info_type {
        int min_remort[NUM_CLASSES][NUM_KIN];
        int min_level[NUM_CLASSES][NUM_KIN];
	bool classknow[NUM_CLASSES][NUM_KIN];
	bool natural_classfeat[NUM_CLASSES][NUM_KIN];
	struct obj_affected_type
			affected[MAX_FEAT_AFFECT];
        int type;
        const char *name;
};
