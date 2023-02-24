#include "entities/char_data.h"
#include "handler.h"

void DoLightwalk(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedFeat timed;

	if (ch->IsNpc() || !CanUseFeat(ch, EFeat::kLightWalk)) {
		SendMsgToChar("Вы не можете этого.\r\n", ch);
		return;
	}

	if (ch->IsOnHorse()) {
		act("Позаботьтесь сперва о мягких тапочках для $N3...", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}

	if (IsAffectedBySpell(ch, ESpell::kLightWalk)) {
		SendMsgToChar("Вы уже двигаетесь легким шагом.\r\n", ch);
		return;
	}
	if (IsTimedByFeat(ch, EFeat::kLightWalk)) {
		SendMsgToChar("Вы слишком утомлены для этого.\r\n", ch);
		return;
	}

	RemoveAffectFromChar(ch, ESpell::kLightWalk);

	timed.feat = EFeat::kLightWalk;
	timed.time = 24;
	ImposeTimedFeat(ch, &timed);

	SendMsgToChar("Хорошо, вы попытаетесь идти, не оставляя лишних следов.\r\n", ch);
	Affect<EApply> af;
	af.type = ESpell::kLightWalk;
	af.duration = CalcDuration(ch, 2, GetRealLevel(ch), 5, 2, 8);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.flags = 0;
	if (number(1, 1000) > number(1, GetRealDex(ch) * 50)) {
		af.affect_bits = 0;
		SendMsgToChar("Вам не хватает ловкости...\r\n", ch);
	} else {
		af.affect_bits = to_underlying(EAffect::kLightWalk);
		SendMsgToChar("Ваши шаги стали легче перышка.\r\n", ch);
	}
	affect_to_char(ch, af);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :