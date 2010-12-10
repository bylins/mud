#include "pugixml.hpp"

#include "celebrates.hpp"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"


namespace Celebrates
{

int tab_day [12]= {31,28,31,30,31,30,31,31,30,31,30,31};//да и хрен с ним, с 29 февраля!

CelebrateList mono_celebrates, poly_celebrates, real_celebrates;

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

std::string get_name_real(int day)
{
	if (real_celebrates.find(day) == real_celebrates.end())
		return "";
	else
		if (real_celebrates[day]->start_at <= get_real_hour() && real_celebrates[day]->finish_at >= get_real_hour())
			return real_celebrates[day]->celebrate->name;
		else return "";
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
		CelebrateRoomPtr tmp_room(new СelebrateRoom);
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
		tmp_day->start_at = 0;
		tmp_day->finish_at = 24;
		celebrates[baseDay] = tmp_day;
		if (start>0)
		{
			int dayBefore = baseDay;
			while (start > 24)
			{
				start -= 24;
				dayBefore = get_previous_day(dayBefore, is_real);
				celebrates[dayBefore] = tmp_day; //копируем указатель на базовый день - этот день такой же
			}
			CelebrateDayPtr tmp_day1(new CelebrateDay);
			tmp_day1->celebrate = tmp_holiday;
			tmp_day1->start_at = 24-start;
			tmp_day1->finish_at = 24;
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
				celebrates[dayAfter] = tmp_day; //копируем указатель на базовый день - этот день такой же
			}
			CelebrateDayPtr tmp_day2(new CelebrateDay);
			tmp_day2->celebrate = tmp_holiday;
			tmp_day2->start_at = 0;
			tmp_day2->finish_at = end;
			dayAfter = get_next_day(dayAfter, is_real);
			celebrates[dayAfter] = tmp_day2;
		}
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


CelebrateDataPtr get_mono_celebrate()
{
	int day = get_mud_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (mono_celebrates.find(day) != mono_celebrates.end())
		if (time_info.hours >= mono_celebrates[day]->start_at && time_info.hours <= mono_celebrates[day]->finish_at)
			result = mono_celebrates[day]->celebrate;
	return result;
};

CelebrateDataPtr get_poly_celebrate()
{
	int day = get_mud_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (poly_celebrates.find(day) != poly_celebrates.end())
		if (time_info.hours >= poly_celebrates[day]->start_at && time_info.hours <= poly_celebrates[day]->finish_at)
			result = poly_celebrates[day]->celebrate;
	return result;
};

CelebrateDataPtr get_real_celebrate()
{
	int day = get_real_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (real_celebrates.find(day) != real_celebrates.end())
		if (get_real_hour() >= real_celebrates[day]->start_at && get_real_hour() <= real_celebrates[day]->finish_at)
			result = real_celebrates[day]->celebrate;
	return result;
};

}