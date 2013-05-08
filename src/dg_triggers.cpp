/**************************************************************************
*  File: dg_triggers.cpp                                   Part of Bylins *
*                                                                         *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include <boost/lexical_cast.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "olc.h"
#include "char.hpp"
#include "room.hpp"
#include "spells.h"

extern INDEX_DATA **trig_index;

extern const char *dirs[];

#ifndef LVL_BUILDER
#define LVL_BUILDER LVL_GOD
#endif


// external functions from scripts.cpp
extern int script_driver(void *go, TRIG_DATA * trig, int type, int mode);
char *matching_quote(char *p);


// mob trigger types
const char *trig_types[] = { "Global",
							 "Random",
							 "Command",
							 "Speech",
							 "Act",
							 "Death",
							 "Greet",
							 "Greet-All",
							 "Entry",
							 "Receive",
							 "Fight",
							 "HitPrcnt",
							 "Bribe",
							 "Load",
							 "Memory",
							 "Damage",
							 "Great PC",
							 "Great-All PC",
							 "Income",
							 "Income PC",
							 "Агродействие",
							 "Раунд боя",
							 "Каст в моба",
							 "Смена времени",
							 "UNUSED",
							 "Auto",
							 "\n"
						   };


// obj trigger types
const char *otrig_types[] = { "Global",
							  "Random",
							  "Command",
							  "UNUSED",
							  "UNUSED",
							  "Timer",
							  "Get",
							  "Drop",
							  "Give",
							  "Wear",
							  "UNUSED",
							  "Remove",
							  "UNUSED",
							  "Load",
							  "Unlock",
							  "Open",
							  "Lock",
							  "Close",
							  "Взломать",
							  "Вход PC",
							  "Смена времени",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "Auto",
							  "\n"
							};


// wld trigger types
const char *wtrig_types[] = { "Global",
							  "Random",
							  "Command",
							  "Speech",
							  "Enter PC",
							  "Zone Reset",
							  "Enter",
							  "Drop",
							  "Unlock",
							  "Open",
							  "Lock",
							  "Close",
							  "Взломать",
							  "Смена времени",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "UNUSED",
							  "Auto",
							  "\n"
							};


// * General functions used by several triggers


// * Copy first phrase into first_arg, returns rest of string
char *one_phrase(char *arg, char *first_arg)
{
	skip_spaces(&arg);

	if (!*arg)
		*first_arg = '\0';
	else if (*arg == '"')
	{
		char *p, c;
		p = matching_quote(arg);
		c = *p;
		*p = '\0';
		strcpy(first_arg, arg + 1);
		if (c == '\0')
			return p;
		else
			return p + 1;
	}
	else
	{
		char *s, *p;

		s = first_arg;
		p = arg;

		while (*p && !a_isspace(*p) && *p != '"')
			*s++ = *p++;

		*s = '\0';
		return p;
	}

	return arg;
}


int is_substring(char *sub, char *string)
{
	char *s;

	if ((s = str_str(string, sub)))
	{
		int len = strlen(string);
		int sublen = strlen(sub);

		// check front
		if ((s == string || a_isspace(*(s - 1)) || ispunct(*(s - 1))) &&
				// check end
				((s + sublen == string + len) || a_isspace(s[sublen]) || ispunct(s[sublen])))
			return 1;
	}

	return 0;
}


/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
int word_check(char *str, char *wordlist)
{
	char words[MAX_INPUT_LENGTH], phrase[MAX_INPUT_LENGTH], *s;

	if (*wordlist == '*')
		return 1;

	strcpy(words, wordlist);

	for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase))
		if (is_substring(phrase, str))
			return 1;

	return 0;
}



// *  mob triggers

void random_mtrigger(CHAR_DATA * ch)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(ch, MTRIG_RANDOM) || AFF_FLAGGED(ch, AFF_CHARM))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_RANDOM) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void bribe_mtrigger(CHAR_DATA * ch, CHAR_DATA * actor, int amount)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_BRIBE) || !CAN_START_MTRIG(ch))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_BRIBE) && (amount >= GET_TRIG_NARG(t)))
		{
			sprintf(buf, "%d", amount);
			add_var_cntx(&GET_TRIG_VARS(t), "amount", buf, 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void greet_memory_mtrigger(CHAR_DATA * actor)
{
	TRIG_DATA *t;
	CHAR_DATA *ch;
	struct script_memory *mem;
	char buf[MAX_INPUT_LENGTH];
	int command_performed = 0;

	if (IN_ROOM(actor) == NOWHERE)
		return;

	for (ch = world[IN_ROOM(actor)]->people; ch; ch = ch->next_in_room)
	{
		if (!SCRIPT_MEM(ch) || !AWAKE(ch) || ch->get_fighting() || (ch == actor) || !CAN_START_MTRIG(ch))
			continue;

		// find memory line with command only
		for (mem = SCRIPT_MEM(ch); mem && SCRIPT_MEM(ch); mem = mem->next)
		{
			if (GET_ID(actor) != mem->id)
				continue;
			if (mem->cmd)
			{
				command_interpreter(ch, mem->cmd);	// no script
				command_performed = 1;
				break;
			}
		}

		// if a command was not performed execute the memory script
		if (mem && !command_performed)
		{
			for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
			{
				if (IS_SET(GET_TRIG_TYPE(t), MTRIG_MEMORY) &&
						CAN_SEE(ch, actor) && !GET_TRIG_DEPTH(t) && number(1, 100) <= GET_TRIG_NARG(t))
				{
					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
					script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
					break;
				}
			}
		}

		// delete the memory
		if (mem)
		{
			if (SCRIPT_MEM(ch) == mem)
			{
				SCRIPT_MEM(ch) = mem->next;
			}
			else
			{
				struct script_memory *prev;
				prev = SCRIPT_MEM(ch);
				while (prev->next != mem)
					prev = prev->next;
				prev->next = mem->next;
			}
			if (mem->cmd)
				free(mem->cmd);
			free(mem);
		}
	}
}


int greet_mtrigger(CHAR_DATA * actor, int dir)
{
	TRIG_DATA *t;
	CHAR_DATA *ch, *ch_next;
	char buf[MAX_INPUT_LENGTH];
	int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
	int intermediate, final = TRUE;

	for (ch = world[IN_ROOM(actor)]->people; ch && !ch->purged(); ch = ch_next)
	{
		ch_next = ch->next_in_room;
		if (!SCRIPT_CHECK
				(ch,
				 MTRIG_GREET | MTRIG_GREET_ALL | MTRIG_GREET_PC | MTRIG_GREET_PC_ALL) || !AWAKE(ch) || ch->get_fighting()
				|| (ch == actor) || !CAN_START_MTRIG(ch))
			continue;

		for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
		{
			if (((IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET)
					&& CAN_SEE(ch, actor))
					|| IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_ALL)
					|| (IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_PC)
						&& !IS_NPC(actor) && CAN_SEE(ch, actor))
					|| (IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_PC_ALL)
						&& !IS_NPC(actor))) && !GET_TRIG_DEPTH(t)
					&& (number(1, 100) <= GET_TRIG_NARG(t)))
			{
				if (dir >= 0)
					add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]], 0);
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				intermediate = script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
				if (ch->purged())
				{
					break;
				}
				if (!intermediate)
					final = FALSE;
				continue;
			}
		}
	}
	return final;
}


void entry_memory_mtrigger(CHAR_DATA * ch)
{
	TRIG_DATA *t;
	CHAR_DATA *actor;
	struct script_memory *mem;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_MEM(ch) || !CAN_START_MTRIG(ch))
		return;


	for (actor = world[IN_ROOM(ch)]->people; actor && SCRIPT_MEM(ch); actor = actor->next_in_room)
	{
		if (actor != ch && SCRIPT_MEM(ch))
		{
			for (mem = SCRIPT_MEM(ch); mem && SCRIPT_MEM(ch); mem = mem->next)
			{
				if (GET_ID(actor) == mem->id)
				{
					struct script_memory *prev;
					if (mem->cmd)
						command_interpreter(ch, mem->cmd);
					else
					{
						for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
						{
							if (TRIGGER_CHECK(t, MTRIG_MEMORY) &&
									(number(1, 100) <= GET_TRIG_NARG(t)))
							{
								ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
								script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
								break;
							}
						}
					}
					// delete the memory
					if (SCRIPT_MEM(ch) == mem)
					{
						SCRIPT_MEM(ch) = mem->next;
					}
					else
					{
						prev = SCRIPT_MEM(ch);
						while (prev->next != mem)
							prev = prev->next;
						prev->next = mem->next;
					}
					if (mem->cmd)
						free(mem->cmd);
					free(mem);
				}
			}	// for (mem =.....
		}
	}
}

void income_mtrigger(CHAR_DATA * ch, int dir)
{
	TRIG_DATA *t;
	int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
	int ispcinroom = 0;
	CHAR_DATA *i, *actor = NULL;

	if ((!SCRIPT_CHECK(ch, MTRIG_INCOME)
			&& !SCRIPT_CHECK(ch, MTRIG_INCOME_PC)) || !CAN_START_MTRIG(ch))
		return;

	for (i = world[IN_ROOM(ch)]->people; i; i = i->next_in_room)
		if (!IS_NPC(i) && CAN_SEE(ch, i))
		{
			ispcinroom = 1;
			actor = i;
		}

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if ((TRIGGER_CHECK(t, MTRIG_INCOME) || (TRIGGER_CHECK(t, MTRIG_INCOME_PC) && ispcinroom))
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			if (dir >= 0)
				add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]], 0);
			if (actor)
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}

	return;
}


int entry_mtrigger(CHAR_DATA * ch)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(ch, MTRIG_ENTRY) || !CAN_START_MTRIG(ch))
		return 1;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_ENTRY)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
	return 1;
}

int compare_cmd(int mode, char *source, char *dest)
{
	int result = FALSE;
	if (!source || !*source || !dest || !*dest)
		return (FALSE);
	if (*source == '*')
		return (TRUE);

	switch (mode)
	{
	case 0:
		result = word_check(dest, source);

		break;
	case 1:
		result = is_substring(dest, source);

		break;
	default:
		if (!str_cmp(source, dest))
			return (TRUE);
	}
	return (result);
}


int command_mtrigger(CHAR_DATA * actor, char *cmd, char *argument)
{
	CHAR_DATA *ch, *ch_next;
	TRIG_DATA *t;
	TRIG_DATA *dummy;
	char buf[MAX_INPUT_LENGTH];

	for (ch = world[IN_ROOM(actor)]->people; ch; ch = ch_next)
	{
		ch_next = ch->next_in_room;

		if (SCRIPT_CHECK(ch, MTRIG_COMMAND) && CAN_START_MTRIG(ch) && (actor != ch))
		{
			for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
			{
				if (t->attach_type != MOB_TRIGGER)//детачим триги не для мобов
				{
					sprintf(buf, "SYSERR: M-Trigger #%d has wrong attach_type %s expected %s char:%s[%d]!",
						GET_TRIG_VNUM(t), attach_name[(int)t->attach_type], attach_name[MOB_TRIGGER], GET_PAD(ch, 0), GET_MOB_VNUM(ch));
					mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
					sprintf(buf, "%d", GET_TRIG_VNUM(t));
					remove_trigger(SCRIPT(ch), buf, &dummy);
					if (!TRIGGERS(SCRIPT(ch)))
					{
						free_script(SCRIPT(ch));
						SCRIPT(ch) = NULL;
					}
					break;
				}

				if (!TRIGGER_CHECK(t, MTRIG_COMMAND))
					continue;

				if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
				{
					sprintf(buf,
							"SYSERR: Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
					mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
					continue;
				}
				if (compare_cmd(GET_TRIG_NARG(t), GET_TRIG_ARG(t), cmd))
				{
					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
					skip_spaces(&argument);
					add_var_cntx(&GET_TRIG_VARS(t), "arg", argument, 0);
					skip_spaces(&cmd);
					add_var_cntx(&GET_TRIG_VARS(t), "cmd", cmd, 0);

					if (script_driver(ch, t, MOB_TRIGGER, TRIG_NEW))
						return 1;
				}
			}
		}
	}

	return 0;
}


void speech_mtrigger(CHAR_DATA * actor, char *str)
{
	CHAR_DATA *ch, *ch_next;
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	for (ch = world[IN_ROOM(actor)]->people; ch; ch = ch_next)
	{
		ch_next = ch->next_in_room;

		if (SCRIPT_CHECK(ch, MTRIG_SPEECH) && AWAKE(ch) &&
				!AFF_FLAGGED(ch, AFF_DEAFNESS) && CAN_START_MTRIG(ch) && (actor != ch))
			for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
			{
				if (!TRIGGER_CHECK(t, MTRIG_SPEECH))
					continue;

				if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
				{
					sprintf(buf,
							"SYSERR: Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
					mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
					continue;
				}

				if (compare_cmd(GET_TRIG_NARG(t), GET_TRIG_ARG(t), str))
					// ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
					//  (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))
				{
					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
					add_var_cntx(&GET_TRIG_VARS(t), "speech", str, 0);
					script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
					break;
				}
			}
	}
}


void act_mtrigger(CHAR_DATA * ch, char *str, CHAR_DATA * actor, CHAR_DATA * victim,
				  const OBJ_DATA * object, const OBJ_DATA * target, char *arg)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (SCRIPT_CHECK(ch, MTRIG_ACT) && CAN_START_MTRIG(ch) && (actor != ch))
		for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
		{
			if (!TRIGGER_CHECK(t, MTRIG_ACT))
				continue;

			if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
			{
				sprintf(buf, "SYSERR: Act Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
				mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
				continue;
			}

			if (compare_cmd(GET_TRIG_NARG(t), GET_TRIG_ARG(t), str)
					/* ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
					   (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str))) */
			   )
			{
				if (actor)
					ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				if (victim)
					ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
				if (object)
					ADD_UID_OBJ_VAR(buf, t, object, "object", 0);
				if (target)
					ADD_UID_CHAR_VAR(buf, t, target, "target", 0);
				if (arg)
				{
					skip_spaces(&arg);
					add_var_cntx(&GET_TRIG_VARS(t), "arg", arg, 0);
				}
				script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
				break;
			}
		}
}


