/**************************************************************************
*  File: dg_wldcmd.cpp                                     Part of Bylins *
*  Usage: contains the command_interpreter for rooms,                     *
*         room commands.                                                  *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "entities/char_data.h"
#include "cmd/follow.h"
#include "game_fight/fight.h"
#include "handler.h"
#include "utils/id_converter.h"
#include "obj_prototypes.h"
#include "game_skills/townportal.h"
#include "game_magic/magic_utils.h"
//#include "entities/zone.h"
#include "structs/global_objects.h"

extern const char *dirs[];

void die(CharData *ch, CharData *killer);
void sub_write(char *arg, CharData *ch, byte find_invis, int targets);
void send_to_zone(char *messg, int zone_rnum);
CharData *get_char_by_room(RoomData *room, char *name);
RoomData *get_room(char *name);
ObjData *get_obj_by_room(RoomData *room, char *name);

bool mob_script_command_interpreter(CharData *ch, char *argument, Trigger *trig);

extern int reloc_target;
extern Trigger *cur_trig;

struct wld_command_info {
	const char *command;
	typedef void (*handler_f)(RoomData *room, char *argument, int cmd, int subcmd, Trigger *trig);
	handler_f command_pointer;
	int subcmd;
};


// do_wsend
#define SCMD_WSEND        0
#define SCMD_WECHOAROUND  1

// attaches room vnum to msg_set and sends it to script_log
void wld_log(RoomData *room, const char *msg, LogMode type = LogMode::OFF) {
	char buf[kMaxInputLength + 100];

	sprintf(buf, "(Room: %d, trig: %d): %s", room->room_vn, last_trig_vnum, msg);
	script_log(buf, type);
}

// sends str to room
void act_to_room(char *str, RoomData *room) {
	// no one is in the room
	if (!room->first_character()) {
		return;
	}

	/*
	 * since you can't use act(..., TO_ROOM) for an room, send it
	 * TO_ROOM and TO_CHAR for some char in the room.
	 * (just dont use $n or you might get strange results)
	 */
	act(str, false, room->first_character(), 0, 0, kToRoom);
	act(str, false, room->first_character(), 0, 0, kToChar);
}

// World commands

// prints the argument to all the rooms aroud the room
void do_wasound(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	int door;

	skip_spaces(&argument);

	if (!*argument) {
		wld_log(room, "wasound called with no argument");
		return;
	}

	for (door = 0; door < EDirection::kMaxDirNum; door++) {
		const auto &exit = room->dir_option[door];

		if (exit
			&& (exit->to_room() != kNowhere)
			&& room != world[exit->to_room()]) {
			act_to_room(argument, world[exit->to_room()]);
		}
	}
}

void do_wecho(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	skip_spaces(&argument);

	if (!*argument)
		wld_log(room, "wecho called with no args");
	else
		act_to_room(argument, room);
}

