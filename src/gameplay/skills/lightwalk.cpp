#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/magic/magic.h"

void DoLightwalk(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	struct TimedFeat timed;

	if (ch->IsNpc() || !CanUseFeat(ch, EFeat::kLightWalk)) {
		SendMsgToChar("Вы не можете этого.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		act("Позаботьтесь сперва о мягких тапочках для $N3...", false, ch, nullptr, mount::GetHorse(ch), kToChar);
		return;
	}

	if (IsAffected(ch, EAffect::kLightWalk)) {
		SendMsgToChar("Вы уже двигаетесь легким шагом.\r\n", ch);
		return;
	}
	if (IsTimedByFeat(ch, EFeat::kLightWalk)) {
		SendMsgToChar("Вы слишком утомлены для этого.\r\n", ch);
		return;
	}

	RemoveAffectFromChar(ch, EAffect::kLightWalk);

	timed.feat = EFeat::kLightWalk;
	timed.time = 24;
	ImposeTimedFeat(ch, &timed);

	SendMsgToChar("Хорошо, вы попытаетесь идти, не оставляя лишних следов.\r\n", ch);
	Affect<EApply> af;
	af.duration = CalcDuration(ch, ch, ESkill::kHide, 2, 13, 2, 8);
	af.modifier = 0;
	af.location = EApply::kNone;
	const bool lw_failed = (number(1, 1000) > number(1, GetRealDex(ch) * 50));
	af.affect_type = EAffect::kLightWalk;
	af.battleflag = lw_failed ? static_cast<Bitvector>(kAfFailed) : 0;
	affect_to_char(ch, af);
	// issue.affect-migration: success/fail narration on the kLightWalk affect (kAfFailed-driven).
	EmitAffectImpose(ch, nullptr, EAffect::kLightWalk, lw_failed);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :