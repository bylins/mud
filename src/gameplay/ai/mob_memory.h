/**
\file mob_memory.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 13.09.2024.
\brief description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_AI_MOB_MEMORY_H_
#define BYLINS_SRC_GAMEPLAY_AI_MOB_MEMORY_H_

class CharData;

namespace mob_ai {

struct MemoryRecord {
  long id = 0;
  long time = 0;
  struct MemoryRecord *next = nullptr;
};

void mobRemember(CharData *ch, CharData *victim);
void mobForget(CharData *ch, CharData *victim);
void clearMemory(CharData *ch);
void update_mob_memory(CharData *ch, CharData *victim);
CharData *FimdRememberedEnemyInRoom(CharData *mob, int check_sneak);
void AttackToRememberedVictim(CharData *mob, CharData *victim);

} //namespace mob_ai

#endif //BYLINS_SRC_GAMEPLAY_AI_MOB_MEMORY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
