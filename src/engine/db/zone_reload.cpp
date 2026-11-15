#include "zone_reload.h"

#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_data_source.h"
#include "engine/db/world_data_source_base.h"
#include "engine/db/world_data_source_manager.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "engine/olc/olc.h"
#include "engine/scripting/dg_scripts.h"

#include "gameplay/ai/spec_assign.h"
#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/mobmax.h"
#include "gameplay/mechanics/obj_sets_stuff.h"
#include "gameplay/mechanics/rune_stones.h"
#include "gameplay/mechanics/stuff.h"

#include <fmt/format.h>

#include <algorithm>
#include <set>

// Эти функции живут в OLC и в db.cpp и в заголовки не вынесены. Перезагрузка зоны делает с
// каждой сущностью ровно то же, что OLC делает при сохранении уже существующей: именно эти
// функции и отлажены на том, чтобы подменить содержимое прототипа, не оборвав живые ссылки.
extern void CopyRoom(RoomData *dst, RoomData *src);
extern void medit_mobile_free(CharData *mob);
extern void CopyMobilePrototypeForMedit(CharData *dst, CharData *src, bool partial_copy);
extern void olc_update_objects(int robj_num, ObjData *olc_obj);
extern void RestoreRoomExitData(RoomRnum rrn);
extern void IndexZoneObjLoads();
extern void SetZoneRnumForObjects();
extern void SetZoneRnumForMobiles();
extern void SetZoneRnumForTriggers();
extern void ConvertObjValues();

namespace zone_reload {

namespace {

// Виртуальная комната X99. Её нет ни в одном файле мира: её добавляет каждой зоне
// AddVirtualRoomsToAllZones() при загрузке, а CalculateFirstAndLastRooms() тут же выбрасывает
// её из границ зоны (second--). В сравнении «файл против памяти» она всегда выглядела бы
// удалённой -- и это было бы не результатом правки, а нашим служебным хозяйством.
bool IsVirtualRoom(ZoneVnum zone_vnum, int room_vnum) {
	return room_vnum == zone_vnum * 100 + 99;
}

// Прочитанное с диска. Держим в одном месте, потому что разведке и перезагрузке нужно одно
// и то же чтение, а освобождать прочитанное надо в обоих случаях -- просто в разный момент.
struct DiskZone {
	std::vector<world_loader::LoadedTrigger> triggers;
	std::vector<world_loader::LoadedRoom> rooms;
	std::vector<world_loader::LoadedMob> mobs;
	std::vector<world_loader::LoadedObject> objects;

