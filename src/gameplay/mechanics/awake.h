/**
\file awake.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Механика awake - флагов блестит, шумит и так далее.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_AWAKE_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_AWAKE_H_

#include "engine/structs/structs.h"

enum EAwakeCheck {
  kAcheckAffects = 1 << 0,
  kAcheckLight = 1 << 1,
  kAcheckHumming = 1 << 2,
  kAcheckGlowing = 1 << 3,
  kAcheckWeight = 1 << 4
};

enum EAwakeMode {
  kAwHide		= 1 << 0,
  kAwInvis		= 1 << 1,
  kAwCamouflage	= 1 << 2,
  kAwSneak		= 1 << 3
};

class CharData;
int check_awake(CharData *ch, int what);
int awake_hide(CharData *ch);
int awake_camouflage(CharData *ch);
int awake_sneak(CharData *ch);
int awaking(CharData *ch, int mode);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_AWAKE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
