/* ************************************************************************
*   File: mobact.cpp                                    Part of Bylins    *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "mobact.hpp"

#include "world.characters.hpp"
#include "world.objects.hpp"
#include "obj.hpp"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "skills.h"
#include "constants.h"
#include "pk.h"
#include "random.hpp"
#include "char.hpp"
#include "house.h"
#include "room.hpp"
#include "shop_ext.hpp"
#include "fight.h"
#include "fight_hit.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

// external structs
extern INDEX_DATA *mob_index;
extern int no_specials;
extern TIME_INFO_DATA time_info;
extern int guild_poly(CHAR_DATA*, void*, int, char*);
extern guardian_type guardian_list;
extern struct ZoneData * zone_table;
extern bool check_mighthit_weapon(CHAR_DATA *ch);

void do_get(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void go_bash(CHAR_DATA * ch, CHAR_DATA * vict);
void go_backstab(CHAR_DATA * ch, CHAR_DATA * vict);
void go_disarm(CHAR_DATA * ch, CHAR_DATA * vict);
void go_chopoff(CHAR_DATA * ch, CHAR_DATA * vict);
void go_mighthit(CHAR_DATA * ch, CHAR_DATA * vict);
void go_stupor(CHAR_DATA * ch, CHAR_DATA * vict);
void go_throw(CHAR_DATA * ch, CHAR_DATA * vict);
void go_strangle(CHAR_DATA * ch, CHAR_DATA * vict);
int skip_hiding(CHAR_DATA * ch, CHAR_DATA * vict);
int skip_sneaking(CHAR_DATA * ch, CHAR_DATA * vict);
int skip_camouflage(CHAR_DATA * ch, CHAR_DATA * vict);
int legal_dir(CHAR_DATA * ch, int dir, int need_specials_check, int show_msg);
int npc_track(CHAR_DATA * ch);
int npc_scavenge(CHAR_DATA * ch);
int npc_loot(CHAR_DATA * ch);
int npc_move(CHAR_DATA * ch, int dir, int need_specials_check);
void npc_wield(CHAR_DATA * ch);
void npc_armor(CHAR_DATA * ch);
void npc_group(CHAR_DATA * ch);
void npc_groupbattle(CHAR_DATA * ch);
int npc_walk(CHAR_DATA * ch);
int npc_steal(CHAR_DATA * ch);
void npc_light(CHAR_DATA * ch);
void pulse_affect_update(CHAR_DATA * ch);
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
bool guardian_attack(CHAR_DATA *ch, CHAR_DATA *vict);
extern bool is_room_forbidden(ROOM_DATA*room);
void drop_obj_on_zreset(CHAR_DATA *ch, OBJ_DATA *obj, bool inv, bool zone_reset);
int remove_otrigger(OBJ_DATA * obj, CHAR_DATA * actor);

// local functions
CHAR_DATA *try_protect(CHAR_DATA * victim, CHAR_DATA * ch);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

int extra_aggressive(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int time_ok = FALSE, no_time = TRUE, month_ok = FALSE, no_month = TRUE, agro = FALSE;

	if (!IS_NPC(ch))
		return (FALSE);

	if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
		return (TRUE);

	if (MOB_FLAGGED(ch, MOB_GUARDIAN)&& guardian_attack(ch, victim))
		return (TRUE);

	if (victim && MOB_FLAGGED(ch, MOB_AGGRMONO) && !IS_NPC(victim) && GET_RELIGION(victim) == RELIGION_MONO)
		agro = TRUE;

	if (victim && MOB_FLAGGED(ch, MOB_AGGRPOLY) && !IS_NPC(victim) && GET_RELIGION(victim) == RELIGION_POLY)
		agro = TRUE;

//Пока что убрал обработку флагов, тем более что персов кроме русичей и нет
//Поскольку расы и рода убраны из кода то так вот в лоб этот флаг не сделать,
//надо или по названию расы смотреть или еще что придумывать
	/*if (victim && MOB_FLAGGED(ch, MOB_AGGR_RUSICHI) && !IS_NPC(victim) && GET_KIN(victim) == KIN_RUSICHI)
		agro = TRUE;

	if (victim && MOB_FLAGGED(ch, MOB_AGGR_VIKINGI) && !IS_NPC(victim) && GET_KIN(victim) == KIN_VIKINGI)
		agro = TRUE;

	if (victim && MOB_FLAGGED(ch, MOB_AGGR_STEPNYAKI) && !IS_NPC(victim) && GET_KIN(victim) == KIN_STEPNYAKI)
		agro = TRUE; */

	if (MOB_FLAGGED(ch, MOB_AGGR_DAY))
	{
		no_time = FALSE;
		if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
			time_ok = TRUE;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_NIGHT))
	{
		no_time = FALSE;
		if (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET)
			time_ok = TRUE;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_WINTER))
	{
		no_month = FALSE;
		if (weather_info.season == SEASON_WINTER)
			month_ok = TRUE;
	}


	if (MOB_FLAGGED(ch, MOB_AGGR_SPRING))
	{
		no_month = FALSE;
		if (weather_info.season == SEASON_SPRING)
			month_ok = TRUE;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_SUMMER))
	{
		no_month = FALSE;
		if (weather_info.season == SEASON_SUMMER)
			month_ok = TRUE;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_AUTUMN))
	{
		no_month = FALSE;
		if (weather_info.season == SEASON_AUTUMN)
			month_ok = TRUE;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_FULLMOON))
	{
		no_time = FALSE;
		if (weather_info.moon_day >= 12 && weather_info.moon_day <= 15 &&
				(weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET))
			time_ok = TRUE;
	}
	if (agro || !no_time || !no_month)
		return ((no_time || time_ok) && (no_month || month_ok));
	else
		return (FALSE);
}

