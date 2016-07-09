// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "obj.hpp"

#include "parse.hpp"
#include "handler.h"
#include "char.hpp"
#include "constants.h"
#include "db.h"
#include "screen.h"
#include "celebrates.hpp"
#include "pk.h"
#include "cache.hpp"
#include "utils.h"
#include "conf.h"

#include <boost/algorithm/string.hpp>

#include <cmath>
#include <sstream>

extern void get_from_container(CHAR_DATA * ch, OBJ_DATA * cont, char *arg, int mode, int amount, bool autoloot);
extern void set_obj_eff(OBJ_DATA *itemobj, const EApplyLocation type, int mod);
extern void set_obj_aff(OBJ_DATA *itemobj, const EAffectFlag bitv);

id_to_set_info_map OBJ_DATA::set_table;

namespace
{

// список шмоток после пуржа для последующего удаления оболочки
typedef std::vector<OBJ_DATA *> PurgedObjList;
PurgedObjList purged_obj_list;

} // namespace

OBJ_DATA::OBJ_DATA()
{
	this->zero_init();
	caching::obj_cache.add(this);
}

OBJ_DATA::OBJ_DATA(const OBJ_DATA& other)
{
	*this = other;
	caching::obj_cache.add(this);
}

OBJ_DATA::~OBJ_DATA()
{
	if (!purged_)
	{
		this->purge(true);
	}
}

// можно было бы отдельной функцией и не делать, но вдруг кто-нибудь потом захочет расширить функционал
struct custom_label *init_custom_label()
{
	struct custom_label *ptr = (struct custom_label *)malloc(sizeof(struct custom_label));
	ptr->label_text = NULL;
	ptr->clan = NULL;
	ptr->author = -2;
	ptr->author_mail = NULL;

	return ptr;
}

// эта функция только освобождает память, поэтому не забываем устанавливать указатель в NULL,
// если сразу после этого не делаем init_custom_label(), иначе будут креши
void free_custom_label(struct custom_label *custom_label) {
	if (custom_label) {
		free(custom_label->label_text);
		if (custom_label->clan)
			free(custom_label->clan);
		if (custom_label->author_mail)
			free(custom_label->author_mail);
		free(custom_label);
	}
}

// * См. Character::zero_init()
void OBJ_DATA::zero_init()
{
	uid = 0;
	item_number = NOTHING;
	in_room = NOWHERE;
	aliases = NULL;
	description = NULL;
	short_description = NULL;
	action_description = NULL;
	ex_description = NULL;
	carried_by = NULL;
	worn_by = NULL;
	worn_on = NOWHERE;
	in_obj = NULL;
	contains = NULL;
	id = 0;
	proto_script.clear();
	script = NULL;
	next_content = NULL;
	next = NULL;
	room_was_in = NOWHERE;
	max_in_world = 0;
	m_skills.clear();
	serial_num_ = 0;
	timer_ = 0;
	manual_mort_req_ = -1;
	purged_ = false;
	ilevel_ = 0;
	cost_ = 0;
	cost_per_day_on_ = 0;
	cost_per_day_off_ = 0;
	activator_.first = false;
	activator_.second = 0;

	custom_label = NULL;

	memset(&obj_flags, 0, sizeof(obj_flag_data));

	for (int i = 0; i < 6; i++)
	{
		PNames[i] = NULL;
	}
}

// * См. Character::purge()
void OBJ_DATA::purge(bool destructor)
{
	if (purged_)
	{
		log("SYSERROR: double purge (%s:%d)", __FILE__, __LINE__);
		return;
	}

	caching::obj_cache.remove(this);
	//см. комментарий в структуре BloodyInfo из pk.cpp
	bloody::remove_obj(this);
	//weak_ptr тут бы был какраз в тему
	Celebrates::remove_from_obj_lists(this->uid);

	if (!destructor)
	{
		// обнуляем все
		this->zero_init();
		// проставляем неподходящие из конструктора поля
		purged_ = true;
		// закидываем в список ожидающих делета указателей
		purged_obj_list.push_back(this);
	}
}

bool OBJ_DATA::purged() const
{
	return purged_;
}