void fight_mtrigger(CHAR_DATA * ch)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}

	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_FIGHT) || !ch->get_fighting() || !CAN_START_MTRIG(ch))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_FIGHT) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, ch->get_fighting(), "actor", 0)
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

int damage_mtrigger(CHAR_DATA * damager, CHAR_DATA * victim)
{
	if (!damager || damager->purged() || !victim || victim->purged())
	{
		log("SYSERROR: damager = %s, victim = %s (%s:%d)",
				damager ? (damager->purged() ? "purged" : "true") : "false",
				victim ? (victim->purged() ? "purged" : "true") : "false",
				__FILE__, __LINE__);
		return 0;
	}

	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(victim, MTRIG_DAMAGE) || !CAN_START_MTRIG(victim))
		return 0;

	for (t = TRIGGERS(SCRIPT(victim)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_DAMAGE) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, damager, "damager", 0)
			return script_driver(victim, t, MOB_TRIGGER, TRIG_NEW);
		}
	}
	return 0;
}


void hitprcnt_mtrigger(CHAR_DATA * ch)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}

	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_HITPRCNT) || !ch->get_fighting() || !CAN_START_MTRIG(ch))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_HITPRCNT) && GET_MAX_HIT(ch) &&
				(((GET_HIT(ch) * 100) / GET_MAX_HIT(ch)) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, ch->get_fighting(), "actor", 0)
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}


