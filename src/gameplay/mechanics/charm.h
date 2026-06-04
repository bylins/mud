/**
 \file charm.h - a part of the Bylins engine.
 \brief issue.spellhandlers: charmed-minion control mechanics.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_CHARM_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_CHARM_H_

class CharData;
enum class ESpell;

// Can `ch` take on one more charmed follower (`victim`) for `spell_id`? Enforces the
// undead/living-group split and the charm-point / follower-count caps; messages the caster and
// returns false when the limit is hit.
int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_CHARM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
