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

extern int dts_are_dumps;
extern int mini_mud;

extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;

//F@N|
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
SPECIAL(gen_board);
void assign_kings_castle(void);
char *str_str(char *cs, char *ct);

/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
	mob_rnum rnum;

	if ((rnum = real_mobile(mob)) >= 0)
		mob_index[rnum].func = fname;
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

	if ((rnum = real_mobile(mob)) >= 0) {
		mob_index[rnum].func = fname;
		mob_index[rnum].stored = learn_info;
	} else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
//  assign_kings_castle();

	ASSIGNMOB(1, puff);

	/* HOTEL */
//Adept: пока закомментил мешающее - потом надо посмотреть какого оно утт вообще делает.
//	ASSIGNMOB(3005, receptionist);
	ASSIGNMOB(3122, receptionist);
	ASSIGNMOB(4022, receptionist);
	ASSIGNMOB(106, receptionist);

	/* POSTMASTER */
	ASSIGNMOB(3027, postmaster);
	ASSIGNMOB(3102, postmaster);
	ASSIGNMOB(4002, postmaster);

	/* BANK */
	ASSIGNMOB(3019, bank);
	ASSIGNMOB(3101, bank);
	ASSIGNMOB(4001, bank);

	/* HORSEKEEPER */
	ASSIGNMOB(3123, horse_keeper);
	ASSIGNMOB(4023, horse_keeper);
}



/* assign special procedures to objects */
void assign_objects(void)
{
	ASSIGNOBJ(250, gen_board);
	ASSIGNOBJ(251, gen_board);
	ASSIGNOBJ(252, gen_board);
	ASSIGNOBJ(253, gen_board);
	ASSIGNOBJ(254, gen_board);
	ASSIGNOBJ(255, gen_board);
	ASSIGNOBJ(256, gen_board);
	ASSIGNOBJ(257, gen_board);
	ASSIGNOBJ(258, gen_board);
	ASSIGNOBJ(259, gen_board);
	ASSIGNOBJ(267, gen_board);
	ASSIGNOBJ(12635, gen_board);
	ASSIGNOBJ(268, gen_board);
}



/* assign special procedures to rooms */
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

	if (!(magic = fopen(LIB_MISC "specials.lst", "r"))) {
		log("Cann't open specials list file...");
		return;
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %d %s", line1, &i, line2) != 3) {
			log("Bad format for special string !\r\n"
			    "Format : <who/what (%%s)> <vnum (%%d)> <type (%%s)>");
			_exit(1);
		}
		log("<%s>-%d-[%s]", line1, i, line2);
		if (!str_cmp(line1, "mob")) {
			if (real_mobile(i) < 0) {
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
//++F@N
			else if (!str_cmp(line2, "exchange"))
				ASSIGNMOB(i, exchange);
//--F@N
			else
				log("Unknown mobile %d assignment type - %s...", i, line2);
		} else if (!str_cmp(line1, "obj")) {
			if (real_object(i) < 0) {
				log("Unknown object %d in specials assignment...", i);
				continue;
			}
			if (!str_cmp(line2, "board"))
				ASSIGNOBJ(i, gen_board);
		} else if (!str_cmp(line1, "room")) {
		} else {
			log("Error in specials file !\r\n" "May be : mob, obj or room...");
			_exit(1);
		}
	}
	fclose(magic);
	return;
}
