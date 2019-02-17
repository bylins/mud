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

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "dg_scripts.h"
#include "obj.hpp"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "spell_parser.hpp"
#include "spells.h"
#include "im.h"
#include "features.hpp"
#include "char.hpp"
#include "skills.h"
#include "name_list.hpp"
#include "room.hpp"
#include "fight.h"
#include "fight_hit.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

struct mob_command_info
{
	const char *command;
	byte minimum_position;	
	typedef void(*handler_f)(CHAR_DATA* ch, char *argument, int cmd, int subcmd);
	handler_f command_pointer;
	int subcmd;				///< Subcommand. See SCMD_* constants.
	bool use_in_lag;
};

#define IS_CHARMED(ch)          (IS_HORSE(ch)||AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))

extern DESCRIPTOR_DATA *descriptor_list;
extern INDEX_DATA *mob_index;

extern const char *dirs[];

extern int reloc_target;
extern TRIG_DATA *cur_trig;

void sub_write(char *arg, CHAR_DATA * ch, byte find_invis, int targets);
ROOM_DATA *get_room(char *name);
OBJ_DATA *get_obj_by_char(CHAR_DATA * ch, char *name);
extern void die(CHAR_DATA * ch, CHAR_DATA * killer);
// * Local functions.
void mob_command_interpreter(CHAR_DATA* ch, char *argument);
bool mob_script_command_interpreter(CHAR_DATA* ch, char *argument);

// attaches mob's name and vnum to msg and sends it to script_log
void mob_log(CHAR_DATA * mob, const char *msg, const int type = 0)
{
	char buf[MAX_INPUT_LENGTH + 100];

	sprintf(buf, "(Mob: '%s', VNum: %d, trig: %d): %s", GET_SHORT(mob), GET_MOB_VNUM(mob), last_trig_vnum, msg);
	script_log(buf, type);
}

// * macro to determine if a mob is permitted to use these commands
#define MOB_OR_IMPL(ch) \
        (IS_NPC(ch) && (!(ch)->desc || GET_LEVEL((ch)->desc->original)>=LVL_IMPL))

// mob commands

//returns the real room number, or NOWHERE if not found or invalid
//copy from find_target_room except char's messages
room_rnum dg_find_target_room(CHAR_DATA * ch, char *rawroomstr)
{
	char roomstr[MAX_INPUT_LENGTH];
	room_rnum location = NOWHERE;

	one_argument(rawroomstr, roomstr);

	if (!*roomstr)
	{
		sprintf(buf, "Undefined mteleport room: %s", rawroomstr);
		mob_log(ch, buf);
		return NOWHERE;
	}

	auto tmp = atoi(roomstr);
	if (tmp > 0)
	{
		location = real_room(tmp);
	}
	else
	{
		sprintf(buf, "Undefined mteleport room: %s", roomstr);
		mob_log(ch, buf);
		return NOWHERE;
	}

	return location;
}

// prints the argument to all the rooms aroud the mobile
void do_masound(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (!*argument)
	{
		mob_log(ch, "masound called with no argument");
		return;
	}

	skip_spaces(&argument);

	int temp_in_room = ch->in_room;
	for (int door = 0; door < NUM_OF_DIRS; door++)
	{
		const auto& exit = world[temp_in_room]->dir_option[door];
		if (exit
			&& exit->to_room() != NOWHERE
			&& exit->to_room() != temp_in_room)
		{
			ch->in_room = exit->to_room();
			sub_write(argument, ch, TRUE, TO_ROOM);
		}
	}

	ch->in_room = temp_in_room;
}

