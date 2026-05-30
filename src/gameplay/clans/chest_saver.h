// Part of Bylins http://www.mud.ru
//
// Сохранение основных клановых сундуков в файлы.
// Полный аналог IngrChestSaver для .obj-файлов.

#ifndef CHEST_SAVER_H_
#define CHEST_SAVER_H_

#include <memory>

class Clan;

namespace ClanSystem {

class ChestSaver {
 public:
	ChestSaver();
	~ChestSaver();

	ChestSaver(const ChestSaver &) = delete;
	ChestSaver &operator=(const ChestSaver &) = delete;

	// Пометить сундук клана как изменённый с последнего сохранения.
	// Вызывается из PutChest, TakeChest, ChestUpdate (при пурже).
	void mark_dirty(Clan *clan);

	// Пройти по Clan::ClanList и сохранить помеченные сундуки в
	// параллельном пуле потоков.
	void run();

 private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};

}  // namespace ClanSystem

#endif  // CHEST_SAVER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
