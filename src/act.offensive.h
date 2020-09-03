#ifndef BYLINS_ACT_OFFENSIVE_H
#define BYLINS_ACT_OFFENSIVE_H

#include "char.hpp"

inline bool dontCanAct(CHAR_DATA *ch);

int set_hit(CHAR_DATA * ch, CHAR_DATA * victim);
void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
void setSkillCooldownInFight(CHAR_DATA* ch, ESkill skill, int cooldownInPulses);
void setSkillCooldown(CHAR_DATA* ch, ESkill skill, int cooldownInPulses);
void drop_from_horse(CHAR_DATA *victim);
CHAR_DATA* findVictim(CHAR_DATA* ch, char* argument);
int isHaveNoExtraAttack(CHAR_DATA * ch);
void parry_override(CHAR_DATA * ch);

#endif //BYLINS_ACT_OFFENSIVE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
