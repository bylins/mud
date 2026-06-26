/**
 \file portal.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: shared portal-effect mechanics (extracted from spells.cpp).
*/

#include "gameplay/mechanics/portal.h"

#include "gameplay/magic/magic_rooms.h"
#include "engine/entities/char_data.h"
#include "engine/entities/room_data.h"
#include "engine/core/comm.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

void AddPortalTimer(CharData *ch, RoomData *from_room, RoomRnum to_room, int time, long pk_unique,
					room_spells::ERoomAffect affect_type) {

	Affect<room_spells::ERoomApply> af;
	af.affect_type = affect_type;
	af.duration = time; //раз в 2 секунды
	af.modifier = to_room;
	af.battleflag = 0;
	af.location = room_spells::ERoomApply::kNone;
	af.caster_id = ch? ch->get_uid() : 0;
	af.apply_time = 0;
	af.pk_unique = pk_unique;
	room_spells::affect_to_room(from_room, af);
	room_spells::AddRoomToAffected(from_room);
}

void RemovePortalGate(RoomRnum rnum) {
	auto aff = room_spells::FindPortalAffect(world[rnum]);
	const RoomRnum to_room = (*aff)->modifier;
	// kPortal sheaf kCustomMsgThree holds "Пентаграмма была
	// разрушена." -- emitted to both char and room of each affected portal endpoint.
	const auto &broken_msg = MUD::SpellMessages().GetMessage(
			ESpell::kPortal, ESpellMsg::kCustomMsgThree);

	if (aff != world[rnum]->affected.end()) {
		room_spells::RoomRemoveAffect(world[rnum], aff);
		act(broken_msg.c_str(), false, world[rnum]->first_character(), 0, 0, kToRoom);
		act(broken_msg.c_str(), false, world[rnum]->first_character(), 0, 0, kToChar);
	}
	aff = room_spells::FindPortalAffect(world[to_room]);
	if (aff != world[to_room]->affected.end()) {
		room_spells::RoomRemoveAffect(world[to_room], aff);
		act(broken_msg.c_str(), false, world[to_room]->first_character(), 0, 0, kToRoom);
		act(broken_msg.c_str(), false, world[to_room]->first_character(), 0, 0, kToChar);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