// lets the mobile kill any player or mobile without murder
void do_mkill(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (MOB_FLAGGED(ch, MOB_NOFIGHT))
	{
		mob_log(ch, "mkill called for mob with NOFIGHT flag");
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		mob_log(ch, "mkill called with no argument");
		return;
	}

	if (*arg == UID_CHAR)
	{
		if (!(victim = get_char(arg)))
		{
			sprintf(buf, "mkill: victim (%s) not found", arg + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_room_vis(ch, arg)))
	{
		sprintf(buf, "mkill: victim (%s) not found", arg);
		mob_log(ch, buf);
		return;
	}

	if (victim == ch)
	{
		mob_log(ch, "mkill: victim is self");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)
		&& ch->get_master() == victim)
	{
		mob_log(ch, "mkill: charmed mob attacking master");
		return;
	}

	if (ch->get_fighting())
	{
		mob_log(ch, "mkill: already fighting");
		return;
	}

	hit(ch, victim, TYPE_UNDEFINED, 1);
	return;
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
void do_mjunk(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	int pos, junk_all = 0;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		mob_log(ch, "mjunk called with no argument");
		return;
	}

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все"))
		junk_all = 1;

	if ((find_all_dots(arg) == FIND_INDIV) && !junk_all)
	{
		if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos)) != NULL)
		{
			unequip_char(ch, pos);
			extract_obj(obj);
			return;
		}
		if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying)) != NULL)
			extract_obj(obj);
		return;
	}
	else
	{
		for (obj = ch->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->get_next_content();
			if (arg[3] == '\0' || isname(arg + 4, obj->get_aliases()))
			{
				extract_obj(obj);
			}
		}
		while ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos)))
		{
			unequip_char(ch, pos);
			extract_obj(obj);
		}
	}
	return;
}

// prints the message to everyone in the room other than the mob and victim
void do_mechoaround(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char *p;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	p = one_argument(argument, arg);
	skip_spaces(&p);

	if (!*arg)
	{
		mob_log(ch, "mechoaround called with no argument");
		return;
	}

	if (*arg == UID_CHAR)
	{
		if (!(victim = get_char(arg)))
		{
			sprintf(buf, "mechoaround: victim (%s) does not exist", arg + 1);
			mob_log(ch, buf, LGH);
			return;
		}
	}
	else if (!(victim = get_char_room_vis(ch, arg)))
	{
		sprintf(buf, "mechoaround: victim (%s) does not exist", arg);
		mob_log(ch, buf, LGH);
		return;
	}

	if (reloc_target != -1 && reloc_target != IN_ROOM(victim))
	{
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
	}

	sub_write(p, victim, TRUE, TO_ROOM);
}

// sends the message to only the victim
void do_msend(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char *p;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	p = one_argument(argument, arg);
	skip_spaces(&p);

	if (!*arg)
	{
		mob_log(ch, "msend called with no argument");
		return;
	}

	if (*arg == UID_CHAR)
	{
		if (!(victim = get_char(arg)))
		{
			sprintf(buf, "msend: victim (%s) does not exist", arg + 1);
			mob_log(ch, buf, LGH);
			return;
		}
	}
	else if (!(victim = get_char_room_vis(ch, arg)))
	{
//		sprintf(buf, "msend: victim (%s) does not exist", arg);
//		mob_log(ch, buf, LGH);
		return;
	}

	if (reloc_target != -1 && reloc_target != IN_ROOM(victim))
	{
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
	}

	sub_write(p, victim, TRUE, TO_CHAR);
}

