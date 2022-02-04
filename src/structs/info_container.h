/**
\authors Created by Sventovit
\date 2.02.2022.
\brief Универсальный, насколько возможно, контейнер для хранения информации об игровых сущностях типа скиллов и классов.
\details
*/

#ifndef BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_
#define BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

#include <memory>
#include <unordered_map>

#include "utils/logger.h"

namespace info_container {

const bool kStrictParsing = true;
constexpr bool kTolerantParsing = !kStrictParsing;

template<class E>
class InfoContainer {
 public:
	InfoContainer() {
		if (!items_) {
			items_ = std::make_unique<Register>();
			//items_->emplace(E::kUndefined, std::make_unique<Pair>(std::make_pair(false, Item())));
		}
	};
	InfoContainer(InfoContainer &s) = delete;
	void operator=(const InfoContainer &s) = delete;

	class Item;
	class ItemBuilder;

	/*
	 *  Возвращает элемент с указанным id или с id kUndefined.
	 */
	const Item &operator[](E id) const {
		try {
			return items_->at(id)->second;
		} catch (const std::out_of_range &) {
			err_log("Incorrect id (%d) passed into %s.", to_underlying(id), typeid(this).name());
			return items_->at(E::kUndefined)->second;
		}
	};

	/*
	 *  Инициализация. Для реинициализации используйте Reload();
	 */
	void Init(const std::string &arg) {
		if (IsInitizalized()) {
			err_log("This MUD already has an initialized %s. Use 'Reload()'.", arg.c_str());
			return;
		}
		Item item;
		items_ = std::move(RegisterBuilder::Build(kTolerantParsing).value());
	};

	/*
	 *  Горячая перезагрузка. Позволяет перегрузить данные контейнера.
	 *  Конфликты между новыми и старыми данными не отслеживаются.
	 */
	void Reload(const std::string &arg) {
		auto new_items = RegisterBuilder::Build(kStrictParsing);
		if (new_items) {
			items_ = std::move(new_items.value());
		} else {
			err_log("%s reloading canceled - file damaged.", arg.c_str());
		}
	};

	/*
	 *  Id известен. Не гарантируется, что он означает корректный элемент.
	 */
	bool IsKnown(E id) { return items_->contains(id); };

	/*
	 *  Id неизвестен.
	 */
	bool IsUnknown(E id) { return !IsKnown(id); };

	/*
	 *  Id известен, определен, не отключен и лежит между kFirst и kLast.
	 */
	bool IsValid(E id) {
		bool validity = IsKnown(id) && id >= E::kFirst && id <= E::kLast;
		if (validity) {
			validity &= IsEnabled(id);
		}
		return validity;
	};

	/*
	 *  Id неизвестен, не определен, отключен или лежит между kFirst и kLast.
	 */
	bool IsInvalid(E id) { return !IsValid(id); };

 private:
	using Pair = std::pair<bool, Item>;
	using PairPtr = std::unique_ptr<Pair>;
	using Optional = std::optional<PairPtr>;
	using Register = std::unordered_map<E, PairPtr>;
	using RegisterPtr = std::unique_ptr<Register>;

	class RegisterBuilder {
	 public:
		Optional Build(bool strict_parsing) {
			strict_pasring_ = strict_parsing;
			pugi::xml_document doc;
			auto nodes = XMLLoad(Item::cfg_file_name, Item::xml_main_tag, Item::load_fail_msg, doc);
			if (nodes.empty()) {
				return std::nullopt;
			}
			return Parse(nodes, Item::xml_entity_tag);
		}
	 private:
		static bool strict_pasring_;

	};

	RegisterBuilder builder;

	RegisterPtr items_;

	/*
	 *  Такой id известен и элемент доступен. Не гарантируется, что элемент корректен.
	 */
	bool IsEnabled(const E id) { return items_->at(id)->first; }

	/*
	 *  Такой id известен и элемент доступен. Не гарантируется, что элемент корректен.
	 */
	bool IsDisabled(const E id) { return !IsEnabled(id); }

	/*
	 *  Контейнер уже инициализирован.
	 */
	bool IsInitizalized() { return (items_->size() > 1); }

};

} // info_container

#endif //BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
