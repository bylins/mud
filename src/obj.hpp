// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_HPP_INCLUDED
#define OBJ_HPP_INCLUDED

#include "conf.h"
#include <vector>
#include <map>
#include <string>
//#include <unordered_map>
#include <array>
#include <boost/array.hpp>
#include <boost/unordered_map.hpp>

#include "sysdep.h"
#include "structs.h"
#include "obj_enchant.hpp"

// object flags; used in obj_data //
#define NUM_OBJ_VAL_POSITIONS 4

struct obj_flag_data
{
	boost::array<int, NUM_OBJ_VAL_POSITIONS> value;
	int type_flag;		// Type of item               //
	uint32_t wear_flags;		// Where you can wear it     //
	FLAG_DATA extra_flags;	// If it hums, glows, etc.      //
	int
	weight;		// Weigt what else              //
	FLAG_DATA bitvector;	// To set chars bits            //

	FLAG_DATA affects;
	FLAG_DATA anti_flag;
	FLAG_DATA no_flag;
	int
	Obj_sex;
	int
	Obj_spell;
	int
	Obj_level;
	int
	Obj_skill;
	int
	Obj_max;
	int
	Obj_cur;
	int
	Obj_mater;
	int
	Obj_owner;
	int
	Obj_destroyer;
	int
	Obj_zone;
	int
	Obj_maker;		// Unique number for object crafters //
	int
	Obj_parent;		// Vnum for object parent //
};

struct obj_affected_type
{
	int location;		// Which ability to change (APPLY_XXX) //
	int modifier;		// How much it changes by              //

	obj_affected_type() : location(APPLY_NONE), modifier(0) {}

	obj_affected_type(int __location, int __modifier)
		: location(__location), modifier(__modifier) {}

	// для сравнения в sedit
	bool operator!=(const obj_affected_type &r) const
	{
		return (location != r.location || modifier != r.modifier);
	}
	bool operator==(const obj_affected_type &r) const
	{
		return !(*this != r);
	}
};

class activation
{
	std::string actmsg, deactmsg, room_actmsg, room_deactmsg;
	flag_data affects;
	std::array<obj_affected_type, MAX_OBJ_AFFECT> affected;
	int weight, ndices, nsides;
	std::map<int, int> skills;

public:
	activation() : affects(clear_flags), weight(-1), ndices(-1), nsides(-1) {}

	activation(const std::string& __actmsg, const std::string& __deactmsg,
			   const std::string& __room_actmsg, const std::string& __room_deactmsg,
			   const flag_data& __affects, const obj_affected_type* __affected,
			   int __weight, int __ndices, int __nsides):
			actmsg(__actmsg), deactmsg(__deactmsg), room_actmsg(__room_actmsg),
			room_deactmsg(__room_deactmsg), affects(__affects), weight(__weight),
			ndices(__ndices), nsides(__nsides)
	{
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = __affected[i];
	}

	bool has_skills() const
	{
		return !skills.empty();
	}

	// * @warning Предполагается, что __out_skills.empty() == true.
	void get_skills(std::map<int, int>& __skills) const
	{
		__skills.insert(skills.begin(), skills.end());
	}

	activation&
	set_skill(int __skillnum, int __percent)
	{
		std::map<int, int>::iterator skill = skills.find(__skillnum);
		if (skill == skills.end())
		{
			if (__percent != 0)
				skills.insert(std::make_pair(__skillnum, __percent));
		}
		else
		{
			if (__percent != 0)
				skill->second = __percent;
			else
				skills.erase(skill);
		}

		return *this;
	}

	void get_dices(int& __ndices, int& __nsides) const
	{
		__ndices = ndices;
		__nsides = nsides;
	}

	activation&
	set_dices(int __ndices, int __nsides)
	{
		ndices = __ndices;
		nsides = __nsides;
		return *this;
	}

	int get_weight() const
	{
		return weight;
	}

	activation&
	set_weight(int __weight)
	{
		weight = __weight;
		return *this;
	}

	const std::string&
	get_actmsg() const
	{
		return actmsg;
	}

	activation&
	set_actmsg(const std::string& __actmsg)
	{
		actmsg = __actmsg;
		return *this;
	}

	const std::string&
	get_deactmsg() const
	{
		return deactmsg;
	}

	activation&
	set_deactmsg(const std::string& __deactmsg)
	{
		deactmsg = __deactmsg;
		return *this;
	}

	const std::string&
	get_room_actmsg() const
	{
		return room_actmsg;
	}

	activation&
	set_room_actmsg(const std::string& __room_actmsg)
	{
		room_actmsg = __room_actmsg;
		return *this;
	}

