/**
 \file portal.h - a part of the Bylins engine.
 \brief issue.spellhandlers: shared portal-effect mechanics used by SpellPortal and townportal.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_PORTAL_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_PORTAL_H_

#include "engine/structs/structs.h"      // RoomRnum
#include "gameplay/magic/magic_rooms.h"  // room_spells::ERoomAffect

class CharData;
struct RoomData;

// Impose a kPortalTimer room affect on `from_room` pointing at `to_room` (the "pentagram gate"
// endpoint). pk_unique stamps the imposing caster for PK; affect_type=kNoPortalExit marks a
// one-way destination (townportal). Default = a plain two-way endpoint (kPortalTimer).
void AddPortalTimer(CharData *ch, RoomData *from_room, RoomRnum to_room, int time, long pk_unique = 0,
					room_spells::ERoomAffect affect_type = room_spells::ERoomAffect::kPortalTimer);

// Break the pentagram gate rooted at `rnum` and its paired endpoint, narrating to both rooms.
void RemovePortalGate(RoomRnum rnum);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_PORTAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
