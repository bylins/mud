/**************************************************************************
*  File:  dg_objcmd.cpp                                  Part of Bylins   *
*  Usage: contains the command_interpreter for objects,                   *
*         object commands.                                                *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "engine/entities/char_data.h"
#include "engine/ui/cmd/do_follow.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/pk.h"
#include "engine/core/handler.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/skills/townportal.h"
#include "utils/id_converter.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/damage.h"

extern const char *dirs[];
extern int up_obj_where(ObjData *obj);
extern int reloc_target;

CharData *get_char_by_obj(ObjData *obj, const char *name);
ObjData *get_obj_by_obj(ObjData *obj, const char *name);
void sub_write(char *arg, CharData *ch, byte find_invis, int targets);
void die(CharData *ch, CharData *killer);
void obj_command_interpreter(ObjData *obj, char *argument, Trigger *trig);
void send_to_zone(char *messg, int zone_rnum);

RoomData *get_room(const char *name);

bool mob_script_command_interpreter(CharData *ch, char *argument, Trigger *trig);

struct obj_command_info {
	const char *command;
	typedef void(*handler_f)(ObjData *obj, char *argument, int cmd, int subcmd, Trigger *trig);
	handler_f command_pointer;
	int subcmd;
};

// do_osend
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1

// attaches object name and vnum to msg_set and sends it to script_log
void obj_log(ObjData *obj, Trigger *trig, const char *msg, LogMode type = LogMode::OFF) {
	char small_buf[kMaxInputLength + 100];

	snprintf(small_buf, kMaxInputLength + 100,
			"(Obj: '%s', VNum: %d, trig: %d): %s [строка: %d]", obj->get_short_description().c_str(), GET_OBJ_VNUM(obj),
			trig_index[(trig)->get_rnum()]->vnum, msg, last_trig_line_num);
	script_log(small_buf, type);
}

// returns the real room number that the object or object's carrier is in
int obj_room(ObjData *obj) {
	if (obj->get_in_room() != kNowhere) {
		return obj->get_in_room();
	} else if (obj->get_carried_by()) {
		return obj->get_carried_by()->in_room;
	} else if (obj->get_worn_by()) {
		return obj->get_worn_by()->in_room;
	} else if (obj->get_in_obj()) {
		return obj_room(obj->get_in_obj());
	} else {
		return kNowhere;
	}
}

// returns the real room number, or kNowhere if not found or invalid
int find_obj_target_room(ObjData *obj, Trigger *trig, char *rawroomstr) {
	char roomstr[kMaxInputLength];
	RoomRnum location = kNowhere;

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		sprintf(buf, "Undefined oteleport room: %s", rawroomstr);
		obj_log(obj, trig, buf);
		return kNowhere;
	}

	auto tmp = atoi(roomstr);
	if (tmp > 0) {
		location = GetRoomRnum(tmp);
	} else {
		sprintf(buf, "Undefined oteleport room: %s", roomstr);
		obj_log(obj, trig, buf);
		return kNowhere;
	}

	return location;
}

void do_oportal(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	int target, howlong, curroom, nr;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		obj_log(obj, trig, "oportal: called with too few args");
		return;
	}

	howlong = atoi(arg2);
	nr = atoi(arg1);
	target = GetRoomRnum(nr);

	if (target == kNowhere) {
		obj_log(obj, trig, "oportal: target is an invalid room");
		return;
	}

	/* Ставим пентаграмму из текущей комнаты в комнату target с
	   длительностью howlong */
	curroom = GetRoomRnum(get_room_where_obj(obj));
	one_way_portal::ReplacePortalTimer(nullptr, curroom, target, howlong * 30 - 1);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), 0, 0, kToChar);
	act("Лазурная пентаграмма возникла в воздухе.",
		false, world[curroom]->first_character(), 0, 0, kToRoom);
}
// Object commands
void do_oecho(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	skip_spaces(&argument);

	int room;
	if (!*argument) {
		obj_log(obj, trig, "oecho called with no args");
	} else if ((room = obj_room(obj)) != kNowhere) {
		if (!world[room]->people.empty()) {
			sub_write(argument, world[room]->first_character(), true, kToRoom | kToChar);
		}
	}
}
void do_oat(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
//	int location;
	char roomstr[kMaxInputLength];
	RoomRnum location = kNowhere;
	if (!*argument) {
		obj_log(obj, trig, "oat: bad argument");
		return;
	}
	one_argument(argument, roomstr);
	auto tmp = atoi(roomstr);
	if (tmp > 0) {
		location = GetRoomRnum(tmp);
	} else {
		sprintf(buf, "oat: invalid location '%d'", tmp);
		obj_log(obj, trig, buf);
		return;
	}
	argument = one_argument(argument, roomstr);
	auto tmp_obj = world_objects.create_from_prototype_by_vnum(obj->get_vnum());
	tmp_obj->set_in_room(location);
	obj_command_interpreter(tmp_obj.get(), argument, trig);
	world_objects.remove(tmp_obj);
}

