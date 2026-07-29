// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#include "sets_drop.h"

#include <unordered_map>

#include "third_party_libs/pugixml/pugixml.h"
#include <fmt/format.h>

#include "engine/db/obj_prototypes.h"
#include "gameplay/clans/house.h"
#include "engine/ui/color.h"
#include "engine/db/help.h"
#include "gameplay/statistics/mob_stat.h"
#include "engine/entities/zone.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/dungeons.h"   // kZoneStartDungeons
#include "gameplay/mechanics/service_zones.h"

namespace SetsDrop {
// список сетин на дроп
const char *CONFIG_FILE = LIB_CFG"mechanics/sets_drop.xml";
// список уникальных мобов
const char *UNIQUE_MOBS = LIB_PLRSTUFF"unique_mobs.xml";
// минимальный уровень моба для участия в груп-списке дропа
const int MIN_GROUP_MOB_LVL = 32;
// мин/макс уровни мобов для выборки соло-сетин
const int MIN_SOLO_MOB_LVL = 25;
const int MAX_SOLO_MOB_LVL = 31;
// макс. кол-во участников в группе учитываемое в статистике
const int MAX_GROUP_SIZE = 12;
const char *DROP_TABLE_FILE = LIB_PLRSTUFF"sets_drop.xml";
// сброс таблицы лоада каждые х часов
const int RESET_TIMER = 35;
// базовый шанс дропа соло сетин *10
const int DEFAULT_SOLO_CHANCE = 30;
// повышенный шанс для мини-сетов относительно фул-сетов
const double MINI_SET_MULT = 1.5;
// сообщение игрокам при резете таблицы дропа
const char *RESET_MESSAGE =
	"Внезапно мир содрогнулся, день поменялся с ночью, земля с небом\r\n"
	"...но через миг все вернулось на круги своя.";
enum { SOLO_MOB, GROUP_MOB, SOLO_ZONE, GROUP_ZONE };

// время следующего сброса таблицы
time_t next_reset_time = 0;
// флаг для сейва таблицы дропа по таймеру
bool need_save_drop_table = false;

// список груп-сетин на лоад (vnum)
std::list<int> group_obj_list;
// список соло-сетин на лоад (vnum)
std::list<int> solo_obj_list;

// уникальные мобы. Таблица вида внум : левел
std::map<int, int> unique_mobs;
struct MobNode {
	MobNode() : vnum(-1), rnum(-1), miw(-1), type(-1) {
		kill_stat.fill(0);
	};
	int vnum;
	int rnum;
	// макс.в.мире
	int miw;
	// имя моба (для фильтрации)
	std::string name;
	// груп/соло моб
	int type;
	// статистика убиств по размеру группы
	mob_stat::KillStat kill_stat;
};

struct ZoneNode {
	ZoneNode() : zone(-1), type(-1) {};

	// внум зоны
	int zone;
	// тип зоны (соло/групповая), вычисляется по статистике убийств мобов
	int type;
	// список мобов в зоне
	std::list<MobNode> mobs;
};

// временный список для отсева неуникальных имен внутри одной зоны
std::list<ZoneNode> mob_name_list;
// временный список мобов на лоад груп-сетин
std::list<ZoneNode> group_mob_list;
// временный список мобов на лоад соло-сетин
std::list<ZoneNode> solo_mob_list;

struct HelpNode {
	// список алиасов
	std::string alias_list;
	// полное имя сета
	std::string title;
	// списки предметов по типам дропа
	std::set<int> solo_list_1;
	std::set<int> solo_list_2;
	std::set<int> group_list;
};
// список сетов для справки
std::vector<HelpNode> help_list;

class Linked {
 public:
	Linked() {};
	size_t list_size() const { return mob_list.size(); };
	int drop_count() const;
	bool need_reset() const;
	void reset();
	void add(int mob_rnum) { mob_list.insert(mob_rnum); };
 private:
	// список мобов залинкованных через справку
	std::set<int /* mob rnum */> mob_list;
};

struct DropNode {
	DropNode() : obj_rnum(0), obj_vnum(0), chance(0), solo(false), can_drop(false), is_big_set(false) {};
	// рнум сетины
	int obj_rnum;
	// внум сетины
	int obj_vnum;
	// шанс дропа (проценты * 10), drop_chance из 1000
	int chance;
	// из соло или груп моб листа эта сетина (для генерации справки)
	bool solo;
	// false = убийства в момент, когда шмотка = макс в мире
	// true = убийства в момент, когда шмотка может дропнуться
	bool can_drop;
	// для мини-сетов при калькуляции шансов дропа (инится только для solo сетин)
	bool is_big_set;
	// линкованные через справку соло-мобы
	Linked linked_mobs;