int receive_mtrigger(CHAR_DATA * ch, CHAR_DATA * actor, OBJ_DATA * obj)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return 1;
	}

	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_RECEIVE) || !CAN_START_MTRIG(ch))
		return 1;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_RECEIVE) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


int death_mtrigger(CHAR_DATA * ch, CHAR_DATA * actor)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return 1;
	}

	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_DEATH) || AFF_FLAGGED(ch, AFF_CHARM))
		return 1;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_DEATH))
		{
			if (actor)
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

void load_mtrigger(CHAR_DATA * ch)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}

	TRIG_DATA *t;

	if (!SCRIPT_CHECK(ch, MTRIG_LOAD))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_LOAD) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void start_fight_mtrigger(CHAR_DATA *ch, CHAR_DATA *actor)
{
	if (!ch || ch->purged() || !actor || actor->purged())
	{
		log("SYSERROR: ch = %s, actor = %s (%s:%d)",
				ch ? (ch->purged() ? "purged" : "true") : "false",
				actor ? (actor->purged() ? "purged" : "true") : "false",
				__FILE__, __LINE__);
		return;
	}

	if (!SCRIPT_CHECK(ch, MTRIG_START_FIGHT))
	{
		return;
	}
	char buf[MAX_INPUT_LENGTH];

	for (TRIG_DATA *t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_START_FIGHT) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			return;
		}
	}
}