void do_oforce(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char arg1[kMaxInputLength], *line;

	line = one_argument(argument, arg1);

	if (!*arg1 || !*line) {
		obj_log(obj, trig, "oforce called with too few args");

		return;
	}

	if (!str_cmp(arg1, "all")
		|| !str_cmp(arg1, "все")) {
		obj_log(obj, trig, "ERROR: \'oforce all\' command disabled.");
		return;

		/*if ((room = obj_room(obj)) == kNowhere)
		{
			obj_log(obj, trig, "oforce called by object in kNowhere");
		}
		else
		{
			const auto people_copy = world[room]->people;
			for (const auto ch : people_copy)
			{
				if (ch->IsNpc()
					|| GetRealLevel(ch) < kLevelImmortal)
				{
					command_interpreter(ch, line);
				}
			}
		}*/
	} else {
		if ((ch = get_char_by_obj(obj, arg1))) {
			// если чар в ЛД
			if (!ch->IsNpc()) {
				if (!ch->desc) {
					return;
				}
			}

			if (ch->IsNpc()) {
				if (mob_script_command_interpreter(ch, line, trig)) {
					obj_log(obj, trig, "Mob trigger commands in oforce. Please rewrite trigger.");
					return;
				}

				command_interpreter(ch, line);
			} else if (GetRealLevel(ch) < kLvlImmortal) {
				command_interpreter(ch, line);
			}
			else
				obj_log(obj, trig, "oforce: попытка принудить бессмертного.");
		} else {
			obj_log(obj, trig, "oforce: no target found");
		}
	}
}

void do_osend(ObjData *obj, char *argument, int/* cmd*/, int subcmd, Trigger *trig) {
	char buf[kMaxInputLength], *msg;
	CharData *ch;

	msg = any_one_arg(argument, buf);

	if (!*buf) {
		obj_log(obj, trig, "osend called with no args");
		return;
	}

	skip_spaces(&msg);

	if (!*msg) {
		obj_log(obj, trig, "osend called without a message");
		return;
	}
	if ((ch = get_char_by_obj(obj, buf))) {
		if (subcmd == SCMD_OSEND)
			sub_write(msg, ch, true, kToChar);
		else if (subcmd == SCMD_OECHOAROUND)
			sub_write(msg, ch, true, kToRoom);
	} else {
		if (*buf != UID_CHAR && *buf != UID_CHAR_ALL) {
			sprintf(buf1, "no target (%s) found for osend", buf);
			obj_log(obj, trig, buf1);
		}
	}
}

// increases the target's exp
void do_oexp(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char name[kMaxInputLength], amount[kMaxInputLength];

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		obj_log(obj, trig, "oexp: too few arguments");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		EndowExpToChar(ch, atoi(amount));
		sprintf(buf, "oexp: victim (%s) получил опыт %d", GET_NAME(ch), atoi(amount));
		obj_log(obj, trig, buf);
	} else {
		obj_log(obj, trig, "oexp: target not found");
		return;
	}
}

// set the object's timer value
void do_otimer(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];

	one_argument(argument, arg);

	if (!*arg)
		obj_log(obj, trig, "otimer: missing argument");
	else if (!a_isdigit(*arg))
		obj_log(obj, trig, "otimer: bad argument");
	else {
		obj->set_timer(atoi(arg));
	}
}