// prints the message to the room at large
void do_mecho(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char *p;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (!*argument)
	{
		mob_log(ch, "mecho called with no arguments");
		return;
	}
	p = argument;
	skip_spaces(&p);

	if (reloc_target != -1 && reloc_target != ch->in_room)
	{
		sprintf(buf,
				"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
				GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
		mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
	}

	sub_write(p, ch, TRUE, TO_ROOM);
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
void do_mload(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int number = 0;
	CHAR_DATA *mob;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		mob_log(ch, "mload: in charm");
		return;
	}
	if (ch->desc && GET_LEVEL(ch->desc->original) < LVL_IMPL)
	{
		mob_log(ch, "mload: not IMPL");
		return;
	}
	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
	{
		mob_log(ch, "mload: bad syntax");
		return;
	}

	if (is_abbrev(arg1, "mob"))
	{
		if ((mob = read_mobile(number, VIRTUAL)) == NULL)
		{
			mob_log(ch, "mload: bad mob vnum");
			return;
		}
		log("Load mob #%d by %s (mload)", number, GET_NAME(ch));
		char_to_room(mob, ch->in_room);
		load_mtrigger(mob);
	}
	else if (is_abbrev(arg1, "obj"))
	{
		const auto object = world_objects.create_from_prototype_by_vnum(number);
		if (!object)
		{
			mob_log(ch, "mload: bad object vnum");
			return;
		}

		if (GET_OBJ_MIW(obj_proto[object->get_rnum()]) >= 0
			&& obj_proto.actual_count(object->get_rnum()) > GET_OBJ_MIW(obj_proto[object->get_rnum()]))
		{
			if (!check_unlimited_timer(obj_proto[object->get_rnum()].get()))
			{
				sprintf(buf, "mload: Попытка загрузить предмет больше чем в MIW для #%d, предмет удален.", number);
				mob_log(ch, buf);
//				extract_obj(object.get());
//				return;
			}
		}

		log("Load obj #%d by %s (mload)", number, GET_NAME(ch));
		object->set_zone(world[ch->in_room]->zone);

		if (CAN_WEAR(object.get(), EWearFlag::ITEM_WEAR_TAKE))
		{
			obj_to_char(object.get(), ch);
		}
		else
		{
			obj_to_room(object.get(), ch->in_room);
		}

		load_otrigger(object.get());
	}
	else
		mob_log(ch, "mload: bad type");
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
void do_mpurge(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		return;
	}

	if (*arg == UID_CHAR)
		victim = get_char(arg);
	else
		victim = get_char_room_vis(ch, arg);

	if (victim == NULL)
	{
		if ((obj = get_obj_by_char(ch, arg)))
		{
			extract_obj(obj);
		}
		else
		{
			mob_log(ch, "mpurge: bad argument");
		}
		return;
	}

	if (!IS_NPC(victim))
	{
		mob_log(ch, "mpurge: purging a PC");
		return;
	}

	if (victim->followers
		|| victim->has_master())
	{
		die_follower(victim);
	}

	extract_char(victim, FALSE);
}

// lets the mobile goto any location it wishes that is not private
void do_mgoto(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	int location;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		mob_log(ch, "mgoto called with no argument");
		return;
	}

	if ((location = dg_find_target_room(ch, arg)) == NOWHERE)
	{
		sprintf(buf, "mgoto: invalid location '%s'", arg);
		mob_log(ch, buf);
		return;
	}

	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	char_from_room(ch);
	char_to_room(ch, location);
}

