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

#include "obj_prototypes.h"
#include "cmd/mercenary.h"
//#include "constants.h"
//#include "utils/logger.h"
#include "house.h"
#include "boards/boards_constants.h"
#include "boards/boards.h"
#include "entities/char_data.h"
#include "noob.h"

extern int dts_are_dumps;

extern IndexData *mob_index;

int dump(CharData *ch, void *me, int cmd, char *argument);
int puff(CharData *ch, void *me, int cmd, char *argument);

void assign_kings_castle(void);

// local functions
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);

typedef int special_f(CharData *, void *, int, char *);

void ASSIGNROOM(RoomVnum room, special_f);
void ASSIGNMOB(MobVnum mob, special_f);
void ASSIGNOBJ(ObjVnum obj, special_f);
void clear_mob_charm(CharData *mob);

// functions to perform assignments

void ASSIGNMOB(MobVnum mob, int fname(CharData *, void *, int, char *)) {
	MobRnum rnum;

	if ((rnum = real_mobile(mob)) >= 0) {
		mob_index[rnum].func = fname;
		// рентерам хардкодом снимаем возможные нежелательные флаги
		if (fname == receptionist) {
			clear_mob_charm(&mob_proto[rnum]);
		}
	} else {
		err_log("Attempt to assign spec to non-existant mob #%d", mob);
	}
}

void ASSIGNOBJ(ObjVnum obj, special_f fname) {
	const ObjRnum rnum = real_object(obj);

	if (rnum >= 0) {
		obj_proto.func(rnum, fname);
	} else {
		err_log("Attempt to assign spec to non-existant obj #%d", obj);
	}
}

void ASSIGNROOM(RoomVnum room, special_f fname) {
	const RoomRnum rnum = real_room(room);

	if (rnum != kNowhere) {
		world[rnum]->func = fname;
	} else {
		err_log("Attempt to assign spec to non-existant room #%d", room);
	}
}

void ASSIGNMASTER(MobVnum mob, special_f fname, int learn_info) {
	MobRnum rnum;

	if ((rnum = real_mobile(mob)) >= 0) {
		mob_index[rnum].func = fname;
		mob_index[rnum].stored = learn_info;
	} else {
		err_log("Attempt to assign spec to non-existant mob #%d", mob);
	}
}

// ********************************************************************
// *  Assignments                                                     *
// ********************************************************************

/**
* Спешиалы на мобов сюда писать не нужно, пишите в lib/misc/specials.lst,
* TODO: вообще убирать надо это тоже в конфиг, всеравно без конфигов мад
* не запустится, толку в коде держать даже этот минимальный набор.
*/
void assign_mobiles(void) {
	// HOTEL //
	ASSIGNMOB(106, receptionist);
	ASSIGNMOB(4022, receptionist);

	// POSTMASTER //
	ASSIGNMOB(4002, postmaster);

	// BANK //
	ASSIGNMOB(4001, bank);
	ASSIGNMOB(4001, mercenary);

	// HORSEKEEPER //
	ASSIGNMOB(4023, horse_keeper);
}

// assign special procedures to objects //
void assign_objects(void) {
	special_f *const function = Boards::Static::Special;
	ASSIGNOBJ(Boards::GODGENERAL_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GENERAL_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GODCODE_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GODPUNISH_BOARD_OBJ, function);
	ASSIGNOBJ(Boards::GODBUILD_BOARD_OBJ, function);
}

// assign special procedures to rooms //
void assign_rooms(void) {
	RoomRnum i;

	if (dts_are_dumps)
		for (i = kFirstRoom; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ERoomFlag::kDeathTrap))
				world[i]->func = dump;
}

void init_spec_procs(void) {
	FILE *magic;
	char line1[256], line2[256], name[256];
	int i;

	if (!(magic = fopen(LIB_MISC "specials.lst", "r"))) {
		log("Cann't open specials list file...");
		assign_mobiles();
		assign_objects();
		return;
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		if (sscanf(name, "%s %d %s", line1, &i, line2) != 3) {
			log("Bad format for special string!\r\n"
				"Format : <who/what (%%s)> <vnum (%%d)> <type (%%s)>");
			graceful_exit(1);
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
			else if (!str_cmp(line2, "exchange"))
				ASSIGNMOB(i, exchange);
			else if (!str_cmp(line2, "torc"))
				ASSIGNMOB(i, torc);
			else if (!str_cmp(line2, "outfit"))
				ASSIGNMOB(i, Noob::outfit);
			else if (!str_cmp(line2, "mercenary"))
				ASSIGNMOB(i, mercenary);
			else
				log("Unknown mobile %d assignment type - %s...", i, line2);
		} else if (!str_cmp(line1, "obj")) {
			if (real_object(i) < 0) {
				log("Unknown object %d in specials assignment...", i);
				continue;
			}
		} else if (!str_cmp(line1, "room")) {
		} else {
			log("Error in specials file!\r\n" "May be : mob, obj or room...");
			graceful_exit(1);
		}
	}
	fclose(magic);
	return;
}

// * Снятие нежелательных флагов у рентеров и продавцов.
void clear_mob_charm(CharData *mob) {
	if (mob && !mob->purged()) {
		MOB_FLAGS(mob).unset(EMobFlag::kMounting);
		MOB_FLAGS(mob).set(EMobFlag::kNoCharm);
		MOB_FLAGS(mob).set(EMobFlag::kNoResurrection);
		NPC_FLAGS(mob).unset(ENpcFlag::kHelped);
	} else {
		log("SYSERROR: mob = %s (%s:%d)",
			mob ? (mob->purged() ? "purged" : "true") : "false",
			__FILE__, __LINE__);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
