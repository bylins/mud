#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "skill_messages.h"
#include "engine/db/global_objects.h"
#include "engine/core/char_movement.h"

void DoCampfire(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int percent, prob;
	if (!GetSkill(ch, ESkill::kCampfire)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCampfire, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kCampfire, ESkillMsg::kCantWhileMounted) + "\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы ничего не видите!\r\n", ch);
		return;
	}

	if (world[ch->in_room]->fires) {
		SendMsgToChar("Здесь уже горит огонь.\r\n", ch);
		return;
	}

	if (SECT(ch->in_room) == ESector::kInside ||
		SECT(ch->in_room) == ESector::kCity ||
		SECT(ch->in_room) == ESector::kWaterSwim ||
		SECT(ch->in_room) == ESector::kWaterNoswim ||
		SECT(ch->in_room) == ESector::kOnlyFlying ||
		SECT(ch->in_room) == ESector::kUnderwater || SECT(ch->in_room) == ESector::kSecret) {
		SendMsgToChar("В этой комнате нельзя разжечь костер.\r\n", ch);
		return;
	}

	if (!check_moves(ch, kFireMoves))
		return;

	percent = number(1, MUD::Skills()[ESkill::kCampfire].difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kCampfire, nullptr);
	if (percent > prob) {
		SendMsgToChar("Вы попытались разжечь костер, но у вас ничего не вышло.\r\n", ch);
		return;
	} else {
		world[ch->in_room]->fires = std::max(0, (prob - percent) / 5) + 1;
		SendMsgToChar("Вы набрали хворосту и разожгли огонь.\n\r", ch);
		act("$n развел$g огонь.",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		ImproveSkill(ch, ESkill::kCampfire, true, nullptr);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
