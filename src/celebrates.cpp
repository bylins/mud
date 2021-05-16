#include "celebrates.hpp"
#include "global.objects.hpp"
#include "logger.hpp"
#include "obj.hpp"
#include "comm.h"
#include "db.h"
#include "dg_script/dg_db_scripts.hpp"
#include "dg_script/dg_scripts.h"
#include "chars/character.h"
#include "room.hpp"
#include "handler.h"
#include "utils.h"
#include "sysdep.h"
#include "conf.h"
#include "pugixml.hpp"
#include "backtrace.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>

extern void extract_trigger(TRIG_DATA * trig);

namespace Celebrates
{
int tab_day [12]= {31,28,31,30,31,30,31,31,30,31,30,31};//да и хрен с ним, с 29 февраля!

CelebrateList& mono_celebrates = GlobalObjects::mono_celebrates();
CelebrateList& poly_celebrates = GlobalObjects::poly_celebrates();
CelebrateList& real_celebrates = GlobalObjects::real_celebrates();
CelebrateMobs& attached_mobs = GlobalObjects::attached_mobs();
CelebrateMobs& loaded_mobs = GlobalObjects::loaded_mobs();
CelebrateObjs& attached_objs = GlobalObjects::attached_objs();
CelebrateObjs& loaded_objs = GlobalObjects::loaded_objs();

using CelebrateMobs_list = std::list<CelebrateMobs::mapped_type>;
void make_CelebratedMobs_list_from_CelebrateMobs(const CelebrateMobs& input, CelebrateMobs_list& result)
{
	result.clear();
	for (const auto& mob : input)
	{
		result.push_back(mob.second);
	}
}

void add_mob_to_attach_list(long uid, CHAR_DATA * mob)
{
		attached_mobs[uid] = mob;
}

void add_mob_to_load_list(long uid, CHAR_DATA * mob)
{
		loaded_mobs[uid] = mob;
}
void add_obj_to_attach_list(long uid, OBJ_DATA * obj)
{
		attached_objs[uid] = obj;
}
void add_obj_to_load_list(long uid, OBJ_DATA * obj)
{
		loaded_objs[uid] = obj;
}

int get_real_hour()
{
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime ( &rawtime );
	return timeinfo->tm_hour;
}

std::string get_name_mono(int day)
{
	if (mono_celebrates.find(day) == mono_celebrates.end())
		return "";
	else
		if (mono_celebrates[day]->start_at <= time_info.hours && mono_celebrates[day]->finish_at >= time_info.hours)
			return mono_celebrates[day]->celebrate->name;
		else return "";
}

std::string get_name_poly(int day)
{
	if (poly_celebrates.find(day) == poly_celebrates.end())
		return "";
	else
		if (poly_celebrates[day]->start_at <= time_info.hours && poly_celebrates[day]->finish_at >= time_info.hours)
			return poly_celebrates[day]->celebrate->name;
		else return "";
}

std::string add_rest(CelebrateList::iterator it, CelebrateDataPtr celebrate)
{
		int days_count = 0;
		while (it->second->celebrate == celebrate)
		{
			days_count++;
			++it;
		}
		--it;
		--days_count;
		int hours = it->second->finish_at;
		hours -= get_real_hour();
		if (hours < 0)
		{
			--days_count;
			hours = 24 + hours;
		}
		return ". До окончания - дней: " + boost::lexical_cast<std::string>(days_count) +
				", часов: " +boost::lexical_cast<std::string>(hours);
}

std::string get_name_real(int day)
{
	CelebrateList::iterator it = real_celebrates.find(day);
	std::string result="";
	if (it != real_celebrates.end())
	{
		if (real_celebrates[day]->start_at <= get_real_hour() && real_celebrates[day]->finish_at >= get_real_hour())
			result = real_celebrates[day]->celebrate->name;// + add_rest(it, real_celebrates[day]->celebrate)
	}
	return result;
}

void parse_trig_list(pugi::xml_node node, TrigList* triggers)
{
	for (pugi::xml_node trig=node.child("trig"); trig; trig = trig.next_sibling("trig"))
	{
		int vnum = trig.attribute("vnum").as_int();
		if (!vnum)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...celebrates - bad trig (node = %s)", node.name());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		triggers->push_back(vnum);
	}
}

void parse_load_data(pugi::xml_node node, LoadPtr node_data)
{
	int vnum = node.attribute("vnum").as_int();
	int max = node.attribute("max").as_int();
	if (!vnum || !max)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...celebrates - bad data (node = %s)", node.name());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	node_data->vnum = vnum;
	node_data->max = max;
}

void parse_load_section(pugi::xml_node node, CelebrateDataPtr holiday)
{
	for (pugi::xml_node room=node.child("room"); room; room = room.next_sibling("room"))
	{
		int vnum = room.attribute("vnum").as_int();
		if (!vnum)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...celebrates - bad room (celebrate = %s)", node.attribute("name").value());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		CelebrateRoomPtr tmp_room(new CelebrateRoom);
		tmp_room->vnum = vnum;
		parse_trig_list(room, &tmp_room->triggers);
		for (pugi::xml_node mob=room.child("mob"); mob; mob = mob.next_sibling("mob"))
		{
			LoadPtr tmp_mob(new ToLoad);
			parse_load_data(mob, tmp_mob);
			parse_trig_list(mob, &tmp_mob->triggers);
				for (pugi::xml_node obj_in=mob.child("obj"); obj_in; obj_in = obj_in.next_sibling("obj"))
				{
					LoadPtr tmp_obj_in(new ToLoad);
					parse_load_data(obj_in, tmp_obj_in);
					parse_trig_list(obj_in, &tmp_obj_in->triggers);
					tmp_mob->objects.push_back(tmp_obj_in);
				}
			tmp_room->mobs.push_back(tmp_mob);
		}
		for (pugi::xml_node obj=room.child("obj"); obj; obj = obj.next_sibling("obj"))
		{
			LoadPtr tmp_obj(new ToLoad);
			parse_load_data(obj, tmp_obj);
			parse_trig_list(obj, &tmp_obj->triggers);
				for (pugi::xml_node obj_in=obj.child("obj"); obj_in; obj_in = obj_in.next_sibling("obj"))
				{
					LoadPtr tmp_obj_in(new ToLoad);
					parse_load_data(obj_in, tmp_obj_in);
					parse_trig_list(obj_in, &tmp_obj_in->triggers);
					tmp_obj->objects.push_back(tmp_obj_in);
				}
			tmp_room->objects.push_back(tmp_obj);
		}
		holiday->rooms[tmp_room->vnum/100].push_back(tmp_room);
	}
}
void parse_attach_section(pugi::xml_node node, CelebrateDataPtr holiday)
{
	pugi::xml_node attaches=node.child("attaches");
	int vnum;
	for (pugi::xml_node mob=attaches.child("mob"); mob; mob = mob.next_sibling("mob"))
	{
		vnum = mob.attribute("vnum").as_int();
		if (!vnum)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...celebrates - bad attach data (node = %s)", node.name());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		parse_trig_list(mob, &holiday->mobsToAttach[vnum/100][vnum]);
	}
	for (pugi::xml_node obj=attaches.child("obj"); obj; obj = obj.next_sibling("obj"))
	{
		vnum = obj.attribute("vnum").as_int();
		if (!vnum)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...celebrates - bad attach data (node = %s)", node.name());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		parse_trig_list(obj, &holiday->objsToAttach[vnum/100][vnum]);
	}
}

int find_real_day_number(int day, int month)
{
	int d = day;
	for (int i = 0; i < month - 1; i++)
			d += tab_day[i];
	return d;
}

int get_previous_day(int day, bool is_real)
{
	if (day == 1)
		return is_real ? 365 : DAYS_PER_MONTH*12;
	else
		return day-1;
}

int get_next_day(int day, bool is_real)
{
	if ((is_real && day == 365) || (!is_real && day == DAYS_PER_MONTH*12))
		return 1;
	else
		return day+1;
}

void load_celebrates(pugi::xml_node node_list, CelebrateList &celebrates, bool is_real)
{
	for (pugi::xml_node node = node_list.child("celebrate"); node; node = node.next_sibling("celebrate"))
	{
		int day = node.attribute("day").as_int();
		int month = node.attribute("month").as_int();
		int start = node.attribute("start").as_int();
		int end = node.attribute("end").as_int();
		std::string name = node.attribute("name").value();
		int baseDay;
		if (!day || !month || name.empty())
		{
			snprintf(buf, MAX_STRING_LENGTH, "...celebrates - bad node struct");
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		CelebrateDataPtr tmp_holiday(new CelebrateData);
		tmp_holiday->name = name;
		parse_load_section(node, tmp_holiday);
		parse_attach_section(node, tmp_holiday);

		if (is_real)
			baseDay = find_real_day_number(day, month);
		else
			baseDay = DAYS_PER_MONTH * (month - 1) + day;
		CelebrateDayPtr tmp_day(new CelebrateDay);
		tmp_day->celebrate = tmp_holiday;
		celebrates[baseDay] = tmp_day;
		if (start>0)
		{
			int dayBefore = baseDay;
			while (start > 24)
			{
				start -= 24;
				dayBefore = get_previous_day(dayBefore, is_real);
				celebrates[dayBefore] = tmp_day;
			}
			CelebrateDayPtr tmp_day1(new CelebrateDay);
			tmp_day1->celebrate = tmp_holiday;
			tmp_day1->start_at = 24-start;
			dayBefore = get_previous_day(dayBefore, is_real);
			celebrates[dayBefore] = tmp_day1;
		}
		if (end > 0)
		{
			int dayAfter = baseDay;
			while (end > 24)
			{
				end -= 24;
				dayAfter = get_next_day(dayAfter, is_real);
				celebrates[dayAfter] = tmp_day;
			}
			CelebrateDayPtr tmp_day2(new CelebrateDay);
			tmp_day2->celebrate = tmp_holiday;
			tmp_day2->finish_at = end;
			tmp_day2->last = true;
			dayAfter = get_next_day(dayAfter, is_real);
			celebrates[dayAfter] = tmp_day2;
		} else
			tmp_day->last = true;
	}
}

void load()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"celebrates.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	pugi::xml_node node_list = doc.child("celebrates");
	if (!node_list)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...celebrates read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	pugi::xml_node mono_node_list = node_list.child("celebratesMono");//православные праздники
	load_celebrates(mono_node_list, mono_celebrates, false);
	pugi::xml_node poly_node_list = node_list.child("celebratesPoly");//языческие праздники
	load_celebrates(poly_node_list, poly_celebrates, false);
	pugi::xml_node real_node_list = node_list.child("celebratesReal");//Российские праздники
	load_celebrates(real_node_list, real_celebrates, true);
}

int get_mud_day()
{
	return time_info.month * DAYS_PER_MONTH + time_info.day + 1;
}

int get_real_day()
{
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime ( &rawtime );
	return find_real_day_number(timeinfo->tm_mday, timeinfo->tm_mon + 1);
}

bool is_active(CelebrateList::iterator it, bool is_real)
{
	int hours = is_real ? get_real_hour() : time_info.hours;
	int day = is_real ? get_real_day() : get_mud_day();
	CelebrateList::const_iterator celebrate;
	if (is_real)
	{
		celebrate = real_celebrates.find(day);
		if (celebrate == real_celebrates.end())
			return false;
		if (celebrate->second->celebrate == it->second->celebrate && hours <= it->second->finish_at)
			return true;
	}
	else
	{
		celebrate = poly_celebrates.find(day);
		if (celebrate == poly_celebrates.end())
			return false;
		if (celebrate != poly_celebrates.end() && celebrate->second->celebrate == it->second->celebrate && hours <= it->second->finish_at)
			return true;
		celebrate = mono_celebrates.find(day);
		if (celebrate == mono_celebrates.end())
			return false;
		if (celebrate != mono_celebrates.end() && celebrate->second->celebrate == it->second->celebrate && hours <= it->second->finish_at)
			return true;
	}
	return false;
}

CelebrateDataPtr get_mono_celebrate()
{
	int day = get_mud_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (mono_celebrates.find(day) != mono_celebrates.end())
		if (is_active(mono_celebrates.find(day), false))
		{
			result = mono_celebrates[day]->celebrate;
			mono_celebrates[day]->celebrate->is_clean = false;
		}
	return result;
};


CelebrateDataPtr get_poly_celebrate()
{
	int day = get_mud_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (poly_celebrates.find(day) != poly_celebrates.end())
		if (is_active(poly_celebrates.find(day), false))
		{
			result = poly_celebrates[day]->celebrate;
			poly_celebrates[day]->celebrate->is_clean = false;
		}
	return result;
};

CelebrateDataPtr get_real_celebrate()
{
	int day = get_real_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (real_celebrates.find(day) != real_celebrates.end())
		if (is_active(real_celebrates.find(day), true))
		{
			result = real_celebrates[day]->celebrate;
			real_celebrates[day]->celebrate->is_clean = false;
		}
	return result;
};

class sc_Checker
{
public:
	sc_Checker(const TrigList& trigs);
	void set_inside_loop() { m_inside_loop = true; }
	void set_current_trigger(const TrigList::value_type current_trigger) { m_current_trigger = current_trigger; }
	void report_null_sc() const;

private:
	auto get_inside_loop() const { return m_inside_loop; }
	const auto& triggers_list() const { return m_triggers_list; }
	auto get_current_trigger() const { return m_current_trigger; }

