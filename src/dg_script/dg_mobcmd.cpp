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

#include "entities/char_data.h"
#include "cmd/follow.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "handler.h"
#include "obj_prototypes.h"
#include "game_magic/magic_utils.h"
#include "game_skills/townportal.h"
#include "utils/id_converter.h"
#include "entities/zone.h"
#include "structs/global_objects.h"

struct mob_command_info {
	const char *command;
	EPosition minimum_position;
	typedef void(*handler_f)(CharData *ch, char *argument, int cmd, int subcmd);
	handler_f command_pointer;
	int subcmd;                ///< Subcommand. See SCMD_* constants.
	bool use_in_lag;
};

#define IS_CHARMED(ch)          (IS_HORSE(ch)||AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))

extern int reloc_target;
extern Trigger *cur_trig;

void sub_write(char *arg, CharData *ch, byte find_invis, int targets);
RoomData *get_room(char *name);
ObjData *get_obj_by_char(CharData *ch, char *name);
// * Local functions.
void mob_command_interpreter(CharData *ch, char *argument);
bool mob_script_command_interpreter(CharData *ch, char *argument);
void send_to_zone(char *messg, int zone_rnum);

// attaches mob's name and vnum to msg_set and sends it to script_log
void mob_log(CharData *mob, const char *msg, LogMode type = LogMode::OFF) {
	char buf[kMaxInputLength + 100];

	sprintf(buf, "(Mob: '%s', VNum: %d, trig: %d): %s", GET_SHORT(mob), GET_MOB_VNUM(mob), last_trig_vnum, msg);
	script_log(buf, type);
}

//returns the real room number, or kNowhere if not found or invalid
//copy from find_target_room except char's messages
RoomRnum dg_find_target_room(CharData *ch, char *rawroomstr) {
	char roomstr[kMaxInputLength];
	RoomRnum location = kNowhere;

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		sprintf(buf, "Undefined mteleport room: %s", rawroomstr);
		mob_log(ch, buf);
		return kNowhere;
	}

	const auto tmp = atoi(roomstr);
	if (tmp > 0) {
		location = real_room(tmp);
	} else {
		sprintf(buf, "Undefined mteleport room: %s", roomstr);
		mob_log(ch, buf);
		return kNowhere;
	}

	return location;
}

void do_mportal(CharData *mob, char *argument, int/* cmd*/, int/* subcmd*/) {
	int target, howlong, curroom, nr;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		mob_log(mob, "mportal: called with too few args");
		return;
	}

	howlong = atoi(arg2);
	nr = atoi(arg1);
	target = real_room(nr);

	if (target == kNowhere) {
		mob_log(mob, "mportal: target is an invalid room");
		return;
	}

	/* Ставим пентаграмму из текущей комнаты в комнату target с
	   длительностью howlong */
	curroom = mob->in_room;
	world[curroom]->portal_room = target;
	world[curroom]->portal_time = howlong;
	world[curroom]->pkPenterUnique = 0;
	OneWayPortal::add(world[target], world[curroom]);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), nullptr, nullptr, kToChar);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), nullptr, nullptr, kToRoom);
}
// prints the argument to all the rooms aroud the mobile
void do_masound(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (!*argument) {
		mob_log(ch, "masound called with no argument");
		return;
	}

	skip_spaces(&argument);

	const int temp_in_room = ch->in_room;
	for (int door = 0; door < kDirMaxNumber; door++) {
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
void do_mkill(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharData *victim;

	if (MOB_FLAGGED(ch, MOB_NOFIGHT)) {
		mob_log(ch, "mkill called for mob with NOFIGHT flag");
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, "mkill called with no argument");
		return;
	}

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "mkill: victim (%s) not found", arg + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_room_vis(ch, arg))) {
		sprintf(buf, "mkill: victim (%s) not found", arg);
		mob_log(ch, buf);
		return;
	}

	if (victim == ch) {
		mob_log(ch, "mkill: victim is self");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)
		&& ch->get_master() == victim) {
		mob_log(ch, "mkill: charmed mob attacking master");
		return;
	}

	if (ch->get_fighting()) {
		mob_log(ch, "mkill: already fighting");
		return;
	}

	hit(ch, victim, ESkill::kUndefined, fight::kMainHand);
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
void do_mjunk(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	int pos, junk_all = 0;
	ObjData *obj;
	ObjData *obj_next;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, "mjunk called with no argument");
		return;
	}

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
		junk_all = 1;

	if ((find_all_dots(arg) == FIND_INDIV) && !junk_all) {
		if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos)) != nullptr) {
			unequip_char(ch, pos, CharEquipFlags());
			extract_obj(obj);
			return;
		}
		if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying)) != nullptr)
			extract_obj(obj);
		return;
	} else {
		for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
			obj_next = obj->get_next_content();
			if (arg[3] == '\0' || isname(arg + 4, obj->get_aliases())) {
				extract_obj(obj);
			}
		}
		while ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos))) {
			unequip_char(ch, pos, CharEquipFlags());
			extract_obj(obj);
		}
	}
}

