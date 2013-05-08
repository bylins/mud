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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "db.h"
#include "im.h"
#include "features.hpp"
#include "deathtrap.hpp"
#include "char.hpp"
#include "skills.h"
#include "room.hpp"
#include "magic.h"
#include "fight.h"

extern const char *dirs[];
extern struct zone_data *zone_table;

void die(CHAR_DATA * ch, CHAR_DATA * killer);
void sub_write(char *arg, CHAR_DATA * ch, byte find_invis, int targets);
void send_to_zone(char *messg, int zone_rnum);
void asciiflag_conv(const char *flag, void *value);
CHAR_DATA *get_char_by_room(room_data * room, char *name);
room_data *get_room(char *name);
OBJ_DATA *get_obj_by_room(room_data * room, char *name);
#define WCMD(name)  \
    void (name)(room_data *room, char *argument, int cmd, int subcmd)

extern int reloc_target;
extern TRIG_DATA *cur_trig;

struct wld_command_info
{
	const char *command;
	void (*command_pointer)
	(room_data * room, char *argument, int cmd, int subcmd);
	int subcmd;
};


// do_wsend
#define SCMD_WSEND        0
#define SCMD_WECHOAROUND  1



// attaches room vnum to msg and sends it to script_log
void wld_log(room_data * room, const char *msg, const int type = 0)
{
	char buf[MAX_INPUT_LENGTH + 100];

	sprintf(buf, "(Room: %d, trig: %d): %s", room->number, last_trig_vnum, msg);
	script_log(buf, type);
}


// sends str to room
void act_to_room(char *str, room_data * room)
{
	// no one is in the room
	if (!room->people)
		return;

	/*
	 * since you can't use act(..., TO_ROOM) for an room, send it
	 * TO_ROOM and TO_CHAR for some char in the room.
	 * (just dont use $n or you might get strange results)
	 */
	act(str, FALSE, room->people, 0, 0, TO_ROOM);
	act(str, FALSE, room->people, 0, 0, TO_CHAR);
}



// World commands

// prints the argument to all the rooms aroud the room
WCMD(do_wasound)
{
	int door;

	skip_spaces(&argument);

	if (!*argument)
	{
		wld_log(room, "wasound called with no argument");
		return;
	}

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		EXIT_DATA *exit;

		if ((exit = room->dir_option[door]) && (exit->to_room != NOWHERE) && room != world[exit->to_room])
			act_to_room(argument, world[exit->to_room]);
	}
}


WCMD(do_wecho)
{
	skip_spaces(&argument);

	if (!*argument)
		wld_log(room, "wecho called with no args");
	else
		act_to_room(argument, room);
}


WCMD(do_wsend)
{
	char buf[MAX_INPUT_LENGTH], *msg;
	CHAR_DATA *ch;

	msg = any_one_arg(argument, buf);

	if (!*buf)
	{
		wld_log(room, "wsend called with no args");
		return;
	}

	skip_spaces(&msg);

	if (!*msg)
	{
		wld_log(room, "wsend called without a message");
		return;
	}

	if ((ch = get_char_by_room(room, buf)))
	{
		if (reloc_target != -1 && reloc_target != IN_ROOM(ch))
		{
			sprintf(buf,
					"&YВНИМАНИЕ&G Неверное использование команды wat в триггере %s (VNUM=%d).",
					GET_TRIG_NAME(cur_trig), GET_TRIG_VNUM(cur_trig));
			mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
		}
		if (subcmd == SCMD_WSEND)
			sub_write(msg, ch, TRUE, TO_CHAR);
		else if (subcmd == SCMD_WECHOAROUND)
			sub_write(msg, ch, TRUE, TO_ROOM);
	}
	else
		wld_log(room, "no target found for wsend", LGH);
}

int real_zone(int number)
{
	int counter;

	for (counter = 0; counter <= top_of_zone_table; counter++)
		if ((number >= (zone_table[counter].number * 100)) && (number <= (zone_table[counter].top)))
			return counter;

	return -1;
}

WCMD(do_wzoneecho)
{
	int zone;
	char zone_name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

	msg = any_one_arg(argument, zone_name);
	skip_spaces(&msg);

	if (!*zone_name || !*msg)
		wld_log(room, "wzoneecho called with too few args");
	else if ((zone = real_zone(atoi(zone_name))) < 0)
		wld_log(room, "wzoneecho called for nonexistant zone");
	else
	{
		sprintf(buf, "%s\r\n", msg);
		send_to_zone(buf, zone);
	}
}