	std::set<int> trigger_vnums;
	std::set<int> room_vnums;
	std::set<int> mob_vnums;
	std::set<int> object_vnums;
};

// Освобождает то, что не ушло в мир. Прототипы предметов держит shared_ptr и они уйдут сами;
// остальное источник создал через new.
void FreeUnused(DiskZone &disk, bool rooms_taken, bool mobs_taken, bool triggers_taken) {
	if (!rooms_taken) {
		for (auto &it : disk.rooms) {
			delete it.room;
		}
	}
	if (!mobs_taken) {
		for (auto &it : disk.mobs) {
			delete it.mob;
		}
	}
	if (!triggers_taken) {
		for (auto &it : disk.triggers) {
			delete it.trig;
		}
	}
	disk.rooms.clear();
	disk.mobs.clear();
	disk.triggers.clear();
	disk.objects.clear();
}

bool ReadZone(ZoneVnum zone_vnum, DiskZone &disk, std::string &error) {
	auto *ds = world_loader::WorldDataSourceManager::Instance().GetDataSource();
	if (!ds) {
		error = "источник данных мира не зарегистрирован";
		return false;
	}
	// Легаси-источник грузит мир своим внутренним путём и сразу пишет в world[]/mob_proto,
	// так что отдать нам зону отдельными данными он не может -- и перечитать её нечем.
	if (!ds->SupportsZoneFilter()) {
		error = fmt::format("источник данных '{}' не умеет читать зону отдельно", ds->GetName());
		return false;
	}

	const std::vector<int> filter{static_cast<int>(zone_vnum)};

	// Те же четыре вызова, какими читает зону загрузка. Возвращаются простые данные: ни
	// world[], ни mob_proto, ни obj_proto, ни trig_index при этом не трогаются -- записью
	// занимается оркестратор в BootWorld, и мы его не зовём.
	disk.triggers = ds->LoadTriggers(&filter);
	disk.rooms = ds->LoadRooms(&filter);
	disk.mobs = ds->LoadMobs(&filter);
	disk.objects = ds->LoadObjects(&filter);

	for (const auto &it : disk.rooms) {
		if (!IsVirtualRoom(zone_vnum, it.vnum)) {
			disk.room_vnums.insert(it.vnum);
		}
	}
	for (const auto &it : disk.mobs) {
		disk.mob_vnums.insert(it.vnum);
	}
	for (const auto &it : disk.objects) {
		disk.object_vnums.insert(it.vnum);
	}
	for (const auto &it : disk.triggers) {
		disk.trigger_vnums.insert(it.vnum);
	}
	return true;
}

void FillDiff(const std::set<int> &disk, const std::set<int> &mem, Diff &out) {
	std::set_difference(disk.begin(), disk.end(), mem.begin(), mem.end(), std::back_inserter(out.added));
	std::set_difference(mem.begin(), mem.end(), disk.begin(), disk.end(), std::back_inserter(out.removed));
	std::set_intersection(disk.begin(), disk.end(), mem.begin(), mem.end(), std::back_inserter(out.kept));
}

// Внумы, которые зона держит в памяти. Перебираем диапазон зоны и спрашиваем рнум, а не идём
// по границам RnumRoomsLocation: границы говорят, где зона лежит, но не какой внум в каждой
// клетке, а сравнивать надо именно внумы.
std::set<int> MemoryRooms(const ZoneData &zone) {
	std::set<int> out;
	for (int vnum = zone.vnum * 100; vnum <= zone.top; ++vnum) {
		if (!IsVirtualRoom(zone.vnum, vnum) && GetRoomRnum(vnum) > 0) {
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

void FillReport(const ZoneData &zone, const DiskZone &disk, Report &report) {
	const auto mem_rooms = MemoryRooms(zone);
	report.mem_rooms = static_cast<int>(mem_rooms.size());
	FillDiff(disk.room_vnums, mem_rooms, report.rooms);
	FillDiff(disk.mob_vnums, MemoryMobs(zone), report.mobs);
	FillDiff(disk.object_vnums, MemoryObjects(zone), report.objects);
	FillDiff(disk.trigger_vnums, MemoryTriggers(zone), report.triggers);
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

// --- применение ---------------------------------------------------------------------------

// Освобождает зону от живых ровно так, как это делает сброс данжа: мобы удаляются, предметы
// с земли тоже, игроков выносит в их точку входа. Делать это надо до подмены прототипов:
// живой моб ссылается на строки своего прототипа, и если прототип переписать под ним, он
// останется со строками, которых больше нет.
void EvictZone(const ZoneData &zone) {
	for (int vnum = zone.vnum * 100; vnum <= zone.top; ++vnum) {
		const auto rrn = GetRoomRnum(vnum);
		if (rrn > 0) {
			dungeons::ClearRoom(world[rrn]);
		}
	}
}

void ApplyTriggers(DiskZone &disk, Report &report) {
	for (auto &it : disk.triggers) {
		const int rnum = GetTriggerRnum(it.vnum);
		if (rnum < 0 || !trig_index[rnum]) {
			continue;
		}
		// Рнум прототипа выставляет загрузка (LoadTriggersUnified делает set_rnum перед
		// CreateTriggerIndex), и он же попадает в каждую копию, которую раздаёт
		// read_trigger(). Без него ExtractTrigger лезет в trig_index по мусорному индексу
		// и роняет сервер на первом же ресете зоны.
		it.trig->set_rnum(rnum);
		// Прикреплённым скриптам подмена не вредит: read_trigger() отдаёт им копию
		// (new Trigger(*index->proto)), а не сам прототип.
		delete trig_index[rnum]->proto;
		trig_index[rnum]->proto = it.trig;
		// Индекс 'vnum trig' -- какие предметы грузит этот триггер. Записи старого тела
		// снимаем, записи нового добавляем: иначе в выдаче останутся предметы, которых
		// в скрипте уже нет.
		for (auto &entry : GlobalObjects::obj_triggers()) {
			entry.second.remove(it.vnum);
		}
		IndexTriggerObjLoads(it.vnum, it.trig);
		it.trig = nullptr;
		++report.applied_triggers;
	}
}

void ApplyRooms(ZoneVnum zone_vnum, DiskZone &disk, Report &report) {
	for (auto &it : disk.rooms) {
		if (IsVirtualRoom(zone_vnum, it.vnum)) {
			continue;
		}
		const auto rrn = GetRoomRnum(it.vnum);
		if (rrn <= 0) {
			continue;
		}
		// Ловушка снимается до подмены: deathtrap держит указатель на RoomData, а флаги
		// комнаты сейчас сменятся, и по новым флагам список уже не сойдётся со старым.
		deathtrap::remove(world[rrn]);

		CleanupRoomData(world[rrn]);
		// CopyRoom бережёт то, что принадлежит не файлу, а текущему состоянию мира:
		// people, contents, track и аффекты комнаты.
		CopyRoom(world[rrn], it.room);

		// Выходы пришли из файла внумами -- тем же порядком, что на загрузке, переводим их
		// в рнумы (RosolveWorldDoorToRoomVnumsToRnums делает это для всего мира сразу).
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
			auto &exit = world[rrn]->dir_option_proto[dir];
			if (exit && exit->to_room() != kNowhere) {
				exit->to_room(GetRoomRnum(exit->to_room()));
			}
		}
		RestoreRoomExitData(rrn);
		CheckRoomForIncompatibleFlags(rrn);

		SCRIPT(world[rrn])->cleanup();
		for (int trigger_vnum : it.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToRoom(rrn, trigger_vnum, it.vnum);
		}
		assign_triggers(world[rrn], WLD_TRIGGER);

		if (ROOM_FLAGGED(rrn, ERoomFlag::kSlowDeathTrap) || ROOM_FLAGGED(rrn, ERoomFlag::kIceTrap)) {
			deathtrap::add(world[rrn]);
		}

		CleanupRoomData(it.room);
		delete it.room;
		it.room = nullptr;
		++report.applied_rooms;
	}
}

void ApplyMobs(DiskZone &disk, Report &report) {
	for (auto &it : disk.mobs) {
		const auto rnum = GetMobRnum(it.vnum);
		if (rnum < 0) {
			continue;
		}
		medit_mobile_free(&mob_proto[rnum]);
		CopyMobilePrototypeForMedit(&mob_proto[rnum], it.mob, false);
		mob_proto[rnum].set_rnum(rnum);

		// Триггеры прототипа: список приходит из файла, старый уже освобождён выше.
		for (int trigger_vnum : it.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToMob(rnum, trigger_vnum, it.vnum);
		}

		medit_mobile_free(it.mob);
		delete it.mob;
		it.mob = nullptr;
		++report.applied_mobs;
	}
}

void ApplyObjects(DiskZone &disk, Report &report) {
	for (auto &it : disk.objects) {
		const auto rnum = GetObjRnum(it.vnum);
		if (rnum < 0 || !it.obj) {
			continue;
		}
		// Источник отдаёт прототип как CObjectPrototype и держит его shared_ptr'ом, а
		// set_rnum() забирает сырой указатель во владение (m_prototypes[i].reset(p)). Отдать
		// туда it.obj.get() значило бы удалить объект дважды, поэтому кладём в таблицу свою
		// копию -- заодно она и нужного типа ObjData, как во всём остальном движке.
		auto *proto = new ObjData(*it.obj);
		// Сначала живые экземпляры: они держат строки прототипа, поэтому их надо перевести на
		// новый до того, как старый уйдёт. Заодно сюда входят склад и посылки.
		olc_update_objects(rnum, proto);
		obj_proto.set_rnum(rnum, proto);

		for (int trigger_vnum : it.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToObject(rnum, trigger_vnum, it.vnum);
		}
		++report.applied_objects;
	}
}

// Шаги загрузки, которые пришивают содержимое к миру. Порядок тот же, в каком их делает
// BootWorld и BootMudDataBase; перечень и обоснование -- в issue #4201.
void Rewire(ZoneRnum zrn) {
	// Значения свитков, палочек и напитков из старого формата -- миграция идемпотентна.
	ConvertObjValues();

	// Зонные рнумы в индексах и таблица «предмет -> где он грузится ресетами».
	SetZoneRnumForObjects();
	SetZoneRnumForMobiles();
	SetZoneRnumForTriggers();
	IndexZoneObjLoads();

	// Таблицы, которые строятся по прототипам. Все они -- полные пересборы.
	oload_table.init();        // выпадение с мобов, хранит ObjRnum
	system_obj::init();        // кошелёк и персональный сундук, хранят ObjRnum
	obj_sets::init_obj_index();  // set_idx по прототипам

	// Спецпроцедуры и лавки: func/farg живут в индексе и затираются подменой прототипа.
	ReloadSpecProcs();

	// Рунные камни -- физические предметы в комнатах, зона могла их потерять или получить.
	MUD::Runestones().SpawnStones();

	// Счётчик мобов по уровням и список ловушек -- оба полные пересборы.
	MobMax::init();
	deathtrap::load();

	UniqueList<ZoneRnum> repop_list;
	repop_list.push_back(zrn);
	DecayObjectsOnRepop(repop_list);
	ResetZone(zrn);
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

	DiskZone disk;
	if (!ReadZone(zone_vnum, disk, report.error)) {
		return report;
	}
	FillReport(zone_table[zrn], disk, report);
	FreeUnused(disk, false, false, false);

	report.ok = true;
	return report;
}

Report Apply(ZoneVnum zone_vnum) {
	Report report;
	report.zone_vnum = zone_vnum;

	const ZoneRnum zrn = GetZoneRnum(zone_vnum);
	if (zrn < 0) {
		report.error = fmt::format("нет зоны {}", zone_vnum);
		return report;
	}
	report.zone_rnum = zrn;

	DiskZone disk;
	if (!ReadZone(zone_vnum, disk, report.error)) {
		return report;
	}
	FillReport(zone_table[zrn], disk, report);

	// Состав изменился -- значит рнумы поедут, а за ними весь мир: выходы, ресеты всех зон,
	// in_room живых, границы зон. Это шаг 3 из issue #4201, его ещё нет.
	if (report.Changed()) {
		FreeUnused(disk, false, false, false);
		report.ok = true;
		return report;
	}

	log("[ZoneReload] перезагружаю зону %d (%s)", zone_vnum, zone_table[zrn].name.c_str());
	EvictZone(zone_table[zrn]);

	// Триггеры первыми: комнаты, мобы и предметы тут же навесят их на себя по внумам.
	ApplyTriggers(disk, report);
	ApplyRooms(zone_vnum, disk, report);
	ApplyMobs(disk, report);
	ApplyObjects(disk, report);
	FreeUnused(disk, true, true, true);

	Rewire(zrn);

	mudlog(fmt::format("Зона {} перечитана с диска: комнат {}, мобов {}, предметов {}, триггеров {}.",
					   zone_vnum, report.applied_rooms, report.applied_mobs,
					   report.applied_objects, report.applied_triggers),
		   NRM, kLvlGreatGod, SYSLOG, true);

	report.applied = true;
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

	if (report.applied) {
		out += fmt::format("Перезагружено: комнат {}, мобов {}, предметов {}, триггеров {}. Зона сброшена.\r\n",
						   report.applied_rooms, report.applied_mobs,
						   report.applied_objects, report.applied_triggers);
	} else if (report.Changed()) {
		out += "Состав зоны изменился -- поедут рнумы, а с ними выходы, ресеты всех зон и границы.\r\n"
			   "Перезагрузка такой зоны (issue #4201, шаг 3) пока не сделана, в мире ничего не изменено.\r\n";
	} else {
		out += "Состав совпадает. Разведка: в мире ничего не изменено.\r\n";
	}
	return out;
}

}  // namespace zone_reload

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
