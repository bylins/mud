/**
\authors Created by Sventovit
\date 2.02.2022.
\brief Универсальный, насколько возможно, контейнер для хранения информации об игровых сущностях типа скиллов и классов.
\details
*/

#ifndef BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_
#define BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "utils/logger.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"

/**
 	kDisabled - элемент отключен полностью. При обнаружении у персонажа - удаляется или заменяется на default value.
	kService - элемент служебный. Не показывается через команды игрока, не должно быть возможно поставить вручную.
	kFrozen - элемент временно отключен. Ивентовые, праздничные и т.п. элементы.
	kTesting - элемент в режиме тестирования. По умолчанию не виден, но не удаляется и может быть установлен имморталом.
	kEnabled - обычный режим по умолчанию, элемент полностью подключен.
 */
enum class EItemMode {
	kDisabled = 0,
	kService,
	kFrozen,
	kTesting,
	kEnabled
};

template<>
const std::string &NAME_BY_ITEM<EItemMode>(EItemMode item);
template<>
EItemMode ITEM_BY_NAME<EItemMode>(const std::string &name);

namespace info_container {

/**
 * Интерфейс элемента контейнера.
 */
template<typename E>
class IItem {
  public:
	[[nodiscard]] virtual EItemMode GetMode() const = 0;
	[[nodiscard]] virtual E GetId() const = 0;
	/**
	 *  Элемент доступен либо тестируется.
	 */
	[[nodiscard]] bool IsValid() const { return (IsAvailable() || GetMode() == EItemMode::kTesting); };
	/**
	 *  Элемент отключен.
	 */
	[[nodiscard]] bool IsInvalid() const { return !IsValid(); };
	/**
	 *  Элемент доступен и не тестируется).
	 */
	[[nodiscard]] bool IsAvailable() const { return (GetMode() == EItemMode::kEnabled); };
	/**
	 *  Элемент недоступен или тестируется.
	 */
	[[nodiscard]] bool IsUnavailable() const { return !IsAvailable(); };
};

template<typename I>
class IItemBuilder {
 public:
	using ItemPtr = std::shared_ptr<I>;
	using ItemOptional = std::optional<ItemPtr>;

	virtual ItemOptional Build(parser_wrapper::DataNode &node) = 0;
	static EItemMode ParseItemMode(parser_wrapper::DataNode &node, EItemMode default_mode);
};

template<typename E, typename I, typename B>
class InfoContainer {
 public:
	InfoContainer();
	InfoContainer(InfoContainer &s) = delete;
	void operator=(const InfoContainer &s) = delete;

	using ItemPtr = std::shared_ptr<I>;
	using ItemOptional = std::optional<ItemPtr>;
	using Register = std::unordered_map<E, ItemPtr>;
	using NodeRange = iterators::Range<parser_wrapper::DataNode>;

/* ----------------------------------------------------------------------
 * 	Интерфейс контейнера.
 ---------------------------------------------------------------------- */
	/**
	 *  Возвращает элемент с указанным id или с id kUndefined.
	 */
	const I &operator[](E id) const;
	/**
	 *  Инициализация. Для реинициализации используйте Reload();
	 */
	void Init(const NodeRange &data);
	/**
	 *  Горячая перезагрузка. Позволяет перегрузить данные контейнера.
	 */
	void Reload(const NodeRange &data);
	/**
	 *  Id известен. Не гарантируется, что он означает корректный элемент.
	 */
	bool IsKnown(const E id) const { return items_->contains(id); };
	/**
	 *  Id неизвестен.
	 */
	bool IsUnknown(const E id) const { return !IsKnown(id); };
	/**
	 *  Id доступен либо тестируется.
	 */
	bool IsValid(const E id) const { return (IsUnknown(id) ? false : IsEnabled(id) || IsBeingTesting(id)); };
	/**
	 *  Id отключен.
	 */
	bool IsInvalid(const E id) const { return !IsValid(id); };
	/**
	 *  Id доступен и не тестируется).
	 */
	bool IsAvailable(const E id) const { return (IsUnknown(id) ? false : IsEnabled(id)); };
	/**
	 *  Id недоступен или тестируется.
	 */
	bool IsUnavailable(const E id) const { return !IsAvailable(id); };
	/**
	 *  Контейнер уже инициализирован.
	 */
	bool IsInitizalized() { return (items_->size() > 1); }
	/**
	 *  Создает опционала для парсера элементов.
	 */
	ItemOptional MakeItemOptional();
	/**
	 * Пара итераторов, возвращающих коннстантные ссылки.
	 * Обычный const_iterator не блокирует возможность изменить значение,
	 * на которое указывает указатель, поэтому была применена обертка.
	 */
	auto begin() const;
	auto end() const;
	/**
	 * Найти элемент, соответствующий числу, скастованному в индекс элемента.
	 * @return - элемент или элемент kUndefined.
	 */
	const I &FindItem(int num) const;
	/**
	 * Найти подключенный элемент, соответствующий числу, скастованному в индекс элемента.
	 * @return - элемент или элемент kUndefined.
	 */
	const I &FindAvailableItem(int num) const;

 private:
	friend class RegisterBuilder;
	using RegisterPtr = std::unique_ptr<Register>;
	using RegisterOptional = std::optional<RegisterPtr>;

	/**
	 *  Класс строителя контейнера в целом.
	 */
	class RegisterBuilder {
	 public:
		static RegisterOptional StrictBuild(const NodeRange &data);
		static RegisterOptional TolerantBuild(const NodeRange &data);

	 private:
		static bool strict_pasring_;
		static B item_builder_;

		static RegisterOptional Build(const NodeRange &data);
		static RegisterOptional Parse(const NodeRange &data);
		static void EmplaceItem(Register &items, ItemPtr &item);
		static void EmplaceDefaultItems(Register &items);
	};

