// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_HPP_INCLUDED
#define OBJ_HPP_INCLUDED

#include <vector>
#include <boost/array.hpp>
#include <string>
#include <map>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/* object flags; used in obj_data */
#define NUM_OBJ_VAL_POSITIONS 4

struct obj_flag_data
{
	boost::array<int, NUM_OBJ_VAL_POSITIONS> value;
	int type_flag;		/* Type of item               */
	int
	wear_flags;		/* Where you can wear it     */
	FLAG_DATA extra_flags;	/* If it hums, glows, etc.      */
	int
	weight;		/* Weigt what else              */
	int
	cost;			/* Value when sold (gp.)        */
	int
	cost_per_day_on;	/* Rent to keep pr. real day if wear       */
	int
	cost_per_day_off;	/* Rent to keep pr. real day if in inv     */
	FLAG_DATA bitvector;	/* To set chars bits            */

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
	Obj_maker;		/* Unique number for object crafters */
	int
	Obj_parent;		/* Vnum for object parent */
};

struct obj_affected_type
{
	int location;		/* Which ability to change (APPLY_XXX) */
	int modifier;		/* How much it changes by              */

	obj_affected_type() : location(APPLY_NONE), modifier(0) {}

	obj_affected_type(int __location, int __modifier)
		: location(__location), modifier(__modifier) {}
};

class activation
{
	std::string actmsg, deactmsg, room_actmsg, room_deactmsg;
	flag_data affects;
	boost::array<obj_affected_type, MAX_OBJ_AFFECT> affected;
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

	/**
	 * @warning Предполагается, что __out_skills.empty() == true.
	 */
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

	const boost::array<obj_affected_type, MAX_OBJ_AFFECT>&
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
};

typedef std::map< int, set_info > id_to_set_info_map;

extern std::vector < OBJ_DATA * >obj_proto;

/**
* Временное заклинание на предмете (одно).
*/
class TimedSpell
{
public:
	bool check_spell(int spell) const;
	int is_spell_poisoned() const;
	bool empty() const;
	std::string print() const;
	void dec_timer(OBJ_DATA *obj, int time = 1);
	void add(OBJ_DATA *obj, int spell, int time);
	std::string diag_to_char(CHAR_DATA *ch);

private:
	void remove_spell(OBJ_DATA *obj, int spell, bool message);

	std::map<int /* номер заклинания (SPELL_ХХХ) */, int /* таймер в минутах */> spell_list_;
};

////////////////////////////////////////////////////////////////////////////////

enum { ACQUIRED_ENCHANT, ACQUIRED_STONE, ACQUIRED_TOTAL_TYPES };

// список аффектов от какого-то одного источника (энчанта, камня)
class AcquiredAffects
{
	// имя источника аффектов
	std::string name_;
	// тип источника аффектов
	int type_;
	// список APPLY аффектов (affected[MAX_OBJ_AFFECT])
	std::vector<obj_affected_type> affected_;
	// аффекты обкаста (obj_flags.affects)
	FLAG_DATA affects_flags_;
	// экстра аффекты (obj_flags.extra_flags)
	FLAG_DATA extra_flags_;
	// запреты на ношение (obj_flags.no_flag)
	FLAG_DATA no_flags_;
	// изменение веса (+-)
	int weight_;
	// для лоада из файла объектов
	AcquiredAffects();

public:
	// инит свои аффекты из указанного предмета (для энчантов)
	AcquiredAffects(OBJ_DATA *obj);
	// добавить свои аффектф на предмет (на случай стирания, например в сетах)
	void apply_to_obj(OBJ_DATA *obj) const;
	// распечатка аффектов для опознания
	void print(CHAR_DATA *ch) const;
	// тип источника (для удаления из предмета)
	int get_type() const;
	// генерация строки с энчантом для файла объекта
	std::string print_to_file() const;
	// для лоада из файла объектов
	friend OBJ_DATA *read_one_object_new(char **data, int *error);
};

////////////////////////////////////////////////////////////////////////////////

struct obj_data
{
	obj_data();
	obj_data(const obj_data&);
	~obj_data();

	unsigned int uid;
	obj_vnum item_number;	/* Where in data-base            */
	room_rnum in_room;	/* In what room -1 when conta/carr */

	struct obj_flag_data obj_flags;		/* Object information       */
	boost::array<obj_affected_type, MAX_OBJ_AFFECT> affected;	/* affects */

	char *aliases;		/* Title of object :get etc.        */
	char *description;	/* When in room                     */
	char *short_description;	/* when worn/carry/in cont.         */
	char *action_description;	/* What to write when used          */
	EXTRA_DESCR_DATA *ex_description;	/* extra descriptions     */
	CHAR_DATA *carried_by;	/* Carried by :NULL in room/conta   */
	CHAR_DATA *worn_by;	/* Worn by?              */
	short int
	worn_on;		/* Worn where?          */

	OBJ_DATA *in_obj;	/* In what object NULL when none    */
	OBJ_DATA *contains;	/* Contains objects                 */

	long id;			/* used by DG triggers              */
	struct trig_proto_list *proto_script;	/* list of default triggers  */
	struct script_data *script;	/* script info for the object       */

	OBJ_DATA *next_content;	/* For 'contains' lists             */
	OBJ_DATA *next;		/* For the object list              */
	int
	room_was_in;
	boost::array<char *, 6> PNames;
	int
	max_in_world;		/* max in world             */

	TimedSpell timed_spell;    // временный обкаст
	std::vector<AcquiredAffects> acquired_affects;

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
};

namespace ObjSystem
{

bool is_armor_type(const OBJ_DATA *obj);
void release_purged_list();
void init_item_levels();
void init_ilvl(OBJ_DATA *obj);
bool is_mob_item(OBJ_DATA *obj);

} // namespace ObjSystem

#endif // OBJ_HPP_INCLUDED
