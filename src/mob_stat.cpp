// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "mob_stat.h"

#include "utils/pugixml/pugixml.h"
#include "utils/parse.h"
#include "entities/char_data.h"
#include "color.h"
#include "structs/global_objects.h"

namespace char_stat {

/// мобов убито - для 'статистика'
int mobs_killed = 0;
/// игроков убито - для 'статистика'
int players_killed = 0;

using ClassExpStat = std::pair<ECharClass, long long int>;
using ExpStatArray = std::array<ClassExpStat, to_underlying(ECharClass::kLast) + 1>;
using ExpStatArrayPtr = std::unique_ptr<ExpStatArray>;
/// Опыт за ребут с делением по профам - для 'статистика'
std::unordered_map<ECharClass, unsigned long long> classes_exp_stat;

/*
 * Подобная инициализация небезопасна: всегда существует возможность, что кто-то сунется в структуру
 * статистики до ее инициализации.
 *  \todo Нужно сделать глобальное хранилище статистики в global_objects и засунуть это все туда
 *  инициализацию, соответственно, разместить в процедуре загрузки мада. ABYRVALG
 */
void InitClassesExpStat() {
	for ( const auto &it : MUD::Classes()) {
		if (it.IsAvailable() && !classes_exp_stat.contains(it.GetId())) {
			classes_exp_stat.emplace(it.GetId(), 0);
		}
	}
}

void AddClassExp(ECharClass class_id, int exp) {
	if (classes_exp_stat.empty()) {
		InitClassesExpStat();
	}
	if (exp > 0 && MUD::Classes().IsAvailable(class_id)) {
		classes_exp_stat[class_id] += exp;
	}
}

std::string PrintClassExpStat(const ECharClass id, unsigned long long top_exp) {
	std::ostringstream out;
	out << std::left << std::setw(15) << MUD::Classes()[id].GetPluralName() << " " << std::left << KICYN;
	const int points_amount{10};
	int stars{0};
	if (top_exp > 0) {
		stars = static_cast<int>(classes_exp_stat[id] / (top_exp / points_amount));
		for (int i = 0; i < stars; ++i) {
			out << "*";
		}
	}
	out << KNRM << std::setfill('.') << std::setw(points_amount - stars) << "";

	return out.str();
}

ExpStatArrayPtr BuildExpStatArray() {
	auto array_ptr = std::make_unique<ExpStatArray>();
	std::copy(classes_exp_stat.begin(), classes_exp_stat.end(), array_ptr->begin());
	std::sort(array_ptr->begin(), array_ptr->end(),
			  [](const ClassExpStat &l, const ClassExpStat&r) {
				  return l.second > r.second;
			  });
	return array_ptr;
}

void PrintClassesExpStat(std::ostringstream &out) {
	if (classes_exp_stat.empty()) {
		InitClassesExpStat();
	}
	auto tmp_array = BuildExpStatArray();
	const unsigned long long top_exp = (*tmp_array)[0].second;

	out << " Соотношения набранного с перезагрузки опыта:" << std::endl << std::endl;
	/*
	 * При более чем 2 столбцах будет работать неправильно.
	 * Но возиться сейчас с написанием универсальной функции не вижу смысла:
	 * нужен полноценный модуль форматирования таблиц, а не такие костыли по всему коду.
	 * \todo table format
	 */
	const int columns{2};
	const auto rows = std::ceil(tmp_array->size()/columns);
	auto firs_col = (*tmp_array).begin();
	auto second_col = firs_col;
	std::advance(second_col, rows);
	for (int i = 0; i < rows; ++i) {
		out << " "  << PrintClassExpStat(firs_col->first, top_exp);
		++firs_col;
		if (second_col < (*tmp_array).end()) {
			out << "  " << PrintClassExpStat(second_col->first, top_exp);
			++second_col;
		}
		out << std::endl;
	}
	out << std::endl;
}

void LogClassesExpStat() {
	auto tmp_array = BuildExpStatArray();
	const unsigned long long top_exp = (*tmp_array)[0].second;

	log("Saving class exp stats.");
	for (const auto & i : (*tmp_array)) {
		log("class_exp: %13s   %15lld   %3llu%%",
			MUD::Classes()[i.first].GetPluralCName(), i.second, top_exp != 0 ? i.second * 100 / top_exp : 0);
	}
}

} // namespace char_stat