int OBJ_DATA::get_serial_num()
{
	return serial_num_;
}

void OBJ_DATA::set_serial_num(int num)
{
	serial_num_ = num;
}

const std::string OBJ_DATA::activate_obj(const activation& __act)
{
	if (item_number >= 0)
	{
		obj_flags.affects = __act.get_affects();
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = __act.get_affected_i(i);

		int weight = __act.get_weight();
		if (weight > 0)
			obj_flags.weight = weight;

		if (obj_flags.type_flag == obj_flag_data::ITEM_WEAPON)
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
			m_skills.clear();
			__act.get_skills(m_skills);
		}

		return __act.get_actmsg() + "\n" + __act.get_room_actmsg();
	}
	else
		return "\n";
}

const std::string OBJ_DATA::deactivate_obj(const activation& __act)
{
	if (item_number >= 0)
	{
		obj_flags.affects = obj_proto[item_number]->obj_flags.affects;
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			affected[i] = obj_proto[item_number]->affected[i];
		}

		obj_flags.weight = obj_proto[item_number]->obj_flags.weight;

		if (obj_flags.type_flag == obj_flag_data::ITEM_WEAPON)
		{
			obj_flags.value[1] = obj_proto[item_number]->obj_flags.value[1];
			obj_flags.value[2] = obj_proto[item_number]->obj_flags.value[2];
		}

		// Деактивируем умения.
		if (__act.has_skills())
		{
			// При активации мы создавали новый массив с умениями. Его
			// можно смело удалять.
			m_skills.clear();
			m_skills = obj_proto[item_number]->m_skills;
		}

		return __act.get_deactmsg() + "\n" + __act.get_room_deactmsg();
	}
	else
		return "\n";
}

void OBJ_DATA::set_skill(int skill_num, int percent)
{
	if (!m_skills.empty())
	{
		const auto skill = m_skills.find(skill_num);
		if (skill == m_skills.end())
		{
			if (percent != 0)
			{
				m_skills.insert(std::make_pair(skill_num, percent));
			}
		}
		else
		{
			if (percent != 0)
			{
				skill->second = percent;
			}
			else
			{
				m_skills.erase(skill);
			}
		}
	}
	else
	{
		if (percent != 0)
		{
			m_skills.clear();
			m_skills.insert(std::make_pair(skill_num, percent));
		}
	}
}

int OBJ_DATA::get_skill(int skill_num) const
{
	const auto skill = m_skills.find(skill_num);
	if (skill != m_skills.end())
	{
		return skill->second;
	}

	return 0;
}

// * @warning Предполагается, что __out_skills.empty() == true.
void OBJ_DATA::get_skills(std::map<int, int>& out_skills) const
{
	if (!m_skills.empty())
	{
		out_skills.insert(m_skills.begin(), m_skills.end());
	}
}

bool OBJ_DATA::has_skills() const
{
	return !m_skills.empty();
}

void OBJ_DATA::set_timer(int timer)
{
	timer_ = MAX(0, timer);	
}

int OBJ_DATA::get_timer() const
{
	return timer_;
}

 //заколдование предмета
void OBJ_DATA::set_enchant(int skill)
{
    int i = 0;

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (affected[i].location != APPLY_NONE)
       affected[i].location = APPLY_NONE;

    affected[0].location = APPLY_HITROLL;
    affected[1].location = APPLY_DAMROLL;

    if (skill <= 100)
    // 4 мортов (скил магия света 100)
    {
       affected[0].modifier = 1 + number(0, 1);
       affected[1].modifier = 1 + number(0, 1);
    }
    else if (skill <= 125)
    // 8 мортов (скил магия света 125)
    {
       affected[0].modifier = 1 + number(-3, 2);
       affected[1].modifier = 1 + number(-3, 2);
    }
    else if (skill <= 160)
    // 12 мортов (скил магия света 160)
    {
       affected[0].modifier = 1 + number(-4, 3);
       affected[1].modifier = 1 + number(-4, 3);
    }
    else if (skill >160)
    // 16 мортов (скил магия света 160+)
    {
       affected[0].modifier = 1 + number(-5, 4);
       affected[1].modifier = 1 + number(-5, 4);
    }
    else
    {  // волхвы
       affected[0].modifier = 2;
       affected[1].modifier = 2;
    };
    set_extraflag(EExtraFlag::ITEM_MAGIC);    
}

