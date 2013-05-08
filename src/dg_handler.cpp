/* ************************************************************************
*  File: dg_handler.cpp                                    Part of Bylins *
*                                                                         *
*  Usage: This file contains some trigers handle functions                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                   *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "dg_event.h"
#include "im.h"
#include "features.hpp"
#include "char.hpp"

extern INDEX_DATA **trig_index;
extern TRIG_DATA *trigger_list;

void trig_data_free(TRIG_DATA * this_data);

// return memory used by a trigger 
void free_trigger(TRIG_DATA * trig)
{
	// threw this in for minor consistance in names with the rest of circle 
	trig_data_free(trig);
}


// remove a single trigger from a mob/obj/room 
void extract_trigger(TRIG_DATA * trig)
{
	TRIG_DATA *temp;

	if (GET_TRIG_WAIT(trig))
	{
		// см. объяснения в вызове trig_data_free()
		free(GET_TRIG_WAIT(trig)->info);
		remove_event(GET_TRIG_WAIT(trig));
		GET_TRIG_WAIT(trig) = NULL;
	}

	trig_index[trig->nr]->number--;

	// walk the trigger list and remove this one 
	REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

	free_trigger(trig);
}

// remove all triggers from a mob/obj/room 
void extract_script(SCRIPT_DATA * sc)
{
	TRIG_DATA *trig, *next_trig;

	for (trig = TRIGGERS(sc); trig; trig = next_trig)
	{
		next_trig = trig->next;
		extract_trigger(trig);
	}
	TRIGGERS(sc) = NULL;
}

// erase the script memory of a mob 
void extract_script_mem(struct script_memory *sc)
{
	struct script_memory *next;
	while (sc)
	{
		next = sc->next;
		if (sc->cmd)
			free(sc->cmd);
		free(sc);
		sc = next;
	}
}

// perhaps not the best place for this, but I didn't want a new file 
const char * skill_percent(CHAR_DATA * ch, char *skill)
{
	static char retval[256];
	im_rskill *rs;
	int skillnum, rid;

	skillnum = find_skill_num(skill);
	if (skillnum > 0)
	{
		//edited by WorM 2011.05.23 триги должны возвращать реальный скилл без бонусов от стафа
		sprintf(retval, "%d", ch->get_trained_skill(skillnum));
		return retval;
	}

	rid = im_get_recipe_by_name(skill);
	if (rid >= 0)
	{
		rs = im_get_char_rskill(ch, rid);
		if (!rs)
			return "0";
		sprintf(retval, "%d", rs->perc);
		return retval;
	}
	return ("-1");
}

bool feat_owner(CHAR_DATA * ch, char *feat)
{
	int featnum;

	featnum = find_feat_num(feat);
	if (featnum > 0)
		if (HAVE_FEAT(ch, featnum))
			return 1;
	return 0;
}

const char * spell_count(CHAR_DATA * ch, char *spell)
{
	static char retval[256];
	int spellnum;

	spellnum = find_spell_num(spell);
	if (spellnum <= 0)
		return ("-1");

	if (GET_SPELL_MEM(ch, spellnum))
		sprintf(retval, "%d", GET_SPELL_MEM(ch, spellnum));
	else
		strcpy(retval, "");
	return retval;
}

const char * spell_knowledge(CHAR_DATA * ch, char *spell)
{
	static char retval[256];
	int spellnum;

	spellnum = find_spell_num(spell);
	if (spellnum <= 0)
		return ("-1");

	if (GET_SPELL_TYPE(ch, spellnum))
		sprintf(retval, "%d", GET_SPELL_TYPE(ch, spellnum));
	else
		strcpy(retval, "");
	return retval;
}