int attack_best(CHAR_DATA * ch, CHAR_DATA * victim)
{
	OBJ_DATA *wielded = GET_EQ(ch, WEAR_WIELD);
	if (victim)
	{
		if (ch->get_skill(SKILL_STRANGLE) && !timed_by_skill(ch, SKILL_STRANGLE))
		{
			go_strangle(ch, victim);
			return (TRUE);
		}
		if (ch->get_skill(SKILL_BACKSTAB) && !victim->get_fighting())
		{
			go_backstab(ch, victim);
			return (TRUE);
		}
		if (ch->get_skill(SKILL_MIGHTHIT))
		{
			go_mighthit(ch, victim);
			return (TRUE);
		}
		if (ch->get_skill(SKILL_STUPOR))
		{
			go_stupor(ch, victim);
			return (TRUE);
		}
		if (ch->get_skill(SKILL_BASH))
		{
			go_bash(ch, victim);
			return (TRUE);
		}
		if (ch->get_skill(SKILL_THROW)
			&& wielded
			&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON
			&& wielded->get_extra_flag(EExtraFlag::ITEM_THROWING))
		{
			go_throw(ch, victim);
		}
		if (ch->get_skill(SKILL_DISARM))
		{
			go_disarm(ch, victim);
		}
		if (ch->get_skill(SKILL_CHOPOFF))
		{
			go_chopoff(ch, victim);
		}
		if (!ch->get_fighting())
		{
			victim = try_protect(victim, ch);
			hit(ch, victim, TYPE_UNDEFINED, 1);
		}
		return (TRUE);
	}
	else
		return (FALSE);
}

#define KILL_FIGHTING   (1 << 0)
#define CHECK_HITS      (1 << 10)
#define SKIP_HIDING     (1 << 11)
#define SKIP_CAMOUFLAGE (1 << 12)
#define SKIP_SNEAKING   (1 << 13)
#define CHECK_OPPONENT  (1 << 14)
#define GUARD_ATTACK    (1 << 15)


CHAR_DATA *find_best_stupidmob_victim(CHAR_DATA * ch, int extmode)
{
	CHAR_DATA *victim, *use_light = NULL, *min_hp = NULL, *min_lvl = NULL, *caster = NULL, *best = NULL;
	int kill_this, extra_aggr = 0;

	victim = ch->get_fighting();

	for (const auto vict : world[ch->in_room]->people)
	{
		if ((IS_NPC(vict)
				&& !IS_SET(extmode, CHECK_OPPONENT)
				&& !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict)
				&& !vict->get_fighting()) // чармиса агрим только если он уже с кем-то сражается
			|| PRF_FLAGGED(vict, PRF_NOHASSLE)
			|| !MAY_SEE(ch, ch, vict)
			|| (IS_SET(extmode, CHECK_OPPONENT)
				&& ch != vict->get_fighting())
			|| (!may_kill_here(ch, vict)
				&& !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = FALSE;

		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& MOB_FLAGGED(ch, MOB_WIMPY)
			&& AWAKE(vict)
			&& GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch))
		{
			continue;
		}

		// Mobile helpers... //ассист
		if (IS_SET(extmode, KILL_FIGHTING)
			&& vict->get_fighting()
			&& vict->get_fighting() != ch
			&& IS_NPC(vict->get_fighting())
			&& !AFF_FLAGGED(vict->get_fighting(), EAffectFlag::AFF_CHARM)
			&& SAME_ALIGN(ch, vict->get_fighting()))
		{
			kill_this = TRUE;
		}
		else
		{
			// ... but no aggressive for this char
			if (!(extra_aggr = extra_aggressive(ch, vict))
				&& !IS_SET(extmode, GUARD_ATTACK))
			{
				continue;
			}
		}

		// skip sneaking, hiding and camouflaging pc
		if (IS_SET(extmode, SKIP_SNEAKING))
		{
			skip_sneaking(vict, ch);
			if ((EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK)))
			{
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_SNEAK);
			}
			if (AFF_FLAGGED(vict, EAffectFlag::AFF_SNEAK))
				continue;
		}

		if (IS_SET(extmode, SKIP_HIDING))
		{
			skip_hiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
			{
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_HIDE);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE))
		{
			skip_camouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
			{
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_CAMOUFLAGE);
			}
		}

		if (!CAN_SEE(ch, vict))
			continue;

		// Mobile aggresive
		if (!kill_this && extra_aggr)
		{
			if (can_use_feat(vict, SILVER_TONGUED_FEAT))
			{
				const int number1 = number(1, GET_LEVEL(vict) * GET_REAL_CHA(vict));
				const int range = ((GET_LEVEL(ch) > 30)
					? (GET_LEVEL(ch) * 2 * GET_REAL_INT(ch) + GET_REAL_INT(ch) * 20)
					: (GET_LEVEL(ch) * GET_REAL_INT(ch)));
				const int number2 = number(1, range);
				const bool do_continue = number1 > number2;
				if (do_continue)
				{
					continue;
				}
			}
			kill_this = TRUE;
		}

		if (!kill_this)
			continue;

		// define victim if not defined
		if (!victim)
			victim = vict;

		if (IS_DEFAULTDARK(ch->in_room)
			&& ((GET_EQ(vict, OBJ_DATA::ITEM_LIGHT)
					&& GET_OBJ_VAL(GET_EQ(vict, OBJ_DATA::ITEM_LIGHT), 2))
				|| (!AFF_FLAGGED(vict, EAffectFlag::AFF_HOLYDARK)
					&& (AFF_FLAGGED(vict, EAffectFlag::AFF_SINGLELIGHT)
						|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLYLIGHT))))
			&& (!use_light
				|| GET_REAL_CHA(use_light) > GET_REAL_CHA(vict)))
		{
			use_light = vict;
		}

		if (!min_hp
			|| GET_HIT(vict) + GET_REAL_CHA(vict) * 10 < GET_HIT(min_hp) + GET_REAL_CHA(min_hp) * 10)
		{
			min_hp = vict;
		}

		if (!min_lvl
			|| GET_LEVEL(vict) + number(1, GET_REAL_CHA(vict)) < GET_LEVEL(min_lvl) + number(1, GET_REAL_CHA(min_lvl)))
		{
			min_lvl = vict;
		}

		if (IS_CASTER(vict)
			&& (!caster
				|| GET_CASTER(caster) * GET_REAL_CHA(vict) < GET_CASTER(vict) * GET_REAL_CHA(caster)))
		{
			caster = vict;
		}
	}

	if (GET_REAL_INT(ch) < 5 + number(1, 6))
		best = victim;
	else if (GET_REAL_INT(ch) < 10 + number(1, 6))
		best = use_light ? use_light : victim;
	else if (GET_REAL_INT(ch) < 15 + number(1, 6))
		best = min_lvl ? min_lvl : (use_light ? use_light : victim);
	else if (GET_REAL_INT(ch) < 25 + number(1, 6))
		best = caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim));
	else
		best = min_hp ? min_hp : (caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim)));

	if (best && !ch->get_fighting() && MOB_FLAGGED(ch, MOB_AGGRMONO) &&
			!IS_NPC(best) && GET_RELIGION(best) == RELIGION_MONO)
	{
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на вас.", FALSE, ch, 0, best, TO_VICT);
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на $N3.", FALSE, ch, 0, best, TO_NOTVICT);
	}

	if (best && !ch->get_fighting() && MOB_FLAGGED(ch, MOB_AGGRPOLY) &&
			!IS_NPC(best) && GET_RELIGION(best) == RELIGION_POLY)
	{
		act("$n закричал$g: 'Умри, грязный язычник!' и набросил$u на вас.", FALSE, ch, 0, best, TO_VICT);
		act("$n закричал$g: 'Умри, грязный язычник!' и набросил$u на $N3.", FALSE, ch, 0, best, TO_NOTVICT);
	}

	return best;
}
// TODO invert and rename for clarity: -> isStrayCharmice(), to return true if a charmice, and master is absent =II
bool find_master_charmice(CHAR_DATA *charmice)
{
	// проверяем на спелл чарма, ищем хозяина и сравниваем румы
	if (!IS_CHARMICE(charmice) || !charmice->has_master())
	{
		return true;
	}

	if (charmice->in_room == charmice->get_master()->in_room)
	{
		return true;
	}

	return false;
}

