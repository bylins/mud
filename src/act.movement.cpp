/* ************************************************************************
*   File: act.movement.cpp                              Part of Bylins    *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "logger.hpp"
#include "utils.h"
#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "skills.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "screen.h"
#include "pk.h"
#include "features.hpp"
#include "deathtrap.hpp"
#include "privilege.hpp"
#include "char.hpp"
#include "corpse.hpp"
#include "room.hpp"
#include "named_stuff.hpp"
#include "fight.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "random.hpp"
#include <cmath>

// external functs
void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
int find_eq_pos(CHAR_DATA * ch, OBJ_DATA * obj, char *arg);
int attack_best(CHAR_DATA * ch, CHAR_DATA * victim);
int awake_others(CHAR_DATA * ch);
void affect_from_char(CHAR_DATA * ch, int type);
void do_aggressive_room(CHAR_DATA * ch, int check_sneak);
void die(CHAR_DATA * ch, CHAR_DATA * killer);
// local functions
void check_ice(int room);
int has_boat(CHAR_DATA * ch);
int find_door(CHAR_DATA * ch, const char *type, char *dir, const char *cmdname);
int has_key(CHAR_DATA * ch, obj_vnum key);
void do_doorcmd(CHAR_DATA * ch, OBJ_DATA * obj, int door, int scmd);
int ok_pick(CHAR_DATA * ch, obj_vnum keynum, OBJ_DATA* obj, int door, int scmd);
extern int get_pick_chance(int skill_pick, int lock_complexity);

void do_gen_door(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_enter(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stand(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sleep(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wake(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_follow(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

const int Reverse[NUM_OF_DIRS] = { 2, 3, 0, 1, 5, 4 };
const char *DirIs[] =
{
	"север",
	"восток",
	"юг",
	"запад",
	"вверх",
	"вниз",
	"\n"
};

// check ice in room
int check_death_ice(int room, CHAR_DATA* /*ch*/)
{
	int sector, mass = 0, result = FALSE;

	if (room == NOWHERE)
		return (FALSE);
	sector = SECT(room);
	if (sector != SECT_WATER_SWIM && sector != SECT_WATER_NOSWIM)
		return (FALSE);
	if ((sector = real_sector(room)) != SECT_THIN_ICE && sector != SECT_NORMAL_ICE)
		return (FALSE);

	for (const auto vict : world[room]->people)
	{
		if (!IS_NPC(vict)
			&& !AFF_FLAGGED(vict, EAffectFlag::AFF_FLY))
		{
			mass += GET_WEIGHT(vict) + IS_CARRYING_W(vict);
		}
	}

	if (!mass)
	{
		return (FALSE);
	}

	if ((sector == SECT_THIN_ICE && mass > 500) || (sector == SECT_NORMAL_ICE && mass > 1500))
	{
		const auto first_in_room = world[room]->first_character();

		act("Лед проломился под вашей тяжестью.", FALSE, first_in_room, 0, 0, TO_ROOM);
		act("Лед проломился под вашей тяжестью.", FALSE, first_in_room, 0, 0, TO_CHAR);

		world[room]->weather.icelevel = 0;
		world[room]->ices = 2;
		GET_ROOM(room)->set_flag(ROOM_ICEDEATH);
		DeathTrap::add(world[room]);
	}
	else
	{
		return (FALSE);
	}

	return (result);
}

// simple function to determine if char can walk on water
int has_boat(CHAR_DATA * ch)
{
	OBJ_DATA *obj;
	int i;

	//if (ROOM_IDENTITY(ch->in_room) == DEAD_SEA)
	//	return (1);

	if (IS_IMMORTAL(ch))
		return (TRUE);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_WATERWALK))
		return (TRUE);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_WATERBREATH))
		return (TRUE);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
		return (TRUE);

	// non-wearable boats in inventory will do it
	for (obj = ch->carrying; obj; obj = obj->get_next_content())
	{
		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_BOAT
			&& (find_eq_pos(ch, obj, NULL) < 0))
		{
			return TRUE;
		}
	}

	// and any boat you're wearing will do it too
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i)
			&& GET_OBJ_TYPE(GET_EQ(ch, i)) == OBJ_DATA::ITEM_BOAT)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void make_visible(CHAR_DATA * ch, const EAffectFlag affect)
{
	char to_room[MAX_STRING_LENGTH], to_char[MAX_STRING_LENGTH];

	*to_room = *to_char = 0;

	switch (affect)
	{
	case EAffectFlag::AFF_HIDE:
		strcpy(to_char, "Вы прекратили прятаться.\r\n");
		strcpy(to_room, "$n прекратил$g прятаться.\r\n");
		break;

	case EAffectFlag::AFF_CAMOUFLAGE:
		strcpy(to_char, "Вы прекратили маскироваться.\r\n");
		strcpy(to_room, "$n прекратил$g маскироваться.\r\n");
		break;

	default:
		break;
	}
	AFF_FLAGS(ch).unset(affect);
	CHECK_AGRO(ch) = TRUE;
	if (*to_char)
		send_to_char(to_char, ch);
	if (*to_room)
		act(to_room, FALSE, ch, 0, 0, TO_ROOM);
}