void OBJ_DATA::set_enchant(int skill, OBJ_DATA *obj)
{
    int i = 0;
    int random_drop = 0;

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (affected[i].location != APPLY_NONE)
       affected[i].location = APPLY_NONE;

    affected[0].location = APPLY_HITROLL;
    affected[1].location = APPLY_DAMROLL;

    if (skill <= 100)
    // 4 мортов (скил магия света 100)
    {
       affected[0].modifier = 1 + number(0, 1);
       affected[1].modifier = 1 + number(0, 1);
    }
    else if (skill <= 125)
    // 8 мортов (скил магия света 125)
    {
       random_drop = 1;
       affected[0].modifier = 1 + number(-3, 2);
       affected[1].modifier = 1 + number(-3, 2);
    }
    else if (skill <= 160)
    // 12 мортов (скил магия света 160)
    {
       random_drop = 2;
       affected[0].modifier = 1 + number(-4, 3);
       affected[1].modifier = 1 + number(-4, 3);
    }
    else if (skill >160)
    // 16 мортов (скил магия света 160+)
    {
       random_drop = 3;
       affected[0].modifier = 1 + number(-5, 4);
       affected[1].modifier = 1 + number(-5, 4);
    }
    else
    {  // волхвы
       affected[0].modifier = 2;
       affected[1].modifier = 2;
    };
    
    for (i = 0; i < random_drop; i++)
            if (obj->affected[i].location != APPLY_NONE)
                {
                    set_obj_eff(this,obj->affected[i].location,obj->affected[i].modifier);
                };
    GET_OBJ_AFFECTS(this) += GET_OBJ_AFFECTS(obj);
    GET_OBJ_EXTRA(this) += GET_OBJ_EXTRA(obj);
    obj_flags.no_flag += GET_OBJ_NO(obj);

    set_extraflag(EExtraFlag::ITEM_MAGIC);    
   
}

void OBJ_DATA::unset_enchant()
{
    int i = 0;
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
        {
                if (obj_proto.at(item_number)->affected[i].location != APPLY_NONE)
                {
                        affected[i].location = obj_proto.at(item_number)->affected[i].location;
                        affected[i].modifier = obj_proto.at(item_number)->affected[i].modifier;
                }
                else
                {
                        affected[i].location = APPLY_NONE;
                }

        }
    unset_extraflag(EExtraFlag::ITEM_MAGIC);

}
 extern bool check_unlimited_timer(OBJ_DATA *obj);
 extern float count_remort_requred(OBJ_DATA *obj);
 extern float count_unlimited_timer(OBJ_DATA *obj);

/**
* Реальное старение шмотки (без всяких технических сетов таймера по коду).
* Помимо таймера самой шмотки снимается таймер ее временного обкаста.
* \param time по дефолту 1.
*/
void OBJ_DATA::dec_timer(int time, bool ignore_utimer)
{
	if (!m_timed_spell.empty())
	{
		m_timed_spell.dec_timer(this, time);
	}

	if (!ignore_utimer && check_unlimited_timer(this))
		return;
	if (time > 0)
	{
		timer_ -= time;
	}
}

float OBJ_DATA::show_mort_req() 
{
	return count_remort_requred(this);
}

float OBJ_DATA::show_koef_obj() 
{
	return count_unlimited_timer(this);
}

int OBJ_DATA::get_manual_mort_req() const
{
	return manual_mort_req_;
}

void OBJ_DATA::set_manual_mort_req(int param)
{
	manual_mort_req_ = param;
}

unsigned OBJ_DATA::get_ilevel() const
{
	return ilevel_;
}

void OBJ_DATA::set_ilevel(unsigned ilvl)
{
	ilevel_ = ilvl;
}

int OBJ_DATA::get_mort_req() const
{
	if (manual_mort_req_ >= 0)
	{
		return manual_mort_req_;
	}
	else if (ilevel_ > 30)
	{
		return 9;
	}
	return 0;
}

int OBJ_DATA::get_cost() const
{
	return cost_;
}