namespace mob_stat {

//const char *kMobStatFile = LIB_PLRSTUFF"mob_stat.xml";
const char *kMobStatFileNew = LIB_PLRSTUFF"mob_stat_new.xml";
/// за сколько месяцев хранится статистика (+ текущий месяц)
const int kMobStatHistorySize = 6;
/// выборка кол-ва мобов для show stats при старте мада <months, mob-count>
std::map<int, int> count_stats;
std::map<int, int> kill_stats;
/// список мобов по внуму и месяцам

const time_t MobKillStat::default_date = 0;

MobStatRegister mob_stat_register;

/// month, year
std::pair<int, int> GetDate() {
	time_t curr_time = time(nullptr);
	struct tm *tmp_tm = localtime(&curr_time);
	return std::make_pair(tmp_tm->tm_mon + 1, tmp_tm->tm_year + 1900);
}

void Load() {
	mob_stat_register.clear();

	char buf_[kMaxInputLength];

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(kMobStatFileNew);
	if (!result) {
		snprintf(buf_, sizeof(buf_), "...%s", result.description());
		mudlog(buf_, CMP, kLevelImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("mob_list");
	if (!node_list) {
		snprintf(buf_, sizeof(buf_), "...<mob_stat_new> read fail");
		mudlog(buf_, CMP, kLevelImmortal, SYSLOG, true);
		return;
	}
	for (pugi::xml_node xml_mob = node_list.child("mob"); xml_mob;
		 xml_mob = xml_mob.next_sibling("mob")) {
		const int mob_vnum = parse::ReadAttrAsInt(xml_mob, "vnum");
		if (real_mobile(mob_vnum) < 0) {
			snprintf(buf_, sizeof(buf_),
					 "...bad mob attributes (vnum=%d)", mob_vnum);
			mudlog(buf_, CMP, kLevelImmortal, SYSLOG, true);
			continue;
		}

		time_t kill_date = MobKillStat::default_date;
		pugi::xml_attribute xml_date = xml_mob.attribute("date");
		if (xml_date) {
			kill_date = static_cast<time_t>(xml_date.as_ullong(kill_date));
		}

		// инит статы конкретного моба по месяцам
		MobKillStat tmp_time(kill_date);
		for (pugi::xml_node xml_time = xml_mob.child("t"); xml_time;
			 xml_time = xml_time.next_sibling("t")) {
			struct MobMonthKillStat tmp_mob;
			tmp_mob.month = parse::ReadAttrAsInt(xml_time, "m");
			tmp_mob.year = parse::ReadAttrAsInt(xml_time, "y");
			if (tmp_mob.month <= 0 || tmp_mob.month > 12 || tmp_mob.year <= 0) {
				snprintf(buf_, sizeof(buf_),
						 "...bad mob attributes (month=%d, year=%d)",
						 tmp_mob.month, tmp_mob.year);
				mudlog(buf_, CMP, kLevelImmortal, SYSLOG, true);
				continue;
			}
			auto date = GetDate();
			if ((date.first + date.second * 12)
				- (tmp_mob.month + tmp_mob.year * 12) > kMobStatHistorySize + 1) {
				continue;
			}
			count_stats[tmp_mob.month] += 1;
			for (int k = 0; k <= kMaxGroupSize; ++k) {
				snprintf(buf_, sizeof(buf_), "n%d", k);
				pugi::xml_attribute attr = xml_time.attribute(buf_);
				if (attr && attr.as_int() > 0) {
					tmp_mob.kills.at(k) = attr.as_int();

					if (k > 0) {
						kill_stats[tmp_mob.month] += attr.as_int();
					}
				}
			}
			tmp_time.stats.push_back(tmp_mob);
		}
		mob_stat_register.emplace(mob_vnum, tmp_time);
	}
}

void Save() {
	pugi::xml_document doc;
	doc.append_child().set_name("mob_list");
	pugi::xml_node xml_mob_list = doc.child("mob_list");
	char buf_[kMaxInputLength];

	for (const auto & i : mob_stat_register) {
		pugi::xml_node mob_node = xml_mob_list.append_child();
		mob_node.set_name("mob");
		mob_node.append_attribute("vnum") = i.first;
		mob_node.append_attribute("date") = static_cast<unsigned long long>(i.second.date);
		// стата по месяцам
		for (const auto & stat : i.second.stats) {
			pugi::xml_node time_node = mob_node.append_child();
			time_node.set_name("t");
			time_node.append_attribute("m") = stat.month;
			time_node.append_attribute("y") = stat.year;
			for (int g = 0; g <= kMaxGroupSize; ++g) {
				if (stat.kills.at(g) > 0) {
					snprintf(buf_, sizeof(buf_), "n%d", g);
					time_node.append_attribute(buf_) = stat.kills.at(g);
				}
			}
		}
	}
	doc.save_file(kMobStatFileNew);
}

void ClearZoneStat(ZoneVnum zone_vnum) {
	for (auto i = mob_stat_register.begin(), iend = mob_stat_register.end(); i != iend; /**/) {
		if (i->first / 100 == zone_vnum) {
			mob_stat_register.erase(i++);
		} else {
			++i;
		}
	}
	Save();
}

void ShowStats(CharData *ch) {
	std::stringstream out;
	out << "  Всего уникальных мобов в статистике убийств: "
		<< mob_stat_register.size() << std::endl
		<< "  Количество уникальных мобов по месяцам:";
	for (auto & count_stat : count_stats) {
		out << " " << std::setw(2) << std::setfill('0') << count_stat.first << ":" << count_stat.second;
	}

	out << "\r\n" << "  Количество убитых мобов по месяцам:";
	for (auto & kill_stat : kill_stats) {
		out << " " << std::setw(2) << std::setfill('0') << kill_stat.first << ":" << kill_stat.second;
	}

	out << std::endl;
	send_to_char(out.str(), ch);
}

void UpdateMobNode(std::list<MobMonthKillStat> &node_list, int members) {
	auto date = GetDate();
	auto k = node_list.rbegin();
	const int months = k->month + k->year * 12;
	// сравнение номера месяца с переключением на следующий
	if (months == date.first + date.second * 12) {
		k->kills.at(members) += 1;
	} else {
		struct MobMonthKillStat node;
		node.month = date.first;
		node.year = date.second;
		node.kills.at(members) += 1;
		node_list.push_back(node);
		// проверка на переполнение кол-ва месяцев
		if (node_list.size() > kMobStatHistorySize + 1) {
			node_list.erase(node_list.begin());
		}
	}
}

time_t GetMobKilllastTime(MobVnum vnum) {
	auto i = mob_stat_register.find(vnum);
	if (i != mob_stat_register.end() && i->second.date != 0) {
		const auto killtime = i->second.date;
		return killtime;
	} else {
		return 0;
	}
}

void GetLastMobKill(CharData *mob, std::string &result) {
	auto i = mob_stat_register.find(GET_MOB_VNUM(mob));
	if (i != mob_stat_register.end() && i->second.date != 0) {
		const auto killtime = i->second.date;
		result = asctime(localtime(&killtime));
	} else {
		result = "никогда!\r\n";
	}

}
void AddMob(CharData *mob, int members) {
	if (members < 0 || members > kMaxGroupSize) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_),
				 "SYSERROR: MobVnum=%d, members=%d (%s:%d)",
				 GET_MOB_VNUM(mob), members, __FILE__, __LINE__);
		mudlog(buf_, CMP, kLevelImmortal, SYSLOG, true);
		return;
	}
	auto i = mob_stat_register.find(GET_MOB_VNUM(mob));
	if (i != mob_stat_register.end() && !i->second.stats.empty()) {
		i->second.date = time(nullptr);
		UpdateMobNode(i->second.stats, members);
	} else {
		struct MobMonthKillStat node;
		auto date = GetDate();
		node.month = date.first;
		node.year = date.second;
		node.kills.at(members) += 1;
		MobKillStat list_node;
		list_node.stats.push_back(node);
		list_node.date = time(nullptr);

		mob_stat_register[GET_MOB_VNUM(mob)] = list_node;
	}
	if (members == 0) {
		++char_stat::players_killed;
	} else {
		++char_stat::mobs_killed;
	}
}

