//
// Created Sventovit on 07.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_STABLE_OBJS_H_
#define BYLINS_SRC_GAME_MECHANICS_STABLE_OBJS_H_

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"

class CObjectPrototype;

namespace stable_objs {

// Loads cfg/mechanics/stable_objs.xml into the per-wear-slot criteria. Plain cfg_manager loader
// (no Vedun editing): supports boot load + hot reload (reload is idempotent -- clears first).
class StableObjsLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

bool IsTimerUnlimited(const CObjectPrototype *obj);
bool IsObjFromSystemZone(ObjVnum vnum);
double CountUnlimitedTimer(const CObjectPrototype *obj);

} // stable_objs

#endif //BYLINS_SRC_GAME_MECHANICS_STABLE_OBJS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