void round_num_mtrigger(CHAR_DATA *ch, CHAR_DATA *actor)
{
	if (!ch || ch->purged() || !actor || actor->purged())
	{
		log("SYSERROR: ch = %s, actor = %s (%s:%d)",
				ch ? (ch->purged() ? "purged" : "true") : "false",
				actor ? (actor->purged() ? "purged" : "true") : "false",
				__FILE__, __LINE__);
		return;
	}

	if (!SCRIPT_CHECK(ch, MTRIG_ROUND_NUM))
	{
		return;
	}

	char buf[MAX_INPUT_LENGTH];
	for (TRIG_DATA *t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_ROUND_NUM) && ROUND_COUNTER(ch) == GET_TRIG_NARG(t))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			return;
		}
	}
}

/**
* Реакиця на каст в моба.
* \param ch - моб
* \param actor - кастер, идет в скрипт как %actor%
* \param spellnum - номер закла, идет в скрипт как %cast_num% и %castname%
*/
void cast_mtrigger(CHAR_DATA *ch, CHAR_DATA *actor, int spellnum)
{
	if (!ch || ch->purged() || !actor || actor->purged())
	{
		log("SYSERROR: ch = %s, actor = %s (%s:%d)",
				ch ? (ch->purged() ? "purged" : "true") : "false",
				actor ? (actor->purged() ? "purged" : "true") : "false",
				__FILE__, __LINE__);
		return;
	}

	if (spellnum < 0 || spellnum >= TOP_SPELL_DEFINE)
	{
		log("SYSERROR: spellnum = %d (%s:%d)", spellnum, __FILE__, __LINE__);
		return;
	}
	if (!SCRIPT_CHECK(ch, MTRIG_CAST) || !CAN_START_MTRIG(ch))
	{
		return;
	}
	char local_buf[MAX_INPUT_LENGTH];
	for (TRIG_DATA *t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_CAST) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(local_buf, t, actor, "actor", 0);
			add_var_cntx(&GET_TRIG_VARS(t), "castnum", boost::lexical_cast<std::string>(spellnum).c_str(), 0);
			add_var_cntx(&GET_TRIG_VARS(t), "castname", spell_info[spellnum].name, 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			return;
		}
	}
}