std::string PrintMobName(int mob_vnum, unsigned int len) {
	std::string name = "null";
	const int rnum = real_mobile(mob_vnum);
	if (rnum > 0 && rnum <= top_of_mobt) {
		name = mob_proto[rnum].get_name();
	}
	if (name.size() >= len) {
		name = name.substr(0, len);
	}
	return name;
}

MobMonthKillStat SumStat(const std::list<MobMonthKillStat> &mob_list, int months) {
	auto date = GetDate();
	const int min_month = (date.first + date.second * 12) - months;
	struct MobMonthKillStat tmp_stat;

	for (auto i = mob_list.rbegin(), iend = mob_list.rend(); i != iend; ++i) {
		if (months == 0 || min_month < (i->month + i->year * 12)) {
			for (int k = 0; k <= kMaxGroupSize; ++k) {
				tmp_stat.kills.at(k) += i->kills.at(k);
			}
		}
	}
	return tmp_stat;
}

void ShowZoneMobKillsStat(CharData *ch, ZoneVnum zone_vnum, int months) {
	std::map<int, MobMonthKillStat> sort_list;
	for (auto & i : mob_stat_register) {
		if (i.first / 100 == zone_vnum) {
			MobMonthKillStat sum = SumStat(i.second.stats, months);
			sort_list.insert(std::make_pair(i.first, sum));
		}
	}

	std::stringstream out;
	out << "Статистика убийств мобов в зоне номер " << zone_vnum
		<< ", месяцев: " << months << "\r\n"
									  "   vnum : имя : pk : группа = убийств (n3=100 моба убили 100 раз втроем)\r\n\r\n";

	for (auto & i : sort_list) {
		out << i.first << " : " << std::setw(20)
			<< PrintMobName(i.first, 20) << " : "
			<< i.second.kills.at(0) << " :";
		for (int g = 1; g <= kMaxGroupSize; ++g) {
			if (i.second.kills.at(g) > 0) {
				out << " n" << g << "=" << i.second.kills.at(g);
			}
		}
		out << std::endl;
	}

	send_to_char(out.str().c_str(), ch);
}

} // namespace mob_stat

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
