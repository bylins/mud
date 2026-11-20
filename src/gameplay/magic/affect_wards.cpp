/**
 \file affect_wards.cpp - a part of the Bylins engine.
 \brief Affect wards: defenses carried by the target's affects that reflect or absorb an incoming cast,
        plus the code-only magical shield of the Vityaz.
*/


#include "administration/privilege.h"
#include "gameplay/affects/affect_messages.h"
#include "magic_utils.h"
#include "engine/db/global_objects.h"

namespace {
// The Vityaz magical-shield block: a skill+feat+worn-shield absorption. The chance is
// (kShieldBlock / 20 + shield_weight / 2) percent. Mass/area/warcry casts bypass the shield.
bool TryBlockByMagicalShield(CharData *ch, CharData *victim, ESpell spell_id) {
	// У каста по объекту/комнате нет цели-персонажа: без этой проверки ниже разыменуется nullptr.
	if (!victim) return false;
	if (ch == victim) return false;
	if (!MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagWarcry)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagMasses)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagAreas)) return false;
	if (GetSkill(victim, ESkill::kShieldBlock) <= 100) return false;
	if (!GET_EQ(victim, EEquipPos::kShield)) return false;
	if (!CanUseFeat(victim, EFeat::kMagicalShield)) return false;
	const int chance = GetSkill(victim, ESkill::kShieldBlock) / 20
		+ GET_EQ(victim, EEquipPos::kShield)->get_weight() / 2;
	if (number(1, 100) >= chance) return false;
	act("Ваши чары повисли на щите $N1, и затем развеялись.", false, ch, nullptr, victim, kToChar);
	act("Щит $N1 поглотил злые чары $n1.", false, ch, nullptr, victim, kToNotVict);
	act("Ваш щит уберег вас от злых чар $n1.", false, ch, nullptr, victim, kToVict);
	return true;
}

// The target's current value of the resist apply named by <absorption chance=>.
int WardResistStat(CharData *victim, EApply ap) {
	switch (ap) {
		case EApply::kMagicResist:  return GET_MR(victim);
		case EApply::kPhysicResist: return GET_PR(victim);
		default:                    return 0;
	}
}

// Emits a fired ward's kWardTo{Char,Vict,Room} flavor.
void EmitWardMsgs(CharData *caster, CharData *victim, EAffect at) {
	const std::string &mc = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToChar);
	const std::string &mv = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToVict);
	const std::string &mr = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToRoom);
	if (!mc.empty()) { act(mc.c_str(), false, caster, nullptr, victim, kToChar); }
	if (!mv.empty()) { act(mv.c_str(), false, caster, nullptr, victim, kToVict); }
	if (!mr.empty()) { act(mr.c_str(), false, caster, nullptr, victim, kToNotVict); }
}

// A defender ward reacts only to a violent, non-warcry magic attack by a non-god on a distinct
// same-room victim. `is_magic` is always true for now; it lets non-magic skills sharing this pipeline
// opt out of wards later.
bool WardEligible(CharData *caster, CharData *victim, ESpell spell_id, bool is_magic) {
	return is_magic && caster && victim && caster != victim
		&& caster->in_room == victim->in_room
		&& !privilege::IsGod(caster)
		&& !MUD::Spell(spell_id).IsFlagged(kMagWarcry)
		&& MUD::Spell(spell_id).IsViolentAgainst(caster, victim);
}

}  // namespace