void OBJ_DATA::set_cost(int x)
{
	if (x >= 0)
	{
		cost_ = x;
	}
}

int OBJ_DATA::get_rent() const
{
	/* if (check_unlimited_timer(this))
		return 0; */
	return cost_per_day_off_;
}

void OBJ_DATA::set_rent(int x)
{
	if (x >= 0)
	{
		cost_per_day_off_ = x;
	}
}

int OBJ_DATA::get_rent_eq() const
{
	/* if (check_unlimited_timer(this))
		return 0; */
	return cost_per_day_on_;
}

void OBJ_DATA::set_rent_eq(int x)
{
	if (x >= 0)
	{
		cost_per_day_on_ = x;
	}
}

void OBJ_DATA::set_activator(bool flag, int num)
{
	activator_.first = flag;
	activator_.second = num;
}

std::pair<bool, int> OBJ_DATA::get_activator() const
{
	return activator_;
}

void OBJ_DATA::add_timed_spell(const int spell, const int time)
{
	if (spell < 1 || spell >= SPELLS_COUNT)
	{
		log("SYSERROR: func: %s, spell = %d, time = %d", __func__, spell, time);
		return;
	}
	m_timed_spell.add(this, spell, time);
}

void OBJ_DATA::del_timed_spell(const int spell, const bool message)
{
	m_timed_spell.del(this, spell, message);
}

////////////////////////////////////////////////////////////////////////////////

namespace
{

const float SQRT_MOD = 1.7095f;
const int AFF_SHIELD_MOD = 30;
const int AFF_BLINK_MOD = 10;

} // namespace

////////////////////////////////////////////////////////////////////////////////

namespace ObjSystem
{

float count_affect_weight(OBJ_DATA* /*obj*/, int num, int mod)
{
	float weight = 0;

	switch(num)
	{
	case APPLY_STR:
		weight = mod * 5.0;
		break;
	case APPLY_DEX:
		weight = mod * 10.0;
		break;
	case APPLY_INT:
		weight = mod * 10.0;
		break;
	case APPLY_WIS:
		weight = mod * 10.0;
		break;
	case APPLY_CON:
		weight = mod * 10.0;
		break;
	case APPLY_CHA:
		weight = mod * 10.0;
		break;
	case APPLY_HIT:
		weight = mod * 0.2;
		break;
	case APPLY_AC:
		weight = mod * -1.0;
		break;
	case APPLY_HITROLL:
		weight = mod * 3.3;
		break;
	case APPLY_DAMROLL:
		weight = mod * 3.3;
		break;
	case APPLY_SAVING_WILL:
		weight = mod * -1.0;
		break;
	case APPLY_SAVING_CRITICAL:
		weight = mod * -1.0;
		break;
	case APPLY_SAVING_STABILITY:
		weight = mod * -1.0;
		break;
	case APPLY_SAVING_REFLEX:
		weight = mod * -1.0;
		break;
	case APPLY_CAST_SUCCESS:
		weight = mod * 1.0;
		break;
	case APPLY_MORALE:
		weight = mod * 2.0;
		break;
	case APPLY_INITIATIVE:
		weight = mod * 1.0;
		break;
	case APPLY_ABSORBE:
		weight = mod * 1.0;
		break;
	}

	return weight;
}

bool is_armor_type(const OBJ_DATA *obj)
{
	switch (GET_OBJ_TYPE(obj))
	{
	case obj_flag_data::ITEM_ARMOR:
	case obj_flag_data::ITEM_ARMOR_LIGHT:
	case obj_flag_data::ITEM_ARMOR_MEDIAN:
	case obj_flag_data::ITEM_ARMOR_HEAVY:
		return true;

	default:
		return false;
	}
}

// * См. CharacterSystem::release_purged_list()
void release_purged_list()
{
	for (PurgedObjList::iterator i = purged_obj_list.begin();
		i != purged_obj_list.end(); ++i)
	{
		delete *i;
	}
	purged_obj_list.clear();
}

bool is_mob_item(OBJ_DATA *obj)
{
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MALE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_FEMALE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MONO)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_POLY)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_RUSICHI)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CLERIC)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_THIEF)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_WARRIOR)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_ASSASINE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_GUARD)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_PALADINE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_RANGER)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_SMITH)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MERCHANT)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_DRUID)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_BATTLEMAGE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMMAGE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_DEFENDERMAGE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_NECROMANCER)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MALE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_FEMALE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MONO)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLY)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_RUSICHI)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CLERIC)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_THIEF)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_WARRIOR)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_ASSASINE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_GUARD)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_PALADINE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_RANGER)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_SMITH)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MERCHANT)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_DRUID)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_BATTLEMAGE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMMAGE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_DEFENDERMAGE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_NECROMANCER)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_SEVERANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_KRIVICHI)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_VATICHI)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_VELANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_DREVLANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE))
	{
		return true;
	}

	return false;
}

