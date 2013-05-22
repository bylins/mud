// Part of Bylins http://www.mud.ru

#include <sstream>
#include <iostream>
#include <set>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "sets_drop.hpp"
#include "db.h"
#include "utils.h"
#include "char.hpp"
#include "comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "im.h"
#include "room.hpp"
#include "pugixml.hpp"
#include "modify.h"
#include "house.h"
#include "screen.h"

extern int top_of_helpt;
extern struct help_index_element *help_table;
extern int hsort(const void *a, const void *b);
extern void go_boot_xhelp(void);

namespace SetsDrop
{

// список сетин на дроп
const char *CONFIG_FILE = LIB_MISC"full_set_drop.xml";
// минимальный уровень моба для участия в груп-списке дропа
const int MIN_GROUP_MOB_LVL = 32;
// мин/макс уровни мобов для выборки соло-сетин
const int MIN_SOLO_MOB_LVL = 25;
const int MAX_SOLO_MOB_LVL = 31;
// макс. кол-во участников в группе учитываемое в статистике
const int MAX_GROUP_SIZE = 12;
const char *MOB_STAT_FILE = LIB_PLRSTUFF"mob_stat.xml";
const char *DROP_TABLE_FILE = LIB_PLRSTUFF"sets_drop.xml";
// сброс таблицы лоада каждый х часов
const int RESET_TIMER = 35;
// сообщение игрокам при резете таблицы дропа
const char *RESET_MESSAGE =
	"Внезапно мир содрогнулся, день поменялся с ночью, земля с небом\r\n"
	"...но через миг все вернулось на круги своя.";

enum { SOLO_MOB, GROUP_MOB, SOLO_ZONE, GROUP_ZONE };

// время следующего сброса таблицы
time_t next_reset_time = 0;

// список груп-сетин на лоад (vnum)
std::list<int> group_obj_list;
// список соло-сетин на лоад (vnum)
std::list<int> solo_obj_list;

// статистика по мобам: vnum моба, размер группы, кол-во убийств
std::map<int, std::vector<int> > mob_stat;

struct MobNode
{
	MobNode() : vnum(-1), rnum(-1), miw(-1),
		type(-1), kill_stat(MAX_GROUP_SIZE + 1, 0) {};

