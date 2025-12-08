/***************************************************************************
 *  File: dg_mobcmd.cpp                                    Part of Bylins  *
 *  Usage: functions for usage in mob triggers.                            *
 *                                                                         *
 *                                                                         *
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  $Author$                                                         *
 *  $Date$                                           *
 *  $Revision$                                                   *
 ***************************************************************************/

#include "engine/entities/char_data.h"
#include "engine/ui/cmd/do_follow.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/core/handler.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/skills/townportal.h"
#include "utils/id_converter.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/damage.h"

struct mob_command_info {
	const char *command;
	EPosition minimum_position;
	typedef void(*handler_f)(CharData *ch, char *argument, int cmd, int subcmd, Trigger *trig);
	handler_f command_pointer;
	int subcmd;                ///< Subcommand. See SCMD_* constants.
	bool use_in_stoped;
};

extern int reloc_target;
extern Trigger *cur_trig;

void sub_write(char *arg, CharData *ch, byte find_invis, int targets);
RoomData *get_room(const char *name);
ObjData *get_obj_by_char(CharData *ch, char *name);
// * Local functions.
void mob_command_interpreter(CharData *ch, char *argument, Trigger *trig);
bool mob_script_command_interpreter(CharData *ch, char *argument, Trigger *trig);
void send_to_zone(char *messg, int zone_rnum);

// attaches mob's name and vnum to msg_set and sends it to script_log
void mob_log(CharData *mob, Trigger *trig, const char *msg, LogMode type = LogMode::OFF) {
	char small_buf[kMaxInputLength + 100];

	snprintf(small_buf,kMaxInputLength + 100, "(Mob: '%s', VNum: %d, trig: %d): %s [строка: %d]", 
			GET_SHORT(mob), GET_MOB_VNUM(mob), trig_index[(trig)->get_rnum()]->vnum, msg, last_trig_line_num);
	script_log(small_buf, type);
}

//returns the real room number, or kNowhere if not found or invalid
//copy from find_target_room except char's messages
RoomRnum dg_find_target_room(CharData *ch, Trigger *trig,char *rawroomstr) {
	char roomstr[kMaxInputLength];
	RoomRnum location = kNowhere;

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		sprintf(buf, "Undefined mteleport room: %s", rawroomstr);
		mob_log(ch, trig, buf);
		return kNowhere;
	}

	const auto tmp = atoi(roomstr);
	if (tmp > 0) {
		location = GetRoomRnum(tmp);
	} else {
		sprintf(buf, "Undefined mteleport room: %s", roomstr);
		mob_log(ch, trig, buf);
		return kNowhere;
	}

	return location;
}

void do_mportal(CharData *mob, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	int target, howlong, curroom, nr;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		mob_log(mob, trig, "mportal: called with too few args");
		return;
	}

	howlong = atoi(arg2);
	nr = atoi(arg1);
	target = GetRoomRnum(nr);

	if (target == kNowhere) {
		mob_log(mob, trig, "mportal: target is an invalid room");
		return;
	}

	/* Ставим пентаграмму из текущей комнаты в комнату target с
	   длительностью howlong */
	curroom = mob->in_room;
	one_way_portal::ReplacePortalTimer(mob, curroom, target, howlong * 30 - 1);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), nullptr, nullptr, kToChar);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), nullptr, nullptr, kToRoom);
}
// prints the argument to all the rooms aroud the mobile
void do_masound(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (!*argument) {
		mob_log(ch, trig, "masound called with no argument");
		return;
	}

	skip_spaces(&argument);

	const int temp_in_room = ch->in_room;
	for (int door = 0; door < EDirection::kMaxDirNum; door++) {
		const auto &exit = world[temp_in_room]->dir_option[door];
		if (exit
			&& exit->to_room() != kNowhere
			&& exit->to_room() != temp_in_room) {
			ch->in_room = exit->to_room();
			sub_write(argument, ch, true, kToRoom);
		}
	}

	ch->in_room = temp_in_room;
}

// lets the mobile kill any player or mobile without murder
void do_mkill(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *victim;

	if (ch->IsFlagged(EMobFlag::kNoFight)) {
		mob_log(ch, trig, "mkill called for mob with NOFIGHT flag");
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, trig, "mkill called with no argument");
		return;
	}

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "mkill: victim (%s) not found, команда: %s", arg + 1, argument);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_room_vis(ch, arg))) {
		sprintf(buf, "mkill: victim (%s) not found, , команда: %s", arg, argument);
		mob_log(ch, trig, buf);
		return;
	}

	if (victim == ch) {
		mob_log(ch, trig, "mkill: victim is self");
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed) && ch->get_master() == victim) {
		mob_log(ch, trig, "mkill: charmed mob attacking master");
		return;
	}

	if (ch->GetEnemy()) {
		mob_log(ch, trig, "mkill: already fighting");
		return;
	}

	hit(ch, victim, ESkill::kUndefined, fight::kMainHand);
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
void do_mjunk(CharData *ch, char */*argument*/, int/* cmd*/, int/* subcmd*/, Trigger *) {
	int pos;
	ObjData *obj;
	ObjData *obj_next;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;
	for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
		obj_next = obj->get_next_content();
		ExtractObjFromWorld(obj, false);
	}
	for (pos = 0; pos < EEquipPos::kNumEquipPos; pos++) {
		obj = GET_EQ(ch, pos);
		if (obj) {
			UnequipChar(ch, pos, CharEquipFlags());
			world_objects.AddToExtractedList(obj);
		}
	}
}

