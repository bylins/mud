#include "zone_reload.h"

#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_data_source.h"
#include "engine/db/world_data_source_manager.h"
#include "engine/entities/char_data.h"
#include "engine/entities/room_data.h"
#include "engine/scripting/dg_scripts.h"

#include <fmt/format.h>

#include <algorithm>
#include <set>

namespace zone_reload {

namespace {

// Виртуальная комната X99. Её нет ни в одном файле мира (проверено по всем зонам): её
// добавляет каждой зоне AddVirtualRoomsToAllZones() при загрузке, а
// CalculateFirstAndLastRooms() тут же выбрасывает её из границ зоны (second--). Значит в
// сравнении «файл против памяти» она всегда выглядела бы удалённой -- и это был бы не
// результат правки, а наше собственное служебное хозяйство.
bool IsVirtualRoom(ZoneVnum zone_vnum, int room_vnum) {
	return room_vnum == zone_vnum * 100 + 99;
}

void FillDiff(const std::set<int> &disk, const std::set<int> &mem, Diff &out) {
	std::set_difference(disk.begin(), disk.end(), mem.begin(), mem.end(), std::back_inserter(out.added));
	std::set_difference(mem.begin(), mem.end(), disk.begin(), disk.end(), std::back_inserter(out.removed));
	std::set_intersection(disk.begin(), disk.end(), mem.begin(), mem.end(), std::back_inserter(out.kept));
}

// Внумы, которые зона держит в памяти. Перебираем диапазон зоны и спрашиваем рнум, а не
// идём по границам RnumRoomsLocation: границы считаны при загрузке и по ним не видно,
// какой внум лежит в каждой клетке, а нам нужно сравнивать именно внумы.
std::set<int> MemoryRooms(const ZoneData &zone) {
	std::set<int> out;
	for (int vnum = zone.vnum * 100; vnum <= zone.top; ++vnum) {
		if (IsVirtualRoom(zone.vnum, vnum)) {
			continue;
		}
		if (GetRoomRnum(vnum) > 0) {
			out.insert(vnum);
		}
	}
	return out;
}

std::set<int> MemoryMobs(const ZoneData &zone) {
	std::set<int> out;
	for (int vnum = zone.vnum * 100; vnum <= zone.top; ++vnum) {
		if (GetMobRnum(vnum) >= 0) {
			out.insert(vnum);
		}
	}
	return out;
}

std::set<int> MemoryObjects(const ZoneData &zone) {
	std::set<int> out;
	for (int vnum = zone.vnum * 100; vnum <= zone.top; ++vnum) {
		if (GetObjRnum(vnum) >= 0) {
			out.insert(vnum);
		}
	}
	return out;
}

std::set<int> MemoryTriggers(const ZoneData &zone) {
	std::set<int> out;
	for (int vnum = zone.vnum * 100; vnum <= zone.top; ++vnum) {
		if (GetTriggerRnum(vnum) >= 0) {
			out.insert(vnum);
		}
	}
	return out;
}

std::string FormatVnums(const std::vector<int> &vnums) {
	constexpr std::size_t kShow = 12;
	if (vnums.empty()) {
		return "";
	}
	std::string out;
	for (std::size_t i = 0; i < std::min(kShow, vnums.size()); ++i) {
		out += (i ? ", " : "") + std::to_string(vnums[i]);
	}
	if (vnums.size() > kShow) {
		out += fmt::format(" и ещё {}", vnums.size() - kShow);
	}
	return out;
}

std::string FormatDiff(const char *what, const Diff &diff) {
	if (!diff.Changed()) {
		return fmt::format("  {}: без изменений состава, {} шт.\r\n", what, diff.kept.size());
	}
	std::string out = fmt::format("  {}: было {}, станет {}\r\n",
								  what, diff.kept.size() + diff.removed.size(), diff.kept.size() + diff.added.size());
	if (!diff.added.empty()) {
		out += fmt::format("    добавится {}: {}\r\n", diff.added.size(), FormatVnums(diff.added));
	}
	if (!diff.removed.empty()) {
		out += fmt::format("    исчезнет {}: {}\r\n", diff.removed.size(), FormatVnums(diff.removed));
	}
	return out;
}

}  // namespace

Report Inspect(ZoneVnum zone_vnum) {
	Report report;
	report.zone_vnum = zone_vnum;

	const ZoneRnum zrn = GetZoneRnum(zone_vnum);
	if (zrn < 0) {
		report.error = fmt::format("нет зоны {}", zone_vnum);
		return report;
	}
	report.zone_rnum = zrn;

	auto *ds = world_loader::WorldDataSourceManager::Instance().GetDataSource();
	if (!ds) {
		report.error = "источник данных мира не зарегистрирован";
		return report;
	}
	// Легаси-источник грузит мир своим внутренним путём и сразу пишет в world[]/mob_proto,
	// так что отдать нам зону отдельными данными он не может -- и перечитать её нечем.
	if (!ds->SupportsZoneFilter()) {
		report.error = fmt::format("источник данных '{}' не умеет читать зону отдельно", ds->GetName());
		return report;
	}

	const std::vector<int> filter{static_cast<int>(zone_vnum)};

	// Читаем всё четырьмя вызовами, теми же, какими читает зону загрузка. Возвращаются
	// простые данные: ни world[], ни mob_proto, ни obj_proto, ни trig_index при этом не
	// трогаются -- записью занимается оркестратор в BootWorld, и мы его не зовём.
	auto disk_triggers = ds->LoadTriggers(&filter);
	auto disk_rooms = ds->LoadRooms(&filter);
	auto disk_mobs = ds->LoadMobs(&filter);
	auto disk_objects = ds->LoadObjects(&filter);

	std::set<int> dr, dm, dobj, dt;
	for (const auto &it : disk_rooms) {
		if (!IsVirtualRoom(zone_vnum, it.vnum)) {
			dr.insert(it.vnum);
		}
	}
	for (const auto &it : disk_mobs) {
		dm.insert(it.vnum);
	}
	for (const auto &it : disk_objects) {
		dobj.insert(it.vnum);
	}
	for (const auto &it : disk_triggers) {
		dt.insert(it.vnum);
	}

	const auto &zone = zone_table[zrn];
	const auto mr = MemoryRooms(zone);
	report.mem_rooms = static_cast<int>(mr.size());

	FillDiff(dr, mr, report.rooms);
	FillDiff(dm, MemoryMobs(zone), report.mobs);
	FillDiff(dobj, MemoryObjects(zone), report.objects);
	FillDiff(dt, MemoryTriggers(zone), report.triggers);

	// Прочитанное больше не нужно: разведка ничего не применяет. Прототипы предметов
	// держит shared_ptr и они уйдут сами, остальное создано через new в источнике.
	for (auto &it : disk_rooms) {
		delete it.room;
	}
	for (auto &it : disk_mobs) {
		delete it.mob;
	}
	for (auto &it : disk_triggers) {
		delete it.trig;
	}

	report.ok = true;
	return report;
}

std::string Format(const Report &report) {
	if (!report.ok) {
		return fmt::format("Перезагрузка зоны {}: {}.\r\n", report.zone_vnum, report.error);
	}

	std::string out = fmt::format("Зона {} ({}), комнат в памяти {}, внумы {}-{}.\r\n",
								  report.zone_vnum,
								  zone_table[report.zone_rnum].name,
								  report.mem_rooms,
								  report.zone_vnum * 100,
								  zone_table[report.zone_rnum].top);
	out += FormatDiff("комнаты ", report.rooms);
	out += FormatDiff("мобы    ", report.mobs);
	out += FormatDiff("предметы", report.objects);
	out += FormatDiff("триггеры", report.triggers);
	out += report.Changed()
		? "Состав зоны изменился: перезагрузка потребует переиндексации (issue #4201, шаг 3).\r\n"
		: "Состав совпадает: такую зону можно перезагрузить без переиндексации (issue #4201, шаг 2).\r\n";
	out += "Разведка: в мире ничего не изменено.\r\n";
	return out;
}

}  // namespace zone_reload

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