WCMD(do_wdoor)
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
		wld_log(room, "wdoor called with too few args");
		return;
	}

	if ((rm = get_room(target)) == NULL)
	{
		wld_log(room, "wdoor: invalid target");
		return;
	}

	if ((dir = search_block(direction, dirs, FALSE)) == -1)
	{
		wld_log(room, "wdoor: invalid direction");
		return;
	}

	if ((fd = search_block(field, door_field, FALSE)) == -1)
	{
		wld_log(room, "wdoor: invalid field");
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
		case 1:	// description //
			if (exit->general_description)
				free(exit->general_description);
			CREATE(exit->general_description, char, strlen(value) + 3);
			strcpy(exit->general_description, value);
			strcat(exit->general_description, "\r\n");
			break;
		case 2:	// flags       //
			asciiflag_conv(value, &exit->exit_info);
			break;
		case 3:	// key         //
			exit->key = atoi(value);
			break;
		case 4:	// name        //
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
		case 5:	// room        //
			if ((to_room = real_room(atoi(value))) != NOWHERE)
				exit->to_room = to_room;
			else
				wld_log(room, "wdoor: invalid door target");
			break;
		case 6:	// lock - сложность замка         //
			lock = atoi(value);
			if (!(lock < 0 || lock >255))
				exit->lock_complexity = lock;
			else
				wld_log(room, "wdoor: invalid lock complexity");
			break;
		}
	}
}


WCMD(do_wteleport)
{
	CHAR_DATA *ch, *next_ch, *horse, *charmee, *ncharmee;
	int target, nr;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2)
	{
		wld_log(room, "wteleport called with too few args");
		return;
	}

	nr = atoi(arg2);
	target = real_room(nr);

	if (target == NOWHERE)
		wld_log(room, "wteleport target is an invalid room");
	else if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
	{
		if (nr == room->number)
		{
			wld_log(room, "wteleport all target is itself");
			return;
		}
		for (ch = room->people; ch; ch = next_ch)
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
		if ((ch = get_char_by_room(room, arg1)))
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
			wld_log(room, "wteleport: no target found");
	}
}


WCMD(do_wforce)
{
	CHAR_DATA *ch, *next_ch;
	char arg1[MAX_INPUT_LENGTH], *line;

	line = one_argument(argument, arg1);

	if (!*arg1 || !*line)
	{
		wld_log(room, "wforce called with too few args");
		return;
	}

	if (!str_cmp(arg1, "all") || !str_cmp(arg1, "все"))
	{
		for (ch = room->people; ch; ch = next_ch)
		{
			next_ch = ch->next_in_room;
			if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
			{
				command_interpreter(ch, line);
			}
		}
	}
	else
	{
		if ((ch = get_char_by_room(room, arg1)))
		{
			if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
			{
				command_interpreter(ch, line);
			}
		}
		else
			wld_log(room, "wforce: no target found");
	}
}


// increases the target's exp //
WCMD(do_wexp)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

	two_arguments(argument, name, amount);

	if (!*name || !*amount)
	{
		wld_log(room, "wexp: too few arguments");
		return;
	}

	if ((ch = get_char_by_room(room, name)))
		gain_exp(ch, atoi(amount));
	else
	{
		wld_log(room, "wexp: target not found");
		return;
	}
}



// purge all objects an npcs in room, or specified object or mob //
WCMD(do_wpurge)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *ch /*, *next_ch */;
	OBJ_DATA *obj /*, *next_obj */;

	one_argument(argument, arg);

	if (!*arg)
	{
		return;
/*
		for (ch = room->people; ch; ch = next_ch)
		{
			next_ch = ch->next_in_room;
			if (IS_NPC(ch))
			{
				if (ch->followers || ch->master)
					die_follower(ch);
				extract_char(ch, FALSE);
			}
		}

		for (obj = room->contents; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
			extract_obj(obj);
		}

		return;
*/
	}

	if (!(ch = get_char_by_room(room, arg)))
	{
		if ((obj = get_obj_by_room(room, arg)))
		{
			extract_obj(obj);
		}
		else
			wld_log(room, "wpurge: bad argument");
		return;
	}

	if (!IS_NPC(ch))
	{
		wld_log(room, "wpurge: purging a PC");
		return;
	}

	if (ch->followers || ch->master)
		die_follower(ch);
	extract_char(ch, FALSE);
}


// loads a mobile or object into the room //
WCMD(do_wload)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int number = 0;
	CHAR_DATA *mob;
	OBJ_DATA *object;


	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
	{
		wld_log(room, "wload: bad syntax");
		return;
	}

	if (is_abbrev(arg1, "mob"))
	{
		if ((mob = read_mobile(number, VIRTUAL)) == NULL)
		{
			wld_log(room, "wload: bad mob vnum");
			return;
		}
		char_to_room(mob, real_room(room->number));
		load_mtrigger(mob);
	}
	else if (is_abbrev(arg1, "obj"))
	{
		if ((object = read_object(number, VIRTUAL)) == NULL)
		{
			wld_log(room, "wload: bad object vnum");
			return;
		}
		log("Load obj #%d by %s (wload)", number, room->name);
		GET_OBJ_ZONE(object) = world[real_room(room->number)]->zone;
		obj_to_room(object, real_room(room->number));
		load_otrigger(object);
	}
	else
		wld_log(room, "wload: bad type");
}

