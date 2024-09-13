//
// Created Sventovit on 07.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_STABLE_OBJS_H_
#define BYLINS_SRC_GAME_MECHANICS_STABLE_OBJS_H_

#include "engine/entities/entities_constants.h"
#include "third_party_libs/pugixml/pugixml.h"

class CObjectPrototype;

namespace stable_objs {

void LoadCriterionsCfg();
bool IsTimerUnlimited(const CObjectPrototype *obj);
bool IsObjFromSystemZone(ObjVnum vnum);
double CountUnlimitedTimer(const CObjectPrototype *obj);

} // stable_objs

#endif //BYLINS_SRC_GAME_MECHANICS_STABLE_OBJS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
