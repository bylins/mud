/**
\file zone_reload.h
\brief Перечитывание одной зоны с диска на работающем сервере.

Нужно, когда файлы зоны правятся снаружи движка -- руками, скриптом, подтягиванием из git.
Правки через OLC сюда не относятся: они пишутся на диск сами и в памяти уже применены.

Разбор задачи, перечень шагов загрузки, которые перезагрузка обязана повторить, и план --
в issue #4201. Сейчас сделан только первый шаг: разведка, которая читает зону с диска и
сравнивает состав с памятью, НИЧЕГО в мире не меняя.
*/

#ifndef BYLINS_SRC_ENGINE_DB_ZONE_RELOAD_H_
#define BYLINS_SRC_ENGINE_DB_ZONE_RELOAD_H_

#include "engine/structs/structs.h"

#include <string>
#include <vector>

namespace zone_reload {

// Что изменилось по одному виду сущностей. Внумы, не рнумы: рнум после перезагрузки
// у половины зоны другой, а внум -- то, чем сущность названа и в файле, и в мире.
struct Diff {
	std::vector<int> added;      // есть в файле, нет в памяти
	std::vector<int> removed;    // есть в памяти, нет в файле
	std::vector<int> kept;       // есть и там, и там

	bool Changed() const { return !added.empty() || !removed.empty(); }
};

struct Report {
	ZoneVnum zone_vnum{0};
	ZoneRnum zone_rnum{kNoZone};
	bool ok{false};
	std::string error;           // заполнено, когда ok == false

	Diff rooms;
	Diff mobs;
	Diff objects;
	Diff triggers;

	// Сколько сущностей зона держит в памяти по границам zone_table. Считается отдельно
	// от Diff: расхождение между этим числом и kept + removed означает, что в диапазоне
	// зоны лежит что-то, чего мы не ждали.
	int mem_rooms{0};

	bool Changed() const {
		return rooms.Changed() || mobs.Changed() || objects.Changed() || triggers.Changed();
	}
};

/**
 Читает зону с диска и сравнивает её состав с тем, что сейчас в памяти.
 Мир не меняется: прочитанное разбирается и освобождается на выходе.

 \param zone_vnum внум зоны
 \return отчёт; при ok == false заполнено поле error
*/
Report Inspect(ZoneVnum zone_vnum);

// Отчёт в текст для вывода богу в игре.
std::string Format(const Report &report);

}  // namespace zone_reload

#endif  // BYLINS_SRC_ENGINE_DB_ZONE_RELOAD_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
