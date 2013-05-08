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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "im.h"
#include "features.hpp"
#include "char.hpp"
#include "skills.h"
#include "name_list.hpp"
#include "room.hpp"
#include "magic.h"
#include "fight.h"

extern INDEX_DATA *obj_index;
extern const char *dirs[];
extern int dg_owner_purged;
extern int up_obj_where(OBJ_DATA * obj);

CHAR_DATA *get_char_by_obj(OBJ_DATA * obj, char *name);
OBJ_DATA *get_obj_by_obj(OBJ_DATA * obj, char *name);
void sub_write(char *arg, CHAR_DATA * ch, byte find_invis, int targets);
void die(CHAR_DATA * ch, CHAR_DATA * killer);
room_data *get_room(char *name);
void asciiflag_conv(const char *flag, void *value);
#define OCMD(name)  \
   void (name)(OBJ_DATA *obj, char *argument, int cmd, int subcmd)


struct obj_command_info
{
	const char *command;
	void (*command_pointer)(OBJ_DATA * obj, char *argument, int cmd, int subcmd);
	int subcmd;
};


// do_osend 
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1



// attaches object name and vnum to msg and sends it to script_log 
void obj_log(OBJ_DATA * obj, const char *msg, const int type = 0)
{
	char buf[MAX_INPUT_LENGTH + 100];

	sprintf(buf, "(Obj: '%s', VNum: %d, trig: %d): %s", obj->short_description, GET_OBJ_VNUM(obj), last_trig_vnum, msg);
	script_log(buf, type);
}


// returns the real room number that the object or object's carrier is in 
int obj_room(OBJ_DATA * obj)
{
	if (obj->in_room != NOWHERE)
		return obj->in_room;
	else if (obj->carried_by)
		return IN_ROOM(obj->carried_by);
	else if (obj->worn_by)
		return IN_ROOM(obj->worn_by);
	else if (obj->in_obj)
		return obj_room(obj->in_obj);
	else
		return NOWHERE;
}


// returns the real room number, or NOWHERE if not found or invalid 
int find_obj_target_room(OBJ_DATA * obj, char *rawroomstr)
{
	int tmp;
	int location;
	CHAR_DATA *target_mob;
	OBJ_DATA *target_obj;
	char roomstr[MAX_INPUT_LENGTH];

	one_argument(rawroomstr, roomstr);

	if (!*roomstr)
		return NOWHERE;

	if (isdigit(*roomstr) && !strchr(roomstr, '.'))
	{
		tmp = atoi(roomstr);
		if ((location = real_room(tmp)) == NOWHERE)
			return NOWHERE;
	}
	else if ((target_mob = get_char_by_obj(obj, roomstr)))
		location = IN_ROOM(target_mob);
	else if ((target_obj = get_obj_by_obj(obj, roomstr)))
	{
		if (target_obj->in_room != NOWHERE)
			location = target_obj->in_room;
		else
			return NOWHERE;
	}
	else
		return NOWHERE;

	// a room has been found.  Check for permission 
	if (ROOM_FLAGGED(location, ROOM_GODROOM) ||
#ifdef ROOM_IMPROOM
			ROOM_FLAGGED(location, ROOM_IMPROOM) ||
#endif
			ROOM_FLAGGED(location, ROOM_HOUSE))
		return NOWHERE;

	return location;
}



// Object commands 

OCMD(do_oecho)
{
	int room;

	skip_spaces(&argument);

	if (!*argument)
		obj_log(obj, "oecho called with no args");
	else if ((room = obj_room(obj)) != NOWHERE)
	{
		if (world[room]->people)
			sub_write(argument, world[room]->people, TRUE, TO_ROOM | TO_CHAR);
	}
	// WorM: особой необходимости спамить это нету
	//else
	//	obj_log(obj, "oecho called by object in NOWHERE");
}