// transform into a different object
// note: this shouldn't be used with containers unless both objects
// are containers!
void do_otransform(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *wearer = nullptr;
	int pos = -1;

	one_argument(argument, arg);

	if (!*arg) {
		obj_log(obj, trig, "otransform: missing argument");
	} else if (!a_isdigit(*arg)) {
		obj_log(obj, trig, "otransform: bad argument");
	} else {
		const auto o = world_objects.create_from_prototype_by_vnum(atoi(arg));
		if (o == nullptr) {
			obj_log(obj, trig, "otransform: bad object vnum");
			return;
		}
		// Описание работы функции см. в mtransform()

		if (obj->get_worn_by()) {
			pos = obj->get_worn_on();
			wearer = obj->get_worn_by();
			UnequipChar(obj->get_worn_by(), pos, CharEquipFlags());
		}
		obj->swap(*o);
		if (o->has_flag(EObjFlag::kTicktimer)) {
			obj->set_extra_flag(EObjFlag::kTicktimer);
		}

		if (wearer) {
			EquipObj(wearer, obj, pos, CharEquipFlags());
		}
		world_objects.AddToExtractedList(o.get());
	}
}

// purge all objects an npcs in room, or specified object or mob
void do_opurge(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg[kMaxInputLength];
	CharData *ch;
	ObjData *o;

	one_argument(argument, arg);

	if (!*arg) {
		return;
	}

	if (!(ch = get_char_by_obj(obj, arg))) {
		if ((o = get_obj_by_obj(obj, arg))) {
			log("Purge obj #%d by %s (opurge)", GET_OBJ_VNUM(o), arg);
			if (o == obj) {
				trig->halt();
			}
			world_objects.AddToExtractedList(o);
		} else
			obj_log(obj, trig, "opurge: bad argument");
		return;
	}

	if (!ch->IsNpc()) {
		obj_log(obj, trig, "opurge: purging a PC");
		return;
	}

	if (ch->followers
		|| ch->has_master()) {
		die_follower(ch);
	}
	character_list.AddToExtractedList(ch);
}

void do_oteleport(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch, *lastchar = nullptr;
	bool onhorse = false;
	int target, rm;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		obj_log(obj, trig, "oteleport called with too few args");
		return;
	}

	target = find_obj_target_room(obj, trig, arg2);

	if (target == kNowhere) {
		obj_log(obj, trig, "oteleport target is an invalid room");
		return;
	}
	rm = obj_room(obj);
	if (rm == kNowhere) {
		obj_log(obj, trig, "oteleport called in kNowhere");
		return;
	}
	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все")) {
		const auto people_copy = world[rm]->people;
		decltype(world[rm]->people)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch) {
			const auto ch = *ch_i;
			++next_ch;
			if (ch->in_room == kNowhere) {
				obj_log(obj, trig, "oteleport transports from kNowhere");
				return;
			}
			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, target);
			if (!ch->IsNpc()) {
				look_at_room(ch, true);
				lastchar = ch;
			}
		}
		if (lastchar) {
			greet_mtrigger(lastchar, -1);
			greet_otrigger(lastchar, -1);
		}
	} else if (!str_cmp(arg1, "allchar") || !str_cmp(arg1, "всечары")) {
		const auto people_copy = world[rm]->people;
		decltype(world[rm]->people)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch) {
			const auto ch = *ch_i;
			++next_ch;
			if (ch->in_room == kNowhere) {
				obj_log(obj, trig, "oteleport transports allchar from kNowhere");
				return;
			}
			if (ch->IsNpc() && !IS_CHARMICE(ch)) {
				continue;
			}
			if (target == ch->in_room) {
//				obj_log(obj, trig, "oteleport allchar: target is itself");
				continue;
			}

			if (ch->get_horse()) {
				if (ch->IsOnHorse() || ch->has_horse(true)) {
					RemoveCharFromRoom(ch->get_horse());
					PlaceCharToRoom(ch->get_horse(), target);
					onhorse = true;
				}
			}
			if (target == ch->in_room) {
				obj_log(obj, trig, "oteleport allchar: target is itself");
			}
			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, target);
			if (!onhorse)
				ch->dismount();
			if (!ch->IsNpc()) {
				look_at_room(ch, true);
				lastchar = ch;
			}
		}
		if (lastchar) {
			greet_mtrigger(lastchar, -1);
			greet_otrigger(lastchar, -1);
		}
	} else {
		if (!(ch = get_char_by_obj(obj, arg1))) {
			obj_log(obj, trig, "oteleport: no target found");
			return;
		}
		if (target == ch->in_room) {
//			obj_log(obj, trig, "oteleport: target is itself");
			return;
		}
		if (IS_CHARMICE(ch) && ch->in_room == ch->get_master()->in_room)
			ch = ch->get_master();
		const auto people_copy = world[ch->in_room]->people;
		for (const auto charmee: people_copy) {
			if (IS_CHARMICE(charmee) && charmee->get_master() == ch) {
				RemoveCharFromRoom(charmee);
				PlaceCharToRoom(charmee, target);
			}
		}
		if (ch->get_horse()) {
			if (ch->IsOnHorse() || ch->has_horse(true)) {
				RemoveCharFromRoom(ch->get_horse());
				PlaceCharToRoom(ch->get_horse(), target);
				onhorse = true;
			}
		}
		if (target == ch->in_room) {
			obj_log(obj, trig, "oteleport: target is itself");
		}
		RemoveCharFromRoom(ch);
		PlaceCharToRoom(ch, target);
		if (!onhorse)
			ch->dismount();
		look_at_room(ch, true);
		greet_mtrigger(ch, -1);
		greet_otrigger(ch, -1);
	}
}