// prints the message to everyone in the room other than the mob and victim
void do_mechoaround(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *victim;
	char *p;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	p = one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, trig, "mechoaround called with no argument");
		return;
	}
	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "mechoaround: victim (%s) UID does not exist, команда: %s", arg + 1, argument);
			mob_log(ch, trig, buf, LGH);
			return;
		}
	} else if (!(victim = get_char_room_vis(ch, arg))) {
		sprintf(buf, "mechoaround: victim (%s) does not exist, команда: %s", arg, argument);
		mob_log(ch, trig, buf, LGH);
		return;
	}

	if (reloc_target != -1 && reloc_target != victim->in_room) {
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	}

	sub_write(p, victim, true, kToRoom);
}

// sends the message to only the victim
void do_msend(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *victim;
	char *p;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	p = one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, trig, "msend called with no argument");
		return;
	}

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
// Надоел спам чармисов
//			sprintf(buf, "msend: victim (%s) UID does not exist", arg + 1);
//			mob_log(ch, buf, LGH);
			return;
		}
	} else if (!(victim = get_char_room_vis(ch, arg))) {
		sprintf(buf, "msend: victim (%s) does not exist, команда: %s", arg, argument);
		mob_log(ch, trig, buf, LGH);
		return;
	}

	if (reloc_target != -1 && reloc_target != victim->in_room) {
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	}

	sub_write(p, victim, true, kToChar);
}

// prints the message to the room at large
void do_mecho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char *p;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (!*argument) {
		mob_log(ch, trig, "mecho called with no arguments");
		return;
	}
	p = argument;
	skip_spaces(&p);

	if (reloc_target != -1 && reloc_target != ch->in_room) {
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	}

	sub_write(p, ch, true, kToRoom);
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
void do_mload(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	CharData *mob;
	char uid[kMaxInputLength], varname[kMaxInputLength] = "LoadedUid";
	char uid_type;
	int number = 0, idnum;

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		mob_log(ch, trig, "mload: попытка почармленным мобом загрузать моба/предмет.");
		return;
	}

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
		mob_log(ch, trig, "mload: bad syntax");
		return;
	}
	if (utils::IsAbbr(arg1, "mob")) {
		if ((mob = ReadMobile(number, kVirtual)) == nullptr) {
			mob_log(ch, trig, "mload: bad mob vnum");
			return;
		}
		log("Load mob #%d by %s (mload)", number, GET_NAME(ch));
		uid_type = UID_CHAR;
		idnum = mob->get_uid();
		PlaceCharToRoom(mob, ch->in_room);
		load_mtrigger(mob);
	} else if (utils::IsAbbr(arg1, "obj")) {
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object) {
			mob_log(ch, trig, "mload: bad object vnum");
			return;
		}
		if (GetObjMIW(object->get_rnum()) >= 0 && obj_proto.actual_count(object->get_rnum()) > GetObjMIW(object->get_rnum())) {
			if (!stable_objs::IsTimerUnlimited(obj_proto[object->get_rnum()].get())) {
				sprintf(buf, "mload: Попытка загрузить предмет больше чем в MIW для #%d.", number);
				mob_log(ch, trig, buf);
//				extract_obj(object.get());
//				return;
			}
		}
		log("Load obj #%d by %s (mload)", number, GET_NAME(ch));
		object->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));
		uid_type = UID_OBJ;
		idnum = object->get_id();
		if (CAN_WEAR(object.get(), EWearFlag::kTake)) {
			PlaceObjToInventory(object.get(), ch);
		} else {
			PlaceObjToRoom(object.get(), ch->in_room);
		}
		load_otrigger(object.get());
	} else {
		mob_log(ch, trig, "mload: bad type");
		return;
	}
	sprintf(uid, "%c%d", uid_type, idnum);
	add_var_cntx(trig->var_list, varname, uid, 0);
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
void do_mpurge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *victim;
	ObjData *obj;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		return;
	}

	if (*arg == UID_CHAR)
		victim = get_char(arg);
	else
		victim = get_char_room_vis(ch, arg);

	if (victim == nullptr) {
		if ((obj = get_obj_by_char(ch, arg))) {
			world_objects.AddToExtractedList(obj);
		} else {
			mob_log(ch, trig, "mpurge: bad argument");
		}
		return;
	}

	if (!victim->IsNpc()) {
		mob_log(ch, trig, "mpurge: purging a PC");
		return;
	}

	if (victim->followers
		|| victim->has_master()) {
		die_follower(victim);
	}
	if(ch == victim) {
		trig->halt();
	}
	character_list.AddToExtractedList(victim);
}