// lets the mobile do a command at another location. Very useful
void do_mat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	int location;
	int original;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	argument = one_argument(argument, arg);

	if (!*arg || !*argument)
	{
		mob_log(ch, "mat: bad argument");
		return;
	}

	if ((location = dg_find_target_room(ch, arg)) == NOWHERE)
	{
		sprintf(buf, "mat: invalid location '%s'", arg);
		mob_log(ch, buf);
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
void do_mteleport(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int target;
	room_rnum from_room;
	CHAR_DATA *vict, *horse;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2)
	{
		mob_log(ch, "mteleport: bad syntax");
		return;
	}

	target = dg_find_target_room(ch, arg2);

	if (target == NOWHERE)
	{
		mob_log(ch, "mteleport target is an invalid room");
	}
	else if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
	{
		if (target == ch->in_room)
		{
			mob_log(ch, "mteleport all: target is itself");
			return;
		}

		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy)
		{
			if (IS_NPC(vict)
				&& !(IS_HORSE(vict)
					|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
					|| MOB_FLAGGED(vict, MOB_ANGEL)
					|| MOB_FLAGGED(vict, MOB_GHOST)))
			{
				continue;
			}

			if (IN_ROOM(vict) == NOWHERE)
			{
				mob_log(ch, "mteleport transports from NOWHERE");

				return;
			}

			char_from_room(vict);
			char_to_room(vict, target);

			look_at_room(vict, TRUE);
		}
	}
	else
	{
		if (*arg1 == UID_CHAR)
		{
			if (!(vict = get_char(arg1)))
			{
				sprintf(buf, "mteleport: victim (%s) does not exist", arg1 + 1);
				mob_log(ch, buf);
				return;
			}
		}
		else if (!(vict = get_char_vis(ch, arg1, FIND_CHAR_WORLD)))
		{
			sprintf(buf, "mteleport: victim (%s) does not exist", arg1);
			mob_log(ch, buf);
			return;
		}

		const auto people_copy = world[IN_ROOM(vict)]->people;
		for (const auto charmee : people_copy)
		{
			if (IS_NPC(charmee)
				&& (AFF_FLAGGED(charmee, EAffectFlag::AFF_CHARM)
					|| MOB_FLAGGED(charmee, MOB_ANGEL)
					|| MOB_FLAGGED(charmee, MOB_GHOST))
				&& charmee->get_master() == vict)
			{
				char_from_room(charmee);
				char_to_room(charmee, target);
			}
		}

		if (on_horse(vict)
			|| has_horse(vict, TRUE))
		{
			horse = get_horse(vict);
		}
		else
		{
			horse = NULL;
		}

		from_room = vict->in_room;

		char_from_room(vict);
		char_to_room(vict, target);
		if (!str_cmp(argument, "horse") && horse)
		{
			char_from_room(horse);
			char_to_room(horse, target);
		}

//Polud реализуем режим followers. за аргументом телепорта перемешаются все последователи-NPC
		if (!str_cmp(argument, "followers") && vict->followers)
		{
			follow_type *ft;
			for (ft = vict->followers; ft; ft = ft->next)
			{
				if (IN_ROOM(ft->follower) == from_room && IS_NPC(ft->follower))
				{
					char_from_room(ft->follower);
					char_to_room(ft->follower, target);
				}
			}
		}
//-Polud
		check_horse(vict);
		look_at_room(vict, TRUE);
	}
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
void do_mforce(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	argument = one_argument(argument, arg);

	if (!*arg || !*argument)
	{
		mob_log(ch, "mforce: bad syntax");
		return;
	}

	if (!str_cmp(arg, "all")
		|| !str_cmp(arg, "все"))
	{
		mob_log(ch, "ERROR: \'mforce all\' command disabled.");
		return;
		//DESCRIPTOR_DATA *i;

		//// не знаю почему здесь идут только по плеерам, но раз так,
		//// то LVL_IMMORT+ для мобов здесь исключать пока нет смысла
		//for (i = descriptor_list; i; i = i->next)
		//{
		//	if ((i->character.get() != ch) && !i->connected && (IN_ROOM(i->character) == ch->in_room))
		//	{
		//		const auto vch = i->character;
		//		if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) && GET_LEVEL(vch) < LVL_IMMORT)
		//		{
		//			command_interpreter(vch.get(), argument);
		//		}
		//	}
		//}
	}
	else
	{
		CHAR_DATA *victim = nullptr;

		if (*arg == UID_CHAR)
		{
			if (!(victim = get_char(arg)))
			{
				sprintf(buf, "mforce: victim (%s) does not exist", arg + 1);
				mob_log(ch, buf);
				return;
			}
		}
		else if ((victim = get_char_room_vis(ch, arg)) == NULL)
		{
			mob_log(ch, "mforce: no such victim");
			return;
		}

		if (!IS_NPC(victim))
		{
			if ((!victim->desc))
			{
				return;
			}
		}

		if (victim == ch)
		{
			mob_log(ch, "mforce: forcing self");
			return;
		}

		if (IS_NPC(victim))
		{
			if (mob_script_command_interpreter(victim, argument))
			{
				mob_log(ch, "Mob trigger commands in mforce. Please rewrite trigger.");
				return;
			}

			command_interpreter(victim, argument);
		}
		else if(GET_LEVEL(victim) < LVL_IMMORT)
		{
			command_interpreter(victim, argument);
		}
	}
}

// increases the target's exp
void do_mexp(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

	mob_log(ch, "WARNING: mexp command is depracated! Use: %actor.exp(amount-to-add)%");

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	two_arguments(argument, name, amount);

	if (!*name || !*amount)
	{
		mob_log(ch, "mexp: too few arguments");
		return;
	}

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mexp: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mexp: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	}
	sprintf(buf, "mexp: victim (%s) получил опыт %d", name, atoi(amount));
	mob_log(ch, buf);
	gain_exp(victim, atoi(amount));
}

// increases the target's gold
void do_mgold(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

	mob_log(ch, "WARNING: mgold command is depracated! Use: %actor.gold(amount-to-add)%");

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	two_arguments(argument, name, amount);

	if (!*name || !*amount)
	{
		mob_log(ch, "mgold: too few arguments");
		return;
	}

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mgold: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mgold: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	}

	int num = atoi(amount);
	if (num >= 0)
	{
		victim->add_gold(num);
	}
	else
	{
		num = victim->remove_gold(num);
		if (num > 0)
		{
			mob_log(ch, "mgold subtracting more gold than character has");
		}
	}
}