	void reset_chance() { chance = is_big_set ? DEFAULT_SOLO_CHANCE : DEFAULT_SOLO_CHANCE * MINI_SET_MULT; };
};

std::map<int, int> get_unique_mob() {
	return unique_mobs;
}

// финальный список дропа по мобам (MobRnum)
std::map<int, DropNode> drop_list;

// проверяется макс в мире шмотки и флаг возможности дропа, при необходимости
// выставляется флаг дропа и возвращется флаг необходимости резета шансов для всей группы
bool Linked::need_reset() const {
	bool flag = false;
	for (std::set<int>::const_iterator i = mob_list.begin(),
			 iend = mob_list.end(); i != iend; ++i) {
		std::map<int, DropNode>::iterator k = drop_list.find(*i);
		if (k != drop_list.end()) {
			const int num = obj_proto.actual_count(k->second.obj_rnum);
			if (num < GetObjMIW(k->second.obj_rnum) && !k->second.can_drop) {
				flag = true;
				k->second.can_drop = true;
			}
			if (num >= GetObjMIW(k->second.obj_rnum) && k->second.can_drop) {
				k->second.can_drop = false;
			}
		}
	}
	return flag;
}

void Linked::reset() {
	for (std::set<int>::const_iterator i = mob_list.begin(),
			 iend = mob_list.end(); i != iend; ++i) {
		std::map<int, DropNode>::iterator k = drop_list.find(*i);
		if (k != drop_list.end()) {
			k->second.reset_chance();
		}
	}
}

int Linked::drop_count() const {
	int count = 0;

	for (std::set<int>::const_iterator i = mob_list.begin(),
			 iend = mob_list.end(); i != iend; ++i) {
		std::map<int, DropNode>::const_iterator k = drop_list.find(*i);
		if (k != drop_list.end() && k->second.can_drop) {
			++count;
		}
	}

	return count;
}

// создаем копию стафины миника + idx устанавливает точно такой же
void create_clone_miniset(int vnum) {
	const int new_vnum = DUPLICATE_MINI_SET_VNUM + vnum;

	// если такой зоны нет, то делаем ретурн
	if ((new_vnum % 100) >= static_cast<int>(zone_table.size())) {
		return;
	}

	const int rnum = GetObjRnum(vnum);

	// проверяем, есть ли у нашей сетины клон в системной зоне
	const int rnum_nobj = GetObjRnum(new_vnum);

	if (rnum_nobj < 0) {
		return;
	}

	// здесь сохраняем рнум нашего нового объекта
	obj_proto.set_idx(rnum_nobj, obj_proto.set_idx(rnum));
}

// * Инициализация списка сетов на лоад.
void init_obj_list() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("set_list");
	if (!node_list) {
		mudlog("...<set_list> read fail", CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	for (pugi::xml_node set_node = node_list.child("set");
		 set_node; set_node = set_node.next_sibling("set")) {
		HelpNode node;

		node.alias_list = parse::ReadAattrAsStr(set_node, "help_alias");
		if (node.alias_list.empty()) {
			mudlog("...bad set attributes (empty help_alias)",
				   CMP, kLvlImmortal, SYSLOG, true);
			continue;
		}

		// manual_mode=true: для каждой сетины руками задан group="true|false".
		// false (по умолчанию): сет делится на solo/group автоматом по активаторам.
		const bool manual_mode = set_node.attribute("manual_mode").as_bool();

		std::set<int> tmp_solo_list;

		if (manual_mode) {
			for (pugi::xml_node obj_node = set_node.child("obj");
				 obj_node; obj_node = obj_node.next_sibling("obj")) {
				const int obj_vnum = parse::ReadAttrAsInt(obj_node, "vnum");
				const int obj_rnum = GetObjRnum(obj_vnum);
				if (obj_rnum < 0) {
					snprintf(buf, sizeof(buf),
							 "...bad obj_node attributes (vnum=%d)", obj_vnum);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}

				// group="true" -- груп-список, false/без атрибута -- соло-список
				const bool group = obj_node.attribute("group").as_bool();
				if (!group) {
					solo_obj_list.push_back(obj_vnum);
					// предварительный соло список, который
					// дальше может быть поделен еще на два
					tmp_solo_list.insert(obj_vnum);
				} else {
					group_obj_list.push_back(obj_vnum);
					// груп список инится сразу
					node.group_list.insert(obj_vnum);
				}
				// имя сета
				if (node.title.empty()) {
					if (obj_proto.set_idx(obj_rnum) != static_cast<size_t>(-1)) {
						node.title = obj_sets::get_name(obj_proto.set_idx(obj_rnum));
					} else {
						for (const auto &it : ObjData::set_table) {
							const auto k = it.second.find(obj_vnum);
							if (k != it.second.end()) {
								node.title = it.second.get_name();
							}
						}
					}
				}
				//create_clone_miniset(ObjVnum);
			}
		} else {
			// список сета сортированный по макс.активаторам
			std::multimap<int, int> set_sort_list;

			for (pugi::xml_node obj_node = set_node.child("obj"); obj_node; obj_node = obj_node.next_sibling("obj")) {
				const int obj_vnum = parse::ReadAttrAsInt(obj_node, "vnum");
				if (GetObjRnum(obj_vnum) < 0) {
					snprintf(buf, sizeof(buf),
							 "...bad obj_node attributes (vnum=%d)", obj_vnum);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}
				// заполнение списка активаторов
				for (id_to_set_info_map::const_iterator it = ObjData::set_table.begin(),
						 iend = ObjData::set_table.end(); it != iend; ++it) {
					set_info::const_iterator k = it->second.find(obj_vnum);
					if (k != it->second.end() && !k->second.empty()) {
						// берется последний (максимальный) в списке активатор
						set_sort_list.insert(
							std::make_pair(k->second.rbegin()->first, obj_vnum));
						// имя сета
						if (node.title.empty()) {
							node.title = it->second.get_name();
						}
					}
				}
			}
			// первая половина активаторов в соло-лист, вторая в групп
			size_t num = 0;
			size_t total_num = set_sort_list.size();
			for (std::multimap<int, int>::const_iterator i = set_sort_list.begin(),
					 iend = set_sort_list.end(); i != iend; ++i, ++num) {
				if (num < total_num / 2) {
					solo_obj_list.push_back(i->second);
					tmp_solo_list.insert(i->second);
				} else {
					group_obj_list.push_back(i->second);
					node.group_list.insert(i->second);
				}
			}
		}
		// деление соло списка на два, если требуется
		if (tmp_solo_list.size() > 4) {
			size_t num = 0;
			size_t total_num = tmp_solo_list.size();
			for (std::set<int>::const_iterator i = tmp_solo_list.begin(),
					 iend = tmp_solo_list.end(); i != iend; ++i, ++num) {
				if (num < total_num / 2) {
					node.solo_list_1.insert(*i);
				} else {
					node.solo_list_2.insert(*i);
				}
			}
		} else {
			node.solo_list_1 = tmp_solo_list;
		}

		// список алиасов и сетин для справки
		help_list.push_back(node);
	}
}

void add_to_zone_list(std::list<ZoneNode> &cont, MobNode &node) {
	int zone = node.vnum / 100;
	std::list<ZoneNode>::iterator k = std::find_if(cont.begin(), cont.end(),
												   [&](const ZoneNode &zn) {
													   return zn.zone == zone;
												   });

	if (k != cont.end()) {
		k->mobs.push_back(node);
	} else {
		ZoneNode tmp_node;
		tmp_node.zone = zone;
		tmp_node.mobs.push_back(node);
		cont.push_back(tmp_node);
	}
}

/**
 * Генерация предварительного общего списка мобов для последующего
 * отсева неуникальных по именам внутри одной зоны.
 * Инится у моба: внум, рун, имя
 * Отсекаются: мобы без прототипа
 *             левые зоны: клан-замки, города с рентой, почтой и банком
 */
void init_mob_name_list() {
	std::set<int> bad_zones;

	// клан-замки -- меняются в игре, поэтому считаем каждый раз
	for (const auto &clan : Clan::ClanList) {
		bad_zones.insert(clan->GetRent() / 100);
	}

	// города -- флаг зоны, посчитанный при загрузке мира (SetZonesTownFlags): там уже пройдены
	// комнаты каждой зоны в поисках ренты, банка и почты, второй раз то же самое делать незачем.
	for (const auto &zone : zone_table) {
		if (zone.is_town) {
			bad_zones.insert(zone.vnum);
		}
	}

	// тестовые зоны
	for (std::size_t nr = 0; nr < zone_table.size(); nr++) {
		if (zone_table[nr].under_construction) {
			bad_zones.insert(zone_table[nr].vnum);
		}
	}

	// Кандидаты берутся из команд загрузки мобов в зон-ресетах, а не из статистики убийств:
	// в реестр статистики попадают только те, кого недавно били, поэтому глухие зоны выпадали
	// из таблицы навсегда и дроп сползал на популярных мобов. Команда 'M' дает все нужное сразу:
	// самого моба, его лимит в мире и комнату, куда он грузится.
	for (const auto &zone : zone_table) {
		const int zone_vnum = zone.vnum;
		if (service_zones::IsNoLootZone(zone_vnum)   // служебные и начальные, см. cfg/mechanics
			|| zone_vnum >= dungeons::kZoneStartDungeons   // в данжах сеты не падают
			|| bad_zones.count(zone_vnum) > 0) {
			continue;
		}

		for (int cmd_no = 0; zone.cmd && zone.cmd[cmd_no].command != 'S'; ++cmd_no) {
			if (zone.cmd[cmd_no].command != 'M') {
				continue;
			}
			const int rnum = zone.cmd[cmd_no].arg1;      // rnum моба (переведен при загрузке)
			const int max_in_world = zone.cmd[cmd_no].arg2;
			const RoomRnum room_rnum = zone.cmd[cmd_no].arg3;   // rnum комнаты, тоже переведен

			if (rnum < 0 || rnum > top_of_mobt) {
				continue;
			}
			// пока только уникальные мобы
			if (max_in_world != 1) {
				continue;
			}
			if (room_rnum <= 0 || static_cast<size_t>(room_rnum) >= world.size()) {
				continue;
			}
			// В мирной комнате моба не убить -- но только если он там и стоит. Бродячий моб
			// из мирной выйдет, такой в списке нужен: дождется, когда выйдет, и будет убит.
			if (ROOM_FLAGGED(room_rnum, ERoomFlag::kPeaceful)
				&& mob_proto[rnum].IsFlagged(EMobFlag::kSentinel)) {
				continue;
			}
			// засадные мобы сидят в виртуальных комнатах без выходов -- до них не добраться
			bool has_exit = false;
			for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
				if (world[room_rnum]->dir_option[dir]) {
					has_exit = true;
					break;
				}
			}
			if (!has_exit) {
				continue;
			}

			MobNode node;
			node.vnum = mob_index[rnum].vnum;
			node.rnum = rnum;
			node.name = mob_proto[rnum].get_name();
			node.miw = max_in_world;
			// статистика убийств есть не у всех -- она только уточняет тип моба
			const auto stat_it = mob_stat::mob_stat_register.find(node.vnum);
			if (stat_it != mob_stat::mob_stat_register.cend()) {
				node.kill_stat = mob_stat::SumStat(stat_it->second.stats, 4).kills;
			}

			add_to_zone_list(mob_name_list, node);
		}
	}
}

// * Инится у зоны: тип
void init_zone_type() {
	for (std::list<ZoneNode>::iterator i = mob_name_list.begin(),
			 iend = mob_name_list.end(); i != iend; ++i) {
		// Тип берется из свойства зоны ("оптимальное число игроков", <= 1 -- соло): оно задано
		// билдером и есть у любой зоны, в том числе у той, где никого ни разу не били. Раньше тип
		// считался по статистике убийств, и зона без статистики выпадала из таблицы целиком.
		const auto zone_rnum = GetZoneRnum(i->zone);
		i->type = (zone_rnum >= 0 && zone_table[zone_rnum].group >= 2) ? GROUP_ZONE : SOLO_ZONE;
	}
}

// * Инится у моба: тип
void init_mob_type() {
	for (std::list<ZoneNode>::iterator i = mob_name_list.begin(),
			 iend = mob_name_list.end(); i != iend; ++i) {
		for (std::list<MobNode>::iterator k = i->mobs.begin(),
				 kend = i->mobs.end(); k != kend; ++k) {
			// по умолчанию моб той же природы, что и его зона -- это работает и без статистики
			k->type = (i->type == GROUP_ZONE) ? GROUP_MOB : SOLO_MOB;

			// если моба били, статистика уточняет: в групповой зоне его могли валить в одиночку
			// и наоборот
			int group_cnt = 0;
			for (int cnt = 2; cnt <= MAX_GROUP_SIZE; ++cnt) {
				group_cnt += k->kill_stat.at(cnt);
			}
			const int solo_cnt = k->kill_stat.at(1);
			if (solo_cnt + group_cnt > 0) {
				k->type = (solo_cnt > group_cnt) ? SOLO_MOB : GROUP_MOB;
			}
		}
	}
}

/**
 * С этого момента начинают отсекаться отдельные мобы
 * Отсекаются: мобы с неуникальным именем внутри одной зоны,
 *             мобы ниже требуемого уровня для данного типа
 *             мобы с флагами только полнолуние/времена года
 * Мобы больше 1 макс.в.мире отсеиваются раньше, при отборе по командам зон-ресетов.
 */
void filter_dupe_names() {
	int vnum = 0;
	int level = 0;
	for (std::list<ZoneNode>::iterator it = mob_name_list.begin(),
			 iend = mob_name_list.end(); it != iend; ++it) {
		std::list<MobNode> tmp_list;
		// Сколько мобов зоны носит каждое имя -- одним проходом. Раньше для каждого моба
		// перебирались все мобы зоны со сравнением строк, то есть квадрат на зону.
		std::unordered_map<std::string, int> name_count;
		for (const auto &m : it->mobs) {
			++name_count[m.name];
		}
		// отсеиваем (включая оригинал) одинаковые имена с разными внумами
		for (std::list<MobNode>::iterator k = it->mobs.begin(),
				 kend = it->mobs.end(); k != kend; ++k) {
			// одинаковые имена в пределах зоны
			const bool good = name_count[k->name] <= 1;
			if (!good || k->type == -1) {
				continue;
			}
			if (k->type == GROUP_MOB
				&& mob_proto[k->rnum].GetLevel() < MIN_GROUP_MOB_LVL) {
				continue;
			}
			// редко появляющиеся мобы, мобы без экспы
			const CharData *mob = &mob_proto[k->rnum];
			if (mob->IsFlagged(EMobFlag::kAppearsFullmoon)
				|| mob->IsFlagged(EMobFlag::kAppearsWinter)
				|| mob->IsFlagged(EMobFlag::kAppearsSpring)
				|| mob->IsFlagged(EMobFlag::kAppearsSummer)
				|| mob->IsFlagged(EMobFlag::kAppearsAutumn)
				|| NPC_FLAGGED(mob, ENpcFlag::kFreeDrop) //не падает фридроп
				|| mob->IsFlagged(EMobFlag::kProtect) //урон обнуляется, не убить
				|| mob->IsFlagged(EMobFlag::kNoFight) //на него нельзя даже напасть
				|| mob->get_exp() <= 0) {
				continue;
			}




			vnum = mob_index[k->rnum].vnum;
			level = mob_proto[k->rnum].GetLevel();
			unique_mobs.insert(std::make_pair(vnum, level));

			// проверка на левел моба
			if (k->type == SOLO_MOB
				&& (mob_proto[k->rnum].GetLevel() < MIN_SOLO_MOB_LVL
					|| mob_proto[k->rnum].GetLevel() > MAX_SOLO_MOB_LVL)) {
				continue;
			}

			tmp_list.push_back(*k);
		}
		it->mobs = std::move(tmp_list);
	}
}

/**
 * Отсекаются: лишние мобы в зонах с самым длинным списком мобов
 * для более равномерного распределения по разным зонам
 */
void filter_extra_mobs(int total, int type) {
	std::list<ZoneNode> &cont = (type == GROUP_MOB) ? group_mob_list : solo_mob_list;
	const size_t obj_total = (type == GROUP_MOB) ? group_obj_list.size() : solo_obj_list.size();
	// обрезание лишних мобов в самых заполненных зонах
	int num_del = total - static_cast<int>(obj_total);
	while (num_del > 0) {
		size_t max_num = 0;
		for (std::list<ZoneNode>::iterator it = cont.begin(),
				 iend = cont.end(); it != iend; ++it) {
			if (it->mobs.size() > max_num) {
				max_num = it->mobs.size();
			}
		}
		for (std::list<ZoneNode>::iterator it = cont.begin(),
				 iend = cont.end(); it != iend; ++it) {
			if (it->mobs.size() >= max_num) {
				std::list<MobNode>::iterator l = it->mobs.begin();
				std::advance(l, number(0, static_cast<int>(it->mobs.size()) - 1));
				it->mobs.erase(l);
				if (it->mobs.empty()) {
					cont.erase(it);
				}
				--num_del;
				break;
			}
		}
	}
}

// * Разделение общего списка мобов на групп- и соло- списки.
void split_mob_name_list() {
	int total_group_mobs = 0, total_solo_mobs = 0;

	for (std::list<ZoneNode>::iterator it = mob_name_list.begin(),
			 iend = mob_name_list.end(); it != iend; ++it) {
		for (std::list<MobNode>::iterator k = it->mobs.begin(),
				 kend = it->mobs.end(); k != kend; ++k) {
			if (k->type == GROUP_MOB) {
				add_to_zone_list(group_mob_list, *k);
				++total_group_mobs;
			} else if (k->type == SOLO_MOB) {
				add_to_zone_list(solo_mob_list, *k);
				++total_solo_mobs;
			}
		}
	}
	mob_name_list.clear();

	filter_extra_mobs(total_group_mobs, GROUP_MOB);
	filter_extra_mobs(total_solo_mobs, SOLO_MOB);
}

int calc_drop_chance(std::list<MobNode>::iterator &mob, int obj_rnum) {
	int chance = 0;

	if (mob->type == GROUP_MOB) {
		// два состава максимальных по кол-ву убийств
		int max_kill = 0;
		int num1 = 2;
		// в два цикла как-то нагляднее
		for (int i = 2; i <= MAX_GROUP_SIZE; ++i) {
			if (mob->kill_stat.at(i) > max_kill) {
				max_kill = mob->kill_stat.at(i);
				num1 = i;
			}
		}
		int max_kill2 = 0;
		int num2 = 2;
		for (int i = 2; i <= MAX_GROUP_SIZE; ++i) {
			if (i != num1
				&& mob->kill_stat.at(i) > max_kill2) {
				max_kill2 = mob->kill_stat.at(i);
				num2 = i;
			}
		}
		// и среднее между ними
		double num_mod = (num1 + num2) / 2.0;
		// 5.8% .. 14.8%
		chance = (40 + num_mod * 9) / mob->miw;
	} else {
		// 2.6% .. 3.4% / 38 ... 28
/*		int mob_lvl = mob_proto[mob->rnum].get_level();
		int lvl_mod = MIN(MAX(0, mob_lvl - MIN_SOLO_MOB_LVL), 6);
		drop_chance = (26 + lvl_mod * 1.45) / mob->miw;
*/
		chance = DEFAULT_SOLO_CHANCE;
	}

	// мини сеты увеличенный шанс на дроп
	const auto &obj = obj_proto[obj_rnum];
	if (!SetSystem::is_big_set(obj.get())) {
		chance *= MINI_SET_MULT;
	}

	return chance;
}

// * Генерация финальной таблицы дропа с мобов.
void init_drop_table(int type) {
	std::list<ZoneNode> &mob_list = (type == GROUP_MOB) ? group_mob_list : solo_mob_list;
	std::list<int> &obj_list = (type == GROUP_MOB) ? group_obj_list : solo_obj_list;

	while (!obj_list.empty() && !mob_list.empty()) {
		std::list<int>::iterator it = obj_list.begin();
		std::advance(it, number(0, static_cast<int>(obj_list.size()) - 1));
		const int obj_rnum = GetObjRnum(*it);

		std::list<ZoneNode>::iterator k = mob_list.begin();
		std::advance(k, number(0, static_cast<int>(mob_list.size()) - 1));

		// по идее чистить надо сразу после удаления последнего
		// элемента, но тут может получиться ситуация, что на входе
		// изначально был пустой список после всяких фильтраций
		if (k->mobs.empty()) {
			mob_list.erase(k);
			continue;
		}

		std::list<MobNode>::iterator l = k->mobs.begin();
		std::advance(l, number(0, static_cast<int>(k->mobs.size()) - 1));

		DropNode tmp_node;
		tmp_node.obj_rnum = obj_rnum;
		tmp_node.obj_vnum = *it;
		tmp_node.chance = calc_drop_chance(l, obj_rnum);
		tmp_node.solo = (type == GROUP_MOB) ? false : true;

		drop_list.insert(std::make_pair(l->rnum, tmp_node));

		obj_list.erase(it);
		k->mobs.erase(l);
	}

	obj_list.clear();
	mob_list.clear();
}

void init_linked_mobs(const std::set<int> &node) {
	// инит рнумов залинкованных мобов
	Linked tmp_linked_mobs;
	for (std::set<int>::const_iterator l = node.begin(),
			 lend = node.end(); l != lend; ++l) {
		for (std::map<int, DropNode>::iterator k = drop_list.begin(),
				 kend = drop_list.end(); k != kend; ++k) {
			if (k->second.obj_vnum == *l) {
				tmp_linked_mobs.add(k->first);
			}
		}
	}
	// суем сформированный список каждому мобу из этого же списка,
	// но уже в рабочий DropNode
	for (std::set<int>::const_iterator l = node.begin(),
			 lend = node.end(); l != lend; ++l) {
		for (std::map<int, DropNode>::iterator k = drop_list.begin(),
				 kend = drop_list.end(); k != kend; ++k) {
			if (k->second.obj_vnum == *l) {
				k->second.linked_mobs = tmp_linked_mobs;
			}
		}
	}
}

void init_link_system() {
	// инит DropNode::linked_mobs
	for (std::vector<HelpNode>::const_iterator i = help_list.begin(),
			 iend = help_list.end(); i != iend; ++i) {
		init_linked_mobs(i->solo_list_1);
		init_linked_mobs(i->solo_list_2);
	}
	// инит DropNode::is_big_set
	for (std::map<int, DropNode>::iterator k = drop_list.begin(),
			 kend = drop_list.end(); k != kend; ++k) {
		const auto &obj = obj_proto[k->second.obj_rnum];
		k->second.is_big_set = SetSystem::is_big_set(obj.get());
	}
}

std::string print_solo_list(const std::set<int> &node) {
	if (node.empty()) return "";

	std::stringstream out;
	std::stringstream solo_obj_names;
	std::vector<std::string> solo_mob_list;
	int new_line = -1;

	for (std::set<int>::const_iterator l = node.begin(),
			 lend = node.end(); l != lend; ++l) {
		for (std::map<int, DropNode>::iterator k = drop_list.begin(),
				 kend = drop_list.end(); k != kend; ++k) {
			if (k->second.obj_vnum == *l) {
				if (new_line == -1) {
					solo_obj_names << "   ";
					new_line = 0;
				} else if (new_line == 0) {
					solo_obj_names << ", ";
					new_line = 1;
				} else if (new_line == 1) {
					solo_obj_names << "\r\n   ";
					new_line = 0;
				}
				solo_obj_names << obj_proto[k->second.obj_rnum]->get_PName(grammar::ECase::kNom);
				std::stringstream solo_out;
				solo_out.precision(1);
				solo_out << "    - " << mob_proto[k->first].get_name()
						 << " (" << zone_table[mob_index[k->first].zone].name << ")"
						 << " - " << std::fixed << k->second.chance / 10.0 << "%\r\n";
				solo_mob_list.push_back(solo_out.str());
			}
		}
	}

	std::srand(std::time(0));
	std::shuffle(solo_mob_list.begin(), solo_mob_list.end(), std::mt19937(std::random_device()()));
	out << solo_obj_names.str() << "\r\n";
	for (std::vector<std::string>::const_iterator i = solo_mob_list.begin(),
			 iend = solo_mob_list.end(); i != iend; ++i) {
		out << *i;
	}

	return out.str();
}

/**
 * Распечатка конкретного сета на основе ранее сформированного HelpNode.
 * С делением вывода на соло и груп сетины.
 */
std::string print_current_set(const HelpNode &node) {
	std::stringstream out;
	out << node.title << "\r\n";
	out.precision(1);

	out << print_solo_list(node.solo_list_1);
	out << print_solo_list(node.solo_list_2);

	for (const auto l : node.group_list) {
		for (const auto &k : drop_list) {
			if (obj_proto[k.second.obj_rnum]->get_vnum() == l && k.second.chance > 0) {
				out << "   " << obj_proto[k.second.obj_rnum]->get_PName(grammar::ECase::kNom)
					<< " - " << mob_proto[k.first].get_name()
					<< " (" << zone_table[mob_index[k.first].zone].name << ")"
					<< " - " << std::fixed << k.second.chance / 10.0 << "%\r\n";
				break;
			}
		}
	}

	return out.str();
}

/**
 * Генерация 'справка сеты'.
 */
void init_xhelp() {
	std::stringstream out;
	out << "Наборы предметов, участвующие в системе автоматического выпадения:\r\n";

	for (const auto & i : help_list) {
		out << "\r\n" << print_current_set(i);
	}

	HelpSystem::add_sets_drop("сетывсе", out.str());
	HelpSystem::add_sets_drop("сэтывсе", out.str());
	HelpSystem::add_sets_drop("наборыпредметов", out.str());
}

/**
 * Генерация справки по каждому набору отдельно.
 */
void init_xhelp_full() {
	for (std::vector<HelpNode>::const_iterator i = help_list.begin(),
			 iend = help_list.end(); i != iend; ++i) {
		std::string text = print_current_set(*i);
		std::vector<std::string> str_list = utils::SplitAny(i->alias_list, ", ");

		for (std::vector<std::string>::const_iterator k = str_list.begin(),
				 kend = str_list.end(); k != kend; ++k) {
			HelpSystem::add_sets_drop(*k, text);
		}
	}
}

bool load_unique_mobs() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(UNIQUE_MOBS);
	int vnum = 0;
	int level = 0;
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return false;
	}

	pugi::xml_node node_list = doc.child("mobs");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...<mobs> read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return false;
	}

	for (pugi::xml_node node = node_list.child("mob"); node; node = node.next_sibling("mob")) {
		vnum = parse::ReadAttrAsInt(node, "vnum");
		level = parse::ReadAttrAsInt(node, "level");
		unique_mobs.insert(std::make_pair(vnum, level));
	}
	return true;
}

