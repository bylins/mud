/**
\file zone_reload.h
\brief Перечитывание одной зоны с диска на работающем сервере.

Нужно, когда файлы зоны правятся снаружи движка -- руками, скриптом, подтягиванием из git.
Правки через OLC сюда не относятся: они пишутся на диск сами и в памяти уже применены.

Разбор задачи, перечень шагов загрузки, которые перезагрузка обязана повторить, и план --
в issue #4201. Сделаны первые два шага:

  Inspect() -- разведка: читает зону с диска и сравнивает состав с памятью, мир не трогает.
  Apply()   -- перезагрузка, пока только когда состав не изменился: ни один рнум не двигается,
               содержимое прототипов переписывается на месте.

Изменившийся состав (шаг 3) -- это снос блока записей зоны, вставка нового и сдвиг хвоста на
дельту: зона занимает непрерывный отрезок рнумов, поэтому переотображение получается
кусочно-линейным. Пока такой случай Apply() отвергает.
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

	// Сколько комнат зона держит в памяти по своему диапазону внумов.
	int mem_rooms{0};

	// Заполняется только Apply(): сколько сущностей переписано.
	bool applied{false};
	int applied_rooms{0};
	int applied_mobs{0};
	int applied_objects{0};
	int applied_triggers{0};
	std::string timings;   // разбивка по этапам, заполняет Apply()

	bool Changed() const {
		return rooms.Changed() || mobs.Changed() || objects.Changed() || triggers.Changed();
	}
};

/**
 Читает зону с диска и сравнивает её состав с тем, что сейчас в памяти.
 Мир не меняется: прочитанное разбирается и освобождается на выходе.
*/
Report Inspect(ZoneVnum zone_vnum);

/**
 Перечитывает зону с диска и применяет её к миру.

 Работает, только если состав не изменился: тогда ни один рнум не двигается и переиндексация
 не нужна. Если состав изменился, возвращает отчёт с ok == false и ничего не делает.

 Зона перед заливкой освобождается от живых: мобы удаляются, предметы с земли тоже,
 игроков выносит в их точку входа (dungeons::ClearRoom), -- и в конце зона ресетится.
*/
Report Apply(ZoneVnum zone_vnum);

// Отчёт в текст для вывода богу в игре.
std::string Format(const Report &report);

}  // namespace zone_reload

#endif  // BYLINS_SRC_ENGINE_DB_ZONE_RELOAD_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
