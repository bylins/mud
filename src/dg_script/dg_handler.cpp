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
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic.utils.hpp"
#include "spells.h"
#include "dg_event.h"
#include "im.h"
#include "features.hpp"
#include "chars/character.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

// remove a single trigger from a mob/obj/room
void extract_trigger(TRIG_DATA *trig) {
  if (GET_TRIG_WAIT(trig)) {
    // см. объяснения в вызове trig_data_free()
    free(GET_TRIG_WAIT(trig)->info);
    remove_event(GET_TRIG_WAIT(trig));
    GET_TRIG_WAIT(trig) = NULL;
  }

  trig_index[trig->get_rnum()]->number--;

  // walk the trigger list and remove this one
  trigger_list.remove(trig);

  trig->clear_var_list();

  delete trig;
}

// remove all triggers from a mob/obj/room
void extract_script(SCRIPT_DATA *sc) {
  sc->trig_list.clear();
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
const char *skill_percent(TRIG_DATA *trig, CHAR_DATA *ch, char *skill) {
  static char retval[256];
  im_rskill *rs;
  int rid;

  const ESkill skillnum = fix_name_and_find_skill_num(skill);
  if (skillnum > 0) {
    sprintf(retval, "%d", ch->get_trained_skill(skillnum));
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
  if ((skillnum == 0) && (rid < 0)) {
    sprintf(buf2, "Wrong skill\recipe name: %s", skill);
    trig_log(trig, buf2);
  }
  return ("0");
}

bool feat_owner(TRIG_DATA *trig, CHAR_DATA *ch, char *feat) {
  int featnum;

  featnum = find_feat_num(feat);
  if (featnum > 0) {
    if (HAVE_FEAT(ch, featnum))
      return 1;
  } else {
    sprintf(buf2, "Wrong feat name: %s", feat);
    trig_log(trig, buf2);
  }
  return 0;
}

const char *spell_count(TRIG_DATA *trig, CHAR_DATA *ch, char *spell) {
  static char retval[256];
  int spellnum;

  spellnum = fix_name_and_find_spell_num(spell);
  if (spellnum <= 0) {
    sprintf(buf2, "Wrong spell name: %s", spell);
    trig_log(trig, buf2);
    return ("0");
  }

  if (GET_SPELL_MEM(ch, spellnum))
    sprintf(retval, "%d", GET_SPELL_MEM(ch, spellnum));
  else
    strcpy(retval, "0");
  return retval;
}

const char *spell_knowledge(TRIG_DATA *trig, CHAR_DATA *ch, char *spell) {
  static char retval[256];
  int spellnum;

  spellnum = fix_name_and_find_spell_num(spell);
  if (spellnum <= 0) {
    sprintf(buf2, "Wrong spell name: %s", spell);
    trig_log(trig, buf2);
    return ("0");
  }

  if (GET_SPELL_TYPE(ch, spellnum))
    sprintf(retval, "%d", GET_SPELL_TYPE(ch, spellnum));
  else
    strcpy(retval, "0");
  return retval;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
