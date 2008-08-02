/* ************************************************************************
*   File: limits.cpp                                      Part of Bylins    *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "skills.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "screen.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "house.h"
#include "constants.h"
#include "exchange.h"
#include "top.h"
#include "deathtrap.hpp"
#include "ban.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "features.hpp"
#include "char.hpp"

extern int check_dupes_host(DESCRIPTOR_DATA * d, bool autocheck = 0);

extern room_rnum r_unreg_start_room;

extern CHAR_DATA *character_list;
extern CHAR_DATA *mob_proto;
extern OBJ_DATA *object_list;

extern DESCRIPTOR_DATA *descriptor_list;
extern struct zone_data *zone_table;
extern INDEX_DATA *obj_index;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int free_rent;
extern unsigned long dg_global_pulse;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern struct spell_create_type spell_create[];
extern int CheckProxy(DESCRIPTOR_DATA * ch);

void decrease_level(CHAR_DATA * ch);
int max_exp_gain_pc(CHAR_DATA * ch);
int max_exp_loss_pc(CHAR_DATA * ch);
int average_day_temp(void);
int calc_loadroom(CHAR_DATA * ch);

int mag_manacost(CHAR_DATA * ch, int spellnum);

/* local functions */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void Crash_rentsave(CHAR_DATA * ch, int cost);
int level_exp(CHAR_DATA * ch, int level);
void update_char_objects(CHAR_DATA * ch);	/* handler.cpp */
// Delete this, if you delete overflow fix in beat_points_update below.
void die(CHAR_DATA * ch, CHAR_DATA * killer);

/* When age < 20 return the value p0 */
/* When age in 20..29 calculate the line between p1 & p2 */
/* When age in 30..34 calculate the line between p2 & p3 */
/* When age in 35..49 calculate the line between p3 & p4 */
/* When age in 50..74 calculate the line between p4 & p5 */
/* When age >= 75 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

	if (age < 20)
		return (p0);	/* < 20   */
	else if (age <= 29)
		return (int) (p1 + (((age - 20) * (p2 - p1)) / 10));	/* 20..29 */
	else if (age <= 34)
		return (int) (p2 + (((age - 30) * (p3 - p2)) / 5));	/* 30..34 */
	else if (age <= 49)
		return (int) (p3 + (((age - 35) * (p4 - p3)) / 15));	/* 35..59 */
	else if (age <= 74)
		return (int) (p4 + (((age - 50) * (p5 - p4)) / 25));	/* 50..74 */
	else
		return (p6);	/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(CHAR_DATA * ch)
{
	int gain = 0, restore = int_app[GET_REAL_INT(ch)].mana_per_tic, percent = 100;
	int stopmem = FALSE;

	if (IS_NPC(ch)) {
		gain = GET_LEVEL(ch);
	} else {
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);

		if (!IS_MANA_CASTER(ch))
			gain =
			    graf(age(ch)->year, restore - 8, restore - 4, restore,
				 restore + 5, restore, restore - 4, restore - 8);
		else
			gain = mana_gain_cs[GET_REAL_INT(ch)];

		/* Room specification    */
		if (LIKE_ROOM(ch))
			percent += 25;
		/* Weather specification */
		if (average_day_temp() < -20)
			percent -= 10;
		else if (average_day_temp() < -10)
			percent -= 5;
	}

	if (world[IN_ROOM(ch)]->fires)
		percent += MAX(50, 10 + world[IN_ROOM(ch)]->fires * 5);

	if (AFF_FLAGGED(ch, AFF_DEAFNESS))
		percent += 15;

	/* Skill/Spell calculations */


	/* Position calculations    */
	if (FIGHTING(ch))
		percent -= 90;
	else
		switch (GET_POS(ch)) {
		case POS_SLEEPING:
			if (IS_MANA_CASTER(ch)) {
				percent += 80;
			} else {
				stopmem = TRUE;
				percent = 0;
			}
			break;
		case POS_RESTING:
			percent += 45;
			break;
		case POS_SITTING:
			percent += 30;
			break;
		case POS_STANDING:
			break;
		default:
			stopmem = TRUE;
			percent = 0;
			break;
		}

	if (!IS_MANA_CASTER(ch) &&
	    (AFF_FLAGGED(ch, AFF_HOLD) ||
	     AFF_FLAGGED(ch, AFF_BLIND) ||
	     AFF_FLAGGED(ch, AFF_SLEEP) ||
	    ((IN_ROOM(ch) != NOWHERE) && IS_DARK(IN_ROOM(ch)) && !can_use_feat(ch, DARK_READING_FEAT)))) {
		stopmem = TRUE;
		percent = 0;
	}
	if (!IS_NPC(ch)) {
		if (GET_COND(ch, FULL) == 0)
			percent -= 50;
		if (GET_COND(ch, THIRST) == 0)
			percent -= 25;
		if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
			percent -= 10;
	}

	if (!IS_MANA_CASTER(ch))
		percent += GET_MANAREG(ch);
	if (AFF_FLAGGED(ch, AFF_POISON) && percent > 0)
		percent /= 4;
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	return (stopmem ? 0 : gain);
}


/* Hitpoint gain pr. game hour */
int hit_gain(CHAR_DATA * ch)
{
	int gain = 0, restore = MAX(10, GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp), percent = 100;

	if (IS_NPC(ch))
		gain = GET_LEVEL(ch) + GET_REAL_CON(ch);
	else {
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);
		gain =
		    graf(age(ch)->year, restore - 3, restore, restore, restore - 2,
			 restore - 3, restore - 5, restore - 7);
		/* Room specification    */
		if (LIKE_ROOM(ch))
			percent += 25;
		/* Weather specification */
		if (average_day_temp() < -20)
			percent -= 15;
		else if (average_day_temp() < -10)
			percent -= 10;
	}

	if (world[IN_ROOM(ch)]->fires)
		percent += MAX(50, 10 + world[IN_ROOM(ch)]->fires * 5);

	/* Skill/Spell calculations */

	/* Position calculations    */
	switch (GET_POS(ch)) {
	case POS_SLEEPING:
		percent += 25;
		break;
	case POS_RESTING:
		percent += 15;
		break;
	case POS_SITTING:
		percent += 10;
		break;
	}

	if (!IS_NPC(ch)) {
		if (GET_COND(ch, FULL) == 0)
			percent -= 50;
		if (GET_COND(ch, THIRST) == 0)
			percent -= 25;
	}

	percent += GET_HITREG(ch);
	if (AFF_FLAGGED(ch, AFF_POISON) && percent > 0)
		percent /= 4;
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	if (!IS_NPC(ch)) {
		if (GET_POS(ch) == POS_INCAP || GET_POS(ch) == POS_MORTALLYW)
			gain = 0;
	}
	if (AFF_FLAGGED(ch, AFF_POISON))
		gain -= (GET_POISON(ch) * (IS_NPC(ch) ? 4 : 5));
	return (gain);
}



