// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "obj.hpp"
#include "utils.h"

id_to_set_info_map obj_data::set_table;

obj_data::obj_data() :
	uid(0),
	item_number(NOTHING),
	in_room(NOWHERE),
	name(NULL),
	description(NULL),
	short_description(NULL),
	action_description(NULL),
	ex_description(NULL),
	carried_by(NULL),
	worn_by(NULL),
	worn_on(NOWHERE),
	in_obj(NULL),
	contains(NULL),
	id(0),
	proto_script(NULL),
	script(NULL),
	next_content(NULL),
	next(NULL),
	room_was_in(NOWHERE),
	max_in_world(0),
	skills(NULL),
	serial_num_(0),
	timer_(0),
	mort_req_(0)
{
	memset(&obj_flags, 0, sizeof(obj_flag_data));

	for (int i = 0; i < 6; i++)
		PNames[i] = NULL;
}

int obj_data::get_serial_num()
{
	return serial_num_;
}

void obj_data::set_serial_num(int num)
{
	serial_num_ = num;
}

const std::string obj_data::activate_obj(const activation& __act)
{
	if (item_number >= 0)
	{
		obj_flags.affects = __act.get_affects();
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = __act.get_affected_i(i);

		int weight = __act.get_weight();
		if (weight > 0)
			obj_flags.weight = weight;

		if (obj_flags.type_flag == ITEM_WEAPON)
		{
			int nsides, ndices;
			__act.get_dices(ndices, nsides);
			// Типа такая проверка на то, устанавливались ли эти параметры.
			if (ndices > 0 && nsides > 0)
			{
				obj_flags.value[1] = ndices;
				obj_flags.value[2] = nsides;
			}
		}

		// Активируем умения.
		if (__act.has_skills())
		{
			// У всех объектов с умениями skills указывает на один и тот же
			// массив. И у прототипов. Поэтому тут надо создавать новый,
			// если нет желания "активировать" сразу все такие объекты.
			// Умения, проставленные в сете, заменяют родные умения предмета.
			skills = new std::map<int, int>;
			__act.get_skills(*skills);
		}

		return __act.get_actmsg() + "\n" + __act.get_room_actmsg();
	}
	else
		return "\n";
}

const std::string obj_data::deactivate_obj(const activation& __act)
{
	if (item_number >= 0)
	{
		obj_flags.affects = obj_proto[item_number]->obj_flags.affects;
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = obj_proto[item_number]->affected[i];

		obj_flags.weight = obj_proto[item_number]->obj_flags.weight;

		if (obj_flags.type_flag == ITEM_WEAPON)
		{
			obj_flags.value[1] = obj_proto[item_number]->obj_flags.value[1];
			obj_flags.value[2] = obj_proto[item_number]->obj_flags.value[2];
		}

		// Деактивируем умения.
		if (__act.has_skills())
		{
			// При активации мы создавали новый массив с умениями. Его
			// можно смело удалять.
			delete skills;
			skills = obj_proto[item_number]->skills;
		}

		return __act.get_deactmsg() + "\n" + __act.get_room_deactmsg();
	}
	else
		return "\n";
}

void obj_data::set_skill(int skill_num, int percent)
{
	if (skills)
	{
		std::map<int, int>::iterator skill = skills->find(skill_num);
		if (skill == skills->end())
		{
			if (percent != 0)
				skills->insert(std::make_pair(skill_num, percent));
		}
		else
		{
			if (percent != 0)
				skill->second = percent;
			else
				skills->erase(skill);
		}
	}
	else
	{
		if (percent != 0)
		{
			skills = new std::map<int, int>;
			skills->insert(std::make_pair(skill_num, percent));
		}
	}
}

int obj_data::get_skill(int skill_num) const
{
	if (skills)
	{
		std::map<int, int>::iterator skill = skills->find(skill_num);
		if (skill != skills->end())
			return skill->second;
		else
			return 0;
	}
	else
	{
		return 0;
	}
}

/**
 * @warning Предполагается, что __out_skills.empty() == true.
 */
void obj_data::get_skills(std::map<int, int>& out_skills) const
{
	if (skills)
		out_skills.insert(skills->begin(), skills->end());
}

bool obj_data::has_skills() const
{
	if (skills)
		return !skills->empty();
	else
		return false;
}

void obj_data::set_timer(int timer)
{
	timer_ = MAX(0, timer);
}

int obj_data::get_timer() const
{
	return timer_;
}

/**
* Реальное старение шмотки (без всяких технических сетов таймера по коду).
* Помимо таймера самой шмотки снимается таймер ее временного обкаста.
* \param time по дефолту 1.
*/
void obj_data::dec_timer(int time)
{
	if (time > 0)
	{
		timer_ -= time;
		if (!timed_spell.empty())
		{
			timed_spell.dec_timer(this, time);
		}
	}
}

int obj_data::get_mort_req() const
{
	return mort_req_;
}

void obj_data::set_mort_req(int param)
{
	mort_req_ = MAX(0, param);
}

namespace ObjSystem
{

bool is_armor_type(const OBJ_DATA *obj)
{
	switch GET_OBJ_TYPE(obj)
	{
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		return true;
	}
	return false;
}

} // namespace ObjSystem
