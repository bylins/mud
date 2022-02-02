/**
\authors Created by Sventovit
\date 2.02.2022.
\brief Универсальный, елико возможно, контейнер для хранения информации об игровых сущнсотях типа скиллов и классов.
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

template<class E, class I, class B>
class InfoContainer {
 public:
	InfoContainer() {
		if (!items_) {
			items_ = std::make_unique<Register>();
			items_->emplace(E::kUndefined, std::make_unique<Pair>(std::make_pair(false, I())));
		}
	};
	InfoContainer(InfoContainer &s) = delete;
	void operator=(const InfoContainer &s) = delete;

	/*
	 *  Получить элемент с указанным id или с id kUndefined.
	 */
	const I &operator[](E id) const;

	/*
	 *  Инициализация. Для реинициализации используйте Reload();
	 */
	void Init();

	/*
	 *  Горячая перезагрузка. Позволяет перегрузить данные контейнера.
	 *  Разрешение конфликтов между новыми и старыми данными - на советси пользователя.
	 */
	void Reload();

	/*
	 *  Id известен. Не гарантируется, что он означает корректный элемент.
	 */
	bool IsKnown(E id);

	/*
	 *  Id известен и окорректен, т.е. определен, лежит между первым и последним элементом
	 *  и не откллючен.
	 */
	bool IsValid(E id);

	/*
	 *  Id не определен, отключен или лежит вне корректного диапазона.
	 */
	bool IsInvalid(E id) { return !IsValid(id); };

 private:
	using Pair = std::pair<bool, I>;
	using PairPtr = std::unique_ptr<Pair>;
	using Register = std::unordered_map<E, PairPtr>;
	using RegisterPtr = std::unique_ptr<Register>;
//	using RegisterOptional = std::optional<RegisterPtr>;

	RegisterPtr items_;

	bool IsInitizalized();
	bool IsEnabled(E id);

};

#endif //BYLINS_SRC_STRUCTS_INFO_CONTAINER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