void init_ilvl(OBJ_DATA *obj)
{
	if (is_mob_item(obj)
		|| obj->get_extraflag(EExtraFlag::ITEM_SETSTUFF)
		|| obj->get_manual_mort_req() >= 0)
	{
		obj->set_ilevel(0);
		return;
	}

	float total_weight = 0.0;

	// аффекты APPLY_x
	for (int k = 0; k < MAX_OBJ_AFFECT; k++)
	{
		if (obj->affected[k].location == 0) continue;

		// случай, если один аффект прописан в нескольких полях
		for (int kk = 0; kk < MAX_OBJ_AFFECT; kk++)
		{
			if (obj->affected[k].location == obj->affected[kk].location
				&& k != kk)
			{
				log("SYSERROR: double affect=%d, obj_vnum=%d",
					obj->affected[k].location, GET_OBJ_VNUM(obj));
				obj->set_ilevel(1000000);
				return;
			}
		}
		//если аффект отрицательный. убирем ошибку от степени
		if (obj->affected[k].modifier < 0) continue;
		float weight = count_affect_weight(obj, obj->affected[k].location,
			obj->affected[k].modifier);
		total_weight += pow(weight, SQRT_MOD);
	}
	// аффекты AFF_x через weapon_affect
	for (const auto& m : weapon_affect)
	{
		if (IS_OBJ_AFF(obj, m.aff_pos))
		{
			if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_AIRSHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_FIRESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_ICESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_BLINK)
			{
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			}
		}
	}

	obj->set_ilevel(ceil(pow(total_weight, 1/SQRT_MOD)));
}

void init_item_levels()
{
	for (const auto i : obj_proto)
	{
		init_ilvl(i);
	}
}

} // namespace ObjSystem