	const std::string&
	get_room_deactmsg() const
	{
		return room_deactmsg;
	}

	activation&
	set_room_deactmsg(const std::string& __room_deactmsg)
	{
		room_deactmsg = __room_deactmsg;
		return *this;
	}

	const flag_data&
	get_affects() const
	{
		return affects;
	}

	activation&
	set_affects(const flag_data& __affects)
	{
		affects = __affects;
		return *this;
	}

	const std::array<obj_affected_type, MAX_OBJ_AFFECT>&
	get_affected() const
	{
		return affected;
	}

	activation&
	set_affected(const obj_affected_type* __affected)
	{
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = __affected[i];
		return *this;
	}

	const obj_affected_type&
	get_affected_i(int __i) const
	{
		return __i < 0              ? affected[0] :
			   __i < MAX_OBJ_AFFECT ? affected[__i] : affected[MAX_OBJ_AFFECT-1];
	}

	activation&
	set_affected_i(int __i, const obj_affected_type& __affected)
	{
		if (__i >= 0 && __i < MAX_OBJ_AFFECT)
			affected[__i] = __affected;

		return *this;
	}
};

typedef std::map< unique_bit_flag_data, activation > class_to_act_map;

typedef std::map< unsigned int, class_to_act_map > qty_to_camap_map;

class set_info : public std::map< obj_vnum, qty_to_camap_map >
{
	std::string name;
	std::string alias;
public:
	typedef std::map< obj_vnum, qty_to_camap_map > ovnum_to_qamap_map;

	set_info() {}

	set_info(const ovnum_to_qamap_map& __base, const std::string& __name) : ovnum_to_qamap_map(__base), name(__name) {}

	const std::string&
	get_name() const
	{
		return name;
	}

	set_info&
	set_name(const std::string& __name)
	{
		name = __name;
		return *this;
	}

	const std::string& get_alias() const
	{
		return alias;
	}

	void set_alias(const std::string & _alias)
	{
		alias = _alias;
	}
};

typedef std::map< int, set_info > id_to_set_info_map;

extern std::vector < OBJ_DATA * >obj_proto;

// * Временное заклинание на предмете (одно).
class TimedSpell
{
public:
	bool check_spell(int spell) const;
	int is_spell_poisoned() const;
	bool empty() const;
	std::string print() const;
	void dec_timer(OBJ_DATA *obj, int time = 1);
	void add(OBJ_DATA *obj, int spell, int time);
	void del(OBJ_DATA *obj, int spell, bool message);
	std::string diag_to_char(CHAR_DATA *ch);

private:
	std::map<int /* номер заклинания (SPELL_ХХХ) */, int /* таймер в минутах */> spell_list_;
};

// метки для команды "нацарапать"
struct custom_label {
	char *label_text; // текст
	char *clan;       // аббревиатура клана, если метка предназначена для клана
	int author;       // кем нанесена: содержит результат ch->get_idnum(), по умолчанию -2
	char *author_mail;// будем проверять по емейлу тоже
};

struct custom_label *init_custom_label();
void free_custom_label(struct custom_label *);

/// Чуть более гибкий, но не менее упоротый аналог GET_OBJ_VAL полей
/// Если поле нужно сохранять в обж-файл - вписываем в TextId::init_obj_vals()
/// Соответствие полей и типов предметов смотреть/обновлять в remove_incorrect_keys()
class ObjVal
{
public:
	// \return -1 - ключ не был найден
	int get(unsigned key) const;
	// сет новой записи/обновление существующей
	// \param val < 0 - запись (если была) удаляется
	void set(unsigned key, int val);
	// если key не найден, то ничего не сетится
	// \param val допускается +-
	void inc(unsigned key, int val = 1);
	// save/load в файлы предметов
	std::string print_to_file() const;
	bool init_from_file(const char *str);
	// тоже самое в файлы зон
	std::string print_to_zone() const;
	void init_from_zone(const char *str);
	// для сравнения с прототипом
	bool operator==(const ObjVal &r) const
	{
		return list_ == r.list_;
	}
	bool operator!=(const ObjVal &r) const
	{
		return list_ != r.list_;
	}
	// чистка левых параметров (поменяли тип предмета в олц/файле и т.п.)
	// дергается после редактирований в олц, лоада прототипов и просто шмоток
	void remove_incorrect_keys(int type);

