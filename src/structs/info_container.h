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
#include <typeinfo>

#include "utils/logger.h"
#include "utils/wrapper.h"

enum class EItemMode {
	kDisabled = 0,
	kTesting,
	kEnabled
};

template<>
const std::string &NAME_BY_ITEM<EItemMode>(EItemMode item);
template<>
EItemMode ITEM_BY_NAME<EItemMode>(const std::string &name);

template<typename T, typename I>
struct PredicateIterator {
 private:
	T it_;
	//using I = decltype(*it_->second);
 public:
	using iterator_category = std::input_iterator_tag;
	using difference_type   = std::ptrdiff_t;
	using value_type        = I;
	using pointer           = const I*;
	using reference         = const I&;

	PredicateIterator() = default;
	PredicateIterator(const PredicateIterator &p) = default;
	explicit PredicateIterator(T it) :
		it_{it} {};

	PredicateIterator &operator++() {
		++it_;
		return *this;
	};

	const PredicateIterator operator++(int) {
		auto retval = *this;
		++*this;
		return retval;
	};

	bool operator==(const PredicateIterator &it) const { return it_ == it.it_; };

	bool operator!=(const PredicateIterator &other) const { return !(*this == other); };

	reference operator*() const { return *(it_->second); }

	pointer operator->() { return it_->second.get(); }
};

namespace info_container {

/*
 * Интерфейсы для элемента.
 */
template<typename E>
class IItem {
 public:
	virtual EItemMode GetMode() = 0;
	virtual E GetId() = 0;
};

template<typename I>
class IItemBuilder {
 public:
	using ItemPtr = std::shared_ptr<I>;
	using ItemOptional = std::optional<ItemPtr>;
	virtual ItemOptional Build(parser_wrapper::DataNode &node) = 0;
};

template<typename E, typename I, typename B>
class InfoContainer {
 public:
	InfoContainer() {
		if (!items_) {
			items_ = std::make_unique<Register>();
		}
	}
	InfoContainer(InfoContainer &s) = delete;
	void operator=(const InfoContainer &s) = delete;

	using ItemPtr = std::shared_ptr<I>;
	using ItemOptional = std::optional<ItemPtr>;
	using Register = std::unordered_map<E, ItemPtr>;
	using NodeRange = parser_wrapper::NodeRange<parser_wrapper::DataNode>;

/* ----------------------------------------------------------------------
 * 	Интерфейс контейнера.
 ---------------------------------------------------------------------- */

	/*
	 *  Возвращает элемент с указанным id или с id kUndefined.
	 */
	const I &operator[](E id) const {
		try {
			return *(items_->at(id));
		} catch (const std::out_of_range &) {
			err_log("Incorrect id (%d) passed into %s.", to_underlying(id), typeid(this).name());
			return *(items_->at(E::kUndefined));
		}
	};

	/*
	 *  Инициализация. Для реинициализации используйте Reload();
	 */
	void Init(const NodeRange &data) {
		if (IsInitizalized()) {
			err_log("Don't try re-init containers. Use 'Reload()'.");
			return;
		}
		items_ = std::move(RegisterBuilder::TolerantBuild(data).value());
	};

	/*
	 *  Горячая перезагрузка. Позволяет перегрузить данные контейнера.
	 *  Конфликты между новыми и старыми данными не отслеживаются.
	 */
	void Reload(const NodeRange &data) {
		auto new_items = RegisterBuilder::StrictBuild(data);
		if (new_items) {
			items_ = std::move(new_items.value());
		} else {
			err_log("Reloading canceled - file damaged.");
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
	 *  Id известен и доступен либо тестируется.
	 */
	bool IsValid(E id) {
		return (IsKnown(id) ? (IsEnabled(id) || IsBeingTesting(id)) : false);
	};

	/*
	 *  Id неизвестен или отключен.
	 */
	bool IsInvalid(E id) { return !IsValid(id); };

	/*
	 *  Id известен, определен и доступен (не тестируется).
	 */
	bool IsAvailable(E id) {
		return (IsKnown(id) ? IsEnabled(id) : false);
	};

	/*
	 *  Id неизвестен, неопределен или тестируется.
	 */
	bool IsUnavailable(E id) { return !IsAvailable(id); };

	/*
	 *  Контейнер уже инициализирован.
	 */
	bool IsInitizalized() { return (items_->size() > 1); }


	/*
	 *  Создание опционала для парсера элементов.
	 */
	ItemOptional MakeItemOptional() {
		return std::make_optional(std::make_shared<I>());
	}

	auto begin() {
		PredicateIterator<decltype(items_->begin()), I> it(items_->begin());
		return it;
	}

	auto end() {
		PredicateIterator<decltype(items_->end()), I> it(items_->end());
		return it;
	}

 private:
	friend class RegisterBuilder;
	using RegisterPtr = std::unique_ptr<Register>;
	using RegisterOptional = std::optional<RegisterPtr>;

	/*
	 *  Класс строителя контейнера в целом.
	 */
	class RegisterBuilder {
	 public:
		static RegisterOptional StrictBuild(const NodeRange &data) {
			strict_pasring_ = true;
			return Build(data);
		}

		static RegisterOptional TolerantBuild(const NodeRange &data) {
			strict_pasring_ = false;
			return Build(data);
		}

	 private:
		static bool strict_pasring_;
		static B item_builder_;

		static RegisterOptional Build(const NodeRange &data) {
			if (data.begin().IsEmpty()) {
				return std::nullopt;
			}
			return Parse(data);
		}

		static RegisterOptional Parse(const NodeRange &data) {
			auto items = std::make_optional(std::make_unique<Register>());
			ItemOptional item;
			for (auto &node : data) {
				item = item_builder_.Build(node);
				if (item) {
					EmplaceItem(*items.value(), item.value());
				} else if (strict_pasring_) {
					return std::nullopt;
				}
			}
			EmplaceDefaultItems(*items.value());

			return items;
		}

		static void EmplaceItem(Register &items, ItemPtr &item) {
			auto id = item->GetId();
			auto it = items.try_emplace(id, std::move(item));
			if (!it.second) {
				err_log("Item '%s' has already exist. Redundant definition had been ignored.\n",
						NAME_BY_ITEM<E>(id).c_str());
			}
		}

		static void EmplaceDefaultItems(Register &items) {
			items.try_emplace(E::kUndefined, std::make_shared<I>());
		}

	};

	//RegisterBuilder builder_;
	RegisterPtr items_;

	/*
	 *  Такой id известен, но элемент отключен.
	 */
	bool IsDisabled(const E id) { return items_->at(id)->GetMode() == EItemMode::kDisabled; }

	/*
	 *  Такой id известен и находится в режиме тестирования.
	 */
	bool IsBeingTesting(const E id) { return items_->at(id)->GetMode() == EItemMode::kTesting; }

	/*
	 *  Такой id известен и элемент доступен. Не гарантируется, что элемент корректен.
	 */
	bool IsEnabled(const E id) { return items_->at(id)->GetMode() == EItemMode::kEnabled; }

};

template<typename E, typename I, typename B> B InfoContainer<E, I, B>::RegisterBuilder::item_builder_;
template<typename E, typename I, typename B> bool InfoContainer<E, I, B>::RegisterBuilder::strict_pasring_;

} // info_container

#endif //BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
