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

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "obj.hpp"
#include "screen.h"
#include "dg_scripts.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spell_parser.hpp"
#include "spells.h"
#include "im.h"
#include "char.hpp"
#include "skills.h"
#include "name_list.hpp"
#include "room.hpp"
#include "magic.h"
#include "fight.h"
#include "features.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

extern const char *dirs[];
extern int up_obj_where(OBJ_DATA * obj);
extern int reloc_target;

CHAR_DATA *get_char_by_obj(OBJ_DATA * obj, char *name);
OBJ_DATA *get_obj_by_obj(OBJ_DATA * obj, char *name);
void sub_write(char *arg, CHAR_DATA * ch, byte find_invis, int targets);
void die(CHAR_DATA * ch, CHAR_DATA * killer);
void obj_command_interpreter(OBJ_DATA * obj, char *argument);

ROOM_DATA *get_room(char *name);

bool mob_script_command_interpreter(CHAR_DATA* ch, char *argument);

struct obj_command_info
{
	const char *command;
	typedef void(*handler_f)(OBJ_DATA * obj, char *argument, int cmd, int subcmd);
	handler_f command_pointer;
	int subcmd;
};

// do_osend 
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1

// attaches object name and vnum to msg and sends it to script_log 
void obj_log(OBJ_DATA * obj, const char *msg, const int type = 0)
{
	char buf[MAX_INPUT_LENGTH + 100];

	sprintf(buf, "(Obj: '%s', VNum: %d, trig: %d): %s", obj->get_short_description().c_str(), GET_OBJ_VNUM(obj), last_trig_vnum, msg);
	script_log(buf, type);
}

// returns the real room number that the object or object's carrier is in 
int obj_room(OBJ_DATA * obj)
{
	if (obj->get_in_room() != NOWHERE)
	{
		return obj->get_in_room();
	}
	else if (obj->get_carried_by())
	{
		return IN_ROOM(obj->get_carried_by());
	}
	else if (obj->get_worn_by())
	{
		return IN_ROOM(obj->get_worn_by());
	}
	else if (obj->get_in_obj())
	{
		return obj_room(obj->get_in_obj());
	}
	else
	{
		return NOWHERE;
	}
}

// returns the real room number, or NOWHERE if not found or invalid 
int find_obj_target_room(OBJ_DATA * obj, char *rawroomstr)
{
	char roomstr[MAX_INPUT_LENGTH];
	room_rnum location = NOWHERE;

	one_argument(rawroomstr, roomstr);

	if (!*roomstr)
	{
		sprintf(buf, "Undefined oteleport room: %s", rawroomstr);
		obj_log(obj, buf);
		return NOWHERE;
	}

	auto tmp = atoi(roomstr);
	if (tmp > 0)
	{
		location = real_room(tmp);
	}
	else
	{
		sprintf(buf, "Undefined oteleport room: %s", roomstr);
		obj_log(obj, buf);
		return NOWHERE;
	}

	return location;
}

// Object commands 
void do_oecho(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	skip_spaces(&argument);

	int room;
	if (!*argument)
	{
		obj_log(obj, "oecho called with no args");
	}
	else if ((room = obj_room(obj)) != NOWHERE)
	{
		if (!world[room]->people.empty())
		{
			sub_write(argument, world[room]->first_character(), TRUE, TO_ROOM | TO_CHAR);
		}
	}
}
void do_oat(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
//	int location;
	char roomstr[MAX_INPUT_LENGTH];
	room_rnum location = NOWHERE;
	if (!*argument)
	{
		obj_log(obj, "oat: bad argument");
		return;
	}
	one_argument(argument, roomstr);
	auto tmp = atoi(roomstr);
	if (tmp > 0)
	{
	    location = real_room(tmp);
	}
	else
	{
		sprintf(buf, "oat: invalid location '%d'", tmp);
		obj_log(obj, buf);
		return;
	}
	argument = one_argument(argument, roomstr);
	auto tmp_obj = world_objects.create_from_prototype_by_vnum(obj->get_vnum());
	tmp_obj->set_in_room(location);
	obj_command_interpreter(tmp_obj.get(), argument);
	world_objects.remove(tmp_obj);
}

