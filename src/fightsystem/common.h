#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include "entities/char.h"

inline bool dontCanAct(CharacterData *ch) {
	return (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT));
}

int isHaveNoExtraAttack(CharacterData *ch);

void set_wait(CharacterData *ch, int waittime, int victim_in_room);
void setSkillCooldown(CharacterData *ch, ESkill skill, int cooldownInPulses);
void setSkillCooldownInFight(CharacterData *ch, ESkill skill, int cooldownInPulses);
CharacterData *findVictim(CharacterData *ch, char *argument);

#endif //BYLINS_COMMON_H