void do_dgoload(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	CharData *mob;
	char uid[kMaxInputLength], varname[kMaxInputLength] = "LoadedUid";
	char uid_type;
	int number = 0, idnum, room;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
		obj_log(obj, trig, "oload: bad syntax");
		return;
	}

	if ((room = obj_room(obj)) == kNowhere) {
		obj_log(obj, trig, "oload: object in kNowhere trying to load");
		return;
	}

	if (utils::IsAbbr(arg1, "mob")) {
		if ((mob = ReadMobile(number, kVirtual)) == nullptr) {
			obj_log(obj, trig, "oload: bad mob vnum");
			return;
		}
		uid_type = UID_CHAR;
		idnum = mob->get_uid();
		PlaceCharToRoom(mob, room);
		load_mtrigger(mob);
	} else if (utils::IsAbbr(arg1, "obj")) {
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object) {
			obj_log(obj, trig, "oload: bad object vnum");
			return;
		}
		if (GetObjMIW(object->get_rnum()) >= 0 && obj_proto.actual_count(object->get_rnum()) > GetObjMIW(object->get_rnum())) {
			if (!stable_objs::IsTimerUnlimited(obj_proto[object->get_rnum()].get())) {
				sprintf(buf, "oload: Попытка загрузить предмет больше чем в MIW для #%d.", number);
				obj_log(obj, trig, buf);
//				extract_obj(object.get());
//				return;
			}
		}
		log("Load obj #%d by %s (oload)", number, obj->get_aliases().c_str());
		object->set_vnum_zone_from(zone_table[world[room]->zone_rn].vnum);
		uid_type = UID_OBJ;
		idnum = object->get_id();
		PlaceObjToRoom(object.get(), room);
		load_otrigger(object.get());
	} else {
		obj_log(obj, trig, "oload: bad type");
		return;
	}
	sprintf(uid, "%c%d", uid_type, idnum);
	add_var_cntx(trig->var_list, varname, uid, 0);
}

void ApplyDamage(CharData* target, int damage) {
	target->set_hit(target->get_hit() - damage);
	update_pos(target);
	char_dam_message(damage, target, target, 0);
	if (target->GetPosition() == EPosition::kDead) {
		if (!target->IsNpc()) {
			sprintf(buf2, "%s killed by odamage at %s [%d]", GET_NAME(target),
					target->in_room == kNowhere ? "NOWHERE" : world[target->in_room]->name, GET_ROOM_VNUM(target->in_room));
			mudlog(buf2, BRF, kLvlBuilder, SYSLOG, true);
		}
		die(target, nullptr);
	}
}