// Whole-cast defender wards: reflection (the entire cast bounces to the caster), scope="all"
// absorption and the code-only magical shield. Evaluated once at the is_entry gate, for the top-level
// cast only -- a side-spell sub-cast is part of the same incoming spell (see ctx.Nested()). Scoped
// absorbs are per-manifest (TryScopedAbsorb). First firing ward wins; returns true if one fired.
bool RunWholeCastWards(ActionContext &ctx, bool is_magic) {
	CharData *caster = ctx.caster();
	CharData *victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	if (!is_magic || !caster || !victim || caster == victim || caster->in_room != victim->in_room) {
		return false;
	}
	// Only a violent, non-warcry cast by a non-god triggers affect wards (a warcry against a god
	// fizzles at CastOnTarget's IsGod gate instead).
	if (!privilege::IsGod(caster) && !MUD::Spell(spell_id).IsFlagged(kMagWarcry)
			&& MUD::Spell(spell_id).IsViolentAgainst(caster, victim)) {
		std::vector<EAffect> seen;
		for (const auto &aff : victim->affected) {
			if (!aff || std::find(seen.begin(), seen.end(), aff->affect_type) != seen.end()) {
				continue;
			}
			seen.push_back(aff->affect_type);
			for (const auto &action : affects::AffectActions(aff->affect_type).list()) {
				if (!action.GetTrigger().test(talents_actions::EActionTrigger::kWardAttack)) {
					continue;
				}
				const auto &refl = action.GetReflection();
				const auto &absb = action.GetAbsorption();
				const bool is_reflect = refl.present;
				// Only whole-cast outcomes belong here: a reflect, or an absorption scoped kAll.
				if (!is_reflect
						&& !(absb.present && absb.scope == talents_actions::EWardScope::kAll)) {
					continue;
				}
				int chance;
				if (is_reflect) {
					// potency contest (max>0): clamp(mirror potency - incoming spell potency, min, max);
					// otherwise the fixed prob (spell-side <reflection> / flat-prob reflect).
					chance = (refl.max > 0)
						? std::clamp(static_cast<int>(aff->potency - CalcCastPotency(ctx.potency())),
									 refl.min, std::max(refl.min, refl.max))
						: refl.prob;
				} else if (absb.chance != EApply::kNone) {
					// stat-driven: capped GET_<apply>(victim), same clamp as the elemental-resist path.
					const int cap = caster->IsNpc() ? kMaxNpcResist : kMaxPcResist;
					chance = std::min(cap, WardResistStat(victim, absb.chance));
				} else {
					chance = absb.prob;
				}
				if (number(1, 100) > chance) {
					continue;
				}
				const EAffect at = aff->affect_type;
				const std::string &mc = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToChar);
				const std::string &mv = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToVict);
				const std::string &mr = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToRoom);
				if (!mc.empty()) { act(mc.c_str(), false, caster, nullptr, victim, kToChar); }
				if (!mv.empty()) { act(mv.c_str(), false, caster, nullptr, victim, kToVict); }
				if (!mr.empty()) { act(mr.c_str(), false, caster, nullptr, victim, kToNotVict); }
				if (is_reflect) {
					ctx.cvict = caster;   // whole cast bounces back at the attacker
				} else {
					ctx.SetWardStop();    // scope="all": the whole cast is swallowed
				}
				return true;
			}
		}
	}
	// Code-only defense: the magical shield (absorb-all).
	if (TryBlockByMagicalShield(caster, victim, spell_id)) {
		ctx.SetWardStop();
		return true;
	}
	return false;
}

// A scoped absorption ward (e.g. Shadow Cloak, scope="damage") rolls independently for each payload
// delivery: CastDamage/CastAffect call it right before applying the stage's effect, so a multi-hit
// spell rolls per hit and a nested side-spell is warded as a separate attack. Returns true if this
// manifest was swallowed (the caller skips just this stage). First matching ward wins.
bool TryScopedAbsorb(ActionContext &ctx, talents_actions::EWardScope want) {
	CharData *caster = ctx.caster();
	CharData *victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	if (!WardEligible(caster, victim, spell_id, /*is_magic=*/true)) {
		return false;
	}
	std::vector<EAffect> seen;
	for (const auto &aff : victim->affected) {
		if (!aff || std::find(seen.begin(), seen.end(), aff->affect_type) != seen.end()) {
			continue;
		}
		seen.push_back(aff->affect_type);
		const EAffect at = aff->affect_type;
		for (const auto &action : affects::AffectActions(at).list()) {
			if (!action.GetTrigger().test(talents_actions::EActionTrigger::kWardAttack)) {
				continue;
			}
			const auto &absb = action.GetAbsorption();
			if (!absb.present || absb.scope != want) {
				continue;
			}
			int chance;
			if (absb.chance != EApply::kNone) {
				// GET_MR is a damage-axis stat (the affect stage uses GET_AR), so MR bookkeeping applies only to
				// damage-scoped wards, and only one MR-based ward may roll per stage.
				const bool mr_ward = (want == talents_actions::EWardScope::kDamage
									  && absb.chance == EApply::kMagicResist);
				if (mr_ward && ctx.MrApplied()) {
					continue;
				}
				const int cap = caster->IsNpc() ? kMaxNpcResist : kMaxPcResist;
				chance = std::min(cap, WardResistStat(victim, absb.chance));
				// Spends the stage's single MR roll whether or not it absorbs, so CastDamage won't roll GET_MR again.
				if (mr_ward) {
					ctx.SetMrApplied();
				}
			} else {
				chance = absb.prob;
			}
			if (number(1, 100) > chance) {
				continue;
			}
			EmitWardMsgs(caster, victim, at);
			return true;
		}
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