OCMD(do_oforce)
{
	CHAR_DATA *ch, *next_ch;
	int room;
	char arg1[MAX_INPUT_LENGTH], *line;

	line = one_argument(argument, arg1);

	if (!*arg1 || !*line)
	{
		obj_log(obj, "oforce called with too few args");
		return;
	}

	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
	{
		if ((room = obj_room(obj)) == NOWHERE)
			obj_log(obj, "oforce called by object in NOWHERE");
		else
		{
			for (ch = world[room]->people; ch; ch = next_ch)
			{
				next_ch = ch->next_in_room;
				if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
				{
					command_interpreter(ch, line);
				}
			}
		}
	}
	else
	{
		if ((ch = get_char_by_obj(obj, arg1)))
		{
			if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
			{
				command_interpreter(ch, line);
			}
		}
		else
			obj_log(obj, "oforce: no target found");
	}
}


OCMD(do_osend)
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
OCMD(do_oexp)
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
		gain_exp(ch, atoi(amount));
	else
	{
		obj_log(obj, "oexp: target not found");
		return;
	}
}


// set the object's timer value 
OCMD(do_otimer)
{
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!*arg)
		obj_log(obj, "otimer: missing argument");
	else if (!isdigit(*arg))
		obj_log(obj, "otimer: bad argument");
	else
	{
		obj->set_timer(atoi(arg));
	}
}


// transform into a different object 
// note: this shouldn't be used with containers unless both objects 
// are containers! 
OCMD(do_otransform)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *o;
	CHAR_DATA *wearer = NULL;
	int pos = -1;

	one_argument(argument, arg);

	if (!*arg)
		obj_log(obj, "otransform: missing argument");
	else if (!isdigit(*arg))
		obj_log(obj, "otransform: bad argument");
	else
	{
		o = read_object(atoi(arg), VIRTUAL);
		if (o == NULL)
		{
			obj_log(obj, "otransform: bad object vnum");
			return;
		}
		// Описание работы функции см. в mtransform()

		if (obj->worn_by)
		{
			pos = obj->worn_on;
			wearer = obj->worn_by;
			unequip_char(obj->worn_by, pos);
		}

		OBJ_DATA tmpobj(*o);
		*o = *obj;
		*obj = tmpobj;

// Имею:
//  obj -> старый указатель, новое наполнение из объекта o
//  o -> новый указатель, старое наполнение из объекта obj
//  tmpobj -> врем. переменная, наполнение из оригинального объекта o

// Копирование игровой информации (для o сохраняются оригинальные значения)
		obj->in_room = o->in_room;
		o->in_room = tmpobj.in_room;
		obj->carried_by = o->carried_by;
		o->carried_by = tmpobj.carried_by;
		obj->worn_by = o->worn_by;
		o->worn_by = tmpobj.worn_by;
		obj->worn_on = o->worn_on;
		o->worn_on = tmpobj.worn_on;
		obj->in_obj = o->in_obj;
		o->in_obj = tmpobj.in_obj;
		obj->set_timer(o->get_timer());
		o->set_timer(tmpobj.get_timer());
		obj->contains = o->contains;
		o->contains = tmpobj.contains;
		obj->id = o->id;
		o->id = tmpobj.id;
		obj->script = o->script;
		o->script = tmpobj.script;
		obj->next_content = o->next_content;
		o->next_content = tmpobj.next_content;
		obj->next = o->next;
		o->next = tmpobj.next;
		// для name_list
		obj->set_serial_num(o->get_serial_num());
		o->set_serial_num(tmpobj.get_serial_num());
		//копируем также инфу о зоне, вообще мне не совсем понятна замута с этой инфой об оригинальной зоне
		GET_OBJ_ZONE(obj) = GET_OBJ_ZONE(o);
		GET_OBJ_ZONE(o) = GET_OBJ_ZONE(&tmpobj);
//		ObjectAlias::remove(obj);
//		ObjectAlias::add(obj);
		if (OBJ_FLAGGED(o, ITEM_TICKTIMER))
			SET_BIT(GET_OBJ_EXTRA(obj, ITEM_TICKTIMER), ITEM_TICKTIMER);

		if (wearer)
		{
			equip_char(wearer, obj, pos);
		}
		extract_obj(o);

	}
}