void do_oforce(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch;
	char arg1[MAX_INPUT_LENGTH], *line;

	line = one_argument(argument, arg1);

	if (!*arg1 || !*line)
	{
		obj_log(obj, "oforce called with too few args");

		return;
	}

	if (!str_cmp(arg1, "all")
		|| !str_cmp(arg1, "все"))
	{
		obj_log(obj, "ERROR: \'oforce all\' command disabled.");
		return;

		/*if ((room = obj_room(obj)) == NOWHERE)
		{
			obj_log(obj, "oforce called by object in NOWHERE");
		}
		else
		{
			const auto people_copy = world[room]->people;
			for (const auto ch : people_copy)
			{
				if (IS_NPC(ch)
					|| GET_LEVEL(ch) < LVL_IMMORT)
				{
					command_interpreter(ch, line);
				}
			}
		}*/
	}
	else
	{
		if ((ch = get_char_by_obj(obj, arg1)))
		{
			// если чар в ЛД
			if (!IS_NPC(ch))
			{
				if (!ch->desc)
				{
					return;
				}
			}

			if (IS_NPC(ch))
			{
				if (mob_script_command_interpreter(ch, line))
				{
					obj_log(obj, "Mob trigger commands in oforce. Please rewrite trigger.");
					return;
				}

				command_interpreter(ch, line);
			}
			else if (GET_LEVEL(ch) < LVL_IMMORT)
			{
				command_interpreter(ch, line);
			}
		}
		else
		{
			obj_log(obj, "oforce: no target found");
		}
	}
}

void do_osend(OBJ_DATA *obj, char *argument, int/* cmd*/, int subcmd)
{
	char buf[MAX_INPUT_LENGTH], *msg;
	CHAR_DATA *ch;

	msg = any_one_arg(argument, buf);

	if (!*buf)
	{
		obj_log(obj, "osend called with no args");
		return;
	}

	skip_spaces(&msg);

	if (!*msg)
	{
		obj_log(obj, "osend called without a message");
		return;
	}
	if ((ch = get_char_by_obj(obj, buf)))
	{
		if (subcmd == SCMD_OSEND)
			sub_write(msg, ch, TRUE, TO_CHAR);
		else if (subcmd == SCMD_OECHOAROUND)
			sub_write(msg, ch, TRUE, TO_ROOM);
	}
	else
		obj_log(obj, "no target found for osend");
}

// increases the target's exp 
void do_oexp(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

	two_arguments(argument, name, amount);

	if (!*name || !*amount)
	{
		obj_log(obj, "oexp: too few arguments");
		return;
	}

	if ((ch = get_char_by_obj(obj, name)))
	{
		gain_exp(ch, atoi(amount));
		sprintf(buf, "oexp: victim (%s) получил опыт %d", GET_NAME(ch) , atoi(amount));
		obj_log(obj, buf);
	}
	else
	{
		obj_log(obj, "oexp: target not found");
		return;
	}
}

// set the object's timer value 
void do_otimer(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!*arg)
		obj_log(obj, "otimer: missing argument");
	else if (!a_isdigit(*arg))
		obj_log(obj, "otimer: bad argument");
	else
	{
		obj->set_timer(atoi(arg));
	}
}

// transform into a different object 
// note: this shouldn't be used with containers unless both objects 
// are containers! 
void do_otransform(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *wearer = NULL;
	int pos = -1;

	one_argument(argument, arg);

	if (!*arg)
	{
		obj_log(obj, "otransform: missing argument");
	}
	else if (!a_isdigit(*arg))
	{
		obj_log(obj, "otransform: bad argument");
	}
	else
	{
		const auto o = world_objects.create_from_prototype_by_vnum(atoi(arg));
		if (o == NULL)
		{
			obj_log(obj, "otransform: bad object vnum");
			return;
		}
		// Описание работы функции см. в mtransform()

		if (obj->get_worn_by())
		{
			pos = obj->get_worn_on();
			wearer = obj->get_worn_by();
			unequip_char(obj->get_worn_by(), pos);
		}

		obj->swap(*o);

		if (o->get_extra_flag(EExtraFlag::ITEM_TICKTIMER))
		{
			obj->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);
		}

		if (wearer)
		{
			equip_char(wearer, obj, pos);
		}
		extract_obj(o.get());
	}
}