int skip_hiding(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob;

	if (MAY_SEE(ch, vict, ch) && (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE) || affected_by_spell(ch, SPELL_HIDE)))
	{
		if (awake_hide(ch))  	//if (affected_by_spell(ch, SPELL_HIDE))
		{
			send_to_char("Вы попытались спрятаться, но ваша экипировка выдала вас.\r\n", ch);
			affect_from_char(ch, SPELL_HIDE);
			make_visible(ch, EAffectFlag::AFF_HIDE);
			EXTRA_FLAGS(ch).set(EXTRA_FAILHIDE);
		}
		else if (affected_by_spell(ch, SPELL_HIDE))
		{
			percent = number(1, 82 + GET_REAL_INT(vict));
			prob = calculate_skill(ch, SKILL_HIDE, vict);
			if (percent > prob)
			{
				affect_from_char(ch, SPELL_HIDE);
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE))
				{
					improove_skill(ch, SKILL_HIDE, FALSE, vict);
					act("Вы не сумели остаться незаметным.", FALSE, ch, 0, vict, TO_CHAR);
				}
			}
			else
			{
				improove_skill(ch, SKILL_HIDE, TRUE, vict);
				act("Вам удалось остаться незаметным.\r\n", FALSE, ch, 0, vict, TO_CHAR);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

int skip_camouflage(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob;

	if (MAY_SEE(ch, vict, ch)
		&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
			|| affected_by_spell(ch, SPELL_CAMOUFLAGE)))
	{
		if (awake_camouflage(ch))  	//if (affected_by_spell(ch,SPELL_CAMOUFLAGE))
		{
			send_to_char("Вы попытались замаскироваться, но ваша экипировка выдала вас.\r\n", ch);
			affect_from_char(ch, SPELL_CAMOUFLAGE);
			make_visible(ch, EAffectFlag::AFF_CAMOUFLAGE);
			EXTRA_FLAGS(ch).set(EXTRA_FAILCAMOUFLAGE);
		}
		else if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
		{
			percent = number(1, 82 + GET_REAL_INT(vict));
			prob = calculate_skill(ch, SKILL_CAMOUFLAGE, vict);
			if (percent > prob)
			{
				affect_from_char(ch, SPELL_CAMOUFLAGE);
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE))
				{
					improove_skill(ch, SKILL_CAMOUFLAGE, FALSE, vict);
					act("Вы не сумели правильно замаскироваться.", FALSE, ch, 0, vict, TO_CHAR);
				}
			}
			else
			{
				improove_skill(ch, SKILL_CAMOUFLAGE, TRUE, vict);
				act("Ваша маскировка оказалась на высоте.\r\n", FALSE, ch, 0, vict, TO_CHAR);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}


int skip_sneaking(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob, absolute_fail;
	bool try_fail;

	if (MAY_SEE(ch, vict, ch) && (AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK) || affected_by_spell(ch, SPELL_SNEAK)))
	{
		if (awake_sneak(ch))  	//if (affected_by_spell(ch,SPELL_SNEAK))
		{
			send_to_char("Вы попытались подкрасться, но ваша экипировка выдала вас.\r\n", ch);
			affect_from_char(ch, SPELL_SNEAK);
			if (affected_by_spell(ch, SPELL_HIDE))
				affect_from_char(ch, SPELL_HIDE);
			make_visible(ch, EAffectFlag::AFF_SNEAK);
			EXTRA_FLAGS(ch).get(EXTRA_FAILSNEAK);
		}
		else if (affected_by_spell(ch, SPELL_SNEAK))
		{
			//if (can_use_feat(ch, STEALTHY_FEAT)) //тать или наем
				//percent = number(1, 140 + GET_REAL_INT(vict));
			//else
			percent = number(1, (can_use_feat(ch, STEALTHY_FEAT) ? 102 : 112) + (GET_REAL_INT(vict) * (vict->get_role(MOB_ROLE_BOSS) ? 3 : 1)) + (GET_LEVEL(vict) > 30 ? GET_LEVEL(vict) : 0));
			prob = calculate_skill(ch, SKILL_SNEAK, vict);

			int catch_level = (GET_LEVEL(vict) - GET_LEVEL(ch));
			if (catch_level > 5)
			{
			//5% шанс фэйла при prob==200 всегда, при prob = 100 - 10%, если босс, шанс множим на 5
			absolute_fail = ((200 - prob) / 20 + 5)*(vict->get_role(MOB_ROLE_BOSS) ? 5 : 1 );
			try_fail = number(1, 100) < absolute_fail;
			}
			else 
				try_fail = false;


			if ((percent > prob) || try_fail)
			{
				affect_from_char(ch, SPELL_SNEAK);
				if (affected_by_spell(ch, SPELL_HIDE))
					affect_from_char(ch, SPELL_HIDE);
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK))
				{
					improove_skill(ch, SKILL_SNEAK, FALSE, vict);
					act("Вы не сумели пробраться незаметно.", FALSE, ch, 0, vict, TO_CHAR);
				}
			}
			else
			{
				improove_skill(ch, SKILL_SNEAK, TRUE, vict);
				act("Вам удалось прокрасться незаметно.\r\n", FALSE, ch, 0, vict, TO_CHAR);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
/*
 * Check for special routines (North is 1 in command list, but 0 here) Note
 * -- only check if following; this avoids 'double spec-proc' bug
 */

int real_forest_paths_sect(int sect)
{
	switch (sect)
	{
	case SECT_FOREST:
		return SECT_FIELD;
		break;
	case SECT_FOREST_SNOW:
		return SECT_FIELD_SNOW;
		break;
	case SECT_FOREST_RAIN:
		return SECT_FIELD_RAIN;
		break;
	}
	return sect;
}

int real_mountains_paths_sect(int sect)
{
	switch (sect)
	{
	case SECT_HILLS:
	case SECT_MOUNTAIN:
		return SECT_FIELD;
		break;
	case SECT_HILLS_RAIN:
		return SECT_FIELD_RAIN;
		break;
	case SECT_HILLS_SNOW:
	case SECT_MOUNTAIN_SNOW:
		return SECT_FIELD_SNOW;
		break;
	}
	return sect;
}

int calculate_move_cost(CHAR_DATA* ch, int dir)
{
	// move points needed is avg. move loss for src and destination sect type
	auto ch_inroom = real_sector(ch->in_room);
	auto ch_toroom = real_sector(EXIT(ch, dir)->to_room());

	if (can_use_feat(ch, FOREST_PATHS_FEAT))
	{
		ch_inroom = real_forest_paths_sect(ch_inroom);
		ch_toroom = real_forest_paths_sect(ch_toroom);
	}

	if (can_use_feat(ch, MOUNTAIN_PATHS_FEAT))
	{
		ch_inroom = real_mountains_paths_sect(ch_inroom);
		ch_toroom = real_mountains_paths_sect(ch_toroom);
	}

	int need_movement = (IS_FLY(ch) || on_horse(ch)) ? 1 :
		(movement_loss[ch_inroom] + movement_loss[ch_toroom]) / 2;

	if (IS_IMMORTAL(ch))
		need_movement = 0;
	else if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
		need_movement += CAMOUFLAGE_MOVES;
	else if (affected_by_spell(ch, SPELL_SNEAK))
		need_movement += SNEAK_MOVES;

	return need_movement;
}

int legal_dir(CHAR_DATA * ch, int dir, int need_specials_check, int show_msg)
{
	buf2[0] = '\0';
	if (need_specials_check && special(ch, dir + 1, buf2, 1))
		return (FALSE);

	if (!CAN_GO(ch, dir))
		return (FALSE);

	// не пускать в ванрумы после пк, если его там прибьет сразу
	if (DeathTrap::check_tunnel_death(ch, EXIT(ch, dir)->to_room()))
	{
		if (show_msg)
		{
			send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
		}
		return (FALSE);
	}

	// charmed
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& ch->has_master()
		&& ch->in_room == ch->get_master()->in_room)
	{
		if (show_msg)
		{
			send_to_char("Вы не можете покинуть свой идеал.\r\n", ch);
			act("$N попытал$U покинуть вас.", FALSE, ch->get_master(), 0, ch, TO_CHAR);
		}
		return (FALSE);
	}

	// check NPC's
	if (IS_NPC(ch))
	{
		if (GET_DEST(ch) != NOWHERE)
		{
			return (TRUE);
		}

		//  if this room or the one we're going to needs a boat, check for one */
		if (!MOB_FLAGGED(ch, MOB_SWIMMING)
			&& !MOB_FLAGGED(ch, MOB_FLYING)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_FLY)
			&& (real_sector(ch->in_room) == SECT_WATER_NOSWIM
				|| real_sector(EXIT(ch, dir)->to_room()) == SECT_WATER_NOSWIM))
		{
			if (!has_boat(ch))
			{
				return (FALSE);
			}
		}

		// Добавляем проверку на то что моб может вскрыть дверь
		if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) &&
				!MOB_FLAGGED(ch, MOB_OPENDOOR))
			return (FALSE);

		if (!MOB_FLAGGED(ch, MOB_FLYING) &&
				!AFF_FLAGGED(ch, EAffectFlag::AFF_FLY) && SECT(EXIT(ch, dir)->to_room()) == SECT_FLYING)
			return (FALSE);

		if (MOB_FLAGGED(ch, MOB_ONLYSWIMMING) &&
				!(real_sector(EXIT(ch, dir)->to_room()) == SECT_WATER_SWIM ||
				  real_sector(EXIT(ch, dir)->to_room()) == SECT_WATER_NOSWIM ||
				  real_sector(EXIT(ch, dir)->to_room()) == SECT_UNDERWATER))
			return (FALSE);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_NOMOB) &&
				!IS_HORSE(ch) &&
				!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !(MOB_FLAGGED(ch, MOB_ANGEL)||MOB_FLAGGED(ch, MOB_GHOST)) && !MOB_FLAGGED(ch, MOB_IGNORNOMOB))
			return (FALSE);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_DEATH) && !IS_HORSE(ch))
			return (FALSE);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_GODROOM))
			return (FALSE);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_NOHORSE) && IS_HORSE(ch))
			return (FALSE);
	}
	else
	{
		//Вход в замок
		if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM))
		{
			if (!Clan::MayEnter(ch, EXIT(ch, dir)->to_room(), HCE_ATRIUM))
			{
				if (show_msg)
					send_to_char("Частная собственность! Вход воспрещен!\r\n", ch);
				return (FALSE);
			}
		}

		if (real_sector(ch->in_room) == SECT_WATER_NOSWIM ||
				real_sector(EXIT(ch, dir)->to_room()) == SECT_WATER_NOSWIM)
		{
			if (!has_boat(ch))
			{
				if (show_msg)
					send_to_char("Вам нужна лодка, чтобы попасть туда.\r\n", ch);
				return (FALSE);
			}
		}
		if (real_sector(EXIT(ch, dir)->to_room()) == SECT_FLYING
			&& !IS_GOD(ch)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
		{
			if (show_msg)
			{
				send_to_char("Туда можно только влететь.\r\n", ch);
			}
			return (FALSE);
		}

		// если там ДТ и чар верхом на пони
		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_DEATH) && on_horse(ch))
		{
		    if (show_msg)
		    {
			// я весьма костоязычен, исправьте кто-нибудь на нормальную
			// мессагу, антуражненькую
			send_to_char("Ваш скакун не хочет идти туда.\r\n", ch);
		    }
		    return (FALSE);
		}

		const auto need_movement = calculate_move_cost(ch, dir);
		if (GET_MOVE(ch) < need_movement)
		{
			if (need_specials_check
				&& ch->has_master())
			{
				if (show_msg)
				{
					send_to_char("Вы слишком устали, чтобы следовать туда.\r\n", ch);
				}
			}
			else
			{
				if (show_msg)
				{
					send_to_char("Вы слишком устали.\r\n", ch);
				}
			}
			return (FALSE);
		}
		//Вход в замок
		if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM))
		{
			if (!Clan::MayEnter(ch, EXIT(ch, dir)->to_room(), HCE_ATRIUM))
			{
				if (show_msg)
					send_to_char("Частная собственность! Вход воспрещен!\r\n", ch);
				return (FALSE);
			}
		}

		//чтобы конь не лез в комнату с флагом !лошадь
		if (on_horse(ch)
				&& !legal_dir(get_horse(ch), dir, need_specials_check, FALSE))
		{
			if (show_msg)
			{
				act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
					FALSE, ch, 0, get_horse(ch), TO_CHAR);
				act("$n соскочил$g с $N1.", FALSE, ch, 0, get_horse(ch), TO_ROOM | TO_ARENA_LISTEN);
				AFF_FLAGS(ch).unset(EAffectFlag::AFF_HORSE);
			}
		}
		//проверка на ванрум: скидываем игрока с коня, если там незанято
		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_TUNNEL) &&
				(num_pc_in_room((world[EXIT(ch, dir)->to_room()])) > 0))
		{
			if (show_msg)
				send_to_char("Слишком мало места.\r\n", ch);
			return (FALSE);
		}

		if (on_horse(ch) && GET_HORSESTATE(get_horse(ch)) <= 0)
		{
			if (show_msg)
				act("$Z $N загнан$G настолько, что не может нести вас на себе.",
					FALSE, ch, 0, get_horse(ch), TO_CHAR);
			return (FALSE);
		}

		if (on_horse(ch)
			&& (AFF_FLAGGED(get_horse(ch), EAffectFlag::AFF_HOLD)
				|| AFF_FLAGGED(get_horse(ch), EAffectFlag::AFF_SLEEP)))
		{
			if (show_msg)
				act("$Z $N не в состоянии нести вас на себе.\r\n", FALSE, ch, 0, get_horse(ch),	TO_CHAR);
			return (FALSE);
		}

		if(on_horse(ch)
			&& (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_TUNNEL)
				|| ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_NOHORSE)))
		{
			if (show_msg)
				act("$Z $N не в состоянии пройти туда.\r\n", FALSE, ch, 0, get_horse(ch), TO_CHAR);
			return FALSE;
		}

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_GODROOM) && !IS_GRGOD(ch))
		{
			if (show_msg)
				send_to_char("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return (FALSE);
		}

		for (const auto tch : world[ch->in_room]->people)
		{
			if (!IS_NPC(tch))
				continue;
			if (NPC_FLAGGED(tch, 1 << dir)
				&& AWAKE(tch)
				&& GET_POS(tch) > POS_SLEEPING
				&& CAN_SEE(tch, ch)
				&& !AFF_FLAGGED(tch, EAffectFlag::AFF_CHARM)
				&& !AFF_FLAGGED(tch, EAffectFlag::AFF_HOLD)
				&& !IS_GRGOD(ch))
			{
				if (show_msg)
				{
					act("$N преградил$G вам путь.", FALSE, ch, 0, tch, TO_CHAR);
				}

				return FALSE;
			}
		}
	}

	return TRUE;
}

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