namespace system_obj
{

/// кошелек для кун с игрока
const int PURSE_VNUM = 1924;
int PURSE_RNUM = -1;
/// персональное хранилище
const int PERS_CHEST_VNUM = 331;
int PERS_CHEST_RNUM = -1;

/// при старте сразу после лоада зон
void init()
{
	PURSE_RNUM = real_object(PURSE_VNUM);
	PERS_CHEST_RNUM = real_object(PERS_CHEST_VNUM);
}

OBJ_DATA* create_purse(CHAR_DATA *ch, int/* gold*/)
{
	OBJ_DATA *obj = read_object(PURSE_RNUM, REAL);
	if (!obj)
	{
		return obj;
	}

	obj->aliases = str_dup("тугой кошелек");
	obj->short_description = str_dup("тугой кошелек");
	obj->description = str_dup(
		"Кем-то оброненный тугой кошелек лежит здесь.");
	GET_OBJ_PNAME(obj, 0) = str_dup("тугой кошелек");
	GET_OBJ_PNAME(obj, 1) = str_dup("тугого кошелька");
	GET_OBJ_PNAME(obj, 2) = str_dup("тугому кошельку");
	GET_OBJ_PNAME(obj, 3) = str_dup("тугой кошелек");
	GET_OBJ_PNAME(obj, 4) = str_dup("тугим кошельком");
	GET_OBJ_PNAME(obj, 5) = str_dup("тугом кошельке");

	char buf_[MAX_INPUT_LENGTH];
	snprintf(buf_, sizeof(buf_),
		"--------------------------------------------------\r\n"
		"Владелец: %s\r\n"
		"В случае потери просьба вернуть за вознаграждение.\r\n"
		"--------------------------------------------------\r\n"
		, ch->get_name().c_str());
	CREATE(obj->ex_description, 1);
	obj->ex_description->keyword = str_dup(obj->PNames[0]);
	obj->ex_description->description = str_dup(buf_);
	obj->ex_description->next = 0;

	GET_OBJ_TYPE(obj) = obj_flag_data::ITEM_CONTAINER;
	GET_OBJ_WEAR(obj) = to_underlying(EWearFlag::ITEM_WEAR_TAKE);
	GET_OBJ_VAL(obj, 0) = 0;
	// CLOSEABLE + CLOSED
	GET_OBJ_VAL(obj, 1) = 5;
	GET_OBJ_VAL(obj, 2) = -1;
	GET_OBJ_VAL(obj, 3) = ch->get_uid();

	obj->set_rent(0);
	obj->set_rent_eq(0);
	// чтобы скавенж мобов не трогать
	obj->set_cost(2);
	obj->set_extraflag(EExtraFlag::ITEM_NODONATE);
	obj->set_extraflag(EExtraFlag::ITEM_NOSELL);

	return obj;
}

bool is_purse(OBJ_DATA *obj)
{
	return GET_OBJ_RNUM(obj) == PURSE_RNUM;
}

/// вываливаем и пуржим кошелек при попытке открыть или при взятии хозяином
void process_open_purse(CHAR_DATA *ch, OBJ_DATA *obj)
{
	REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
	char buf_[MAX_INPUT_LENGTH];
	snprintf(buf_, sizeof(buf_), "all");
	get_from_container(ch, obj, buf_, FIND_OBJ_INV, 1, false);
	act("$o рассыпал$U в ваших руках...", FALSE, ch, obj, 0, TO_CHAR);
	extract_obj(obj);
}

} // namespace system_obj

int ObjVal::get(const EValueKey key) const
{
	auto i = m_values.find(key);
	if (i != m_values.end())
	{
		return i->second;
	}
	return -1;
}

void ObjVal::set(const EValueKey key, int val)
{
	if (val >= 0)
	{
		m_values[key] = val;
	}
	else
	{
		auto i = m_values.find(key);
		if (i != m_values.end())
		{
			m_values.erase(i);
		}
	}
}

void ObjVal::inc(const ObjVal::EValueKey key, int val)
{
	auto i = m_values.find(key);
	if (i == m_values.end() || val == 0) return;

	if (val < 0 && i->second + val <= 0)
	{
		i->second = 0;
		return;
	}
	else if (val >= std::numeric_limits<int>::max() - i->second)
	{
		i->second = std::numeric_limits<int>::max();
		return;
	}

	i->second += val;
}

std::string ObjVal::print_to_file() const
{
	if (m_values.empty()) return "";

	std::stringstream out;
	out << "Vals:\n";

	for(auto i = m_values.begin(), iend = m_values.end(); i != iend; ++i)
	{
		std::string key_str = TextId::to_str(TextId::OBJ_VALS, to_underlying(i->first));
		if (!key_str.empty())
		{
			out << key_str << " " << i->second << "\n";
		}
	}
	out << "~\n";

	return out.str();
}

bool ObjVal::init_from_file(const char *str)
{
	m_values.clear();
	std::stringstream text(str);
	std::string key_str;
	bool result = true;
	int val = -1;

	while (text >> key_str >> val)
	{
		const int key = TextId::to_num(TextId::OBJ_VALS, key_str);
		if (key >= 0 && val >= 0)
		{
			m_values.emplace(static_cast<EValueKey>(key), val);
			key_str.clear();
			val = -1;
		}
		else
		{
			err_log("invalid key=%d or val=%d (%s %s:%d)",
				key, val, __func__, __FILE__, __LINE__);
		}
	}

	return result;
}

std::string ObjVal::print_to_zone() const
{
	if (m_values.empty()) return "";

	std::stringstream out;

	for(auto i = m_values.begin(), iend = m_values.end(); i != iend; ++i)
	{
		std::string key_str = TextId::to_str(TextId::OBJ_VALS, to_underlying(i->first));
		if (!key_str.empty())
		{
			out << "V " << key_str << " " << i->second << "\n";
		}
	}

	return out.str();
}

