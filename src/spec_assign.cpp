/* ************************************************************************
*   File: spec_assign.cpp                               Part of Bylins    *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
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
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "utils.h"
#include "house.h"
#include "boards.h"
#include "char.hpp"
#include "room.hpp"
#include "noob.hpp"

extern int dts_are_dumps;
extern int mini_mud;

extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;

SPECIAL(exchange);
SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(guild_mono);
SPECIAL(guild_poly);
SPECIAL(horse_keeper);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(bank);
SPECIAL(torc);
SPECIAL(Noob::outfit);

void assign_kings_castle(void);

// local functions
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));
void clear_mob_charm(CHAR_DATA *mob);

// functions to perform assignments

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
	mob_rnum rnum;

	if ((rnum = real_mobile(mob)) >= 0)
	{
		mob_index[rnum].func = fname;
		// рентерам хардкодом снимаем возможные нежелательные флаги
		if (fname == receptionist)
		{
			clear_mob_charm(&mob_proto[rnum]);
		}
	}
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
	obj_rnum rnum;

	if ((rnum = real_object(obj)) >= 0)
		obj_index[rnum].func = fname;
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
	room_rnum rnum;

	if ((rnum = real_room(room)) != NOWHERE)
		world[rnum]->func = fname;
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

void ASSIGNMASTER(mob_vnum mob, SPECIAL(fname), int learn_info)
{
	mob_rnum rnum;

	if ((rnum = real_mobile(mob)) >= 0)
	{
		mob_index[rnum].func = fname;
		mob_index[rnum].stored = learn_info;
	}
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}


// ********************************************************************
// *  Assignments                                                     *
// ********************************************************************

/**
* Спешиалы на мобов сюда писать не нужно, пишите в lib/misc/specials.lst,
* TODO: вообще убирать надо это тоже в конфиг, всеравно без конфигов мад
* не запустится, толку в коде держать даже этот минимальный набор.
*/
void assign_mobiles(void)
{
	// HOTEL //
	ASSIGNMOB(106, receptionist);
	ASSIGNMOB(4022, receptionist);

	// POSTMASTER //
	ASSIGNMOB(4002, postmaster);

	// BANK //
	ASSIGNMOB(4001, bank);

	// HORSEKEEPER //
	ASSIGNMOB(4023, horse_keeper);
}



// assign special procedures to objects //
void assign_objects(void)
{
	ASSIGNOBJ(Boards::GODGENERAL_BOARD_OBJ, Board::Special);
	ASSIGNOBJ(Boards::GENERAL_BOARD_OBJ, Board::Special);
	ASSIGNOBJ(Boards::GODCODE_BOARD_OBJ, Board::Special);
	ASSIGNOBJ(Boards::GODPUNISH_BOARD_OBJ, Board::Special);
	ASSIGNOBJ(Boards::GODBUILD_BOARD_OBJ, Board::Special);
}



// assign special procedures to rooms //
void assign_rooms(void)
{
	room_rnum i;

	if (dts_are_dumps)
		for (i = FIRST_ROOM; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ROOM_DEATH))
				world[i]->func = dump;
}



void init_spec_procs(void)
{
	FILE *magic;
	char line1[256], line2[256], name[256];
	int i;

	if (!(magic = fopen(LIB_MISC "specials.lst", "r")))
	{
		log("Cann't open specials list file...");
		assign_mobiles();
		assign_objects();
		return;
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %d %s", line1, &i, line2) != 3)
		{
			log("Bad format for special string!\r\n"
				"Format : <who/what (%%s)> <vnum (%%d)> <type (%%s)>");
			_exit(1);
		}
		log("<%s>-%d-[%s]", line1, i, line2);
		if (!str_cmp(line1, "mob"))
		{
			if (real_mobile(i) < 0)
			{
				log("Unknown mobile %d in specials assignment...", i);
				continue;
			}
			if (!str_cmp(line2, "puff"))
				ASSIGNMOB(i, puff);
			else if (!str_cmp(line2, "rent"))
				ASSIGNMOB(i, receptionist);
			else if (!str_cmp(line2, "mail"))
				ASSIGNMOB(i, postmaster);
			else if (!str_cmp(line2, "bank"))
				ASSIGNMOB(i, bank);
			else if (!str_cmp(line2, "horse"))
				ASSIGNMOB(i, horse_keeper);
			else if (!str_cmp(line2, "exchange"))
				ASSIGNMOB(i, exchange);
			else if (!str_cmp(line2, "torc"))
				ASSIGNMOB(i, torc);
			else if (!str_cmp(line2, "outfit"))
				ASSIGNMOB(i, Noob::outfit);
			else
				log("Unknown mobile %d assignment type - %s...", i, line2);
		}
		else if (!str_cmp(line1, "obj"))
		{
			if (real_object(i) < 0)
			{
				log("Unknown object %d in specials assignment...", i);
				continue;
			}
		}
		else if (!str_cmp(line1, "room"))
		{
		}
		else
		{
			log("Error in specials file!\r\n" "May be : mob, obj or room...");
			_exit(1);
		}
	}
	fclose(magic);
	return;
}

// * Снятие нежелательных флагов у рентеров и продавцов.
void clear_mob_charm(CHAR_DATA *mob)
{
	if (mob && !mob->purged())
	{
		REMOVE_BIT(MOB_FLAGS(mob, MOB_MOUNTING), MOB_MOUNTING);
		SET_BIT(MOB_FLAGS(mob, MOB_NOCHARM), MOB_NOCHARM);
		SET_BIT(MOB_FLAGS(mob, MOB_NORESURRECTION), MOB_NORESURRECTION);
		REMOVE_BIT(NPC_FLAGS(mob, NPC_HELPED), NPC_HELPED);
	}
	else
	{
		log("SYSERROR: mob = %s (%s:%d)",
			mob ? (mob->purged() ? "purged" : "true") : "false",
			__FILE__, __LINE__);
	}
}
