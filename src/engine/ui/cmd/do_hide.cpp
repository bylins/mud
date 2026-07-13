/**
\file do_hide.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/skills/skills.h"
#include "engine/db/global_objects.h"


void do_hide(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int prob, percent;

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kHide)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		act("А куда вы хотите спрятать $N3?", false, ch, nullptr, mount::GetHorse(ch), kToChar);
		return;
	}

	if (IsAffectedOrAttempting(ch, EAffect::kHide)) {
		SendMsgToChar("Вы уже пытаетесь спрятаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы и не видите куда прятаться.\r\n", ch);
		return;
	}

	if (IsAffected(ch, EAffect::kGlitterDust)) {
		SendMsgToChar("Спрятаться?! Да вы сверкаете как корчма во время гулянки!.\r\n", ch);
		return;
	}

	SendMsgToChar("Хорошо, вы попытаетесь спрятаться.\r\n", ch);
	ch->Temporary.unset(ECharExtraFlag::kFailHide);
	percent = number(1, MUD::Skill(ESkill::kHide).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kHide, nullptr);

	Affect<EApply> af;
	af.duration = CalcDuration(ch, ch, ESkill::kHide, 0, 20, 0, 1);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;

	if (percent > prob) {
		af.affect_type = EAffect::kHide;
		af.battleflag = kAfFailed;
	} else {
		af.affect_type = EAffect::kHide;
	}

	affect_to_char(ch, af);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
