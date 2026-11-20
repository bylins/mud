#include "zone_reload.h"

#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"
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
#include "gameplay/clans/house.h"
#include "gameplay/affects/room_affects.h"

#include "utils/utils_time.h"

#include <fmt/format.h>

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

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
extern void CalculateFirstAndLastRooms();
extern void CalculateFirstAndLastMobs();
extern ZoneRnum get_zone_rnum_by_obj_vnum(ObjVnum vnum);
extern RoomRnum r_frozen_start_room;

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

// --- переиндексация, когда состав изменился (issue #4201, шаг 3) ---------------------------

// Зона занимает непрерывный отрезок рнумов: таблицы отсортированы по внуму (поиск по ним
// двоичный), а внумы зоны -- сплошной блок N*100..top. Поэтому переотображение получается
// кусочно-линейным: до блока ничего не меняется, внутри ищем по внуму, за блоком -- сдвиг на
// дельту. Ни одного прохода «вставили одну запись, обошли весь мир» -- он ровно один.
struct Remap {
	int begin{0};                         // первый рнум блока зоны (в старой нумерации)
	int end{0};                           // за последним рнумом блока (в старой нумерации)
	int delta{0};                         // насколько изменилось число записей в блоке
	std::unordered_map<int, int> inside;  // старый рнум -> новый, для уцелевших записей

	int operator()(int rnum) const {
		if (rnum < begin) {
			return rnum;
		}
		if (rnum >= end) {
			return rnum + delta;
		}
		const auto it = inside.find(rnum);
		return it == inside.end() ? kNowhere : it->second;
	}
};

// Границы блока зоны в таблице: от первого рнума с внумом зоны до последнего.
template<typename VnumAt>
void FindBlock(int first_rnum, int last_rnum, int lo_vnum, int hi_vnum, VnumAt vnum_at,
			   int &block_begin, int &block_end) {
	block_begin = -1;
	block_end = -1;
	for (int rnum = first_rnum; rnum <= last_rnum; ++rnum) {
		const int vnum = vnum_at(rnum);
		if (vnum >= lo_vnum && vnum <= hi_vnum) {
			if (block_begin < 0) {
				block_begin = rnum;
			}
			block_end = rnum + 1;
		} else if (block_begin >= 0) {
			break;
		}
	}
	if (block_begin < 0) {
		// Зона в таблице ещё не представлена -- вставляем блок туда, где он должен стоять
		// по порядку внумов.
		block_begin = last_rnum + 1;
		for (int rnum = first_rnum; rnum <= last_rnum; ++rnum) {
			if (vnum_at(rnum) > hi_vnum) {
				block_begin = rnum;
				break;
			}
		}
		block_end = block_begin;
	}
}

