/**
 \file summon.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: shared summon-pipeline helpers (extracted from magic.cpp).
*/

#include "gameplay/mechanics/summon.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/ai/spec_procs.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/core/char_handler.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/abilities/talents_actions.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/weather.h"
#include "utils/random.h"

int SummonScaledStat(const ActionContext &ctx, double min, double weight, double beta, int cap) {
	talents_actions::TalentAffect::Apply a;
	a.min = min;
	a.weight = weight;
	a.beta = beta;
	a.factor = 1;
	a.cap = cap;
	return ComputeApplyModifier(a, ctx.CompetenceBase(), ctx.potency());
}

int FinalizeSummonedMob(CharData *ch, CharData *mob, ESpell spell_id, bool keeper) {
	mob->set_exp(0);
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
	currencies::SetHand(*mob, currencies::kGold, 0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
	const auto days_from_full_moon =
		(weather_info.moon_day < 14) ? (14 - weather_info.moon_day) : (weather_info.moon_day - 14);
	const int duration = CalcDuration(ch, mob, ESkill::kUndefined,
		GetRealWis(ch) + number(0, days_from_full_moon), 0, 0, 0);
	Affect<EApply> af;
	af.duration = duration;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.affect_type = EAffect::kCharmed;
	af.battleflag = kAfCharmBond;
	affect_to_char(mob, af);
	if (keeper) {
		af.affect_type = EAffect::kHelper;
		affect_to_char(mob, af);
		SetSkill(mob, ESkill::kRescue, 100);
	}
	mob->SetFlag(EMobFlag::kCorpse);
	mob->SetFlag(EMobFlag::kCompanion);	// any NPC ally
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonToRoom).c_str(),
		false, ch, nullptr, mob, kToRoom | kToArenaListen);
	PlaceCharToRoom(mob, ch->in_room);
	follow::AddFollower(ch, mob);
	return duration;
}

bool IsSummonTargetProtected(CharData *ch, CharData *mob, ESpell spell_id) {
	if (!privilege::IsImmortal(ch) && (AFF_FLAGGED(mob, EAffect::kSanctuary) || mob->IsFlagged(EMobFlag::kProtect))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectConsecrated) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	if (!privilege::IsImmortal(ch)
		&& (specials::IsMobSpecial(GET_MOB_VNUM(mob)) || mob->IsFlagged(EMobFlag::kNoResurrection) || mob->IsFlagged(EMobFlag::kAreaAttack))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectNoPower) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	if (!privilege::IsImmortal(ch) && AFF_FLAGGED(mob, EAffect::kGodsShield)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectProtected) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	if (mob->IsFlagged(EMobFlag::kMounting)) {
		mob->UnsetFlag(EMobFlag::kMounting);
	}
	if (mount::IsHorse(mob)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonWarhorse) + "\r\n", ch);
		ExtractCharFromWorld(mob, false);
		return true;
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
