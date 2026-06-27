//
// Created by Sventovit on 07.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_TREASURE_CASES_H_
#define BYLINS_SRC_GAME_MECHANICS_TREASURE_CASES_H_

#include "engine/structs/structs.h"
#include "engine/boot/cfg_manager.h"   // issue.lib-template: загрузка через CfgManager

#include <vector>

class CharData;
class ObjData;

namespace treasure_cases {

// issue.lib-template: сундуки грузятся из cfg/mechanics/cases.xml через CfgManager
// (ParserWrapper). Формат (casef/object - атрибуты) не менялся.
class TreasureCasesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

void UnlockTreasureCase(CharData *ch, ObjData *obj);

} //namespace treasure_cases

#endif //BYLINS_SRC_GAME_MECHANICS_TREASURE_CASES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