Remap RebuildRooms(const ZoneData &zone, DiskZone &disk, Report &report) {
	Remap map;
	const int lo = zone.vnum * 100;
	const int hi = zone.top;
	FindBlock(kFirstRoom, top_of_world, lo, hi,
			  [](int rnum) { return static_cast<int>(world[rnum]->vnum); }, map.begin, map.end);

	// Что было: внум -> рнум. Объекты комнат переиспользуем, а не пересоздаём: на RoomData
	// держат указатели список ловушек и список зачарованных комнат, и если объект уцелел,
	// их трогать не придётся.
	std::map<int, RoomData *> old_rooms;
	std::map<int, int> old_rnum;
	for (int rnum = map.begin; rnum < map.end; ++rnum) {
		old_rooms[world[rnum]->vnum] = world[rnum];
		old_rnum[world[rnum]->vnum] = rnum;
	}

	// Что станет: комнаты из файла плюс виртуальная X99, если она была. В файле её нет
	// никогда -- её ставит AddVirtualRoomsToAllZones, и при перезагрузке она просто
	// остаётся тем же объектом на своём месте в порядке внумов.
	std::map<int, RoomData *> block;
	for (auto &it : disk.rooms) {
		if (IsVirtualRoom(zone.vnum, it.vnum)) {
			continue;
		}
		auto found = old_rooms.find(it.vnum);
		RoomData *room = (found != old_rooms.end()) ? found->second : new RoomData;
		if (found != old_rooms.end()) {
			deathtrap::remove(room);
			CleanupRoomData(room);
		}
		CopyRoom(room, it.room);
		room->vnum = it.vnum;
		room->zone_rn = GetZoneRnum(zone.vnum);
		block[it.vnum] = room;
		CleanupRoomData(it.room);
		delete it.room;
		it.room = nullptr;
		++report.applied_rooms;
	}
	const int virtual_vnum = zone.vnum * 100 + 99;
	if (old_rooms.count(virtual_vnum)) {
		block[virtual_vnum] = old_rooms[virtual_vnum];
	}

	// Комнаты, которых в файле больше нет: выносим живых, снимаем ловушку и чары, удаляем.
	for (auto &[vnum, room] : old_rooms) {
		if (block.count(vnum)) {
			continue;
		}
		dungeons::ClearRoom(room);
		deathtrap::remove(room);
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
			room_spells::ClearExitAffects(room, dir);
		}
		CleanupRoomData(room);
		delete room;
	}

	// Собираем таблицу заново: голова, новый блок, хвост.
	std::vector<RoomData *> rebuilt;
	rebuilt.reserve(world.size() + block.size());
	for (int rnum = 0; rnum < map.begin; ++rnum) {
		rebuilt.push_back(world[rnum]);
	}
	int pos = map.begin;
	for (auto &[vnum, room] : block) {
		rebuilt.push_back(room);
		if (old_rnum.count(vnum)) {
			map.inside[old_rnum[vnum]] = pos;
		}
		++pos;
	}
	for (int rnum = map.end; rnum <= top_of_world; ++rnum) {
		rebuilt.push_back(world[rnum]);
	}
	map.delta = static_cast<int>(block.size()) - (map.end - map.begin);

	world.clear();
	for (auto *room : rebuilt) {
		world.push_back(room);
	}
	top_of_world = static_cast<RoomRnum>(world.size() - 1);
	return map;
}

Remap RebuildMobs(const ZoneData &zone, DiskZone &disk, Report &report) {
	Remap map;
	const int lo = zone.vnum * 100;
	const int hi = zone.top;
	FindBlock(0, top_of_mobt, lo, hi,
			  [](int rnum) { return static_cast<int>(mob_index[rnum].vnum); }, map.begin, map.end);

	std::map<int, int> old_rnum;
	for (int rnum = map.begin; rnum < map.end; ++rnum) {
		old_rnum[mob_index[rnum].vnum] = rnum;
	}
	// Старые прототипы блока уходят целиком: для уцелевших внумов тело всё равно берётся из
	// файла, а для исчезнувших прототипа больше не будет.
	for (int rnum = map.begin; rnum < map.end; ++rnum) {
		medit_mobile_free(&mob_proto[rnum]);
	}

	std::map<int, world_loader::LoadedMob *> block;
	for (auto &it : disk.mobs) {
		block[it.vnum] = &it;
	}
	const int new_count = static_cast<int>(block.size());
	map.delta = new_count - (map.end - map.begin);

	const int new_top = top_of_mobt + map.delta;
	auto *new_proto = new CharData[new_top + 1];
	IndexData *new_index = nullptr;
	CREATE(new_index, new_top + 1);

	// Перенос только через CopyMobilePrototypeForMedit: у CharData оператор присваивания
	// копирует ОДИН лишь рнум (ProtectedCharData::operator=, char_data.cpp:76), так что
	// new_proto[i] = mob_proto[i] оставило бы в таблице пустых безымянных мобов.
	for (int rnum = 0; rnum < map.begin; ++rnum) {
		CopyMobilePrototypeForMedit(&new_proto[rnum], &mob_proto[rnum], false);
		new_index[rnum] = mob_index[rnum];
		new_proto[rnum].set_rnum(rnum);
	}
	int pos = map.begin;
	for (auto &[vnum, loaded] : block) {
		CopyMobilePrototypeForMedit(&new_proto[pos], loaded->mob, false);
		new_proto[pos].set_rnum(pos);
		new_index[pos].vnum = vnum;
		new_index[pos].total_online = 0;
		new_index[pos].func = nullptr;
		new_index[pos].set_idx = -1;
		if (old_rnum.count(vnum)) {
			new_index[pos].stored = mob_index[old_rnum[vnum]].stored;
			map.inside[old_rnum[vnum]] = pos;
		}
		medit_mobile_free(loaded->mob);
		delete loaded->mob;
		loaded->mob = nullptr;
		++report.applied_mobs;
		++pos;
	}
	for (int rnum = map.end; rnum <= top_of_mobt; ++rnum) {
		CopyMobilePrototypeForMedit(&new_proto[pos], &mob_proto[rnum], false);
		new_index[pos] = mob_index[rnum];
		new_proto[pos].set_rnum(pos);
		++pos;
	}

	delete[] mob_proto;
	free(mob_index);
	mob_proto = new_proto;
	mob_index = new_index;
	top_of_mobt = new_top;

	// Триггеры прототипов навешиваем после того, как таблица встала на место: AttachTriggerToMob
	// ищет моба по рнуму в новой таблице.
	for (auto &[vnum, loaded] : block) {
		const auto rnum = GetMobRnum(vnum);
		if (rnum < 0) {
			continue;
		}
		for (int trigger_vnum : loaded->triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToMob(rnum, trigger_vnum, vnum);
		}
	}
	return map;
}