// пока тестово
CHAR_DATA *find_best_mob_victim(CHAR_DATA * ch, int extmode)
{
	// интелект моба
	int i = GET_REAL_INT(ch);
	// если у моба меньше 20 инты, то моб тупой
	if (i < INT_STUPID_MOD)
	{
		return find_best_stupidmob_victim(ch, extmode);
	}
	CHAR_DATA *victim,  *caster = NULL, *best = NULL;
	CHAR_DATA *druid = NULL, *cler = NULL, *charmmage = NULL;
	int extra_aggr = 0;
	bool kill_this;
	victim = ch->get_fighting();
	// проходим по всем чарам в комнате
	for (const auto vict : world[ch->in_room]->people)
	{
		if ((IS_NPC(vict) && !IS_CHARMICE(vict))
				|| (IS_CHARMICE(vict) && !vict->get_fighting() && find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
				|| PRF_FLAGGED(vict, PRF_NOHASSLE)
				|| !MAY_SEE(ch, ch, vict) // если не видим цель,
				|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->get_fighting())
				|| (!may_kill_here(ch, vict) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = FALSE;
		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& MOB_FLAGGED(ch, MOB_WIMPY)
			&& AWAKE(vict) && GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch))
		{
			continue;
		}

		// Mobile helpers... //ассист
		if ((vict->get_fighting())
			&& (vict->get_fighting() != ch)
			&& (IS_NPC(vict->get_fighting()))
			&& (!AFF_FLAGGED(vict->get_fighting(), EAffectFlag::AFF_CHARM)))
		{
			kill_this = TRUE;
		}
		else
		{
			// ... but no aggressive for this char
			if (!(extra_aggr = extra_aggressive(ch, vict))
				&& !IS_SET(extmode, GUARD_ATTACK))
			{
				continue;
			}
		}
		if (IS_SET(extmode, SKIP_SNEAKING))
		{
			skip_sneaking(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))
			{
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_SNEAK);
			}

			if (AFF_FLAGGED(vict, EAffectFlag::AFF_SNEAK))
			{
				continue;
			}
		}

		if (IS_SET(extmode, SKIP_HIDING))
		{
			skip_hiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
			{
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_HIDE);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE))
		{
			skip_camouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
			{
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_CAMOUFLAGE);
			}
		}
		if (!CAN_SEE(ch, vict))
			continue;

		if (!kill_this && extra_aggr)
		{
			if (can_use_feat(vict, SILVER_TONGUED_FEAT)
				&& number(1, GET_LEVEL(vict) * GET_REAL_CHA(vict)) > number(1, GET_LEVEL(ch) * GET_REAL_INT(ch)))
			{
				continue;
			}
			kill_this = TRUE;
		}

		if (!kill_this)
			continue;
		// волхв
		if (GET_CLASS(vict) == CLASS_DRUID)
		{
			druid = vict;
			caster = vict;
			continue;
		}
		// лекарь
		if (GET_CLASS(vict) == CLASS_CLERIC)
		{
			cler = vict;
			caster = vict;
			continue;
		}
		// кудес
		if (GET_CLASS(vict) == CLASS_CHARMMAGE)
		{
			charmmage = vict;
			caster = vict;
			continue;
		}
		// если у чара меньше 100 хп, то переключаемся на него
		if (GET_HIT(vict) <= MIN_HP_MOBACT)
		{
			//continue;
			//Кто-то сильно очепятался. Теперь тем у кого меньше 100 хп меньше повезет
			return vict;
		}
		if (IS_CASTER(vict))
		{
			caster = vict;
			continue;
		}
		best = vict;
	}

	if (!best)
	{
		best = victim;
	}

	// если цель кастер, то зачем переключаться ?
	// проверка, а вдруг это существо моб
	if (victim && !IS_NPC(victim))
	{
		if (IS_CASTER(victim))
		{
			return victim;
		}
	}

	if (i < INT_MIDDLE_AI)
	{
		int rand = number(0, 2);
		if (caster)
		{
			best = caster;
		}
		if ((rand == 0) && (druid))
		{
			best = druid;
		}
		if ((rand == 1) && (cler))
		{
			best = cler;
		}
		if ((rand == 2) && (charmmage))
		{
			best = charmmage;
		}
		return best;
	}

	if (i < INT_HIGH_AI)
	{
		int rand = number(0, 1);
		if (caster)
			best = caster;
		if (charmmage)
			best = charmmage;
		if ((rand == 0) && (druid))
			best = druid;
		if ((rand == 1) && (cler))
			best = cler;

		return best;
	}

	//  и если >= 40 инты
	if (caster)
		best = caster;
	if (charmmage)
		best = charmmage;
	if (cler)
		best = cler;
	if (druid)
		best = druid;

	return best;
}

