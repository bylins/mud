#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include "entities/char_data.h"

inline bool IsUnableToAct(CharData *ch) {
	return (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT));
}

int IsHaveNoExtraAttack(CharData *ch);

void SetWait(CharData *ch, int waittime, int victim_in_room);
void SetSkillCooldown(CharData *ch, ESkill skill, int cooldownInPulses);
void SetSkillCooldownInFight(CharData *ch, ESkill skill, int cooldownInPulses);
CharData *FindVictim(CharData *ch, char *argument);

#endif //BYLINS_COMMON_H