bool load_drop_table() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(DROP_TABLE_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return false;
	}

	pugi::xml_node node_list = doc.child("drop_list");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...<drop_list> read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return false;
	}

	pugi::xml_node time_node = node_list.child("time");
	std::string timer = parse::ReadAattrAsStr(time_node, "reset");
	if (!timer.empty()) {
		try {
			next_reset_time = std::stoull(timer, nullptr, 10);
		}
		catch (...) {
			snprintf(buf, kMaxStringLength, "...timer (%s) lexical_cast fail", timer.c_str());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return false;
		}
	} else {
		mudlog("...empty reset time", CMP, kLvlImmortal, SYSLOG, true);
		return false;
	}

	for (pugi::xml_node item_node = node_list.child("item"); item_node;
		 item_node = item_node.next_sibling("item")) {
		const int obj_vnum = parse::ReadAttrAsInt(item_node, "vnum");
		const int obj_rnum = GetObjRnum(obj_vnum);
		if (obj_vnum <= 0 || obj_rnum < 0) {
			snprintf(buf, sizeof(buf),
					 "...bad item attributes (vnum=%d)", obj_vnum);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return false;
		}

		const int mob_vnum = parse::ReadAttrAsInt(item_node, "mob");
		const int mob_rnum = GetMobRnum(mob_vnum);
		if (mob_vnum <= 0 || mob_rnum < 0) {
			snprintf(buf, sizeof(buf),
					 "...bad item attributes (mob=%d)", mob_vnum);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return false;
		}

		const int chance = parse::ReadAttrAsInt(item_node, "drop_chance");
		if (chance < 0) {
			snprintf(buf, sizeof(buf),
					 "...bad item attributes (drop_chance=%d)", chance);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return false;
		}

		std::string solo = parse::ReadAattrAsStr(item_node, "solo");
		if (solo.empty()) {
			snprintf(buf, sizeof(buf), "...bad item attributes (solo=empty)");
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return false;
		}

		std::string can_drop = parse::ReadAattrAsStr(item_node, "can_drop");
		if (can_drop.empty()) {
			snprintf(buf, sizeof(buf), "...bad item attributes (can_drop=empty)");
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return false;
		}

		DropNode tmp_node;
		tmp_node.obj_rnum = obj_rnum;
		tmp_node.obj_vnum = obj_vnum;
		tmp_node.chance = chance;
		tmp_node.solo = solo == "true" ? true : false;
		tmp_node.can_drop = can_drop == "true" ? true : false;

		drop_list.insert(std::make_pair(mob_rnum, tmp_node));
	}
	return true;
}

