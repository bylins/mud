/**
 \file affect_actions.h - a part of the Bylins engine.
 \brief Public entry for running an affect's own actions through the cast pipeline. For the affect
        modules (affects/), which must not include magic_internal.h.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MAGIC_AFFECT_ACTIONS_H_
#define BYLINS_SRC_GAMEPLAY_MAGIC_AFFECT_ACTIONS_H_

#include "gameplay/magic/magic.h"
#include "gameplay/abilities/talents_actions.h"

#include <string>
#include <vector>

class CharData;
class RoomData;

// Run an affect's own (trigger-filtered) actions on the room each tick, cycled
// by phase. Context from ctx_spell; tick_duration (when non-null) carries the affect's current duration
// in/out so a manual_cast handler can branch on / modify it. Defined in affect_triggers.cpp.
ECastResult CastRoomTickActionFromActions(CharData *ch, RoomData *room, ESpell ctx_spell,
										  const std::vector<talents_actions::Action> &actions, int phase,
										  int *tick_duration = nullptr, float fixed_potency = -1.0f,
										  const std::string &aff_dmg_char = "", const std::string &aff_dmg_vict = "",
										  const std::string &aff_dmg_room = "", long damage_author = 0);

#endif  // BYLINS_SRC_GAMEPLAY_MAGIC_AFFECT_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
