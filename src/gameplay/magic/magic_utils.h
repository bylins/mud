#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include "gameplay/abilities/feats.h"
#include "engine/entities/entities_constants.h"
#include "spells.h"
#include "gameplay/skills/skills.h"
#include "gameplay/abilities/talents_actions.h"

#include "magic.h"  // RollResult -- used by CalcCastPotency / ComputeApplyModifier
#include <cmath>
#include <algorithm>

class CharData;
struct RoomData;
class ObjData;

/**
 * Проверяется что первая строка является эквивалентом второй, например
 * строка 'ог ша' является эквивалентом строки 'огненный шар'.
 */
ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillId(char *name);
ESkill FixNameFndFindSkillId(std::string &name);

ESpell FindSpellId(const std::string &name);
ESpell FindSpellId(const char *name);
ESpell FixNameAndFindSpellId(std::string &name);

EFeat FindFeatId(const char *name);
EFeat FixNameAndFindFeatId(const std::string &name);

abilities::EAbility FindAbilityId(const std::string &name);
abilities::EAbility FixNameAndFindAbilityId(const std::string &name);

ESpell FindSpellIdWithName(const std::string &name);

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom);
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj);

// True if `caster` bypasses the spell's room-level <blocking> (kNoMagic etc.).
// Greater gods always bypass; uncharmed NPCs (other than tutelars) bypass too -- they
// ignore the room block when ticking their own affects. (issue.room-affects: renamed
// from MayCastInNomagic; the kMagWarcry exception was removed because warcry spells
// simply do not list kNoMagic as a blocking flag.)
bool MayCastInForbiddenRoom(CharData *caster);

// True if `room` carries any of the room flags listed in the spell's
// <blocking><room_flags val="..."/></blocking>. Used in CallMagic and
// CallMagicToRoom to fizzle casts whose XML config forbids the room.
bool IsRoomBlocked(RoomData *room, const talents_actions::FlagCondition &cond);

// Cast-recorded potency: the scalar strength stored on every imposed affect
// (Affect::potency) for later dispel comparisons (DispelSucceeds). It is the
// straight sum of the cast's potency-roll components -- dice + skill + stat --
// captured at the moment of imposition. Shared by CastAffect and
// CallMagicToRoom so both record the same value for the same cast.
float CalcCastPotency(const RollResult &potency);

// Apply-side modifier value used by every affect imposition path:
//     raw = min + ceil(dices·dices_weight·(1 + alpha·C) + beta·C)   C = skill_coeff+stat_coeff
//     raw = min(raw, cap)        if cap > 0
//     return static_cast<int>(factor · raw)
// Shared by CastAffect's per-target apply_one lambda and CallMagicToRoom's
// first-apply default. cap == 0 disables the clamp; factor is allowed to be
// negative (debuffs), with the clamp acting on the pre-factor magnitude.
// issue.affects-improve: templated so it serves a spell apply (TalentAffect::Apply) AND an
// affect-owned apply (affects::AffectApply) -- both carry min/dices_weight/alpha/beta/factor/cap.
template<class ApplyT>
int ComputeApplyModifier(const ApplyT &apply, double competence, const RollResult &potency) {
	const double competencies = competence;
	double raw = apply.min + std::ceil(
			potency.dices * apply.dices_weight * (1.0 + apply.alpha * competencies)
			+ apply.beta * competencies);
	if (apply.cap > 0) {
		raw = std::min(raw, static_cast<double>(apply.cap));
	}
	return static_cast<int>(apply.factor * raw);
}

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);

// Resistance calculate //

void CastWeaponAffect(CharData *ch, ESpell spell_id);

#endif // SPELL_PARSER_HPP_
