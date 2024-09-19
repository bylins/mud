/**
\file do_courage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/handler.h"

void do_courage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	ObjData *obj;
	int prob, dur;
	struct TimedSkill timed;
	int i;
	if (ch->IsNpc())        // Cannot use GET_COND() on mobs.
		return;

	if (!ch->GetSkill(ESkill::kCourage)) {
		SendMsgToChar("Вам это не по силам.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kCourage)) {
		SendMsgToChar("Вы не можете слишком часто впадать в ярость.\r\n", ch);
		return;
	}

	timed.skill = ESkill::kCourage;
	timed.time = 6;
	ImposeTimedSkill(ch, &timed);
	prob = CalcCurrentSkill(ch, ESkill::kCourage, nullptr) / 20;
	dur = 1 + MIN(5, ch->GetSkill(ESkill::kCourage) / 40);
	Affect<EApply> af[4];
	af[0].type = ESpell::kCourage;
	af[0].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[0].modifier = 40;
	af[0].location = EApply::kAc;
	af[0].bitvector = to_underlying(EAffect::kNoFlee);
	af[0].battleflag = 0;
	af[1].type = ESpell::kCourage;
	af[1].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[1].modifier = MAX(1, prob);
	af[1].location = EApply::kDamroll;
	af[1].bitvector = to_underlying(EAffect::kCourage);
	af[1].battleflag = 0;
	af[2].type = ESpell::kCourage;
	af[2].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[2].modifier = MAX(1, prob * 7);
	af[2].location = EApply::kAbsorbe;
	af[2].bitvector = to_underlying(EAffect::kCourage);
	af[2].battleflag = 0;
	af[3].type = ESpell::kCourage;
	af[3].duration = CalcDuration(ch, dur, 0, 0, 0, 0);
	af[3].modifier = 50;
	af[3].location = EApply::kHpRegen;
	af[3].bitvector = to_underlying(EAffect::kCourage);
	af[3].battleflag = 0;

	for (i = 0; i < 4; i++) {
		ImposeAffect(ch, af[i], false, false, false, false);
	}

	SendMsgToChar("Вы пришли в ярость.\r\n", ch);

	if ((obj = GET_EQ(ch, EEquipPos::kWield)) || (obj = GET_EQ(ch, EEquipPos::kBoths)))
		strcpy(buf, "Глаза $n1 налились кровью и $e яростно сжал$g в руках $o3.");
	else
		strcpy(buf, "Глаза $n1 налились кровью.");
	act(buf, false, ch, obj, nullptr, kToRoom | kToArenaListen);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
