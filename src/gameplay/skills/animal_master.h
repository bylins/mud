// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_ANIMAL_MASTER_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_ANIMAL_MASTER_H_

#include "gameplay/affects/affect_data.h"
#include "gameplay/skills/skills.h"

class CharData;

// Applies the AnimalMaster feat's transformation to a freshly-charmed kAnimal NPC.
// Rolls a random "charmice type" from the slots not already in use by the caster's
// existing followers, scales stats from the caster's charisma + charm-school skill,
// picks per-type skills/feats/affects, and emits the shape-change narration to the
// room. Writes the chosen weapon-skill into `skill_id` and the computed competency
// percentage into `k_skills` so the SpellCharm tail can pass them to
// create_charmice_stuff. `af` is the in-progress kCharm affect being built for the
// bind; its duration is reused, and it's used as scratch storage for the per-type
// bonus affect applications. issue.magic-code-cleaning: extracted from SpellCharm.
void ApplyAnimalMaster(CharData *ch, CharData *victim, Affect<EApply> &af,
                       int &k_skills, ESkill &skill_id);

#endif // BYLINS_SRC_GAMEPLAY_SKILLS_ANIMAL_MASTER_H_
