#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include "entities/char_data.h"

inline bool IsUnableToAct(CharData *ch) {
	return (AFF_FLAGGED(ch, EAffect::kStopFight) || AFF_FLAGGED(ch, EAffect::kMagicStopFight));
}

int IsHaveNoExtraAttack(CharData *ch);

void SetWait(CharData *ch, int waittime, int victim_in_room);
void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);
void SetSkillCooldownInFight(CharData *ch, ESkill skill, int pulses);
CharData *FindVictim(CharData *ch, char *argument);

#endif //BYLINS_COMMON_H