// prints the message to everyone in the room other than the mob and victim
void do_mechoaround(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharData *victim;
	char *p;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	p = one_argument(argument, arg);
	skip_spaces(&p);

	if (!*arg) {
		mob_log(ch, "mechoaround called with no argument");
		return;
	}

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "mechoaround: victim (%s) UID does not exist", arg + 1);
			mob_log(ch, buf, LGH);
			return;
		}
	} else if (!(victim = get_char_room_vis(ch, arg))) {
		sprintf(buf, "mechoaround: victim (%s) does not exist", arg);
		mob_log(ch, buf, LGH);
		return;
	}

	if (reloc_target != -1 && reloc_target != IN_ROOM(victim)) {
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	}

	sub_write(p, victim, true, kToRoom);
}

// sends the message to only the victim
void do_msend(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharData *victim;
	char *p;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	p = one_argument(argument, arg);
	skip_spaces(&p);

	if (!*arg) {
		mob_log(ch, "msend called with no argument");
		return;
	}

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "msend: victim (%s) UID does not exist", arg + 1);
			mob_log(ch, buf, LGH);
			return;
		}
	} else if (!(victim = get_char_room_vis(ch, arg))) {
//		sprintf(buf, "msend: victim (%s) does not exist", arg);
//		mob_log(ch, buf, LGH);
		return;
	}

	if (reloc_target != -1 && reloc_target != IN_ROOM(victim)) {
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
	}

	sub_write(p, victim, true, kToChar);
}

// prints the message to the room at large
void do_mecho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char *p;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (!*argument) {
		mob_log(ch, "mecho called with no arguments");
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
void do_mload(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	int number = 0;
	CharData *mob;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		mob_log(ch, "mload: попытка почармленным мобом загрузать моба/предмет.");
		return;
	}

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
		mob_log(ch, "mload: bad syntax");
		return;
	}

	if (utils::IsAbbrev(arg1, "mob")) {
		if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
			mob_log(ch, "mload: bad mob vnum");
			return;
		}
		log("Load mob #%d by %s (mload)", number, GET_NAME(ch));
		char_to_room(mob, ch->in_room);
		load_mtrigger(mob);
	} else if (utils::IsAbbrev(arg1, "obj")) {
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object) {
			mob_log(ch, "mload: bad object vnum");
			return;
		}

		if (GET_OBJ_MIW(obj_proto[object->get_rnum()]) >= 0
			&& obj_proto.actual_count(object->get_rnum()) > GET_OBJ_MIW(obj_proto[object->get_rnum()])) {
			if (!check_unlimited_timer(obj_proto[object->get_rnum()].get())) {
				sprintf(buf, "mload: Попытка загрузить предмет больше чем в MIW для #%d.", number);
				mob_log(ch, buf);
//				extract_obj(object.get());
//				return;
			}
		}

		log("Load obj #%d by %s (mload)", number, GET_NAME(ch));
		object->set_vnum_zone_from(zone_table[world[ch->in_room]->zone_rn].vnum);

		if (CAN_WEAR(object.get(), EWearFlag::ITEM_WEAR_TAKE)) {
			obj_to_char(object.get(), ch);
		} else {
			obj_to_room(object.get(), ch->in_room);
		}

		load_otrigger(object.get());
	} else
		mob_log(ch, "mload: bad type");
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
void do_mpurge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharData *victim;
	ObjData *obj;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
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
			extract_obj(obj);
		} else {
			mob_log(ch, "mpurge: bad argument");
		}
		return;
	}

	if (!IS_NPC(victim)) {
		mob_log(ch, "mpurge: purging a PC");
		return;
	}

	if (victim->followers
		|| victim->has_master()) {
		die_follower(victim);
	}

	extract_char(victim, false);
}

