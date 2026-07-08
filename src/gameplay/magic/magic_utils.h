#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include <limits>

#include "gameplay/abilities/feats.h"
#include "engine/entities/entities_constants.h"
#include "spells.h"
#include "gameplay/skills/skills.h"
#include "gameplay/abilities/talents_actions.h"

#include "magic.h"  // RollResult -- used by CalcCastPotency / ComputeApplyModifier

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
int ComputeApplyModifier(const talents_actions::TalentAffect::Apply &apply, double competence,
						 const RollResult &potency);

// issue.random-noise-rework (P1): multiplicative truncated-normal amount. mean = floor + scaled,
// std = sigma*scaled, so the coefficient of variation is ~sigma -- CONSTANT as `scaled` (= k*competence)
// grows, unlike additive dice noise (whose relative spread shrinks). Truncated to [floor, cap]; cap<=0 =
// no upper bound. Uses the seeded global RNG (GaussIntNumber), so simulator runs stay reproducible.
int CalcNoisyAmount(double floor_val, double scaled, double sigma, int cap,
		double fixed_z = std::numeric_limits<double>::quiet_NaN());

// issue.potion-hotfix P3: a potion's casting potency (competence C). The brewed-in kPotionPotency if
// present (crafted); otherwise the potency a fixed-skill potion-maker would brew, from the potion's
// FIRST spell. Returns <=0 if the potion has no castable first spell.
[[nodiscard]] float PotionPotency(const ObjData *potion, ESpell spell_id);

// issue.potion-hotfix: the maker's skill for a potion's buff DURATION (brew skill for crafted, the
// authored maker skill otherwise). The drinker's own skill never matters.
[[nodiscard]] int PotionCastSkill(const ObjData *potion);
[[nodiscard]] int PotionCastStat(const ObjData *potion);

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);

// Resistance calculate //

void CastWeaponAffect(CharData *ch, ESpell spell_id);

#endif // SPELL_PARSER_HPP_
