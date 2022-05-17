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
#include <map>

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
 * Базовый элемента контейнера. Хранимые в info_container элементы должны наследоваться от него.
 */
template<typename IdEnum>
class BaseItem {
	IdEnum id_{IdEnum::kUndefined};
	EItemMode mode_{EItemMode::kDisabled};

  public:
	BaseItem<IdEnum>() = default;
	BaseItem<IdEnum>(IdEnum id, EItemMode mode)
	    	: id_(id), mode_(mode) {};

	[[nodiscard]] EItemMode GetMode() const { return mode_; };
	[[nodiscard]] auto GetId() const { return id_; };
	/**
	 *  Элемент доступен либо тестируется.
	 */
	[[nodiscard]] bool IsValid() const { return (GetMode() > EItemMode::kFrozen); };
	/**
	 *  Элемент отключен или является служебным.
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

template<typename Item>
class IItemBuilder {
 public:
	using ItemPtr = std::shared_ptr<Item>;

	virtual ItemPtr Build(parser_wrapper::DataNode &node) = 0;
	static EItemMode ParseItemMode(parser_wrapper::DataNode &node, EItemMode default_mode);
};

template<typename IdEnum, typename Item, typename ItemBuilder>
class InfoContainer {
 public:
	InfoContainer();
	InfoContainer(InfoContainer &s) = delete;
	void operator=(const InfoContainer &s) = delete;

	using ItemPtr = std::shared_ptr<Item>;
	using Register = std::map<IdEnum, ItemPtr>;
	using NodeRange = iterators::Range<parser_wrapper::DataNode>;

/* ----------------------------------------------------------------------
 * 	Интерфейс контейнера.
 ---------------------------------------------------------------------- */
	/**
	 *  Возвращает элемент с указанным id или с id kUndefined.
	 */
	const Item &operator[](IdEnum id) const;
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
	bool IsKnown(const IdEnum id) const { return items_->contains(id); };
	/**
	 *  Id неизвестен.
	 */
	bool IsUnknown(const IdEnum id) const { return !IsKnown(id); };
	/**
	 *  Id доступен либо тестируется.
	 */
	bool IsValid(const IdEnum id) const { return (IsUnknown(id) ?
		false : items_->at(id)->GetMode() > EItemMode::kFrozen); };
	/**
	 *  Id отключен.
	 */
	bool IsInvalid(const IdEnum id) const { return !IsValid(id); };
	/**
	 *  Id доступен и не тестируется).
	 */
	bool IsAvailable(const IdEnum id) const { return (IsUnknown(id) ? false : IsEnabled(id)); };
	/**
	 *  Id недоступен (тестируется, служебный или отключен).
	 */
	bool IsUnavailable(const IdEnum id) const { return !IsAvailable(id); };
	/**
	 *  Контейнер уже инициализирован.
	 */
	bool IsInitizalized() { return (items_->size() > 1); }
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
	const Item &FindItem(int num) const;
	/**
	 * Найти подключенный элемент, соответствующий числу, скастованному в индекс элемента.
	 * @return - элемент или элемент kUndefined.
	 */
	const Item &FindAvailableItem(int num) const;

 private:
	friend class RegisterBuilder;
	using RegisterPtr = std::unique_ptr<Register>;

	/**
	 *  Класс строителя контейнера в целом.
	 */
	class RegisterBuilder {
	 public:
		static RegisterPtr Build(const NodeRange &data, bool stop_on_error);

	 private:
		static RegisterPtr Parse(const NodeRange &data, bool stop_on_error);
		static void EmplaceItem(Register &items, ItemPtr &item);
		static void EmplaceDefaultItems(Register &items);
	};

	RegisterPtr items_;

	/**
	 *  Такой id известен, но элемент отключен.
	 */
	bool IsDisabled(const IdEnum id) const { return items_->at(id)->GetMode() == EItemMode::kDisabled; }
	/**
	 *  Такой id известен и находится в режиме тестирования.
	 */
	bool IsBeingTesting(const IdEnum id) const { return items_->at(id)->GetMode() == EItemMode::kTesting; }
	/**
	 *  Такой id известен и элемент доступен. Не гарантируется, что элемент корректен.
	 */
	bool IsEnabled(const IdEnum id) const { return items_->at(id)->GetMode() == EItemMode::kEnabled; }

};

/* ----------------------------------------------------------------------
 * 	Реализация InfoContainer
 ---------------------------------------------------------------------- */

template<typename IdEnum, typename Item, typename ItemBuilder>
InfoContainer<IdEnum, Item, ItemBuilder>::InfoContainer() {
	if (!items_) {
		items_ = std::make_unique<Register>();
	}
}

template<typename IdEnum, typename Item, typename ItemBuilder>
void InfoContainer<IdEnum, Item, ItemBuilder>::Reload(const NodeRange &data) {
	auto new_items = std::move(RegisterBuilder::Build(data, true));
	if (new_items) {
		items_ = std::move(new_items);
	} else {
		err_log("Reloading was canceled - file damaged.");
	}
};

template<typename IdEnum, typename Item, typename ItemBuilder>
const Item &InfoContainer<IdEnum, Item, ItemBuilder>::operator[](IdEnum id) const {
	try {
		return *(items_->at(id));
	} catch (const std::out_of_range &) {
		//err_log("Incorrect id (%d) passed into %s.", to_underlying(id), typeid(this).name()); ABYRVALG
		return *(items_->at(IdEnum::kUndefined));
	}
};

