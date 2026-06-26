#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/magic/magic.h"

void CheckDeathRage(CharData *ch) {
	struct TimedFeat timed;
	int prob;

	if (IsAffected(ch, EAffect::kBerserk) &&
		(ch->get_hit() > ch->get_real_max_hit() / 2)) {
		RemoveAffectFromChar(ch, EAffect::kBerserk);
		SendMsgToChar("Предсмертное исступление оставило вас.\r\n", ch);
	}

	if (CanUseFeat(ch, EFeat::kBerserker) && ch->GetEnemy() &&
		!IsTimedByFeat(ch, EFeat::kBerserker) && !AFF_FLAGGED(ch, EAffect::kBerserk)
		&& (ch->get_hit() < ch->get_real_max_hit() / 4)) {
		CharData *vict = ch->GetEnemy();
		timed.feat = EFeat::kBerserker;
		timed.time = 4;
		ImposeTimedFeat(ch, &timed);

		Affect<EApply> af;
		af.duration = CalcDuration(ch, ch, ESkill::kUndefined, 3, 0, 0, 0);
		af.modifier = 0;
		af.location = EApply::kNone;
		af.battleflag = 0;

		prob = ch->IsNpc() ? 601 : (751 - GetRealLevel(ch) * 5);
		const bool berserk_failed = !(number(1, 1000) < prob);
		af.affect_type = EAffect::kBerserk;
		af.battleflag = berserk_failed ? static_cast<Bitvector>(kAfFailed) : 0;
		ImposeAffect(ch, af, true, false, true, false);
		// issue.affect-migration: success/fail narration on kBerserk; vict = the combat foe (external $N).
		EmitAffectImpose(ch, vict, EAffect::kBerserk, berserk_failed);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
