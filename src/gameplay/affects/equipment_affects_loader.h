/**
 \brief CfgManager loader for cfg/equipment_affects.xml -- the equipment_affect table
 (issue.equipment-affects-cfg). Kept out of equipment_affects.h so the low-level enum header
 (included by obj_data.h) does not pull in cfg_manager.
*/

#ifndef BYLINS_SRC_AFFECTS_EQUIPMENT_AFFECTS_LOADER_H_
#define BYLINS_SRC_AFFECTS_EQUIPMENT_AFFECTS_LOADER_H_

#include "engine/boot/cfg_manager.h"

class EquipmentAffectsLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

#endif  // BYLINS_SRC_AFFECTS_EQUIPMENT_AFFECTS_LOADER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
