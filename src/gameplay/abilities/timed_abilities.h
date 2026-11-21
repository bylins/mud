/**
\file timed_abilities.h - a part of the Bylins engine.
\brief Per-character timers for feats and skills (timed talents).
\details issue.handler-cleaning (Bucket 5): relocated from handler.h/handler.cpp. The feat
 and skill variants remain duplicated on purpose -- they are meant to collapse once the
 unified abilities system lands; not reworked here.
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_TIMED_ABILITIES_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_TIMED_ABILITIES_H_

#include "gameplay/abilities/feats.h"   // TimedFeat, EFeat
#include "gameplay/skills/skills.h"     // TimedSkill, ESkill

class CharData;

const int kSecsPerPlayerTimed = 1;

void ImposeTimedFeat(CharData *ch, TimedFeat *timed);
// The delay comes from the feat's <timed base> in feats.xml. Without one it is a mistake in the file:
// logged, and the longest delay (kHoursPerDay) is imposed so the feat cannot be spammed.
void ImposeTimedFeat(CharData *ch, EFeat feat);
int IsTimedByFeat(CharData *ch, EFeat feat);
void ImposeTimedSkill(CharData *ch, TimedSkill *timed);
int IsTimedBySkill(CharData *ch, ESkill id);

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_TIMED_ABILITIES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