// lets the mobile goto any location it wishes that is not private
void do_mgoto(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	int location;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, trig, "mgoto called with no argument");
		return;
	}

	if ((location = dg_find_target_room(ch, trig, arg)) == kNowhere) {
		std::stringstream buffer;
		buffer << "mgoto: invalid location '" << arg << "'";
//		sprintf(buf, "mgoto: invalid location '%s'", arg);
		mob_log(ch, trig, buffer.str().c_str());
		return;
	}

	if (ch->GetEnemy())
		stop_fighting(ch, true);

	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
}

// lets the mobile do a command at another location. Very useful
void do_mat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	int location;
	int original;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	argument = one_argument(argument, arg);

	if (!*arg || !*argument) {
		mob_log(ch, trig, "mat: bad argument");
		return;
	}

	if ((location = dg_find_target_room(ch, trig, arg)) == kNowhere) {
		std::stringstream buffer;
		buffer << "mat: invalid location '" << arg << "'";
//		sprintf(buf, "mat: invalid location '%s'", arg);
		mob_log(ch, trig, buffer.str().c_str());
		return;
	}

	reloc_target = location;
	original = ch->in_room;
	ch->in_room = location;
	mob_command_interpreter(ch, argument, trig);
	ch->in_room = original;
	reloc_target = -1;
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
void do_mteleport(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	int target;
	CharData *vict, *lastchar = nullptr;
	bool onhorse = false;
	RoomRnum from_room;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;
	char *textstr = argument;
	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		sprintf(buf, "mteleport: bad syntax, arg: %s", textstr);
		mob_log(ch, trig, buf);
		return;
	}

	target = dg_find_target_room(ch, trig, arg2);

	if (target == kNowhere) {
		mob_log(ch, trig, "mteleport target is an invalid room");
		return;
	}
	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все")) {
		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (vict->in_room == kNowhere) {
				mob_log(ch, trig, "mteleport transports from kNowhere");
				return;
			}
			if (target == vict->in_room) {
//				mob_log(ch, trig, "mteleport all: target is itself");
				continue;
			}
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, target);
			if (!vict->IsNpc()) {
				look_at_room(vict, true);
				lastchar = vict;
			}
		}
		if (lastchar) {
			greet_mtrigger(lastchar, -1);
			greet_otrigger(lastchar, -1);
		}
	} else if (!str_cmp(arg1, "allchar") || !str_cmp(arg1, "всечары")) {
		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (vict->in_room == kNowhere) {
				mob_log(ch, trig, "mteleport transports allchar from kNowhere");
				return;
			}
			if (vict->IsNpc() && !IS_CHARMICE(vict)) {
				continue;
			}
			if (target == vict->in_room) {
//				mob_log(ch, trig, "mteleport allchar: target is itself");
				continue;
			}
			if (vict->get_horse()) {
				if (vict->IsOnHorse() || vict->has_horse(true)) {
					RemoveCharFromRoom(vict->get_horse());
					PlaceCharToRoom(vict->get_horse(), target);
					onhorse = true;
				}
			}
			if (target == vict->in_room) {
				mob_log(ch, trig, "mteleport allchar: target is itself");
			}
			RemoveCharFromRoom(vict);
			PlaceCharToRoom(vict, target);
			if (!onhorse)
				vict->dismount();
			if (!vict->IsNpc()) {
				look_at_room(vict, true);
				lastchar = vict;
			}
		}
		if (lastchar) {
			greet_mtrigger(lastchar, -1);
			greet_otrigger(lastchar, -1);
		}
	} else {
		if (*arg1 == UID_CHAR) {
			if (!(vict = get_char(arg1))) {
				sprintf(buf, "mteleport: victim (%s) UID does not exist", arg1 + 1);
				mob_log(ch, trig, buf);
				return;
			}
		} else if (!(vict = get_char_vis(ch, arg1, EFind::kCharInWorld))) {
			sprintf(buf, "mteleport: victim (%s) does not exist", arg1);
			mob_log(ch, trig, buf);
			return;
		}
		if (target == vict->in_room) {
//			mob_log(ch, trig, "mteleport: target is itself");
			return;
		}
		if (IS_CHARMICE(vict) && vict->in_room == vict->get_master()->in_room)
			vict = vict->get_master();
		const auto people_copy = world[vict->in_room]->people;
		for (const auto charmee : people_copy) {
			if (IS_CHARMICE(charmee) && charmee->get_master()  == vict) {
				RemoveCharFromRoom(charmee);
				PlaceCharToRoom(charmee, target);
			}
		}
		if (vict->get_horse()) {
			if (vict->IsOnHorse() || vict->has_horse(true)) {
				RemoveCharFromRoom(vict->get_horse());
				PlaceCharToRoom(vict->get_horse(), target);
				onhorse = true;
			}
		}
		from_room = vict->in_room;
		if (!str_cmp(argument, "followers") && vict->followers) {
			FollowerType *ft;
			for (ft = vict->followers; ft; ft = ft->next) {
				if (ft->follower->in_room == from_room && ft->follower->IsNpc()) {
					RemoveCharFromRoom(ft->follower);
					PlaceCharToRoom(ft->follower, target);
				}
			}
		}
		RemoveCharFromRoom(vict);
		PlaceCharToRoom(vict, target);
		if (!onhorse)
			vict->dismount();
		look_at_room(vict, true);
		greet_mtrigger(vict, -1);
		greet_otrigger(vict, -1);
	}
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
void do_mforce(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	argument = one_argument(argument, arg);

	if (!*arg || !*argument) {
		mob_log(ch, trig, "mforce: bad syntax");
		return;
	}

	if (!str_cmp(arg, "all")
		|| !str_cmp(arg, "все")) {
		mob_log(ch, trig, "ERROR: \'mforce all\' command disabled.");
		return;
	}

	CharData *victim = nullptr;

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "mforce: victim (%s) UID does not exist", arg + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if ((victim = get_char_room_vis(ch, arg)) == nullptr) {
		mob_log(ch, trig, "mforce: no such victim");
		return;
	}

	if (!victim->IsNpc()) {
		if ((!victim->desc)) {
			return;
		}
	}

	if (victim == ch) {
		mob_log(ch, trig, "mforce: forcing self");
		return;
	}

	if (victim->IsNpc()) {
		if (mob_script_command_interpreter(victim, argument, trig)) {
			mob_log(ch, trig, "Mob trigger commands in mforce. Please rewrite trigger.");
			return;
		}

		command_interpreter(victim, argument);
	} else if (GetRealLevel(victim) < kLvlImmortal) {
		command_interpreter(victim, argument);
	}
	else
		mob_log(ch, trig, "mforce: попытка принудить бессмертного.");
}

