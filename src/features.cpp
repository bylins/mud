/*************************************************************************
*   File: features.cpp                                 Part of Bylins    *
*   Features code                                                        *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                                                  *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "skills.h"
#include "features.hpp"

extern const char *unused_spellname;

struct feat_info_type feat_info[MAX_FEATS];

void unused_feat(int feat);
void assign_feats(void);
int find_feat_num(char *name);
bool can_use_feat(CHAR_DATA *ch, int feat);
bool can_get_feat(CHAR_DATA *ch, int feat);
int number_of_features(CHAR_DATA *ch);

class aff_array {
	public:
		explicit aff_array()
		{
			_pos = 0;
			for (i = 0; i < MAX_FEAT_AFFECT; i++) {
				affected[i].location = 0;
				affected[i].modifier = 0;
			}
		}

		int pos(int pos = -1)
		{
			if (pos == -1) {
				return _pos;
			} else if  (pos >= 0 && pos < MAX_FEAT_AFFECT) {
				_pos = pos;
				return _pos;;
			} 
			sprintf(buf, "SYSERR:: invalid arg passed to aff_aray.pos!");
			mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
		}

		void insert(byte location, sbyte modifier)
		{
			affected[_pos].location = location;
			affected[_pos].modifier = modifier;
			_pos++;
			if (_pos >= MAX_FEAT_AFFECT)
				_pos = 0;			
		}


		void clear()
		{
			_pos = 0;
			for (i = 0; i < MAX_FEAT_AFFECT; i++) {
				affected[i].location = 0;
				affected[i].modifier = 0;
			}
		}

		struct obj_affected_type
			affected[MAX_FEAT_AFFECT];
	private:
		int _pos, i;
}; 

int find_feat_num(char *name)
{
        int index, ok;
        char *temp, *temp2;
        char first[256], first2[256];
        for (index = 1; index < MAX_FEATS; index++) {
                if (is_abbrev(name, feat_info[index].name))
                        return (index);
                ok = TRUE;
                /* It won't be changed, but other uses of this function elsewhere may. */
                temp = any_one_arg((char *) feat_info[index].name, first);
                temp2 = any_one_arg(name, first2);
                while (*first && *first2 && ok) {
                        if (!is_abbrev(first2, first))
                                ok = FALSE;
                        temp = any_one_arg(temp, first);
                        temp2 = any_one_arg(temp2, first2);
                }

                if (ok && !*first2)
                        return (index);
        }
        return (-1);
}

void feato(int feat, const char *name, int type, aff_array app)
{
	int i,j;
	for (i = 0; i < NUM_CLASSES; i++)
		for (j=0; j < NUM_KIN; j++) {
			feat_info[feat].min_remort[i][j] = 0;
			feat_info[feat].min_level[i][j] = 0;
		}
	feat_info[feat].name = name;
	feat_info[feat].type = type;
	for (i = 0; i < MAX_FEAT_AFFECT; i++) {
		feat_info[feat].affected[i].location = app.affected[i].location;
		feat_info[feat].affected[i].modifier = app.affected[i].modifier;
	} 
}

void unused_feat(int feat)
{
        int i,j;

        for (i = 0; i < NUM_CLASSES; i++)
                for (j=0; j <NUM_KIN; j++){
                        feat_info[feat].min_remort[i][j] = MAX_REMORT;
                        feat_info[feat].min_level[i][j] = LVL_IMPL + 1;
			feat_info[feat].natural_classfeat[i][j] = FALSE;
			feat_info[feat].classknow[i][j] = FALSE;
                }

        feat_info[feat].name = unused_spellname;
	feat_info[feat].type = UNUSED_FTYPE;

	for (i = 0; i <= MAX_FEAT_AFFECT; i++) {
		feat_info[feat].affected[i].location = APPLY_NONE;
		feat_info[feat].affected[i].modifier = 0;
	} 
}