#define MAX_DRUNK_SONG 6
const char *drunk_songs[MAX_DRUNK_SONG] = { "\"Шумел камыш, и-к-к..., деревья гнулися\"",
		"\"Куда ты, тропинка, меня завела\"",
		"\"Бабам, пара пабабам\"",
		"\"А мне любое море по колено\"",
		"\"Не жди меня мама, хорошего сына\"",
		"\"Бываааали дниии, весеееелыя\"",
										  };

#define MAX_DRUNK_VOICE 5
const char *drunk_voice[MAX_DRUNK_VOICE] = { " - затянул$g $n",
		" - запел$g $n.",
		" - прохрипел$g $n.",
		" - зычно заревел$g $n.",
		" - разухабисто протянул$g $n.",
										   };

int check_drunk_move(CHAR_DATA* ch, int direction, bool need_specials_check)
{
	if (!IS_NPC(ch)
		&& GET_COND(ch, DRUNK) >= CHAR_MORTALLY_DRUNKED
		&& !on_horse(ch)
		&& GET_COND(ch, DRUNK) >= number(CHAR_DRUNKED, 50))
	{
		int dirs[NUM_OF_DIRS];
		int correct_dirs = 0;

		for (auto i = 0; i < NUM_OF_DIRS; ++i)
		{
			if (legal_dir(ch, i, need_specials_check, TRUE))
			{
				dirs[correct_dirs] = i;
				++correct_dirs;
			}
		}

		if (correct_dirs > 0
			&& !bernoulli_trial(std::pow((1.0 - static_cast<double>(correct_dirs) / NUM_OF_DIRS), NUM_OF_DIRS)))
		{
			return dirs[number(0, correct_dirs - 1)];
		}
	}

	return direction;
}

int do_simple_move(CHAR_DATA * ch, int dir, int need_specials_check, CHAR_DATA * leader, bool is_flee)
{
	struct track_data *track;
	room_rnum was_in, go_to;
	int i, invis = 0, use_horse = 0, is_horse = 0, direction = 0;
	int mob_rnum = -1;
	CHAR_DATA *horse = NULL;

	// Mortally drunked - it is loss direction
	if (!IS_NPC(ch) && !leader)
	{
		const auto drunk_move = check_drunk_move(ch, dir, need_specials_check);
		if (dir != drunk_move)
		{
			sprintf(buf, "Ваши ноги не хотят слушаться вас...\r\n");
			send_to_char(buf, ch);
			dir = drunk_move;
		}

		if (!ch->get_fighting() && number(10, 24) < GET_COND(ch, DRUNK))
		{
			sprintf(buf, "%s", drunk_songs[number(0, MAX_DRUNK_SONG - 1)]);
			send_to_char(buf, ch);
			send_to_char("\r\n", ch);
			strcat(buf, drunk_voice[number(0, MAX_DRUNK_VOICE - 1)]);
			act(buf, FALSE, ch, 0, 0, TO_ROOM | CHECK_DEAF);
			affect_from_char(ch, SPELL_SNEAK);
			affect_from_char(ch, SPELL_CAMOUFLAGE);
		}
	}

	if (!legal_dir(ch, dir, need_specials_check, TRUE))
	    return (FALSE);

	if (!entry_mtrigger(ch))
		return (FALSE);

	if (ch->purged())
		return FALSE;

	if (!enter_wtrigger(world[EXIT(ch, dir)->to_room()], ch, dir))
		return (FALSE);

	if (ch->purged())
		return FALSE;

	// Now we know we're allow to go into the room.
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch))
		GET_MOVE(ch) -= calculate_move_cost(ch, dir);

	i = skill_info[SKILL_SNEAK].max_percent;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK) && !is_flee)
	{
		if (IS_NPC(ch))
			invis = 1;
		else if (awake_sneak(ch))
		{
			affect_from_char(ch, SPELL_SNEAK);
		}
		else
			if (!affected_by_spell(ch, SPELL_SNEAK) || calculate_skill(ch, SKILL_SNEAK, 0) >= number(1, i))
				invis = 1;
	}

	i = skill_info[SKILL_CAMOUFLAGE].max_percent;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE) && !is_flee)
	{
		if (IS_NPC(ch))
			invis = 1;
		else if (awake_camouflage(ch))
		{
			affect_from_char(ch, SPELL_CAMOUFLAGE);
		}
		else
			if (!affected_by_spell(ch, SPELL_CAMOUFLAGE) ||
					calculate_skill(ch, SKILL_CAMOUFLAGE, 0) >= number(1, i))
				invis = 1;
	}

	if (!is_flee)
	{
		sprintf(buf, "Вы поплелись %s%s.", leader ? "следом за $N4 " : "", DirsTo[dir]);
		act(buf, FALSE, ch, 0, leader, TO_CHAR);
	}

	was_in = ch->in_room;
	go_to = world[was_in]->dir_option[dir]->to_room();
	direction = dir + 1;
	use_horse = AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE)
		&& has_horse(ch, FALSE)
		&& (IN_ROOM(get_horse(ch)) == was_in
			|| IN_ROOM(get_horse(ch)) == go_to);
	is_horse = IS_HORSE(ch)
		&& ch->has_master()
		&& !AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_INVISIBLE)
		&& (IN_ROOM(ch->get_master()) == was_in
			|| IN_ROOM(ch->get_master()) == go_to);

	if (!invis && !is_horse)
	{
		if (is_flee)
			strcpy(buf1, "сбежал$g");
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVERUN))
			strcpy(buf1, "убежал$g");
		else if ((!use_horse && AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
			|| (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEFLY)))
		{
			strcpy(buf1, "улетел$g");
		}
		else if (IS_NPC(ch)
			&& NPC_FLAGGED(ch, NPC_MOVESWIM)
			&& (real_sector(was_in) == SECT_WATER_SWIM
				|| real_sector(was_in) == SECT_WATER_NOSWIM
				|| real_sector(was_in) == SECT_UNDERWATER))
		{
			strcpy(buf1, "уплыл$g");
		}
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEJUMP))
			strcpy(buf1, "ускакал$g");
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVECREEP))
			strcpy(buf1, "уполз$q");
		else if (real_sector(was_in) == SECT_WATER_SWIM
			|| real_sector(was_in) == SECT_WATER_NOSWIM
			|| real_sector(was_in) == SECT_UNDERWATER)
		{
			strcpy(buf1, "уплыл$g");
		}
		else if (use_horse)
		{
			CHAR_DATA *horse = get_horse(ch);
			if (horse && AFF_FLAGGED(horse, EAffectFlag::AFF_FLY))
			{
				strcpy(buf1, "улетел$g");
			}
			else
			{
				strcpy(buf1, "уехал$g");
			}
		}
		else
			strcpy(buf1, "уш$y");

		if (is_flee && !IS_NPC(ch) && can_use_feat(ch, WRIGGLER_FEAT))
			sprintf(buf2, "$n %s.", buf1);
		else
			sprintf(buf2, "$n %s %s.", buf1, DirsTo[dir]);
		act(buf2, TRUE, ch, 0, 0, TO_ROOM);
	}

	if (invis && !is_horse)
	{
		act("Кто-то тихо удалился отсюда.", TRUE, ch, 0, 0, TO_ROOM_HIDE);
	}

	if (on_horse(ch)) // || has_horse(ch, TRUE))
		horse = get_horse(ch);

	// Если сбежали, и по противнику никто не бьет, то убираем с него аттаку
	if (is_flee)
	{
		stop_fighting(ch, TRUE);
	}

	// track improovment
	if (!IS_NPC(ch) && IS_BITS(ch->track_dirs, dir))
	{
		send_to_char("Вы двинулись по следу.\r\n", ch);
		improove_skill(ch, SKILL_TRACK, TRUE, 0);
	}

	char_from_room(ch);
	//затычка для бегства. чтоьы не отрабатывал MSDP протокол
	if (is_flee && !IS_NPC(ch) && !can_use_feat(ch, CALMNESS_FEAT))
		char_flee_to_room(ch, go_to);
	else	
		char_to_room(ch, go_to);
	if (horse)
	{
		GET_HORSESTATE(horse) -= 1;
		char_from_room(horse);
		char_to_room(horse, go_to);
	}

	if (PRF_FLAGGED(ch, PRF_BLIND))
	{
		for (int i = 0; i < NUM_OF_DIRS; i++)
		{
			if (CAN_GO(ch, i)
				|| (EXIT(ch, i)
					&& EXIT(ch, i)->to_room() != NOWHERE))
			{
				const auto& rdata = EXIT(ch, i);
				if (ROOM_FLAGGED(rdata->to_room(), ROOM_DEATH))
				{
					send_to_char("\007", ch);
				}
			}
	    }
	}

	if (!invis && !is_horse)
	{
		if (is_flee
			|| (IS_NPC(ch)
				&& NPC_FLAGGED(ch, NPC_MOVERUN)))
		{
			strcpy(buf1, "прибежал$g");
		}
		else if ((!use_horse && AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
			|| (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEFLY)))
		{
			strcpy(buf1, "прилетел$g");
		}
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVESWIM)
			&& (real_sector(go_to) == SECT_WATER_SWIM
				|| real_sector(go_to) == SECT_WATER_NOSWIM
				|| real_sector(go_to) == SECT_UNDERWATER))
		{
			strcpy(buf1, "приплыл$g");
		}
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEJUMP))
			strcpy(buf1, "прискакал$g");
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVECREEP))
			strcpy(buf1, "приполз$q");
		else if (real_sector(go_to) == SECT_WATER_SWIM
			|| real_sector(go_to) == SECT_WATER_NOSWIM
			|| real_sector(go_to) == SECT_UNDERWATER)
		{
			strcpy(buf1, "приплыл$g");
		}
		else if (use_horse)
		{
			CHAR_DATA *horse = get_horse(ch);
			if (horse && AFF_FLAGGED(horse, EAffectFlag::AFF_FLY))
			{
				strcpy(buf1, "прилетел$g");
			}
			else
			{
				strcpy(buf1, "приехал$g");
			}

		}
		else
			strcpy(buf1, "приш$y");

		//log("%s-%d",GET_NAME(ch),ch->in_room);
		sprintf(buf2, "$n %s %s.", buf1, DirsFrom[dir]);
		//log(buf2);
		act(buf2, TRUE, ch, 0, 0, TO_ROOM);
		//log("ACT OK !");
	};

	if (invis && !is_horse)
	{
		act("Кто-то тихо подкрался сюда.", TRUE, ch, 0, 0, TO_ROOM_HIDE);
	}

	if (ch->desc != NULL)
		look_at_room(ch, 0);

	if (!IS_NPC(ch))
		room_affect_process_on_entry(ch, ch->in_room);

	if (DeathTrap::check_death_trap(ch))
	{
		if (horse)
		{
			extract_char(horse, FALSE);
		}
		return (FALSE);
	}

	if (check_death_ice(go_to, ch))
	{
		return (FALSE);
	}

	if (DeathTrap::tunnel_damage(ch))
	{
		return (FALSE);
	}

	greet_mtrigger(ch, dir);
	greet_otrigger(ch, dir);

	// add track info
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_NOTRACK)
		&& (!IS_NPC(ch)
			|| (mob_rnum = GET_MOB_RNUM(ch)) >= 0))
	{
		for (track = world[go_to]->track; track; track = track->next)
		{
			if ((IS_NPC(ch) && IS_SET(track->track_info, TRACK_NPC) && track->who == mob_rnum)
				|| (!IS_NPC(ch) && !IS_SET(track->track_info, TRACK_NPC) && track->who == GET_IDNUM(ch)))
			{
				break;
			}
		}

		if (!track && !ROOM_FLAGGED(go_to, ROOM_NOTRACK))
		{
			CREATE(track, 1);
			track->track_info = IS_NPC(ch) ? TRACK_NPC : 0;
			track->who = IS_NPC(ch) ? mob_rnum : GET_IDNUM(ch);
			track->next = world[go_to]->track;
			world[go_to]->track = track;
		}

		if (track)
		{
			SET_BIT(track->time_income[Reverse[dir]], 1);
			if (affected_by_spell(ch, SPELL_LIGHT_WALK) && !on_horse(ch))
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_LIGHT_WALK))
					track->time_income[Reverse[dir]] <<= number(15, 30);
			REMOVE_BIT(track->track_info, TRACK_HIDE);
		}

		for (track = world[was_in]->track; track; track = track->next)
			if ((IS_NPC(ch) && IS_SET(track->track_info, TRACK_NPC)
					&& track->who == mob_rnum) || (!IS_NPC(ch)
													&& !IS_SET(track->track_info, TRACK_NPC)
													&& track->who == GET_IDNUM(ch)))
				break;

		if (!track && !ROOM_FLAGGED(was_in, ROOM_NOTRACK))
		{
			CREATE(track, 1);
			track->track_info = IS_NPC(ch) ? TRACK_NPC : 0;
			track->who = IS_NPC(ch) ? mob_rnum : GET_IDNUM(ch);
			track->next = world[was_in]->track;
			world[was_in]->track = track;
		}
		if (track)
		{
			SET_BIT(track->time_outgone[dir], 1);
			if (affected_by_spell(ch, SPELL_LIGHT_WALK) && !on_horse(ch))
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_LIGHT_WALK))
					track->time_outgone[dir] <<= number(15, 30);
			REMOVE_BIT(track->track_info, TRACK_HIDE);
		}
	}

	// hide improovment
	if (IS_NPC(ch))
	{
		for (const auto vict : world[ch->in_room]->people)
		{
			if (!IS_NPC(vict))
			{
				skip_hiding(vict, ch);
			}
		}
	}

	income_mtrigger(ch, direction - 1);

	if (ch->purged())
		return FALSE;

	// char income, go mobs action
	if (!IS_NPC(ch))
	{
		for (const auto vict : world[ch->in_room]->people)
		{
			if (!IS_NPC(vict))
			{
				continue;
			}

			if (!CAN_SEE(vict, ch)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
				|| vict->get_fighting()
				|| GET_POS(vict) < POS_RESTING)
			{
				continue;
			}

			// AWARE mobs
			if (MOB_FLAGGED(vict, MOB_AWARE)
				&& GET_POS(vict) < POS_FIGHTING
				&& !AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				&& GET_POS(vict) > POS_SLEEPING)
			{
				act("$n поднял$u.", FALSE, vict, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				GET_POS(vict) = POS_STANDING;
			}
		}
	}

	// If flee - go agressive mobs
	if (!IS_NPC(ch)
		&& is_flee)
	{
		do_aggressive_room(ch, FALSE);
	}

	return direction;
}