	enum
	{
		// номер и уровень заклинаний в зелье/емкости с зельем
		POTION_SPELL1_NUM,
		POTION_SPELL1_LVL,
		POTION_SPELL2_NUM,
		POTION_SPELL2_LVL,
		POTION_SPELL3_NUM,
		POTION_SPELL3_LVL,
		// внум прототипа зелья, перелитого в емкость
		// 0 - если зелье в емкости проставлено через олц
		POTION_PROTO_VNUM
	};

private:
//	std::unordered_map<unsigned, int> list_;
	boost::unordered_map<unsigned, int> list_;
};

struct obj_data
{
	obj_data();
	obj_data(const obj_data&);
	~obj_data();

	unsigned int uid;
	obj_vnum item_number;	// Where in data-base            //
	room_rnum in_room;	// In what room -1 when conta/carr //

	struct obj_flag_data obj_flags;		// Object information       //
	std::array<obj_affected_type, MAX_OBJ_AFFECT> affected;	// affects //

	char *aliases;		// Title of object :get etc.        //
	char *description;	// When in room                     //
	char *short_description;	// when worn/carry/in cont.         //
	char *action_description;	// What to write when used          //
	EXTRA_DESCR_DATA *ex_description;	// extra descriptions     //
	CHAR_DATA *carried_by;	// Carried by :NULL in room/conta   //
	CHAR_DATA *worn_by;	// Worn by?              //

	struct custom_label *custom_label;		// наносимая чаром метка //

	short int
	worn_on;		// Worn where?          //

	OBJ_DATA *in_obj;	// In what object NULL when none    //
	OBJ_DATA *contains;	// Contains objects                 //

	long id;			// used by DG triggers              //
	struct trig_proto_list *proto_script;	// list of default triggers  //
	struct script_data *script;	// script info for the object       //

	OBJ_DATA *next_content;	// For 'contains' lists             //
	OBJ_DATA *next;		// For the object list              //
	int
	room_was_in;
	boost::array<char *, 6> PNames;
	int
	max_in_world;		// max in world             //

	TimedSpell timed_spell;    // временный обкаст
	obj::Enchants enchants;
	ObjVal values;

	const std::string activate_obj(const activation& __act);
	const std::string deactivate_obj(const activation& __act);

	void set_skill(int skill_num, int percent);
	int get_skill(int skill_num) const;

	void get_skills(std::map<int, int>& out_skills) const;
	bool has_skills() const;

	int get_serial_num();
	void set_serial_num(int num);

	void set_timer(int timer);
	int get_timer() const;
	void dec_timer(int time = 1);

	static id_to_set_info_map set_table;
	static void init_set_table();

	void purge(bool destructor = false);
	bool purged() const;

	unsigned get_ilevel() const;
	void set_ilevel(unsigned ilvl);
	int get_manual_mort_req() const;
	void set_manual_mort_req(int);
	int get_mort_req() const;

	int get_cost() const;
	void set_cost(int x);
	int get_rent() const;
	void set_rent(int x);
	int get_rent_eq() const;
	void set_rent_eq(int x);

	void set_activator(bool flag, int num);
	std::pair<bool, int> get_activator() const;

private:
	void zero_init();
	// если этот массив создался, то до выхода из программы уже не удалится. тут это вроде как "нормально"
	std::map<int, int>* skills;
	// порядковый номер в списке чаров (для name_list)
	int serial_num_;
	// таймер (в минутах рл)
	int timer_;
	// если >= 0 - требование по минимальным мортам, проставленное в олц
	int manual_mort_req_;
	// true - объект спуржен и ждет вызова delete для оболочки
	bool purged_;
	// расчетный уровень шмотки, не сохраняется
	unsigned ilevel_;
	// цена шмотки при продаже
	int cost_;
	// стоимость ренты, если надета
	int cost_per_day_on_;
	// стоимость ренты, если в инве
	int cost_per_day_off_;
	// для сообщений сетов <активировано или нет, размер активатора>
	std::pair<bool, int> activator_;
};

namespace ObjSystem
{

bool is_armor_type(const OBJ_DATA *obj);
void release_purged_list();
void init_item_levels();
void init_ilvl(OBJ_DATA *obj);
bool is_mob_item(OBJ_DATA *obj);

} // namespace ObjSystem

std::string char_get_custom_label(OBJ_DATA *obj, CHAR_DATA *ch);

namespace system_obj
{

/// кошелек для кун с игрока
extern int PURSE_RNUM;
/// персональное хранилище
extern int PERS_CHEST_RNUM;

void init();
void renumber(int rnum);
OBJ_DATA* create_purse(CHAR_DATA *ch, int gold);
bool is_purse(OBJ_DATA *obj);
void process_open_purse(CHAR_DATA *ch, OBJ_DATA *obj);

} // namespace system_obj

#endif // OBJ_HPP_INCLUDED