/* move gain pr. game hour */
int move_gain(CHAR_DATA * ch)
{
	int gain = 0, restore = con_app[GET_REAL_CON(ch)].hitp, percent = 100;

	if (IS_NPC(ch))
		gain = GET_LEVEL(ch);
	else {
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);
		gain =
		    graf(age(ch)->year, 15 + restore, 20 + restore, 25 + restore,
			 20 + restore, 16 + restore, 12 + restore, 8 + restore);
		/* Room specification    */
		if (LIKE_ROOM(ch))
			percent += 25;
		/* Weather specification */
		if (average_day_temp() < -20)
			percent -= 10;
		else if (average_day_temp() < -10)
			percent -= 5;
	}

	if (world[IN_ROOM(ch)]->fires)
		percent += MAX(50, 10 + world[IN_ROOM(ch)]->fires * 5);

	/* Class/Level calculations */

	/* Skill/Spell calculations */


	/* Position calculations    */
	switch (GET_POS(ch)) {
	case POS_SLEEPING:
		percent += 25;
		break;
	case POS_RESTING:
		percent += 15;
		break;
	case POS_SITTING:
		percent += 10;
		break;
	}

	if (!IS_NPC(ch)) {
		if (GET_COND(ch, FULL) == 0)
			percent -= 50;
		if (GET_COND(ch, THIRST) == 0)
			percent -= 25;
		if (!IS_IMMORTAL(ch) && affected_by_spell(ch, SPELL_HIDE))
			percent -= 20;
		if (!IS_IMMORTAL(ch) && affected_by_spell(ch, SPELL_CAMOUFLAGE))
			percent -= 30;
	}

	percent += GET_MOVEREG(ch);
	if (AFF_FLAGGED(ch, AFF_POISON) && percent > 0)
		percent /= 4;
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	return (gain);
}

#define MINUTE            60
#define UPDATE_PC_ON_BEAT TRUE