// purge all objects an npcs in room, or specified object or mob 
void do_opurge(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *ch;
	OBJ_DATA *o;

	one_argument(argument, arg);

	if (!*arg)
	{
		return;
	}

	if (!(ch = get_char_by_obj(obj, arg)))
	{
		if ((o = get_obj_by_obj(obj, arg)))
		{
			extract_obj(o);
		}
		else
			obj_log(obj, "opurge: bad argument");
		return;
	}

	if (!IS_NPC(ch))
	{
		obj_log(obj, "opurge: purging a PC");
		return;
	}

	if (ch->followers
		|| ch->has_master())
	{
		die_follower(ch);
	}
	extract_char(ch, FALSE);
}

void do_oteleport(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch, *horse;
	int target, rm;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2)
	{
		obj_log(obj, "oteleport called with too few args");
		return;
	}

	target = find_obj_target_room(obj, arg2);

	if (target == NOWHERE)
		obj_log(obj, "oteleport target is an invalid room");
	else if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
	{
		rm = obj_room(obj);
		if (rm == NOWHERE)
		{
			obj_log(obj, "oteleport called in NOWHERE");
			return;
		}

		if (target == rm)
		{
			obj_log(obj, "oteleport target is itself");
		}

		const auto people_copy = world[rm]->people;
		decltype(world[rm]->people)::const_iterator next_ch = people_copy.begin();
		for (auto ch_i = next_ch; ch_i != people_copy.end(); ch_i = next_ch)
		{
			const auto ch = *ch_i;
			++next_ch;

			if (IS_NPC(ch)
				&& !(IS_HORSE(ch)
					|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
					|| MOB_FLAGGED(ch, MOB_ANGEL)
					|| MOB_FLAGGED(ch, MOB_GHOST)))
			{
				continue;
			}

			if (on_horse(ch)
				|| has_horse(ch, TRUE))
			{
				horse = get_horse(ch);
			}
			else
			{
				horse = NULL;
			}

			if (ch->in_room == NOWHERE)
			{
				obj_log(obj, "oteleport transports from NOWHERE");
				return;
			}

			char_from_room(ch);
			char_to_room(ch, target);

			if (!str_cmp(argument, "horse")
				&& horse)
			{
				// skip horse
				if (next_ch != people_copy.end()
					&& horse == *next_ch)
				{
					++next_ch;
				}

				char_from_room(horse);
				char_to_room(horse, target);
			}

			check_horse(ch);
			look_at_room(ch, TRUE);
		}
	}
	else
	{
		if ((ch = get_char_by_obj(obj, arg1)))
		{
			if (on_horse(ch)
				|| has_horse(ch, TRUE))
			{
				horse = get_horse(ch);
			}
			else
			{
				horse = NULL;
			}

			const auto people_copy = world[ch->in_room]->people;
			for (const auto charmee : people_copy)
			{
				if (IS_NPC(charmee)
					&& (AFF_FLAGGED(charmee, EAffectFlag::AFF_CHARM)
						|| MOB_FLAGGED(charmee, MOB_ANGEL)
						|| MOB_FLAGGED(charmee, MOB_GHOST))
					&& charmee->get_master() == ch)
				{
					char_from_room(charmee);
					char_to_room(charmee, target);
				}
			}

			char_from_room(ch);
			char_to_room(ch, target);

			if (!str_cmp(argument, "horse")
				&& horse)
			{
				char_from_room(horse);
				char_to_room(horse, target);
			}

			check_horse(ch);
			look_at_room(ch, TRUE);
		}
		else
		{
			obj_log(obj, "oteleport: no target found");
		}
	}
}

