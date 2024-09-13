//
// Created by Sventovit on 07.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_TREASURE_CASES_H_
#define BYLINS_SRC_GAME_MECHANICS_TREASURE_CASES_H_

#include "engine/structs/structs.h"

#include <vector>

class CharData;
class ObjData;

namespace treasure_cases {

void LoadTreasureCases();
void UnlockTreasureCase(CharData *ch, ObjData *obj);

} //namespace treasure_cases

#endif //BYLINS_SRC_GAME_MECHANICS_TREASURE_CASES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
