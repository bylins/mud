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
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
	virtual void Load(parser_wrapper::DataNode data) = 0;
	virtual void Reload(parser_wrapper::DataNode data) = 0;
	virtual ~ICfgLoader() = default;
};

// issue.vedun-editor: a cfg loader that opts into in-game editing (the Vedun editor). Implementing
// this makes the data discoverable + editable; the editor reuses the loader as the validator.
struct EditableElement {
	std::string id;     // id-token, e.g. "kArmor"
	std::string label;  // human label (e.g. the Russian name)
};
struct ValidationResult {
	bool ok{false};
	std::string error;
};
class IEditableCfgLoader : public virtual ICfgLoader {
 public:
	[[nodiscard]] virtual std::string EditableWhat() const = 0;                       // `vedun <what>`
	[[nodiscard]] virtual std::vector<EditableElement> ListElements() const = 0;      // editable items
	[[nodiscard]] virtual ValidationResult Validate(parser_wrapper::DataNode &doc) const = 0;  // dry-run
	// Locate one element's DOM node inside the loaded file root, so the editor need not hard-code
	// the file layout. Default: a direct child of the root whose `id` attribute matches. Override
	// for a file whose elements are nested differently or keyed by another attribute. The returned
	// node shares the passed root's document (an empty node means "not found").
	[[nodiscard]] virtual parser_wrapper::DataNode FindElementNode(
			parser_wrapper::DataNode root, const std::string &id) const {
		for (auto &child : root.Children()) {
			if (id == child.GetValue("id")) {
				return child;
			}
		}
		return parser_wrapper::DataNode{};
	}
};

using LoaderPtr = std::unique_ptr<ICfgLoader>;

class CfgManager {
 public:
	CfgManager();

	void LoadCfg(const std::string &id);
	void ReloadCfg(const std::string &id);

	// issue.vedun-editor: the editor's discovery surface (loaders implementing IEditableCfgLoader).
	struct EditableEntry {
		std::string what;
		std::filesystem::path file;
		IEditableCfgLoader *loader;
	};
	[[nodiscard]] std::vector<EditableEntry> EditableEntries() const;
	[[nodiscard]] std::optional<EditableEntry> FindEditable(const std::string &what) const;
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