Remap RebuildTriggers(const ZoneData &zone, DiskZone &disk, Report &report) {
	Remap map;
	const int lo = zone.vnum * 100;
	const int hi = zone.top;
	// top_of_trigt -- это количество триггеров, а не последний индекс.
	FindBlock(0, top_of_trigt - 1, lo, hi,
			  [](int rnum) { return trig_index[rnum] ? static_cast<int>(trig_index[rnum]->vnum) : -1; },
			  map.begin, map.end);

	std::map<int, int> old_rnum;
	for (int rnum = map.begin; rnum < map.end; ++rnum) {
		old_rnum[trig_index[rnum]->vnum] = rnum;
	}

	std::map<int, world_loader::LoadedTrigger *> block;
	for (auto &it : disk.triggers) {
		block[it.vnum] = &it;
	}
	map.delta = static_cast<int>(block.size()) - (map.end - map.begin);
	const int new_count = top_of_trigt + map.delta;

	IndexData **rebuilt = nullptr;
	CREATE(rebuilt, new_count);
	for (int rnum = 0; rnum < map.begin; ++rnum) {
		rebuilt[rnum] = trig_index[rnum];
	}
	int pos = map.begin;
	for (auto &[vnum, loaded] : block) {
		IndexData *index = nullptr;
		if (old_rnum.count(vnum)) {
			index = trig_index[old_rnum[vnum]];
			delete index->proto;
			map.inside[old_rnum[vnum]] = pos;
		} else {
			CREATE(index, 1);
			index->vnum = vnum;
			index->func = nullptr;
		}
		index->total_online = 0;
		loaded->trig->set_rnum(pos);
		index->proto = loaded->trig;
		loaded->trig = nullptr;
		rebuilt[pos] = index;
		// Индекс 'vnum trig' -- какие предметы грузит этот триггер.
		for (auto &entry : GlobalObjects::obj_triggers()) {
			entry.second.remove(vnum);
		}
		IndexTriggerObjLoads(vnum, index->proto);
		++report.applied_triggers;
		++pos;
	}
	// Исчезнувшие триггеры: индекс и тело больше не нужны.
	for (auto &[vnum, rnum] : old_rnum) {
		if (block.count(vnum)) {
			continue;
		}
		for (auto &entry : GlobalObjects::obj_triggers()) {
			entry.second.remove(vnum);
		}
		delete trig_index[rnum]->proto;
		free(trig_index[rnum]);
	}
	for (int rnum = map.end; rnum < top_of_trigt; ++rnum) {
		rebuilt[pos++] = trig_index[rnum];
	}

	free(trig_index);
	trig_index = rebuilt;
	top_of_trigt = new_count;
	return map;
}

