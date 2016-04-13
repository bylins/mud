// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_HPP_INCLUDED
#define OBJ_HPP_INCLUDED

#include "obj_enchant.hpp"
#include "features.hpp"
#include "spells.h"
#include "skills.h"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <boost/array.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <array>
#include <vector>
#include <string>
#include <map>

// object flags; used in obj_data //
#define NUM_OBJ_VAL_POSITIONS 4

struct obj_flag_data
{
public:
	typedef boost::array<int, NUM_OBJ_VAL_POSITIONS> value_t;

	enum EObjectType
	{
		ITEM_LIGHT = 1,			// Item is a light source  //
		ITEM_SCROLL = 2,		// Item is a scroll     //
		ITEM_WAND = 3,			// Item is a wand    //
		ITEM_STAFF = 4,			// Item is a staff      //
		ITEM_WEAPON = 5,		// Item is a weapon     //
		ITEM_FIREWEAPON = 6,	// Unimplemented     //
		ITEM_MISSILE = 7,		// Unimplemented     //
		ITEM_TREASURE = 8,		// Item is a treasure, not gold  //
		ITEM_ARMOR = 9,			// Item is armor     //
		ITEM_POTION = 10,		// Item is a potion     //
		ITEM_WORN = 11,			// Unimplemented     //
		ITEM_OTHER = 12,		// Misc object       //
		ITEM_TRASH = 13,		// Trash - shopkeeps won't buy   //
		ITEM_TRAP = 14,			// Unimplemented     //
		ITEM_CONTAINER = 15,	// Item is a container     //
		ITEM_NOTE = 16,			// Item is note      //
		ITEM_DRINKCON = 17,		// Item is a drink container  //
		ITEM_KEY = 18,			// Item is a key     //
		ITEM_FOOD = 19,			// Item is food         //
		ITEM_MONEY = 20,		// Item is money (gold)    //
		ITEM_PEN = 21,			// Item is a pen     //
		ITEM_BOAT = 22,			// Item is a boat    //
		ITEM_FOUNTAIN = 23,		// Item is a fountain      //
		ITEM_BOOK = 24,			// Item is book //
		ITEM_INGREDIENT = 25,	// Item is magical ingradient //
		ITEM_MING = 26,			// Магический ингредиент //
		ITEM_MATERIAL = 27,		// Материал для крафтовых умений //
		ITEM_BANDAGE = 28,		// бинты для перевязки
		ITEM_ARMOR_LIGHT = 29,	// легкий тип брони
		ITEM_ARMOR_MEDIAN = 30,	// средний тип брони
		ITEM_ARMOR_HEAVY = 31,	// тяжелый тип брони
		ITEM_ENCHANT = 32		// зачарование предмета
	};

	enum EObjectMaterial
	{
		MAT_NONE = 0,
		MAT_BULAT = 1,
		MAT_BRONZE = 2,
		MAT_IRON = 3,
		MAT_STEEL = 4,
		MAT_SWORDSSTEEL = 5,
		MAT_COLOR = 6,
		MAT_CRYSTALL = 7,
		MAT_WOOD = 8,
		MAT_SUPERWOOD = 9,
		MAT_FARFOR = 10,
		MAT_GLASS = 11,
		MAT_ROCK = 12,
		MAT_BONE = 13,
		MAT_MATERIA = 14,
		MAT_SKIN = 15,
		MAT_ORGANIC = 16,
		MAT_PAPER = 17,
		MAT_DIAMOND = 18
	};

	const static int DEFAULT_MAXIMUM_DURABILITY = 100;
	const static int DEFAULT_CURRENT_DURABILITY = DEFAULT_MAXIMUM_DURABILITY;
	const static int DEFAULT_LEVEL = 0;
	const static int DEFAULT_WEIGHT = INT_MAX;
	const static EObjectType DEFAULT_TYPE = ITEM_OTHER;
	const static EObjectMaterial DEFAULT_MATERIAL = MAT_NONE;

	value_t value;
	EObjectType type_flag;		///< Type of item               //
	std::underlying_type<EWearFlag>::type wear_flags;		// Where you can wear it     //
	FLAG_DATA extra_flags;	// If it hums, glows, etc.      //
	int weight;		// Weight what else              //
	FLAG_DATA bitvector;	// To set chars bits            //

	FLAG_DATA affects;
	FLAG_DATA anti_flag;
	FLAG_DATA no_flag;
	ESex Obj_sex;
	int Obj_spell;
	int Obj_level;
	int Obj_skill;
	int Obj_max;
	int Obj_cur;
	EObjectMaterial Obj_mater;
	int Obj_owner;
	int Obj_destroyer;
	int Obj_zone;
	int Obj_maker;		// Unique number for object crafters //
	int Obj_parent;		// Vnum for object parent //
	bool Obj_is_rename;
	int craft_timer; // таймер крафтовой вещи при создании
};