// increases spells & skills //
const char *skill_name(int num);
const char *spell_name(int num);
int find_skill_num(const char *name);
int find_spell_num(char *name);



WCMD(do_wdamage)
{
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
	int dam = 0;
	CHAR_DATA *ch;

	two_arguments(argument, name, amount);

	if (!*name || !*amount || !isdigit(*amount))
	{
		wld_log(room, "wdamage: bad syntax");
		return;
	}

	dam = atoi(amount);

	if ((ch = get_char_by_room(room, name)))
	{
		if (world[IN_ROOM(ch)]->zone != room->zone)
			return;

		if (GET_LEVEL(ch) >= LVL_IMMORT && dam > 0)
		{
			send_to_char("Будучи бессмертным, вы избежали повреждения...", ch);
			return;
		}
		GET_HIT(ch) -= dam;
		if (dam < 0)
		{
			send_to_char("Вы почувствовали себя лучше.\r\n", ch);
			return;
		}

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
		wld_log(room, "wdamage: target not found");
}

WCMD(do_wat)
{
	char location[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int vnum, rnum = 0;
//    room_data *r2;

	void wld_command_interpreter(room_data * room, char *argument);

	half_chop(argument, location, arg2);

	if (!*location || !*arg2 || !isdigit(*location))
	{
		wld_log(room, "wat: bad syntax");
		return;
	}
	vnum = atoi(location);
	rnum = real_room(vnum);
	if (NOWHERE == rnum)
	{
		wld_log(room, "wat: location not found");
		return;
	}

	reloc_target = rnum;
	wld_command_interpreter(world[rnum], arg2);
	reloc_target = -1;
}

WCMD(do_wfeatturn)
{
	int isFeat = 0;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], featname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int featnum = 0, featdiff = 0;

	one_argument(two_arguments(argument, name, featname), amount);

	if (!*name || !*featname || !*amount)
	{
		wld_log(room, "wfeatturn: too few arguments");
		return;
	}

	while ((pos = strchr(featname, '.')))
		* pos = ' ';
	while ((pos = strchr(featname, '_')))
		* pos = ' ';

	if ((featnum = find_feat_num(featname)) > 0 && featnum <= MAX_FEATS)
		isFeat = 1;
	else
	{
		wld_log(room, "wfeatturn: feature not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		featdiff = 1;
	else if (!str_cmp(amount, "clear"))
		featdiff = 0;
	else
	{
		wld_log(room, "wfeatturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_room(room, name)))
	{
		wld_log(room, "wfeatturn: target not found");
		return;
	}

	if (isFeat)
		trg_featturn(ch, featnum, featdiff);
}

WCMD(do_wskillturn)
{
	int isSkill = 0;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int skillnum = 0, skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		wld_log(room, "wskillturn: too few arguments");
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
		wld_log(room, "wskillturn: skill/recipe not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		skilldiff = 1;
	else if (!str_cmp(amount, "clear"))
		skilldiff = 0;
	else
	{
		wld_log(room, "wskillturn: unknown set variable");
		return;
	}

	if (!(ch = get_char_by_room(room, name)))
	{
		wld_log(room, "wskillturn: target not found");
		return;
	}

	if (isSkill)
		trg_skillturn(ch, skillnum, skilldiff);
	else
		trg_recipeturn(ch, skillnum, skilldiff);
}


WCMD(do_wskilladd)
{
	int isSkill = 0;
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], skillname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int skillnum = 0, skilldiff = 0;

	one_argument(two_arguments(argument, name, skillname), amount);

	if (!*name || !*skillname || !*amount)
	{
		wld_log(room, "wskilladd: too few arguments");
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
		wld_log(room, "wskilladd: skill/recipe not found");
		return;
	}

	skilldiff = atoi(amount);

	if (!(ch = get_char_by_room(room, name)))
	{
		wld_log(room, "wskilladd: target not found");
		return;
	}

	if (isSkill)
		trg_skilladd(ch, skillnum, skilldiff);
	else
		trg_recipeadd(ch, skillnum, skilldiff);
}




WCMD(do_wspellturn)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int spellnum = 0, spelldiff = 0;

//    one_argument (two_arguments(argument, name, spellname), amount);
	argument = one_argument(argument, name);
	two_arguments(argument, spellname, amount);


	if (!*name || !*spellname || !*amount)
	{
		wld_log(room, "wspellturn: too few arguments");
		return;
	}

	if ((pos = strchr(spellname, '.')))
		* pos = ' ';

	if ((spellnum = find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		wld_log(room, "wspellturn: spell not found");
		return;
	}

	if (!str_cmp(amount, "set"))
		spelldiff = 1;
	else if (!str_cmp(amount, "clear"))
		spelldiff = 0;
	else
	{
		wld_log(room, "wspellturn: unknown set variable");
		return;
	}

	if ((ch = get_char_by_room(room, name)))
	{
		trg_spellturn(ch, spellnum, spelldiff);
	}
	else
	{
		wld_log(room, "wspellturn: target not found");
		return;
	}
}


WCMD(do_wspelladd)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], *pos;
	int spellnum = 0, spelldiff = 0;

	one_argument(two_arguments(argument, name, spellname), amount);

	if (!*name || !*spellname || !*amount)
	{
		wld_log(room, "wspelladd: too few arguments");
		return;
	}

	if ((pos = strchr(spellname, '.')))
		* pos = ' ';

	if ((spellnum = find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		wld_log(room, "wspelladd: spell not found");
		return;
	}

	spelldiff = atoi(amount);

	if ((ch = get_char_by_room(room, name)))
	{
		trg_spelladd(ch, spellnum, spelldiff);
	}
	else
	{
		wld_log(room, "wspelladd: target not found");
		return;
	}
}

WCMD(do_wspellitem)
{
	CHAR_DATA *ch;
	char name[MAX_INPUT_LENGTH], spellname[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], turn[MAX_INPUT_LENGTH], *pos;
	int spellnum = 0, spelldiff = 0, spell = 0;

	two_arguments(two_arguments(argument, name, spellname), type, turn);

	if (!*name || !*spellname || !*type || !*turn)
	{
		wld_log(room, "wspellitem: too few arguments");
		return;
	}

	if ((pos = strchr(spellname, '.')))
		* pos = ' ';

	if ((spellnum = find_spell_num(spellname)) < 0 || spellnum == 0 || spellnum > MAX_SPELLS)
	{
		wld_log(room, "wspellitem: spell not found");
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
		wld_log(room, "wspellitem: type spell not found");
		return;
	}

	if (!str_cmp(turn, "set"))
		spelldiff = 1;
	else if (!str_cmp(turn, "clear"))
		spelldiff = 0;
	else
	{
		wld_log(room, "wspellitem: unknown set variable");
		return;
	}

	if ((ch = get_char_by_room(room, name)))
	{
		trg_spellitem(ch, spellnum, spelldiff, spell);
	}
	else
	{
		wld_log(room, "wspellitem: target not found");
		return;
	}
}

/* Команда открывает пентаграмму из текущей комнаты в заданную комнату
   синтаксис wportal <номер комнаты> <длительность портала>
*/
WCMD(do_wportal)
{
	int target, howlong, curroom, nr;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	argument = two_arguments(argument, arg1, arg2);
	skip_spaces(&argument);

	if (!*arg1 || !*arg2)
	{
		wld_log(room, "wportal: called with too few args");
		return;
	}

	howlong = atoi(arg2);
	nr = atoi(arg1);
	target = real_room(nr);

	if (target == NOWHERE)
	{
		wld_log(room, "wportal: target is an invalid room");
		return;
	}

	/* Ставим пентаграмму из текущей комнаты в комнату target с
	   длительностью howlong */
	curroom = real_room(room->number);
	world[curroom]->portal_room = target;
	world[curroom]->portal_time = howlong;
	world[curroom]->pkPenterUnique = 0;
	OneWayPortal::add(world[target], world[curroom]);
	act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[curroom]->people, 0, 0, TO_CHAR);
	act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[curroom]->people, 0, 0, TO_ROOM);
}

const struct wld_command_info wld_cmd_info[] =
{
	{"RESERVED", 0, 0},	// this must be first -- for specprocs

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
	{"wportal", do_wportal, 0},
	{"\n", 0, 0}		// this must be last
};


// *  This is the command interpreter used by rooms, called by script_driver.
void wld_command_interpreter(room_data * room, char *argument)
{
	int cmd, length;
	char *line, arg[MAX_INPUT_LENGTH];

	skip_spaces(&argument);

	// just drop to next line for hitting CR
	if (!*argument)
		return;

	line = any_one_arg(argument, arg);


	// find the command
	for (length = strlen(arg), cmd = 0; *wld_cmd_info[cmd].command != '\n'; cmd++)
		if (!strncmp(wld_cmd_info[cmd].command, arg, length))
			break;

	if (*wld_cmd_info[cmd].command == '\n')
	{
		sprintf(buf2, "Unknown world cmd: '%s'", argument);
		wld_log(room, buf2, LGH);
	}
	else
		((*wld_cmd_info[cmd].command_pointer)
				(room, line, cmd, wld_cmd_info[cmd].subcmd));
}
