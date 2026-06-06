/**
\file do_hidemove.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/char_movement.h"
#include "engine/db/global_objects.h"

void DoHidemove(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kSneak)) {
		SendMsgToChar("Вы не умеете этого.\r\n", ch);
		return;
	}

	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar("И куда это вы направляетесь?\r\n", ch);
		return;
	}

	int dir;
	if ((dir = search_block(argument, dirs, false)) < 0 && (dir = search_block(argument, dirs_rus, false)) < 0) {
		SendMsgToChar("Неизвестное направление.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		act("Вам мешает $N.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	auto sneaking = IsAffectedBySpell(ch, ESpell::kSneak);
	if (!sneaking) {
		Affect<EApply> af;
		af.type = ESpell::kSneak;
		af.location = EApply::kNone;
		af.modifier = 0;
		af.duration = 1;
		const int calculated_skill = CalcCurrentSkill(ch, ESkill::kSneak, nullptr);
		const int chance = number(1, MUD::Skill(ESkill::kSneak).difficulty);
		af.bitvector = (chance < calculated_skill) ? to_underlying(EAffect::kSneak) : 0;
		af.battleflag = 0;
		ImposeAffect(ch, af, false, false, false, false);
	}
	PerformMove(ch, dir, 0, true, nullptr);
	if (!sneaking || IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		RemoveAffectFromChar(ch, ESpell::kSneak);
		AFF_FLAGS(ch).unset(EAffect::kSneak);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