// lets the mobile goto any location it wishes that is not private
void do_mgoto(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	int location;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		mob_log(ch, "mgoto called with no argument");
		return;
	}

	if ((location = dg_find_target_room(ch, arg)) == kNowhere) {
		std::stringstream buffer;
		buffer << "mgoto: invalid location '" << arg << "'";
//		sprintf(buf, "mgoto: invalid location '%s'", arg);
		mob_log(ch, buffer.str().c_str());
		return;
	}

	if (ch->get_fighting())
		stop_fighting(ch, true);

	char_from_room(ch);
	char_to_room(ch, location);
}

// lets the mobile do a command at another location. Very useful
void do_mat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	int location;
	int original;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	argument = one_argument(argument, arg);

	if (!*arg || !*argument) {
		mob_log(ch, "mat: bad argument");
		return;
	}

	if ((location = dg_find_target_room(ch, arg)) == kNowhere) {
		std::stringstream buffer;
		buffer << "mat: invalid location '" << arg << "'";
//		sprintf(buf, "mat: invalid location '%s'", arg);
		mob_log(ch, buffer.str().c_str());
		return;
	}

	reloc_target = location;
	original = ch->in_room;
	ch->in_room = location;
	mob_command_interpreter(ch, argument);
	ch->in_room = original;
	reloc_target = -1;
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
void do_mteleport(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	int target;
	CharData *vict, *horse;
	RoomRnum from_room;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;
	char *textstr = argument;
	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		sprintf(buf, "mteleport: bad syntax, arg: %s", textstr);
		mob_log(ch, buf);
		return;
	}

	target = dg_find_target_room(ch, arg2);

	if (target == kNowhere) {
		mob_log(ch, "mteleport target is an invalid room");
		return;
	}
	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все")) {
		if (target == ch->in_room) {
			mob_log(ch, "mteleport all: target is itself");
			return;
		}
		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (IN_ROOM(vict) == kNowhere) {
				mob_log(ch, "mteleport transports from kNowhere");
				return;
			}
			char_from_room(vict);
			char_to_room(vict, target);
			// переделать чтоб чары смотрели в клетку после переноса, походу еще один цикл крутить, ну и мутево будет
			if (!IS_NPC(vict))
				look_at_room(vict, true);
		}
	} else if (!str_cmp(arg1, "allchar") || !str_cmp(arg1, "всечары")) {
		if (target == ch->in_room) {
			mob_log(ch, "mteleport all: target is itself");
			return;
		}
		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (IN_ROOM(vict) == kNowhere) {
				mob_log(ch, "mteleport transports allchar from kNowhere");
				return;
			}
			if (IS_NPC(vict) && !IS_CHARMICE(vict))
				continue;
			char_from_room(vict);
			char_to_room(vict, target);
			look_at_room(vict, true);
		}
	} else {
		if (*arg1 == UID_CHAR) {
			if (!(vict = get_char(arg1))) {
				sprintf(buf, "mteleport: victim (%s) UID does not exist", arg1 + 1);
				mob_log(ch, buf);
				return;
			}
		} else if (!(vict = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) {
			sprintf(buf, "mteleport: victim (%s) does not exist", arg1);
			mob_log(ch, buf);
			return;
		}
		if (IS_CHARMICE(vict) && vict->in_room == vict->get_master()->in_room)
			vict = vict->get_master();
		const auto people_copy = world[IN_ROOM(vict)]->people;
		for (const auto charmee : people_copy) {
			if (IS_CHARMICE(charmee) && charmee->get_master()  == vict) {
				char_from_room(charmee);
				char_to_room(charmee, target);
			}
		}
		if (vict->ahorse() || vict->has_horse(true)) {
			horse = vict->get_horse();
		} else {
			horse = nullptr;
		}
		if (!str_cmp(argument, "horse") && horse) {
			char_from_room(horse);
			char_to_room(horse, target);
		}
		from_room = vict->in_room;
//Polud реализуем режим followers. за аргументом телепорта перемешаются все последователи-NPC
		if (!str_cmp(argument, "followers") && vict->followers) {
			Follower *ft;
			for (ft = vict->followers; ft; ft = ft->next) {
				if (IN_ROOM(ft->ch) == from_room && IS_NPC(ft->ch)) {
					char_from_room(ft->ch);
					char_to_room(ft->ch, target);
				}
			}
		}
//-Polud
		char_from_room(vict);
		char_to_room(vict, target);
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
void do_mforce(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	argument = one_argument(argument, arg);

	if (!*arg || !*argument) {
		mob_log(ch, "mforce: bad syntax");
		return;
	}

	if (!str_cmp(arg, "all")
		|| !str_cmp(arg, "все")) {
		mob_log(ch, "ERROR: \'mforce all\' command disabled.");
		return;
	}

	CharData *victim = nullptr;

	if (*arg == UID_CHAR) {
		if (!(victim = get_char(arg))) {
			sprintf(buf, "mforce: victim (%s) UID does not exist", arg + 1);
			mob_log(ch, buf);
			return;
		}
	} else if ((victim = get_char_room_vis(ch, arg)) == nullptr) {
		mob_log(ch, "mforce: no such victim");
		return;
	}

	if (!IS_NPC(victim)) {
		if ((!victim->desc)) {
			return;
		}
	}

	if (victim == ch) {
		mob_log(ch, "mforce: forcing self");
		return;
	}

	if (IS_NPC(victim)) {
		if (mob_script_command_interpreter(victim, argument)) {
			mob_log(ch, "Mob trigger commands in mforce. Please rewrite trigger.");
			return;
		}

		command_interpreter(victim, argument);
	} else if (GetRealLevel(victim) < kLvlImmortal) {
		command_interpreter(victim, argument);
	}
	else
		mob_log(ch, "mforce: попытка принудить бессмертного.");
}

// increases the target's exp
void do_mexp(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], amount[kMaxInputLength];

	mob_log(ch, "WARNING: mexp command is depracated! Use: %actor.exp(amount-to-add)%");

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GetRealLevel(ch->desc->original) < kLvlImplementator))
		return;

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		mob_log(ch, "mexp: too few arguments");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mexp: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mexp: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	}
	sprintf(buf, "mexp: victim (%s) получил опыт %d", name, atoi(amount));
	mob_log(ch, buf);
	gain_exp(victim, atoi(amount));
}