void save_unique_mobs() {
	pugi::xml_document doc;
	doc.append_child().set_name("mobs");
	pugi::xml_node node_list = doc.child("mobs");
	std::map<int, int>::iterator it;
	for (it = unique_mobs.begin(); it != unique_mobs.end(); it++) {
		pugi::xml_node mob_node = node_list.append_child();
		mob_node.set_name("mob");
		mob_node.append_attribute("vnum") = it->first;
		mob_node.append_attribute("level") = it->second;
	}
	doc.save_file(UNIQUE_MOBS);
}

void save_drop_table() {
	if (!need_save_drop_table) return;

	pugi::xml_document doc;
	doc.append_child().set_name("drop_list");
	pugi::xml_node node_list = doc.child("drop_list");

	pugi::xml_node time_node = node_list.append_child();
	time_node.set_name("time");
	time_node.append_attribute("reset") = fmt::format("{}", next_reset_time).c_str();

	for (std::map<int, DropNode>::iterator i = drop_list.begin(),
			 iend = drop_list.end(); i != iend; ++i) {
		pugi::xml_node mob_node = node_list.append_child();
		mob_node.set_name("item");
		mob_node.append_attribute("vnum") = obj_proto[i->second.obj_rnum]->get_vnum();
		mob_node.append_attribute("mob") = mob_index[i->first].vnum;
		mob_node.append_attribute("drop_chance") = i->second.chance;
		mob_node.append_attribute("solo") = i->second.solo ? "true" : "false";
		mob_node.append_attribute("can_drop") = i->second.can_drop ? "true" : "false";
	}

	doc.save_file(DROP_TABLE_FILE);

	need_save_drop_table = false;
}