void do_dgoload(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int number = 0, room;
	CHAR_DATA *mob;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
	{
		obj_log(obj, "oload: bad syntax");
		return;
	}

	if ((room = obj_room(obj)) == NOWHERE)
	{
		obj_log(obj, "oload: object in NOWHERE trying to load");
		return;
	}

	if (is_abbrev(arg1, "mob"))
	{
		if ((mob = read_mobile(number, VIRTUAL)) == NULL)
		{
			obj_log(obj, "oload: bad mob vnum");
			return;
		}
		char_to_room(mob, room);
		load_mtrigger(mob);
	}
	else if (is_abbrev(arg1, "obj"))
	{
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object)
		{
			obj_log(obj, "oload: bad object vnum");
			return;
		}

		if (GET_OBJ_MIW(obj_proto[object->get_rnum()]) >= 0
			&& obj_proto.actual_count(object->get_rnum()) > GET_OBJ_MIW(obj_proto[object->get_rnum()]))
		{
			if (!check_unlimited_timer(obj_proto[object->get_rnum()].get()))
			{
				sprintf(buf, "oload: Попытка загрузить предмет больше чем в MIW для #%d, предмет удален.", number);
				obj_log(obj, buf);
//				extract_obj(object.get());
//				return;
			}
		}
		log("Load obj #%d by %s (oload)", number, obj->get_aliases().c_str());
		object->set_zone(world[room]->zone);
		obj_to_room(object.get(), room);
		load_otrigger(object.get());
	}
	else
	{
		obj_log(obj, "oload: bad type");
	}
}

void do_odamage(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int dam = 0;
	CHAR_DATA *ch;

	two_arguments(argument, name, amount);

	if (!*name || !*amount || !a_isdigit(*amount))
	{
		obj_log(obj, "odamage: bad syntax");
		return;
	}

	dam = atoi(amount);

	if ((ch = get_char_by_obj(obj, name)))
	{
		if (world[ch->in_room]->zone != world[up_obj_where(obj)]->zone)
			return;

		if (GET_LEVEL(ch) >= LVL_IMMORT)
		{
			send_to_char
			("Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.", ch);
			return;
		}
		GET_HIT(ch) -= dam;
		update_pos(ch);
		char_dam_message(dam, ch, ch, 0);
		if (GET_POS(ch) == POS_DEAD)
		{
			if (!IS_NPC(ch))
			{
				sprintf(buf2, "%s killed by odamage at %s [%d]", GET_NAME(ch),
						ch->in_room == NOWHERE ? "NOWHERE" : world[ch->in_room]->name, GET_ROOM_VNUM(ch->in_room));
				mudlog(buf2, BRF, LVL_BUILDER, SYSLOG, TRUE);
			}
			die(ch, NULL);
		}
	}
	else
		obj_log(obj, "odamage: target not found");
}