int perform_move(CHAR_DATA *ch, int dir, int need_specials_check, int checkmob, CHAR_DATA *master)
{
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BANDAGE))
	{
		send_to_char("Перевязка была прервана!\r\n", ch);
		affect_from_char(ch, SPELL_BANDAGE);
	}
	ch->set_motion(true);

	room_rnum was_in;
	struct follow_type *k, *next;

	if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || ch->get_fighting())
		return FALSE;
	else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == NOWHERE)
		send_to_char("Вы не сможете туда пройти...\r\n", ch);
	else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
	{
		if (EXIT(ch, dir)->keyword)
		{
			sprintf(buf2, "Закрыто (%s).\r\n", EXIT(ch, dir)->keyword);
			send_to_char(buf2, ch);
		}
		else
			send_to_char("Закрыто.\r\n", ch);
	}
	else
	{
		if (!ch->followers)
		{
			if (!do_simple_move(ch, dir, need_specials_check, master, false))
				return (FALSE);
		}
		else
		{
			was_in = ch->in_room;
			// When leader mortally drunked - he change direction
			// So returned value set to FALSE or DIR + 1
			if (!(dir = do_simple_move(ch, dir, need_specials_check, master, false)))
				return (FALSE);

			--dir;
			for (k = ch->followers; k && k->follower->get_master(); k = next)
			{
				next = k->next;
				if (k->follower->in_room == was_in
					&& !k->follower->get_fighting()
					&& HERE(k->follower)
					&& !GET_MOB_HOLD(k->follower)
					&& AWAKE(k->follower)
					&& (IS_NPC(k->follower)
						|| (!PLR_FLAGGED(k->follower, PLR_MAILING)
							&& !PLR_FLAGGED(k->follower, PLR_WRITING)))
					&& (!IS_HORSE(k->follower)
						|| !AFF_FLAGGED(k->follower, EAffectFlag::AFF_TETHERED)))
				{
					if (GET_POS(k->follower) < POS_STANDING)
					{
						if (IS_NPC(k->follower)
							&& IS_NPC(k->follower->get_master())
							&& GET_POS(k->follower) > POS_SLEEPING
							&& !GET_WAIT(k->follower))
						{
							act("$n поднял$u.", FALSE, k->follower, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
							GET_POS(k->follower) = POS_STANDING;
						}
						else
						{
							continue;
						}
					}
//                   act("Вы поплелись следом за $N4.",FALSE,k->follower,0,ch,TO_CHAR);
					perform_move(k->follower, dir, 1, FALSE, ch);
				}
			}
		}
		if (checkmob)
		{
			do_aggressive_room(ch, TRUE);
		}
		return (TRUE);
	}
	return (FALSE);
}

void do_move(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int subcmd)
{
	/*
	 * This is basically a mapping of cmd numbers to perform_move indices.
	 * It cannot be done in perform_move because perform_move is called
	 * by other functions which do not require the remapping.
	 */
	perform_move(ch, subcmd - 1, 0, TRUE, 0);
}

void do_hidemove(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int dir = 0, sneaking = affected_by_spell(ch, SPELL_SNEAK);

	skip_spaces(&argument);
	if (!ch->get_skill(SKILL_SNEAK))
	{
		send_to_char("Вы не умеете этого.\r\n", ch);
		return;
	}

	if (!*argument)
	{
		send_to_char("И куда это вы направляетесь?\r\n", ch);
		return;
	}

	if ((dir = search_block(argument, dirs, FALSE)) < 0 && (dir = search_block(argument, DirIs, FALSE)) < 0)
	{
		send_to_char("Неизвестное направление.\r\n", ch);
		return;
	}
	if (on_horse(ch))
	{
		act("Вам мешает $N.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}
	if (!sneaking)
	{
		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_SNEAK;
		af.location = EApplyLocation::APPLY_NONE;
		af.modifier = 0;
		af.duration = 1;
		const int calculated_skill = calculate_skill(ch, SKILL_SNEAK, 0);
		const int chance = number(1, skill_info[SKILL_SNEAK].max_percent);
		af.bitvector = (chance < calculated_skill) ? to_underlying(EAffectFlag::AFF_SNEAK) : 0;
		af.battleflag = 0;
		affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
	}
	perform_move(ch, dir, 0, TRUE, 0);
	if (!sneaking || affected_by_spell(ch, SPELL_GLITTERDUST))
	{
		affect_from_char(ch, SPELL_SNEAK);
	}
}

#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
			(EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS(ch, door)	((EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))

#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_BROKEN(ch, obj, door)	((obj) ? \
	(OBJVAL_FLAGGED(obj, CONT_BROKEN)) :\
	(EXIT_FLAGGED(EXIT(ch, door), EX_BROKEN)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
	(OBJVAL_FLAGGED(obj, CONT_PICKPROOF) || OBJVAL_FLAGGED(obj, CONT_BROKEN)) : \
	(EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF) || EXIT_FLAGGED(EXIT(ch, door), EX_BROKEN)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 1)) : \
					(EXIT(ch, door)->exit_info))
#define DOOR_LOCK_COMPLEX(ch, obj, door) ((obj) ? \
			(GET_OBJ_VAL(obj,3)) :\
			(EXIT(ch, door)->lock_complexity))

