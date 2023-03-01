/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер файлов конфигурации должен хранить информацию об именах и местоположении файлов конфигурации,
 порядке их загрузки и управлять процессом загрузки данных из файлов по контейнерам.
 */

#ifndef BYLINS_SRC_BOOT_CFG_MANAGER_H_
#define BYLINS_SRC_BOOT_CFG_MANAGER_H_

#include <filesystem>
#include <unordered_map>
#include <utility>

#include "utils/parser_wrapper.h"

namespace cfg_manager {

/**
 *  Интерфейс загрузчика конкретного файла конфигурации.
 *  Должен реализовываться в конкретном модуле, для которого и предназначен конфиг,
 *  чтобы менеджер загрузок не захламлялся логикой парсинга отдельных конфигов.
 *  Задача загрузчика - отделить регулярные данные (например - список скиллов
 *  или классов) от вспомогательных , которые тоже могут быть в файле, (например -
 *  список ассортиментов в конфиге магазинов) и каждый набор данных передать
 *  в соответствующий парсер/билдер.
 */
class ICfgLoader {
 public:
	virtual ~ICfgLoader() = default;
	virtual void Load(parser_wrapper::DataNode data) = 0;
	virtual void Reload(parser_wrapper::DataNode data) = 0;
};

using LoaderPtr = std::unique_ptr<ICfgLoader>;

class CfgManager {
 public:
	CfgManager();

	void LoadCfg(const std::string &id);
	void ReloadCfg(const std::string &id);
 private:
	struct LoaderInfo {
		LoaderInfo(std::filesystem::path file, LoaderPtr loader) :
			file{std::move(file)}, loader{std::move(loader)} {};
		std::filesystem::path file;
		LoaderPtr loader;
	};

	std::unordered_map<std::string, LoaderInfo> loaders_;

};

} // namespace cfg manager

#endif //BYLINS_SRC_BOOT_CFG_MANAGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
