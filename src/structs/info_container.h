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

/*template<class T>
bool IsInRange(const T &t) {
	return (t >= T::kFirst && t <= T::kLast);
}*/

template<class E>
class InfoContainer {
 public:
	InfoContainer() {
		if (!items_) {
			items_ = std::make_unique<Register>();
			items_->emplace(E::kUndefined, std::make_unique<Pair>(std::make_pair(false, Item())));
		}
	};
	InfoContainer(InfoContainer &s) = delete;
	void operator=(const InfoContainer &s) = delete;

	class ContainerBuilder;
	class Item;
	class ItemBuilder;

	/*
	 *  Получить элемент с указанным id или с id kUndefined.
	 */
	const Item &operator[](E id) const;

	/*
	 *  Инициализация. Для реинициализации используйте Reload();
	 */
	void Init();

	/*
	 *  Горячая перезагрузка. Позволяет перегрузить данные контейнера.
	 *  Rонфликты между новыми и старыми данными не отслеживаются.
	 */
	void Reload();

	/*
	 *  Id известен. Не гарантируется, что он означает корректный элемент.
	 */
	bool IsKnown(E id);

	/*
	 *  Id известен, определен, не отключен и лежит между kFirst и kLast.
	 */
	bool IsValid(E id);

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

	ContainerBuilder builder;

	RegisterPtr items_;

	bool IsInitizalized();
	bool IsEnabled(E id);

};

#endif //BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