	int vnum;
	int rnum;
	// макс.в.мире
	int miw;
	// имя моба (для фильтрации)
	std::string name;
	// груп/соло моб
	int type;
	// статистика убиств по размеру группы
	std::vector<int> kill_stat;
};

struct ZoneNode
{
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

struct DropNode
{
	DropNode() : obj_rnum(0), chance(0), solo(false) {};
	// рнум сетины
	int obj_rnum;
	// шанс дропа (проценты * 10), chance из 1000
	int chance;
	// из соло или груп моб листа эта сетина (для генерации справки)
	bool solo;
};

// финальный список дропа по мобам (mob_rnum)
std::map<int, DropNode> drop_list;

struct HelpNode
{
	// список алиасов для справки
	std::string alias_list;
	// полное имя сета для текста справки
	std::string title;
	// список предметов
	std::set<int> vnum_list;
};
// список соответствий алиасов сетов для справки
std::vector<HelpNode> help_list;

// * Инициализация списка сетов на лоад.
void init_obj_list()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("set_list");
    if (!node_list)
    {
		mudlog("...<set_list> read fail", CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	for (pugi::xml_node set_node = node_list.child("set");
		set_node; set_node = set_node.next_sibling("set"))
	{
		HelpNode node;

		node.alias_list = xmlparse_str(set_node, "help_alias");
		if (node.alias_list.empty())
		{
			mudlog("...bad set attributes (empty help_alias)",
				CMP, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		std::string type = xmlparse_str(set_node, "type");
		if (type.empty() || (type != "auto" && type != "manual"))
		{
			snprintf(buf, sizeof(buf),
				"...bad set attributes (type=%s)", type.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		if (type == "manual")
		{
			for (pugi::xml_node obj_node = set_node.child("obj");
				obj_node; obj_node = obj_node.next_sibling("obj"))
			{
				const int obj_vnum = xmlparse_int(obj_node, "vnum");
				if (real_object(obj_vnum) < 0)
				{
					snprintf(buf, sizeof(buf),
						"...bad obj_node attributes (vnum=%d)", obj_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				std::string list_type = xmlparse_str(obj_node, "list");
				if (list_type.empty()
					|| (list_type != "solo" && list_type != "group"))
				{
					snprintf(buf, sizeof(buf),
						"...bad manual obj attributes (list=%s, obj_vnum=%d)",
						list_type.c_str(), obj_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (list_type == "solo")
				{
					solo_obj_list.push_back(obj_vnum);
				}
				else
				{
					group_obj_list.push_back(obj_vnum);
				}
				// список сетин для справки
				node.vnum_list.insert(obj_vnum);
				// имя сета
				if (node.title.empty())
				{
					for (id_to_set_info_map::const_iterator it = obj_data::set_table.begin(),
						iend = obj_data::set_table.end(); it != iend; ++it)
					{
						set_info::const_iterator k = it->second.find(obj_vnum);
						if (k != it->second.end())
						{
							node.title = it->second.get_name();
						}
					}
				}
			}
		}
		else
		{
			// список сета сортированный по макс.активаторам
			std::multimap<int, int> set_sort_list;

			for (pugi::xml_node obj_node = set_node.child("obj");
				obj_node; obj_node = obj_node.next_sibling("obj"))
			{
				const int obj_vnum = xmlparse_int(obj_node, "vnum");
				if (real_object(obj_vnum) < 0)
				{
					snprintf(buf, sizeof(buf),
						"...bad obj_node attributes (vnum=%d)", obj_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}
				// заполнение списка активаторов
				for (id_to_set_info_map::const_iterator it = obj_data::set_table.begin(),
					iend = obj_data::set_table.end(); it != iend; ++it)
				{
					set_info::const_iterator k = it->second.find(obj_vnum);
					if (k != it->second.end() && !k->second.empty())
					{
						// берется последний (максимальный) в списке активатор
						set_sort_list.insert(
							std::make_pair(k->second.rbegin()->first, obj_vnum));
						// список сетин для справки
						node.vnum_list.insert(obj_vnum);
						// имя сета
						if (node.title.empty())
						{
							node.title = it->second.get_name();
						}
					}
				}
			}
			// первая половина активаторов в соло-лист, вторая в групп
			int num = 0, total_num = set_sort_list.size();
			for (std::multimap<int, int>::const_iterator i = set_sort_list.begin(),
				iend = set_sort_list.end(); i != iend; ++i, ++num)
			{
				if (num < total_num / 2)
				{
					solo_obj_list.push_back(i->second);
				}
				else
				{
					group_obj_list.push_back(i->second);
				}
			}
		}
		// список алиасов и сетин для справки
		help_list.push_back(node);
	}
}

// * Лоад статистики по убийствам мобов с делением на размер группы.
void init_mob_stat()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOB_STAT_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("mob_list");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<mob_list> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	for (pugi::xml_node mob_node = node_list.child("mob"); mob_node; mob_node = mob_node.next_sibling("mob"))
	{
		int mob_vnum = xmlparse_int(mob_node, "vnum");
		if (real_mobile(mob_vnum) < 0)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...bad mob attributes (vnum=%d)", mob_vnum);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		std::vector<int> node(MAX_GROUP_SIZE + 1, 0);

		for (int k = 1; k <= MAX_GROUP_SIZE; ++k)
		{
			snprintf(buf, sizeof(buf), "n%d", k);
			pugi::xml_attribute attr = mob_node.attribute(buf);
			if (attr && attr.as_int() > 0)
			{
				node[k] = attr.as_int();
			}
		}

		mob_stat.insert(std::make_pair(mob_vnum, node));
	}
}

void add_to_zone_list(std::list<ZoneNode> &cont, MobNode &node)
{
	int zone = node.vnum/100;
	std::list<ZoneNode>::iterator k = std::find_if(cont.begin(), cont.end(),
		boost::bind(std::equal_to<int>(),
		boost::bind(&ZoneNode::zone, _1), zone));

	if (k != cont.end())
	{
		k->mobs.push_back(node);
	}
	else
	{
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
void init_mob_name_list()
{
	std::set<int> bad_zones;

	// клан-замки
	for (ClanListType::const_iterator clan = Clan::ClanList.begin();
		clan != Clan::ClanList.end(); ++clan)
	{
		bad_zones.insert((*clan)->GetRent()/100);
	}

	// города
	int curr_zone = 0;
	bool rent = false, mail = false, banker = false;
	for (std::vector<ROOM_DATA *>::const_iterator i = world.begin(),
		iend = world.end(); i != iend; ++i)
	{
		if (curr_zone != zone_table[(*i)->zone].number)
		{
			if (rent && mail && banker)
			{
				bad_zones.insert(curr_zone);
			}
			rent = false;
			mail = false;
			banker = false;
			curr_zone = zone_table[(*i)->zone].number;
		}
		for (CHAR_DATA *ch = (*i)->people; ch; ch = ch->next_in_room)
		{
			if (IS_RENTKEEPER(ch))
			{
				rent = true;
			}
			else if (IS_POSTKEEPER(ch))
			{
				mail = true;
			}
			else if (IS_BANKKEEPER(ch))
			{
				banker = true;
			}
		}
	}

	// тестовые зоны
	for (int nr = 0; nr <= top_of_zone_table; nr++)
	{
		if (zone_table[nr].under_construction)
		{
			bad_zones.insert(zone_table[nr].number);
		}
	}

	for (std::map<int, std::vector<int> >::iterator i = mob_stat.begin(),
		iend = mob_stat.end(); i != iend; ++i)
	{
		const int rnum = real_mobile(i->first);
		const int zone = i->first/100;
		std::set<int>::const_iterator k = bad_zones.find(zone);

		if (rnum < 0
			|| zone < 100
			|| k != bad_zones.end())
		{
			continue;
		}

		MobNode node;
		node.vnum = i->first;
		node.rnum = rnum;
		node.name = mob_proto[rnum].get_name();
		node.kill_stat = i->second;

		add_to_zone_list(mob_name_list, node);
	}
}

// * Инится у зоны: тип
void init_zone_type()
{
	for (std::list<ZoneNode>::iterator i = mob_name_list.begin(),
		iend = mob_name_list.end(); i != iend; ++i)
	{
		int killed_solo = 0;
		for (std::list<MobNode>::iterator k = i->mobs.begin(),
			kend = i->mobs.end(); k != kend; ++k)
		{
			int group_cnt = 0;
			for (int cnt = 2; cnt <= MAX_GROUP_SIZE; ++cnt)
			{
				group_cnt += k->kill_stat[cnt];
			}
			if (k->kill_stat[1] > group_cnt)
			{
				++killed_solo;
			}
		}
		if (killed_solo >= i->mobs.size() * 0.8)
		{
			i->type = SOLO_ZONE;
		}
		else
		{
			i->type = GROUP_ZONE;
		}
	}
}

// * Инится у моба: тип
void init_mob_type()
{
	for (std::list<ZoneNode>::iterator i = mob_name_list.begin(),
		iend = mob_name_list.end(); i != iend; ++i)
	{
		for (std::list<MobNode>::iterator k = i->mobs.begin(),
			kend = i->mobs.end(); k != kend; ++k)
		{
			int group_cnt = 0;
			for (int cnt = 2; cnt <= MAX_GROUP_SIZE; ++cnt)
			{
				group_cnt += k->kill_stat[cnt];
			}
			if (i->type == SOLO_ZONE && k->kill_stat[1] > group_cnt)
			{
				k->type = SOLO_MOB;
			}
			else if (i->type == GROUP_ZONE && k->kill_stat[1] < group_cnt)
			{
				k->type = GROUP_MOB;
			}
		}
	}
}

/**
 * Расчет макс.в.мире на основе поля резета зоны.
 * Если мест лоада несколько - берется максимальное значение.
 */
int calc_max_in_world(int mob_rnum)
{
	int max_in_world = 0;
	for (int i = 0; i <= top_of_zone_table; ++i)
	{
		for (int cmd_no = 0; zone_table[i].cmd[cmd_no].command != 'S'; ++cmd_no)
		{
			if (zone_table[i].cmd[cmd_no].command == 'M'
				&& zone_table[i].cmd[cmd_no].arg1 == mob_rnum)
			{
				max_in_world = MAX(max_in_world, zone_table[i].cmd[cmd_no].arg2);
			}
		}
	}
	return max_in_world;
}

/**
 * С этого момента начинают отсекаться отдельные мобы
 * Отсекаются: мобы с неуникальным именем внутри одной зоны,
 *             мобы больше 1 макс.в.мире
 *             мобы ниже требуемого уровня для данного типа
 *             мобы с флагами только полнолуние/времена года
 * Инится у моба: макс.в.мире
 */
void filter_dupe_names()
{
	for (std::list<ZoneNode>::iterator it = mob_name_list.begin(),
		iend = mob_name_list.end(); it != iend; ++it)
	{
		std::list<MobNode> tmp_list;
		// отсеиваем (включая оригинал) одинаковые имена с разными внумами
		for (std::list<MobNode>::iterator k = it->mobs.begin(),
			kend = it->mobs.end(); k != kend; ++k)
		{
			// одинаковые имена в пределах зоны
			bool good = true;
			for (std::list<MobNode>::iterator l = it->mobs.begin(),
				lend = it->mobs.end(); l != lend; ++l)
			{
				if (k->vnum != l->vnum && k->name == l->name)
				{
					good = false;
					break;
				}
			}
			if (!good || k->type == -1)
			{
				continue;
			}
			// проверка на левел моба
			if (k->type == SOLO_MOB
				&& (mob_proto[k->rnum].get_level() < MIN_SOLO_MOB_LVL
					|| mob_proto[k->rnum].get_level() > MAX_SOLO_MOB_LVL))
			{
				continue;
			}
			if (k->type == GROUP_MOB
				&& mob_proto[k->rnum].get_level() < MIN_GROUP_MOB_LVL)
			{
				continue;
			}
			// редко появляющиеся мобы, мобы без экспы
			const CHAR_DATA *mob = &mob_proto[k->rnum];
			if (MOB_FLAGGED(mob, MOB_LIKE_FULLMOON)
				|| MOB_FLAGGED(mob, MOB_LIKE_WINTER)
				|| MOB_FLAGGED(mob, MOB_LIKE_SPRING)
				|| MOB_FLAGGED(mob, MOB_LIKE_SUMMER)
				|| MOB_FLAGGED(mob, MOB_LIKE_AUTUMN)
				|| mob->get_exp() <= 0)
			{
				continue;
			}
			// пока только уникальные мобы
			k->miw = calc_max_in_world(k->rnum);
			if (k->miw != 1)
			{
				continue;
			}

			tmp_list.push_back(*k);
		}
		it->mobs = tmp_list;
	}
}

/**
 * Отсекаются: лишние мобы в зонах с самым длинным списком мобов
 * для более равномерного распределения по разным зонам
 */
void filter_extra_mobs(int total, int type)
{
	std::list<ZoneNode> &cont = (type == GROUP_MOB) ? group_mob_list : solo_mob_list;
	const int obj_total = (type == GROUP_MOB) ? group_obj_list.size() : solo_obj_list.size();
	// обрезание лишних мобов в самых заполненных зонах
	int num_del = total - obj_total;
	while (num_del > 0)
	{
		unsigned max_num = 0;
		for (std::list<ZoneNode>::iterator it = cont.begin(),
			iend = cont.end(); it != iend; ++it)
		{
			if (it->mobs.size() > max_num)
			{
				max_num = it->mobs.size();
			}
		}
		for (std::list<ZoneNode>::iterator it = cont.begin(),
			iend = cont.end(); it != iend; ++it)
		{
			if (it->mobs.size() >= max_num)
			{
				std::list<MobNode>::iterator l = it->mobs.begin();
				std::advance(l, number(0, it->mobs.size() - 1));
				it->mobs.erase(l);
				if (it->mobs.empty())
				{
					cont.erase(it);
				}
				--num_del;
				break;
			}
		}
	}
}

// * Разделение общего списка мобов на групп- и соло- списки.
void split_mob_name_list()
{
	int total_group_mobs = 0, total_solo_mobs = 0;

	for (std::list<ZoneNode>::iterator it = mob_name_list.begin(),
		iend = mob_name_list.end(); it != iend; ++it)
	{
		for (std::list<MobNode>::iterator k = it->mobs.begin(),
			kend = it->mobs.end(); k != kend; ++k)
		{
			if (k->type == GROUP_MOB)
			{
				add_to_zone_list(group_mob_list, *k);
				++total_group_mobs;
			}
			else if (k->type == SOLO_MOB)
			{
				add_to_zone_list(solo_mob_list, *k);
				++total_solo_mobs;
			}
		}
	}
	mob_name_list.clear();

	filter_extra_mobs(total_group_mobs, GROUP_MOB);
	filter_extra_mobs(total_solo_mobs, SOLO_MOB);
}

int calc_drop_chance(std::list<MobNode>::iterator &mob, int obj_rnum)
{
	int chance = 0;

	if (mob->type == GROUP_MOB)
	{
		// два состава максимальных по кол-ву убийств
		int max_kill = 0;
		int num1 = 2;
		// в два цикла как-то нагляднее
		for (int i = 2; i <= MAX_GROUP_SIZE; ++i)
		{
			if (mob->kill_stat[i] > max_kill)
			{
				max_kill = mob->kill_stat[i];
				num1 = i;
			}
		}
		int max_kill2 = 0;
		int num2 = 2;
		for (int i = 2; i <= MAX_GROUP_SIZE; ++i)
		{
			if (i != num1
				&& mob->kill_stat[i] > max_kill2)
			{
				max_kill2 = mob->kill_stat[i];
				num2 = i;
			}
		}
		// и среднее между ними
		double num_mod = (num1 + num2) / 2.0;
		// 5.8% .. 14.8%
		chance = (40 + num_mod * 9) / mob->miw;
	}
	else
	{
		// 3% .. 4.2%
		int mob_lvl = mob_proto[mob->rnum].get_level();
		int lvl_mod = MIN(MAX(0, mob_lvl - MIN_SOLO_MOB_LVL), 6);
		chance = (30 + lvl_mod * 2) / mob->miw;
		// мини сеты в соло увеличенный шанс на дроп
		const OBJ_DATA *obj = obj_proto[obj_rnum];
		if (!SetSystem::is_big_set(obj))
		{
			chance *= 1.75;
		}
	}

	return chance;
}

// * Генерация финальной таблицы дропа с мобов.
void init_drop_table(int type)
{
	std::list<ZoneNode> &mob_list = (type == GROUP_MOB) ? group_mob_list : solo_mob_list;
	std::list<int> &obj_list = (type == GROUP_MOB) ? group_obj_list : solo_obj_list;

	while(!obj_list.empty() && !mob_list.empty())
	{
		std::list<int>::iterator it = obj_list.begin();
		std::advance(it, number(0, obj_list.size() - 1));
		const int obj_rnum = real_object(*it);

		std::list<ZoneNode>::iterator k = mob_list.begin();
		std::advance(k, number(0, mob_list.size() - 1));

		// по идее чистить надо сразу после удаления последнего
		// элемента, но тут может получиться ситуация, что на входе
		// изначально был пустой список после всяких фильтраций
		if (k->mobs.empty())
		{
			mob_list.erase(k);
			continue;
		}

		std::list<MobNode>::iterator l = k->mobs.begin();
		std::advance(l, number(0, k->mobs.size() - 1));

		DropNode tmp_node;
		tmp_node.obj_rnum = obj_rnum;
		tmp_node.chance = calc_drop_chance(l, obj_rnum);
		tmp_node.solo = (type == GROUP_MOB) ? false : true;

		drop_list.insert(std::make_pair(l->rnum, tmp_node));

		obj_list.erase(it);
		k->mobs.erase(l);
	}

	obj_list.clear();
	mob_list.clear();
}

void add_to_help_table(const std::vector<std::string> &key_list, const std::string &text)
{
	if (key_list.empty() || text.empty())
	{
		snprintf(buf, sizeof(buf), "add_to_help_table error: key_list or text empty");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	int num = key_list.size();
	RECREATE(help_table, struct help_index_element, top_of_helpt + num + 1);

	struct help_index_element el;
	el.min_level = 0;
	el.duplicate = 0;
	el.entry = str_dup(text.c_str());
	el.sets_drop_page = true;

	for (std::vector<std::string>::const_iterator i = key_list.begin(),
		iend = key_list.end(); i != iend; ++i)
	{
		el.keyword = str_dup((*i).c_str());
		help_table[++top_of_helpt] = el;
		++el.duplicate;
	}
	// немного избыточно в цикле, но зато все в одном месте
	qsort(help_table, top_of_helpt + 1, sizeof(struct help_index_element), hsort);
}

/**
 * Распечатка конкретного сета на основе ранее сформированного HelpNode.
 * С делением вывода на соло и груп сетины.
 */
std::string print_current_set(const HelpNode &node)
{
	std::stringstream solo_obj_names;
	std::vector<std::string> solo_mob_list;
	std::stringstream group_out;
	group_out.precision(1);
	int names_delim = -1;

	for (std::set<int>::const_iterator l = node.vnum_list.begin(),
		lend = node.vnum_list.end(); l != lend; ++l)
	{
		for (std::map<int, DropNode>::iterator k = drop_list.begin(),
				kend = drop_list.end(); k != kend; ++k)
		{
			if (obj_index[k->second.obj_rnum].vnum == *l)
			{
				if (k->second.solo == true)
				{
					if (names_delim == 1)
					{
						names_delim = 0;
						solo_obj_names << "\r\n   ";
					}
					else if (names_delim == 0)
					{
						names_delim = 1;
						solo_obj_names << ", ";
					}
					else
					{
						names_delim = 0;
						solo_obj_names << "   ";
					}
					solo_obj_names << GET_OBJ_PNAME(obj_proto[k->second.obj_rnum], 0);

					std::stringstream solo_out;
					solo_out.precision(1);
					solo_out << "    - " << mob_proto[k->first].get_name()
						<< " (" << zone_table[mob_index[k->first].zone].name << ")"
						<< " - " << std::fixed << k->second.chance / 10.0 << "%\r\n";
					solo_mob_list.push_back(solo_out.str());
				}
				else
				{
					group_out << "   " << GET_OBJ_PNAME(obj_proto[k->second.obj_rnum], 0)
						<< " - " << mob_proto[k->first].get_name()
						<< " (" << zone_table[mob_index[k->first].zone].name << ")"
						<< " - " << std::fixed << k->second.chance / 10.0 << "%\r\n";
				}
				break;
			}
		}
	}

	std::srand(std::time(0));
	std::random_shuffle(solo_mob_list.begin(), solo_mob_list.end());

	std::stringstream out;
	out << node.title << "\r\n";
	out << solo_obj_names.str() << "\r\n";
	for (std::vector<std::string>::const_iterator i = solo_mob_list.begin(),
		iend = solo_mob_list.end(); i != iend; ++i)
	{
		out << *i;
	}
	out << group_out.str();

	return out.str();
}

/**
 * Генерация 'справка сеты'.
 */
void init_xhelp()
{
	std::stringstream out;
	out << "Наборы предметов, участвующие в системе автоматического выпадения:\r\n";

	for (std::vector<HelpNode>::const_iterator i = help_list.begin(),
		iend = help_list.end(); i != iend; ++i)
	{
		out << "\r\n" << print_current_set(*i);
	}

	std::vector<std::string> help_list;
	help_list.push_back("сеты");
	help_list.push_back("сэты");
	help_list.push_back("наборыпредметов");
	add_to_help_table(help_list, out.str());
}

/**
 * Генерация справки по каждому набору отдельно.
 */
void init_xhelp_full()
{
	for (std::vector<HelpNode>::const_iterator i = help_list.begin(),
		iend = help_list.end(); i != iend; ++i)
	{
		std::string text = print_current_set(*i);
		std::vector<std::string> str_list;
		boost::split(str_list, i->alias_list, boost::is_any_of(", "));
		add_to_help_table(str_list, text);
	}
}

bool load_drop_table()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(DROP_TABLE_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return false;
	}

    pugi::xml_node node_list = doc.child("drop_list");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<drop_list> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return false;
    }

	pugi::xml_node time_node = node_list.child("time");
	std::string timer = xmlparse_str(time_node, "reset");
	if (!timer.empty())
	{
		try
		{
			next_reset_time = boost::lexical_cast<time_t>(timer);
		}
		catch(...)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"...timer (%s) lexical_cast fail", timer.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return false;
		}
	}
	else
	{
		mudlog("...empty reset time", CMP, LVL_IMMORT, SYSLOG, TRUE);
		return false;
	}

	for (pugi::xml_node item_node = node_list.child("item"); item_node;
		item_node = item_node.next_sibling("item"))
	{
		const int obj_vnum = xmlparse_int(item_node, "vnum");
		const int obj_rnum = real_object(obj_vnum);
		if (obj_vnum <= 0 || obj_rnum < 0)
		{
			snprintf(buf, sizeof(buf),
				"...bad item attributes (vnum=%d)", obj_vnum);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return false;
		}

		const int mob_vnum = xmlparse_int(item_node, "mob");
		const int mob_rnum = real_mobile(mob_vnum);
		if (mob_vnum <= 0 || mob_rnum < 0)
		{
			snprintf(buf, sizeof(buf),
				"...bad item attributes (mob=%d)", mob_vnum);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return false;
		}

		const int chance = xmlparse_int(item_node, "chance");
		if (chance <= 0)
		{
			snprintf(buf, sizeof(buf),
				"...bad item attributes (chance=%d)", chance);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return false;
		}

		std::string solo = xmlparse_str(item_node, "solo");
		if (solo.empty())
		{
			snprintf(buf, sizeof(buf), "...bad item attributes (solo=empty)");
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return false;
		}

		DropNode tmp_node;
		tmp_node.obj_rnum = obj_rnum;
		tmp_node.chance = chance;
		tmp_node.solo = solo == "true" ? true : false;
		drop_list.insert(std::make_pair(mob_rnum, tmp_node));
	}
	return true;
}

void save_drop_table()
{
	pugi::xml_document doc;
	doc.append_child().set_name("drop_list");
	pugi::xml_node node_list = doc.child("drop_list");

	next_reset_time = time(0) + 60 * 60 * RESET_TIMER;
	pugi::xml_node time_node = node_list.append_child();
	time_node.set_name("time");
	time_node.append_attribute("reset") = boost::lexical_cast<std::string>(next_reset_time).c_str();

	for (std::map<int, DropNode>::iterator i = drop_list.begin(),
		iend = drop_list.end(); i != iend; ++i)
	{
		pugi::xml_node mob_node = node_list.append_child();
		mob_node.set_name("item");
		mob_node.append_attribute("vnum") = obj_index[i->second.obj_rnum].vnum;
		mob_node.append_attribute("mob") = mob_index[i->first].vnum;
		mob_node.append_attribute("chance") = i->second.chance;
		mob_node.append_attribute("solo") = i->second.solo ? "true" : "false";
	}

	doc.save_file(DROP_TABLE_FILE);
}

void clear_zone_stat(int zone_vnum)
{
	for (std::map<int, std::vector<int> >::iterator i = mob_stat.begin(),
		iend = mob_stat.end(); i != iend; /* пусто */)
	{
		if (i->first/100 == zone_vnum)
		{
			mob_stat.erase(i++);
		}
		else
		{
			++i;
		}
	}
	save_mob_stat();
}

void init_drop_system()
{
	init_mob_name_list();
	init_zone_type();
	init_mob_type();
	filter_dupe_names();
	split_mob_name_list();
	init_drop_table(SOLO_MOB);
	init_drop_table(GROUP_MOB);
	save_drop_table();
}

void reload_by_timer()
{
	if (next_reset_time <= time(0))
	{
		reload();
	}
}

void message_for_players()
{
	for (DESCRIPTOR_DATA *i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING && i->character)
		{
			send_to_char(i->character, "%s%s%s\r\n",
				CCICYN(i->character, C_NRM), RESET_MESSAGE,
				CCNRM(i->character, C_NRM));
		}
	}
}

/**
 * Релоад таблицы дропа, без релоада статистики по убийствам мобов.
 * \param zone_vnum - если не нулевой, удаляет из статистики мобов
 * всю указанную зону и релоадит список дропов уже без нее
 */
void reload(int zone_vnum)
{
	group_obj_list.clear();
	solo_obj_list.clear();
	mob_name_list.clear();
	group_mob_list.clear();
	solo_mob_list.clear();
	drop_list.clear();
	help_list.clear();

	if (zone_vnum > 0)
	{
		clear_zone_stat(zone_vnum);
	}

	init_obj_list();
	init_drop_system();

	// справку надо полностью срелоадить
	// init_xhelp() и init_xhelp_full() вызовется там же
	go_boot_xhelp();

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
void init()
{
	init_mob_stat();
	init_obj_list();

	if (!load_drop_table())
	{
		init_drop_system();
	}

	init_xhelp();
	init_xhelp_full();
}

void show_stats(CHAR_DATA *ch)
{
	send_to_char(ch, "  Мобов в статистике для сетов: %d\r\n", mob_stat.size());
}

// * \return рнум шмотки или -1 если дропать нечего
int check_mob(int mob_rnum)
{
	std::map<int, DropNode>::iterator it = drop_list.find(mob_rnum);
	if (it != drop_list.end())
	{
		int num = obj_index[it->second.obj_rnum].stored
			+ obj_index[it->second.obj_rnum].number;
		if (num < GET_OBJ_MIW(obj_proto[it->second.obj_rnum])
			&& number(0, 1000) <= it->second.chance)
		{
			return it->second.obj_rnum;
		}
	}
	return -1;
}

void renumber_obj_rnum(const int rnum, const int mob_rnum)
{
	if(rnum < 0 && mob_rnum < 0)
	{
		snprintf(buf, MAX_STRING_LENGTH,
			"SetsDrop: renumber_obj_rnum wrong parameters...");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	if (rnum > -1)
	{
		for (std::map<int, DropNode>::iterator it = drop_list.begin(),
			iend = drop_list.end(); it != iend; ++it)
		{
			if (it->second.obj_rnum >= rnum)
			{
				it->second.obj_rnum += 1;
			}
		}
	}
	if (mob_rnum > -1)
	{
		std::map<int, DropNode> tmp_list;
		for (std::map<int, DropNode>::iterator it = drop_list.begin(),
			iend = drop_list.end(); it != iend; ++it)
		{
			if (it->first >= mob_rnum)
			{
				DropNode tmp_node;
				tmp_node.obj_rnum = it->second.obj_rnum;
				tmp_node.chance = it->second.chance;
				tmp_list.insert(std::make_pair(it->first + 1, tmp_node));
			}
			else
			{
				tmp_list.insert(std::make_pair(it->first, it->second));
			}
		}
		drop_list = tmp_list;
	}
}

void save_mob_stat()
{
	pugi::xml_document doc;
	doc.append_child().set_name("mob_list");
	pugi::xml_node mob_list = doc.child("mob_list");

	for (std::map<int, std::vector<int> >::const_iterator i = mob_stat.begin(),
		iend = mob_stat.end(); i != iend; ++i)
	{
		pugi::xml_node mob_node = mob_list.append_child();
		mob_node.set_name("mob");
		mob_node.append_attribute("vnum") = i->first;

		for (int k = 1; k <= MAX_GROUP_SIZE; ++k)
		{
			if (i->second[k] > 0)
			{
				snprintf(buf, sizeof(buf), "n%d", k);
				mob_node.append_attribute(buf) = i->second[k];
			}
		}
	}
	doc.save_file(MOB_STAT_FILE);
}

void add_mob_stat(CHAR_DATA *mob, int members)
{
	if (members < 1 || members > MAX_GROUP_SIZE)
	{
		snprintf(buf, sizeof(buf), "add_mob_stat error: mob_vnum=%d, members=%d",
			GET_MOB_VNUM(mob), members);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	std::map<int, std::vector<int> >::iterator i = mob_stat.find(GET_MOB_VNUM(mob));
	if (i != mob_stat.end())
	{
		i->second[members] += 1;
	}
	else
	{
		std::vector<int> node(MAX_GROUP_SIZE + 1, 0);
		node[members] += 1;
		mob_stat.insert(std::make_pair(GET_MOB_VNUM(mob), node));
	}
}

std::string print_mobstat_name(int mob_vnum)
{
	std::string name = "null";
	const int rnum = real_mobile(mob_vnum);
	if (rnum > 0 && rnum <= top_of_mobt)
	{
		name = mob_proto[rnum].get_name();
	}
	if (name.size() > 20)
	{
		name = name.substr(0, 20);
	}
	return name;
}

// * show mobstat
void show_zone_stat(CHAR_DATA *ch, int zone_vnum)
{
	std::stringstream out;
	out << "Статистика убийств мобов в зоне номер " << zone_vnum << "\r\n";
	out << "   vnum : имя : размер группы = кол-во убийств (n3=100 моба убили 100 раз втроем)\r\n\r\n";

	for (std::map<int, std::vector<int> >::const_iterator i = mob_stat.begin(),
		iend = mob_stat.end(); i != iend; ++i)
	{
		if (i->first/100 == zone_vnum)
		{
			out << i->first << " : " << std::setw(20)
				<< print_mobstat_name(i->first) << " :";
			for (int k = 1; k <= MAX_GROUP_SIZE; ++k)
			{
				if (i->second[k] > 0)
				{
					out << " n" << k << "=" << i->second[k];
				}
			}
			out << "\r\n";
		}
	}

	send_to_char(out.str().c_str(), ch);
}

void print_timer_str(CHAR_DATA *ch)
{
	char time_buf[17];
	strftime(time_buf, sizeof(time_buf), "%H:%M %d-%m-%Y", localtime(&next_reset_time));

	std::stringstream out;
	out << "Следующее обновление таблицы: " << time_buf;

	const int minutes = (next_reset_time - time(0)) / 60;
	if (minutes >= 60)
	{
		out << " (" << minutes / 60 << "ч)";
	}
	else if (minutes >= 1)
	{
		out << " (" << minutes << "м)";
	}
	else
	{
		out << " (меньше минуты)";
	}
	out << "\r\n";

	send_to_char(out.str().c_str(), ch);
}

} // namespace SetsDrop