// transform into a different mobile
void do_mtransform(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *m;
	OBJ_DATA *obj[NUM_WEARS];
	int keep_hp = 1;	// new mob keeps the old mob's hp/max hp/exp
	int pos;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc)
	{
		send_to_char("You've got no VNUM to return to, dummy! try 'switch'\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg)
		mob_log(ch, "mtransform: missing argument");
	else if (!a_isdigit(*arg) && *arg != '-')
		mob_log(ch, "mtransform: bad argument");
	else
	{
		if (a_isdigit(*arg))
			m = read_mobile(atoi(arg), VIRTUAL);
		else
		{
			keep_hp = 0;
			m = read_mobile(atoi(arg + 1), VIRTUAL);
		}
		if (m == NULL)
		{
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


		for (pos = 0; pos < NUM_WEARS; pos++)
		{
			if (GET_EQ(ch, pos))
				obj[pos] = unequip_char(ch, pos);
			else
				obj[pos] = NULL;
		}

		// put the mob in the same room as ch so extract will work
		char_to_room(m, ch->in_room);

// Обмен содержимым
		CHAR_DATA tmpmob(*m);
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

		if (keep_hp)
		{
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

		for (pos = 0; pos < NUM_WEARS; pos++)
		{
			if (obj[pos])
				equip_char(ch, obj[pos], pos | 0x40);
		}
		extract_char(m, FALSE);
	}
}

void do_mdoor(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
	char field[MAX_INPUT_LENGTH], *value;
	ROOM_DATA *rm;
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


	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);
	skip_spaces(&value);

	if (!*target || !*direction || !*field)
	{
		mob_log(ch, "mdoor called with too few args");
		return;
	}

	if ((rm = get_room(target)) == NULL)
	{
		mob_log(ch, "mdoor: invalid target");
		return;
	}

	if ((dir = search_block(direction, dirs, FALSE)) == -1)
	{
		mob_log(ch, "mdoor: invalid direction");
		return;
	}

	if ((fd = search_block(field, door_field, FALSE)) == -1)
	{
		mob_log(ch, "mdoor: invalid field");
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
		std::string::size_type i;

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
				mob_log(ch, "mdoor: invalid door target");
			}
			break;

		case 6:	// lock - сложность замка         
			lock = atoi(value);
			if (!(lock < 0 || lock >255))
				exit->lock_complexity = lock;
			else
				mob_log(ch, "mdoor: invalid lock complexity");
			break;
		}
	}
}

// increases spells & skills 
const char *skill_name(int num);
const char *spell_name(int num);
int fix_name_and_find_spell_num(char *name);