void timechange_mtrigger(CHAR_DATA * ch, const int time)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)", ch ? "purged" : "false", __FILE__, __LINE__);
		return;
	}

	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_TIMECHANGE))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, MTRIG_TIMECHANGE) && (time == GET_TRIG_NARG(t)))
		{
			sprintf(buf, "%d", time);
			add_var_cntx(&GET_TRIG_VARS(t), "time", buf, 0);
			script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
			continue;
		}
	}
}

// *  object triggers

void random_otrigger(OBJ_DATA * obj)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(obj, OTRIG_RANDOM))
		return;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_RANDOM) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			break;
		}
	}
}


void timer_otrigger(OBJ_DATA * obj)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(obj, OTRIG_TIMER))
		return;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_TIMER))
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
	}

	return;
}


int get_otrigger(OBJ_DATA * obj, CHAR_DATA * actor)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_GET))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_GET)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


// checks for command trigger on specific object. assumes obj has cmd trig
int cmd_otrig(OBJ_DATA * obj, CHAR_DATA * actor, char *cmd, char *argument, int type)
{
	TRIG_DATA *t;
	TRIG_DATA *dummy;
	char buf[MAX_INPUT_LENGTH];

	if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND))
		for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
		{
			if (t->attach_type != OBJ_TRIGGER)//детачим триги не для объектов
			{
				sprintf(buf, "SYSERR: O-Trigger #%d has wrong attach_type %s expected %s Object:%s[%d]!",
					GET_TRIG_VNUM(t), attach_name[(int)t->attach_type], attach_name[OBJ_TRIGGER], not_null(obj->PNames[0], NULL), GET_OBJ_VNUM(obj));
				mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
				sprintf(buf, "%d", GET_TRIG_VNUM(t));
				remove_trigger(SCRIPT(obj), buf, &dummy);
				if (!TRIGGERS(SCRIPT(obj)))
				{
					free_script(SCRIPT(obj));
					SCRIPT(obj) = NULL;
				}
				break;
			}
			if (!TRIGGER_CHECK(t, OTRIG_COMMAND))
				continue;

			if (IS_SET(GET_TRIG_NARG(t), type) && (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)))
			{
				sprintf(buf, "SYSERR: O-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
				mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
				continue;
			}

			if (IS_SET(GET_TRIG_NARG(t), type) &&
					(*GET_TRIG_ARG(t) == '*' || !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(GET_TRIG_ARG(t)))))
			{
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				skip_spaces(&argument);
				add_var_cntx(&GET_TRIG_VARS(t), "arg", argument, 0);
				skip_spaces(&cmd);
				add_var_cntx(&GET_TRIG_VARS(t), "cmd", cmd, 0);

				if (script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW))
					return 1;
			}
		}

	return 0;
}