void do_odamage(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char name[kMaxInputLength], amount[kMaxInputLength], damage_type[kMaxInputLength];
	three_arguments(argument, name, amount, damage_type);
	if (!*name || !*amount || !a_isdigit(*amount)) {
		sprintf(buf, "odamage: bad syntax, команда: %s", argument);
		obj_log(obj, trig, buf);
		return;
	}

	int dam = atoi(amount);

	CharData *ch = get_char_by_obj(obj, name);
	if (!ch) {
		sprintf(buf, "odamage: target not found, команда: %s", argument);
		obj_log(obj, trig, buf);
		return;
	}
	if (world[ch->in_room]->zone_rn != world[up_obj_where(obj)]->zone_rn) {
		return;
	}

	if (IS_IMMORTAL(ch)) {
		SendMsgToChar("Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.", ch);
		return;
	}

	CharData *damager = dg_caster_owner_obj(obj);
	if (!damager || damager == ch) {
		ApplyDamage(ch, dam);
	} else {
		const std::map<std::string, fight::DmgType> kDamageTypes = {
			{"physic", fight::kPhysDmg},
			{"magic", fight::kMagicDmg},
			{"poisonous", fight::kPoisonDmg}
		};
		if (!may_kill_here(damager, ch, name)) {
			return;
		}
		fight::DmgType type = fight::kPureDmg;
		if (*damage_type) {
			try {
				type = kDamageTypes.at(damage_type);
			} catch (const std::out_of_range &) {
				obj_log(obj, trig, "odamage: incorrect damage type.");;
			}
			die(ch, nullptr);
		}
		Damage odamage(SimpleDmg(kTypeTriggerdeath), dam, type);
		odamage.Process(damager, ch);
	}
}

void do_odoor(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char target[kMaxInputLength], direction[kMaxInputLength];
	char field[kMaxInputLength], *value;
	RoomData *rm;
	int dir, fd, to_room, lock;
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

	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);

	if (!*target || !*direction || !*field) {
		sprintf(buf, "odoor called with too few args, команда: %s", argument);
		obj_log(obj, trig, buf);
		return;
	}

	if ((rm = get_room(target)) == nullptr) {
		sprintf(buf, "odoor: invalid target, команда: %s", argument);
		obj_log(obj, trig, buf);
		return;
	}

	if ((dir = search_block(direction, dirs, false)) == -1) {
		sprintf(buf, "odoor: invalid direction, команда: %s", argument);
		obj_log(obj, trig, buf);
		return;
	}

	if ((fd = search_block(field, door_field, false)) == -1) {
		sprintf(buf, "odoor: invalid field, команда: %s", argument);
		obj_log(obj, trig, buf);
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
					obj_log(obj, trig, "odoor: invalid door target");
				}
				break;

			case 6:    // lock - сложность замка
				lock = atoi(value);
				if (!(lock < 0 || lock > 255))
					exit->lock_complexity = lock;
				else
					obj_log(obj, trig, "odoor: invalid lock complexity");
				break;
		}
	}
}

void do_osetval(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	int position, new_value;

	two_arguments(argument, arg1, arg2);
	if (!*arg1 || !*arg2 || !is_number(arg1) || !is_number(arg2)) {
		obj_log(obj, trig, "osetval: bad syntax");
		return;
	}

	position = atoi(arg1);
	new_value = atoi(arg2);
	if (position >= 0 && position < ObjData::VALS_COUNT) {
		obj->set_val(position, new_value);
	} else {
		obj_log(obj, trig, "osetval: position out of bounds!");
	}
}

