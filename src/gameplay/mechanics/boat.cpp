/**
\file boat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "boat.h"

#include "engine/entities/entities_constants.h"
#include "gameplay/affects/affect_contants.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/ui/cmd/do_equip.h"

bool IsBoatNeedHere(RoomRnum room_rnum);
bool IsCharIgnoresDeepWater(CharData *ch);

/**
 * Return true if char can walk on water
 */
bool HasBoat(CharData *ch) {
	if (IS_IMMORTAL(ch)) {
		return true;
	}

	for (auto obj = ch->carrying; obj; obj = obj->get_next_content()) {
		if (obj->get_type() == EObjType::kBoat && (find_eq_pos(ch, obj, nullptr) < 0)) {
			return true;
		}
	}

	for (auto i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i) && GET_EQ(ch, i)->get_type() == EObjType::kBoat) {
			return true;
		}
	}

	return false;
}

bool IsBoatNeedHere(RoomRnum room_rnum) {
	return (real_sector(room_rnum) == ESector::kWaterNoswim);
}

bool IsCharNeedBoatThere(CharData *ch, RoomRnum room_rnum) {
	return (!(IsCharIgnoresDeepWater(ch)) && IsBoatNeedHere(room_rnum));
}

bool IsCharIgnoresDeepWater(CharData *ch) {
	return (IS_GOD(ch) || ch->IsFlagged(EMobFlag::kSwimming) || AFF_FLAGGED(ch, EAffect::kWaterWalk) ||
		ch->IsFlagged(EMobFlag::kFlying) ||	AFF_FLAGGED(ch, EAffect::kFly));
}

bool IsCharCanDrownThere(CharData *ch, RoomRnum room_rnum) {
	return (!ch->IsNpc() && IsCharNeedBoatThere(ch, room_rnum) && !HasBoat(ch));
}

bool IsCharNeedBoatToMove(CharData *ch, int dir) {
	return ((IsBoatNeedHere(ch->in_room) || IsBoatNeedHere(EXIT(ch, dir)->to_room())) &&
		!(HasBoat(ch) || IsCharIgnoresDeepWater(ch)));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