void assign_feats(void)
{ 
	int i;
	aff_array feat_app;
	for (i = 0; i < MAX_FEATS; i++) {
		unused_feat(i);
	}

//1
	feato(BERSERK_FEAT, "предсмертная ярость", NORMAL_FTYPE, feat_app);
/*
//2
	feato(PARRY_ARROW_FEAT, "отбить стрелу", NORMAL_FTYPE, feat_app);
//3
	feato(BLIND_FIGHT_FEAT, "слепой боец", NORMAL_FTYPE, feat_app);
//4
	feato(CLEAVE_FEAT, "мгновенная атака", NORMAL_FTYPE, feat_app);
//5
	feato(APPROACHING_ATTACK_FEAT, "встречная атака", NORMAL_FTYPE, feat_app);
//6
	feato(DODGE_FEAT, "увертка", NORMAL_FTYPE, feat_app);
//7
	feato(TWO_WEAPON_FIGHTING_FEAT, "двуручный бой", NORMAL_FTYPE, feat_app);
//8
	feato(LIGHT_WALK_FEAT, "легкая поступь", NORMAL_FTYPE, feat_app);
//9
	feato(DEPOSIT_FINDING_FEAT, "лозоходство", NORMAL_FTYPE, feat_app);
//10
	feato(SPELL_SUBSTITUTE_FEAT, "подмена заклинания", NORMAL_FTYPE, feat_app);
//11
	feato(POWER_ATTACK_FEAT, "мощная атака", NORMAL_FTYPE, feat_app);
*/
//12
	feat_app.insert(APPLY_RESIST_FIRE, 5);
	feat_app.insert(APPLY_RESIST_AIR, 5);
	feat_app.insert(APPLY_RESIST_WATER, 5);
	feat_app.insert(APPLY_RESIST_EARTH, 5);
	feat_app.insert(APPLY_ABSORBE, 5);
	feato(WOODEN_SKIN_FEAT, "деревянная кожа", AFFECT_FTYPE, feat_app);
	feat_app.clear();

//13
	feat_app.insert(APPLY_RESIST_FIRE, 10);
	feat_app.insert(APPLY_RESIST_AIR, 10);
	feat_app.insert(APPLY_RESIST_WATER, 10);
	feat_app.insert(APPLY_RESIST_EARTH, 10);
	feat_app.insert(APPLY_ABSORBE, 5);
	feato(IRON_SKIN_FEAT, "железная кожа", AFFECT_FTYPE, feat_app);
	feat_app.clear();

//14
	feat_app.insert(SKILL_IDENTIFY, 10);
	feat_app.insert(FEAT_TIMER, 4);
	feato(CONNOISEUR_FEAT, "глубокие познания", SKILL_MOD_FTYPE, feat_app);
	feat_app.clear();

//15
	feato(EXORCIST_FEAT, "изгоняющий нежить", NORMAL_FTYPE, feat_app);
/*
//16
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(HEALER_FEAT, "целитель", NORMAL_FTYPE, feat_app);
*/
//17
	feat_app.insert(APPLY_SAVING_REFLEX, -4);
	feato(LIGHTING_REFLEX_FEAT, "мгновенная реакция", AFFECT_FTYPE, feat_app);
	feat_app.clear();
/*
//18
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(DRUNKARD_FEAT, "пьяница", NORMAL_FTYPE, feat_app);
//19
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(DIEHARD_FEAT, "второе дыхание", NORMAL_FTYPE, feat_app);
*/
//20
	feat_app.insert(APPLY_MOVEREG, 20);
	feato(ENDURANCE_FEAT, "выносливость", AFFECT_FTYPE, feat_app);
	feat_app.clear();

//21
	feat_app.insert(APPLY_SAVING_WILL, -4);
	feat_app.insert(APPLY_SAVING_STABILITY, -4);
	feato(GREAT_FORTITUDE_FEAT, "сила духа", AFFECT_FTYPE, feat_app);
	feat_app.clear();
/*
//22
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SKILL_FOCUS_FEAT, "любимое умение", NORMAL_FTYPE, feat_app);
//23
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(STEALTHY_FEAT, "незаметность", NORMAL_FTYPE, feat_app);
//24
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(WEAPON_FOCUS_FEAT, "излюбленное оружие", NORMAL_FTYPE, feat_app);
*/
//25
	feat_app.insert(APPLY_HITREG, 10);
	feat_app.insert(APPLY_SAVING_CRITICAL, -4);
	feato(SPLENDID_HEALTH_FEAT, "богатырское здоровье", AFFECT_FTYPE, feat_app);
	feat_app.clear();
/*
//26
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(TRACKER_FEAT, "следопыт", NORMAL_FTYPE, feat_app);
//27
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(WEAPON_FINESSE_FEAT, "проворство", NORMAL_FTYPE, feat_app);
//28
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(COMBAT_CASTING_FEAT, "боевое чародейство", NORMAL_FTYPE, feat_app);
//29
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(PUNCH_MASTER_FEAT, "кулачный боец", NORMAL_FTYPE, feat_app);
//30
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(CLUBS_MASTER_FEAT, "мастер палицы", NORMAL_FTYPE, feat_app);
//31
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(AXES_MASTER_FEAT, "мастер секиры", NORMAL_FTYPE, feat_app);
//32
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(LONGS_MASTER_FEAT, "мастер меча", NORMAL_FTYPE, feat_app);
//33
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SHORTS_MASTER_FEAT, "мастер кинжала", NORMAL_FTYPE, feat_app);
//34
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(NOSTANDART_MASTER_FEAT, "необычное оружие", NORMAL_FTYPE, feat_app);
//35
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(BOTHHANDS_MASTER_FEAT, "мастер двуручника", NORMAL_FTYPE, feat_app);
//36
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(PICK_MASTER_FEAT, "мастер ножа", NORMAL_FTYPE, feat_app);
//37
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SPADES_MASTER_FEAT, "мастер копья", NORMAL_FTYPE, feat_app);
//38
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(BOWS_MASTER_FEAT, "мастер стрельбы", NORMAL_FTYPE, feat_app);
//39
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(FOREST_PATHS_FEAT, "лесные тропы", NORMAL_FTYPE, feat_app);
//40
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(MOUNTAIN_PATHS_FEAT, "горные тропы", NORMAL_FTYPE, feat_app);
*/
//41
	feat_app.insert(APPLY_MORALE, 5);
	feato(LUCKY_FEAT, "счастливчик", AFFECT_FTYPE, feat_app);
	feat_app.clear();
/*
//42
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SPIRIT_WARRIOR_FEAT, "боевой дух", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(FIGHTING_TRICK_FEAT, "боевая уловка", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SPELL_FOCUS_FEAT, "любимое заклинание", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(THIRD_RING_SPELL_FEAT, " ", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(FOURTH_RING_SPELL_FEAT, " ", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(FIFTH_RING_SPELL_FEAT, " ", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SIXTH_RING_SPELL_FEAT, " ", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(SEVENTH_RING_SPELL_FEAT, " ", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(LEGIBLE_WRITTING_FEAT, "четкий почерк", NORMAL_FTYPE, feat_app);
//
	feat_app = {0, 0, 0, 0, 0, 0, 0, 0};
	feato(BREW_POTION_FEAT, "варка зелья", NORMAL_FTYPE, feat_app); */

}

/* Может ли персонаж использовать способность? Проверка по уровню, ремортам, параметрам персонажа, требованиям. */
bool can_use_feat(CHAR_DATA *ch, int feat)
{
	if (!HAVE_FEAT(ch, feat))
		return FALSE;
	if (GET_LEVEL(ch) < feat_info[feat].min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])	
		return FALSE;
	if (GET_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])	
		return FALSE;

	return TRUE;
}

/* Можеи ли персонаж изучить эту способность? */
bool can_get_feat(CHAR_DATA *ch, int feat)
{
	if (feat <= 0 || feat >= MAX_FEATS) {
		sprintf(buf, "Неверный номер способности - %d!", feat);
		mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
		return FALSE;
	}

	if (!feat_info[feat].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])	
		return FALSE;
	if (GET_LEVEL(ch) < feat_info[feat].min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])	
		return FALSE;
	if (GET_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])	
		return FALSE;

	return TRUE;
}


/* Возвращает количество НЕ врожденных способностей */
int number_of_features(CHAR_DATA *ch)
{
	int i, j = 0;
	for (i = 1; i < MAX_FEATS; i++)
		if (HAVE_FEAT(ch, i) && !feat_info[i].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
			j++;
	return j;
}

