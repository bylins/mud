/**
\file bandage.cpp - a part of Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Do bandage command.
*/

#include "engine/entities/char_data.h"
#include "engine/core/obj_handler.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/magic/magic.h"

void DoBandage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}
	if (ch->get_hit() == ch->get_real_max_hit()) {
		SendMsgToChar("Вы не нуждаетесь в перевязке!\r\n", ch);
		return;
	}
	if (ch->GetEnemy()) {
		SendMsgToChar("Вы не можете перевязывать раны во время боя!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kBandage)) {
		SendMsgToChar("Вы и так уже занимаетесь перевязкой!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kCannotBeBandaged)) {
		SendMsgToChar("Вы не можете перевязывать свои раны так часто!\r\n", ch);
		return;
	}

	ObjData *bandage = nullptr;
	for (ObjData *i = ch->carrying; i; i = i->get_next_content()) {
		if (i->get_type() == EObjType::kBandage) {
			bandage = i;
			break;
		}
	}
	if (!bandage || bandage->get_weight() <= 0) {
		SendMsgToChar("В вашем инвентаре нет подходящих для перевязки бинтов!\r\n", ch);
		return;
	}


	Affect<EApply> af;
	af.location = EApply::kNone;
	af.modifier = GET_OBJ_VAL(bandage, 0);
	af.duration = CalcDuration(ch, ch, ESkill::kUndefined, 10, 0, 0, 0);
	af.affect_type = EAffect::kBandage;
	af.battleflag = kAfPulsedec;
	ImposeAffect(ch, af, false, false, false, false);

	af.location = EApply::kNone;
	af.duration = CalcDuration(ch, ch, ESkill::kUndefined, 60, 0, 0, 0);
	af.affect_type = EAffect::kCannotBeBandaged;
	af.battleflag = kAfPulsedec;
	ImposeAffect(ch, af, false, false, false, false);
	// issue.affect-migration: imposition narration on the kBandage affect (self).
	EmitAffectImpose(ch, nullptr, EAffect::kBandage, false);

	bandage->set_weight(bandage->get_weight() - 1);
	IS_CARRYING_W(ch) -= 1;
	if (bandage->get_weight() <= 0) {
		SendMsgToChar("Очередная пачка бинтов подошла к концу.\r\n", ch);
		ExtractObjFromWorld(bandage);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
