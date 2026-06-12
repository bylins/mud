/**
\file experience.cpp - a part of the Bylins engine.
\brief issue.chardata-cleaning: experience-gain rules (see experience.h).
*/

#include "gameplay/core/experience.h"

#include "engine/entities/char_data.h"
#include "engine/db/db.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/mount.h"

#include <algorithm>

namespace experience {

// was CharData::get_zone_group -- the group size is a property of the mob's zone.
int GetZoneGroup(const CharData *ch) {
	const auto rnum = ch->get_rnum();
	if (ch->IsNpc()
		&& rnum >= 0
		&& mob_index[rnum].zone >= 0) {
		const auto zone = GetZoneRnum(GET_MOB_VNUM(ch) / 100);
		return std::max(1, zone_table[zone].group);
	}

	return 1;
}

// was the free OK_GAIN_EXP that lived in char_data.cpp.
bool OkGainExp(const CharData *ch, const CharData *victim) {
	return !NAME_BAD(ch)
		&& (NAME_FINE(ch)
			|| !(GetRealLevel(ch) == kNameLevel))
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		&& victim->IsNpc()
		&& (victim->get_exp() > 0)
		&& (!victim->IsNpc()
			|| !ch->IsNpc()
			|| AFF_FLAGGED(ch, EAffect::kCharmed))
		&& !mount::IsHorse(victim)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena);
}

}  // namespace experience

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