void do_wsend(RoomData *room, char *argument, int/* cmd*/, int subcmd, Trigger *) {
	char buf[kMaxInputLength], *msg;
	CharData *ch;

	msg = any_one_arg(argument, buf);

	if (!*buf) {
		wld_log(room, "wsend called with no args");
		return;
	}

	skip_spaces(&msg);

	if (!*msg) {
		wld_log(room, "wsend called without a message");
		return;
	}

	if ((ch = get_char_by_room(room, buf))) {
		if (reloc_target != -1 && reloc_target != ch->in_room) {
			sprintf(buf,
					"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
					GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
			mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
		}
		if (subcmd == SCMD_WSEND)
			sub_write(msg, ch, true, kToChar);
		else if (subcmd == SCMD_WECHOAROUND)
			sub_write(msg, ch, true, kToRoom);
	} else {
		if (*buf != UID_CHAR && *buf != UID_CHAR_ALL) {
			sprintf(buf1, "no target (%s) found for wsend", buf);
			wld_log(room, buf1);
		}
	}
}

void do_wzoneecho(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	ZoneRnum zone;
	char zone_name[kMaxInputLength], buf[kMaxInputLength], *msg;

	msg = any_one_arg(argument, zone_name);
	skip_spaces(&msg);

	if (!*zone_name || !*msg)
		wld_log(room, "wzoneecho called with too few args");
	else if ((zone = get_zone_rnum_by_room_vnum(atoi(zone_name))) < 0) {
		std::stringstream str_log;
		str_log << "wzoneecho called for nonexistant zone: " << zone_name;
		wld_log(room, str_log.str().c_str());
	}
	else {
		sprintf(buf, "%s\r\n", msg);
		send_to_zone(buf, zone);
	}
}

void do_wdoor(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
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
	strcpy(error, argument);
	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);
	skip_spaces(&value);
	if (!*target || !*direction || !*field) {
		wld_log(room, "wdoor called with too few args");
		sprintf(buf, "wdoor argument: %s", error);
		wld_log(room, buf);
		return;
	}
	if ((rm = get_room(target)) == nullptr) {
		wld_log(room, "wdoor: invalid target");
		sprintf(buf, "wdoor argument: %s", error);
		wld_log(room, buf);
		return;
	}
	if ((dir = search_block(direction, dirs, false)) == -1) {
		wld_log(room, "wdoor: invalid direction");
		sprintf(buf, "wdoor argument: %s", error);
		wld_log(room, buf);
		return;
	}
	if ((fd = search_block(field, door_field, false)) == -1) {
		wld_log(room, "wdoor: invalid field");
		sprintf(buf, "wdoor argument: %s", error);
		wld_log(room, buf);
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
			case 1:    // description //
				exit->general_description = std::string(value) + "\r\n";
				break;
			case 2:    // flags       //
				asciiflag_conv(value, &exit->exit_info);
				break;
			case 3:    // key         //
				exit->key = atoi(value);
				break;
			case 4:    // name        //
				exit->set_keywords(value);
				break;
			case 5:    // room        //
				if ((to_room = real_room(atoi(value))) != kNowhere) {
					exit->to_room(to_room);
				} else {
					wld_log(room, "wdoor: invalid door target");
				}
				break;
			case 6:    // lock - сложность замка         //
				lock = atoi(value);
				if (!(lock < 0 || lock > 255))
					exit->lock_complexity = lock;
				else
					wld_log(room, "wdoor: invalid lock complexity");
				break;
		}
	}
}

void do_wteleport(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch, *lastchar = nullptr;
	bool onhorse = false;
	int target, nr;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		wld_log(room, "wteleport called with too few args");
		return;
	}
	nr = atoi(arg2);
	if (nr > 0) {
		target = real_room(nr);
	} else {
		sprintf(buf, "Undefined wteleport room: %s", arg2);
		wld_log(room, buf);
		return;
	}

	if (target == kNowhere) {
		wld_log(room, "wteleport target is an invalid room");
		return;
	}
	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все")) {
		const auto people_copy = room->people;
		decltype(people_copy)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch) {
			const auto ch = *ch_i;
			++next_ch;
			if (target == ch->in_room) {
//				wld_log(room, "wteleport all target is itself");
				continue;
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
	}
	else if (!str_cmp(arg1, "allchar") || !str_cmp(arg1, "всечары")) {
		const auto people_copy = room->people;
		decltype(people_copy)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch) {
			const auto ch = *ch_i;
			++next_ch;
			if (ch->IsNpc() && !IS_CHARMICE(ch)) {
				continue;
			}
			if (target == ch->in_room) {
//				wld_log(room, "wteleport allchar target is itself");
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
				wld_log(room, "wteleport allchar target is itself");
//				return;
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
		if ((ch = get_char_by_room(room, arg1))) { //уид ищется внутри
			if (target == ch->in_room) {
//				wld_log(room, "wteleport target is itself");
				return;
			}
			if (IS_CHARMICE(ch) && ch->in_room == ch->get_master()->in_room)
				ch = ch->get_master();
			const auto people_copy = world[ch->in_room]->people;
			for (const auto charmee : people_copy) {
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
				wld_log(room, "wteleport target is itself");
//				return;
			}
			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, target);
			if (!onhorse)
				ch->dismount();
			look_at_room(ch, true);
			greet_mtrigger(ch, -1);
			greet_otrigger(ch, -1);
		} else {
			wld_log(room, "wteleport: no target found");
			return;
		}
	}
}