int perform_best_mob_attack(CHAR_DATA * ch, int extmode)
{
	CHAR_DATA *best;
	int clone_number = 0;
	best = find_best_mob_victim(ch, extmode);

	if (best)
	{
	/*
				   sprintf(buf,"Attacker-%s,B-%s,IS-NPC-%s,IS-CLONE-%s",
				   GET_NAME(ch),
				   best ? GET_NAME(best) : "None",
				   IS_NPC(best)   ? "1" : "0",
				   MOB_FLAGGED(best, MOB_CLONE)   ? "1" : "0");
				   act(buf,FALSE,ch,0,0,TO_ROOM);
				 */
		if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
		{
			act("$n вскочил$g на ноги.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_STANDING;
		}

		if (IS_SET(extmode, KILL_FIGHTING) && best->get_fighting())
		{
			if (MOB_FLAGGED(best->get_fighting(), MOB_NOHELPS))
				return (FALSE);
			act("$n вступил$g в битву на стороне $N1.", FALSE, ch, 0, best->get_fighting(), TO_ROOM);
		}

		if (IS_SET(extmode, GUARD_ATTACK))
		{
			act("'$N - за грехи свои ты заслуживаешь смерти!', сурово проговорил$g $n.", FALSE, ch, 0, best, TO_ROOM);
			act("'Как страж этого города, я намерен$g привести приговор в исполнение немедленно. Защищайся!'", FALSE, ch, 0, best, TO_ROOM);
		}

		if (!IS_NPC(best))
		{
			struct follow_type *f;
			// Обработка клоунов
			// Теперь работает как обычный mirror image
// неявно предполагаем, что клоны могут следовать только за своим создателем
			for (f = best->followers; f; f = f->next)
				if (MOB_FLAGGED(f->follower, MOB_CLONE))
					clone_number++;
			for (f = best->followers; f; f = f->next)
				if (IS_NPC(f->follower) &&
						MOB_FLAGGED(f->follower, MOB_CLONE) && IN_ROOM(f->follower) == IN_ROOM(best))
				{
					if (number(0, clone_number) == 1)
						break;
					if( (GET_REAL_INT(ch)<20) && number(0,clone_number) ) break;
					if( GET_REAL_INT(ch)>=30 ) break;
					if( (GET_REAL_INT(ch)>=20) && number(1, 10+VPOSI((35-GET_REAL_INT(ch)),0,15)*clone_number)<=10  ) break;
					best = f->follower;
					break;
				}
		}
		if (!attack_best(ch, best) && !ch->get_fighting())
			hit(ch, best, TYPE_UNDEFINED, 1);
		return (TRUE);
	}
	return (FALSE);
}

int perform_best_horde_attack(CHAR_DATA * ch, int extmode)
{
	if (perform_best_mob_attack(ch, extmode))
	{
		return (TRUE);
	}

	for (const auto vict : world[ch->in_room]->people)
	{
		if (!IS_NPC(vict)
			|| !MAY_SEE(ch, ch, vict)
			|| MOB_FLAGGED(vict, MOB_PROTECT))
		{
			continue;
		}

		if (!SAME_ALIGN(ch, vict))
		{
			if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
			{
				act("$n вскочил$g на ноги.", FALSE, ch, 0, 0, TO_ROOM);
				GET_POS(ch) = POS_STANDING;
			}

			if (!attack_best(ch, vict) && !ch->get_fighting())
			{
				hit(ch, vict, TYPE_UNDEFINED, 1);
			}

			return (TRUE);
		}
	}
	return (FALSE);
}

int perform_mob_switch(CHAR_DATA * ch)
{
	CHAR_DATA *best;
	best = find_best_mob_victim(ch, SKIP_HIDING | SKIP_CAMOUFLAGE | SKIP_SNEAKING | CHECK_OPPONENT);

	if (!best)
		return FALSE;

	best = try_protect(best, ch);
	if (best == ch->get_fighting())
		return FALSE;

	// переключаюсь на best
	stop_fighting(ch, FALSE);
	set_fighting(ch, best);
	set_wait(ch, 2, FALSE);

	if (ch->get_skill(SKILL_MIGHTHIT)
		&& check_mighthit_weapon(ch))
	{
		SET_AF_BATTLE(ch, EAF_MIGHTHIT);
	}
	else if (ch->get_skill(SKILL_STUPOR))
	{
		SET_AF_BATTLE(ch, SKILL_STUPOR);
	}

	return TRUE;
}

void do_aggressive_mob(CHAR_DATA *ch, int check_sneak)
{
	if (!ch || ch->in_room == NOWHERE || !IS_NPC(ch)
		|| !MAY_ATTACK(ch) || AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		return;
	}

	int mode = check_sneak ? SKIP_SNEAKING : 0;

	// ****************  Horde
	if (MOB_FLAGGED(ch, MOB_HORDE))
	{
		perform_best_horde_attack(ch, mode | SKIP_HIDING | SKIP_CAMOUFLAGE);
		return;
	}

	// ****************  Aggressive Mobs
	if (extra_aggressive(ch, NULL))
	{
		perform_best_mob_attack(ch, mode | SKIP_HIDING | SKIP_CAMOUFLAGE | CHECK_HITS);
		return;
	}
	//Polud стражники
	if (MOB_FLAGGED(ch, MOB_GUARDIAN))
	{
		perform_best_mob_attack(ch, SKIP_HIDING | SKIP_CAMOUFLAGE | SKIP_SNEAKING | GUARD_ATTACK);
		return;
	}

	// *****************  Mob Memory
	if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
	{
		CHAR_DATA* victim = nullptr;
		// Find memory in room
		const auto people_copy = world[ch->in_room]->people;
		for (auto vict_i = people_copy.begin(); vict_i != people_copy.end() && !victim; ++vict_i)
		{
			const auto vict = *vict_i;

			if (IS_NPC(vict)
				|| PRF_FLAGGED(vict, PRF_NOHASSLE))
			{
				continue;
			}
			for (memory_rec *names = MEMORY(ch); names && !victim; names = names->next)
			{
				if (names->id == GET_IDNUM(vict))
				{
					if (!MAY_SEE(ch, ch, vict) || !may_kill_here(ch, vict))
					{
						continue;
					}
					if (check_sneak)
					{
						skip_sneaking(vict, ch);
						if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))
						{
							AFF_FLAGS(vict).unset(EAffectFlag::AFF_SNEAK);
						}
						if (AFF_FLAGGED(vict, EAffectFlag::AFF_SNEAK))
								continue;
					}
					skip_hiding(vict, ch);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
					{
						AFF_FLAGS(vict).unset(EAffectFlag::AFF_HIDE);
					}
					skip_camouflage(vict, ch);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
					{
						AFF_FLAGS(vict).unset(EAffectFlag::AFF_CAMOUFLAGE);
					}
					if (CAN_SEE(ch, vict))
					{
						victim = vict;
					}
				}
			}
		}

		// Is memory found ?
		if (victim && !CHECK_WAIT(ch))
		{
			if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
			{
				act("$n вскочил$g на ноги.", FALSE, ch, 0, 0, TO_ROOM);
				GET_POS(ch) = POS_STANDING;
			}
			if (GET_RACE(ch) != NPC_RACE_HUMAN)
			{
				act("$n вспомнил$g $N3.", FALSE, ch, 0, victim, TO_ROOM);
			}
			else
			{
				act("'$N - ты пытал$U убить меня ! Попал$U ! Умри !!!', воскликнул$g $n.",
					FALSE, ch, 0, victim, TO_ROOM);
			}
			if (!attack_best(ch, victim))
			{
				hit(ch, victim, TYPE_UNDEFINED, 1);
			}
			return;
		}
	}

	// ****************  Helper Mobs
	if (MOB_FLAGGED(ch, MOB_HELPER))
	{
		perform_best_mob_attack(ch, mode | KILL_FIGHTING | CHECK_HITS);
		return;
	}
}