// purge all objects an npcs in room, or specified object or mob 
OCMD(do_opurge)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *ch /*, *next_ch*/;
	OBJ_DATA *o /*, *next_obj */;
	// int rm;

	one_argument(argument, arg);

	if (!*arg)
	{
		return;
/*
		if ((rm = obj_room(obj)) != NOWHERE)
		{
			for (ch = world[rm]->people; ch; ch = next_ch)
			{
				next_ch = ch->next_in_room;
				if (IS_NPC(ch))
				{
					if (ch->followers || ch->master)
						die_follower(ch);
					extract_char(ch, FALSE);
				}
			}
			for (o = world[rm]->contents; o; o = next_obj)
			{
				next_obj = o->next_content;
				if (o != obj)
					extract_obj(o);
			}
		}
		return;
*/
	}

	if (!(ch = get_char_by_obj(obj, arg)))
	{
		if ((o = get_obj_by_obj(obj, arg)))
		{
			if (o == obj)
				dg_owner_purged = 1;
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

	if (ch->followers || ch->master)
		die_follower(ch);
	extract_char(ch, FALSE);
}


OCMD(do_oteleport)
{
	CHAR_DATA *ch, *next_ch, *horse, *charmee, *ncharmee;
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
			obj_log(obj, "oteleport target is itself");
		for (ch = world[rm]->people; ch; ch = next_ch)
		{
			next_ch = ch->next_in_room;
			if (IS_NPC(ch)
					&& !(IS_HORSE(ch) || AFF_FLAGGED(ch, AFF_CHARM)
						 || MOB_FLAGGED(ch, MOB_ANGEL)))
				continue;

			if (on_horse(ch) || has_horse(ch, TRUE))
				horse = get_horse(ch);
			else
				horse = NULL;
			if (IN_ROOM(ch) == NOWHERE)
			{
				obj_log(obj, "oteleport transports from NOWHERE");
				return;
			}
			char_from_room(ch);
			char_to_room(ch, target);
			if (!str_cmp(argument, "horse") && horse)
			{
				if (horse == next_ch)
					next_ch = horse->next_in_room;
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
			if (on_horse(ch) || has_horse(ch, TRUE))
				horse = get_horse(ch);
			else
				horse = NULL;
			for (charmee = world[IN_ROOM(ch)]->people; charmee; charmee = ncharmee)
			{
				ncharmee = charmee->next_in_room;
				if (IS_NPC(charmee) && (AFF_FLAGGED(charmee, AFF_CHARM)
										|| MOB_FLAGGED(charmee, MOB_ANGEL))
						&& charmee->master == ch)
				{
					char_from_room(charmee);
					char_to_room(charmee, target);
				}
			}

			char_from_room(ch);
			char_to_room(ch, target);
			if (!str_cmp(argument, "horse") && horse)
			{
				char_from_room(horse);
				char_to_room(horse, target);
			}
			check_horse(ch);
			look_at_room(ch, TRUE);
		}
		else
			obj_log(obj, "oteleport: no target found");
	}
}


OCMD(do_dgoload)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int number = 0, room;
	CHAR_DATA *mob;
	OBJ_DATA *object;

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
		if ((object = read_object(number, VIRTUAL)) == NULL)
		{
			obj_log(obj, "oload: bad object vnum");
			return;
		}
		log("Load obj #%d by %s (oload)", number, obj->aliases);
		GET_OBJ_ZONE(object) = world[room]->zone;
		obj_to_room(object, room);
		load_otrigger(object);
	}
	else
		obj_log(obj, "oload: bad type");
}