// Предметам переиндексация не нужна: рнум ищется по карте внумов (CObjectPrototypes::get_rnum),
// а add() кладёт прототип в хвост -- порядок в таблице ничего не значит. Новые дописываем,
// уцелевшие подменяем на месте, исчезнувшие забываем по внуму.
void RebuildObjects(DiskZone &disk, Report &report, const std::set<int> &removed) {
	for (auto &it : disk.objects) {
		if (!it.obj) {
			continue;
		}
		auto *proto = new ObjData(*it.obj);
		const auto rnum = GetObjRnum(it.vnum);
		if (rnum >= 0) {
			olc_update_objects(rnum, proto);
			obj_proto.set_rnum(rnum, proto);
		} else {
			const auto index = obj_proto.add(proto, it.vnum);
			obj_proto.zone(index, get_zone_rnum_by_obj_vnum(it.vnum));
		}
		const auto placed = GetObjRnum(it.vnum);
		for (int trigger_vnum : it.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToObject(placed, trigger_vnum, it.vnum);
		}
		++report.applied_objects;
	}
	for (int vnum : removed) {
		obj_proto.forget_vnum(vnum);
	}
}

// Один проход по всему, что закэшировало рнумы. Порядок внутри не важен: карты уже построены.
// zone_begin..zone_end -- новые рнумы комнат перезагруженной зоны: их выходы уже переведены из
// внумов в рнумы по новой таблице, и переотображать их второй раз нельзя.
void ApplyRemap(const Remap &rooms, const Remap &mobs, const Remap &trigs,
				int zone_begin, int zone_end) {
	// Выходы всех остальных комнат мира.
	for (RoomRnum rnum = kFirstRoom; rnum <= top_of_world; ++rnum) {
		if (rnum >= zone_begin && rnum < zone_end) {
			continue;
		}
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
			auto &exit = world[rnum]->dir_option_proto[dir];
			if (exit && exit->to_room() != kNowhere) {
				exit->to_room(rooms(exit->to_room()));
			}
		}
	}

	// Живые: где стоят, откуда пришли, из какого прототипа сделаны.
	for (const auto &ch : character_list) {
		if (ch->in_room != kNowhere) {
			ch->in_room = rooms(ch->in_room);
		}
		if (ch->get_was_in_room() != kNowhere) {
			ch->set_was_in_room(rooms(ch->get_was_in_room()));
		}
		if (ch->IsNpc() && ch->get_rnum() >= 0) {
			ch->set_rnum(mobs(ch->get_rnum()));
		}
	}
	world_objects.foreach([&rooms](const ObjData::shared_ptr &obj) {
		if (obj->get_in_room() != kNowhere) {
			obj->set_in_room(rooms(obj->get_in_room()));
		}
	});

	// Точки входа в мир.
	r_mortal_start_room = rooms(r_mortal_start_room);
	r_immort_start_room = rooms(r_immort_start_room);
	r_frozen_start_room = rooms(r_frozen_start_room);
	r_helled_start_room = rooms(r_helled_start_room);
	r_named_start_room = rooms(r_named_start_room);
	r_unreg_start_room = rooms(r_unreg_start_room);

	// Клановые короба с ингредиентами.
	for (const auto &clan : Clan::ClanList) {
		if (clan->get_ingr_chest_room_rnum() > 0) {
			clan->set_ingr_chest_room_rnum(rooms(clan->get_ingr_chest_room_rnum()));
		}
	}

	// Ресеты всех зон мира: аргументы переведены в рнумы при загрузке. Предметные рнумы не
	// двигаются, поэтому O/G/E/P и arg2 у R не трогаем.
	for (auto &zone : zone_table) {
		if (!zone.cmd) {
			continue;
		}
		for (int cmd_no = 0; zone.cmd[cmd_no].command != 'S'; ++cmd_no) {
			auto &cmd = zone.cmd[cmd_no];
			switch (cmd.command) {
				case 'M': cmd.arg1 = mobs(cmd.arg1); cmd.arg3 = rooms(cmd.arg3); break;
				case 'Q': cmd.arg1 = mobs(cmd.arg1); break;
				case 'F': cmd.arg1 = rooms(cmd.arg1); cmd.arg2 = mobs(cmd.arg2); cmd.arg3 = mobs(cmd.arg3); break;
				case 'O': if (cmd.arg3 != kNowhere) cmd.arg3 = rooms(cmd.arg3); break;
				case 'D': cmd.arg1 = rooms(cmd.arg1); break;
				case 'R': cmd.arg1 = rooms(cmd.arg1); break;
				case 'T': cmd.arg2 = trigs(cmd.arg2);
					if (cmd.arg1 == WLD_TRIGGER) cmd.arg3 = rooms(cmd.arg3);
					break;
				case 'V': if (cmd.arg1 == WLD_TRIGGER) cmd.arg2 = rooms(cmd.arg2); break;
				default: break;
			}
			// Команда, потерявшая цель, выключается -- ровно как это делает загрузка, когда
			// внум не находится ("Invalid vnum, cmd disabled").
			if (cmd.arg1 == kNowhere && (cmd.command == 'M' || cmd.command == 'Q'
										 || cmd.command == 'F' || cmd.command == 'D' || cmd.command == 'R')) {
				cmd.command = '*';
			}
		}
	}

	// Копии триггеров у живых носителей: read_trigger раздаёт копию с рнумом прототипа, и
	// ExtractTrigger ходит по нему в trig_index.
	auto remap_script = [&trigs](const Script::shared_ptr &script) {
		if (!script) {
			return;
		}
		for (auto it = script->script_trig_list.begin(); it != script->script_trig_list.end(); ++it) {
			const auto rnum = (*it)->get_rnum();
			if (rnum >= 0) {
				(*it)->set_rnum(trigs(rnum));
			}
		}
	};
	for (RoomRnum rnum = kFirstRoom; rnum <= top_of_world; ++rnum) {
		remap_script(SCRIPT(world[rnum]));
	}
	for (const auto &ch : character_list) {
		remap_script(SCRIPT(ch));
	}
	world_objects.foreach([&remap_script](const ObjData::shared_ptr &obj) {
		remap_script(obj->get_script());
	});

	// Комнаты в открытых редакторах: там лежат свои ExitData со ссылками на рнумы.
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state == EConState::kRedit && OLC_ROOM(d)) {
			for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
				auto &exit = OLC_ROOM(d)->dir_option_proto[dir];
				if (exit && exit->to_room() != kNowhere) {
					exit->to_room(rooms(exit->to_room()));
				}
			}
		}
	}
}

