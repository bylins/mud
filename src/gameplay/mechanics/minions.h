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

class CharData;
float get_effective_cha(CharData *ch);
float CalcEffectiveWis(CharData *ch, ESpell spell_id);
float get_effective_int(CharData *ch);
int CalcCharmPoint(CharData *ch, ESpell spell_id);
void ClearMinionTalents(CharData *ch);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_MINIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