void do_wforce(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg1[kMaxInputLength], *line;

	line = one_argument(argument, arg1);

	if (!*arg1 || !*line) {
		wld_log(room, "wforce called with too few args");
		return;
	}

	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все")) {
		wld_log(room, "ERROR: \'wforce all\' command disabled.");
		return;
	} else {
		const auto ch = get_char_by_room(room, arg1);
		if (ch) {
			if (!ch->IsNpc()) {
				if ((!ch->desc)) {
					return;
				}
			}

			if (ch->IsNpc()) {
				if (mob_script_command_interpreter(ch, line, trig)) {
					wld_log(room, "Mob trigger commands in wforce. Please rewrite trigger.");
					return;
				}

				command_interpreter(ch, line);
			} else if (GetRealLevel(ch) < kLvlImmortal) {
				command_interpreter(ch, line);
			}
			else
				wld_log(room, "wforce: попытка принудить бессмертного.");
		} else {
			wld_log(room, "wforce: no target found");
		}
	}
}

// increases the target's exp //
void do_wexp(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], amount[kMaxInputLength];

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		wld_log(room, "wexp: too few arguments");
		return;
	}

	if ((ch = get_char_by_room(room, name))) {
		EndowExpToChar(ch, atoi(amount));
		sprintf(buf, "wexp: victim (%s) получил опыт %d", GET_NAME(ch), atoi(amount));
		wld_log(room, buf);
	} else {
		wld_log(room, "wexp: target not found");
		return;
	}
}

// purge all objects an npcs in room, or specified object or mob //
void do_wpurge(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	char arg[kMaxInputLength];
	CharData *ch /*, *next_ch */;
	ObjData *obj /*, *next_obj */;

	one_argument(argument, arg);

	if (!*arg) {
		return;
	}

	if (!(ch = get_char_by_room(room, arg))) {
		if ((obj = get_obj_by_room(room, arg))) {
			ExtractObjFromWorld(obj, false);
		} else {
			wld_log(room, "wpurge: bad argument");
		}
		return;
	}

	if (!ch->IsNpc()) {
		wld_log(room, "wpurge: purging a PC");
		return;
	}

	if (ch->followers
		|| ch->get_master()) {
		die_follower(ch);
	}
	ExtractCharFromWorld(ch, false);
}

// loads a mobile or object into the room //
void do_wload(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char arg1[kMaxInputLength], arg2[kMaxInputLength];
	char uid[kMaxInputLength], varname[kMaxInputLength] = "LoadedUid";
	char uid_type;
	int number = 0, idnum;
	CharData *mob;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
		wld_log(room, "wload: bad syntax");
		return;
	}
	if (utils::IsAbbr(arg1, "mob")) {
		if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
			wld_log(room, "wload: bad mob vnum");
			return;
		}
		uid_type = UID_CHAR;
		idnum = mob->id;
		PlaceCharToRoom(mob, real_room(room->room_vn));
		load_mtrigger(mob);
	} else if (utils::IsAbbr(arg1, "obj")) {
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object) {
			wld_log(room, "wload: bad object vnum");
			return;
		}
		if (GET_OBJ_MIW(obj_proto[object->get_rnum()]) >= 0
			&& obj_proto.actual_count(object->get_rnum()) > GET_OBJ_MIW(obj_proto[object->get_rnum()])) {
			if (!check_unlimited_timer(obj_proto[object->get_rnum()].get())) {
				sprintf(buf, "wload: количество больше чем в MIW для #%d.", number);
				wld_log(room, buf);
//				extract_obj(object.get());
//				return;
			}
		}
		log("Load obj #%d by %s (wload)", number, room->name);
		object->set_vnum_zone_from(zone_table[room->zone_rn].vnum);
		uid_type = UID_OBJ;
		idnum = object->get_id();
		PlaceObjToRoom(object.get(), real_room(room->room_vn));
		load_otrigger(object.get());
	} else {
		wld_log(room, "wload: bad type");
		return;
	}
	sprintf(uid, "%c%d", uid_type, idnum);
	add_var_cntx(&GET_TRIG_VARS(trig), varname, uid, 0);
}

