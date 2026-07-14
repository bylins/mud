/**
\file animate_dead.h - a part of the Bylins engine.
\brief kAnimateDead mechanic: per-type skill-weight control budget + potency-driven undead stats.
\detail issue.animate-dead (#3548). A necromancer's raised undead scale off a single cast-potency
        value C (skill_coeff + stat_coeff from the spell's <potency_roll>), not the caster's
        level/remort. The number and mix of undead a caster can control is a per-type skill-weight
        budget (each undead type "costs" a fixed number of Dark-Magic skill points).
*/

#ifndef GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_
#define GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_

class CharData;

namespace animate_dead {

// The strongest necro-mob tier (vnum) not exceeding `desired` that fits the caster's remaining
// minion budget, DEMOTING down the tier ladder as needed. The budget is
// min(Dark-Magic skill, novice threshold) minus the skill-weight already spent on held undead;
// each undead type "costs" a fixed skill-weight (skeleton 12 ... bonedragon 25 ... necrotank 75),
// so at the novice threshold (75) a caster can hold floor(75/weight) of a type: skeleton 6,
// zombie 5, bonedog 4, bonedragon 3, bonespirit/necrodamager 2, necrotank/breather/caster 1.
// A mix is allowed (2 dragons + 1 bonedog = 50 + 16 <= 68). Returns 0 when even a skeleton does
// not fit, so the caller reports "too many minions". Replaces the old flat count cap.
int FitUndeadTier(CharData *ch, int desired);

// Scale a freshly-read undead's HP and damage off the caster's cast potency C (= competence =
// skill_coeff + stat_coeff). The tier prototype stays the base; C adds on top -- HP multiplicative
// (keeps tier proportions), damage additive on the dice count (necromancer minions are
// damage-first). Saves/resists are left at the prototype (the hook feats plug into).
void SetupUndeadStats(CharData *ch, CharData *mob, double competence);

}  // namespace animate_dead

#endif  // GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
