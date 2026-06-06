#ifndef BYLINS_SRC_GAMEPLAY_AI_SPEC_ASSIGN_H_
#define BYLINS_SRC_GAMEPLAY_AI_SPEC_ASSIGN_H_

#include "engine/boot/cfg_manager.h"

void AssignMobiles();
void AssignObjects();
void AssignRooms();
void ReloadSpecProcs();

// issue.specials: data-driven spec-proc assignment from cfg/specials.xml (replaces the
// lib/misc/specials.lst flat file + InitSpecProcs). Mob handler name -> proto func.
class SpecialsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

#endif // BYLINS_SRC_GAMEPLAY_AI_SPEC_ASSIGN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