void Reindex(ZoneRnum zrn, ZoneVnum zone_vnum, DiskZone &disk, Report &report) {
	const auto &zone = zone_table[zrn];
	std::set<int> removed_objects(report.objects.removed.begin(), report.objects.removed.end());

	// Таблицу трогаем только если состав этого вида сущностей изменился. Иначе правка одних
	// комнат заставляла бы пересобирать ещё и мобов с триггерами -- лишняя работа и лишний
	// риск; содержимое же применяется на месте, как в случае без переиндексации.
	Remap trigs, rooms, mobs;
	if (report.triggers.Changed()) {
		trigs = RebuildTriggers(zone, disk, report);
	} else {
		ApplyTriggers(disk, report);
	}
	if (report.rooms.Changed()) {
		rooms = RebuildRooms(zone, disk, report);
	} else {
		ApplyRooms(zone_vnum, disk, report);
	}
	if (report.mobs.Changed()) {
		mobs = RebuildMobs(zone, disk, report);
	} else {
		ApplyMobs(disk, report);
	}
	RebuildObjects(disk, report, removed_objects);

	// Комнаты зоны залиты из файла, и выходы в них пока записаны ВНУМАМИ -- так же, как сразу
	// после чтения мира, до RosolveWorldDoorToRoomVnumsToRnums. Переводим их в рнумы по уже
	// перестроенной таблице и исключаем этот участок из общего переотображения: иначе рнум,
	// только что посчитанный, будет сдвинут второй раз.
	// Границы блока комнат зоны в уже перестроенной таблице. Считаем по внумам, а не по
	// rooms.begin: если комнаты не менялись, переотображение для них тождественное, и его
	// begin ничего не говорит о том, где зона лежит.
	int zone_begin = top_of_world + 1;
	int zone_end = kFirstRoom;
	for (RoomRnum rnum = kFirstRoom; rnum <= top_of_world; ++rnum) {
		const int vnum = world[rnum]->vnum;
		if (vnum >= zone.vnum * 100 && vnum <= zone.top) {
			zone_begin = std::min(zone_begin, static_cast<int>(rnum));
			zone_end = std::max(zone_end, static_cast<int>(rnum) + 1);
		}
	}
	for (int rnum = zone_begin; rnum < zone_end && rnum <= top_of_world; ++rnum) {
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
			auto &exit = world[rnum]->dir_option_proto[dir];
			if (exit && exit->to_room() != kNowhere) {
				exit->to_room(GetRoomRnum(exit->to_room()));
			}
		}
	}

	ApplyRemap(rooms, mobs, trigs, zone_begin, zone_end);

	// Границы зон: блок уехал, значит у всех зон за нашей first/last другие. Функция считает
	// их для всего мира разом и заодно подставляет entrance, если он не задан.
	CalculateFirstAndLastRooms();
	CalculateFirstAndLastMobs();

	for (RoomRnum rnum = zone.RnumRoomsLocation.first; rnum <= zone.RnumRoomsLocation.second + 1; ++rnum) {
		if (rnum < kFirstRoom || rnum > top_of_world) {
			continue;
		}
		RestoreRoomExitData(rnum);
		CheckRoomForIncompatibleFlags(rnum);
		SCRIPT(world[rnum])->cleanup();
	}
	// Триггеры комнат -- по спискам из файла.
	for (auto &it : disk.rooms) {
		const auto rnum = GetRoomRnum(it.vnum);
		if (rnum <= 0) {
			continue;
		}
		for (int trigger_vnum : it.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToRoom(rnum, trigger_vnum, it.vnum);
		}
		assign_triggers(world[rnum], WLD_TRIGGER);
	}
}

