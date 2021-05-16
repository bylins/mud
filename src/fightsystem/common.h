#ifndef BYLINS_COMMON_H
#define BYLINS_COMMON_H

#include "chars/character.h"

inline bool dontCanAct(CHAR_DATA *ch) {
    return (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT));
}

int isHaveNoExtraAttack(CHAR_DATA * ch);

void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
void setSkillCooldown(CHAR_DATA* ch, ESkill skill, int cooldownInPulses);
void setSkillCooldownInFight(CHAR_DATA* ch, ESkill skill, int cooldownInPulses);
CHAR_DATA* findVictim(CHAR_DATA* ch, char* argument);


#endif //BYLINS_COMMON_H
