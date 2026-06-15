/**
 \file basic_values.h - a part of the Bylins engine.
 \brief issue.basic: загрузчик cfg/basic.xml - таблицы модификаторов характеристик
        (cha/int/size/weapon/wiz), заполняющие массивы из constants.cpp.
*/

#ifndef BYLINS_SRC_GAMEPLAY_CORE_BASIC_VALUES_H_
#define BYLINS_SRC_GAMEPLAY_CORE_BASIC_VALUES_H_

#include "engine/boot/cfg_manager.h"

// Загрузка cfg/basic.xml через cfg_manager (boot + reload basic).
class BasicValuesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

#endif // BYLINS_SRC_GAMEPLAY_CORE_BASIC_VALUES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