// increases the target's gold
void do_mgold(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], amount[kMaxInputLength];

	mob_log(ch, "WARNING: mgold command is depracated! Use: %actor.gold(amount-to-add)%");

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		mob_log(ch, "mgold: too few arguments");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mgold: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mgold: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	}

	int num = atoi(amount);
	if (num >= 0) {
		victim->add_gold(num);
	} else {
		num = victim->remove_gold(num);
		if (num > 0) {
			mob_log(ch, "mgold subtracting more gold than character has");
		}
	}
}

// transform into a different mobile
void do_mtransform(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg[kMaxInputLength];
	CharData *m;
	ObjData *obj[NUM_WEARS];
	int keep_hp = 1;    // new mob keeps the old mob's hp/max hp/exp
	int pos;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc) {
		send_to_char("You've got no VNUM to return to, dummy! try 'switch'\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg)
		mob_log(ch, "mtransform: missing argument");
	else if (!a_isdigit(*arg) && *arg != '-')
		mob_log(ch, "mtransform: bad argument");
	else {
		if (a_isdigit(*arg))
			m = read_mobile(atoi(arg), VIRTUAL);
		else {
			keep_hp = 0;
			m = read_mobile(atoi(arg + 1), VIRTUAL);
		}
		if (m == nullptr) {
			mob_log(ch, "mtransform: bad mobile vnum");
			return;
		}
// Тактика:
// 1. Прочитан новый моб (m), увеличено количество в mob_index
// 2. Чтобы уменьшить кол-во мобов ch, нужно экстрактить ch,
//    но этого делать НЕЛЬЗЯ, т.к. на него очень много ссылок.
// 3. Вывод - a) обмениваю содержимое m и ch.
//            b) в ch (бывший m) копирую игровую информацию из m (бывший ch)
//            c) удаляю m (на самом деле это данные ch в другой оболочке)


		for (pos = 0; pos < NUM_WEARS; pos++) {
			if (GET_EQ(ch, pos))
				obj[pos] = unequip_char(ch, pos, CharEquipFlags());
			else
				obj[pos] = nullptr;
		}

		// put the mob in the same room as ch so extract will work
		char_to_room(m, ch->in_room);

// Обмен содержимым
		CharData tmpmob(*m);
		*m = *ch;
		*ch = tmpmob;
		ch->set_normal_morph();

// Имею:
//  ch -> старый указатель, новое наполнение из моба m
//  m -> новый указатель, старое наполнение из моба ch
//  tmpmob -> врем. переменная, наполнение из оригинального моба m

// Копирование игровой информации (для m сохраняются оригинальные значения)
		ch->id = m->id;
		m->id = tmpmob.id;
		ch->affected = m->affected;
		m->affected = tmpmob.affected;
		ch->carrying = m->carrying;
		m->carrying = tmpmob.carrying;
		ch->script = m->script;
		m->script = tmpmob.script;

		ch->next_fighting = m->next_fighting;
		m->next_fighting = tmpmob.next_fighting;
		ch->followers = m->followers;
		m->followers = tmpmob.followers;
		ch->set_wait(m->get_wait());  // а лаг то у нас не копировался
		m->set_wait(tmpmob.get_wait());
		ch->set_master(m->get_master());
		m->set_master(tmpmob.get_master());

		if (keep_hp) {
			GET_HIT(ch) = GET_HIT(m);
			GET_MAX_HIT(ch) = GET_MAX_HIT(m);
			ch->set_exp(m->get_exp());
		}

		ch->set_gold(m->get_gold());
		GET_POS(ch) = GET_POS(m);
		IS_CARRYING_W(ch) = IS_CARRYING_W(m);
		IS_CARRYING_W(m) = IS_CARRYING_W(&tmpmob);
		IS_CARRYING_N(ch) = IS_CARRYING_N(m);
		IS_CARRYING_N(m) = IS_CARRYING_N(&tmpmob);
		ch->set_fighting(m->get_fighting());
		m->set_fighting(tmpmob.get_fighting());
		// для name_list
		ch->set_serial_num(m->get_serial_num());
		m->set_serial_num(tmpmob.get_serial_num());

		for (pos = 0; pos < NUM_WEARS; pos++) {
			if (obj[pos])
				equip_char(ch, obj[pos], pos, CharEquipFlag::no_cast);
		}
		extract_char(m, false);
	}
}

void do_mdoor(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char target[kMaxInputLength], direction[kMaxInputLength];
	char field[kMaxInputLength], *value;
	RoomData *rm;
	int dir, fd, to_room, lock;

	const char *door_field[] =
		{
			"purge",
			"description",
			"flags",
			"key",
			"name",
			"room",
			"lock",
			"\n"
		};

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);
	skip_spaces(&value);

	if (!*target || !*direction || !*field) {
		mob_log(ch, "mdoor called with too few args");
		return;
	}

	if ((rm = get_room(target)) == nullptr) {
		mob_log(ch, "mdoor: invalid target");
		return;
	}

	if ((dir = search_block(direction, dirs, false)) == -1) {
		mob_log(ch, "mdoor: invalid direction");
		return;
	}

	if ((fd = search_block(field, door_field, false)) == -1) {
		mob_log(ch, "mdoor: invalid field");
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
				if ((to_room = real_room(atoi(value))) != kNowhere) {
					exit->to_room(to_room);
				} else {
					mob_log(ch, "mdoor: invalid door target");
				}
				break;

			case 6:    // lock - сложность замка
				lock = atoi(value);
				if (!(lock < 0 || lock > 255))
					exit->lock_complexity = lock;
				else
					mob_log(ch, "mdoor: invalid lock complexity");
				break;
		}
	}
}

