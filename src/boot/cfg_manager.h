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

#include "utils/wrapper.h"

namespace cfg_manager {

/*
 *  Интерфейс загрузчика конкретного файла конфигурации.
 *  Должен реализовываться в конкретном модуле, для которого и предназначен конфиг,
 *  чтобы менеджер загрузок не захламлялся логикой парсинга отдельных конфигов.
 *  Задача загрузчика - отделить регулярные данные (например - список скиллов или классов)
 *  от вспомогательных , которые тоже могут быть в файле, (например - список ассортиментов в конфиге магазинов)
 *  и каждый набор данных передать в соответствующий парсер/билдер.
 */
class ICfgLoader {
 public:
	virtual void Load(parser_wrapper::DataNode data) = 0;
};

using LoaderPtr = std::unique_ptr<ICfgLoader>;

class CfgManager {
 public:
	CfgManager();

	/*
 	*  Загрузка базовых конфигов - списов умений, аффектов и т.д.,
 	*  без чего невозможна корректная загрузка прочих файлов.
 	*/
	void BootBaseCfgs();
	void ReloadCfgFile(const std::string &id);
 private:
	struct LoaderInfo {
		LoaderInfo(std::filesystem::path file, LoaderPtr loader) :
			file{std::move(file)}, loader{std::move(loader)} {};
		std::filesystem::path file;
		LoaderPtr loader;
	};

	std::unordered_map<std::string, LoaderInfo> loaders_;

	/*
	 *  Загрузка дополнительных, но необходимых конфигов - описаний классов,
	 *  родов персонажей и так далее. Без чего игре навозможна, но загружаться
	 *  это должно после базовых конфигов.
	 */
	//void BootAdvancedCfg();

	/*
	 * Загрузка необязательных конфигов - ассортиментов магазинов, базы данных базара
	 * и так далее, без чего игра запустится и будет работать (возможно, без части функций).
	 */
	//void BootOtherCfg();

	/*
	 *  Загрузка отдельного файла
	 */
	void BootSIngleFile(const std::string &id);
};

} // namespace cfg manager

#endif //BYLINS_SRC_BOOT_CFG_MANAGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