void ObjVal::init_from_zone(const char *str)
{
	std::stringstream text(str);
	std::string key_str;
	int val = -1;

	if (text >> key_str >> val)
	{
		const int key = TextId::to_num(TextId::OBJ_VALS, key_str);
		if (key >= 0 && val >= 0)
		{
			m_values.emplace(static_cast<EValueKey>(key), val);
		}
		else
		{
			err_log("invalid key=%d or val=%d (%s %s:%d)",
				key, val, __func__, __FILE__, __LINE__);
		}
	}
}

bool is_valid_drinkcon(const ObjVal::EValueKey key)
{
	switch(key)
	{
	case ObjVal::EValueKey::POTION_SPELL1_NUM:
	case ObjVal::EValueKey::POTION_SPELL1_LVL:
	case ObjVal::EValueKey::POTION_SPELL2_NUM:
	case ObjVal::EValueKey::POTION_SPELL2_LVL:
	case ObjVal::EValueKey::POTION_SPELL3_NUM:
	case ObjVal::EValueKey::POTION_SPELL3_LVL:
	case ObjVal::EValueKey::POTION_PROTO_VNUM:
		return true;
	}
	return false;
}

void ObjVal::remove_incorrect_keys(int type)
{
	for (auto i = m_values.begin(); i != m_values.end(); /* empty */)
	{
		bool erased = false;
		switch(type)
		{
		case obj_flag_data::ITEM_DRINKCON:
		case obj_flag_data::ITEM_FOUNTAIN:
			if (!is_valid_drinkcon(i->first))
			{
				i = m_values.erase(i);
				erased = true;
			}
			break;
		} // switch

		if (!erased)
		{
			++i;
		}
	}
}

std::string print_obj_affects(const obj_affected_type &affect)
{
	sprinttype(affect.location, apply_types, buf2);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++)
	{
		if (!str_cmp(buf2, apply_negative[j]))
		{
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0)
	{
		negative = true;
	}
	else if (negative && affect.modifier < 0)
	{
		negative = false;
	}

	sprintf(buf, "%s%s%s%s%s%d%s\r\n",
		KCYN, buf2, KNRM,
		KCYN, (negative ? " ухудшает на " : " улучшает на "),
		abs(affect.modifier), KNRM);

	return std::string(buf);
}

void print_obj_affects(CHAR_DATA *ch, const obj_affected_type &affect)
{
	sprinttype(affect.location, apply_types, buf2);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++)
	{
		if (!str_cmp(buf2, apply_negative[j]))
		{
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0)
	{
		negative = true;
	}
	else if (negative && affect.modifier < 0)
	{
		negative = false;
	}
	sprintf(buf, "   %s%s%s%s%s%d%s\r\n",
		CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM),
		negative ? " ухудшает на " : " улучшает на ", abs(affect.modifier), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
}

