// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <sstream>
#include <cmath>
#include <boost/algorithm/string.hpp>

#include "obj.hpp"
#include "utils.h"
#include "pk.h"
#include "celebrates.hpp"
#include "cache.hpp"
#include "screen.h"
#include "comm.h"
#include "char.hpp"
#include "db.h"
#include "constants.h"
#include "handler.h"
#include "parse.hpp"

extern void get_from_container(CHAR_DATA * ch, OBJ_DATA * cont, char *arg, int mode, int amount, bool autoloot);

id_to_set_info_map obj_data::set_table;

namespace
{

// список шмоток после пуржа для последующего удаления оболочки
typedef std::vector<OBJ_DATA *> PurgedObjList;
PurgedObjList purged_obj_list;

} // namespace

obj_data::obj_data()
{
	this->zero_init();
	caching::obj_cache.add(this);
}

obj_data::obj_data(const obj_data& other)
{
	*this = other;
	caching::obj_cache.add(this);
}

obj_data::~obj_data()
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
void obj_data::zero_init()
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
	proto_script = NULL;
	script = NULL;
	next_content = NULL;
	next = NULL;
	room_was_in = NOWHERE;
	max_in_world = 0;
	skills = NULL;
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
void obj_data::purge(bool destructor)
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

bool obj_data::purged() const
{
	return purged_;
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

// * @warning Предполагается, что __out_skills.empty() == true.
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

int obj_data::get_manual_mort_req() const
{
	return manual_mort_req_;
}

void obj_data::set_manual_mort_req(int param)
{
	manual_mort_req_ = param;
}

unsigned obj_data::get_ilevel() const
{
	return ilevel_;
}

void obj_data::set_ilevel(unsigned ilvl)
{
	ilevel_ = ilvl;
}

int obj_data::get_mort_req() const
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

int obj_data::get_cost() const
{
	return cost_;
}

void obj_data::set_cost(int x)
{
	if (x >= 0)
	{
		cost_ = x;
	}
}

int obj_data::get_rent() const
{
	return cost_per_day_off_;
}

void obj_data::set_rent(int x)
{
	if (x >= 0)
	{
		cost_per_day_off_ = x;
	}
}

int obj_data::get_rent_eq() const
{
	return cost_per_day_on_;
}

void obj_data::set_rent_eq(int x)
{
	if (x >= 0)
	{
		cost_per_day_on_ = x;
	}
}

void obj_data::set_activator(bool flag, int num)
{
	activator_.first = flag;
	activator_.second = num;
}

std::pair<bool, int> obj_data::get_activator() const
{
	return activator_;
}

////////////////////////////////////////////////////////////////////////////////

namespace
{

const float SQRT_MOD = 1.7095f;
const int AFF_SHIELD_MOD = 30;
const int AFF_BLINK_MOD = 10;

float count_affect_weight(OBJ_DATA *obj, int num, int mod)
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

} // namespace

////////////////////////////////////////////////////////////////////////////////

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
	if (IS_OBJ_NO(obj, ITEM_NO_MALE)
		&& IS_OBJ_NO(obj, ITEM_NO_FEMALE)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_MONO)
		&& IS_OBJ_NO(obj, ITEM_NO_POLY)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_RUSICHI)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_CLERIC)
		&& IS_OBJ_NO(obj, ITEM_NO_THIEF)
		&& IS_OBJ_NO(obj, ITEM_NO_WARRIOR)
		&& IS_OBJ_NO(obj, ITEM_NO_ASSASINE)
		&& IS_OBJ_NO(obj, ITEM_NO_GUARD)
		&& IS_OBJ_NO(obj, ITEM_NO_PALADINE)
		&& IS_OBJ_NO(obj, ITEM_NO_RANGER)
		&& IS_OBJ_NO(obj, ITEM_NO_SMITH)
		&& IS_OBJ_NO(obj, ITEM_NO_MERCHANT)
		&& IS_OBJ_NO(obj, ITEM_NO_DRUID)
		&& IS_OBJ_NO(obj, ITEM_NO_BATTLEMAGE)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMMAGE)
		&& IS_OBJ_NO(obj, ITEM_NO_DEFENDERMAGE)
		&& IS_OBJ_NO(obj, ITEM_NO_NECROMANCER)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_SEVERANE)
		&& IS_OBJ_NO(obj, ITEM_NO_POLANE)
		&& IS_OBJ_NO(obj, ITEM_NO_KRIVICHI)
		&& IS_OBJ_NO(obj, ITEM_NO_VATICHI)
		&& IS_OBJ_NO(obj, ITEM_NO_VELANE)
		&& IS_OBJ_NO(obj, ITEM_NO_DREVLANE)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_MALE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_FEMALE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_MONO)
		&& IS_OBJ_ANTI(obj, ITEM_AN_POLY)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_RUSICHI)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_CLERIC)
		&& IS_OBJ_ANTI(obj, ITEM_AN_THIEF)
		&& IS_OBJ_ANTI(obj, ITEM_AN_WARRIOR)
		&& IS_OBJ_ANTI(obj, ITEM_AN_ASSASINE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_GUARD)
		&& IS_OBJ_ANTI(obj, ITEM_AN_PALADINE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_RANGER)
		&& IS_OBJ_ANTI(obj, ITEM_AN_SMITH)
		&& IS_OBJ_ANTI(obj, ITEM_AN_MERCHANT)
		&& IS_OBJ_ANTI(obj, ITEM_AN_DRUID)
		&& IS_OBJ_ANTI(obj, ITEM_AN_BATTLEMAGE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMMAGE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_DEFENDERMAGE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_NECROMANCER)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_SEVERANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_POLANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_KRIVICHI)
		&& IS_OBJ_ANTI(obj, ITEM_AN_VATICHI)
		&& IS_OBJ_ANTI(obj, ITEM_AN_VELANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_DREVLANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}

	return false;
}