int command_otrigger(CHAR_DATA * actor, char *cmd, char *argument)
{
	OBJ_DATA *obj;
	int i;

	for (i = 0; i < NUM_WEARS; i++)
		if (cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP))
			return 1;

	for (obj = actor->carrying; obj; obj = obj->next_content)
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN))
			return 1;

	for (obj = world[IN_ROOM(actor)]->contents; obj; obj = obj->next_content)
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM))
			return 1;

	return 0;
}


int wear_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, int where)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_WEAR))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_WEAR))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			sprintf(buf, "%d", where);
			add_var_cntx(&GET_TRIG_VARS(t), "where", buf, 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


int remove_otrigger(OBJ_DATA * obj, CHAR_DATA * actor)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_REMOVE))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_REMOVE))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


int drop_otrigger(OBJ_DATA * obj, CHAR_DATA * actor)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_DROP))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_DROP)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


int give_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, CHAR_DATA * victim)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_GIVE))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_GIVE)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

void load_otrigger(OBJ_DATA * obj)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
		return;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_LOAD) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

int pick_otrigger(OBJ_DATA * obj, CHAR_DATA * actor)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_PICK))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_PICK)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}



int open_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, int unlock)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];
	int open_mode = unlock ? OTRIG_UNLOCK : OTRIG_OPEN;

	if (!SCRIPT_CHECK(obj, open_mode))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, open_mode)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			add_var_cntx(&GET_TRIG_VARS(t), "mode", unlock ? "1" : "0", 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int close_otrigger(OBJ_DATA * obj, CHAR_DATA * actor, int lock)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];
	int close_mode = lock ? OTRIG_LOCK : OTRIG_CLOSE;

	if (!SCRIPT_CHECK(obj, close_mode))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, close_mode)
				&& (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			add_var_cntx(&GET_TRIG_VARS(t), "mode", lock ? "1" : "0", 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


int greet_otrigger(CHAR_DATA * actor, int dir)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
	int intermediate, final = TRUE;

	if (IS_NPC(actor))
		return (TRUE);

	for (obj = world[IN_ROOM(actor)]->contents; obj; obj = obj->next_content)
	{
		if (!SCRIPT_CHECK(obj, OTRIG_GREET_ALL_PC))
			continue;

		for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
		{
			if (TRIGGER_CHECK(t, OTRIG_GREET_ALL_PC))
			{
				ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				if (dir >= 0)
					add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]], 0);
				intermediate = script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
				if (!intermediate)
					final = FALSE;
				continue;
			}
		}
	}
	return final;
}

int timechange_otrigger(OBJ_DATA * obj, const int time)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(obj, OTRIG_TIMECHANGE))
		return 1;

	for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, OTRIG_TIMECHANGE)
				&& (time == GET_TRIG_NARG(t)))
		{
			sprintf(buf, "%d", time);
			add_var_cntx(&GET_TRIG_VARS(t), "time", buf, 0);
			return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}


// *  world triggers

void reset_wtrigger(ROOM_DATA * room)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(room, WTRIG_RESET))
		return;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, WTRIG_RESET) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

void random_wtrigger(ROOM_DATA * room, int num, void *s, int types, void *list, void *next)
{
	TRIG_DATA *t;

	if (!SCRIPT_CHECK(room, WTRIG_RANDOM))
		return;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, WTRIG_RANDOM) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;
		}
	}
}


int enter_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];
	int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };

	if (!SCRIPT_CHECK(room, WTRIG_ENTER | WTRIG_ENTER_PC))
		return 1;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if ((TRIGGER_CHECK(t, WTRIG_ENTER) ||
				(TRIGGER_CHECK(t, WTRIG_ENTER_PC) && !IS_NPC(actor))) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			if (dir >= 0)
				add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			// триггер может удалить выход, но не вернуть 0 (есть такие билдеры)
			return (script_driver(room, t, WLD_TRIGGER, TRIG_NEW) && CAN_GO(actor, dir));
		}
	}

	return 1;
}


