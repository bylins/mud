/**
\file animate_dead.h - a part of the Bylins engine.
\brief kAnimateDead mechanic: control limit + cast-potency-driven undead stats.
\detail issue.animate-dead (#3548). A necromancer's raised undead scale off a single
        cast-potency value C (skill_coeff + stat_coeff from the spell's <potency_roll>),
        not the shared charm-point budget or the caster's level/remort. This removes the
        non-monotonic level scaling and lets feats add C to a specific stat.
*/

#ifndef GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_
#define GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_

class CharData;

namespace animate_dead {

// Maximum simultaneous undead a necromancer may control (the kAnimateDead control cap).
// Skill-driven and capped at 4 (the old MaxCharmices ceiling): only Dark-Magic skill up to
// the novice threshold counts toward headcount -- skill beyond it flows into per-minion
// potency instead. Wisdom never affects the count. Replaces the reformed-HP budget for
// animate dead (the sole limit now, mirroring kClone which is budget-exempt).
int MaxUndead(CharData *ch);

// Scale a freshly-read undead's HP and damage off the caster's cast potency C
// (= competence = skill_coeff + stat_coeff). Option B: the tier prototype stays the base and
// C adds on top -- HP multiplicative (mirrors the old x(1+remort/10), keeps tier proportions),
// damage additive on the dice count (necromancer minions are damage-first). Saves/resists are
// left at the prototype (the hook feats plug into).
void SetupUndeadStats(CharData *ch, CharData *mob, double competence);

}  // namespace animate_dead

#endif  // GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