// increases spells & skills
const char *GetSpellName(int num);
int FixNameAndFindSpellNum(char *name);

void do_mfeatturn(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int isFeat = 0;
	CharData *victim;
	char name[kMaxInputLength], featname[kMaxInputLength], amount[kMaxInputLength], *pos;
	int featnum = 0, featdiff = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount) {
		mob_log(ch, "mfeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.')))
		*pos = ' ';
	while ((pos = strchr(featname, '_')))
		*pos = ' ';

	if ((featnum = FindFeatNum(featname)) > 0 && featnum < kMaxFeats)
		isFeat = 1;
	else {
		mob_log(ch, "mfeatturn: feature not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else {
		mob_log(ch, "mfeatturn: unknown set variable");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mfeatturn: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mfeatturn: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	if (isFeat)
		trg_featturn(victim, featnum, featdiff, last_trig_vnum);
}

void do_mskillturn(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], skill_name[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		return;
	}

	one_argument(two_arguments(argument, name, skill_name), amount);

	if (!*name || !*skill_name || !*amount) {
		mob_log(ch, "mskillturn: too few arguments");
		return;
	}

	auto skill_id = FixNameAndFindSkillNum(skill_name);
	bool is_skill = false;
	if (MUD::Skills().IsValid(skill_id)) {
		is_skill = true;
	} else if ((recipenum = im_get_recipe_by_name(skill_name)) < 0) {
		sprintf(buf, "mskillturn: %s skill not found", skill_name);
		mob_log(ch, buf);
		return;
	}

	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		mob_log(ch, "mskillturn: unknown set variable");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mskillturn: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mskillturn: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	}

	if (is_skill) {
		if (MUD::Classes()[victim->get_class()].HasSkill(skill_id)) {
			trg_skillturn(victim, skill_id, skilldiff, last_trig_vnum);
		} else {
			sprintf(buf, "mskillturn: skill and character class mismatch");
			mob_log(ch, buf);
		}
	} else {
		trg_recipeturn(victim, recipenum, skilldiff);
	}
}

void do_mskilladd(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	bool isSkill = false;
	CharData *victim;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		return;
	}

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount) {
		mob_log(ch, "mskilladd: too few arguments");
		return;
	}
	auto skill_id = FixNameAndFindSkillNum(skillname);
	if (MUD::Skills().IsValid(skill_id)) {
		isSkill = true;
	} else if ((recipenum = im_get_recipe_by_name(skillname)) < 0) {
		sprintf(buf, "mskilladd: %s skill/recipe not found", skillname);
		mob_log(ch, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mskilladd: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mskilladd: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	if (isSkill) {
		AddSkill(victim, skill_id, skilldiff, last_trig_vnum);
	} else {
		AddRecipe(victim, recipenum, skilldiff);
	}
}

void do_mspellturn(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int skillnum = 0, skilldiff = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		return;
	}

	argument = one_argument(argument, name);
	two_arguments(argument, skillname, amount);

	if (!*name || !*skillname || !*amount) {
		mob_log(ch, "mspellturn: too few arguments");
		return;
	}

	if ((skillnum = FixNameAndFindSpellNum(skillname)) < 0 || skillnum == 0 || skillnum > kSpellCount) {
		mob_log(ch, "mspellturn: spell not found");
		return;
	}

	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		mob_log(ch, "mspellturn: unknown set variable");
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspellturn: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mspellturn: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spellturn(victim, skillnum, skilldiff, last_trig_vnum);
}