// increases spells & skills //
ESpell FixNameAndFindSpellId(char *name);

void do_wdamage(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	char name[kMaxInputLength], amount[kMaxInputLength];
	int dam = 0;
	CharData *ch;

	two_arguments(argument, name, amount);

	if (!*name || !*amount || !a_isdigit(*amount)) {
		wld_log(room, "wdamage: bad syntax");
		return;
	}

	dam = atoi(amount);

	if ((ch = get_char_by_room(room, name))) {
		if (world[ch->in_room]->zone_rn != room->zone_rn) {
			return;
		}

		if (IS_IMMORTAL(ch) && dam > 0) {
			SendMsgToChar("Будучи бессмертным, вы избежали повреждения...\r\n", ch);
			return;
		}
		GET_HIT(ch) -= dam;
		if (dam < 0) {
			SendMsgToChar("Вы почувствовали себя лучше.\r\n", ch);
			return;
		}

		update_pos(ch);
		char_dam_message(dam, ch, ch, 0);
		if (GET_POS(ch) == EPosition::kDead) {
			if (!ch->IsNpc()) {
				sprintf(buf2, "%s killed by wdamage at %s [%d]", GET_NAME(ch),
						ch->in_room == kNowhere ? "kNowhere" : world[ch->in_room]->name, GET_ROOM_VNUM(ch->in_room));
				mudlog(buf2, BRF, kLvlBuilder, SYSLOG, true);
			}
			die(ch, nullptr);
		}
	} else {
		wld_log(room, "wdamage: target not found");
	}
}

void do_wat(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	char location[kMaxInputLength], arg2[kMaxInputLength];
	int vnum, rnum = 0;
//    room_data *r2;

	void wld_command_interpreter(RoomData *room, char *argument, Trigger *trig);

	half_chop(argument, location, arg2);

	if (!*location || !*arg2 || !a_isdigit(*location)) {
		wld_log(room, "wat: bad syntax");
		return;
	}
	vnum = atoi(location);
	rnum = real_room(vnum);
	if (kNowhere == rnum) {
		wld_log(room, "wat: location not found");
		return;
	}

	reloc_target = rnum;
	wld_command_interpreter(world[rnum], arg2, trig);
	reloc_target = -1;
}

void do_wfeatturn(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	bool isFeat = false;
	CharData *ch;
	char name[kMaxInputLength], featname[kMaxInputLength], amount[kMaxInputLength], *pos;
	int featdiff = 0;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount) {
		wld_log(room, "wfeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.'))) {
		*pos = ' ';
	}
	while ((pos = strchr(featname, '_'))) {
		*pos = ' ';
	}

	const auto feat_id = FindFeatId(featname);
	if (MUD::Feat(feat_id).IsAvailable()) {
		isFeat = true;
	} else {
		wld_log(room, "wfeatturn: feature not found");
		return;
	}

	if (!str_cmp(amount, "set")) {
		featdiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		featdiff = 0;
	} else {
		wld_log(room, "wfeatturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_room(room, name))) {
		wld_log(room, "wfeatturn: target not found");
		return;
	}

	if (isFeat) {
		trg_featturn(ch, feat_id, featdiff, last_trig_vnum);
	}
}
/*
 *  Данная функция практически без изменения скопирована в команды мобов и предметов.
 *  \todo Нужно избавиться от дублирования кода (вероятно, оно не только тут).
 */
void do_wskillturn(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], skill_name[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skill_name), amount);

	if (!*name || !*skill_name || !*amount) {
		wld_log(room, "wskillturn: too few arguments");
		return;
	}

	auto skill_id = FixNameAndFindSkillId(skill_name);
	bool is_skill = false;
	if (MUD::Skills().IsValid(skill_id)) {
		is_skill = true;
	} else if ((recipenum = im_get_recipe_by_name(skill_name)) < 0) {
		sprintf(buf, "wskillturn: %s skill not found", skill_name);
		wld_log(room, buf);
		return;
	}

	if (!str_cmp(amount, "set")) {
		skilldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		skilldiff = 0;
	} else {
		wld_log(room, "wskillturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_room(room, name))) {
		wld_log(room, "wskillturn: target not found");
		return;
	}

	if (is_skill) {
		if (MUD::Class(ch->GetClass()).skills[skill_id].IsAvailable()) {
			trg_skillturn(ch, skill_id, skilldiff, last_trig_vnum);
		} else {
			sprintf(buf, "wskillturn: skill and character class mismatch");
			wld_log(room, buf);
		}
	} else {
		trg_recipeturn(ch, recipenum, skilldiff);
	}
}