typedef std::map<obj_flag_data::EObjectType, std::string> EObjectType_name_by_value_t;
typedef std::map<const std::string, obj_flag_data::EObjectType> EObjectType_value_by_name_t;
EObjectType_name_by_value_t EObjectType_name_by_value;
EObjectType_value_by_name_t EObjectType_value_by_name;
void init_EObjectType_ITEM_NAMES()
{
	EObjectType_value_by_name.clear();
	EObjectType_name_by_value.clear();

	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_LIGHT] = "ITEM_LIGHT";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_SCROLL] = "ITEM_SCROLL";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_WAND] = "ITEM_WAND";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_STAFF] = "ITEM_STAFF";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_WEAPON] = "ITEM_WEAPON";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_FIREWEAPON] = "ITEM_FIREWEAPON";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_MISSILE] = "ITEM_MISSILE";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_TREASURE] = "ITEM_TREASURE";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_ARMOR] = "ITEM_ARMOR";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_POTION] = "ITEM_POTION";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_WORN] = "ITEM_WORN";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_OTHER] = "ITEM_OTHER";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_TRASH] = "ITEM_TRASH";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_TRAP] = "ITEM_TRAP";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_CONTAINER] = "ITEM_CONTAINER";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_NOTE] = "ITEM_NOTE";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_DRINKCON] = "ITEM_DRINKCON";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_KEY] = "ITEM_KEY";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_FOOD] = "ITEM_FOOD";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_MONEY] = "ITEM_MONEY";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_PEN] = "ITEM_PEN";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_BOAT] = "ITEM_BOAT";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_FOUNTAIN] = "ITEM_FOUNTAIN";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_BOOK] = "ITEM_BOOK";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_INGREDIENT] = "ITEM_INGREDIENT";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_MING] = "ITEM_MING";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_MATERIAL] = "ITEM_MATERIAL";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_BANDAGE] = "ITEM_BANDAGE";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_ARMOR_LIGHT] = "ITEM_ARMOR_LIGHT";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_ARMOR_MEDIAN] = "ITEM_ARMOR_MEDIAN";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_ARMOR_HEAVY] = "ITEM_ARMOR_HEAVY";
	EObjectType_name_by_value[obj_flag_data::EObjectType::ITEM_ENCHANT] = "ITEM_ENCHANT";

	for (const auto& i : EObjectType_name_by_value)
	{
		EObjectType_value_by_name[i.second] = i.first;
	}
}

template <>
const std::string& NAME_BY_ITEM<obj_flag_data::EObjectType>(const obj_flag_data::EObjectType item)
{
	if (EObjectType_name_by_value.empty())
	{
		init_EObjectType_ITEM_NAMES();
	}
	return EObjectType_name_by_value.at(item);
}

template <>
obj_flag_data::EObjectType ITEM_BY_NAME(const std::string& name)
{
	if (EObjectType_name_by_value.empty())
	{
		init_EObjectType_ITEM_NAMES();
	}
	return EObjectType_value_by_name.at(name);
}

typedef std::map<obj_flag_data::EObjectMaterial, std::string> EObjectMaterial_name_by_value_t;
typedef std::map<const std::string, obj_flag_data::EObjectMaterial> EObjectMaterial_value_by_name_t;
EObjectMaterial_name_by_value_t EObjectMaterial_name_by_value;
EObjectMaterial_value_by_name_t EObjectMaterial_value_by_name;
void init_EObjectMaterial_ITEM_NAMES()
{
	EObjectMaterial_value_by_name.clear();
	EObjectMaterial_name_by_value.clear();

	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_NONE] = "MAT_NONE";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_BULAT] = "MAT_BULAT";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_BRONZE] = "MAT_BRONZE";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_IRON] = "MAT_IRON";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_STEEL] = "MAT_STEEL";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_SWORDSSTEEL] = "MAT_SWORDSSTEEL";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_COLOR] = "MAT_COLOR";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_CRYSTALL] = "MAT_CRYSTALL";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_WOOD] = "MAT_WOOD";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_SUPERWOOD] = "MAT_SUPERWOOD";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_FARFOR] = "MAT_FARFOR";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_GLASS] = "MAT_GLASS";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_ROCK] = "MAT_ROCK";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_BONE] = "MAT_BONE";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_MATERIA] = "MAT_MATERIA";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_SKIN] = "MAT_SKIN";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_ORGANIC] = "MAT_ORGANIC";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_PAPER] = "MAT_PAPER";
	EObjectMaterial_name_by_value[obj_flag_data::EObjectMaterial::MAT_DIAMOND] = "MAT_DIAMOND";

	for (const auto& i : EObjectMaterial_name_by_value)
	{
		EObjectMaterial_value_by_name[i.second] = i.first;
	}
}

template <>
const std::string& NAME_BY_ITEM<obj_flag_data::EObjectMaterial>(const obj_flag_data::EObjectMaterial item)
{
	if (EObjectMaterial_name_by_value.empty())
	{
		init_EObjectMaterial_ITEM_NAMES();
	}
	return EObjectMaterial_name_by_value.at(item);
}

template <>
obj_flag_data::EObjectMaterial ITEM_BY_NAME(const std::string& name)
{
	if (EObjectMaterial_name_by_value.empty())
	{
		init_EObjectMaterial_ITEM_NAMES();
	}
	return EObjectMaterial_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