int find_door(CHAR_DATA* ch, const char *type, char *dir, const char* /*cmdname*/)
{
	int door;

	if (*dir)  //Указано направление (второй аргумент)
	{
		//Проверяем соответствует ли аргумент английским или русским направлениям
		if ((door = search_block(dir, dirs, FALSE)) == -1 && (door = search_block(dir, DirIs, FALSE)) == -1)  	// Partial Match
		{
			//strcpy(doorbuf,"Уточните направление.\r\n");
			return (-1); //НЕВЕРНОЕ НАПРАВЛЕНИЕ
		}
		if (EXIT(ch, door)) //Проверяем есть ли такая дверь в указанном направлении
		{
			if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword) //Дверь как-то по-особенному называется?
			{
				if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword))
					//Первый аргумент соответствует именительному или винительному алиасу двери
					return (door);
				else
				{
					return (-2); //НЕ ПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
				}
			}
			else if (is_abbrev(type, "дверь") || is_abbrev(type, "door"))
				//Аргумент соответствует "дверь" или "door" и есть в указанном направлении
				return (door);
			else
				//Дверь с названием "дверь" есть, но аргумент не соответствует
				return (-2);
		}
		else
		{
			return (-3); //В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРЕЙ
		}
	}
	else //Направления не указано, ищем дверь по названию
	{
		if (!*type) //Названия не указано
		{
			return (-4); //НЕ УКАЗАНО АРГУМЕНТОВ
		}
		for (door = 0; door < NUM_OF_DIRS; door++) //Проверяем все направления, не найдется ли двери?
		{
			if (EXIT(ch, door)) //Есть выход в этом направлении
			{
				if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword) //Дверь как-то по-особенному называется?
				{
					if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword))
						//Аргумент соответствует имени этой двери
						return (door);
				}
				else if (DOOR_IS(ch, door) && (is_abbrev(type, "дверь") || is_abbrev(type, "door")))
					//Дверь не имеет особых алиасов, аргумент соответствует двери
					return (door);
			}
		}
		return (-5); //НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ БЕЗ УКАЗАНИЯ НАПРАВЛЕНИЯ
	}
}

int has_key(CHAR_DATA * ch, obj_vnum key)
{
	for (auto o = ch->carrying; o; o = o->get_next_content())
	{
		if (GET_OBJ_VNUM(o) == key && key != -1)
		{
			return (TRUE);
		}
	}

	if (GET_EQ(ch, WEAR_HOLD))
	{
		if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key && key != -1)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}



#define NEED_OPEN	(1 << 0)
#define NEED_CLOSED	(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED	(1 << 3)

const char *cmd_door[] =
{
	"открыл$g",
	"закрыл$g",
	"отпер$q",
	"запер$q",
	"взломал$g"
};

const char *a_cmd_door[] =
{
	"открыть",
	"закрыть",
	"отпереть",
	"запереть",
	"взломать"
};

const int flags_door[] =
{
	NEED_CLOSED | NEED_UNLOCKED,
	NEED_OPEN,
	NEED_CLOSED | NEED_LOCKED,
	NEED_CLOSED | NEED_UNLOCKED,
	NEED_CLOSED | NEED_LOCKED
};


#define EXITN(room, door)		(world[room]->dir_option[door])

inline void OPEN_DOOR(const room_rnum room, OBJ_DATA* obj, const int door)
{
	if (obj)
	{
		auto v = obj->get_val(1);
		TOGGLE_BIT(v, CONT_CLOSED);
		obj->set_val(1, v);
	}
	else
	{
		TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED);
	}
}

inline void LOCK_DOOR(const room_rnum room, OBJ_DATA* obj, const int door)
{
	if (obj)
	{
		auto v = obj->get_val(1);
		TOGGLE_BIT(v, CONT_LOCKED);
		obj->set_val(1, v);
	}
	else
	{
		TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED);
	}
}

