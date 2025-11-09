// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef POISON_HPP_INCLUDED
#define POISON_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/fight/fight_hit.h"

void ProcessToxicMob(CharData *ch, CharData *victim, HitData &hit_data);
void ProcessPoisonedWeapom(CharData *ch, CharData *victim, HitData &hit_data);

bool poison_in_vessel(int liquid_num);
void set_weap_poison(ObjData *weapon, int liquid_num);
void TryDrinkPoison(CharData *ch, ObjData *jar, int amount);

std::string GetPoisonName(ESpell spell_id);
bool IsSpellPoison(ESpell spell_id);

int ProcessPoisonDmg(CharData *ch, const Affect<EApply>::shared_ptr &af);

#endif // POISON_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
