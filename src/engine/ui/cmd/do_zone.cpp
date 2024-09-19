/**
\file do_zone.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/mapsystem.h"
#include "gameplay/mechanics/sight.h"
#include "engine/entities/zone.h"
#include "gameplay/mechanics/illumination.h"

void DoZone(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->desc
		&& !(is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !CanUseFeat(ch, EFeat::kDarkReading))
		&& !AFF_FLAGGED(ch, EAffect::kBlind)) {
		MapSystem::print_map(ch);
	}

	print_zone_info(ch);
	if (zone_table[world[ch->in_room]->zone_rn].copy_from_zone > 0) {
		SendMsgToChar(ch, "Зазеркальный мир.\r\n");
	}
	if ((IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo))
		&& !zone_table[world[ch->in_room]->zone_rn].comment.empty()) {
		SendMsgToChar(ch, "Комментарий: %s.\r\n", zone_table[world[ch->in_room]->zone_rn].comment.c_str());
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