void reload_by_timer() {
	if (next_reset_time <= time(0)) {
		reload();
	}
}

void message_for_players() {
	for (DescriptorData *i = descriptor_list; i; i = i->next) {
		if  (i->state == EConState::kPlaying && i->character) {
			SendMsgToChar(i->character.get(), "%s%s%s\r\n",
						  kColorBoldCyn, RESET_MESSAGE,
						  kColorNrm);
		}
	}
}

void init_drop_system() {
	init_mob_name_list();
	init_zone_type();
	init_mob_type();
	filter_dupe_names();
	split_mob_name_list();

	init_drop_table(SOLO_MOB);
	init_drop_table(GROUP_MOB);

	next_reset_time = time(0) + 60 * 60 * RESET_TIMER;
	need_save_drop_table = true;
	save_drop_table();
}

/**
 * Релоад таблицы дропа, без релоада статистики по убийствам мобов.
 * \param zone_vnum - если не нулевой, удаляет из статистики мобов
 * всю указанную зону и релоадит список дропов уже без нее
 */
void reload(int zone_vnum) {
	group_obj_list.clear();
	solo_obj_list.clear();
	//mob_name_list.clear();
	//group_mob_list.clear();
	//solo_mob_list.clear();
	drop_list.clear();
	help_list.clear();

	if (zone_vnum > 0) {
		mob_stat::ClearZoneStat(zone_vnum);
	}

	init_obj_list();
	init_drop_system();
	init_link_system();

	HelpSystem::reload(HelpSystem::DYNAMIC);

	message_for_players();
}