void do_mfeatturn(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int isFeat = 0;
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], featname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int featnum = 0, featdiff = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount)
	{
		mob_log(ch, "mfeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.')))
		*pos = ' ';
	while ((pos = strchr(featname, '_')))
		*pos = ' ';

	if ((featnum = find_feat_num(featname)) > 0 && featnum < MAX_FEATS)
		isFeat = 1;
	else
	{
		mob_log(ch, "mfeatturn: feature not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else
	{
		mob_log(ch, "mfeatturn: unknown set variable");
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mfeatturn: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mfeatturn: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	if (isFeat)
		trg_featturn(victim, featnum, featdiff, last_trig_vnum);
}

void do_mskillturn(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	bool isSkill = false;
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	ESkill skillnum = SKILL_INVALID;
	int recipenum = 0;
	int skilldiff = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		mob_log(ch, "mskillturn: too few arguments");
		return;
	}

	if ((skillnum = fix_name_and_find_skill_num(skillname)) > 0 && skillnum <= MAX_SKILL_NUM)
	{
		isSkill = 1;
	}
	else if ((recipenum = im_get_recipe_by_name(skillname)) < 0)
	{
		sprintf(buf, "mskillturn: %s skill/recipe not found", skillname);
		mob_log(ch, buf);
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
		mob_log(ch, "mskillturn: unknown set variable");
		return;
	}

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
	{
		return;
	}

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mskillturn: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mskillturn: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	if (isSkill)
	{
		if (skill_info[skillnum].classknow[GET_CLASS(victim)][GET_KIN(victim)] == KNOW_SKILL)
        {
            trg_skillturn(victim, skillnum, skilldiff, last_trig_vnum);
        }
		else 
		{
			sprintf(buf, "mskillturn: несоответсвие устанавливаемого умения классу игрока");
			mob_log(ch, buf);
		}
	}
	else
	{
		trg_recipeturn(victim, recipenum, skilldiff);
	}
}

void do_mskilladd(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	bool isSkill = false;
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	ESkill skillnum = SKILL_INVALID;
	int recipenum = 0;
	int skilldiff = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		mob_log(ch, "mskilladd: too few arguments");
		return;
	}

	if ((skillnum = fix_name_and_find_skill_num(skillname)) > 0 && skillnum <= MAX_SKILL_NUM)
	{
		isSkill = true;
	}
	else if ((recipenum = im_get_recipe_by_name(skillname)) < 0)
	{
		sprintf(buf, "mskilladd: %s skill/recipe not found", skillname);
		mob_log(ch, buf);
		return;
	}

	skilldiff = atoi(amount);

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
	{
		return;
	}

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mskilladd: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mskilladd: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	if (isSkill)
	{
		trg_skilladd(victim, skillnum, skilldiff, last_trig_vnum);
	}
	else
	{
		trg_recipeadd(victim, recipenum, skilldiff);
	}
}

void do_mspellturn(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int skillnum = 0, skilldiff = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	argument = one_argument(argument, name);
	two_arguments(argument, skillname, amount);

	if (!*name || !*skillname || !*amount)
	{
		mob_log(ch, "mspellturn: too few arguments");
		return;
	}

	if ((skillnum = fix_name_and_find_spell_num(skillname)) < 0 || skillnum == 0 || skillnum > MAX_SPELLS)
	{
		mob_log(ch, "mspellturn: spell not found");
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
		mob_log(ch, "mspellturn: unknown set variable");
		return;
	}

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mspellturn: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mspellturn: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spellturn(victim, skillnum, skilldiff, last_trig_vnum);
}

void do_mspellturntemp(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int spellnum = 0, spelltime = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	argument = one_argument(argument, name);
	two_arguments(argument, spellname, amount);

	if (!*name || !*spellname || !*amount)
	{
		mob_log(ch, "mspellturntemp: too few arguments");
		return;
	}

	if ((spellnum = fix_name_and_find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		mob_log(ch, "mspellturntemp: spell not found");
		return;
	}

	spelltime = atoi(amount);

	if(spelltime < 0)
	{
		mob_log(ch, "mspellturntemp: time is negative");
		return;
	}

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
		return;

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mspellturntemp: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mspellturntemp: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spellturntemp(victim, spellnum, spelltime, last_trig_vnum);
}

void do_mspelladd(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int skillnum = 0, skilldiff = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		mob_log(ch, "mspelladd: too few arguments");
		return;
	}

	if ((skillnum = fix_name_and_find_spell_num(skillname)) < 0 || skillnum == 0 || skillnum > MAX_SPELLS)
	{
		mob_log(ch, "mspelladd: skill not found");
		return;
	}

	skilldiff = atoi(amount);

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
	{
		return;
	}

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mspelladd: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mspelladd: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spelladd(victim, skillnum, skilldiff, last_trig_vnum);
}

void do_mspellitem(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], turn[MAX_INPUT_LENGTH];
	int spellnum = 0, spelldiff = 0, spell = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn)
	{
		mob_log(ch, "mspellitem: too few arguments");
		return;
	}

	if ((spellnum = fix_name_and_find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		mob_log(ch, "mspellitem: spell not found");
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
		mob_log(ch, "mspellitem: type spell not found");
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
		mob_log(ch, "mspellitem: unknown set variable");
		return;
	}

	if (*name == UID_CHAR)
	{
		if (!(victim = get_char(name)))
		{
			sprintf(buf, "mspellitem: victim (%s) does not exist", name + 1);
			mob_log(ch, buf);
			return;
		}
	}
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		sprintf(buf, "mspellitem: victim (%s) does not exist", name);
		mob_log(ch, buf);
		return;
	};

	trg_spellitem(victim, spellnum, spelldiff, spell);
}