void do_ofeatturn(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	int isFeat = 0;
	CharData *ch;
	char name[kMaxInputLength], featname[kMaxInputLength], amount[kMaxInputLength], *pos;
	int featdiff = 0;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount) {
		obj_log(obj, trig, "ofeatturn: too few arguments");
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
		sprintf(buf, "ofeatturn: '%s' feat not found", featname);
		obj_log(obj, trig, buf);
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else {
		obj_log(obj, trig, "ofeatturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_obj(obj, name))) {
		obj_log(obj, trig, "ofeatturn: target not found");
		return;
	}

	if (isFeat)
		trg_featturn(ch, feat_id, featdiff, last_trig_vnum);
}

void do_oskillturn(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char name[kMaxInputLength], skill_name[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skill_name), amount);

	if (!*name || !*skill_name || !*amount) {
		obj_log(obj, trig, "oskillturn: too few arguments");
		return;
	}

	auto skill_id = FixNameAndFindSkillId(skill_name);
	bool is_skill = false;
	if (MUD::Skills().IsValid(skill_id)) {
		is_skill = true;
	} else if ((recipenum = im_get_recipe_by_name(skill_name)) < 0) {
		sprintf(buf, "oskillturn: %s skill not found", skill_name);
		obj_log(obj, trig, buf);
		return;
	}

	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		obj_log(obj, trig, "oskillturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_obj(obj, name))) {
		obj_log(obj, trig, "oskillturn: target not found");
		return;
	}

	if (is_skill) {
		if (MUD::Class(ch->GetClass()).skills[skill_id].IsAvailable()) {
			trg_skillturn(ch, skill_id, skilldiff, last_trig_vnum);
		} else {
			sprintf(buf, "oskillturn: skill and character class mismatch");
			obj_log(obj, trig, buf);
		}
	} else {
		trg_recipeturn(ch, recipenum, skilldiff);
	}
}

void do_oskilladd(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	bool isSkill = false;
	CharData *ch;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount) {
		obj_log(obj, trig, "oskilladd: too few arguments");
		return;
	}
	auto skillnum = FixNameAndFindSkillId(skillname);
	if (MUD::Skills().IsValid(skillnum)) {
		isSkill = true;
	} else if ((recipenum = im_get_recipe_by_name(skillname)) < 0) {
		sprintf(buf, "oskilladd: %s skill/recipe not found", skillname);
		obj_log(obj, trig, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (!(ch = get_char_by_obj(obj, name))) {
		obj_log(obj, trig, "oskilladd: target not found");
		return;
	}

	if (isSkill) {
		AddSkill(ch, skillnum, skilldiff, last_trig_vnum);
	} else {
		AddRecipe(ch, recipenum, skilldiff);
	}
}

void do_ospellturn(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];
	int spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		obj_log(obj, trig, "ospellturn: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		obj_log(obj, trig, "ospellturn: spell not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		spelldiff = 1;
	else if (!str_cmp(amount, "clear"))
		spelldiff = 0;
	else {
		obj_log(obj, trig, "ospellturn: unknown set variable");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spellturn(ch, spell_id, spelldiff, last_trig_vnum);
	} else {
		obj_log(obj, trig, "ospellturn: target not found");
		return;
	}
}

void do_ospellturntemp(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		obj_log(obj, trig, "ospellturntemp: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		obj_log(obj, trig, "ospellturntemp: spell not found");
		return;
	}

	auto spelltime = atoi(amount);

	if (spelltime <= 0) {
		obj_log(obj, trig, "ospellturntemp: time is zero or negative");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spellturntemp(ch, spell_id, spelltime, last_trig_vnum);
	} else {
		obj_log(obj, trig, "ospellturntemp: target not found");
		return;
	}
}

void do_ospelladd(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount) {
		obj_log(obj, trig, "ospelladd: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		obj_log(obj, trig, "ospelladd: spell not found");
		return;
	}

	auto spelldiff = atoi(amount);

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spelladd(ch, spell_id, spelldiff, last_trig_vnum);
	} else {
		obj_log(obj, trig, "ospelladd: target not found");
		return;
	}
}

void do_ospellitem(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], type[kMaxInputLength], turn[kMaxInputLength];

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn) {
		obj_log(obj, trig, "ospellitem: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		obj_log(obj, trig, "ospellitem: spell not found");
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
		obj_log(obj, trig, "ospellitem: type spell not found");
		return;
	}

	auto spelldiff{0};
	if (!str_cmp(turn, "set")) {
		spelldiff = 1;
	} else if (!str_cmp(turn, "clear")) {
		spelldiff = 0;
	} else {
		obj_log(obj, trig, "ospellitem: unknown set variable");
		return;
	}

	if ((ch = get_char_by_obj(obj, name))) {
		trg_spellitem(ch, spell_id, spelldiff, spell);
	} else {
		obj_log(obj, trig, "ospellitem: target not found");
		return;
	}
}

