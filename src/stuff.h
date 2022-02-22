/* ************************************************************************
*   File: stuff.hpp                                     Part of Bylins    *
*  Usage: header file: stuff load table handling                          *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _STUFF_HPP_
#define _STUFF_HPP_

#include "structs/structs_double_map.h"
#include "structs/structs.h"
#include "conf.h"
#include "game_skills/skills.h"

#include <vector>
#include <fstream>
#include <sstream>

const int MAX_LOAD_PROB = 1000;

struct obj_load_info {
	int obj_qty;
	int load_prob;
	obj_load_info() : obj_qty(0), load_prob(0) {}
	obj_load_info(int __i, int __j) : obj_qty(__i), load_prob(__j) {}
};

typedef double_map<ObjVnum, MobVnum, obj_load_info> oload_map;

class oload_class : public oload_map {
 public:
	void init();
};

extern oload_class oload_table;

void obj_load_on_death(ObjData *corpse, CharData *ch);
void create_charmice_stuff(CharData *ch, ESkill skill_id, int diff);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