void init_ilvl(OBJ_DATA *obj)
{
	if (is_mob_item(obj)
		|| OBJ_FLAGGED(obj, ITEM_SETSTUFF)
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
		float weight = count_affect_weight(obj, obj->affected[k].location,
			obj->affected[k].modifier);
		total_weight += pow(weight, SQRT_MOD);
	}
	// аффекты AFF_x через weapon_affect
	for (int m = 0; weapon_affect[m].aff_bitvector != -1; ++m)
	{
		if (weapon_affect[m].aff_bitvector == AFF_AIRSHIELD
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
		}
		else if (weapon_affect[m].aff_bitvector == AFF_FIRESHIELD
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
		}
		else if (weapon_affect[m].aff_bitvector == AFF_ICESHIELD
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
		}
		else if (weapon_affect[m].aff_bitvector == AFF_BLINK
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
		}
	}

	obj->set_ilevel(ceil(pow(total_weight, 1/SQRT_MOD)));
}

void init_item_levels()
{
	for (std::vector <OBJ_DATA *>::iterator i = obj_proto.begin(),
		iend = obj_proto.end(); i != iend; ++i)
	{
		init_ilvl(*i);
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

void update_rnum(int &obj_rnum, int obj_vnum, int rnum)
{
	if (obj_rnum == -1)
	{
		obj_rnum = real_object(obj_vnum);
	}
	else if (obj_rnum >= rnum)
	{
		obj_rnum += 1;
	}
}

/// при добавлении предметов через олц (renumber_obj_rnum)
void renumber(int rnum)
{
	update_rnum(PURSE_RNUM, PURSE_VNUM, rnum);
	update_rnum(PERS_CHEST_RNUM, PERS_CHEST_VNUM, rnum);
}

OBJ_DATA* create_purse(CHAR_DATA *ch, int gold)
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
		, ch->get_name());
	CREATE(obj->ex_description, EXTRA_DESCR_DATA, 1);
	obj->ex_description->keyword = str_dup(obj->PNames[0]);
	obj->ex_description->description = str_dup(buf_);
	obj->ex_description->next = 0;

	GET_OBJ_TYPE(obj) = ITEM_CONTAINER;
	GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
	GET_OBJ_VAL(obj, 0) = 0;
	// CLOSEABLE + CLOSED
	GET_OBJ_VAL(obj, 1) = 5;
	GET_OBJ_VAL(obj, 2) = -1;
	GET_OBJ_VAL(obj, 3) = ch->get_uid();

	obj->set_rent(0);
	obj->set_rent_eq(0);
	// чтобы скавенж мобов не трогать
	obj->set_cost(2);
	SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODONATE), ITEM_NODONATE);
	SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NOSELL), ITEM_NOSELL);

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

int ObjVal::get(unsigned key) const
{
	auto i = list_.find(key);
	if (i != list_.end())
	{
		return i->second;
	}
	return -1;
}

void ObjVal::set(unsigned key, int val)
{
	if (val >= 0)
	{
		list_[key] = val;
	}
	else
	{
		auto i = list_.find(key);
		if (i != list_.end())
		{
			list_.erase(i);
		}
	}
}

void ObjVal::inc(unsigned key, int val)
{
	auto i = list_.find(key);
	if (i == list_.end() || val == 0) return;

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
	if (list_.empty()) return "";

	std::stringstream out;
	out << "Vals:\n";

	for(auto i = list_.begin(), iend = list_.end(); i != iend; ++i)
	{
		std::string key_str = TextId::to_str(TextId::OBJ_VALS, i->first);
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
	list_.clear();
	std::stringstream text(str);
	std::string key_str;
	bool result = true;
	int val = -1;

	while (text >> key_str >> val)
	{
		int key = TextId::to_num(TextId::OBJ_VALS, key_str);
		if (key >= 0 && val >= 0)
		{
			list_.emplace(key, val);
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
	if (list_.empty()) return "";

	std::stringstream out;

	for(auto i = list_.begin(), iend = list_.end(); i != iend; ++i)
	{
		std::string key_str = TextId::to_str(TextId::OBJ_VALS, i->first);
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
		int key = TextId::to_num(TextId::OBJ_VALS, key_str);
		if (key >= 0 && val >= 0)
		{
			list_.emplace(key, val);
		}
		else
		{
			err_log("invalid key=%d or val=%d (%s %s:%d)",
				key, val, __func__, __FILE__, __LINE__);
		}
	}
}

bool is_valid_drinkcon(unsigned key)
{
	switch(key)
	{
	case ObjVal::POTION_SPELL1_NUM:
	case ObjVal::POTION_SPELL1_LVL:
	case ObjVal::POTION_SPELL2_NUM:
	case ObjVal::POTION_SPELL2_LVL:
	case ObjVal::POTION_SPELL3_NUM:
	case ObjVal::POTION_SPELL3_LVL:
	case ObjVal::POTION_PROTO_VNUM:
		return true;
	}
	return false;
}

void ObjVal::remove_incorrect_keys(int type)
{
	for (auto i = list_.begin(); i != list_.end(); /* empty */)
	{
		bool erased = false;
		switch(type)
		{
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
			if (!is_valid_drinkcon(i->first))
			{
				i = list_.erase(i);
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
