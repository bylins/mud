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
void TryPoisonWithWeapom(CharData *ch, CharData *vict, ESpell spell_id);

bool poison_in_vessel(int liquid_num);
void set_weap_poison(ObjData *weapon, int liquid_num);

std::string GetPoisonName(ESpell spell_id);
bool IsSpellPoison(ESpell spell_id);

int ProcessPoisonDmg(CharData *ch, const Affect<EApply>::shared_ptr &af);

#endif // POISON_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
