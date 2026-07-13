#ifndef GAMEPLAY_MECHANICS_INITIATIVE_H_
#define GAMEPLAY_MECHANICS_INITIATIVE_H_

class CharData;

// Combat initiative -- decides, within a single combat round, both the ORDER in
// which fighters act and how many attack passes each one gets. See the module
// comment in initiative.cpp for the full description of the mechanic.

// Roll/compute a fighter's initiative value. `mode == true` adds the per-round
// d10 random component (used once per round when the turn order is decided);
// `mode == false` returns the stable, deterministic value (used for display in
// score/identify and as a factor in some skills).
int calc_initiative(CharData *ch, bool mode);

// Roll this round's initiative for `ch` and record it on ch->initiative, folding
// the value into the running [min_init, max_init] window of the current round.
// A rolled 0 becomes -100 ("act last", 1/201 chance). Called once per fighter at
// the start of the round, before the ordered attack passes run.
void roll_round_initiative(CharData *ch, int &min_init, int &max_init);

// Extra-attack pacing: while a fighter's remaining initiative is above the
// round's floor (min_init), spend one point and report that the caller should
// yield its turn (return) so lower-initiative fighters act before it comes back
// around. Returns true when a point was spent (caller must return), false when
// the fighter is already at the floor and should finish its pass.
bool try_consume_extra_pass(CharData *ch, int min_init);

#endif  // GAMEPLAY_MECHANICS_INITIATIVE_H_