void do_ozoneecho(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	ZoneRnum zone;
	char zone_name[kMaxInputLength], buf[kMaxInputLength], *msg;

	msg = any_one_arg(argument, zone_name);
	skip_spaces(&msg);

	if (!*zone_name || !*msg)
		obj_log(obj, trig, "ozoneecho called with too few args");
	else if ((zone = get_zone_rnum_by_vnumum(atoi(zone_name))) < 0) {
		std::stringstream str_log;
		str_log << "ozoneecho called for nonexistant zone: " << zone_name;
		obj_log(obj, trig, str_log.str().c_str());
	} else {
		sprintf(buf, "%s\r\n", msg);
		send_to_zone(buf, zone);
	}
}
// для команды oat
void ObjDgCast(ObjData *obj, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char *dg_arg = str_dup("DgCast ");
	strcat(dg_arg, argument);
	do_dg_cast(obj, trig, OBJ_TRIGGER, argument);
	free(dg_arg);
}

const struct obj_command_info obj_cmd_info[] =
	{
		{"RESERVED", 0, 0},    // this must be first -- for specprocs
		{"oat", do_oat, 0},
		{"oecho", do_oecho, 0},
		{"oechoaround", do_osend, SCMD_OECHOAROUND},
		{"oexp", do_oexp, 0},
		{"oforce", do_oforce, 0},
		{"oload", do_dgoload, 0},
		{"opurge", do_opurge, 0},
		{"osend", do_osend, SCMD_OSEND},
		{"osetval", do_osetval, 0},
		{"oteleport", do_oteleport, 0},
		{"odamage", do_odamage, 0},
		{"otimer", do_otimer, 0},
		{"otransform", do_otransform, 0},
		{"odoor", do_odoor, 0},
		{"ospellturn", do_ospellturn, 0},
		{"ospellturntemp", do_ospellturntemp, 0},
		{"ospelladd", do_ospelladd, 0},
		{"ofeatturn", do_ofeatturn, 0},
		{"oskillturn", do_oskillturn, 0},
		{"oskilladd", do_oskilladd, 0},
		{"ospellitem", do_ospellitem, 0},
		{"oportal", do_oportal, 0},
		{"ozoneecho", do_ozoneecho, 0},
		{"at", do_oat, 0},
		{"echo", do_oecho, 0},
		{"echoaround", do_osend, SCMD_OECHOAROUND},
		{"exp", do_oexp, 0},
		{"force", do_oforce, 0},
		{"load", do_dgoload, 0},
		{"purge", do_opurge, 0},
		{"send", do_osend, SCMD_OSEND},
		{"setval", do_osetval, 0},
		{"teleport", do_oteleport, 0},
		{"damage", do_odamage, 0},
		{"timer", do_otimer, 0},
		{"transform", do_otransform, 0},
		{"door", do_odoor, 0},
		{"spellturn", do_ospellturn, 0},
		{"spellturntemp", do_ospellturntemp, 0},
		{"spelladd", do_ospelladd, 0},
		{"featturn", do_ofeatturn, 0},
		{"skillturn", do_oskillturn, 0},
		{"skilladd", do_oskilladd, 0},
		{"spellitem", do_ospellitem, 0},
		{"portal", do_oportal, 0},
		{"zoneecho", do_ozoneecho, 0},
		{"dgcast", ObjDgCast, 0},
		{"\n", 0, 0}        // this must be last
	};

// *  This is the command interpreter used by objects, called by script_driver.
void obj_command_interpreter(ObjData *obj, char *argument, Trigger *trig) {
	char *line, arg[kMaxInputLength];

	skip_spaces(&argument);

	// just drop to next line for hitting CR
	if (!*argument)
		return;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;
	while (*obj_cmd_info[cmd].command != '\n') {
		if (!strcmp(obj_cmd_info[cmd].command, arg)) {
			break;
		}
		cmd++;
	}

	if (*obj_cmd_info[cmd].command == '\n') {
		sprintf(buf2, "Unknown object cmd: '%s'", argument);
		obj_log(obj, trig, buf2, LGH);
	} else {
		const obj_command_info::handler_f &command = obj_cmd_info[cmd].command_pointer;
		command(obj, line, cmd, obj_cmd_info[cmd].subcmd, trig);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