void do_mspellturntemp(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];
	int spellnum = 0, spelltime = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		return;
	}

	argument = one_argument(argument, name);
	two_arguments(argument, spellname, amount);

	if (!*name || !*spellname || !*amount) {
		mob_log(ch, "mspellturntemp: too few arguments");
		return;
	}

	if ((spellnum = FixNameAndFindSpellNum(spellname)) < 0 || spellnum == 0 || spellnum > kSpellCount) {
		mob_log(ch, "mspellturntemp: spell not found");
		return;
	}

	spelltime = atoi(amount);

	if (spelltime < 0) {
		mob_log(ch, "mspellturntemp: time is negative");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspellturntemp: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mspellturntemp: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spellturntemp(victim, spellnum, spelltime, last_trig_vnum);
}

void do_mspelladd(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int skillnum = 0, skilldiff = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount) {
		mob_log(ch, "mspelladd: too few arguments");
		return;
	}

	if ((skillnum = FixNameAndFindSpellNum(skillname)) < 0 || skillnum == 0 || skillnum > kSpellCount) {
		mob_log(ch, "mspelladd: skill not found");
		return;
	}

	skilldiff = atoi(amount);

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspelladd: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mspelladd: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spelladd(victim, skillnum, skilldiff, last_trig_vnum);
}