int interpolate(int min_value, int pulse)
{
	int sign = 1, int_p, frac_p, i, carry, x, y;

	if (min_value < 0) {
		sign = -1;
		min_value = -min_value;
	}
	int_p = min_value / MINUTE;
	frac_p = min_value % MINUTE;
	if (!frac_p)
		return (sign * int_p);
	pulse = time(NULL) % MINUTE + 1;
	x = MINUTE;
	y = 0;
	for (i = 0, carry = 0; i <= pulse; i++) {
		y += frac_p;
		if (y >= x) {
			x += MINUTE;
			carry = 1;
		} else
			carry = 0;
	}
	return (sign * (int_p + carry));
}
void beat_punish(CHAR_DATA * i)
{
	int restore;
	// Проверяем на выпуск чара из кутузки
	if (PLR_FLAGGED(i, PLR_HELLED) && HELL_DURATION(i) && HELL_DURATION(i) <= time(NULL)) {
		restore = PLR_TOG_CHK(i, PLR_HELLED);
		if (HELL_REASON(i))
			free(HELL_REASON(i));
		HELL_REASON(i) = 0;
		GET_HELL_LEV(i) = 0;
		HELL_GODID(i) = 0;
		HELL_DURATION(i) = 0;
		send_to_char("Вас выпустили из темницы.\r\n", i);
		if ((restore = GET_LOADROOM(i)) == NOWHERE)
		restore = calc_loadroom(i);
		restore = real_room(restore);
		if (restore == NOWHERE) {
		if (GET_LEVEL(i) >= LVL_IMMORT)
			restore = r_immort_start_room;
		else
			restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i, restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
		    FALSE, i, 0, 0, TO_ROOM);
	}
	if (PLR_FLAGGED(i, PLR_NAMED) && NAME_DURATION(i) && NAME_DURATION(i) <= time(NULL)) {
		restore = PLR_TOG_CHK(i, PLR_NAMED);
		if (NAME_REASON(i))
		free(NAME_REASON(i));
		NAME_REASON(i) = 0;
		GET_NAME_LEV(i) = 0;
		NAME_GODID(i) = 0;
		NAME_DURATION(i) = 0;
		send_to_char("Вас выпустили из КОМНАТЫ ИМЕНИ.\r\n", i);

		if ((restore = GET_LOADROOM(i)) == NOWHERE)
			restore = calc_loadroom(i);
		restore = real_room(restore);
		if (restore == NOWHERE) {
			if (GET_LEVEL(i) >= LVL_IMMORT)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i, restore);
		act("С ревом \"Имья, сестра, имья...\", $n появил$u в центре комнаты.",
		    FALSE, i, 0, 0, TO_ROOM);
	}
	if (PLR_FLAGGED(i, PLR_MUTE) && MUTE_DURATION(i) != 0 && MUTE_DURATION(i) <= time(NULL)) {
		restore = PLR_TOG_CHK(i, PLR_MUTE);
		if (MUTE_REASON(i))
		free(MUTE_REASON(i));
		MUTE_REASON(i) = 0;
		GET_MUTE_LEV(i) = 0;
		MUTE_GODID(i) = 0;
		MUTE_DURATION(i) = 0;
			send_to_char("Вы можете орать.\r\n", i);
	}
	if (PLR_FLAGGED(i, PLR_DUMB) && DUMB_DURATION(i) != 0 && DUMB_DURATION(i) <= time(NULL)) {
		restore = PLR_TOG_CHK(i, PLR_DUMB);
		if (DUMB_REASON(i))
			free(DUMB_REASON(i));
		DUMB_REASON(i) = 0;
		GET_DUMB_LEV(i) = 0;
		DUMB_GODID(i) = 0;
		DUMB_DURATION(i) = 0;
		send_to_char("Вы можете говорить.\r\n", i);
	}

	if (!PLR_FLAGGED(i, PLR_REGISTERED) && UNREG_DURATION(i) != 0 && UNREG_DURATION(i) <= time(NULL)) {
		restore = PLR_TOG_CHK(i, PLR_REGISTERED);
		if (UNREG_REASON(i))
			free(UNREG_REASON(i));
		UNREG_REASON(i) = 0;
		GET_UNREG_LEV(i) = 0;
		UNREG_GODID(i) = 0;
		UNREG_DURATION(i) = 0;
		send_to_char("Ваша регистрация восстановлена.\r\n", i);

		if (IN_ROOM(i) == r_unreg_start_room)
		{
			if ((restore = GET_LOADROOM(i)) == NOWHERE)
				restore = calc_loadroom(i);

			restore = real_room(restore);

			if (restore == NOWHERE) {
				if (GET_LEVEL(i) >= LVL_IMMORT)
					restore = r_immort_start_room;
				else
					restore = r_mortal_start_room;
			}

			char_from_room(i);
			char_to_room(i, restore);
			look_at_room(i, restore);

			act("$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации !",
			    FALSE, i, 0, 0, TO_ROOM);
		};

	}

	if (GET_GOD_FLAG(i, GF_GODSLIKE) && GCURSE_DURATION(i) != 0 && GCURSE_DURATION(i) <= time(NULL)) {
		CLR_GOD_FLAG(i, GF_GODSLIKE);
		send_to_char("Вы более не под защитой Богов.\r\n", i);
	}
	if (GET_GOD_FLAG(i, GF_GODSCURSE) && GCURSE_DURATION(i) != 0 && GCURSE_DURATION(i) <= time(NULL)) {
		CLR_GOD_FLAG(i, GF_GODSCURSE);
		send_to_char("Боги более не в обиде на Вас.\r\n", i);
	}
	if (PLR_FLAGGED(i, PLR_FROZEN) && FREEZE_DURATION(i) != 0 && FREEZE_DURATION(i) <= time(NULL)) {
		restore = PLR_TOG_CHK(i, PLR_FROZEN);
		if (FREEZE_REASON(i))
			free(FREEZE_REASON(i));
		FREEZE_REASON(i) = 0;
		GET_FREEZE_LEV(i) = 0;
		FREEZE_GODID(i) = 0;
		FREEZE_DURATION(i) = 0;
		send_to_char("Вы оттаяли.\r\n", i);
		Glory::remove_freeze(GET_UNIQUE(i));
	}
	// Проверяем а там ли мы где должны быть по флагам.
	if (IN_ROOM(i) == STRANGE_ROOM)
		restore = GET_WAS_IN(i);
	else
		restore = IN_ROOM(i);

	if (PLR_FLAGGED(i, PLR_HELLED))
	{
		if (restore != r_helled_start_room)
		{
			if (IN_ROOM(i) == STRANGE_ROOM)
				GET_WAS_IN(i) = r_helled_start_room;
			else
			{
				send_to_char("Чья-то злая воля вернула Вас в темницу.\r\n", i);
				act("$n возвращен$a в темницу.",
				    FALSE, i, 0, 0, TO_ROOM);

        			char_from_room(i);
				char_to_room(i, r_helled_start_room);
				look_at_room(i, r_helled_start_room);
				GET_WAS_IN(i) = NOWHERE;
			};
		}
	} else if (PLR_FLAGGED(i, PLR_NAMED))
	{
		if (restore != r_named_start_room)
		{
			if (IN_ROOM(i) == STRANGE_ROOM)
				GET_WAS_IN(i) = r_named_start_room;
			else
			{
				send_to_char("Чья-то злая воля вернула Вас в комнату имени.\r\n", i);
				act("$n возвращен$a в комнату имени.",
				    FALSE, i, 0, 0, TO_ROOM);
				char_from_room(i);
				char_to_room(i, r_named_start_room);
				look_at_room(i, r_named_start_room);
				GET_WAS_IN(i) = NOWHERE;
			};
		};
	}
	else if (!RegisterSystem::is_registered(i) && i->desc && STATE(i->desc) == CON_PLAYING)
	{
		if (restore != r_unreg_start_room
			&& !RENTABLE(i)
			&& !DeathTrap::is_slow_dt(IN_ROOM(i))
			&& !check_dupes_host(i->desc, 1))
		{
			if (IN_ROOM(i) == STRANGE_ROOM)
				GET_WAS_IN(i) = r_unreg_start_room;
			else
			{
				act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.\r\n",
					FALSE, i, 0, 0, TO_ROOM);
				char_from_room(i);
				char_to_room(i, r_unreg_start_room);
				look_at_room(i, r_unreg_start_room);
				GET_WAS_IN(i) = NOWHERE;
			};
		}
		else if (restore == r_unreg_start_room && check_dupes_host(i->desc, 1) && !IS_IMMORTAL(i))
		{
			send_to_char("Неведомая вытолкнула вас из комнаты для незарегистрированных игроков.\r\n", i);
			act("$n появил$u в центре комнаты, правда без штампика регистрации...\r\n",
				FALSE, i, 0, 0, TO_ROOM);
			restore = GET_WAS_IN(i);
			if (restore == NOWHERE || restore == r_unreg_start_room)
			{
				restore = GET_LOADROOM(i);
				if (restore == NOWHERE)
					restore = calc_loadroom(i);
				restore = real_room(restore);
			}
			char_from_room(i);
			char_to_room(i, restore);
			look_at_room(i, restore);
			GET_WAS_IN(i) = NOWHERE;
		}
	}
}

