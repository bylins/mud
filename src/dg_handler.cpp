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

#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spell_parser.hpp"
#include "spells.h"
#include "dg_event.h"
#include "im.h"
#include "features.hpp"
#include "char.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

extern INDEX_DATA **trig_index;

void trig_data_free(TRIG_DATA * this_data);

// return memory used by a trigger 
void free_trigger(TRIG_DATA* trig)
{
	// threw this in for minor consistance in names with the rest of circle 
	trig_data_free(trig);
}

// remove a single trigger from a mob/obj/room 
void extract_trigger(TRIG_DATA * trig)
{
	if (GET_TRIG_WAIT(trig))
	{
		// см. объяснения в вызове trig_data_free()
		free(GET_TRIG_WAIT(trig)->info);
		remove_event(GET_TRIG_WAIT(trig));
		GET_TRIG_WAIT(trig) = NULL;
	}

	trig_index[trig->get_rnum()]->number--;

	// walk the trigger list and remove this one 
	trigger_list.remove(trig);

	free_trigger(trig);
}

// remove all triggers from a mob/obj/room 
void extract_script(SCRIPT_DATA * sc)
{
	TRIG_DATA* next_trig;

	for (auto trig = TRIGGERS(sc); trig; trig = next_trig)
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
    int rid;

	const ESkill skillnum = find_skill_num(skill);
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
		return ("");

	if (GET_SPELL_MEM(ch, spellnum))
		sprintf(retval, "%d", GET_SPELL_MEM(ch, spellnum));
	else
		strcpy(retval, "0");
	return retval;
}

const char * spell_knowledge(CHAR_DATA * ch, char *spell)
{
	static char retval[256];
	int spellnum;

	spellnum = find_spell_num(spell);
	if (spellnum <= 0)
	{
		return ("");
	}

	if (GET_SPELL_TYPE(ch, spellnum))
		sprintf(retval, "%d", GET_SPELL_TYPE(ch, spellnum));
	else
		strcpy(retval, "");
	return retval;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
