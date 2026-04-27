// Part of Bylins http://www.mud.ru
//
// Сохранение сундуков с ингредиентами всех кланов в файлы.
// Интерфейс минимален: все детали реализации (пул потоков, dirty-set,
// порядок обхода) инкапсулированы в Impl.

#ifndef INGR_CHEST_SAVER_H_
#define INGR_CHEST_SAVER_H_

#include <memory>

class Clan;

namespace ClanSystem {

class IngrChestSaver {
 public:
	IngrChestSaver();
	~IngrChestSaver();

	IngrChestSaver(const IngrChestSaver &) = delete;
	IngrChestSaver &operator=(const IngrChestSaver &) = delete;

	// Пометить сундук клана как изменённый с последнего сохранения.
	// Вызывается из put/take/purge_ingr_chest.
	void mark_dirty(Clan *clan);

	// Пройти по Clan::ClanList и сохранить помеченные сундуки в
	// параллельном пуле потоков. Логирует результат каждого сундука
	// и общий wall time.
	void run();

 private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};

}  // namespace ClanSystem

#endif  // INGR_CHEST_SAVER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