void beat_points_update(int pulse)
{
	CHAR_DATA *i, *next_char;
	int restore;

	if (!UPDATE_PC_ON_BEAT)
		return;

	/* only for PC's */
	for (i = character_list; i; i = next_char) {
		next_char = i->next;
		if (IS_NPC(i))
			continue;

		if (IN_ROOM(i) == NOWHERE)
		{
			log("SYSERR: Pulse character in NOWHERE.");
			continue;
		}

		if (RENTABLE(i) < time(NULL)) {
			RENTABLE(i) = 0;
			AGRESSOR(i) = 0;
			AGRO(i) = 0;
		}
		if (AGRO(i) < time(NULL))
			AGRO(i) = 0;

		beat_punish(i);

// This line is used only to control all situations when someone is
// dead (POS_DEAD). You can comment it to minimize heartbeat function
// working time, if you're sure, that you control these situations
// everywhere. To the time of this code revision I've fix some of them
// and haven't seen any other.
//             if (GET_POS(i) == POS_DEAD)
//                     die(i, NULL);

		if (GET_POS(i) < POS_STUNNED)
			continue;

		// Restore hitpoints
		restore = hit_gain(i);
		restore = interpolate(restore, pulse);
		if (restore < 0) {
			if (damage(i, i, -restore, SPELL_POISON, FALSE) < 0)
				continue;
		} else if (GET_HIT(i) < GET_REAL_MAX_HIT(i))
			GET_HIT(i) = MIN(GET_HIT(i) + restore, GET_REAL_MAX_HIT(i));

		// Проверка аффекта !исступление!. Поместил именно здесь,
		// но если кто найдет более подходящее место переносите =)
		//Gorrah: перенес в handler::affect_total
		//check_berserk(i);

		// Restore PC caster mem
		if (!IS_MANA_CASTER(i) && !MEMQUEUE_EMPTY(i)) {
			restore = mana_gain(i);
			restore = interpolate(restore, pulse);
			GET_MEM_COMPLETED(i) += restore;

			while (GET_MEM_COMPLETED(i) > GET_MEM_CURRENT(i)
			       && !MEMQUEUE_EMPTY(i)) {
				int spellnum;
				spellnum = MemQ_learn(i);
				GET_SPELL_MEM(i, spellnum)++;
				GET_CASTER(i) += spell_info[spellnum].danger;
			}

			if (MEMQUEUE_EMPTY(i)) {
				if (GET_RELIGION(i) == RELIGION_MONO) {
					send_to_char
					    ("Наконец Ваши занятия окончены. Вы с улыбкой захлопнули свой часослов.\r\n",
					     i);
					act("Окончив занятия, $n с улыбкой захлопнул$g часослов.",
					    FALSE, i, 0, 0, TO_ROOM);
				} else {
					send_to_char
					    ("Наконец Ваши занятия окончены. Вы с улыбкой убрали свои резы.\r\n", i);
					act("Окончив занятия, $n с улыбкой убрал$g резы.", FALSE, i, 0, 0, TO_ROOM);
				}
			}
		}

		if (!IS_MANA_CASTER(i) && MEMQUEUE_EMPTY(i)) {
			GET_MEM_COMPLETED(i) = 0;
		}

		// Гейн маны у волхвов
		if (IS_MANA_CASTER(i) && GET_MANA_STORED(i) < GET_MAX_MANA(i)) {
			GET_MANA_STORED(i) += mana_gain(i);
			if (GET_MANA_STORED(i) >= GET_MAX_MANA(i)) {
				GET_MANA_STORED(i) = GET_MAX_MANA(i);
				send_to_char("Ваша магическая энергия полностью восстановилась\r\n", i);
			}
		}
		if (IS_MANA_CASTER(i) && GET_MANA_STORED(i) > GET_MAX_MANA(i)) {
			GET_MANA_STORED(i) = GET_MAX_MANA(i);
		}
		// Restore moves
		restore = move_gain(i);
		restore = interpolate(restore, pulse);
//		GET_MOVE(i) = MIN(GET_MOVE(i) + restore, GET_REAL_MAX_MOVE(i));
//MZ.overflow_fix
               if (GET_MOVE(i) < GET_REAL_MAX_MOVE(i))
                       GET_MOVE(i) = MIN(GET_MOVE(i) + restore, GET_REAL_MAX_MOVE(i));
//-MZ.overflow_fix
	}
}

void gain_exp(CHAR_DATA * ch, int gain, int clan_exp)
{
	int is_altered = FALSE;
	int num_levels = 0;
	char buf[128];

	if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
		return;

	if (IS_NPC(ch)) {
		GET_EXP(ch) += gain;
		return;
	}
	if (gain > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
		gain = MIN(max_exp_gain_pc(ch), gain);	/* put a cap on the max gain per kill */
		GET_EXP(ch) += gain;
		if (GET_EXP(ch) >= level_exp(ch, LVL_IMMORT)) {
			if (!GET_GOD_FLAG(ch, GF_REMORT) && GET_REMORT(ch) < MAX_REMORT) {
				sprintf(buf,
					"%sПоздравляем, Вы получили право на перевоплощение !%s\r\n",
					CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				SET_GOD_FLAG(ch, GF_REMORT);
			}
		}
		GET_EXP(ch) = MIN(GET_EXP(ch), level_exp(ch, LVL_IMMORT) - 1);
		while (GET_LEVEL(ch) < LVL_IMMORT && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
			GET_LEVEL(ch) += 1;
			num_levels++;
			sprintf(buf, "%sВы достигли следующего уровня !%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
			advance_level(ch);
			is_altered = TRUE;
		}

		if (is_altered) {
			sprintf(buf, "%s advanced %d level%s to level %d.",
				GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
			mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
		}
	} else if (gain < 0 && GET_LEVEL(ch) < LVL_IMMORT) {
		gain = MAX(-max_exp_loss_pc(ch), gain);	/* Cap max exp lost per death */
		GET_EXP(ch) += gain;
		if (CLAN(ch))
			CLAN(ch)->SetClanExp(ch, gain); // клану
		if (GET_EXP(ch) < 0)
			GET_EXP(ch) = 0;
		while (GET_LEVEL(ch) > 1 && GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch))) {
			GET_LEVEL(ch) -= 1;
			num_levels++;
			sprintf(buf,
				"%sВы потеряли уровень. Вам должно быть стыдно !%s\r\n",
				CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
			decrease_level(ch);
			is_altered = TRUE;
		}
		if (is_altered) {
			sprintf(buf, "%s decreases %d level%s to level %d.",
				GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
			mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
		}
	}
	if ((GET_EXP(ch) < level_exp(ch, LVL_IMMORT) - 1) &&
	    GET_GOD_FLAG(ch, GF_REMORT) && gain && (GET_LEVEL(ch) < LVL_IMMORT)) {
		sprintf(buf, "%sВы потеряли право на перевоплощение !%s\r\n", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		CLR_GOD_FLAG(ch, GF_REMORT);
	}

	TopPlayer::Refresh(ch);
	if (CLAN(ch))
		CLAN(ch)->AddTopExp(ch, gain + clan_exp); // для рейтинга кланов
}

// юзается исключительно в act.wizards.cpp в имм командах "advance" и "set exp".
void gain_exp_regardless(CHAR_DATA * ch, int gain)
{
	int is_altered = FALSE;
	int num_levels = 0;

	GET_EXP(ch) += gain;
	if (GET_EXP(ch) < 0)
		GET_EXP(ch) = 0;
	if (!IS_NPC(ch)) {
		if (gain > 0) {
			while (GET_LEVEL(ch) < LVL_IMPL && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
				GET_LEVEL(ch) += 1;
				num_levels++;
				sprintf(buf, "%sВы достигли следующего уровня !%s\r\n",
					CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);

				advance_level(ch);
				is_altered = TRUE;
			}

			if (is_altered) {
				sprintf(buf, "%s advanced %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
				mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
			}
		} else if (gain < 0) {
// Pereplut: глупый участок кода.
//			gain = MAX(-max_exp_loss_pc(ch), gain);	/* Cap max exp lost per death */
//			GET_EXP(ch) += gain;
//			if (GET_EXP(ch) < 0)
//				GET_EXP(ch) = 0;
			while (GET_LEVEL(ch) > 1 && GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch))) {
				GET_LEVEL(ch) -= 1;
				num_levels++;
				sprintf(buf,
					"%sВы потеряли уровень!%s\r\n",
					CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				decrease_level(ch);
				is_altered = TRUE;
			}
			if (is_altered) {
				sprintf(buf, "%s decreases %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
				mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
			}
		}

	}

	TopPlayer::Refresh(ch);
}


void gain_condition(CHAR_DATA * ch, int condition, int value)
{
	bool intoxicated;

	if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
		return;

	intoxicated = (GET_COND(ch, DRUNK) >= CHAR_DRUNKED);

	GET_COND(ch, condition) += value;

	GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
	GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

	if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
		return;

	switch (condition) {
	case FULL:
		send_to_char("Вы голодны.\r\n", ch);
		return;
	case THIRST:
		send_to_char("Вас мучает жажда.\r\n", ch);
		return;
	case DRUNK:
		if (AFF_FLAGGED(ch, AFF_ABSTINENT) || AFF_FLAGGED(ch, AFF_DRUNKED))
			GET_COND(ch, DRUNK) = MAX(CHAR_DRUNKED, GET_COND(ch, DRUNK));
		if (intoxicated && GET_COND(ch, DRUNK) < CHAR_DRUNKED)
			send_to_char("Наконец то Вы протрезвели.\r\n", ch);
		GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch), GET_COND(ch, DRUNK));
		return;
	default:
		break;
	}

}

void underwater_check(void)
{
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character && SECT(d->character->in_room) == SECT_UNDERWATER && !IS_GOD(d->character) && !AFF_FLAGGED(d->character, AFF_WATERBREATH))
		{
			sprintf(buf,
			"Player %s died under water (room %d)", GET_NAME(d->character), GET_ROOM_VNUM(d->character->in_room));
			if (damage(d->character, d->character, MAX(1, GET_REAL_MAX_HIT(d->character) >> 2), TYPE_WATERDEATH, FALSE) < 0)
			{
				log(buf);
			}
		}
	}

}

void check_idling(CHAR_DATA * ch)
{
	if (!RENTABLE(ch)) {
		if (++(ch->char_specials.timer) > idle_void) {
			if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
				GET_WAS_IN(ch) = ch->in_room;
				if (FIGHTING(ch)) {
					stop_fighting(FIGHTING(ch), FALSE);
					stop_fighting(ch, TRUE);
				}
				act("$n растворил$u в пустоте.", TRUE, ch, 0, 0, TO_ROOM);
				send_to_char("Вы пропали в пустоте этого мира.\r\n", ch);
				save_char(ch, NOWHERE);
				Crash_crashsave(ch);
				char_from_room(ch);
				char_to_room(ch, STRANGE_ROOM);
			} else if (ch->char_specials.timer > idle_rent_time) {
				if (ch->in_room != NOWHERE)
					char_from_room(ch);
				char_to_room(ch, STRANGE_ROOM);
				if (ch->desc) {
					STATE(ch->desc) = CON_DISCONNECT;
					/*
					 * For the 'if (d->character)' test in close_socket().
					 * -gg 3/1/98 (Happy anniversary.)
					 */
					ch->desc->character = NULL;
					ch->desc = NULL;
				}
				if (free_rent)
					Crash_rentsave(ch, 0);
				else
					Crash_idlesave(ch);
				Depot::exit_char(ch);
				sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
				mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
				extract_char(ch, FALSE);
			}
		}
	}
}



