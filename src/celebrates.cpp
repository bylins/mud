#include <algorithm>
#include "pugixml.hpp"

#include "conf.h"
#include "sysdep.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "celebrates.hpp"
#include "char.hpp"
#include "handler.h"

extern void extract_trigger(TRIG_DATA * trig);
extern CHAR_DATA* character_list;
extern OBJ_DATA *object_list;


namespace Celebrates
{

int tab_day [12]= {31,28,31,30,31,30,31,31,30,31,30,31};//да и хрен с ним, с 29 февраля!

CelebrateList mono_celebrates, poly_celebrates, real_celebrates;
typedef std::vector<long> CelebrateUids;

CelebrateUids loaded_mobs, attached_mobs;
CelebrateUids loaded_objs, attached_objs;

void add_mob_to_attach_list(long uid)
{
	if (std::find(attached_mobs.begin(), attached_mobs.end(), uid) == attached_mobs.end())
		attached_mobs.push_back(uid);
}
void add_mob_to_load_list(long uid)
{
	if (std::find(loaded_mobs.begin(), loaded_mobs.end(), uid) == loaded_mobs.end())
		loaded_mobs.push_back(uid);
}
void add_obj_to_attach_list(long uid)
{
	if (std::find(attached_objs.begin(), attached_objs.end(), uid) == attached_objs.end())
		attached_objs.push_back(uid);
}
void add_obj_to_load_list(long uid)
{
	if (std::find(loaded_objs.begin(), loaded_objs.end(), uid) == loaded_objs.end())
		loaded_objs.push_back(uid);
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
		tmp_day->start_at = 0;
		tmp_day->finish_at = 24;
		tmp_day->is_clean = true;
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
			tmp_day1->is_clean = true;
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
			tmp_day2->is_clean = true;
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
	CelebrateList l = poly_celebrates;
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
		{
			result = mono_celebrates[day]->celebrate;
			mono_celebrates[day]->is_clean = false;
		}
	return result;
};

CelebrateDataPtr get_poly_celebrate()
{
	int day = get_mud_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (poly_celebrates.find(day) != poly_celebrates.end())
		if (time_info.hours >= poly_celebrates[day]->start_at && time_info.hours <= poly_celebrates[day]->finish_at)
		{
			result = poly_celebrates[day]->celebrate;
			poly_celebrates[day]->is_clean = false;
		}
	return result;
};

CelebrateDataPtr get_real_celebrate()
{
	int day = get_real_day();
	CelebrateDataPtr result = CelebrateDataPtr();
	if (real_celebrates.find(day) != real_celebrates.end())
		if (get_real_hour() >= real_celebrates[day]->start_at && get_real_hour() <= real_celebrates[day]->finish_at)
		{
			result = real_celebrates[day]->celebrate;
			real_celebrates[day]->is_clean = false;
		}
	return result;
};

void remove_triggers(TrigList trigs, script_data* sc)
{
	TrigList::const_iterator it;
	TRIG_DATA *tr, *tmp;
	
	for (it = trigs.begin(); it!= trigs.end();++it)
	{
		for (tmp = NULL, tr = TRIGGERS(sc); tr; tmp = tr, tr = tr->next)
		{
			if (trig_index[tr->nr]->vnum == *it)
				break;
		}

		if (tr)
		{
			if (tmp)
			{
				tmp->next = tr->next;
				extract_trigger(tr);
			}
			/* this was the first trigger */
			else
			{
				TRIGGERS(sc) = tr->next;
				extract_trigger(tr);
			}
			/* update the script type bitvector */
			SCRIPT_TYPES(sc) = 0;
			for (tr = TRIGGERS(sc); tr; tr = tr->next)
				SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(tr);
		}
	}
}


bool make_clean (CelebrateDataPtr celebrate)
{
	CelebrateZonList::iterator zon;
	CelebrateRoomsList::iterator room;
	AttachZonList::iterator att;


	std::vector<int> mobs_to_remove, objs_to_remove;
	AttachList mobs_to_deatch, objs_to_deatch;

	
	for (att = celebrate->mobsToAttach.begin(); att !=celebrate->mobsToAttach.end();++att)
	{
		mobs_to_deatch.insert(att->second.begin(), att->second.end());
	}
	for (att = celebrate->objsToAttach.begin(); att !=celebrate->objsToAttach.end();++att)
	{
		objs_to_deatch.insert(att->second.begin(), att->second.end());
	}
	for (zon = celebrate->rooms.begin(); zon != celebrate->rooms.end(); ++zon)
	{
		for (room = zon->second.begin(); room != zon->second.end(); ++room)
		{
			LoadList::const_iterator it;
			for (it = (*room)->mobs.begin(); it!= (*room)->mobs.end(); ++it)
				mobs_to_remove.push_back((*it)->vnum);
			for (it = (*room)->objects.begin(); it!= (*room)->objects.end(); ++it)
				objs_to_remove.push_back((*it)->vnum);
		}
	}

//поехали... обходим все мобы и предметы в мире, хача по дороге неугодные нам...
	CHAR_DATA* tmp;
	int vnum;
	CelebrateUids::iterator uid;
	
	for (CHAR_DATA *ch=character_list;ch;ch=tmp)
	{
		tmp=ch->next;
		vnum = mob_index[ch->nr].vnum;
	
		uid = std::find(attached_mobs.begin(), attached_mobs.end(), ch->get_uid());

		if (mobs_to_deatch.find(vnum)!= mobs_to_deatch.end() && uid != attached_mobs.end())
		{
			remove_triggers(mobs_to_deatch[vnum], ch->script);
			attached_mobs.erase(uid);
		}

		if (SCRIPT(ch) && !TRIGGERS(SCRIPT(ch)))
		{
			free_script(SCRIPT(ch));	// без комментариев
			SCRIPT(ch) = NULL;
		}			
		uid = std::find(loaded_mobs.begin(), loaded_mobs.end(), ch->get_uid());
		if (std::find(mobs_to_remove.begin(), mobs_to_remove.end(), vnum) != mobs_to_remove.end()
			&& uid != loaded_mobs.end())
		{
			extract_char(ch, 0);
			loaded_mobs.erase(uid);
		}

	}
	OBJ_DATA* tmpo;	
	for (OBJ_DATA *o=object_list;o;o=tmpo)
	{
		tmpo=o->next;
		vnum = o->item_number;

		uid = std::find(attached_objs.begin(), attached_objs.end(), (int)o->uid);

		if (objs_to_deatch.find(vnum)!= objs_to_deatch.end() && uid != attached_objs.end())
		{
			remove_triggers(objs_to_deatch[vnum], o->script);
			attached_objs.erase(uid);
		}

		if (SCRIPT(o) && !TRIGGERS(SCRIPT(o)))
		{
			free_script(SCRIPT(o));	// без комментариев
			SCRIPT(o) = NULL;
		}			
		uid = std::find(loaded_objs.begin(), loaded_objs.end(), (int)o->uid);
		if (std::find(objs_to_remove.begin(), objs_to_remove.end(), vnum) != objs_to_remove.end()
			&& uid != loaded_objs.end())
		{
			extract_obj(o);
			loaded_objs.erase(uid);
		}

	}

	return true;//пока не знаю зачем
}

void clear_real_celebrates(CelebrateList celebrates)
{
	CelebrateList::iterator it;
	for (it = celebrates.begin();it != celebrates.end(); ++it)
	{
		if (!it->second->is_clean && get_real_day()!=it->first) 
			if (make_clean(it->second->celebrate))
				it->second->is_clean = true;
	}
}

void clear_mud_celebrates(CelebrateList celebrates)
{
	CelebrateList::iterator it;
	for (it = celebrates.begin();it != celebrates.end(); ++it)
	{
		if (!it->second->is_clean && get_mud_day()!=it->first) 
			if (make_clean(it->second->celebrate))
				it->second->is_clean = true;
	}
}

void sanitize()
{
	clear_real_celebrates(real_celebrates);
	clear_mud_celebrates(poly_celebrates);
	clear_mud_celebrates(mono_celebrates);
}

}