void do_mspellitem(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char name[kMaxInputLength], spellname[kMaxInputLength], type[kMaxInputLength], turn[kMaxInputLength];
	int spellnum = 0, spelldiff = 0, spell = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		return;
	}

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn) {
		mob_log(ch, "mspellitem: too few arguments");
		return;
	}

	if ((spellnum = FixNameAndFindSpellNum(spellname)) < 0 || spellnum == 0 || spellnum > kSpellCount) {
		mob_log(ch, "mspellitem: spell not found");
		return;
	}

	if (!str_cmp(type, "potion")) {
		spell = kSpellPotion;
	} else if (!str_cmp(type, "wand")) {
		spell = kSpellWand;
	} else if (!str_cmp(type, "scroll")) {
		spell = kSpellScroll;
	} else if (!str_cmp(type, "items")) {
		spell = kSpellItems;
	} else if (!str_cmp(type, "runes")) {
		spell = kSpellRunes;
	} else {
		mob_log(ch, "mspellitem: type spell not found");
		return;
	}

	if (!str_cmp(turn, "set")) {
		spelldiff = 1;
	} else if (!str_cmp(turn, "clear")) {
		spelldiff = 0;
	} else {
		mob_log(ch, "mspellitem: unknown set variable");
		return;
	}

	if (*name == UID_CHAR) {
		if (!(victim = get_char(name))) {
			sprintf(buf, "mspellitem: victim (%s) UID does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	} else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		sprintf(buf, "mspellitem: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spellitem(victim, spellnum, spelldiff, spell);
}

void do_mdamage(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char name[kMaxInputLength], amount[kMaxInputLength];
	int dam = 0;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
		return;
	}

	two_arguments(argument, name, amount);

	if (!*name || !*amount || !a_isdigit(*amount)) {
		sprintf(buf, "mdamage: bad syntax, arg %s", argument);
		mob_log(ch, buf);
		return;
	}

	dam = atoi(amount);
	auto victim = get_char(name);
	if (victim) {
		if (world[IN_ROOM(victim)]->zone_rn != world[ch->in_room]->zone_rn) {
			return;
		}

		if (IS_IMMORTAL(ch) && dam > 0) {
			send_to_char
				("Будучи очень крутым, вы сделали шаг в сторону и не получили повреждений...\r\n", victim);
			return;
		}
		GET_HIT(victim) -= dam;
		if (dam < 0) {
			send_to_char("Вы почувствовали себя лучше.\r\n", victim);
			return;
		}

		update_pos(victim);
		char_dam_message(dam, victim, victim, 0);
		if (GET_POS(victim) == EPosition::kDead) {
			if (!IS_NPC(victim)) {
				sprintf(buf2,
						"%s killed by mobdamage at %s [%d]",
						GET_NAME(victim),
						IN_ROOM(victim) == kNowhere ? "kNowhere" : world[IN_ROOM(victim)]->name,
						GET_ROOM_VNUM(IN_ROOM(victim)));
				mudlog(buf2, BRF, 0, SYSLOG, true);
			}
			die(victim, ch);
		}
	} else {
		mob_log(ch, "mdamage: target not found");
	}
}

void do_mzoneecho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ZoneRnum zone;
	char zone_name[kMaxInputLength], buf[kMaxInputLength], *msg;

	msg = any_one_arg(argument, zone_name);
	skip_spaces(&msg);

	if (!*zone_name || !*msg)
		mob_log(ch, "mzoneecho called with too few args");
	else if ((zone = get_zone_rnum_by_room_vnum(atoi(zone_name))) < 0) {
		std::stringstream str_log;
		str_log << "mzoneecho called for nonexistant zone: " << zone_name;
		mob_log(ch, str_log.str().c_str());
	}
	else {
		sprintf(buf, "%s\r\n", msg);
		send_to_zone(buf, zone);
	}
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
		{"\n", EPosition::kDead, nullptr, 0, false}
		// this must be last
	};