/**
* Примечание: сам ch после этой функции уже может быть спуржен
* в результате агра на себя кого-то в комнате и начале атаки
* например с глуша.
*/
void do_aggressive_room(CHAR_DATA *ch, int check_sneak)
{
	if (!ch || ch->in_room == NOWHERE)
	{
		return;
	}

	const auto people = world[ch->in_room]->people;	// сделать копию people, т. к. оно может измениться в теле цикла и итераторы будут испорчены
	for (const auto& vict: people)
	{
		// здесь не надо преварително запоминать next_in_room, потому что как раз
		// он то и может быть спуржен по ходу do_aggressive_mob, а вот атакующий нет
		do_aggressive_mob(vict, check_sneak);
	}
}

/**
 * Проверка на наличие в комнате мобов с таким же спешиалом, что и входящий.
 * \param ch - входящий моб
 * \return true - можно войти, false - нельзя
 */
bool allow_enter(ROOM_DATA *room, CHAR_DATA *ch)
{
	if (!IS_NPC(ch) || !GET_MOB_SPEC(ch))
	{
		return true;
	}

	for (const auto vict : room->people)
	{
		if (IS_NPC(vict)
			&& GET_MOB_SPEC(vict) == GET_MOB_SPEC(ch))
		{
			return false;
		}
	}

	return true;
}

namespace
{
	OBJ_DATA* create_charmice_box(CHAR_DATA* ch)
	{
		const auto obj = world_objects.create_blank();

		obj->set_aliases("узелок вещами");
		const std::string descr = std::string("узелок с вещами ") + ch->get_pad(1);
		obj->set_short_description(descr);
		obj->set_description("Туго набитый узел лежит тут.");
		obj->set_ex_description(descr.c_str(), "Кто-то сильно торопился, когда набивал этот узелок.");
		obj->set_PName(0, "узелок");
		obj->set_PName(1, "узелка");
		obj->set_PName(2, "узелку");
		obj->set_PName(3, "узелок");
		obj->set_PName(4, "узелком");
		obj->set_PName(5, "узелке");
		obj->set_sex(ESex::SEX_MALE);
		obj->set_type(OBJ_DATA::ITEM_CONTAINER);
		obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));
		obj->set_weight(1);
		obj->set_cost(1);
		obj->set_rent_off(1);
		obj->set_rent_on(1);
		obj->set_timer(24 * 60);

		obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);
		obj->set_extra_flag(EExtraFlag::ITEM_NOLOCATE);
		obj->set_extra_flag(EExtraFlag::ITEM_NODECAY);
		obj->set_extra_flag(EExtraFlag::ITEM_SWIMMING);
		obj->set_extra_flag(EExtraFlag::ITEM_FLYING);

		return obj.get();
	}

	void extract_charmice(CHAR_DATA* ch)
	{
		std::vector<OBJ_DATA*> objects;
		for (int i = 0; i < NUM_WEARS; ++i)
		{
			if (GET_EQ(ch, i))
			{
				OBJ_DATA* obj = unequip_char(ch, i);
				if (obj)
				{
					remove_otrigger(obj, ch);
					objects.push_back(obj);
				}
			}
		}

		while (ch->carrying)
		{
			OBJ_DATA *obj = ch->carrying;
			obj_from_char(obj);
			objects.push_back(obj);
		}

		if (!objects.empty())
		{
			OBJ_DATA* charmice_box = create_charmice_box(ch);
			for (std::vector<OBJ_DATA*>::iterator it = objects.begin(); it != objects.end(); ++it) {
				obj_to_obj(*it, charmice_box);
			}
			drop_obj_on_zreset(ch, charmice_box, 1, false);
		}

		extract_char(ch, FALSE);
	}
}

