/**
 \file antisaving.h - a part of the Bylins engine.
 \brief Anti-saving ("колдовство"): the caster's side of a saving throw.
 \detail The value is passed as `ext_apply` to CalcGeneralSaving and makes the target's save harder.
         The target's side of the roll lives in saving.h.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_ANTISAVING_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_ANTISAVING_H_

class CharData;
enum class ESpell;

namespace antisaving {

// The caster's general anti-saving: cast success (or a fixed value for immortals and god-flagged
// chars) plus the wisdom bonus, scaled by the condition penalty for players.
int CalcAntiSavings(CharData *ch);

// Class-specific addition for one spell: the class's cast modifier for the spell times the
// caster's skill in the spell's base skill.
int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id);

}  // namespace antisaving

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_ANTISAVING_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