bool mob_script_command_interpreter(CharData *ch, char *argument) {
	char *line, arg[kMaxInputLength];

	// just drop to next line for hitting CR
	skip_spaces(&argument);

	if (!*argument)
		return false;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;
	const size_t length = strlen(arg);

	while (*mob_cmd_info[cmd].command != '\n') {
		if (!strncmp(mob_cmd_info[cmd].command, arg, length)) {
			break;
		}
		cmd++;
	}
// damage mtrigger срабатывает всегда
	if (!SCRIPT_CHECK(ch, MTRIG_DAMAGE)) {
		if (!mob_cmd_info[cmd].use_in_lag && 
				(GET_MOB_HOLD(ch)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT))) {
		return false;
		}
	}
	if (*mob_cmd_info[cmd].command == '\n') {
		return false;
	} else if (GET_POS(ch) < mob_cmd_info[cmd].minimum_position) {
		return false;
	} else {
		check_hiding_cmd(ch, -1);

		const mob_command_info::handler_f &command = mob_cmd_info[cmd].command_pointer;
		command(ch, line, cmd, mob_cmd_info[cmd].subcmd);

		return true;
	}
}

// *  This is the command interpreter used by mob, include common interpreter's commands
void mob_command_interpreter(CharData *ch, char *argument) {
	if (!mob_script_command_interpreter(ch, argument)) {
		command_interpreter(ch, argument);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