template<typename IdEnum, typename Item, typename ItemBuilder>
void InfoContainer<IdEnum, Item, ItemBuilder>::Init(const NodeRange &data) {
	if (IsInitizalized()) {
		err_log("Don't try reinit containers. Use 'Reload()'.");
		return;
	}
	items_ = std::move(RegisterBuilder::Build(data, false));
};

template<typename IdEnum, typename Item, typename ItemBuilder>
auto InfoContainer<IdEnum, Item, ItemBuilder>::begin() const {
	iterators::ConstIterator<decltype(items_->begin()), Item> it(items_->begin());
	return it;
}

template<typename IdEnum, typename Item, typename ItemBuilder>
auto InfoContainer<IdEnum, Item, ItemBuilder>::end() const {
	iterators::ConstIterator<decltype(items_->end()), Item> it(items_->end());
	return it;
}

template<typename IdEnum, typename Item, typename ItemBuilder>
const Item &InfoContainer<IdEnum, Item, ItemBuilder>::FindItem(const int num) const {
	auto id  = static_cast<IdEnum>(num);
	if (IsKnown(id)) {
		return *(items_->at(id));
	} else {
		return *(items_->at(IdEnum::kUndefined));
	}
}

template<typename IdEnum, typename Item, typename ItemBuilder>
const Item &InfoContainer<IdEnum, Item, ItemBuilder>::FindAvailableItem(const int num) const {
	auto id  = static_cast<IdEnum>(num);
	if (IsAvailable(id)) {
		return *(items_->at(id));
	} else {
		return *(items_->at(IdEnum::kUndefined));
	}
}

/* ----------------------------------------------------------------------
 * 	Реализация RegisterBuilder
 ---------------------------------------------------------------------- */

template<typename Item>
EItemMode IItemBuilder<Item>::ParseItemMode(parser_wrapper::DataNode &node, EItemMode default_mode) {
	try {
		return parse::ReadAsConstant<EItemMode>(node.GetValue("mode"));
	} catch (std::exception &) {
		return default_mode;
	}
};

template<typename IdEnum, typename Item, typename ItemBuilder>
typename InfoContainer<IdEnum, Item, ItemBuilder>::RegisterPtr
	InfoContainer<IdEnum, Item, ItemBuilder>::RegisterBuilder::Build(const NodeRange &data, bool stop_on_error) {
	auto items = Parse(data, stop_on_error);
	if (items) {
		EmplaceDefaultItems(*items);
	}
	return items;
}

template<typename IdEnum, typename Item, typename ItemBuilder>
typename InfoContainer<IdEnum, Item, ItemBuilder>::RegisterPtr
	InfoContainer<IdEnum, Item, ItemBuilder>::RegisterBuilder::Parse(const NodeRange &data, bool stop_on_error) {
	auto items = std::make_unique<Register>();

	ItemBuilder builder;
	for (auto &node : data) {
		auto item = builder.Build(node);
		if (item) {
			EmplaceItem(*items, item);
		} else if (stop_on_error) {
			return nullptr;
		}
	}

	return items;
}

template<typename IdEnum, typename Item, typename ItemBuilder>
void InfoContainer<IdEnum, Item, ItemBuilder>::RegisterBuilder::EmplaceItem(Register &items, ItemPtr &item) {
	auto id = item->GetId();
	auto it = items.try_emplace(id, std::move(item));
	if (!it.second) {
		err_log("Item '%s' has already exist. Redundant definition had been ignored.",
				NAME_BY_ITEM<IdEnum>(id).c_str());
	}
}

template<typename IdEnum, typename Item, typename ItemBuilder>
void InfoContainer<IdEnum, Item, ItemBuilder>::RegisterBuilder::EmplaceDefaultItems(Register &items) {
	auto default_item = std::make_shared<Item>();
	items.try_emplace(IdEnum::kUndefined, std::move(default_item));
	//items.try_emplace(IdEnum::kUndefined, default_item); IdEnum::kUndefined, EItemMode::kService
}

/* ----------------------------------------------------------------------
 * 	Частичная специаизация контейнера для случая индексации целыми числами
 * 	Используется, когда необходима возможность добавлять новые элементы,
 * 	не индексируемые через зашитый в код enum.
 ---------------------------------------------------------------------- */
/*
template<>
class IItem<int> {
	[[nodiscard]] virtual EItemMode GetMode() const = 0;
	[[nodiscard]] virtual int GetId() const = 0;
};

template<typename I, typename B>
class InfoContainer<int, I, B> {
 public:
	//using ItemPtr = std::shared_ptr<I>;
	using ItemOptional = std::optional<I>;
	using Register = std::vector<I>;
	using NodeRange = iterators::Range<parser_wrapper::DataNode>;

	const I &operator[](int id) const;

 private:
	friend class RegisterBuilder;
	using RegisterPtr = std::unique_ptr<Register>;
	using RegisterOptional = std::optional<RegisterPtr>;

	class RegisterBuilder {
	 private:
		static void EmplaceDefaultItems(Register &items);
	};

	RegisterPtr items_;
	static const I default_item_ = I();
};


template<typename I, typename B>
const I &InfoContainer<int, I, B>::operator[](int id) const {
	try {
		return items_->at(id);
	} catch (const std::out_of_range &) {
		return default_item_;
	}
};

template<typename I, typename B>
void InfoContainer<int, I, B>::RegisterBuilder::EmplaceDefaultItems(Register &items) {
	//default_item_ = I();
}
*/
} // info_container

#endif //BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
