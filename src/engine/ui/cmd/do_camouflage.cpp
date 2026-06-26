/**
\file do_camouflage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/skills/skills.h"
#include "engine/db/global_objects.h"

void do_camouflage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedSkill timed;
	int prob, percent;

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kDisguise)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar("Вы замаскировались под статую Юрия Долгорукого.\r\n", ch);
		return;
	}

	if (IsAffected(ch, EAffect::kGlitterDust)) {
		SendMsgToChar("Вы замаскировались под золотую рыбку.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kDisguise)) {
		SendMsgToChar("У вас пока не хватает фантазии. Побудьте немного самим собой.\r\n", ch);
		return;
	}

	if (privilege::IsImmortal(ch)) {
		RemoveAffectFromChar(ch, EAffect::kDisguise);
	}

	if (IsAffectedOrAttempting(ch, EAffect::kDisguise)) {
		SendMsgToChar("Вы уже маскируетесь.\r\n", ch);
		return;
	}

	SendMsgToChar("Вы начали усиленно маскироваться.\r\n", ch);
	ch->Temporary.unset(ECharExtraFlag::kFailCamouflage);
	percent = number(1, MUD::Skill(ESkill::kDisguise).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kDisguise, nullptr);

	Affect<EApply> af;
	af.duration = CalcDuration(ch, ch, ESkill::kDisguise, 0, 15, 0, 2);
	af.modifier = world[ch->in_room]->zone_rn;
	af.location = EApply::kNone;
	af.battleflag = 0;

	if (percent > prob) {
		af.affect_type = EAffect::kDisguise;
		af.battleflag = kAfFailed;
	} else {
		af.affect_type = EAffect::kDisguise;
	}

	affect_to_char(ch, af);
	if (!privilege::IsImmortal(ch)) {
		timed.skill = ESkill::kDisguise;
		timed.time = 2;
		ImposeTimedSkill(ch, &timed);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