void mobile_activity(int activity_level, int missed_pulses)
{
	int door, max, was_in = -1, activity_lev, i, ch_activity;
	int std_lev = activity_level % PULSE_MOBILE;

	character_list.foreach_on_copy([&](const CHAR_DATA::shared_ptr& ch)
	{
		if (!IS_MOB(ch)
			|| !ch->in_used_zone())
		{
			return;
		}

		i = missed_pulses;
		while (i--)
		{
			pulse_affect_update(ch.get());
		}

		ch->wait_dec(missed_pulses);

		if (GET_PUNCTUAL_WAIT(ch) > 0)
			GET_PUNCTUAL_WAIT(ch) -= missed_pulses;
		else
			GET_PUNCTUAL_WAIT(ch) = 0;

		if (GET_PUNCTUAL_WAIT(ch) < 0)
			GET_PUNCTUAL_WAIT(ch) = 0;

		if (ch->mob_specials.speed <= 0)
		{
			activity_lev = std_lev;
		}
		else
		{
			activity_lev = activity_level % (ch->mob_specials.speed RL_SEC);
		}

		ch_activity = GET_ACTIVITY(ch);

// на случай вызова mobile_activity() не каждый пульс
		// TODO: by WorM а где-то используется это mob_specials.speed ???
		if (ch_activity - activity_lev < missed_pulses
			&& ch_activity - activity_lev >= 0)
		{
			ch_activity = activity_lev;
		}
		if (ch_activity != activity_lev
			|| (was_in = ch->in_room) == NOWHERE
			|| GET_ROOM_VNUM(ch->in_room) % 100 == 99)
		{
			return;
		}

		// Examine call for special procedure
		if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
		{
			if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
			{
				log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
					GET_NAME(ch), GET_MOB_VNUM(ch));
				MOB_FLAGS(ch).unset(MOB_SPEC);
			}
			else
			{
				buf2[0] = '\0';
				if ((mob_index[GET_MOB_RNUM(ch)].func)(ch.get(), ch.get(), 0, buf2))
				{
					return;	// go to next char
				}
			}
		}
		// Extract free horses
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE)
			&& MOB_FLAGGED(ch, MOB_MOUNTING)
			&& !ch->has_master()) // если скакун, под седлом но нет хозяина
		{
			act("Возникший как из-под земли цыган ловко вскочил на $n3 и унесся прочь.",
				FALSE, ch.get(), 0, 0, TO_ROOM);
			extract_char(ch.get(), FALSE);

			return;
		}

		// Extract uncharmed mobs
		if (EXTRACT_TIMER(ch) > 0)
		{
			if (ch->has_master())
			{
				EXTRACT_TIMER(ch) = 0;
			}
			else
			{
				EXTRACT_TIMER(ch)--;
				if (!EXTRACT_TIMER(ch))
				{
					extract_charmice(ch.get());

					return;
				}
			}
		}

		// If the mob has no specproc, do the default actions
		if (ch->get_fighting() ||
				GET_POS(ch) <= POS_STUNNED ||
				GET_WAIT(ch) > 0 ||
				AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ||
				AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT) ||
				AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
		{
			return;
		}

		if (IS_HORSE(ch))
		{
			if (GET_POS(ch) < POS_FIGHTING)
			{
				GET_POS(ch) = POS_STANDING;
			}

			return;
		}

		if (GET_POS(ch) == POS_SLEEPING && GET_DEFAULT_POS(ch) > POS_SLEEPING)
		{
			GET_POS(ch) = GET_DEFAULT_POS(ch);
			act("$n проснул$u.", FALSE, ch.get(), 0, 0, TO_ROOM);
		}

		if (!AWAKE(ch))
		{
			return;
		}

		max = FALSE;
		bool found = false;
		for (const auto vict : world[ch->in_room]->people)
		{
			if (ch.get() == vict)
			{
				continue;
			}

			if (vict->get_fighting() == ch.get())
			{				
				return;		// Mob is under attack
			}

			if (!IS_NPC(vict)
				&& CAN_SEE(ch, vict))
			{
				max = TRUE;
			}
		}

		// Mob attemp rest if it is not an angel
		if (!max && !MOB_FLAGGED(ch, MOB_NOREST) &&
				GET_HIT(ch) < GET_REAL_MAX_HIT(ch) && !MOB_FLAGGED(ch, MOB_ANGEL) && !MOB_FLAGGED(ch, MOB_GHOST) && GET_POS(ch) > POS_RESTING)
		{
			act("$n присел$g отдохнуть.", FALSE, ch.get(), 0, 0, TO_ROOM);
			GET_POS(ch) = POS_RESTING;
		}

		// Mob return to default pos if full rested or if it is an angel
		if ((GET_HIT(ch) >= GET_REAL_MAX_HIT(ch)
				&& GET_POS(ch) != GET_DEFAULT_POS(ch))
			|| ((MOB_FLAGGED(ch, MOB_ANGEL)
					|| MOB_FLAGGED(ch, MOB_GHOST))
				&& GET_POS(ch) != GET_DEFAULT_POS(ch)))
		{
			switch (GET_DEFAULT_POS(ch))
			{
			case POS_STANDING:
				act("$n встал$g на ноги.", FALSE, ch.get(), 0, 0, TO_ROOM);
				GET_POS(ch) = POS_STANDING;
				break;
			case POS_SITTING:
				act("$n сел$g.", FALSE, ch.get(), 0, 0, TO_ROOM);
				GET_POS(ch) = POS_SITTING;
				break;
			case POS_RESTING:
				act("$n присел$g отдохнуть.", FALSE, ch.get(), 0, 0, TO_ROOM);
				GET_POS(ch) = POS_RESTING;
				break;
			case POS_SLEEPING:
				act("$n уснул$g.", FALSE, ch.get(), 0, 0, TO_ROOM);
				GET_POS(ch) = POS_SLEEPING;
				break;
			}
		}
		// continue, if the mob is an angel
		// если моб ментальная тень или ангел он не должен проявлять активность
		if ((MOB_FLAGGED(ch, MOB_ANGEL))
			||(MOB_FLAGGED(ch, MOB_GHOST)))
		{
			return;
		}

		// look at room before moving
		do_aggressive_mob(ch.get(), FALSE);

		// if mob attack something
		if (ch->get_fighting()
			|| GET_WAIT(ch) > 0)
		{
			return;
		}

		// Scavenger (picking up objects)
		// От одного до трех предметов за раз
		i = number(1, 3);
		while (i)
		{
			npc_scavenge(ch.get());
			i--;
		}

		if (EXTRACT_TIMER(ch) == 0)
		{
			//чармисы, собирающиеся уходить - не лутят! (Купала)
			//Niker: LootCR// Start
			//Не уверен, что рассмотрены все случаи, когда нужно снимать флаги с моба
			//Реализация для лута и воровства
			int grab_stuff = FALSE;
			// Looting the corpses

			grab_stuff += npc_loot(ch.get());
			grab_stuff += npc_steal(ch.get());

			if (grab_stuff)
			{
				MOB_FLAGS(ch).unset(MOB_LIKE_DAY);	//Взял из make_horse
				MOB_FLAGS(ch).unset(MOB_LIKE_NIGHT);
				MOB_FLAGS(ch).unset(MOB_LIKE_FULLMOON);
				MOB_FLAGS(ch).unset(MOB_LIKE_WINTER);
				MOB_FLAGS(ch).unset(MOB_LIKE_SPRING);
				MOB_FLAGS(ch).unset(MOB_LIKE_SUMMER);
				MOB_FLAGS(ch).unset(MOB_LIKE_AUTUMN);
			}
			//Niker: LootCR// End
		}
		npc_wield(ch.get());
		npc_armor(ch.get());

		if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_INVIS))
		{
			ch->set_affect(EAffectFlag::AFF_INVISIBLE);
		}

		if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_MOVEFLY))
		{
			ch->set_affect(EAffectFlag::AFF_FLY);
		}

		if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_SNEAK))
		{
			if (calculate_skill(ch.get(), SKILL_SNEAK, 0) >= number(0, 100))
			{
				ch->set_affect(EAffectFlag::AFF_SNEAK);
			}
			else
			{
				ch->remove_affect(EAffectFlag::AFF_SNEAK);
			}
			affect_total(ch.get());
		}

		if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_CAMOUFLAGE))
		{
			if (calculate_skill(ch.get(), SKILL_CAMOUFLAGE, 0) >= number(0, 100))
			{
				ch->set_affect(EAffectFlag::AFF_CAMOUFLAGE);
			}
			else
			{
				ch->remove_affect(EAffectFlag::AFF_CAMOUFLAGE);
			}

			affect_total(ch.get());
		}

		door = BFS_ERROR;

		// Helpers go to some dest
		if (MOB_FLAGGED(ch, MOB_HELPER)
			&& !MOB_FLAGGED(ch, MOB_SENTINEL)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)
			&& !ch->has_master()
			&& GET_POS(ch) == POS_STANDING)
		{
			for (found = FALSE, door = 0; door < NUM_OF_DIRS; door++)
			{
				ROOM_DATA::exit_data_ptr rdata = EXIT(ch, door);
				if (!rdata
					|| rdata->to_room() == NOWHERE
					|| !legal_dir(ch.get(), door, TRUE, FALSE)
					|| (is_room_forbidden(world[rdata->to_room()])
						&& !MOB_FLAGGED(ch, MOB_IGNORE_FORBIDDEN))
					|| IS_DARK(rdata->to_room())
					|| (MOB_FLAGGED(ch, MOB_STAY_ZONE)
						&& world[ch->in_room]->zone != world[rdata->to_room()]->zone))
				{
					continue;
				}

				const auto room = world[rdata->to_room()];
				for (auto first : room->people)
				{
					if (IS_NPC(first)
						&& !AFF_FLAGGED(first, EAffectFlag::AFF_CHARM)
						&& !IS_HORSE(first)
						&& CAN_SEE(ch, first)
						&& first->get_fighting()
						&& SAME_ALIGN(ch, first))
					{
						found = TRUE;
						break;
					}
				}

				if (found)
				{
					break;
				}
			}

			if (!found)
			{
				door = BFS_ERROR;
			}
		}

		if (GET_DEST(ch) != NOWHERE
			&& GET_POS(ch) > POS_FIGHTING
			&& door == BFS_ERROR)
		{
			npc_group(ch.get());
			door = npc_walk(ch.get());
		}

		if (MEMORY(ch) && door == BFS_ERROR && GET_POS(ch) > POS_FIGHTING && ch->get_skill(SKILL_TRACK))
			door = npc_track(ch.get());

		if (door == BFS_ALREADY_THERE)
		{
			do_aggressive_mob(ch.get(), FALSE);
			return;
		}

		if (door == BFS_ERROR)
		{
			door = number(0, 18);
		}

		// Mob Movement
		if (!MOB_FLAGGED(ch, MOB_SENTINEL)
				&& GET_POS(ch) == POS_STANDING
				&& (door >= 0 && door < NUM_OF_DIRS)
				&& EXIT(ch, door)
				&& EXIT(ch, door)->to_room() != NOWHERE
				&& legal_dir(ch.get(), door, TRUE, FALSE)
				&& (!is_room_forbidden(world[EXIT(ch, door)->to_room()]) || MOB_FLAGGED(ch, MOB_IGNORE_FORBIDDEN))
				&& (!MOB_FLAGGED(ch, MOB_STAY_ZONE)
					|| world[EXIT(ch, door)->to_room()]->zone == world[ch->in_room]->zone)
				&& allow_enter(world[EXIT(ch, door)->to_room()], ch.get()))
		{
			// После хода нпц уже может не быть, т.к. ушел в дт, я не знаю почему
			// оно не валится на муд.ру, но на цигвине у меня падало стабильно,
			// т.к. в ch уже местами мусор после фри-чара // Krodo
			if (npc_move(ch.get(), door, 1))
			{
				npc_group(ch.get());
				npc_groupbattle(ch.get());
			}
			else
			{
				return;
			}
		}

		npc_light(ch.get());

		// *****************  Mob Memory
		if (MOB_FLAGGED(ch, MOB_MEMORY)
			&& MEMORY(ch)
			&& GET_POS(ch) > POS_SLEEPING
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)
			&& !ch->get_fighting())
		{
			// Find memory in world
			for (auto names = MEMORY(ch); names && (GET_SPELL_MEM(ch, SPELL_SUMMON) > 0
				|| GET_SPELL_MEM(ch, SPELL_RELOCATE) > 0); names = names->next)
			{
				for (const auto& vict : character_list)
				{
					if (names->id == GET_IDNUM(vict)
						&& CAN_SEE(ch, vict) && !PRF_FLAGGED(vict, PRF_NOHASSLE))
					{
						if (GET_SPELL_MEM(ch, SPELL_SUMMON) > 0)
						{
							cast_spell(ch.get(), vict.get(), 0, 0, SPELL_SUMMON, SPELL_SUMMON);

							break;
						}
						else if (GET_SPELL_MEM(ch, SPELL_RELOCATE) > 0)
						{
							cast_spell(ch.get(), vict.get(), 0, 0, SPELL_RELOCATE, SPELL_RELOCATE);

							break;
						}
					}
				}
			}
		}

		// Add new mobile actions here

		if (was_in != ch->in_room)
		{
			do_aggressive_room(ch.get(), FALSE);
		}
	});			// end for()
}