// increases the target's exp
void do_mexp(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *victim;
	char name[kMaxInputLength], amount[kMaxInputLength];

	mob_log(ch, trig, "WARNING: mexp command is depracated! Use: %actor.exp(amount-to-add)%");

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (ch->desc && (GetRealLevel(ch->desc->original) < kLvlImplementator))
		return;

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		mob_log(ch, trig, "mexp: too few arguments");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mexp: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mexp: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	}
	sprintf(buf, "mexp: victim (%s) получил опыт %d", name, atoi(amount));
	mob_log(ch, trig, buf);
	EndowExpToChar(victim, atoi(amount));
}

// increases the target's gold
void do_mgold(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *victim;
	char name[kMaxInputLength], amount[kMaxInputLength];

	mob_log(ch, trig, "WARNING: mgold command is depracated! Use: %actor.gold(amount-to-add)%");

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		mob_log(ch, trig, "mgold: too few arguments");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mgold: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mgold: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	}

	int num = atoi(amount);
	if (num >= 0) {
		victim->add_gold(num);
	} else {
		num = victim->remove_gold(num);
		if (num > 0) {
			mob_log(ch, trig, "mgold subtracting more gold than character has");
		}
	}
}

int script_driver(void *go, Trigger *trig, int type, int mode);

// transform into a different mobile
void do_mtransform(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *m;
	bool keep_hp = true;    // new mob keeps the old mob's hp/max hp/exp
	char buf[500];

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (ch->desc) {
		SendMsgToChar("You've got no VNUM to return to, dummy! try 'switch'\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		sprintf(buf, "mtransform: missing argument: %s", argument);
		mob_log(ch, trig, buf);
	} else if (!a_isdigit(*arg) && *arg != '-') {
		sprintf(buf, "mtransform: bad argument: %s", argument);
		mob_log(ch, trig, buf);
	} else {
		if (a_isdigit(*arg))
			m = ReadMobile(atoi(arg), kVirtual);
		else {
			keep_hp = false;
			m = ReadMobile(atoi(arg + 1), kVirtual);
		}
		if (m == nullptr) {
			mob_log(ch, trig, "mtransform: bad mobile vnum");
			return;
		}
/* Чет у нас есть такие триггера
		if (GET_MOB_VNUM(ch) == GET_MOB_VNUM(m)) {
			mob_log(ch, trig, "mtransform: попытка в того же моба");
			return;
		}
*/
		PlaceCharToRoom(m, ch->in_room);
		std::swap(ch, m);
		long tmp = ch->get_uid(); //UID надо осталять старые
		ch->set_uid(m->get_uid());
		m->set_uid(tmp);
		ch->script->types = m->script->types;
		ch->script->script_trig_list.clear();
	 	for (auto t_tmp : m->script->script_trig_list) {
			if (t_tmp->get_rnum() != trig->get_rnum()) {
				auto *t = new Trigger(*trig_index[t_tmp->get_rnum()]->proto);
				ch->script->script_trig_list.add(t);
			}
		}
		// продолжать работать будем по копии
		auto *trig_copy = new Trigger(*trig_index[trig->get_rnum()]->proto);
		GET_TRIG_DEPTH(trig_copy) = GET_TRIG_DEPTH(trig);
		trig_copy->var_list = trig->var_list;
		ch->script->script_trig_list.add(trig_copy);
		ch->script->global_vars = m->script->global_vars;
		trig_copy->context = trig->context;
		if (m->GetEnemy()) {
			m->GetEnemy()->SetEnemy(ch);
			SetFighting(ch, m->GetEnemy());
			m->SetEnemy(nullptr);
		}
		ch->followers = m->followers;
		m->followers = nullptr;
		for (struct FollowerType *l = ch->followers; l; l = l->next) {
			l->follower->set_master(ch);
		}
		ch->set_normal_morph();
		for (const auto &af : m->affected) {
			const auto &affect = *af;

			affect_to_char(ch, affect);
		}
		if (AFF_FLAGGED(m, EAffect::kGroup)) {
			AFF_FLAGS(ch).set(EAffect::kGroup);
		}
		ObjData *obj, *next_obj;

		for (obj = m->carrying; obj; obj = next_obj) {
			next_obj = obj->get_next_content();

			RemoveObjFromChar(obj);
			PlaceObjToInventory(obj, ch);
		}
		ch->set_wait(m->get_wait());  
		ch->set_master(m->get_master());
		if (m->get_master()) {
			for (auto f = m->get_master()->followers; f; f = f->next) {
				if (f->follower == m) {
					f->follower = ch;
				}
			}
		}
		m->set_master(nullptr);
		if (keep_hp) {
			ch->set_hit(m->get_hit());
			ch->set_max_hit(m->get_max_hit());
			ch->set_exp(m->get_exp());
		}
		ch->set_gold(m->get_gold());
		ch->SetPosition(m->GetPosition());
		IS_CARRYING_W(ch) = IS_CARRYING_W(m);
		IS_CARRYING_N(ch) = IS_CARRYING_N(m);
		trig->halt();
		character_list.AddToExtractedList(m);
		chardata_by_uid[ch->get_uid()] = ch;
/*
		trig_copy->cmdlist.reset();
		*trig_copy = *trig_index[trig->get_rnum()]->proto;
		trig_copy->curr_line = *trig_copy->cmdlist;
		for (int num = 1; num < trig->curr_line->line_num; num ++) {
			trig_copy->curr_line = trig_copy->curr_line->next;
		}
		if (trig_copy->curr_line->next) {
			trig_copy->curr_line = trig_copy->curr_line->next;
*/
		if (trig->curr_line->next) { // тут возмьожен крешь так как переносим указатель который должен чиститься
			trig_copy->curr_line = trig->curr_line->next;
			script_driver(ch, trig_copy, MOB_TRIGGER, TRIG_FROM_LINE);
		}
	}
}

void do_mdoor(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char target[kMaxInputLength], direction[kMaxInputLength];
	char field[kMaxInputLength], *value;
	RoomData *rm;
	int dir, fd, to_room, lock;
	char error[kMaxInputLength];
	const char *door_field[] = {
			"purge",
			"description",
			"flags",
			"key",
			"name",
			"room",
			"lock",
			"\n"
	};

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);
	skip_spaces(&value);

	if (!*target || !*direction || !*field) {
		mob_log(ch, trig, "mdoor called with too few args");
		sprintf(buf, "mdoor argument: %s", error);
		mob_log(ch, trig, buf);
		return;
	}

	if ((rm = get_room(target)) == nullptr) {
		mob_log(ch, trig, "mdoor: invalid target");
		sprintf(buf, "mdoor argument: %s", error);
		mob_log(ch, trig, buf);
		return;
	}

	if ((dir = search_block(direction, dirs, false)) == -1) {
		mob_log(ch, trig, "mdoor: invalid direction");
		sprintf(buf, "mdoor argument: %s", error);
		mob_log(ch, trig, buf);
		return;
	}

	if ((fd = search_block(field, door_field, false)) == -1) {
		mob_log(ch, trig, "mdoor: invalid field");
		sprintf(buf, "mdoor argument: %s", error);
		mob_log(ch, trig, buf);
		return;
	}

	auto exit = rm->dir_option[dir];

	// purge exit
	if (fd == 0) {
		if (exit) {
			rm->dir_option[dir].reset();
		}
	} else {
		if (!exit) {
			exit.reset(new ExitData());
			rm->dir_option[dir] = exit;
		}

		std::string buffer;
		switch (fd) {
			case 1:    // description
				exit->general_description = std::string(value) + "\r\n";
				break;

			case 2:    // flags
				asciiflag_conv(value, &exit->exit_info);
				break;

			case 3:    // key
				exit->key = atoi(value);
				break;

			case 4:    // name
				exit->set_keywords(value);
				break;

			case 5:    // room
				if ((to_room = GetRoomRnum(atoi(value))) != kNowhere) {
					exit->to_room(to_room);
				} else {
					mob_log(ch, trig, "mdoor: invalid door target");
				}
				break;

			case 6:    // lock - сложность замка
				lock = atoi(value);
				if (!(lock < 0 || lock > 255))
					exit->lock_complexity = lock;
				else
					mob_log(ch, trig, "mdoor: invalid lock complexity");
				break;
		}
	}
}

