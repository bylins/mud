/**
\file do_hidemove.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/char_movement.h"
#include "engine/db/global_objects.h"

void DoHidemove(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!GetSkill(ch, ESkill::kSneak)) {
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
	if (mount::IsOnHorse(ch)) {
		act("Вам мешает $N.", false, ch, nullptr, mount::GetHorse(ch), kToChar);
		return;
	}
	auto sneaking = IsAffectedFlagOnly(ch, EAffect::kSneak);
	if (!sneaking) {
		Affect<EApply> af;
		af.location = EApply::kNone;
		af.modifier = 0;
		af.duration = 1;
		const int calculated_skill = CalcCurrentSkill(ch, ESkill::kSneak, nullptr);
		const int chance = number(1, MUD::Skill(ESkill::kSneak).difficulty);
		af.affect_type = EAffect::kSneak;
		af.battleflag = (chance < calculated_skill) ? 0 : static_cast<Bitvector>(kAfFailed);
		ImposeAffect(ch, af, false, false, false, false);
	}
	PerformMove(ch, dir, 0, true, nullptr);
	if (!sneaking || IsAffected(ch, EAffect::kGlitterDust)) {
		RemoveAffectFromChar(ch, EAffect::kSneak);
		AFF_FLAGS(ch).unset(EAffect::kSneak);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