void do_mdamage(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int dam = 0;

	if (!MOB_OR_IMPL(ch))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
	{
		return;
	}

	two_arguments(argument, name, amount);

	if (!*name || !*amount || !a_isdigit(*amount))
	{
		mob_log(ch, "mdamage: bad syntax");
		return;
	}

	dam = atoi(amount);
	auto victim = get_char(name);
	if (victim)
	{
		if (world[IN_ROOM(victim)]->zone != world[ch->in_room]->zone)
		{
			return;
		}

		if (GET_LEVEL(victim) >= LVL_IMMORT && dam > 0)
		{
			send_to_char
			("Будучи очень крутым, вы сделали шаг в сторону и не получили повреждений...\r\n", victim);
			return;
		}
		GET_HIT(victim) -= dam;
		if (dam < 0)
		{
			send_to_char("Вы почувствовали себя лучше.\r\n", victim);
			return;
		}

		update_pos(victim);
		char_dam_message(dam, victim, victim, 0);
		if (GET_POS(victim) == POS_DEAD)
		{
			if (!IS_NPC(victim))
			{
				sprintf(buf2, "%s killed by mobdamage at %s [%d]",
						GET_NAME(victim),
						IN_ROOM(victim) == NOWHERE ? "NOWHERE" : world[IN_ROOM(victim)]->name, GET_ROOM_VNUM(IN_ROOM(victim)));
				mudlog(buf2, BRF, 0, SYSLOG, TRUE);
			}
			die(victim, ch);
		}
	}
	else
	{
		mob_log(ch, "mdamage: target not found");
	}
}

const struct mob_command_info mob_cmd_info[] =
{
	{ "RESERVED", 0, 0, 0, 0 },	// this must be first -- for specprocs
	{ "masound", POS_DEAD, do_masound, -1, false},
	{ "mkill", POS_STANDING, do_mkill, -1, false},
	{ "mjunk", POS_SITTING, do_mjunk, -1, true},
	{ "mdamage", POS_DEAD, do_mdamage, -1, false},
	{ "mdoor", POS_DEAD, do_mdoor, -1, false},
	{ "mecho", POS_DEAD, do_mecho, -1, false},
	{ "mechoaround", POS_DEAD, do_mechoaround, -1, false},
	{ "msend", POS_DEAD, do_msend, -1, false},
	{ "mload", POS_DEAD, do_mload, -1, false},
	{ "mpurge", POS_DEAD, do_mpurge, -1, true},
	{ "mgoto", POS_DEAD, do_mgoto, -1, false},
	{ "mat", POS_DEAD, do_mat, -1, false},
	{ "mteleport", POS_DEAD, do_mteleport, -1, false},
	{ "mforce", POS_DEAD, do_mforce, -1, false},
	{ "mexp", POS_DEAD, do_mexp, -1, false},
	{ "mgold", POS_DEAD, do_mgold, -1, false},
	{ "mtransform", POS_DEAD, do_mtransform, -1, false},
	{ "mfeatturn", POS_DEAD, do_mfeatturn, -1, false},
	{ "mskillturn", POS_DEAD, do_mskillturn, -1, false},
	{ "mskilladd", POS_DEAD, do_mskilladd, -1, false},
	{ "mspellturn", POS_DEAD, do_mspellturn, -1, false},
	{ "mspellturntemp", POS_DEAD, do_mspellturntemp, -1, false},
	{ "mspelladd", POS_DEAD, do_mspelladd, -1, false},
	{ "mspellitem", POS_DEAD, do_mspellitem, -1, false},
	{ "\n", 0, 0, 0, 0}		// this must be last
};

bool mob_script_command_interpreter(CHAR_DATA* ch, char *argument)
{
	char *line, arg[MAX_INPUT_LENGTH];

	// just drop to next line for hitting CR
	skip_spaces(&argument);

	if (!*argument)
		return false;

	line = any_one_arg(argument, arg);

	// find the command
	int cmd = 0;
	const size_t length = strlen(arg);

	while (*mob_cmd_info[cmd].command != '\n')
	{
		if (!strncmp(mob_cmd_info[cmd].command, arg, length))
		{
			break;
		}
		cmd++;
	}

	if (!mob_cmd_info[cmd].use_in_lag &&
		(GET_MOB_HOLD(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)))
	{
		return false;
	}

	if (*mob_cmd_info[cmd].command == '\n')
	{
		return false;
	}
	else if (GET_POS(ch) < mob_cmd_info[cmd].minimum_position)
	{
		return false;
	}
	else
	{
		check_hiding_cmd(ch, -1);

		const mob_command_info::handler_f& command = mob_cmd_info[cmd].command_pointer;
		command(ch, line, cmd, mob_cmd_info[cmd].subcmd);

		return true;
	}
}

// *  This is the command interpreter used by mob, include common interpreter's commands
void mob_command_interpreter(CHAR_DATA* ch, char *argument)
{
	if (!mob_script_command_interpreter(ch, argument))
	{
		command_interpreter(ch, argument);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