/**
 * Инициализация системы дропа сетов при старте мада.
 * При наличии валидной таблицы из файла - инициализируется только статистика
 * по мобам, таблица дропа заполняется сразу из файла.
 * init_obj_list() дергается в любом случае из-за алиасов сетов для справки,
 * даже если там будут какие-то несоответствия с файлом дропа - всеравно будет
 * работать 'справка сеты', максимум не будет справки по алиасам.
 */
void init() {
	init_obj_list();

	if (!load_drop_table() || !load_unique_mobs()) {
		init_drop_system();
		save_unique_mobs();
	}

	init_link_system();
}

// * \return рнум шмотки или -1 если дропать нечего
int check_mob(int mob_rnum) {
	int rnum = -1;

	std::map<int, DropNode>::iterator it = drop_list.find(mob_rnum);
	if (it != drop_list.end()
		&& it->second.chance > 0) {
		const int num = obj_proto.actual_count(it->second.obj_rnum);
		// груп сетины по старой системе
		if (!it->second.solo) {
			if (num < GetObjMIW(it->second.obj_rnum)
				&& number(0, 1000) <= it->second.chance) {
				rnum = it->second.obj_rnum;
			}
			return rnum;
		}
//		log("->sd: %d", it->second.ObjVnum);
		// соло сетины - на необходимость резета проверяется вся группа
		if (it->second.linked_mobs.need_reset()) {
			log("reset");
			it->second.linked_mobs.reset();
		}
		// +0% если все 4 шмотки в лоаде
		// +0.1% если 3 из 4 шмоток в лоаде
		// +0.2% если 2 из 4 шмоток в лоаде
		// +0.3% если 1 из 4 шмоток в лоаде
		// таже система, если список в целом меньше 4
		const int mobs_count = static_cast<int>(it->second.linked_mobs.list_size());
		const int drop_count = it->second.linked_mobs.drop_count();
		const int drop_mod = mobs_count - drop_count;
//		log("list_size=%d, drop_count=%d, drop_mod=%d", mobs_count, drop_count, drop_mod);
//		log("num=%d, miw=%d", num, GetObjMIW(obj_proto[it->second.ObjRnum]));
		if (num < GetObjMIW(it->second.obj_rnum)) {
//			log("chance1=%d", it->second.drop_chance);
			it->second.chance += std::max(0, drop_mod);
//			log("chance2=%d", it->second.drop_chance);
			// собственно проверка на лоад
			if (it->second.chance >= 120 || number(0, 1000) <= it->second.chance) {
//				log("drop");
				// если шмоток две и более в мире - вторую нашару не дропаем
				it->second.reset_chance();
				rnum = it->second.obj_rnum;
			}
		} else {
//			log("chance3=%d", it->second.drop_chance);
			// шмотка не в лоаде, увеличиваем шансы как на дропе с проверкой переполнения
			it->second.chance += std::max(0, drop_mod);
//			log("chance4=%d", it->second.drop_chance);
			if (it->second.chance > 1000) {
//				log("reset");
				it->second.reset_chance();
			}
		}
//		log("<-sd");

		need_save_drop_table = true;
		HelpSystem::reload(HelpSystem::DYNAMIC);
	}

	return rnum;
}

