/**
\file minions.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 30.10.2025.
\brief Механика миньонов - чармисов, умертвий, подчиненных и т.д.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_MINIONS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_MINIONS_H_

#include "gameplay/magic/spells_constants.h"
#include <memory>

class CharData;
float get_effective_cha(CharData *ch);
float CalcEffectiveWis(CharData *ch, ESpell spell_id);
float get_effective_int(CharData *ch);
int CalcCharmPoint(CharData *ch, ESpell spell_id);
void ClearMinionTalents(CharData *ch);
float CalcDamagePerRound(CharData *victim);
int GetReformedCharmiceHp(CharData *ch, CharData *victim, ESpell spell_id);
// Can `ch` take on `victim` as another charmed follower for `spell_id`? (was mechanics/charm.)
int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id);
int  MaxCharmices(CharData *ch);

// issue.chardata-cleaning: minion-identity predicates (moved off CharData).
bool IsCharmice(const CharData *ch);                                       // charmed/helper NPC minion
inline bool IsCharmice(const std::shared_ptr<CharData> &ch) { return IsCharmice(ch.get()); }
bool IsMortifier(const CharData *ch);                                      // raised-corpse minion

bool IsCharmExpiring(const CharData *ch);   // has a kCharm affect about to wear off (duration <= 1)

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_MINIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