void do_wskilladd(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], skillname[kMaxInputLength], amount[kMaxInputLength];
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount) {
		wld_log(room, "wskilladd: too few arguments");
		return;
	}

	auto skill_id = FixNameAndFindSkillId(skillname);
	bool is_skill = false;
	if (MUD::Skills().IsValid(skill_id)) {
		is_skill = true;
	} else if ((recipenum = im_get_recipe_by_name(skillname)) < 0) {
		sprintf(buf, "wskillturn: %s skill/recipe not found", skillname);
		wld_log(room, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (!(ch = get_char_by_room(room, name))) {
		wld_log(room, "wskilladd: target not found");
		return;
	}

	if (is_skill) {
		AddSkill(ch, skill_id, skilldiff, last_trig_vnum);
	} else {
		AddRecipe(ch, recipenum, skilldiff);
	}
}

void do_wspellturn(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];

//    one_argument (two_arguments(argument, name, spellname), amount);
	argument = one_argument(argument, name);
	two_arguments(argument, spellname, amount);

	if (!*name || !*spellname || !*amount) {
		wld_log(room, "wspellturn: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		wld_log(room, "wspellturn: spell not found");
		return;
	}

	auto spelldiff{0};
	if (!str_cmp(amount, "set")) {
		spelldiff = 1;
	} else if (!str_cmp(amount, "clear")) {
		spelldiff = 0;
	} else {
		wld_log(room, "wspellturn: unknown set variable");
		return;
	}

	if ((ch = get_char_by_room(room, name))) {
		trg_spellturn(ch, spell_id, spelldiff, last_trig_vnum);
	} else {
		wld_log(room, "wspellturn: target not found");
		return;
	}
}

void do_wspellturntemp(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];

	argument = one_argument(argument, name);
	two_arguments(argument, spellname, amount);

	if (!*name || !*spellname || !*amount) {
		wld_log(room, "wspellturntemp: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		wld_log(room, "mspellturntemp: spell not found");
		return;
	}

	auto spelltime = atoi(amount);
	if (spelltime <= 0) {
		wld_log(room, "wspellturntemp: time is negative");
		return;
	}

	if ((ch = get_char_by_room(room, name))) {
		trg_spellturntemp(ch, spell_id, spelltime, last_trig_vnum);
	} else {
		wld_log(room, "wspellturntemp: target not found");
		return;
	}
}

void do_wspelladd(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], amount[kMaxInputLength];

	one_argument(two_arguments(argument, name, spellname), amount);
	if (!*name || !*spellname || !*amount) {
		wld_log(room, "wspelladd: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		wld_log(room, "wspelladd: spell not found");
		return;
	}

	auto spelldiff = atoi(amount);
	if ((ch = get_char_by_room(room, name))) {
		trg_spelladd(ch, spell_id, spelldiff, last_trig_vnum);
	} else {
		wld_log(room, "wspelladd: target not found");
		return;
	}
}

void do_wspellitem(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	CharData *ch;
	char name[kMaxInputLength], spellname[kMaxInputLength], type[kMaxInputLength], turn[kMaxInputLength];

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn) {
		wld_log(room, "wspellitem: too few arguments");
		return;
	}

	auto spell_id = FixNameAndFindSpellId(spellname);
	if (spell_id == ESpell::kUndefined) {
		wld_log(room, "mspellturntemp: spell not found");
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
		wld_log(room, "wspellitem: type spell not found");
		return;
	}

	auto spelldiff{0};
	if (!str_cmp(turn, "set")) {
		spelldiff = 1;
	} else if (!str_cmp(turn, "clear")) {
		spelldiff = 0;
	} else {
		wld_log(room, "wspellitem: unknown set variable");
		return;
	}

	if ((ch = get_char_by_room(room, name))) {
		trg_spellitem(ch, spell_id, spelldiff, spell);
	} else {
		wld_log(room, "wspellitem: target not found");
		return;
	}
}