/* Update PCs, NPCs, and objects */
#define NO_DESTROY(obj) ((obj)->carried_by   || \
                         (obj)->worn_by      || \
                         (obj)->in_obj       || \
                         (obj)->script       || \
                         GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN || \
	                 obj->in_room == NOWHERE || \
			 (OBJ_FLAGGED(obj, ITEM_NODECAY) && !ROOM_FLAGGED(obj->in_room, ROOM_DEATH)))
#define NO_TIMER(obj)   (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)
			/* || OBJ_FLAGGED(obj, ITEM_NODECAY)) */

int up_obj_where(OBJ_DATA * obj)
{
	if (obj->in_obj)
		return up_obj_where(obj->in_obj);
	else
		return OBJ_WHERE(obj);
}


void hour_update(void)
{
	DESCRIPTOR_DATA *i;

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING || i->character == NULL || PLR_FLAGGED(i->character, PLR_WRITING))
			continue;
		sprintf(buf, "%sМинул час.%s\r\n", CCIRED(i->character, C_NRM), CCNRM(i->character, C_NRM));
		SEND_TO_Q(buf, i);
	}
}


void point_update(void)
{
	memory_rec *mem, *nmem, *pmem;
	CHAR_DATA *i, *next_char;
	OBJ_DATA *j, *next_thing, *jj, *next_thing2;
	struct track_data *track, *next_track, *temp;
	int count, mob_num, spellnum, mana, restore, cont = 0;
	char buffer_mem[MAX_SPELLS + 1], real_spell[MAX_SPELLS + 1];

	for (count = 0; count <= MAX_SPELLS; count++) {
		buffer_mem[count] = count;
		real_spell[count] = 0;
	}
	for (spellnum = MAX_SPELLS; spellnum > 0; spellnum--) {
		count = number(1, spellnum);
		real_spell[MAX_SPELLS - spellnum] = buffer_mem[count];
		for (; count < MAX_SPELLS; buffer_mem[count] = buffer_mem[count + 1], count++);
	}

	/* characters */
	for (i = character_list; i; i = next_char) {
		next_char = i->next;
		/* Если чар или моб попытался проснуться а на нем аффект сон,
		   то он снова должен валиться в сон */
		if (AFF_FLAGGED(i, AFF_SLEEP) && GET_POS(i) > POS_SLEEPING) {
			GET_POS(i) = POS_SLEEPING;
			send_to_char("Вы попытались очнуться, но снова заснули и упали наземь.\r\n", i);
			act("$n попытал$u очнуться, но снова заснул$a и упал$a наземь.", TRUE, i, 0, 0, TO_ROOM);
		}
		if (!IS_NPC(i)) {
			if (average_day_temp() < -20)
				gain_condition(i, FULL, -2);
			else if (average_day_temp() < -5)
				gain_condition(i, FULL, number(-2, -1));
			else
				gain_condition(i, FULL, -1);

			gain_condition(i, DRUNK, -1);

			if (average_day_temp() > 25)
				gain_condition(i, THIRST, -2);
			else if (average_day_temp() > 20)
				gain_condition(i, THIRST, number(-2, -1));
			else
				gain_condition(i, THIRST, -1);

		}
		if (GET_POS(i) >= POS_STUNNED) {	/* Restore hitpoints */
			if (IS_NPC(i) || !UPDATE_PC_ON_BEAT) {
				count = hit_gain(i);
				if (count < 0) {
					if (damage(i, i, -count, SPELL_POISON, FALSE) < 0)
						continue;
				} else if (GET_HIT(i) < GET_REAL_MAX_HIT(i))
					GET_HIT(i) = MIN(GET_HIT(i) + count, GET_REAL_MAX_HIT(i));
			}
			// Restore mobs
			if (IS_NPC(i) && !FIGHTING(i)) {	// Restore horse
				if (IS_HORSE(i)) {
					switch (real_sector(IN_ROOM(i))) {
					case SECT_FLYING:
					case SECT_UNDERWATER:
					case SECT_SECRET:
					case SECT_WATER_SWIM:
					case SECT_WATER_NOSWIM:
					case SECT_THICK_ICE:
					case SECT_NORMAL_ICE:
					case SECT_THIN_ICE:
						mana = 0;
						break;
					case SECT_CITY:
						mana = 20;
						break;
					case SECT_FIELD:
					case SECT_FIELD_RAIN:
						mana = 100;
						break;
					case SECT_FIELD_SNOW:
						mana = 40;
						break;
					case SECT_FOREST:
					case SECT_FOREST_RAIN:
						mana = 80;
						break;
					case SECT_FOREST_SNOW:
						mana = 30;
						break;
					case SECT_HILLS:
					case SECT_HILLS_RAIN:
						mana = 70;
						break;
					case SECT_HILLS_SNOW:
						mana = 25;
						break;
					case SECT_MOUNTAIN:
						mana = 25;
						break;
					case SECT_MOUNTAIN_SNOW:
						mana = 10;
						break;
					default:
						mana = 10;
					}
					if (on_horse(i->master))
						mana /= 2;
					GET_HORSESTATE(i) = MIN(200, GET_HORSESTATE(i) + mana);
				}
				// Forget PC's
				for (mem = MEMORY(i), pmem = NULL; mem; mem = nmem) {
					nmem = mem->next;
					if (mem->time <= 0 && FIGHTING(i)) {
						pmem = mem;
						continue;
					}
					if (mem->time < time(NULL) || mem->time <= 0) {
						if (pmem)
							pmem->next = nmem;
						else
							MEMORY(i) = nmem;
						free(mem);
					} else
						pmem = mem;
				}
				// Remember some spells
				if ((mob_num = GET_MOB_RNUM(i)) >= 0)
					for (count = 0, mana = 0;
					     count < MAX_SPELLS && mana < GET_REAL_INT(i) * 10; count++) {
						spellnum = real_spell[count];
						if (GET_SPELL_MEM(mob_proto + mob_num, spellnum) >
						    GET_SPELL_MEM(i, spellnum)) {
							GET_SPELL_MEM(i, spellnum)++;
							mana +=
							    ((spell_info[spellnum].mana_max +
							      spell_info[spellnum].mana_min) / 2);
							GET_CASTER(i) +=
							    (IS_SET
							     (spell_info[spellnum].routines, NPC_CALCULATE) ? 1 : 0);
							// sprintf(buf,"Remember spell %s for mob %s.\r\n",spell_info[spellnum].name,GET_NAME(i));
							// send_to_gods(buf);
						}
					}
			}
			// Restore moves
			if (IS_NPC(i) || !UPDATE_PC_ON_BEAT)
//				GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_REAL_MAX_MOVE(i));
//MZ.overflow_fix
				if (GET_MOVE(i) < GET_REAL_MAX_MOVE(i))
					GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_REAL_MAX_MOVE(i));
//-MZ.overflow_fix

	  /** Update poisoned - REMOVE TO UP !!!
          if ((IS_NPC(i) || !UPDATE_PC_ON_BEAT) && AFF_FLAGGED(i, AFF_POISON))
             if (damage(i, i, GET_POISON(i) * 5, SPELL_POISON,FALSE) == -1)
                continue;
	   **/

			// Update PC/NPC position
			if (GET_POS(i) <= POS_STUNNED)
				update_pos(i);
		} else if (GET_POS(i) == POS_INCAP) {
			if (damage(i, i, 1, TYPE_SUFFERING, FALSE) == -1)
				continue;
		} else if (GET_POS(i) == POS_MORTALLYW) {
			if (damage(i, i, 2, TYPE_SUFFERING, FALSE) == -1)
				continue;
		}
		update_char_objects(i);
		if (!IS_NPC(i) && GET_LEVEL(i) < idle_max_level && !PRF_FLAGGED(i, PRF_CODERINFO))
			check_idling(i);
	}

	/* rooms */
	for (count = FIRST_ROOM; count <= top_of_world; count++) {
		if (world[count]->fires) {
			switch (get_room_sky(count)) {
			case SKY_CLOUDY:
			case SKY_CLOUDLESS:
				mana = number(1, 2);
				break;
			case SKY_RAINING:
				mana = 2;
				break;
			default:
				mana = 1;
			}
			world[count]->fires -= MIN(mana, world[count]->fires);
			if (world[count]->fires <= 0) {
				act("Костер затух.", FALSE, world[count]->people, 0, 0, TO_ROOM);
				act("Костер затух.", FALSE, world[count]->people, 0, 0, TO_CHAR);
				world[count]->fires = 0;
			}
		}
		if (world[count]->forbidden) {
			world[count]->forbidden--;
			if (!world[count]->forbidden) {
				act("Магия, запечатывающая входы, пропала.", FALSE,
				    world[count]->people, 0, 0, TO_ROOM);
				act("Магия, запечатывающая входы, пропала.", FALSE,
				    world[count]->people, 0, 0, TO_CHAR);
			}
		}
		if (world[count]->portal_time) {
			world[count]->portal_time--;
			if (!world[count]->portal_time) {
				OneWayPortal::remove(world[count]);
				act("Пентаграмма медленно растаяла.", FALSE, world[count]->people, 0, 0, TO_ROOM);
				act("Пентаграмма медленно растаяла.", FALSE, world[count]->people, 0, 0, TO_CHAR);
			}
		}
		if (world[count]->holes) {
			world[count]->holes--;
			if (!world[count]->holes || roundup(world[count]->holes) == world[count]->holes) {
				act("Ямку присыпало землей...", FALSE, world[count]->people, 0, 0, TO_ROOM);
				act("Ямку присыпало землей...", FALSE, world[count]->people, 0, 0, TO_CHAR);
			}
		}
		if (world[count]->ices)
			if (!--world[count]->ices) {
				REMOVE_BIT(ROOM_FLAGS(count, ROOM_ICEDEATH), ROOM_ICEDEATH);
				DeathTrap::remove(world[count]);
			}

		world[count]->glight = MAX(0, world[count]->glight);
		world[count]->gdark = MAX(0, world[count]->gdark);

		for (track = world[count]->track, temp = NULL; track; track = next_track) {
			next_track = track->next;
			switch (real_sector(count)) {
			case SECT_FLYING:
			case SECT_UNDERWATER:
			case SECT_SECRET:
			case SECT_WATER_SWIM:
			case SECT_WATER_NOSWIM:
				spellnum = 31;
				break;
			case SECT_THICK_ICE:
			case SECT_NORMAL_ICE:
			case SECT_THIN_ICE:
				spellnum = 16;
				break;
			case SECT_CITY:
				spellnum = 4;
				break;
			case SECT_FIELD:
			case SECT_FIELD_RAIN:
				spellnum = 2;
				break;
			case SECT_FIELD_SNOW:
				spellnum = 1;
				break;
			case SECT_FOREST:
			case SECT_FOREST_RAIN:
				spellnum = 2;
				break;
			case SECT_FOREST_SNOW:
				spellnum = 1;
				break;
			case SECT_HILLS:
			case SECT_HILLS_RAIN:
				spellnum = 3;
				break;
			case SECT_HILLS_SNOW:
				spellnum = 1;
				break;
			case SECT_MOUNTAIN:
				spellnum = 4;
				break;
			case SECT_MOUNTAIN_SNOW:
				spellnum = 1;
				break;
			default:
				spellnum = 2;
			}

			for (mana = 0, restore = FALSE; mana < NUM_OF_DIRS; mana++) {
				if ((track->time_income[mana] <<= spellnum))
					restore = TRUE;
				if ((track->time_outgone[mana] <<= spellnum))
					restore = TRUE;
			}
			if (!restore) {
				if (temp)
					temp->next = next_track;
				else
					world[count]->track = next_track;
				free(track);
			} else
				temp = track;
		}
	}
