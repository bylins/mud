/**
\file do_courage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/magic/magic.h"

void do_courage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}

	if (!GetSkill(ch, ESkill::kCourage)) {
		SendMsgToChar("Вам это не по силам.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kCourage)) {
		SendMsgToChar("Вы не можете слишком часто впадать в ярость.\r\n", ch);
		return;
	}

	struct TimedSkill timed;
	timed.skill = ESkill::kCourage;
	timed.time = 6;
	ImposeTimedSkill(ch, &timed);
	auto prob = CalcCurrentSkill(ch, ESkill::kCourage, nullptr) / 20;
	auto dur = 1 + std::min(5, GetSkill(ch, ESkill::kCourage) / 40);
	Affect<EApply> af[5];
	af[0].duration = CalcDuration(ch, ch, ESkill::kUndefined, dur, 0, 0, 0);
	af[0].modifier = 40;
	af[0].location = EApply::kAc;
	af[0].affect_type = EAffect::kCourage;
	af[0].battleflag = 0;
	af[1].duration = CalcDuration(ch, ch, ESkill::kUndefined, dur, 0, 0, 0);
	af[1].modifier = std::max(1, prob);
	af[1].location = EApply::kDamroll;
	af[1].affect_type = EAffect::kCourage;
	af[1].battleflag = 0;
	af[2].duration = CalcDuration(ch, ch, ESkill::kUndefined, dur, 0, 0, 0);
	af[2].modifier = std::max(1, prob * 7);
	af[2].location = EApply::kAbsorbe;
	af[2].affect_type = EAffect::kCourage;
	af[2].battleflag = 0;
	af[3].duration = CalcDuration(ch, ch, ESkill::kUndefined, dur, 0, 0, 0);
	af[3].modifier = 50;
	af[3].location = EApply::kHpRegen;
	af[3].affect_type = EAffect::kCourage;
	af[3].battleflag = 0;
	// issue.affects-improve: courage steels you against fleeing (EApply::kBind), replacing the old kNoFlee affect.
	af[4].duration = CalcDuration(ch, ch, ESkill::kUndefined, dur, 0, 0, 0);
	af[4].modifier = 1;
	af[4].location = EApply::kBind;
	af[4].affect_type = EAffect::kCourage;
	af[4].battleflag = 0;

	for (auto & i : af) {
		ImposeAffect(ch, i, false, false, false, false);
	}

	// issue.affect-migration: imposition narration lives on the kCourage affect (affect_msg.xml).
	EmitAffectImpose(ch, nullptr, EAffect::kCourage, false);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