	RegisterPtr items_;

	/**
	 *  Такой id известен, но элемент отключен.
	 */
	bool IsDisabled(const E id) const { return items_->at(id)->GetMode() == EItemMode::kDisabled; }
	/**
	 *  Такой id известен и находится в режиме тестирования.
	 */
	bool IsBeingTesting(const E id) const { return items_->at(id)->GetMode() == EItemMode::kTesting; }
	/**
	 *  Такой id известен и элемент доступен. Не гарантируется, что элемент корректен.
	 */
	bool IsEnabled(const E id) const { return items_->at(id)->GetMode() == EItemMode::kEnabled; }

};

template<typename E, typename I, typename B> B InfoContainer<E, I, B>::RegisterBuilder::item_builder_;
template<typename E, typename I, typename B> bool InfoContainer<E, I, B>::RegisterBuilder::strict_pasring_;

/* ----------------------------------------------------------------------
 * 	Реализация InfoContainer
 ---------------------------------------------------------------------- */

template<typename E, typename I, typename B>
InfoContainer<E, I, B>::InfoContainer() {
	if (!items_) {
		items_ = std::make_unique<Register>();
	}
}

template<typename E, typename I, typename B>
void InfoContainer<E, I, B>::Reload(const NodeRange &data) {
	auto new_items = RegisterBuilder::StrictBuild(data);
	if (new_items) {
		items_ = std::move(new_items.value());
	} else {
		err_log("Reloading was canceled - file damaged.");
	}
};

template<typename E, typename I, typename B>
const I &InfoContainer<E, I, B>::operator[](E id) const {
	try {
		return *(items_->at(id));
	} catch (const std::out_of_range &) {
		err_log("Incorrect id (%d) passed into %s.", to_underlying(id), typeid(this).name());
		return *(items_->at(E::kUndefined));
	}
};

template<typename E, typename I, typename B>
void InfoContainer<E, I, B>::Init(const NodeRange &data) {
	if (IsInitizalized()) {
		err_log("Don't try re-init containers. Use 'Reload()'.");
		return;
	}
	items_ = std::move(RegisterBuilder::TolerantBuild(data).value());
};

template<typename E, typename I, typename B>
typename InfoContainer<E, I, B>::ItemOptional InfoContainer<E, I, B>::MakeItemOptional() {
	return std::make_optional(std::make_shared<I>());
}

template<typename E, typename I, typename B>
auto InfoContainer<E, I, B>::begin() const {
	iterators::ConstIterator<decltype(items_->begin()), I> it(items_->begin());
	return it;
}

template<typename E, typename I, typename B>
auto InfoContainer<E, I, B>::end() const {
	iterators::ConstIterator<decltype(items_->end()), I> it(items_->end());
	return it;
}

template<typename E, typename I, typename B>
const I &InfoContainer<E, I, B>::FindItem(const int num) const {
	E id  = static_cast<E>(num);
	if (IsKnown(id)) {
		return *(items_->at(id));
	} else {
		return *(items_->at(E::kUndefined));
	}
}

template<typename E, typename I, typename B>
const I &InfoContainer<E, I, B>::FindAvailableItem(const int num) const {
	E id  = static_cast<E>(num);
	if (IsAvailable(id)) {
		return *(items_->at(id));
	} else {
		return *(items_->at(E::kUndefined));
	}
}

/* ----------------------------------------------------------------------
 * 	Реализация RegisterBuilder
 ---------------------------------------------------------------------- */

template<typename I>
EItemMode IItemBuilder<I>::ParseItemMode(parser_wrapper::DataNode &node, EItemMode default_mode) {
	try {
		return parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
		return default_mode;
	}
};

template<typename E, typename I, typename B>
typename InfoContainer<E, I, B>::RegisterOptional InfoContainer<E, I, B>::RegisterBuilder::StrictBuild(const NodeRange &data) {
	strict_pasring_ = true;
	return Build(data);
}

template<typename E, typename I, typename B>
typename InfoContainer<E, I, B>::RegisterOptional InfoContainer<E, I, B>::RegisterBuilder::TolerantBuild(const NodeRange &data) {
	strict_pasring_ = false;
	return Build(data);
}

template<typename E, typename I, typename B>
typename InfoContainer<E, I, B>::RegisterOptional InfoContainer<E, I, B>::RegisterBuilder::Build(const NodeRange &data) {
	auto items = Parse(data);
	if (items) {
		EmplaceDefaultItems(*items.value());
	}
	return items;
}

template<typename E, typename I, typename B>
typename InfoContainer<E, I, B>::RegisterOptional InfoContainer<E, I, B>::RegisterBuilder::Parse(const NodeRange &data) {
	auto items = std::make_optional(std::make_unique<Register>());

	for (auto &node : data) {
		auto item = item_builder_.Build(node);
		if (item) {
			EmplaceItem(*items.value(), item.value());
		} else if (strict_pasring_) {
			return std::nullopt;
		}
	}

	return items;
}

template<typename E, typename I, typename B>
void InfoContainer<E, I, B>::RegisterBuilder::EmplaceItem(Register &items, ItemPtr &item) {
	auto id = item->GetId();
	auto it = items.try_emplace(id, std::move(item));
	if (!it.second) {
		err_log("Item '%s' has already exist. Redundant definition had been ignored.\n",
				NAME_BY_ITEM<E>(id).c_str());
	}
}

template<typename E, typename I, typename B>
void InfoContainer<E, I, B>::RegisterBuilder::EmplaceDefaultItems(Register &items) {
	auto default_item = std::make_shared<I>();
	items.try_emplace(E::kUndefined, std::move(default_item));
}

} // info_container

#endif //BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
