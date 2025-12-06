/* ************************************************************************
*  File: dg_handler.cpp                                    Part of Bylins *
*                                                                         *
*  Usage: This file contains some triggers handle functions               *
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
#include "engine/core/handler.h"
#include "gameplay/magic/magic_utils.h"
#include "dg_event.h"
#include "engine/db/global_objects.h"

// remove a single trigger from a mob/obj/room и остановить его выполнение
void ExtractTrigger(Trigger *trig) {
	if (trig->wait_event.time_remaining > 0) {
		auto for_delete = trig->wait_event.info;
		remove_event(trig->wait_event);
		free(for_delete);
		trig->wait_event.time_remaining = 0;
	}
	trig_index[trig->get_rnum()]->total_online--;

	// walk the trigger list and remove this one
	trigger_list.remove(trig);

	trig->clear_var_list();

	delete trig;
}

// remove all triggers from a mob/obj/room
void extract_script(Script *sc) {
	sc->script_trig_list.clear();
}

// erase the script memory of a mob
void extract_script_mem(struct script_memory *sc) {
	struct script_memory *next;
	while (sc) {
		next = sc->next;
		if (sc->cmd)
			free(sc->cmd);
		free(sc);
		sc = next;
	}
}

// perhaps not the best place for this, but I didn't want a new file
const char *skill_percent(Trigger *trig, CharData *ch, char *skill) {
	static char retval[256];
	im_rskill *rs;
	int rid;

	const ESkill skill_id = FixNameAndFindSkillId(skill);
	if (MUD::Skills().IsValid(skill_id)) {
		sprintf(retval, "%d", ch->GetTrainedSkill(skill_id));
		return retval;
	}
	rid = im_get_recipe_by_name(skill);
	if (rid >= 0) {
		rs = im_get_char_rskill(ch, rid);
		if (!rs)
			return "0";
		sprintf(retval, "%d", rs->perc);
		return retval;
	}
	if ((skill_id == ESkill::kUndefined) && (rid < 0)) {
		sprintf(buf2, "Wrong skill\recipe name: %s", skill);
		trig_log(trig, buf2);
	}
	return ("0");
}

bool feat_owner(Trigger *trig, CharData *ch, char *feat) {
	auto featnum = FindFeatId(feat);
	if (featnum != EFeat::kUndefined) {
		if (ch->HaveFeat(featnum)) {
			return true;
		}
	} else {
		sprintf(buf2, "Wrong feat name: %s", feat);
		trig_log(trig, buf2);
	}
	return false;
}

const char *spell_count(Trigger *trig, CharData *ch, char *spell) {
	static char retval[256];

	auto spell_id = FixNameAndFindSpellId(spell);
	if (spell_id == ESpell::kUndefined) {
		sprintf(buf2, "Wrong spell name: %s", spell);
		trig_log(trig, buf2);
		return ("0");
	}

	if (GET_SPELL_MEM(ch, spell_id)) {
		sprintf(retval, "%d", GET_SPELL_MEM(ch, spell_id));
	} else {
		strcpy(retval, "0");
	}
	return retval;
}

const char *spell_knowledge(Trigger *trig, CharData *ch, char *spell) {
	static char retval[256];

	auto spell_id = FixNameAndFindSpellId(spell);
	if (spell_id == ESpell::kUndefined) {
		sprintf(buf2, "Wrong spell name: %s", spell);
		trig_log(trig, buf2);
		return ("0");
	}

	if (GET_SPELL_TYPE(ch, spell_id))
		sprintf(retval, "%d", GET_SPELL_TYPE(ch, spell_id));
	else
		strcpy(retval, "0");
	return retval;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