// increases spells & skills
ESpell FixNameAndFindSpellId(char *name);

void do_mfeatturn(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	int isFeat = 0;
	CharData *victim;
	char name[kMaxInputLength], featname[kMaxInputLength], amount[kMaxInputLength], *pos;
	int featdiff = 0;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount) {
		mob_log(ch, trig, "mfeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.')))
		*pos = ' ';
	while ((pos = strchr(featname, '_')))
		*pos = ' ';

	const auto feat_id = FindFeatId(featname);
	if (MUD::Feat(feat_id).IsAvailable())
		isFeat = 1;
	else {
		mob_log(ch, trig, "mfeatturn: feature not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else {
		mob_log(ch, trig, "mfeatturn: unknown set variable");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mfeatturn: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mfeatturn: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	};

	if (isFeat)
		trg_featturn(victim, feat_id, featdiff, last_trig_vnum);
}

void do_mskillturn(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *victim;
	char name[kMaxInputLength], skill_name[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	one_argument(two_arguments(argument, name, skill_name), amount);

	if (!*name || !*skill_name || !*amount) {
		mob_log(ch, trig, "mskillturn: too few arguments");
		return;
	}

	auto skill_id = FixNameAndFindSkillId(skill_name);
	bool is_skill = false;
	if (MUD::Skills().IsValid(skill_id)) {
		is_skill = true;
	} else if ((recipenum = im_get_recipe_by_name(skill_name)) < 0) {
		sprintf(buf, "mskillturn: %s skill not found", skill_name);
		mob_log(ch, trig, buf);
		return;
	}

	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		mob_log(ch, trig, "mskillturn: unknown set variable");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mskillturn: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mskillturn: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	}

	if (is_skill) {
		if (MUD::Class(victim->GetClass()).skills[skill_id].IsAvailable()) {
			trg_skillturn(victim, skill_id, skilldiff, last_trig_vnum);
		} else {
			sprintf(buf, "mskillturn: skill and character class mismatch");
			mob_log(ch, trig, buf);
		}
	} else {
		trg_recipeturn(victim, recipenum, skilldiff);
	}
}

void do_mskilladd(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	bool isSkill = false;
	CharData *victim;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount) {
		mob_log(ch, trig, "mskilladd: too few arguments");
		return;
	}
	auto skill_id = FixNameAndFindSkillId(skillname);
	if (MUD::Skills().IsValid(skill_id)) {
		isSkill = true;
	} else if ((recipenum = im_get_recipe_by_name(skillname)) < 0) {
		sprintf(buf, "mskilladd: %s skill/recipe not found", skillname);
		mob_log(ch, trig, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mskilladd: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mskilladd: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	};

	if (isSkill) {
		AddSkill(victim, skill_id, skilldiff, last_trig_vnum);
	} else {
		AddRecipe(victim, recipenum, skilldiff);
	}
}

void do_mspellturn(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *victim;
	char name[kMaxInputLength], apellname[kMaxInputLength], amount[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	argument = one_argument(argument, name);
	two_arguments(argument, apellname, amount);

	if (!*name || !*apellname || !*amount) {
		mob_log(ch, trig, "mspellturn: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(apellname);
	if (spell_id == ESpell::kUndefined) {
		mob_log(ch, trig, "mspellturn: spell not found");
		return;
	}

	auto skilldiff{0};
	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		mob_log(ch, trig, "mspellturn: unknown set variable");
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspellturn: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mspellturn: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	};

	trg_spellturn(victim, spell_id, skilldiff, last_trig_vnum);
}

void do_mspellturntemp(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *victim;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	argument = one_argument(argument, name);
	two_arguments(argument, spellname, amount);

	if (!*name || !*spellname || !*amount) {
		mob_log(ch, trig, "mspellturntemp: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		mob_log(ch, trig, "mspellturntemp: spell not found");
		return;
	}

	auto spelltime = atoi(amount);

	if (spelltime <= 0) {
		mob_log(ch, trig, "mspellturntemp: time is zero or negative");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspellturntemp: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mspellturntemp: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	};

	trg_spellturntemp(victim, spell_id, spelltime, last_trig_vnum);
}

void do_mspelladd(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];
	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		mob_log(ch, trig, "mspelladd: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		mob_log(ch, trig, "mspelladd: skill not found");
		return;
	}

	CharData *victim;
	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspelladd: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mspelladd: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	};

	auto skilldiff = atoi(amount);
	trg_spelladd(victim, spell_id, skilldiff, last_trig_vnum);
}

void do_mspellitem(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *victim;
	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	char name[kMaxInputLength], spellname[kMaxInputLength], type[kMaxInputLength], turn[kMaxInputLength];
	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn) {
		mob_log(ch, trig, "mspellitem: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		mob_log(ch, trig, "mspellitem: spell not found");
		return;
	}

	ESpellType spell;
	if (!str_cmp(type, "potion")) {
		spell = ESpellType::kPotionCast;
	} else if (!str_cmp(type, "wand")) {
		spell = ESpellType::kWandCast;
	} else if (!str_cmp(type, "scroll")) {
		spell = ESpellType::kScrollCast;
	} else if (!str_cmp(type, "items")) {
		spell = ESpellType::kItemCast;
	} else if (!str_cmp(type, "runes")) {
		spell = ESpellType::kRunes;
	} else {
		mob_log(ch, trig, "mspellitem: type spell not found");
		return;
	}

	auto spelldiff{0};
	if (!str_cmp(turn, "set")) {
		spelldiff = 1;
	} else if (!str_cmp(turn, "clear")) {
		spelldiff = 0;
	} else {
		mob_log(ch, trig, "mspellitem: unknown set variable");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspellitem: victim (%s) UID does not exist", name + 1);
			mob_log(ch, trig, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, EFind::kCharInWorld))) {
		sprintf(buf, "mspellitem: victim (%s) does not exist", name);
		mob_log(ch, trig, buf);
		return;
	};

	trg_spellitem(victim, spell_id, spelldiff, spell);
}

void do_mdamage(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char name[kMaxInputLength], amount[kMaxInputLength], damage_type[kMaxInputLength];
	int dam = 0;
	const std::map<std::string, fight::DmgType> kDamageTypes = {
			{"physic", fight::kPhysDmg},
			{"magic", fight::kMagicDmg},
			{"poisonous", fight::kPoisonDmg}
	};
	fight::DmgType type = fight::kPureDmg;

	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		return;
	}

	three_arguments(argument, name, amount, damage_type);

	if (!*name || !*amount || !a_isdigit(*amount)) {
		sprintf(buf, "mdamage: bad syntax, команда: %s", argument);
		mob_log(ch, trig, buf);
		return;
	}

	dam = atoi(amount);
	auto victim = get_char(name);
	if (victim) {
		if (world[victim->in_room]->zone_rn != world[ch->in_room]->zone_rn) {
			return;
		}

		if (IS_IMMORTAL(victim) && dam > 0) {
			SendMsgToChar("Будучи очень крутым, вы сделали шаг в сторону и не получили повреждений...\r\n", victim);
			return;
		}
		if (*damage_type) {
			try {
				type = kDamageTypes.at(damage_type);
			} catch (const std::out_of_range &) {
				mob_log(ch, trig, "mdamage: incorrect damage type.");
				return;
			}
			Damage mdamage(SimpleDmg(kTypeTriggerdeath), dam, type);
			mdamage.Process(ch, victim);
		} else {
			victim->set_hit(victim->get_hit() - dam);
			if (dam < 0) {
				SendMsgToChar("Вы почувствовали себя лучше.\r\n", victim);
				return;
			}
			update_pos(victim);
			char_dam_message(dam, victim, victim, 0);
			if (victim->GetPosition() == EPosition::kDead) {
				if (!victim->IsNpc()) {
					sprintf(buf2, "%s killed by mobdamage at %s [%d]",GET_NAME(victim),
						victim->in_room == kNowhere ? "kNowhere" : world[victim->in_room]->name,
						GET_ROOM_VNUM(victim->in_room));
				mudlog(buf2, BRF, 0, SYSLOG, true);
				}
				die(victim, ch);
			}
		}
	} else {
		mob_log(ch, trig, "mdamage: target not found");
	}
}

void do_mzoneecho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	ZoneRnum zone;
	char zone_name[kMaxInputLength], buf[kMaxInputLength], *msg;

	msg = any_one_arg(argument, zone_name);
	skip_spaces(&msg);

	if (!*zone_name || !*msg) {
		sprintf(buf, "mzoneecho called with too few args, команда: %s", argument);
		mob_log(ch, trig, buf);
	}
	else if ((zone = get_zone_rnum_by_vnumum(atoi(zone_name))) < 0) {
		std::stringstream str_log;
		str_log << "mzoneecho called for nonexistant zone: " << zone_name;
		mob_log(ch, trig, str_log.str().c_str());
	}
	else {
		sprintf(buf, "%s\r\n", msg);
		send_to_zone(buf, zone);
	}
}
// для команды mat
void MobDgCast(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char *dg_arg = str_dup("DgCast ");
	strcat(dg_arg, argument);
	do_dg_cast(ch, trig, MOB_TRIGGER, dg_arg);
	free(dg_arg);
}

