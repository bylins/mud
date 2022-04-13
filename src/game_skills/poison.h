// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef POISON_HPP_INCLUDED
#define POISON_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "game_affects/affect_data.h"

void poison_victim(CharData *ch, CharData *vict, int modifier);
void try_weap_poison(CharData *ch, CharData *vict, int spell_num);

bool poison_in_vessel(int liquid_num);
void set_weap_poison(ObjData *weapon, int liquid_num);

std::string get_poison_by_spell(int spell);
bool check_poison(int spell);

int processPoisonDamage(CharData *ch, const Affect<EApply>::shared_ptr &af);

#endif // POISON_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
