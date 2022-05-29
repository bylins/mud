#include "entities/char_data.h"
#include "handler.h"

void CheckDeathRage(CharData *ch) {
	struct TimedFeat timed;
	int prob;

	if (IsAffectedBySpell(ch, ESpell::kBerserk) &&
		(GET_HIT(ch) > GET_REAL_MAX_HIT(ch) / 2)) {
		RemoveAffectFromChar(ch, ESpell::kBerserk);
		SendMsgToChar("Предсмертное исступление оставило вас.\r\n", ch);
	}

	if (CanUseFeat(ch, EFeat::kBerserker) && ch->GetEnemy() &&
		!IsTimedByFeat(ch, EFeat::kBerserker) && !AFF_FLAGGED(ch, EAffect::kBerserk)
		&& (GET_HIT(ch) < GET_REAL_MAX_HIT(ch) / 4)) {
		CharData *vict = ch->GetEnemy();
		timed.feat = EFeat::kBerserker;
		timed.time = 4;
		ImposeTimedFeat(ch, &timed);

		Affect<EApply> af;
		af.type = ESpell::kBerserk;
		af.duration = CalcDuration(ch, 1, 60, 30, 0, 0);
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = 0;

		prob = ch->IsNpc() ? 601 : (751 - GetRealLevel(ch) * 5);
		if (number(1, 1000) < prob) {
			af.bitvector = to_underlying(EAffect::kBerserk);
			act("Вас обуяла предсмертная ярость!", false, ch, nullptr, nullptr, kToChar);
			act("$n0 исступленно взвыл$g и бросил$u на противника!", false, ch, nullptr, vict, kToNotVict);
			act("$n0 исступленно взвыл$g и бросил$u на вас!", false, ch, nullptr, vict, kToVict);
		} else {
			af.bitvector = 0;
			act("Вы истошно завопили, пытаясь напугать противника. Без толку.", false, ch, nullptr, nullptr, kToChar);
			act("$n0 истошно завопил$g, пытаясь напугать противника. Забавно...", false, ch, nullptr, vict, kToNotVict);
			act("$n0 истошно завопил$g, пытаясь напугать вас. Забавно...", false, ch, nullptr, vict, kToVict);
		}
		ImposeAffect(ch, af, true, false, true, false);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