template <> const std::string& NAME_BY_ITEM<obj_flag_data::EObjectType>(const obj_flag_data::EObjectType item);
template <> obj_flag_data::EObjectType ITEM_BY_NAME<obj_flag_data::EObjectType>(const std::string& name);

template <> const std::string& NAME_BY_ITEM<obj_flag_data::EObjectMaterial>(const obj_flag_data::EObjectMaterial item);
template <> obj_flag_data::EObjectMaterial ITEM_BY_NAME<obj_flag_data::EObjectMaterial>(const std::string& name);

std::string print_obj_affects(const obj_affected_type &affect);
void print_obj_affects(CHAR_DATA *ch, const obj_affected_type &affect);

class activation
{
	std::string actmsg, deactmsg, room_actmsg, room_deactmsg;
	FLAG_DATA affects;
	std::array<obj_affected_type, MAX_OBJ_AFFECT> affected;
	int weight, ndices, nsides;
	std::map<int, int> skills;

public:
	activation() : affects(clear_flags), weight(-1), ndices(-1), nsides(-1) {}

	activation(const std::string& __actmsg, const std::string& __deactmsg,
			   const std::string& __room_actmsg, const std::string& __room_deactmsg,
			   const FLAG_DATA& __affects, const obj_affected_type* __affected,
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

	const FLAG_DATA&
	get_affects() const
	{
		return affects;
	}

	activation&
	set_affects(const FLAG_DATA& __affects)
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
	enum class EValueKey : uint32_t
	{
		// номер и уровень заклинаний в зелье/емкости с зельем
		POTION_SPELL1_NUM = 0,
		POTION_SPELL1_LVL = 1,
		POTION_SPELL2_NUM = 2,
		POTION_SPELL2_LVL = 3,
		POTION_SPELL3_NUM = 4,
		POTION_SPELL3_LVL = 5,
		// внум прототипа зелья, перелитого в емкость
		// 0 - если зелье в емкости проставлено через олц
		POTION_PROTO_VNUM = 6
	};

	// \return -1 - ключ не был найден
	int get(const EValueKey key) const;
	// сет новой записи/обновление существующей
	// \param val < 0 - запись (если была) удаляется
	void set(const EValueKey key, int val);
	// если key не найден, то ничего не сетится
	// \param val допускается +-
	void inc(const EValueKey key, int val = 1);
	// save/load в файлы предметов
	std::string print_to_file() const;
	bool init_from_file(const char *str);
	// тоже самое в файлы зон
	std::string print_to_zone() const;
	void init_from_zone(const char *str);
	// для сравнения с прототипом
	bool operator==(const ObjVal &r) const
	{
		return m_values == r.m_values;
	}
	bool operator!=(const ObjVal &r) const
	{
		return m_values != r.m_values;
	}
	// чистка левых параметров (поменяли тип предмета в олц/файле и т.п.)
	// дергается после редактирований в олц, лоада прототипов и просто шмоток
	void remove_incorrect_keys(int type);

private:
	boost::unordered_map<EValueKey, int> m_values;
};

class OBJ_DATA
{
public:
	constexpr static int NUM_PADS = 6;

	constexpr static int DEFAULT_COST = 100;
	constexpr static int DEFAULT_RENT_ON = 100;
	constexpr static int DEFAULT_RENT_OFF = 100;
	constexpr static int UNLIMITED_GLOBAL_MAXIMUM = -1;
	constexpr static int DEFAULT_GLOBAL_MAXIMUM = UNLIMITED_GLOBAL_MAXIMUM;
	constexpr static int DEFAULT_MINIMUM_REMORTS = 0;

	// бесконечный таймер
	constexpr static int UNLIMITED_TIMER = 2147483647;
	constexpr static int ONE_DAY = 24 * 60;
	constexpr static int SEVEN_DAYS = 7 * ONE_DAY;
	constexpr static int DEFAULT_TIMER = SEVEN_DAYS;

	using pnames_t = boost::array<char *, NUM_PADS>;
	using triggers_list_t = std::list<obj_vnum>;

	OBJ_DATA();
	OBJ_DATA(const OBJ_DATA&);
	~OBJ_DATA();

	unsigned int uid;
	obj_vnum item_number;	// Where in data-base            //
	room_rnum in_room;	// In what room -1 when conta/carr //