// для кейсов
extern std::vector<_case> cases;;
void do_doorcmd(CHAR_DATA * ch, OBJ_DATA * obj, int door, int scmd)
{
	int other_room = 0;
	int r_num, vnum;
	int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
	char local_buf[MAX_STRING_LENGTH]; // глобальный buf в тригах переписывается
	// объект, который выпадает из сундука
	sprintf(local_buf, "$n %s ", cmd_door[scmd]);
//  if (IS_NPC(ch))
//     log("MOB DOOR Moving:Моб %s %s дверь в комнате %d",GET_NAME(ch),cmd_door[scmd],GET_ROOM_VNUM(ch->in_room));

	ROOM_DATA::exit_data_ptr back;
	if (!obj && ((other_room = EXIT(ch, door)->to_room()) != NOWHERE))
	{
		back = world[other_room]->dir_option[rev_dir[door]];
		if (back)
		{
			if ((back->to_room() != ch->in_room) ||
				((EXITDATA(ch->in_room, door)->exit_info
					^ EXITDATA(other_room, rev_dir[door])->exit_info)
					& (EX_ISDOOR | EX_CLOSED | EX_LOCKED)))
			{
				back.reset();
			}
		}
	}

	switch (scmd)
	{
	case SCMD_OPEN:
	case SCMD_CLOSE:
		if (scmd == SCMD_OPEN && obj && !open_otrigger(obj, ch, FALSE))
			return;
		if (scmd == SCMD_CLOSE && obj && !close_otrigger(obj, ch, FALSE))
			return;
		if (scmd == SCMD_OPEN && !obj && !open_wtrigger(world[ch->in_room], ch, door, FALSE))
			return;
		if (scmd == SCMD_CLOSE && !obj && !close_wtrigger(world[ch->in_room], ch, door, FALSE))
			return;
		if (scmd == SCMD_OPEN && !obj && back && !open_wtrigger(world[other_room], ch, rev_dir[door], FALSE))
			return;
		if (scmd == SCMD_CLOSE && !obj && back && !close_wtrigger(world[other_room], ch, rev_dir[door], FALSE))
			return;
		OPEN_DOOR(ch->in_room, obj, door);
		if (back)
		{
			OPEN_DOOR(other_room, obj, rev_dir[door]);
		}
		// вываливание и пурж кошелька
		if (obj && system_obj::is_purse(obj))
		{
			sprintf(buf, "<%s> {%d} открыл трупный кошелек %s.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), get_name_by_unique(GET_OBJ_VAL(obj, 3)));
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
			system_obj::process_open_purse(ch, obj);
			return;
		}
		else
		{
			send_to_char(OK, ch);
		}
		break;

	case SCMD_UNLOCK:
	case SCMD_LOCK:
		if (scmd == SCMD_UNLOCK && obj && !open_otrigger(obj, ch, TRUE))
			return;
		if (scmd == SCMD_LOCK && obj && !close_otrigger(obj, ch, TRUE))
			return;
		if (scmd == SCMD_UNLOCK && !obj && !open_wtrigger(world[ch->in_room], ch, door, TRUE))
			return;
		if (scmd == SCMD_LOCK && !obj && !close_wtrigger(world[ch->in_room], ch, door, TRUE))
			return;
		if (scmd == SCMD_UNLOCK && !obj && back && !open_wtrigger(world[other_room], ch, rev_dir[door], TRUE))
			return;
		if (scmd == SCMD_LOCK && !obj && back && !close_wtrigger(world[other_room], ch, rev_dir[door], TRUE))
			return;
		LOCK_DOOR(ch->in_room, obj, door);
		if (back)
			LOCK_DOOR(other_room, obj, rev_dir[door]);
		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
		{
			send_to_char("*Щелк*\r\n", ch);
			if (obj)
			{
				for (unsigned long i = 0; i < cases.size(); i++)
				{
					if (GET_OBJ_VNUM(obj) == cases[i].vnum)
					{
						send_to_char("&GГде-то далеко наверху раздалась звонкая музыка.&n\r\n", ch);
						// chance = cases[i].chance;		
						// chance пока что не учитывается, просто падает одна рандомная стафина из всего этого
						const int maximal_chance = static_cast<int>(cases[i].vnum_objs.size() - 1);
						const int random_number = number(0, maximal_chance);
						vnum = cases[i].vnum_objs[random_number];
						if ((r_num = real_object(vnum)) < 0)
						{
							send_to_char("Ошибка с номером 1, пожалуйста, напишите об этом в воззвать.\r\n", ch);
							return;
						}
						// сначала удалим ключ из инвентаря
						int vnum_key = GET_OBJ_VAL(obj, 2);
						// первый предмет в инвентаре
						OBJ_DATA *obj_inv = ch->carrying;
						OBJ_DATA *i;
						for (i = obj_inv; i; i = i->get_next_content())
						{
							if (GET_OBJ_VNUM(i) == vnum_key)
							{
								extract_obj(i);
								break;
							}
						}
						extract_obj(obj);
						obj = world_objects.create_from_prototype_by_rnum(r_num).get();
						obj->set_crafter_uid(GET_UNIQUE(ch));
						obj_to_char(obj, ch);
						act("$n завизжал$g от радости.", FALSE, ch, 0, 0, TO_ROOM);
						load_otrigger(obj);
						obj_decay(obj);
						olc_log("%s load obj %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), vnum);
						return;
					}
				}
			}
		}
		break;

	case SCMD_PICK:
		if (obj && !pick_otrigger(obj, ch))
			return;
		if (!obj && !pick_wtrigger(world[ch->in_room], ch, door))
			return;
		if (!obj && back && !pick_wtrigger(world[other_room], ch, rev_dir[door]))
			return;
		LOCK_DOOR(ch->in_room, obj, door);
		if (back)
			LOCK_DOOR(other_room, obj, rev_dir[door]);
		send_to_char("Замок очень скоро поддался под вашим натиском.\r\n", ch);
		strcpy(local_buf, "$n умело взломал$g ");
		break;
	}

	// Notify the room
	sprintf(local_buf + strlen(local_buf), "%s.", (obj) ? "$p" : (EXIT(ch, door)->vkeyword ? "$F" : "дверь"));
	if (!obj || (obj->get_in_room() != NOWHERE))
	{
		act(local_buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->vkeyword, TO_ROOM);
	}

	// Notify the other room
	if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back)
	{
		const auto& people = world[EXIT(ch, door)->to_room()]->people;
		if (!people.empty())
		{
			sprintf(local_buf + strlen(local_buf) - 1, " с той стороны.");
			int allowed_items_remained = 1000;
			for (const auto to : people)
			{
				if (0 == allowed_items_remained)
				{
					break;
				}

				perform_act(local_buf, ch, obj, obj ? 0 : EXIT(ch, door)->vkeyword, to);

				--allowed_items_remained;
			}
		}
	}
}

int ok_pick(CHAR_DATA* ch, obj_vnum /*keynum*/, OBJ_DATA* obj, int door, int scmd)
{
	int percent;
	int pickproof = DOOR_IS_PICKPROOF(ch, obj, door);
	percent = number(1, skill_info[SKILL_PICK_LOCK].max_percent);

	if (scmd == SCMD_PICK)
	{
		if (pickproof)
			send_to_char("Вы никогда не сможете взломать ЭТО.\r\n", ch);
		else if (!check_moves(ch, PICKLOCK_MOVES));
		else if (DOOR_LOCK_COMPLEX(ch, obj, door) - ch->get_skill(SKILL_PICK_LOCK) > 10)//Polud очередной magic number...
		//если скилл меньше сложности на 10 и более - даже трениться на таком замке нельзя
			send_to_char("С таким сложным замком даже и пытаться не следует...\r\n", ch);
		else if ((ch->get_skill(SKILL_PICK_LOCK) - DOOR_LOCK_COMPLEX(ch, obj, door) <= 10)  && //если скилл больше сложности на 10 и более - даже трениться на таком замке нельзя
			(percent > train_skill(ch, SKILL_PICK_LOCK, skill_info[SKILL_PICK_LOCK].max_percent, NULL)))
			send_to_char("Взломщик из вас пока еще никудышний.\r\n", ch);
		else if (get_pick_chance(ch->get_skill(SKILL_PICK_LOCK), DOOR_LOCK_COMPLEX(ch, obj, door)) < number(1,10))
		{
			send_to_char("Вы все-таки сломали этот замок...\r\n", ch);
			if (obj)
			{
				auto v = obj->get_val(1);
				SET_BIT(v, CONT_BROKEN);
				obj->set_val(1, v);
			}
			if (door > -1)
			{
				SET_BIT(EXIT(ch, door)->exit_info, EX_BROKEN);
			}
		}
		else
			return (1);
		return (0);
	}
	return (1);
}

void do_gen_door(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	int door = -1;
	obj_vnum keynum;
	char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
	OBJ_DATA *obj = NULL;
	CHAR_DATA *victim = NULL;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Очнитесь, вы же слепы!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_PICK && !ch->get_skill(SKILL_PICK_LOCK))
	{
		send_to_char("Это умение вам недоступно.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	if (!*argument)
	{
		sprintf(buf, "%s что?\r\n", a_cmd_door[subcmd]);
		send_to_char(CAP(buf), ch);
		return;
	}
	two_arguments(argument, type, dir);

	if (isname(dir, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(dir, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(dir, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	//Сначала ищем дверь, считая второй аргумент указанием на сторону света
	door = find_door(ch, type, dir, a_cmd_door[subcmd]);
	//Если двери не нашлось, проверяем объекты в экипировке, инвентаре, на земле
	if (door < 0)
		if (!generic_find(type, where_bits, ch, &victim, &obj))
		{
			//Если и объектов не нашлось, выдаем одно из сообщений об ошибке
			switch (door)
			{
			case -1: //НЕВЕРНОЕ НАПРАВЛЕНИЕ
				send_to_char("Уточните направление.\r\n",ch);
				break;
			case -2: //НЕ ПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
				sprintf(buf, "Вы не видите '%s' в этой комнате.\r\n", type);
				send_to_char(buf, ch);
				break;
			case -3: //В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРЕЙ
				sprintf(buf, "Вы не можете это '%s'.\r\n", a_cmd_door[subcmd]);
				send_to_char(buf, ch);
				break;
			case -4: //НЕ УКАЗАНО АРГУМЕНТОВ
				sprintf(buf, "Что вы хотите '%s'?\r\n", a_cmd_door[subcmd]);
				send_to_char(buf, ch);
				break;
			case -5: //НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ БЕЗ УКАЗАНИЯ НАПРАВЛЕНИЯ
				sprintf(buf, "Вы не видите здесь '%s'.\r\n", type);
				send_to_char(buf, ch);
				break;
			}
			return;
		}
	if ((obj) || (door >= 0))
	{
		if ((obj) && !IS_IMMORTAL(ch) && (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NAMED)) && NamedStuff::check_named(ch, obj, true))//Именной предмет открывать(закрывать) может только владелец
		{
			if (!NamedStuff::wear_msg(ch, obj))
				send_to_char("Просьба не трогать! Частная собственность!\r\n", ch);
			return;
		}
		keynum = DOOR_KEY(ch, obj, door);
		if ((subcmd == SCMD_CLOSE || subcmd == SCMD_LOCK) && !IS_NPC(ch) && RENTABLE(ch))
			send_to_char("Ведите себя достойно во время боевых действий!\r\n", ch);
		else if (!(DOOR_IS_OPENABLE(ch, obj, door)))
			act("Вы никогда не сможете $F это!", FALSE, ch, 0, a_cmd_door[subcmd], TO_CHAR);
		else if (!DOOR_IS_OPEN(ch, obj, door)
				 && IS_SET(flags_door[subcmd], NEED_OPEN))
			send_to_char("Вообще-то здесь закрыто!\r\n", ch);
		else if (!DOOR_IS_CLOSED(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_CLOSED))
			send_to_char("Уже открыто!\r\n", ch);
		else if (!(DOOR_IS_LOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_LOCKED))
			send_to_char("Да отперли уже все...\r\n", ch);
		else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED))
			send_to_char("Угу, заперто.\r\n", ch);
		else if (!has_key(ch, keynum) && !Privilege::check_flag(ch, Privilege::USE_SKILLS) && ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			send_to_char("У вас нет ключа.\r\n", ch);
		else if (DOOR_IS_BROKEN(ch, obj, door) && !Privilege::check_flag(ch, Privilege::USE_SKILLS) && ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			send_to_char("Замок сломан.\r\n", ch);
		else if (ok_pick(ch, keynum, obj, door, subcmd))
			do_doorcmd(ch, obj, door, subcmd);
	}
	return;
}

void do_enter(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int door, from_room;
	const char *p_str = "пентаграмма";
	struct follow_type *k, *k_next;

	one_argument(argument, buf);

	if (*buf)
//     {if (!str_cmp("пентаграмма",buf))
	{
		if (isname(buf, p_str))
		{
			if (!world[ch->in_room]->portal_time)
				send_to_char("Вы не видите здесь пентаграмму.\r\n", ch);
			else
			{
				from_room = ch->in_room;
				door = world[ch->in_room]->portal_room;
				// не пускать игрока на холженном коне
				if (on_horse(ch) && GET_MOB_HOLD(get_horse(ch)))
				{
					act("$Z $N не в состоянии нести вас на себе.\r\n",
						FALSE, ch, 0, get_horse(ch), TO_CHAR);
					return;
				}
				// не пускать в ванрумы после пк, если его там прибьет сразу
				if (DeathTrap::check_tunnel_death(ch, door))
				{
					send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
					return;
				}
				// Если чар под местью, и портал односторонний, то не пускать
				if (RENTABLE(ch) && !IS_NPC(ch) && !world[door]->portal_time)
				{
					send_to_char("Грехи мешают вам воспользоваться вратами.\r\n", ch);
					return;
				}
				//проверка на флаг нельзя_верхом
				if (ROOM_FLAGGED(door, ROOM_NOHORSE) && on_horse(ch))
				{
					act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
						FALSE, ch, 0, get_horse(ch), TO_CHAR);
					act("$n соскочил$g с $N1.", FALSE, ch, 0, get_horse(ch), TO_ROOM | TO_ARENA_LISTEN);
					AFF_FLAGS(ch).unset(EAffectFlag::AFF_HORSE);
				}
				//проверка на ванрум и лошадь
				if (ROOM_FLAGGED(door, ROOM_TUNNEL) &&
						(num_pc_in_room(world[door]) > 0 || on_horse(ch)))
				{
					if (num_pc_in_room(world[door]) > 0)
					{
						send_to_char("Слишком мало места.\r\n", ch);
						return;
					}
					else
					{
						act("$Z $N заупрямил$U, и вам пришлось соскочить.",
							FALSE, ch, 0, get_horse(ch), TO_CHAR);
						act("$n соскочил$g с $N1.", FALSE, ch, 0, get_horse(ch), TO_ROOM | TO_ARENA_LISTEN);
						AFF_FLAGS(ch).unset(EAffectFlag::AFF_HORSE);
					}
				}
				// Обработка флагов NOTELEPORTIN и NOTELEPORTOUT здесь же
				if (!IS_IMMORTAL(ch) && ((!IS_NPC(ch) && (!Clan::MayEnter(ch, door, HCE_PORTAL)
											|| (GET_LEVEL(ch) <= 10 && world[door]->portal_time)))
											|| (ROOM_FLAGGED(from_room, ROOM_NOTELEPORTOUT)
												|| ROOM_FLAGGED(door, ROOM_NOTELEPORTIN))
											|| AFF_FLAGGED(ch, EAffectFlag::AFF_NOTELEPORT)
											|| (world[door]->pkPenterUnique && (ROOM_FLAGGED(door, ROOM_ARENA) || ROOM_FLAGGED(door, ROOM_HOUSE)))
											))
				{
					sprintf(buf, "%sПентаграмма ослепительно вспыхнула!%s\r\n",
							CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
					act(buf, TRUE, ch, 0, 0, TO_CHAR);
					act(buf, TRUE, ch, 0, 0, TO_ROOM);

					send_to_char("Мощным ударом вас отшвырнуло от пентаграммы.\r\n", ch);
					act("$n с визгом отлетел$g от пентаграммы.\r\n", TRUE, ch,
						0, 0, TO_ROOM | CHECK_DEAF);
					act("$n отлетел$g от пентаграммы.\r\n", TRUE, ch, 0, 0, TO_ROOM | CHECK_NODEAF);
					WAIT_STATE(ch, PULSE_VIOLENCE);
					return;
				}
				act("$n исчез$q в пентаграмме.", TRUE, ch, 0, 0, TO_ROOM);
				if (world[from_room]->pkPenterUnique && world[from_room]->pkPenterUnique != GET_UNIQUE(ch) && !IS_IMMORTAL(ch))
				{
					send_to_char(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
						CCIRED(ch, C_NRM), CCINRM(ch, C_NRM));
					pkPortal(ch);
				}
				char_from_room(ch);
				char_to_room(ch, door);
				set_wait(ch, 3, FALSE);
				act("$n появил$u из пентаграммы.", TRUE, ch, 0, 0, TO_ROOM);
				// ищем ангела и лошадь
				for (k = ch->followers; k; k = k_next)
				{
					k_next = k->next;
					if (IS_HORSE(k->follower) &&
							!k->follower->get_fighting() &&
							!GET_MOB_HOLD(k->follower) &&
							IN_ROOM(k->follower) == from_room && AWAKE(k->follower))
					{
						if (!ROOM_FLAGGED(door, ROOM_NOHORSE))
						{
							char_from_room(k->follower);
							char_to_room(k->follower, door);
						}
					}
					if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
						&& !GET_MOB_HOLD(k->follower)
						&& (MOB_FLAGGED(k->follower, MOB_ANGEL)||MOB_FLAGGED(k->follower, MOB_GHOST))
						&& !k->follower->get_fighting()
						&& IN_ROOM(k->follower) == from_room
						&& AWAKE(k->follower))
					{
						act("$n исчез$q в пентаграмме.", TRUE,
							k->follower, 0, 0, TO_ROOM);
						char_from_room(k->follower);
						char_to_room(k->follower, door);
						set_wait(k->follower, 3, FALSE);
						act("$n появил$u из пентаграммы.", TRUE,
							k->follower, 0, 0, TO_ROOM);
					}
					if (IS_CHARMICE(k->follower) &&
							!GET_MOB_HOLD(k->follower) &&
							GET_POS(k->follower) == POS_STANDING &&
							IN_ROOM(k->follower) == from_room)
					{
						snprintf(buf2, MAX_STRING_LENGTH, "войти пентаграмма");
						command_interpreter(k->follower, buf2);
					}
				}
				if (ch->desc != NULL)
					look_at_room(ch, 0);
			}
		}
		else
		{	// an argument was supplied, search for door keyword
			for (door = 0; door < NUM_OF_DIRS; door++)
			{
				if (EXIT(ch, door)
					&& (isname(buf, EXIT(ch, door)->keyword)
						|| isname(buf, EXIT(ch, door)->vkeyword)))
				{
					perform_move(ch, door, 1, TRUE, 0);
					return;
				}
			}
			sprintf(buf2, "Вы не нашли здесь '%s'.\r\n", buf);
			send_to_char(buf2, ch);
		}
	}
	else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
		send_to_char("Вы уже внутри.\r\n", ch);
	else  			// try to locate an entrance
	{
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room() != NOWHERE)
					if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
							ROOM_FLAGGED(EXIT(ch, door)->to_room(), ROOM_INDOORS))
					{
						perform_move(ch, door, 1, TRUE, 0);
						return;
					}
		send_to_char("Вы не можете найти вход.\r\n", ch);
	}
}

void do_stand(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (GET_POS(ch) > POS_SLEEPING && AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
	{
		send_to_char("Вы сладко зевнули и решили еще немного подремать.\r\n", ch);
		act("$n сладко зевнул$a и решил$a еще немного подремать.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SLEEPING;
	}

	if (on_horse(ch))
	{
		act("Прежде всего, вам стоит слезть с $N1.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}
	switch (GET_POS(ch))
	{
	case POS_STANDING:
		send_to_char("А вы уже стоите.\r\n", ch);
		break;
	case POS_SITTING:
		send_to_char("Вы встали.\r\n", ch);
		act("$n поднял$u.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		// Will be sitting after a successful bash and may still be fighting.
		GET_POS(ch) = ch->get_fighting() ? POS_FIGHTING : POS_STANDING;
		break;
	case POS_RESTING:
		send_to_char("Вы прекратили отдыхать и встали.\r\n", ch);
		act("$n прекратил$g отдых и поднял$u.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = ch->get_fighting() ? POS_FIGHTING : POS_STANDING;
		break;
	case POS_SLEEPING:
		send_to_char("Пожалуй, сначала стоит проснуться!\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Вы дрались лежа? Это что-то новенькое.\r\n", ch);
		break;
	default:
		send_to_char("Вы прекратили летать и опустились на грешную землю.\r\n", ch);
		act("$n опустил$u на землю.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_STANDING;
		break;
	}
}

void do_sit(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (on_horse(ch))
	{
		act("Прежде всего, вам стоит слезть с $N1.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}
	switch (GET_POS(ch))
	{
	case POS_STANDING:
		send_to_char("Вы сели.\r\n", ch);
		act("$n сел$g.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
		break;
	case POS_SITTING:
		send_to_char("А вы и так сидите.\r\n", ch);
		break;
	case POS_RESTING:
		send_to_char("Вы прекратили отдыхать и сели.\r\n", ch);
		act("$n прервал$g отдых и сел$g.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
		break;
	case POS_SLEEPING:
		send_to_char("Вам стоит проснуться.\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Сесть? Во время боя? Вы явно не в себе.\r\n", ch);
		break;
	default:
		send_to_char("Вы прекратили свой полет и сели.\r\n", ch);
		act("$n прекратил$g свой полет и сел$g.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
		break;
	}
}

void do_rest(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (on_horse(ch))
	{
		act("Прежде всего, вам стоит слезть с $N1.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}
	switch (GET_POS(ch))
	{
	case POS_STANDING:
		send_to_char("Вы присели отдохнуть.\r\n", ch);
		act("$n присел$g отдохнуть.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_RESTING;
		break;
	case POS_SITTING:
		send_to_char("Вы пристроились поудобнее для отдыха.\r\n", ch);
		act("$n пристроил$u поудобнее для отдыха.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_RESTING;
		break;
	case POS_RESTING:
		send_to_char("Вы и так отдыхаете.\r\n", ch);
		break;
	case POS_SLEEPING:
		send_to_char("Вам лучше сначала проснуться.\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Отдыха в бою вам не будет!\r\n", ch);
		break;
	default:
		send_to_char("Вы прекратили полет и присели отдохнуть.\r\n", ch);
		act("$n прекратил$g полет и пристроил$u поудобнее для отдыха.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
		break;
	}
}

void do_sleep(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		send_to_char("Не время вам спать, родина в опасности!\r\n", ch);
		return;
	}
	if (on_horse(ch))
	{
		act("Прежде всего, вам стоит слезть с $N1.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}
	switch (GET_POS(ch))
	{
	case POS_STANDING:
	case POS_SITTING:
	case POS_RESTING:
		send_to_char("Вы заснули.\r\n", ch);
		act("$n сладко зевнул$g и задал$g храпака.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SLEEPING;
		break;
	case POS_SLEEPING:
		send_to_char("А вы и так спите.\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Вам нужно сражаться! Отоспитесь после смерти.\r\n", ch);
		break;
	default:
		send_to_char("Вы прекратили свой полет и отошли ко сну.\r\n", ch);
		act("$n прекратил$g летать и нагло заснул$g.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SLEEPING;
		break;
	}
}

void do_horseon(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse;

	if (IS_NPC(ch))
		return;

	if (!get_horse(ch))
	{
		send_to_char("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		send_to_char("Не пытайтесь усидеть на двух стульях.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
		horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	else
		horse = get_horse(ch);

	if (horse == NULL)
		send_to_char(NOPERSON, ch);
	else if (IN_ROOM(horse) != ch->in_room)
		send_to_char("Ваш скакун далеко от вас.\r\n", ch);
	else if (!IS_HORSE(horse))
		send_to_char("Это не скакун.\r\n", ch);
	else if (horse->get_master() != ch)
		send_to_char("Это не ваш скакун.\r\n", ch);
	else if (GET_POS(horse) < POS_FIGHTING)
		act("$N не сможет вас нести в таком состоянии.", FALSE, ch, 0, horse, TO_CHAR);
	else if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
		act("Вам стоит отвязать $N3.", FALSE, ch, 0, horse, TO_CHAR);
	//чтоб не вскакивали в ванрумах
	else if (ROOM_FLAGGED(ch->in_room, ROOM_TUNNEL))
		send_to_char("Слишком мало места.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ROOM_NOHORSE))
		act("$Z $N взбрыкнул$G и отказал$U вас слушаться.", FALSE, ch, 0, horse, TO_CHAR);
	else
	{
		if (affected_by_spell(ch, SPELL_SNEAK))
			affect_from_char(ch, SPELL_SNEAK);
		if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
			affect_from_char(ch, SPELL_CAMOUFLAGE);
		act("Вы взобрались на спину $N1.", FALSE, ch, 0, horse, TO_CHAR);
		act("$n вскочил$g на $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
		AFF_FLAGS(ch).set(EAffectFlag::AFF_HORSE);
	}
}

void do_horseoff(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse;

	if (IS_NPC(ch))
		return;
	if (!(horse = get_horse(ch)))
	{
		send_to_char("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (!on_horse(ch))
	{
		send_to_char("Вы ведь и так не на лошади.", ch);
		return;
	}

	act("Вы слезли со спины $N1.", FALSE, ch, 0, horse, TO_CHAR);
	act("$n соскочил$g с $N1.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_HORSE);
}

void do_horseget(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse;

	if (IS_NPC(ch))
		return;

	if (!get_horse(ch))
	{
		send_to_char("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		send_to_char("Вы уже сидите на скакуне.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
		horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	else
		horse = get_horse(ch);

	if (horse == NULL)
		send_to_char(NOPERSON, ch);
	else if (IN_ROOM(horse) != ch->in_room)
		send_to_char("Ваш скакун далеко от вас.\r\n", ch);
	else if (!IS_HORSE(horse))
		send_to_char("Это не скакун.\r\n", ch);
	else if (horse->get_master() != ch)
		send_to_char("Это не ваш скакун.\r\n", ch);
	else if (!AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
		act("А $N и не привязан$A.", FALSE, ch, 0, horse, TO_CHAR);
	else
	{
		act("Вы отвязали $N3.", FALSE, ch, 0, horse, TO_CHAR);
		act("$n отвязал$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
		AFF_FLAGS(horse).unset(EAffectFlag::AFF_TETHERED);
	}
}

void do_horseput(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse;

	if (IS_NPC(ch))
		return;
	if (!get_horse(ch))
	{
		send_to_char("У вас нет скакуна.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		send_to_char("Вам стоит слезть со скакуна.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
		horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	else
		horse = get_horse(ch);
	if (horse == NULL)
		send_to_char(NOPERSON, ch);
	else if (IN_ROOM(horse) != ch->in_room)
		send_to_char("Ваш скакун далеко от вас.\r\n", ch);
	else if (!IS_HORSE(horse))
		send_to_char("Это не скакун.\r\n", ch);
	else if (horse->get_master() != ch)
		send_to_char("Это не ваш скакун.\r\n", ch);
	else if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
		act("А $N уже и так привязан$A.", FALSE, ch, 0, horse, TO_CHAR);
	else
	{
		act("Вы привязали $N3.", FALSE, ch, 0, horse, TO_CHAR);
		act("$n привязал$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
		AFF_FLAGS(horse).set(EAffectFlag::AFF_TETHERED);
	}
}

void do_horsetake(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse = NULL;

	if (IS_NPC(ch))
		return;

	if (get_horse(ch))
	{
		send_to_char("Зачем вам столько скакунов?\r\n", ch);
		return;
	}

	if (ch->is_morphed())
	{
		send_to_char("И как вы собираетесь это проделать без рук и без ног, одними лапами?\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (*arg)
	{
		horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	}

	if (horse == NULL)
	{
		send_to_char(NOPERSON, ch);
		return;
	}
	else if (!IS_NPC(horse))
	{
		send_to_char("Господи, не чуди...\r\n", ch);
		return;
	}
	// Исправил ошибку не дававшую воровать коняжек. -- Четырь (13.10.10)
	else if (!IS_GOD(ch)
		&& !MOB_FLAGGED(horse, MOB_MOUNTING)
		&& !(horse->has_master()
			&& AFF_FLAGGED(horse, EAffectFlag::AFF_HORSE)))
	{
		act("Вы не сможете оседлать $N3.", FALSE, ch, 0, horse, TO_CHAR);
		return;
	}
	else if (get_horse(ch))
	{
		if (get_horse(ch) == horse)
			act("Не стоит седлать $S еще раз.", FALSE, ch, 0, horse, TO_CHAR);
		else
			send_to_char("Вам не усидеть сразу на двух скакунах.\r\n", ch);
		return;
	}
	else if (GET_POS(horse) < POS_STANDING)
	{
		act("$N не сможет стать вашим скакуном.", FALSE, ch, 0, horse, TO_CHAR);
		return;
	}
	else if (IS_HORSE(horse))
	{
		if (!IS_IMMORTAL(ch))
		{
			send_to_char("Это не ваш скакун.\r\n", ch);
			return;
		}
	}
	if (stop_follower(horse, SF_EMPTY))
		return;
	act("Вы оседлали $N3.", FALSE, ch, 0, horse, TO_CHAR);
	act("$n оседлал$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
	make_horse(horse, ch);
}

void do_givehorse(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse, *victim;

	if (IS_NPC(ch))
		return;

	if (!(horse = get_horse(ch)))
	{
		send_to_char("Да нету у вас скакуна.\r\n", ch);
		return;
	}
	if (!has_horse(ch, TRUE))
	{
		send_to_char("Ваш скакун далеко от вас.\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Кому вы хотите передать скакуна?\r\n", ch);
		return;
	}
	if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
	{
		send_to_char("Вам некому передать скакуна.\r\n", ch);
		return;
	}
	else if (IS_NPC(victim))
	{
		send_to_char("Он и без этого обойдется.\r\n", ch);
		return;
	}
	if (get_horse(victim))
	{
		act("У $N1 уже есть скакун.\r\n", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}
	if (on_horse(ch))
	{
		send_to_char("Вам стоит слезть со скакуна.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
	{
		send_to_char("Вам стоит прежде отвязать своего скакуна.\r\n", ch);
		return;
	}
	// Долбанные умертвия при передаче рассыпаются и весело роняют мад на проходе по последователям чара -- Krodo
	if (stop_follower(horse, SF_EMPTY))
		return;
	act("Вы передали своего скакуна $N2.", FALSE, ch, 0, victim, TO_CHAR);
	act("$n передал$g вам своего скакуна.", FALSE, ch, 0, victim, TO_VICT);
	act("$n передал$g своего скакуна $N2.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	make_horse(horse, victim);
}

void do_stophorse(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *horse;

	if (IS_NPC(ch))
		return;

	if (!(horse = get_horse(ch)))
	{
		send_to_char("Да нету у вас скакуна.\r\n", ch);
		return;
	}
	if (!has_horse(ch, TRUE))
	{
		send_to_char("Ваш скакун далеко от вас.\r\n", ch);
		return;
	}
	if (on_horse(ch))
	{
		send_to_char("Вам стоит слезть со скакуна.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(horse, EAffectFlag::AFF_TETHERED))
	{
		send_to_char("Вам стоит прежде отвязать своего скакуна.\r\n", ch);
		return;
	}
	if (stop_follower(horse, SF_EMPTY))
		return;
	act("Вы отпустили $N3.", FALSE, ch, 0, horse, TO_CHAR);
	act("$n отпустил$g $N3.", FALSE, ch, 0, horse, TO_ROOM | TO_ARENA_LISTEN);
	if (GET_MOB_VNUM(horse) == HORSE_VNUM)
	{
		act("$n убежал$g в свою конюшню.\r\n", FALSE, horse, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		extract_char(horse, FALSE);
	}
}

void do_wake(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *vict;
	int self = 0;

	one_argument(argument, arg);

	if (subcmd == SCMD_WAKEUP)
	{
		if (!(*arg))
		{
			send_to_char("Кого будить то будем???\r\n", ch);
			return;
		}
	}
	else
	{
		*arg = 0;
	}

	if (*arg)
	{
		if (GET_POS(ch) == POS_SLEEPING)
			send_to_char("Может быть вам лучше проснуться?\r\n", ch);
		else if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)) == NULL)
			send_to_char(NOPERSON, ch);
		else if (vict == ch)
			self = 1;
		else if (AWAKE(vict))
			act("$E и не спал$G.", FALSE, ch, 0, vict, TO_CHAR);
		else if (GET_POS(vict) < POS_SLEEPING)
			act("$M так плохо! Оставьте $S в покое!", FALSE, ch, 0, vict, TO_CHAR);
		else
		{
			act("Вы $S разбудили.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n растолкал$g вас.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
			GET_POS(vict) = POS_SITTING;
		}
		if (!self)
			return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
		send_to_char("Вы не можете проснуться!\r\n", ch);
	else if (GET_POS(ch) > POS_SLEEPING)
		send_to_char("А вы и не спали...\r\n", ch);
	else
	{
		send_to_char("Вы проснулись и сели.\r\n", ch);
		act("$n проснул$u.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		GET_POS(ch) = POS_SITTING;
	}
}

void do_follow(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *leader;
	struct follow_type *f;
	one_argument(argument, buf);

	if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->get_fighting())
		return;
	if (*buf)
	{
		if (!str_cmp(buf, "я") || !str_cmp(buf, "self") || !str_cmp(buf, "me"))
		{
			if (!ch->has_master())
			{
				send_to_char("Но вы ведь ни за кем не следуете...\r\n", ch);
			}
			else
			{
				stop_follower(ch, SF_EMPTY);
			}
			return;
		}
		if (!(leader = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		{
			send_to_char(NOPERSON, ch);
			return;
		}
	}
	else
	{
		send_to_char("За кем вы хотите следовать?\r\n", ch);
		return;
	}

	if (ch->get_master() == leader)
	{
		act("Вы уже следуете за $N4.", FALSE, ch, 0, leader, TO_CHAR);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& ch->has_master())
	{
		act("Но вы можете следовать только за $N4!", FALSE, ch, 0, ch->get_master(), TO_CHAR);
	}
	else  		// Not Charmed follow person
	{
		if (leader == ch)
		{
			if (!ch->has_master())
			{
				send_to_char("Вы уже следуете за собой.\r\n", ch);
				return;
			}
			stop_follower(ch, SF_EMPTY);
		}
		else
		{
			if (circle_follow(ch, leader))
			{
				send_to_char("Так у вас целый хоровод получится.\r\n", ch);
				return;
			}

			if (ch->has_master())
			{
				stop_follower(ch, SF_EMPTY);
			}
			AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
			//also removing AFF_GROUP flag from all followers
			for (f = ch->followers; f; f = f->next)
			{
				AFF_FLAGS(f->follower).unset(EAffectFlag::AFF_GROUP);
			}

			leader->add_follower(ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