OCMD(do_odamage)
{
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int dam = 0;
	CHAR_DATA *ch;

	two_arguments(argument, name, amount);

	if (!*name || !*amount || !isdigit(*amount))
	{
		obj_log(obj, "odamage: bad syntax");
		return;
	}

	dam = atoi(amount);

	if ((ch = get_char_by_obj(obj, name)))
	{
		if (world[IN_ROOM(ch)]->zone != world[up_obj_where(obj)]->zone)
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
				sprintf(buf2, "%s killed by a trap at %s", GET_NAME(ch),
						IN_ROOM(ch) == NOWHERE ? "NOWHERE" : world[IN_ROOM(ch)]->name);
				mudlog(buf2, BRF, LVL_BUILDER, SYSLOG, TRUE);
			}
			die(ch, NULL);
		}
	}
	else
		obj_log(obj, "odamage: target not found");
}


OCMD(do_odoor)
{
	char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
	char field[MAX_INPUT_LENGTH], *value;
	room_data *rm;
	EXIT_DATA *exit;
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

	exit = rm->dir_option[dir];

	// purge exit
	if (fd == 0)
	{
		if (exit)
		{
			if (exit->general_description)
				free(exit->general_description);
			if (exit->keyword)
				free(exit->keyword);
			if (exit->vkeyword)
				free(exit->vkeyword);
			free(exit);
			rm->dir_option[dir] = NULL;
		}
	}
	else
	{
		if (!exit)
		{
			CREATE(exit, EXIT_DATA, 1);
			rm->dir_option[dir] = exit;
		}

		std::string buffer;
		std::string::size_type i;

		switch (fd)
		{
		case 1:	// description 
			if (exit->general_description)
				free(exit->general_description);
			CREATE(exit->general_description, char, strlen(value) + 3);
			strcpy(exit->general_description, value);
			strcat(exit->general_description, "\r\n");
			break;
		case 2:	// flags       
			asciiflag_conv(value, &exit->exit_info);
			break;
		case 3:	// key         
			exit->key = atoi(value);
			break;
		case 4:	// name        
			if (exit->keyword)
				free(exit->keyword);
			if (exit->vkeyword)
				free(exit->vkeyword);
			buffer = value;
			i = buffer.find('|');
			if (i != std::string::npos)
			{
				exit->keyword = str_dup(buffer.substr(0, i).c_str());
				exit->vkeyword = str_dup(buffer.substr(++i).c_str());
			}
			else
			{
				exit->keyword = str_dup(buffer.c_str());
				exit->vkeyword = str_dup(buffer.c_str());
			}
//			CREATE(exit->keyword, char, strlen(value) + 1);
//			strcpy(exit->keyword, value);
			break;
		case 5:	// room        
			if ((to_room = real_room(atoi(value))) != NOWHERE)
				exit->to_room = to_room;
			else
				obj_log(obj, "odoor: invalid door target");
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


OCMD(do_osetval)
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
	if (position >= 0 && position < NUM_OBJ_VAL_POSITIONS)
		GET_OBJ_VAL(obj, position) = new_value;
	else
		obj_log(obj, "osetval: position out of bounds!");
}

OCMD(do_ofeatturn)
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
		obj_log(obj, "ofeatturn: feat/recipe not found");
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
		trg_featturn(ch, featnum, featdiff);
}

OCMD(do_oskillturn)
{
	int isSkill = 0;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int skillnum = 0, skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		obj_log(obj, "oskillturn: too few arguments");
		return;
	}

	while ((pos = strchr(skillname, '.')))
		* pos = ' ';
	while ((pos = strchr(skillname, '_')))
		* pos = ' ';

	if ((skillnum = find_skill_num(skillname)) > 0 && skillnum <= MAX_SKILL_NUM)
		isSkill = 1;
	else if ((skillnum = im_get_recipe_by_name(skillname)) < 0)
	{
		obj_log(obj, "oskillturn: skill/recipe not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		skilldiff = 1;
	else if (!str_cmp(amount, "clear"))
		skilldiff = 0;
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
		trg_skillturn(ch, skillnum, skilldiff);
	else
		trg_recipeturn(ch, skillnum, skilldiff);
}


OCMD(do_oskilladd)
{
	int isSkill = 0;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int skillnum = 0, skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		obj_log(obj, "oskilladd: too few arguments");
		return;
	}

	while ((pos = strchr(skillname, '.')))
		* pos = ' ';
	while ((pos = strchr(skillname, '_')))
		* pos = ' ';

	if ((skillnum = find_skill_num(skillname)) > 0 && skillnum <= MAX_SKILL_NUM)
		isSkill = 1;
	else if ((skillnum = im_get_recipe_by_name(skillname)) < 0)
	{
		obj_log(obj, "oskilladd: skill/recipe not found");
		return;
	}

	skilldiff = atoi(amount);

	if (!(ch = get_char_by_obj(obj, name)))
	{
		obj_log(obj, "oskilladd: target not found");
		return;
	}

	if (isSkill)
		trg_skilladd(ch, skillnum, skilldiff);
	else
		trg_recipeadd(ch, skillnum, skilldiff);
}


OCMD(do_ospellturn)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount)
	{
		obj_log(obj, "ospellturn: too few arguments");
		return;
	}

	if ((pos = strchr(spellname, '.')))
		* pos = ' ';

	if ((spellnum = find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
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
		trg_spellturn(ch, spellnum, spelldiff);
	}
	else
	{
		obj_log(obj, "ospellturn: target not found");
		return;
	}
}