/* Команда открывает пентаграмму из текущей комнаты в заданную комнату
   синтаксис wportal <номер комнаты> <длительность портала>
*/
void do_wportal(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *) {
	int target, howlong, curroom, nr;
	char arg1[kMaxInputLength], arg2[kMaxInputLength];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2) {
		wld_log(room, "wportal: called with too few args");
		return;
	}

	howlong = atoi(arg2);
	nr = atoi(arg1);
	target = real_room(nr);

	if (target == kNowhere) {
		wld_log(room, "wportal: target is an invalid room");
		return;
	}

	/* Ставим пентаграмму из текущей комнаты в комнату target с
	   длительностью howlong */
	curroom = real_room(room->room_vn);
	world[curroom]->portal_room = target;
	world[curroom]->portal_time = howlong;
	world[curroom]->pkPenterUnique = 0;
	AddPortalTimer(nullptr, world[curroom], howlong * 30 - 1);
	OneWayPortal::add(world[target], world[curroom]);
	act("Лазурная пентаграмма возникла в воздухе.", false, world[curroom]->first_character(), 0, 0, kToChar);
	act("Лазурная пентаграмма возникла в воздухе.", false, world[curroom]->first_character(), 0, 0, kToRoom);
}
// для команды wat
void WldDgCast(RoomData *room, char *argument, int/* cmd*/, int/* subcmd*/, Trigger *trig) {
	do_dg_cast(room, trig, WLD_TRIGGER, argument);
}

const struct wld_command_info wld_cmd_info[] =
	{
		{"RESERVED", 0, 0},    // this must be first -- for specprocs
		{"wasound", do_wasound, 0},
		{"wdoor", do_wdoor, 0},
		{"wecho", do_wecho, 0},
		{"wechoaround", do_wsend, SCMD_WECHOAROUND},
		{"wexp", do_wexp, 0},
		{"wforce", do_wforce, 0},
		{"wload", do_wload, 0},
		{"wpurge", do_wpurge, 0},
		{"wsend", do_wsend, SCMD_WSEND},
		{"wteleport", do_wteleport, 0},
		{"wzoneecho", do_wzoneecho, 0},
		{"wdamage", do_wdamage, 0},
		{"wat", do_wat, 0},
		{"wspellturn", do_wspellturn, 0},
		{"wfeatturn", do_wfeatturn, 0},
		{"wskillturn", do_wskillturn, 0},
		{"wspelladd", do_wspelladd, 0},
		{"wskilladd", do_wskilladd, 0},
		{"wspellitem", do_wspellitem, 0},
		{"wspellturntemp", do_wspellturntemp, 0},
		{"wportal", do_wportal, 0},
		{"dgcast", WldDgCast, 0},
		{"\n", 0, 0}        // this must be last
	};

// *  This is the command interpreter used by rooms, called by script_driver.
void wld_command_interpreter(RoomData *room, char *argument, Trigger *trig) {
	char *line, arg[kMaxInputLength];

	skip_spaces(&argument);

	// just drop to next line for hitting CR
	if (!*argument)
		return;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;
	size_t length = strlen(arg);
	while (*wld_cmd_info[cmd].command != '\n') {
		if (!strn_cmp(wld_cmd_info[cmd].command, arg, length)) {
			break;
		}
		cmd++;
	}

	if (*wld_cmd_info[cmd].command == '\n') {
		sprintf(buf2, "Unknown world cmd: '%s'", argument);
		wld_log(room, buf2, LGH);
	} else {
		const wld_command_info::handler_f &command = wld_cmd_info[cmd].command_pointer;
		command(room, line, cmd, wld_cmd_info[cmd].subcmd, trig);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