void do_odoor(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
	char field[MAX_INPUT_LENGTH], *value;
	ROOM_DATA *rm;
	int dir, fd, to_room, lock;

	const char *door_field[] = { "purge",
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
	skip_spaces(&value);

	if (!*target || !*direction || !*field)
	{
		obj_log(obj, "odoor called with too few args");
		return;
	}

	if ((rm = get_room(target)) == NULL)
	{
		obj_log(obj, "odoor: invalid target");
		return;
	}

	if ((dir = search_block(direction, dirs, FALSE)) == -1)
	{
		obj_log(obj, "odoor: invalid direction");
		return;
	}

	if ((fd = search_block(field, door_field, FALSE)) == -1)
	{
		obj_log(obj, "odoor: invalid field");
		return;
	}

	auto exit = rm->dir_option[dir];

	// purge exit
	if (fd == 0)
	{
		if (exit)
		{
			rm->dir_option[dir].reset();
		}
	}
	else
	{
		if (!exit)
		{
			exit.reset(new EXIT_DATA());
			rm->dir_option[dir] = exit;
		}

		std::string buffer;
		switch (fd)
		{
		case 1:	// description 
			exit->general_description = std::string(value) + "\r\n";
			break;

		case 2:	// flags       
			asciiflag_conv(value, &exit->exit_info);
			break;

		case 3:	// key         
			exit->key = atoi(value);
			break;

		case 4:	// name        
			exit->set_keywords(value);
			break;

		case 5:	// room        
			if ((to_room = real_room(atoi(value))) != NOWHERE)
			{
				exit->to_room(to_room);
			}
			else
			{
				obj_log(obj, "odoor: invalid door target");
			}
			break;

		case 6:	// lock - сложность замка         
			lock = atoi(value);
			if (!(lock < 0 || lock >255))
				exit->lock_complexity = lock;
			else
				obj_log(obj, "odoor: invalid lock complexity");
			break;
		}
	}
}

void do_osetval(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int position, new_value;

	two_arguments(argument, arg1, arg2);
	if (!*arg1 || !*arg2 || !is_number(arg1) || !is_number(arg2))
	{
		obj_log(obj, "osetval: bad syntax");
		return;
	}

	position = atoi(arg1);
	new_value = atoi(arg2);
	if (position >= 0 && position < OBJ_DATA::VALS_COUNT)
	{
		obj->set_val(position, new_value);
	}
	else
	{
		obj_log(obj, "osetval: position out of bounds!");
	}
}

void do_ofeatturn(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int isFeat = 0;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], featname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int featnum = 0, featdiff = 0;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount)
	{
		obj_log(obj, "ofeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.')))
		* pos = ' ';
	while ((pos = strchr(featname, '_')))
		* pos = ' ';

	if ((featnum = find_feat_num(featname)) > 0 && featnum < MAX_FEATS)
		isFeat = 1;
	else
	{
		sprintf(buf, "ofeatturn: %s skill/recipe not found", featname);
		obj_log(obj, buf);
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else
	{
		obj_log(obj, "ofeatturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_obj(obj, name)))
	{
		obj_log(obj, "ofeatturn: target not found");
		return;
	}

	if (isFeat)
		trg_featturn(ch, featnum, featdiff, last_trig_vnum);
}

void do_oskillturn(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	bool isSkill = false;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	ESkill skillnum = SKILL_INVALID;
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		obj_log(obj, "oskillturn: too few arguments");
		return;
	}

	if ((skillnum = fix_name_and_find_skill_num(skillname)) > 0 && skillnum <= MAX_SKILL_NUM)
	{
		isSkill = true;
	}
	else if ((recipenum = im_get_recipe_by_name(skillname)) < 0)
	{
		sprintf(buf, "oskillturn: %s skill/recipe not found", skillname);
		obj_log(obj, buf);
		return;
	}

	if (!str_cmp(amount, "set"))
	{
		skilldiff = 1;
	}
	else if (!str_cmp(amount, "clear"))
	{
		skilldiff = 0;
	}
	else
	{
		obj_log(obj, "oskillturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_obj(obj, name)))
	{
		obj_log(obj, "oskillturn: target not found");
		return;
	}

	if (isSkill)
	{
		if (skill_info[skillnum].classknow[GET_CLASS(ch)][GET_KIN(ch)] == KNOW_SKILL)
		{
			trg_skillturn(ch, skillnum, skilldiff, last_trig_vnum);
		}
		else
		{
			sprintf(buf, "oskillturn: несоответсвие устанавливаемого умения классу игрока");
			obj_log(obj, buf);
		}
	}
	else
	{
		trg_recipeturn(ch, recipenum, skilldiff);
	}
}

void do_oskilladd(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	bool isSkill = false;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	ESkill skillnum = SKILL_INVALID;
	int recipenum = 0;
	int skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		obj_log(obj, "oskilladd: too few arguments");
		return;
	}

	if ((skillnum = fix_name_and_find_skill_num(skillname)) > 0 && skillnum <= MAX_SKILL_NUM)
	{
		isSkill = true;
	}
	else if ((recipenum = im_get_recipe_by_name(skillname)) < 0)
	{
		sprintf(buf, "oskilladd: %s skill/recipe not found", skillname);
		obj_log(obj, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (!(ch = get_char_by_obj(obj, name)))
	{
		obj_log(obj, "oskilladd: target not found");
		return;
	}

	if (isSkill)
	{
		trg_skilladd(ch, skillnum, skilldiff, last_trig_vnum);
	}
	else
	{
		trg_recipeadd(ch, recipenum, skilldiff);
	}
}

void do_ospellturn(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount)
	{
		obj_log(obj, "ospellturn: too few arguments");
		return;
	}

	if ((spellnum = fix_name_and_find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		obj_log(obj, "ospellturn: spell not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		spelldiff = 1;
	else if (!str_cmp(amount, "clear"))
		spelldiff = 0;
	else
	{
		obj_log(obj, "ospellturn: unknown set variable");
		return;
	}

	if ((ch = get_char_by_obj(obj, name)))
	{
		trg_spellturn(ch, spellnum, spelldiff, last_trig_vnum);
	}
	else
	{
		obj_log(obj, "ospellturn: target not found");
		return;
	}
}

void do_ospellturntemp(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int spellnum = 0, spelltime = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount)
	{
		obj_log(obj, "ospellturntemp: too few arguments");
		return;
	}

	if ((spellnum = fix_name_and_find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		obj_log(obj, "ospellturntemp: spell not found");
		return;
	}

	spelltime = atoi(amount);

	if (spelltime < 0)
	{
		obj_log(obj, "ospellturntemp: time is negative");
		return;
	}

	if ((ch = get_char_by_obj(obj, name)))
	{
		trg_spellturntemp(ch, spellnum, spelltime, last_trig_vnum);
	}
	else
	{
		obj_log(obj, "ospellturntemp: target not found");
		return;
	}
}

void do_ospelladd(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount)
	{
		obj_log(obj, "ospelladd: too few arguments");
		return;
	}

	if ((spellnum = fix_name_and_find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		obj_log(obj, "ospelladd: spell not found");
		return;
	}

	spelldiff = atoi(amount);

	if ((ch = get_char_by_obj(obj, name)))
	{
		trg_spelladd(ch, spellnum, spelldiff, last_trig_vnum);
	}
	else
	{
		obj_log(obj, "ospelladd: target not found");
		return;
	}
}

void do_ospellitem(OBJ_DATA *obj, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], turn[MAX_INPUT_LENGTH];
	int spellnum = 0, spelldiff = 0, spell = 0;

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn)
	{
		obj_log(obj, "ospellitem: too few arguments");
		return;
	}

	if ((spellnum = fix_name_and_find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		obj_log(obj, "ospellitem: spell not found");
		return;
	}

	if (!str_cmp(type, "potion"))
	{
		spell = SPELL_POTION;
	}
	else if (!str_cmp(type, "wand"))
	{
		spell = SPELL_WAND;
	}
	else if (!str_cmp(type, "scroll"))
	{
		spell = SPELL_SCROLL;
	}
	else if (!str_cmp(type, "items"))
	{
		spell = SPELL_ITEMS;
	}
	else if (!str_cmp(type, "runes"))
	{
		spell = SPELL_RUNES;
	}
	else
	{
		obj_log(obj, "ospellitem: type spell not found");
		return;
	}

	if (!str_cmp(turn, "set"))
	{
		spelldiff = 1;
	}
	else if (!str_cmp(turn, "clear"))
	{
		spelldiff = 0;
	}
	else
	{
		obj_log(obj, "ospellitem: unknown set variable");
		return;
	}

	if ((ch = get_char_by_obj(obj, name)))
	{
		trg_spellitem(ch, spellnum, spelldiff, spell);
	}
	else
	{
		obj_log(obj, "ospellitem: target not found");
		return;
	}
}

const struct obj_command_info obj_cmd_info[] =
{
	{"RESERVED", 0, 0},	// this must be first -- for specprocs
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
	{"\n", 0, 0}		// this must be last
};

// *  This is the command interpreter used by objects, called by script_driver.
void obj_command_interpreter(OBJ_DATA * obj, char *argument)
{
	char *line, arg[MAX_INPUT_LENGTH];

	skip_spaces(&argument);

	// just drop to next line for hitting CR
	if (!*argument)
		return;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;
	const size_t length = strlen(arg);
	while (*obj_cmd_info[cmd].command != '\n')
	{
		if (!strncmp(obj_cmd_info[cmd].command, arg, length))
		{
			break;
		}
		cmd++;
	}

	if (*obj_cmd_info[cmd].command == '\n')
	{
		sprintf(buf2, "Unknown object cmd: '%s'", argument);
		obj_log(obj, buf2, LGH);
	}
	else
	{
		const obj_command_info::handler_f& command = obj_cmd_info[cmd].command_pointer;
		command(obj, line, cmd, obj_cmd_info[cmd].subcmd);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