OCMD(do_ospelladd)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount)
	{
		obj_log(obj, "ospelladd: too few arguments");
		return;
	}

	if ((pos = strchr(spellname, '.')))
		* pos = ' ';

	if ((spellnum = find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		obj_log(obj, "ospelladd: spell not found");
		return;
	}

	spelldiff = atoi(amount);

	if ((ch = get_char_by_obj(obj, name)))
	{
		trg_spelladd(ch, spellnum, spelldiff);
	}
	else
	{
		obj_log(obj, "ospelladd: target not found");
		return;
	}
}

OCMD(do_ospellitem)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], turn[MAX_INPUT_LENGTH], *pos;
	int spellnum = 0, spelldiff = 0, spell = 0;

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn)
	{
		obj_log(obj, "ospellitem: too few arguments");
		return;
	}

	if ((pos = strchr(spellname, '.')))
		* pos = ' ';

	if ((spellnum = find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		obj_log(obj, "ospellitem: spell not found");
		return;
	}

	if (!str_cmp(type, "potion"))
		spell = SPELL_POTION;
	else if (!str_cmp(type, "wand"))
		spell = SPELL_WAND;
	else if (!str_cmp(type, "scroll"))
		spell = SPELL_SCROLL;
	else if (!str_cmp(type, "items"))
		spell = SPELL_ITEMS;
	else if (!str_cmp(type, "runes"))
		spell = SPELL_RUNES;
	else
	{
		obj_log(obj, "ospellitem: type spell not found");
		return;
	}

	if (!str_cmp(turn, "set"))
		spelldiff = 1;
	else if (!str_cmp(turn, "clear"))
		spelldiff = 0;
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
	int cmd, length;
	char *line, arg[MAX_INPUT_LENGTH];

	skip_spaces(&argument);

	// just drop to next line for hitting CR
	if (!*argument)
		return;

	line = any_one_arg(argument, arg);


	// find the command
	for (length = strlen(arg), cmd = 0; *obj_cmd_info[cmd].command != '\n'; cmd++)
		if (!strncmp(obj_cmd_info[cmd].command, arg, length))
			break;

	if (*obj_cmd_info[cmd].command == '\n')
	{
		sprintf(buf2, "Unknown object cmd: '%s'", argument);
		obj_log(obj, buf2, LGH);
	}
	else
		((*obj_cmd_info[cmd].command_pointer)
				(obj, line, cmd, obj_cmd_info[cmd].subcmd));
}
