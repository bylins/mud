/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер хранит имена и расположение файлов конфигурации (декларативная таблица регистрации
 id -> путь -> фабрика загрузчика) и по запросу выполняет загрузку/перезагрузку/запись данных через
 соответствующий ICfgLoader.
 ВАЖНО: ПОРЯДОК загрузки менеджер пока НЕ задаёт - он определяется последовательностью вызовов
 LoadCfg(...) в BootMudDataBase(), где загрузка конфигов перемежается со сторонними загрузчиками
 (в т.ч. загрузкой игрового мира, которую в менеджер не вынести) и зависит от порядка инициализации
 подсистем (например, города/гильдии должны быть готовы раньше связанных с ними данных). Передача
 управления порядком самому менеджеру - отдельная нетривиальная задача (TODO).
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
	// issue.cfg-manager: сериализовать текущее состояние в DOM (зеркало Load). По умолчанию -
	// пусто (большинство конфигов только читаются). Переопределяют загрузчики, которые умеют
	// сохраняться по запросу CfgManager::SaveCfg(id) - они не знают ни пути, ни имени файла.
	virtual void Save(parser_wrapper::DataNode &/*doc*/) const {}
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
	// Is `id` a valid element key, whether or not an element currently exists? Default: only the ids
	// of existing elements are valid. Override to accept any valid key, so the editor can create a
	// new element for an id that does not exist yet.
	// Resolve `id` (case-insensitively) to the canonical element key for this data set, whether or
	// not an element exists yet. Returns "" when `id` is not a valid key. Default: no key is
	// creatable (only existing ids, already resolved by the caller, work). Override to allow
	// creating new elements -- return the canonical (proper-cased) id.
	[[nodiscard]] virtual std::string CanonicalElementId(const std::string &id) const {
		(void) id;
		return "";
	}
	// Create a fresh, minimal element node for `id` directly under `root` (a child of the file root)
	// and return it -- a handle sharing root's document, so the edit persists on save. Returns an
	// empty node when this data set does not support creating new elements (the default).
	[[nodiscard]] virtual parser_wrapper::DataNode CreateElementNode(
			parser_wrapper::DataNode root, const std::string &id) const {
		(void) root;
		(void) id;
		return parser_wrapper::DataNode{};
	}
};

using LoaderPtr = std::unique_ptr<ICfgLoader>;

class CfgManager {
 public:
	CfgManager();

	void LoadCfg(const std::string &id);
	void ReloadCfg(const std::string &id);

	// issue.cfg-manager: запись конфига по запросу - вызывающий не знает ни пути, ни имени файла,
	// только свой строковый id. CfgManager сам решает, в какой файл писать (по таблице регистрации),
	// и пишет атомарно (во временный файл + rename).
	//  - SaveCfg(id): пересборка из памяти - строит пустой DOM, зовёт loader->Save(doc), пишет.
	//  - Save(id, doc): записать уже готовый DOM (для правок на месте: Vedun, рунные камни).
	bool SaveCfg(const std::string &id);
	bool Save(const std::string &id, const parser_wrapper::DataNode &doc);

	// issue.vedun-editor: the editor's discovery surface (loaders implementing IEditableCfgLoader).
	struct EditableEntry {
		std::string id;     // issue.cfg-manager: registry id (for CfgManager::Save(id, doc))
		std::string what;
		std::filesystem::path file;
		IEditableCfgLoader *loader;
	};
	[[nodiscard]] std::vector<EditableEntry> EditableEntries() const;
	[[nodiscard]] std::optional<EditableEntry> FindEditable(const std::string &what) const;
 private:
	struct LoaderInfo {
		LoaderInfo(std::filesystem::path file, std::string root, LoaderPtr loader) :
			file{std::move(file)}, root{std::move(root)}, loader{std::move(loader)} {};
		std::filesystem::path file;
		std::string root;   // issue.cfg-manager: ожидаемый корневой элемент (центр. проверка разбора)
		LoaderPtr loader;
	};

	// issue.cfg-manager: общая реализация LoadCfg/ReloadCfg (reload=true -> Reload).
	void Apply(const std::string &id, bool reload);

	std::unordered_map<std::string, LoaderInfo> loaders_;

};

} // namespace cfg manager

#endif //BYLINS_SRC_BOOT_CFG_MANAGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