//F@N++
//Exchange countdown timer
	EXCHANGE_ITEM_DATA *exch_item, *next_exch_item;
	for (exch_item = exchange_item_list; exch_item; exch_item = next_exch_item) {
		next_exch_item = exch_item->next;
		if (GET_OBJ_TIMER(GET_EXCHANGE_ITEM(exch_item)) > 0)
			GET_OBJ_TIMER(GET_EXCHANGE_ITEM(exch_item))--;

		if (GET_OBJ_TIMER(GET_EXCHANGE_ITEM(exch_item)) <= 0) {
			sprintf(buf, "Exchange: - %s рассыпал%s от длительного использования.\r\n",
				CAP(GET_EXCHANGE_ITEM(exch_item)->PNames[0]),
				GET_OBJ_SUF_2(GET_EXCHANGE_ITEM(exch_item)));
			log(buf);
			extract_exchange_item(exch_item);
		}
	}
//F@N--


	/* objects */
	int chest = real_object(CLAN_CHEST);

	for (j = object_list; j; j = next_thing) {
		next_thing = j->next;	/* Next in object list */
		// смотрим клан-сундуки
		if (chest >= 0 && j->in_obj && j->in_obj->item_number == chest) {
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;

			if (j && ((OBJ_FLAGGED(j, ITEM_ZONEDECAY) && GET_OBJ_ZONE(j) != NOWHERE
			&& up_obj_where(j->in_obj) != NOWHERE
			&& GET_OBJ_ZONE(j) != world[up_obj_where(j->in_obj)]->zone)
			|| GET_OBJ_TIMER(j) <= 0)) {
				int room = GET_ROOM_VNUM(j->in_obj->in_room);
				DESCRIPTOR_DATA *d;

				for (d = descriptor_list; d; d = d->next)
					if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& CLAN(d->character) && world[real_room(CLAN(d->character)->GetRent())]->zone == world[real_room(room)]->zone && PRF_FLAGGED(d->character, PRF_DECAY_MODE))
						send_to_char(d->character, "[Хранилище]: %s'%s рассыпал%s в прах'%s\r\n",
							CCIRED(d->character, C_NRM), j->short_description, GET_OBJ_SUF_2(j), CCNRM(d->character, C_NRM));
				obj_from_obj(j);
				extract_obj(j);
			}
			continue;
		}

		// контейнеры на земле с флагом !дикей, но не загружаемые в этой комнате, а хз кем брошенные
		// извращение конечно перебирать на каждый объект команды резета зоны, но в голову ниче интересного
		// не лезет, да и не так уж и многона самом деле таких предметов будет, условий порядочно
		// а так привет любителям оставлять книги в клановых сумках или лоадить в замке столы
		if (j->in_obj
			&& !j->in_obj->carried_by
			&& !j->in_obj->worn_by
			&& OBJ_FLAGGED(j->in_obj, ITEM_NODECAY)
			&& GET_ROOM_VNUM(IN_ROOM(j->in_obj)) % 100 != 99)
		{
			int zone = world[j->in_obj->in_room]->zone;
			bool find = 0;
			ClanListType::const_iterator clan = Clan::IsClanRoom(j->in_obj->in_room);
			if (clan == Clan::ClanList.end()) { // внутри замков даже и смотреть не будем
				for (int cmd_no = 0; zone_table[zone].cmd[cmd_no].command != 'S'; ++cmd_no) {
					if (zone_table[zone].cmd[cmd_no].command == 'O'
						&& zone_table[zone].cmd[cmd_no].arg1 == GET_OBJ_RNUM(j->in_obj)
						&& zone_table[zone].cmd[cmd_no].arg3 == IN_ROOM(j->in_obj))
					{
						find = 1;
						break;
					}
				}
			}

			if (!find && GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;
		}

		/* If this is a corpse */
		if (IS_CORPSE(j)) {	/* timer count down */
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;
			if (GET_OBJ_TIMER(j) <= 0) {
				for (jj = j->contains; jj; jj = next_thing2) {
					next_thing2 = jj->next_content;	/* Next in inventory */
					obj_from_obj(jj);
					if (j->in_obj)
						obj_to_obj(jj, j->in_obj);
					else if (j->carried_by)
						obj_to_char(jj, j->carried_by);
					else if (j->in_room != NOWHERE)
						obj_to_room(jj, j->in_room);
					else {
						log("SYSERR: extract %s from %s to NOTHING !!!",
						    jj->PNames[0], j->PNames[0]);
						// core_dump();
						extract_obj(jj);
					}
				}
				// Добавлено Ладником
//              next_thing = j->next; /* пурж по obj_to_room я убрал, но пускай на всякий случай */
				// Конец Ладник
				if (j->carried_by) {
					act("$p рассыпал$U в Ваших руках.", FALSE, j->carried_by, j, 0, TO_CHAR);
					obj_from_char(j);
				} else if (j->in_room != NOWHERE) {
					if (world[j->in_room]->people) {
						act("Черви полностью сожрали $o3.",
						    TRUE, world[j->in_room]->people, j, 0, TO_ROOM);
						act("Черви не оставили от $o1 и следа.",
						    TRUE, world[j->in_room]->people, j, 0, TO_CHAR);
					}
					obj_from_room(j);
				} else if (j->in_obj)
					obj_from_obj(j);
				extract_obj(j);
			}
		}
		/* If the timer is set, count it down and at 0, try the trigger */
		/* note to .rej hand-patchers: make this last in your point-update() */
		else {
			if (SCRIPT_CHECK(j, OTRIG_TIMER)) {
				if (GET_OBJ_TIMER(j) > 0 && OBJ_FLAGGED(j, ITEM_TICKTIMER))
					GET_OBJ_TIMER(j)--;
				if (!GET_OBJ_TIMER(j)) {
					timer_otrigger(j);
					j = NULL;
				}
			} else if (GET_OBJ_DESTROY(j) > 0 && !NO_DESTROY(j))
				GET_OBJ_DESTROY(j)--;

			if (j && (j->in_room != NOWHERE) && GET_OBJ_TIMER(j) > 0 && !NO_DESTROY(j))
				GET_OBJ_TIMER(j)--;

			if (j && ((OBJ_FLAGGED(j, ITEM_ZONEDECAY) && GET_OBJ_ZONE(j) != NOWHERE && up_obj_where(j) != NOWHERE && GET_OBJ_ZONE(j) != world[up_obj_where(j)]->zone) || (GET_OBJ_TIMER(j) <= 0 && !NO_TIMER(j)) || (GET_OBJ_DESTROY(j) == 0 && !NO_DESTROY(j)))) {
	       /**** рассыпание обьекта */
				for (jj = j->contains; jj; jj = next_thing2) {
					next_thing2 = jj->next_content;
					obj_from_obj(jj);
					if (j->in_obj)
						obj_to_obj(jj, j->in_obj);
					else if (j->worn_by)
						obj_to_char(jj, j->worn_by);
					else if (j->carried_by)
						obj_to_char(jj, j->carried_by);
					else if (j->in_room != NOWHERE)
						obj_to_room(jj, j->in_room);
					else {
						log("SYSERR: extract %s from %s to NOTHING !!!",
						    jj->PNames[0], j->PNames[0]);
						// core_dump();
						extract_obj(jj);
					}
				}
				// Добавлено Ладником
//              next_thing = j->next; /* пурж по obj_to_room я убрал, но пускай на всякий случай */
				// Конец Ладник
				if (j->worn_by) {
					switch (j->worn_on) {
					case WEAR_LIGHT:
					case WEAR_SHIELD:
					case WEAR_WIELD:
					case WEAR_HOLD:
					case WEAR_BOTHS:
						act("$o рассыпал$U в Ваших руках...", FALSE, j->worn_by, j, 0, TO_CHAR);
						break;
					default:
						act("$o рассыпал$U прямо на Вас...", FALSE, j->worn_by, j, 0, TO_CHAR);
						break;
					}
					unequip_char(j->worn_by, j->worn_on);
				} else if (j->carried_by) {
					act("$o рассыпал$U в Ваших руках...", FALSE, j->carried_by, j, 0, TO_CHAR);
					obj_from_char(j);
				} else if (j->in_room != NOWHERE) {
					if (world[j->in_room]->people) {
						act("$o рассыпал$U в прах, который был развеян ветром...",
						    FALSE, world[j->in_room]->people, j, 0, TO_CHAR);
						act("$o рассыпал$U в прах, который был развеян ветром...",
						    FALSE, world[j->in_room]->people, j, 0, TO_ROOM);
					}
					obj_from_room(j);
				} else if (j->in_obj)
					obj_from_obj(j);
				extract_obj(j);
			} else {
				if (!j)
					continue;

				/* decay poision && other affects */
				for (count = 0; count < MAX_OBJ_AFFECT; count++)
					if (j->affected[count].location == APPLY_POISON) {
						j->affected[count].modifier--;
						if (j->affected[count].modifier <= 0) {
							j->affected[count].location = APPLY_NONE;
							j->affected[count].modifier = 0;
						}
					}
			}
		}
	}
	/* Тонущие, падающие, и сыпящиеся обьекты. */
	for (j = object_list; j; j = next_thing) {
		next_thing = j->next;	/* Next in object list */
		if (j->contains) {
			cont = TRUE;
		} else {
			cont = FALSE;
		}
		if (obj_decay(j)) {
			if (cont) {
				next_thing = object_list;
			}
		}
	}
}