int command_wtrigger(CHAR_DATA * actor, char *cmd, char *argument)
{
	ROOM_DATA *room;
	TRIG_DATA *dummy;
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!actor || IN_ROOM(actor) == NOWHERE || !SCRIPT_CHECK(world[IN_ROOM(actor)], WTRIG_COMMAND))
		return 0;

	room = world[IN_ROOM(actor)];
	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (t->attach_type != WLD_TRIGGER)//детачим триги не для комнат
		{
			sprintf(buf, "SYSERR: W-Trigger #%d has wrong attach_type %s expected %s room:%s[%d]!",
				GET_TRIG_VNUM(t), attach_name[(int)t->attach_type], attach_name[WLD_TRIGGER], room->name, room->number);
			mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			sprintf(buf, "%d", GET_TRIG_VNUM(t));
			remove_trigger(SCRIPT(room), buf, &dummy);
			if (!TRIGGERS(SCRIPT(room)))
			{
				free_script(SCRIPT(room));
				SCRIPT(room) = NULL;
			}
			break;
		}
				
		if (!TRIGGER_CHECK(t, WTRIG_COMMAND))
			continue;

		if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
		{
			sprintf(buf, "SYSERR: W-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
			mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			continue;
		}

		if (compare_cmd(GET_TRIG_NARG(t), GET_TRIG_ARG(t), cmd)
				/* *GET_TRIG_ARG(t)=='*' ||
				   !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(GET_TRIG_ARG(t))) */
		   )
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			skip_spaces(&argument);
			add_var_cntx(&GET_TRIG_VARS(t), "arg", argument, 0);
			skip_spaces(&cmd);
			add_var_cntx(&GET_TRIG_VARS(t), "cmd", cmd, 0);

			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 0;
}


void speech_wtrigger(CHAR_DATA * actor, char *str)
{
	ROOM_DATA *room;
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!actor || !SCRIPT_CHECK(world[IN_ROOM(actor)], WTRIG_SPEECH))
		return;

	room = world[IN_ROOM(actor)];
	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (!TRIGGER_CHECK(t, WTRIG_SPEECH))
			continue;

		if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
		{
			sprintf(buf, "SYSERR: W-Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
			mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			continue;
		}

		if (compare_cmd(GET_TRIG_NARG(t), GET_TRIG_ARG(t), str)
				/* ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
				   (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str))) */
		   )
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			add_var_cntx(&GET_TRIG_VARS(t), "speech", str, 0);
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;
		}
	}
}

int drop_wtrigger(OBJ_DATA * obj, CHAR_DATA * actor)
{
	ROOM_DATA *room;
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!actor || !SCRIPT_CHECK(world[IN_ROOM(actor)], WTRIG_DROP))
		return 1;

	room = world[IN_ROOM(actor)];
	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
		if (TRIGGER_CHECK(t, WTRIG_DROP) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);
			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			break;
		}

	return 1;
}

int pick_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(room, WTRIG_PICK))
		return 1;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, WTRIG_PICK) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[dir], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}



int open_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir, int unlock)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];
	int open_mode = unlock ? WTRIG_UNLOCK : WTRIG_OPEN;

	if (!SCRIPT_CHECK(room, open_mode))
		return 1;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, open_mode) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			add_var_cntx(&GET_TRIG_VARS(t), "mode", unlock ? "1" : "0", 0);
			add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[dir], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}

int close_wtrigger(ROOM_DATA * room, CHAR_DATA * actor, int dir, int lock)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];
	int close_mode = lock ? WTRIG_LOCK : WTRIG_CLOSE;

	if (!SCRIPT_CHECK(room, close_mode))
		return 1;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, close_mode) && (number(1, 100) <= GET_TRIG_NARG(t)))
		{
			add_var_cntx(&GET_TRIG_VARS(t), "mode", lock ? "1" : "0", 0);
			add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[dir], 0);
			ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
			return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
		}
	}

	return 1;
}
int timechange_wtrigger(ROOM_DATA * room, const int time)
{
	TRIG_DATA *t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(room, WTRIG_TIMECHANGE))
		return 1;

	for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
	{
		if (TRIGGER_CHECK(t, WTRIG_TIMECHANGE) && (time == GET_TRIG_NARG(t)))
		{
			sprintf(buf, "%d", time);
			add_var_cntx(&GET_TRIG_VARS(t), "time", buf, 0);
			script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
			continue;
		}
	}

	return 1;
}