	bool m_inside_loop;
	TrigList m_triggers_list;
	TrigList::value_type m_current_trigger;
};

sc_Checker::sc_Checker(const TrigList& trigs) : m_inside_loop(false), m_triggers_list(trigs), m_current_trigger(0)
{
}

void sc_Checker::report_null_sc() const
{
	std::string joined_triggers_list;
	joinList(triggers_list(), joined_triggers_list);

	std::stringstream ss;
	ss << "Перехвачен кордамп. Список триггеров, переданных в функцию remove_triggers(...): ["
		<< joined_triggers_list << "].";
	if (get_inside_loop())
	{
		ss << " Обратите внимание, что кордамп перехвачен ВНУТРИ цикла на триггере " << get_current_trigger();
	}
	mudlog(ss.str().c_str(), DEF, -1, ERRLOG, true);
	ss << "\nТекущий стек будет распечатан в ERRLOG, выполнение функции будет прервано.";
	debug::backtrace(runtime_config.logs(ERRLOG).handle());
	mudlog(ss.str().c_str(), DEF, LVL_IMPL, ERRLOG, false);
}

void remove_triggers(TrigList trigs, SCRIPT_DATA* sc)
{
	sc_Checker checker(trigs);
	if (nullptr == sc)
	{
		checker.report_null_sc();
		return;
	}

	for (TrigList::const_iterator it = trigs.begin(); it!= trigs.end(); ++it)
	{
		if (nullptr == sc)
		{
			checker.set_inside_loop();
			checker.set_current_trigger(*it);
			checker.report_null_sc();
			return;
		}

		TRIG_DATA* removed = sc->trig_list.remove_by_vnum(*it);
		if (removed)
		{
			extract_trigger(removed);
			SCRIPT_TYPES(sc) = sc->trig_list.get_type();
		}
	}
}

void remove_from_obj_lists(long uid)
{
	CelebrateObjs::iterator it = attached_objs.find(uid);
	if (it != attached_objs.end())
	{
		attached_objs.erase(it);
	}

	it = loaded_objs.find(uid);
	if (it != loaded_objs.end())
	{
		loaded_objs.erase(it);
	}
}

void remove_from_mob_lists(long uid)
{
	CelebrateMobs::iterator it = attached_mobs.find(uid);
	if (it != attached_mobs.end())
		attached_mobs.erase(it);
	it = loaded_mobs.find(uid);
	if (it != loaded_mobs.end())
		loaded_mobs.erase(it);
}

bool make_clean(CelebrateDataPtr celebrate)
{
	CelebrateObjs::iterator obj_it;
	CelebrateMobs::iterator mob_it;

	for (mob_it = attached_mobs.begin(); mob_it != attached_mobs.end(); ++mob_it)
	{
		const auto rnum = mob_it->second->get_rnum();
		int vnum = mob_index[rnum].vnum;
		for (AttachZonList::iterator it = celebrate->mobsToAttach.begin(); it != celebrate->mobsToAttach.end();++it)
		{
			if (it->second.find(vnum) != it->second.end())
			{
				remove_triggers(it->second[vnum], mob_it->second->script.get());
			}
		}

		attached_mobs.erase(mob_it);
		if (attached_mobs.empty())
		{
			break;
		}
	}

	for (obj_it = attached_objs.begin(); obj_it != attached_objs.end(); ++obj_it)
	{
		int vnum = obj_it->second->get_rnum();
		for (AttachZonList::iterator it = celebrate->objsToAttach.begin(); it != celebrate->objsToAttach.end();++it)
		{
			if (it->second.find(vnum) != it->second.end())
			{
				remove_triggers(it->second[vnum], obj_it->second->get_script().get());
			}
		}

		attached_objs.erase(obj_it);
		if (attached_objs.empty())
		{
			break;
		}
	}

	CelebrateMobs_list loaded_mobs_copy;
	make_CelebratedMobs_list_from_CelebrateMobs(loaded_mobs, loaded_mobs_copy);

	for (const auto& mob : loaded_mobs_copy)
	{
		const auto rnum = mob->get_rnum();
		const int vnum = mob_index[rnum].vnum;
		for (CelebrateZonList::iterator rooms = celebrate->rooms.begin(); rooms != celebrate->rooms.end();++rooms)
		{
			for (CelebrateRoomsList::iterator room = rooms->second.begin(); room != rooms->second.end(); ++room)
			{
				for (LoadList::iterator it = (*room)->mobs.begin(); it != (*room)->mobs.end();++it)
				{
					if ((*it)->vnum == vnum)
					{
						// This function is very bad because modifies global variable loaded_mobs
						// Initially loop was over this global variable. Therefore this loop constantly crashed.
						// I've changed loop variable to the copy of loaded_mobs, but it's still a bad approach.
						extract_char(mob, 0);
					}
				}
			}
		}
	}

	for (obj_it = loaded_objs.begin(); obj_it != loaded_objs.end(); ++obj_it)
	{
		const int vnum = obj_it->second->get_rnum();
		for (CelebrateZonList::iterator rooms = celebrate->rooms.begin(); rooms != celebrate->rooms.end();++rooms)
		{
			for (CelebrateRoomsList::iterator room = rooms->second.begin(); room != rooms->second.end(); ++room)
			{
				for (LoadList::iterator it = (*room)->objects.begin(); it != (*room)->objects.end();++it)
				{
					if ((*it)->vnum == vnum)
					{
						extract_obj(obj_it->second);
					}
				}
			}
		}

		if (loaded_objs.empty())
		{
			break;
		}
	}

	for (CelebrateZonList::iterator rooms = celebrate->rooms.begin(); rooms != celebrate->rooms.end();++rooms)
	{
		for (CelebrateRoomsList::iterator room = rooms->second.begin(); room != rooms->second.end(); ++room)
		{
			if (!(*room)->triggers.empty())
			{
				const int rnum = real_room((*room)->vnum);
				remove_triggers((*room)->triggers, world[rnum]->script.get());
			}
		}
	}

	return true;//пока не знаю зачем
}

void clear_real_celebrates(CelebrateList celebrates)
{
	CelebrateList::iterator it;
	for (it = celebrates.begin(); it != celebrates.end(); ++it)
	{
		if (!it->second->celebrate->is_clean
			&& !is_active(it, true)
			&& it->second->last)
		{
			if (make_clean(it->second->celebrate))
			{
				it->second->celebrate->is_clean = true;
			}
		}
	}
}

void clear_mud_celebrates(CelebrateList celebrates)
{
	CelebrateList::iterator it;
	for (it = celebrates.begin();it != celebrates.end(); ++it)
	{
		if (!it->second->celebrate->is_clean
			&& !is_active(it, false)
			&& it->second->last)
		{
			if (make_clean(it->second->celebrate))
			{
				it->second->celebrate->is_clean = true;
			}
		}
	}
}

void sanitize()
{
	clear_real_celebrates(real_celebrates);
	clear_mud_celebrates(poly_celebrates);
	clear_mud_celebrates(mono_celebrates);
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
