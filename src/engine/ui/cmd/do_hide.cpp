/**
\file do_hide.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/skills/skills.h"
#include "engine/db/global_objects.h"


void do_hide(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int prob, percent;

	if (ch->IsNpc() || !ch->GetSkill(ESkill::kHide)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (ch->IsOnHorse()) {
		act("А куда вы хотите спрятать $N3?", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}

	RemoveAffectFromChar(ch, ESpell::kHide);

	if (IsAffectedBySpell(ch, ESpell::kHide)) {
		SendMsgToChar("Вы уже пытаетесь спрятаться.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы и не видите куда прятаться.\r\n", ch);
		return;
	}

	if (IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		SendMsgToChar("Спрятаться?! Да вы сверкаете как корчма во время гулянки!.\r\n", ch);
		return;
	}

	SendMsgToChar("Хорошо, вы попытаетесь спрятаться.\r\n", ch);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	percent = number(1, MUD::Skill(ESkill::kHide).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kHide, nullptr);

	Affect<EApply> af;
	af.type = ESpell::kHide;
	af.duration = CalcDuration(ch, 0, GetRealLevel(ch), 8, 0, 1);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = 0;

	if (percent > prob) {
		af.bitvector = 0;
	} else {
		af.bitvector = to_underlying(EAffect::kHide);
	}

	affect_to_char(ch, af);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