	obj_flag_data obj_flags;		// Object information       //
	std::array<obj_affected_type, MAX_OBJ_AFFECT> affected;	// affects //

	char *aliases;		// Title of object :get etc.        //
	char *description;	// When in room                     //
	//boost::shared_ptr<char> short_description;
	char *short_description;	// when worn/carry/in cont.         //
	char *action_description;	// What to write when used          //
	EXTRA_DESCR_DATA *ex_description;	// extra descriptions     //
	CHAR_DATA *carried_by;	// Carried by :NULL in room/conta   //
	CHAR_DATA *worn_by;	// Worn by?              //

	struct custom_label *custom_label;		// наносимая чаром метка //

	short int worn_on;		// Worn where?          //

	OBJ_DATA *in_obj;	// In what object NULL when none    //
	OBJ_DATA *contains;	// Contains objects                 //

	long id;			// used by DG triggers              //
	triggers_list_t proto_script;	// list of default triggers  //
	struct script_data *script;	// script info for the object       //

	OBJ_DATA *next_content;	// For 'contains' lists             //
	OBJ_DATA *next;		// For the object list              //
	int room_was_in;
	pnames_t PNames;
	int max_in_world;	///< Maximum in the world

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
	void dec_timer(int time = 1, bool ingore_utimer = false);

	static id_to_set_info_map set_table;
	static void init_set_table();

	void purge(bool destructor = false);
	bool purged() const;

	//разные системы расчета привлекательности предмета
	unsigned get_ilevel() const;
	void set_ilevel(unsigned ilvl);
	int get_manual_mort_req() const;
	void set_manual_mort_req(int);
	int get_mort_req() const;
	float show_mort_req();
	float show_koef_obj();

	int get_cost() const;
	void set_cost(int x);
	int get_rent() const;
	void set_rent(int x);
	int get_rent_eq() const;
	void set_rent_eq(int x);

	void set_activator(bool flag, int num);
	std::pair<bool, int> get_activator() const;

	// wrappers to access to timed_spell
	const TimedSpell& timed_spell() const { return m_timed_spell; }
	std::string diag_ts_to_char(CHAR_DATA* character) { return m_timed_spell.diag_to_char(character); }
	bool has_timed_spell() const { return !m_timed_spell.empty(); }
	void add_timed_spell(const int spell, const int time);
	void del_timed_spell(const int spell, const bool message);

	void set_extraflag(const EExtraFlag packed_flag) { obj_flags.extra_flags.set(packed_flag); }
	void set_extraflag(const size_t plane, const uint32_t flag) { obj_flags.extra_flags.set_flag(plane, flag); }
	void unset_extraflag(const EExtraFlag packed_flag) { obj_flags.extra_flags.unset(packed_flag); }
	bool get_extraflag(const EExtraFlag packed_flag) const { return obj_flags.extra_flags.get(packed_flag); }
	bool get_extraflag(const size_t plane, const uint32_t flag) const { return obj_flags.extra_flags.get_flag(plane, flag); }

private:
	void zero_init();

	TimedSpell m_timed_spell;    ///< временный обкаст
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

inline bool CAN_WEAR(const OBJ_DATA *obj, const EWearFlag part)
{
	return IS_SET(obj->obj_flags.wear_flags, to_underlying(part));
}

inline bool CAN_WEAR_ANY(const OBJ_DATA* obj)
{
	return obj->obj_flags.wear_flags > 0
		&& obj->obj_flags.wear_flags != to_underlying(EWearFlag::ITEM_WEAR_TAKE);
}

inline bool OBJ_FLAGGED(const OBJ_DATA* obj, const EExtraFlag flag)
{
	return obj->get_extraflag(flag);
}

inline void SET_OBJ_AFF(OBJ_DATA* obj, const uint32_t packed_flag)
{
	return obj->obj_flags.affects.set(packed_flag);
}

inline bool OBJ_AFFECT(const OBJ_DATA* obj, const uint32_t weapon_affect)
{
	return obj->obj_flags.affects.get(weapon_affect);
}

inline bool OBJ_AFFECT(const OBJ_DATA* obj, const EWeaponAffectFlag weapon_affect)
{
	return OBJ_AFFECT(obj, static_cast<uint32_t>(weapon_affect));
}

namespace ObjSystem
{

float count_affect_weight(OBJ_DATA *obj, int num, int mod);
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
OBJ_DATA* create_purse(CHAR_DATA *ch, int gold);
bool is_purse(OBJ_DATA *obj);
void process_open_purse(CHAR_DATA *ch, OBJ_DATA *obj);

} // namespace system_obj

#endif // OBJ_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