// Шаги загрузки, которые пришивают содержимое к миру. Порядок тот же, в каком их делает
// BootWorld и BootMudDataBase; перечень и обоснование -- в issue #4201.
// reindexed == false означает, что ни один рнум не сдвинулся: состав зоны тот же, подменено
// только содержимое прототипов. Тогда всё, что хранит рнумы -- зонные индексы, таблица
// выпадения, рнумы системных предметов, индексы сетов, карта «предмет -> где грузится
// ресетами», -- остаётся верным, и пересобирать его незачем. Это те самые 130 мс из 225.
void Rewire(ZoneRnum zrn, const Report &report, std::string &timings) {
	// Что именно пересобирать, решается по тому, что изменилось. Рнумы предметов не двигаются
	// никогда (поиск по карте внумов), рнумы мобов и триггеров -- только когда менялся их
	// собственный состав, рнумы комнат -- когда менялся состав комнат.
	const bool rooms_moved = report.rooms.Changed();
	const bool mobs_moved = report.mobs.Changed();
	const bool trigs_moved = report.triggers.Changed();
	const bool objects_touched = report.objects.Changed();
	utils::CExecutionTimer step;
	auto mark = [&step, &timings](const char *what) {
		timings += fmt::format("{} {:.0f}мс, ", what, step.delta().count() * 1000.0);
		step.restart();
	};
	// Значения свитков, палочек и напитков из старого формата -- миграция идемпотентна.
	ConvertObjValues();
	mark("values");

	// Зонные рнумы в индексах.
	if (objects_touched) {
		SetZoneRnumForObjects();
	}
	if (mobs_moved) {
		SetZoneRnumForMobiles();
	}
	if (trigs_moved) {
		SetZoneRnumForTriggers();
	}
	// Карта «предмет -> где он грузится ресетами» держит рнумы комнат и предметов.
	if (rooms_moved || objects_touched) {
		IndexZoneObjLoads();
	}
	if (objects_touched || mobs_moved || trigs_moved || rooms_moved) {
		mark("индексы");
	}

	// Таблицы, построенные по прототипам предметов. Все они хранят ObjRnum, поэтому нужны
	// ровно тогда, когда у зоны менялся состав предметов.
	if (objects_touched) {
		oload_table.init();          // выпадение с мобов, хранит ObjRnum
		system_obj::init();          // кошелёк и персональный сундук, хранят ObjRnum
		obj_sets::init_obj_index();  // set_idx по прототипам
		mark("таблицы");
	}

	// Спецпроцедуры и лавки: func/farg живут в индексе и затираются подменой прототипа.
	ReloadSpecProcs();
	mark("спецпроц");

	// Рунные камни -- физические предметы в комнатах, зона могла их потерять или получить.
	MUD::Runestones().SpawnStones();
	mark("камни");

	// Счётчик мобов по уровням и список ловушек -- оба полные пересборы.
	MobMax::init();
	deathtrap::load();
	mark("мобмакс+ловушки");

	UniqueList<ZoneRnum> repop_list;
	repop_list.push_back(zrn);
	DecayObjectsOnRepop(repop_list);
	ResetZone(zrn);
	mark("ресет");
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

	utils::CExecutionTimer total;
	utils::CExecutionTimer stage;
	std::string timings;

	DiskZone disk;
	if (!ReadZone(zone_vnum, disk, report.error)) {
		return report;
	}
	FillReport(zone_table[zrn], disk, report);
	timings += fmt::format("чтение {:.0f}мс, ", stage.delta().count() * 1000.0);
	stage.restart();

	log("[ZoneReload] перезагружаю зону %d (%s)%s", zone_vnum, zone_table[zrn].name.c_str(),
		report.Changed() ? ", состав изменился" : "");
	EvictZone(zone_table[zrn]);
	timings += fmt::format("очистка {:.0f}мс, ", stage.delta().count() * 1000.0);
	stage.restart();

	if (report.Changed()) {
		// Состав изменился: блок записей зоны сносится целиком и собирается заново, а всё,
		// что лежит за ним, съезжает на дельту. Подробности -- в Reindex().
		Reindex(zrn, zone_vnum, disk, report);
	} else {
		// Триггеры первыми: комнаты, мобы и предметы тут же навесят их на себя по внумам.
		ApplyTriggers(disk, report);
		ApplyRooms(zone_vnum, disk, report);
		ApplyMobs(disk, report);
		ApplyObjects(disk, report);
	}
	FreeUnused(disk, true, true, true);
	timings += fmt::format("заливка {:.0f}мс, ", stage.delta().count() * 1000.0);
	stage.restart();

	Rewire(zrn, report, timings);

	report.timings = fmt::format("{}итого {:.0f}мс", timings, total.delta().count() * 1000.0);
	mudlog(fmt::format("Зона {} перечитана с диска: комнат {}, мобов {}, предметов {}, триггеров {}. {}",
					   zone_vnum, report.applied_rooms, report.applied_mobs,
					   report.applied_objects, report.applied_triggers, report.timings),
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
		out += "Время: " + report.timings + "\r\n";
	} else if (report.Changed()) {
		out += "Состав изменился, рнумы поедут. Повторите с 'применить', если это то, что нужно.\r\n";
	} else {
		out += "Состав совпадает. Разведка: в мире ничего не изменено.\r\n";
	}
	return out;
}

}  // namespace zone_reload

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