void renumber_obj_rnum(const int mob_rnum) {
	if (mob_rnum < 0) {
		snprintf(buf, kMaxStringLength,
				 "SetsDrop: renumber_obj_rnum wrong parameters...");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	if (mob_rnum > -1) {
		std::map<int, DropNode> tmp_list;
		for (std::map<int, DropNode>::iterator it = drop_list.begin(),
				 iend = drop_list.end(); it != iend; ++it) {
			if (it->first >= mob_rnum) {
				tmp_list.insert(std::make_pair(it->first + 1, it->second));
			} else {
				tmp_list.insert(std::make_pair(it->first, it->second));
			}
		}
		drop_list = tmp_list;
	}
}

void print_timer_str(CharData *ch) {
	char time_buf[17];
	strftime(time_buf, sizeof(time_buf), "%H:%M %d-%m-%Y", localtime(&next_reset_time));

	std::stringstream out;
	out << "Следующее обновление таблицы: " << time_buf;

	const int minutes = (next_reset_time - time(0)) / 60;
	if (minutes >= 60) {
		out << " (" << minutes / 60 << "ч)";
	} else if (minutes >= 1) {
		out << " (" << minutes << "м)";
	} else {
		out << " (меньше минуты)";
	}
	out << "\r\n";

	SendMsgToChar(out.str().c_str(), ch);
}

} // namespace SetsDrop

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
