/**
\file bandage.cpp - a part of Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Do bandage command.
*/

#include "entities/char_data.h"
#include "handler.h"

void DoBandage(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}
	if (GET_HIT(ch) == GET_REAL_MAX_HIT(ch)) {
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
		if (GET_OBJ_TYPE(i) == EObjType::kBandage) {
			bandage = i;
			break;
		}
	}
	if (!bandage || GET_OBJ_WEIGHT(bandage) <= 0) {
		SendMsgToChar("В вашем инвентаре нет подходящих для перевязки бинтов!\r\n", ch);
		return;
	}

	SendMsgToChar("Вы достали бинты и начали оказывать себе первую помощь...\r\n", ch);
	act("$n начал$g перевязывать свои раны.&n", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);

	Affect<EApply> af;
	af.type = ESpell::kBandage;
	af.location = EApply::kNone;
	af.modifier = GET_OBJ_VAL(bandage, 0);
	af.duration = CalcDuration(ch, 10, 0, 0, 0, 0);
	af.bitvector = to_underlying(EAffect::kBandage);
	af.battleflag = kAfPulsedec;
	ImposeAffect(ch, af, false, false, false, false);

	af.type = ESpell::kNoBandage;
	af.location = EApply::kNone;
	af.duration = CalcDuration(ch, 60, 0, 0, 0, 0);
	af.bitvector = to_underlying(EAffect::kCannotBeBandaged);
	af.battleflag = kAfPulsedec;
	ImposeAffect(ch, af, false, false, false, false);

	bandage->set_weight(bandage->get_weight() - 1);
	IS_CARRYING_W(ch) -= 1;
	if (GET_OBJ_WEIGHT(bandage) <= 0) {
		SendMsgToChar("Очередная пачка бинтов подошла к концу.\r\n", ch);
		ExtractObjFromWorld(bandage);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
