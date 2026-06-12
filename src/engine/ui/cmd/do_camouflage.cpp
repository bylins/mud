/**
\file do_camouflage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/handler.h"
#include "gameplay/skills/skills.h"
#include "engine/db/global_objects.h"

void do_camouflage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedSkill timed;
	int prob, percent;

	if (ch->IsNpc() || !skills::GetSkill(ch, ESkill::kDisguise)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Вы замаскировались под статую Юрия Долгорукого.\r\n", ch);
		return;
	}

	if (IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		SendMsgToChar("Вы замаскировались под золотую рыбку.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kDisguise)) {
		SendMsgToChar("У вас пока не хватает фантазии. Побудьте немного самим собой.\r\n", ch);
		return;
	}

	if (ch->IsImmortal()) {
		RemoveAffectFromChar(ch, ESpell::kCamouflage);
	}

	if (IsAffectedBySpell(ch, ESpell::kCamouflage)) {
		SendMsgToChar("Вы уже маскируетесь.\r\n", ch);
		return;
	}

	SendMsgToChar("Вы начали усиленно маскироваться.\r\n", ch);
	ch->Temporary.unset(EXTRA_FAILCAMOUFLAGE);
	percent = number(1, MUD::Skill(ESkill::kDisguise).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kDisguise, nullptr);

	Affect<EApply> af;
	af.type = ESpell::kCamouflage;
	af.duration = CalcDuration(ch, ch, ESkill::kDisguise, 0, 15, 0, 2);
	af.modifier = world[ch->in_room]->zone_rn;
	af.location = EApply::kNone;
	af.battleflag = 0;

	if (percent > prob) {
		af.affect_type = EAffect::kUndefined;
	} else {
		af.affect_type = EAffect::kDisguise;
	}

	affect_to_char(ch, af);
	if (!ch->IsImmortal()) {
		timed.skill = ESkill::kDisguise;
		timed.time = 2;
		ImposeTimedSkill(ch, &timed);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