const struct mob_command_info mob_cmd_info[] =
	{
		// this must be first -- for specprocs
		{"RESERVED", EPosition::kDead, nullptr, 0, false},
		{"masound", EPosition::kDead, do_masound, -1, false},
		{"mkill", EPosition::kStand, do_mkill, -1, false},
		{"mjunk", EPosition::kSit, do_mjunk, -1, true},
		{"mdamage", EPosition::kDead, do_mdamage, -1, false},
		{"mdoor", EPosition::kDead, do_mdoor, -1, false},
		{"mecho", EPosition::kDead, do_mecho, -1, false},
		{"mechoaround", EPosition::kDead, do_mechoaround, -1, false},
		{"msend", EPosition::kDead, do_msend, -1, false},
		{"mload", EPosition::kDead, do_mload, -1, false},
		{"mpurge", EPosition::kDead, do_mpurge, -1, true},
		{"mgoto", EPosition::kDead, do_mgoto, -1, false},
		{"mat", EPosition::kDead, do_mat, -1, false},
		{"mteleport", EPosition::kDead, do_mteleport, -1, false},
		{"mforce", EPosition::kDead, do_mforce, -1, false},
		{"mexp", EPosition::kDead, do_mexp, -1, false},
		{"mgold", EPosition::kDead, do_mgold, -1, false},
		{"mtransform", EPosition::kDead, do_mtransform, -1, false},
		{"mfeatturn", EPosition::kDead, do_mfeatturn, -1, false},
		{"mskillturn", EPosition::kDead, do_mskillturn, -1, false},
		{"mskilladd", EPosition::kDead, do_mskilladd, -1, false},
		{"mspellturn", EPosition::kDead, do_mspellturn, -1, false},
		{"mspellturntemp", EPosition::kDead, do_mspellturntemp, -1, false},
		{"mspelladd", EPosition::kDead, do_mspelladd, -1, false},
		{"mspellitem", EPosition::kDead, do_mspellitem, -1, false},
		{"mportal", EPosition::kDead, do_mportal, -1, false},
		{"mzoneecho", EPosition::kDead, do_mzoneecho, -1, false},
		{"asound", EPosition::kDead, do_masound, -1, false},
		{"kill", EPosition::kStand, do_mkill, -1, false},
		{"junk", EPosition::kSit, do_mjunk, -1, true},
		{"damage", EPosition::kDead, do_mdamage, -1, false},
		{"door", EPosition::kDead, do_mdoor, -1, false},
		{"echo", EPosition::kDead, do_mecho, -1, false},
		{"echoaround", EPosition::kDead, do_mechoaround, -1, false},
		{"send", EPosition::kDead, do_msend, -1, false},
		{"load", EPosition::kDead, do_mload, -1, false},
		{"purge", EPosition::kDead, do_mpurge, -1, true},
		{"goto", EPosition::kDead, do_mgoto, -1, false},
		{"at", EPosition::kDead, do_mat, -1, false},
		{"teleport", EPosition::kDead, do_mteleport, -1, false},
		{"force", EPosition::kDead, do_mforce, -1, false},
		{"exp", EPosition::kDead, do_mexp, -1, false},
		{"gold", EPosition::kDead, do_mgold, -1, false},
		{"transform", EPosition::kDead, do_mtransform, -1, false},
		{"featturn", EPosition::kDead, do_mfeatturn, -1, false},
		{"skillturn", EPosition::kDead, do_mskillturn, -1, false},
		{"skilladd", EPosition::kDead, do_mskilladd, -1, false},
		{"spellturn", EPosition::kDead, do_mspellturn, -1, false},
		{"spellturntemp", EPosition::kDead, do_mspellturntemp, -1, false},
		{"spelladd", EPosition::kDead, do_mspelladd, -1, false},
		{"spellitem", EPosition::kDead, do_mspellitem, -1, false},
		{"portal", EPosition::kDead, do_mportal, -1, false},
		{"zoneecho", EPosition::kDead, do_mzoneecho, -1, false},
		{"dgcast", EPosition::kDead, MobDgCast, -1, false},
		{"\n", EPosition::kDead, nullptr, 0, false}
		// this must be last
	};