// Mob Memory Routines
// 11.07.2002 - у зачармленных мобов не работает механизм памяти на время чарма

// make ch remember victim
void remember(CHAR_DATA * ch, CHAR_DATA * victim)
{
	struct timed_type timed;
	memory_rec *tmp;
	bool present = FALSE;

	if (!IS_NPC(ch) ||
			IS_NPC(victim) ||
			PRF_FLAGGED(victim, PRF_NOHASSLE) || !MOB_FLAGGED(ch, MOB_MEMORY) || AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
		if (tmp->id == GET_IDNUM(victim))
		{
			if (tmp->time > 0)
				tmp->time = time(NULL) + MOB_MEM_KOEFF * GET_REAL_INT(ch);
			present = TRUE;
		}

	if (!present)
	{
		CREATE(tmp, 1);
		tmp->next = MEMORY(ch);
		tmp->id = GET_IDNUM(victim);
		tmp->time = time(NULL) + MOB_MEM_KOEFF * GET_REAL_INT(ch);
		MEMORY(ch) = tmp;
	}

	if (!timed_by_skill(victim, SKILL_HIDETRACK))
	{
		timed.skill = SKILL_HIDETRACK;
		timed.time = ch->get_skill(SKILL_TRACK) ? 6 : 3;
		timed_to_char(victim, &timed);
	}
}


// make ch forget victim
void forget(CHAR_DATA * ch, CHAR_DATA * victim)
{
	memory_rec *curr, *prev = NULL;

	// Момент спорный, но думаю, что так правильнее
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (!(curr = MEMORY(ch)))
		return;

	while (curr && curr->id != GET_IDNUM(victim))
	{
		prev = curr;
		curr = curr->next;
	}

	if (!curr)
		return;		// person wasn't there at all.

	if (curr == MEMORY(ch))
		MEMORY(ch) = curr->next;
	else
		prev->next = curr->next;

	free(curr);
}

// erase ch's memory
// Можно заметить, что функция вызывается только при extract char/mob
// Удаляется все подряд
void clearMemory(CHAR_DATA * ch)
{
	memory_rec *curr, *next;

	curr = MEMORY(ch);

	while (curr)
	{
		next = curr->next;
		free(curr);
		curr = next;
	}
	MEMORY(ch) = NULL;
}
//Polud Функция проверяет, является ли моб ch стражником (описан в файле guards.xml)
//и должен ли он сагрить на эту жертву vict
bool guardian_attack(CHAR_DATA *ch, CHAR_DATA *vict)
{
	struct mob_guardian tmp_guard;
	int num_wars_vict = 0;

	if (!IS_NPC(ch) || !vict || !MOB_FLAGGED(ch, MOB_GUARDIAN))
		return false;
//на всякий случай проверим, а вдруг моб да подевался куда из списка...
	guardian_type::iterator it = guardian_list.find(GET_MOB_VNUM(ch));
	if (it == guardian_list.end())
		return false;

	tmp_guard = guardian_list[GET_MOB_VNUM(ch)];

	if ((tmp_guard.agro_all_agressors && AGRESSOR(vict)) ||
		(tmp_guard.agro_killers && PLR_FLAGGED(vict, PLR_KILLER)))
		return true;

	if (CLAN(vict))
	{
		num_wars_vict = Clan::GetClanWars(vict);
		int clan_town_vnum = CLAN(vict)->GetOutRent()/100; //Polud подскажите мне другой способ определить vnum зоны
		int mob_town_vnum = GET_MOB_VNUM(ch)/100;          //по vnum комнаты, не перебирая все комнаты и зоны мира
		if (num_wars_vict && num_wars_vict > tmp_guard.max_wars_allow &&  clan_town_vnum != mob_town_vnum)
			return true;
	}
	if (AGRESSOR(vict))
		for (std::vector<zone_vnum>::iterator iter = tmp_guard.agro_argressors_in_zones.begin(); iter != tmp_guard.agro_argressors_in_zones.end();iter++)
		{
			if (*iter == AGRESSOR(vict)/100) return true;
		}

	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
