/**
 \file service_zones_loaders.h - a part of the Bylins engine.
 \brief Загрузчик реестра служебных зон (cfg/mechanics/service_zones.xml).
 \details Держится отдельно от service_zones.h, чтобы cfg_manager.h не расползался
		  по всем, кому нужна одна проверка IsServiceZone.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_SERVICE_ZONES_LOADERS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_SERVICE_ZONES_LOADERS_H_

#include "engine/boot/cfg_manager.h"

namespace service_zones {

// cfg id "service_zones" -> cfg/mechanics/service_zones.xml
class ServiceZonesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

}  // namespace service_zones

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_SERVICE_ZONES_LOADERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