void repop_decay(zone_rnum zone)
{				/* рассыпание обьектов ITEM_REPOP_DECAY */
	OBJ_DATA *j, *next_thing;
	int cont = FALSE;
	zone_vnum obj_zone_num, zone_num;

	zone_num = zone_table[zone].number;
	for (j = object_list; j; j = next_thing) {
		next_thing = j->next;	/* Next in object list */
		if (j->contains) {
			cont = TRUE;
		} else {
			cont = FALSE;
		}
		obj_zone_num = GET_OBJ_VNUM(j) / 100;
		if (((obj_zone_num == zone_num) && IS_OBJ_STAT(j, ITEM_REPOP_DECAY))) {
                               /* F@N
                                * Если мне кто-нибудь объяснит глубинный смысл последующей строчки,
                                * буду очень признателен
                            */
//                 || (GET_OBJ_TYPE(j) == ITEM_INGRADIENT && GET_OBJ_SKILL(j) > 19)
			if (j->worn_by)
				act("$o рассыпал$U, вспыхнув ярким светом...", FALSE, j->worn_by, j, 0, TO_CHAR);
			else if (j->carried_by)
				act("$o рассыпал$U в Ваших руках, вспыхнув ярким светом...",
				    FALSE, j->carried_by, j, 0, TO_CHAR);
			else if (j->in_room != NOWHERE) {
				if (world[j->in_room]->people) {
					act("$o рассыпал$U, вспыхнув ярким светом...",
					    FALSE, world[j->in_room]->people, j, 0, TO_CHAR);
					act("$o рассыпал$U, вспыхнув ярким светом...",
					    FALSE, world[j->in_room]->people, j, 0, TO_ROOM);
				}
			}
			extract_obj(j);
			if (cont)
				next_thing = object_list;
		}
	}
}