bool mob_script_command_interpreter(CharData *ch, char *argument, Trigger *trig) {
	char *line, arg[kMaxInputLength];
	bool use_in_stoped = false;

	// just drop to next line for hitting CR
	skip_spaces(&argument);

	if (!*argument)
		return false;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;

	while (*mob_cmd_info[cmd].command != '\n') {
		if (arg[0] == '!') {
			if (!strcmp(mob_cmd_info[cmd].command, arg + 1)) {
				use_in_stoped = true;
				break;
			}
		}
		else {
			if (!strcmp(mob_cmd_info[cmd].command, arg))
				break;
		}
		cmd++;
	}
// damage mtrigger срабатывает всегда
	if (!(CheckScript(ch, MTRIG_DAMAGE) || CheckScript(ch, MTRIG_DEATH))) {
		if (!use_in_stoped && !mob_cmd_info[cmd].use_in_stoped
				&& (AFF_FLAGGED(ch, EAffect::kHold)
						|| AFF_FLAGGED(ch, EAffect::kStopFight)
						|| AFF_FLAGGED(ch, EAffect::kMagicStopFight))
				&& !trig->add_flag) {
			if (!strcmp(mob_cmd_info[cmd].command, "mload") || (!strcmp(mob_cmd_info[cmd].command, "load"))) {
				sprintf(buf, "command_interpreter: моб в стане, mload пропущен, команда: %s", argument);
				mob_log(ch, trig, buf);
			}
			return false;
		}
	}
	if (*mob_cmd_info[cmd].command == '\n') {
		return false;
	} else if (ch->GetPosition() < mob_cmd_info[cmd].minimum_position) {
		return false;
	} else {
		check_hiding_cmd(ch, -1);

		const mob_command_info::handler_f &command = mob_cmd_info[cmd].command_pointer;
		command(ch, line, cmd, mob_cmd_info[cmd].subcmd, trig);

		return true;
	}
}

// *  This is the command interpreter used by mob, include common interpreter's commands
void mob_command_interpreter(CharData *ch, char *argument, Trigger *trig) {
	if (!mob_script_command_interpreter(ch, argument, trig)) {
		command_interpreter(ch, argument);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
