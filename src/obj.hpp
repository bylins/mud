// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_HPP_INCLUDED
#define OBJ_HPP_INCLUDED

#include "obj_enchant.hpp"
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

	using wear_flags_t = std::underlying_type<EWearFlag>::type;

	value_t value;
	EObjectType type_flag;		///< Type of item               //
	wear_flags_t wear_flags;		// Where you can wear it     //
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
struct custom_label
{
public:
	custom_label(): label_text(nullptr), clan(nullptr), author(-2), author_mail(nullptr) {}
	~custom_label();

	char *label_text; // текст
	char *clan;       // аббревиатура клана, если метка предназначена для клана
	int author;       // кем нанесена: содержит результат ch->get_idnum(), по умолчанию -2
	char *author_mail;// будем проверять по емейлу тоже
};

inline custom_label::~custom_label()
{
	if (nullptr != label_text)
	{
		free(label_text);
	}

	if (nullptr != clan)
	{
		free(clan);
	}

	if (nullptr != author_mail)
	{
		free(author_mail);
	}
}

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

	using values_t = boost::unordered_map<EValueKey, int>;
	using const_iterator = values_t::const_iterator;
	const_iterator begin() const { return m_values.begin(); }
	const_iterator end() const { return m_values.end(); }
	bool empty() const { return m_values.empty(); }

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
	values_t m_values;
};

struct SCRIPT_DATA;	// to avoid inclusion of "dg_scripts.h"

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

	using pnames_t = boost::array<std::string, NUM_PADS>;
	using triggers_list_t = std::list<obj_vnum>;
	using affected_t = std::array<obj_affected_type, MAX_OBJ_AFFECT>;

	OBJ_DATA();
	OBJ_DATA(const OBJ_DATA&);
	~OBJ_DATA();

	const std::string activate_obj(const activation& __act);
	const std::string deactivate_obj(const activation& __act);

	void set_skill(int skill_num, int percent);
	int get_skill(int skill_num) const;
	auto& get_skills() const { return m_skills; }

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

	auto get_carried_by() const { return m_carried_by; }
	auto get_contains() const { return m_contains; }
	auto get_craft_timer() const { return obj_flags.craft_timer; }
	auto get_crafter_uid() const { return obj_flags.Obj_maker; }
	auto get_current() const { return obj_flags.Obj_cur; }
	auto get_destroyer() const { return obj_flags.Obj_destroyer; }
	auto get_id() const { return m_id; }
	auto get_in_obj() const { return m_in_obj; }
	auto get_in_room() const { return m_in_room; }
	auto get_is_rename() const { return obj_flags.Obj_is_rename; }
	auto get_level() const { return obj_flags.Obj_level; }
	auto get_material() const { return obj_flags.Obj_mater; }
	auto get_max_in_world() const { return m_max_in_world; }
	auto get_maximum() const { return obj_flags.Obj_max; }
	auto get_next() const { return m_next; }
	auto get_next_content() const { return m_next_content; }
	auto get_owner() const { return obj_flags.Obj_owner; }
	auto get_parent() const { return obj_flags.Obj_parent; }
	auto get_rnum() const { return m_item_number; }
	auto get_room_was_in() const { return m_room_was_in; }
	auto get_sex() const { return obj_flags.Obj_sex; }
	auto get_skill() const { return obj_flags.Obj_skill; }
	auto get_spell() const { return obj_flags.Obj_spell; }
	auto get_type() const { return obj_flags.type_flag; }
	auto get_uid() const { return m_uid; }
	auto get_val(size_t index) const { return obj_flags.value[index]; }
	auto get_value(const ObjVal::EValueKey key) const { return m_values.get(key); }
	auto get_wear_flags() const { return obj_flags.wear_flags; }
	auto get_weight() const { return obj_flags.weight; }
	auto get_worn_by() const { return m_worn_by; }
	auto get_worn_on() const { return m_worn_on; }
	auto get_zone() const { return obj_flags.Obj_zone; }
	auto dec_val(size_t index) { return --obj_flags.value[index]; }
	bool can_wear_any() const { return obj_flags.wear_flags > 0 && obj_flags.wear_flags != to_underlying(EWearFlag::ITEM_WEAR_TAKE); }
	bool get_affect(const EWeaponAffectFlag weapon_affect) const { return obj_flags.affects.get(weapon_affect); }
	bool get_affect(const uint32_t weapon_affect) const { return obj_flags.affects.get(weapon_affect); }
	bool get_anti_flag(const EAntiFlag flag) const { return obj_flags.anti_flag.get(flag); }
	bool get_extra_flag(const EExtraFlag packed_flag) const { return obj_flags.extra_flags.get(packed_flag); }
	bool get_extra_flag(const size_t plane, const uint32_t flag) const { return obj_flags.extra_flags.get_flag(plane, flag); }
	bool get_no_flag(const ENoFlag flag) const { return obj_flags.no_flag.get(flag); }
	bool get_wear_flag(const EWearFlag part) const { return IS_SET(obj_flags.wear_flags, to_underlying(part)); }
	bool get_wear_mask(const obj_flag_data::wear_flags_t part) const { return IS_SET(obj_flags.wear_flags, part); }
	const auto& get_action_description() const { return m_action_description; }
	const auto& get_affect_flags() const { return obj_flags.affects; }
	const auto& get_affected() const { return m_affected; }
	const auto& get_affected(const size_t index) const { return m_affected[index]; }
	const auto& get_aliases() const { return m_aliases; }
	const auto& get_anti_flags() const { return obj_flags.anti_flag; }
	const auto& get_custom_label() const { return m_custom_label; }
	const auto& get_description() const { return m_description; }
	const auto& get_enchants() const { return m_enchants; }
	const auto& get_ex_description() const { return m_ex_description; }
	const auto& get_extra_flags() const { return obj_flags.extra_flags; }
	const auto& get_no_flags() const { return obj_flags.no_flag; }
	const auto& get_proto_script() const { return m_proto_script; }
	const auto& get_script() const { return m_script; }
	const auto& get_values() const { return m_values; }
	const std::string& get_PName(const size_t index) const { return m_pnames[index]; }
	const std::string& get_short_description() const { return m_short_description; }
	void add_proto_script(const obj_vnum vnum) { m_proto_script.push_back(vnum); }
	void clear_action_description() { m_action_description.clear(); }
	void clear_proto_script() { m_proto_script.clear(); }
	void init_values_from_zone(const char* str) { m_values.init_from_zone(str); }
	void load_affects(const char* string) { obj_flags.affects.from_string(string); }
	void load_antiflags(const char* string) { obj_flags.anti_flag.from_string(string); }
	void load_extraflags(const char* string) { obj_flags.extra_flags.from_string(string); }
	void load_noflags(const char* string) { obj_flags.no_flag.from_string(string); }
	void remove_custom_label() { m_custom_label.reset(); }
	void remove_incorrect_values_keys(const int type) { m_values.remove_incorrect_keys(type); }
	void remove_set_bonus() { m_enchants.remove_set_bonus(this); }
	void set_action_description(const std::string& _) { m_action_description = _; }
	void set_affect_flags(const FLAG_DATA& flags) { obj_flags.affects = flags; }
	void set_affected(const size_t index, const EApplyLocation location, const int modifier);
	void set_affected(const size_t index, const obj_affected_type& affect) { m_affected[index] = affect; }
	void set_aliases(const std::string& _) { m_aliases = _; }
	void set_anti_flags(const FLAG_DATA& flags) { obj_flags.anti_flag = flags; }
	void set_carried_by(CHAR_DATA* _) { m_carried_by = _; }
	void set_contains(OBJ_DATA* _) { m_contains = _; }
	void set_crafter_uid(const int _) { obj_flags.Obj_maker = _; }
	void set_current(const int _) { obj_flags.Obj_cur = _; }
	void set_custom_label(const std::shared_ptr<custom_label>& _) { m_custom_label = _; }
	void set_description(const std::string& _) { m_description = _; }
	void set_destroyer(const int _) { obj_flags.Obj_destroyer = _; }
	void set_ex_description(const char* keyword, const char* description);
	void set_ex_description(EXTRA_DESCR_DATA* _) { m_ex_description.reset(_); }
	void set_ex_description(const std::shared_ptr<EXTRA_DESCR_DATA>& _) { m_ex_description = _; }
	void set_extra_flags(const FLAG_DATA& flags) { obj_flags.extra_flags = flags; }
	void set_extraflag(const EExtraFlag packed_flag) { obj_flags.extra_flags.set(packed_flag); }
	void set_extraflag(const size_t plane, const uint32_t flag) { obj_flags.extra_flags.set_flag(plane, flag); }
	void set_id(const long _) { m_id = _; }
	void set_in_obj(OBJ_DATA* _) { m_in_obj = _; }
	void set_in_room(const room_rnum _) { m_in_room = _; }
	void set_is_rename(const bool _) { obj_flags.Obj_is_rename = _; }
	void set_level(const int _) { obj_flags.Obj_level = _; }
	void set_material(const obj_flag_data::EObjectMaterial _) { obj_flags.Obj_mater = _; }
	void set_max_in_world(const int _) { m_max_in_world = _; }
	void set_maximum(const int _) { obj_flags.Obj_max = _; }
	void set_next(OBJ_DATA* _) { m_next = _; }
	void set_next_content(OBJ_DATA* _) { m_next_content = _; }
	void set_no_flags(const FLAG_DATA& flags) { obj_flags.no_flag = flags; }
	void set_obj_aff(const uint32_t packed_flag) { obj_flags.affects.set(packed_flag); }
	void set_owner(const int _) { obj_flags.Obj_owner = _; }
	void set_parent(const int _) { obj_flags.Obj_parent = _; }
	void set_PName(const size_t index, const char* _) { m_pnames[index] = _; }
	void set_PName(const size_t index, const std::string& _) { m_pnames[index] = _; }
	void set_PNames(const pnames_t& _) { m_pnames = _; }
	void set_proto_script(const triggers_list_t& _) { m_proto_script = _; }
	void set_rnum(const obj_vnum _) { m_item_number = _; }
	void set_room_was_in(const int _) { m_room_was_in = _; }
	void set_script(const std::shared_ptr<SCRIPT_DATA>& _) { m_script = _; }
	void set_script(SCRIPT_DATA* _);
	void set_sex(const ESex _) { obj_flags.Obj_sex = _; }
	void set_short_description(const char* _) { m_short_description = _; }
	void set_short_description(const std::string& _) { m_short_description = _; }
	void set_skill(const int _) { obj_flags.Obj_skill = _; }
	void set_spell(const int _) { obj_flags.Obj_spell = _; }
	void set_type(const obj_flag_data::EObjectType _) { obj_flags.type_flag = _; }
	void set_uid(const unsigned _) { m_uid = _; }
	void set_val(size_t index, int value) { obj_flags.value[index] = value; }
	void set_value(const ObjVal::EValueKey key, const int value) { return m_values.set(key, value); }
	void set_values(const ObjVal&  _) { m_values = _; }
	void set_wear_flag(const EWearFlag flag) { SET_BIT(obj_flags.wear_flags, flag); }
	void set_wear_flags(const obj_flag_data::wear_flags_t _) { obj_flags.wear_flags = _; }
	void set_weight(const int _) { obj_flags.weight = _; }
	void set_worn_by(CHAR_DATA* _) { m_worn_by = _; }
	void set_worn_on(const short _) { m_worn_on = _; }
	void set_zone(const int _) { obj_flags.Obj_zone = _; }
	void swap_proto_script(triggers_list_t& _) { m_proto_script.swap(_); }
	void unset_extraflag(const EExtraFlag packed_flag) { obj_flags.extra_flags.unset(packed_flag); }
	void add_weight(const int _) { obj_flags.weight += _; }
	void sub_weight(const int _) { obj_flags.weight += _; }
	const auto& get_all_affected() const { return m_affected; }
	void set_all_affected(const affected_t& _) { m_affected = _; }
	void set_craft_timer(const int _) { obj_flags.craft_timer = _; }
	void append_ex_description_tag(const char* tag) { m_ex_description->description = str_add(m_ex_description->description, tag); }
	void gm_extra_flag(const char *subfield, const char **list, char *res) { obj_flags.extra_flags.gm_flag(subfield, list, res); }
	void gm_affect_flag(const char *subfield, const char **list, char *res) { obj_flags.extra_flags.gm_flag(subfield, list, res); }
	void sub_current(const int _) { obj_flags.Obj_cur -= _; }
	void remove_me_from_objects_list(OBJ_DATA*& head) { REMOVE_FROM_LIST(this, head, [](auto list) -> auto& { return list->m_next; }); }
	void remove_me_from_contains_list(OBJ_DATA*& head) { REMOVE_FROM_LIST(this, head, [](auto list) -> auto& { return list->m_next_content; }); }
	void dec_weight() { --obj_flags.weight; }
	void sub_val(const size_t index, const int amount) { obj_flags.value[index] -= amount; }
	void inc_val(const size_t index) { ++obj_flags.value[index]; }
	void add_val(const size_t index, const int amount) { obj_flags.value[index] += amount; }
	void add_maximum(const int amount) { obj_flags.Obj_max += amount; }
	void clear_all_affected();
	void clear_affected(const size_t index) { m_affected[index].location = APPLY_NONE; }
	void dec_destroyer() { --obj_flags.Obj_destroyer; }
	void dec_affected_value(const size_t index) { --m_affected[index].modifier; }
	const auto& get_all_values() const { return m_values; }
	void clear_ex_description() { m_ex_description.reset(); }
	void add_ex_description(const char* keyword, const char* description);
	void add_affected(const size_t index, const int amount) { m_affected[index].modifier += amount; }
	void add_no_flags(const FLAG_DATA& flags) { obj_flags.no_flag += flags; }
	void add_extra_flags(const FLAG_DATA& flags) { obj_flags.extra_flags += flags; }
	void add_anti_flags(const FLAG_DATA& flags) { obj_flags.anti_flag += flags; }
	void add_affect_flags(const FLAG_DATA& flags) { obj_flags.affects += flags; }
	void add_enchant(const obj::enchant& _) { m_enchants.add(_); }

private:
	void zero_init();
	unsigned int m_uid;
	obj_vnum m_item_number;	// Where in data-base            //
	room_rnum m_in_room;	// In what room -1 when conta/carr //

	obj_flag_data obj_flags;		// Object information       //
	affected_t m_affected;	// affects //

	std::string m_aliases;		// Title of object :get etc.        //
	std::string m_description;	// When in room                     //

	std::string m_short_description;	// when worn/carry/in cont.         //
	std::string m_action_description;	// What to write when used          //
	std::shared_ptr<EXTRA_DESCR_DATA> m_ex_description;	// extra descriptions     //
	CHAR_DATA *m_carried_by;	// Carried by :NULL in room/conta   //
	CHAR_DATA *m_worn_by;	// Worn by?              //

	std::shared_ptr<custom_label> m_custom_label;		// наносимая чаром метка //

	short int m_worn_on;		// Worn where?          //

	OBJ_DATA *m_in_obj;	// In what object NULL when none    //
	OBJ_DATA *m_contains;	// Contains objects                 //

	long m_id;			// used by DG triggers              //
	triggers_list_t m_proto_script;	// list of default triggers  //
	std::shared_ptr<SCRIPT_DATA> m_script;	// script info for the object       //

	OBJ_DATA *m_next_content;	// For 'contains' lists             //
	OBJ_DATA *m_next;		// For the object list              //
	int m_room_was_in;
	pnames_t m_pnames;
	int m_max_in_world;	///< Maximum in the world

	obj::Enchants m_enchants;
	ObjVal m_values;

	TimedSpell m_timed_spell;    ///< временный обкаст
	// если этот массив создался, то до выхода из программы уже не удалится. тут это вроде как "нормально"
	std::map<int, int> m_skills;
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

inline void OBJ_DATA::set_affected(const size_t index, const EApplyLocation location, const int modifier)
{
	m_affected[index].location = location;
	m_affected[index].modifier = modifier;
}

inline bool CAN_WEAR(const OBJ_DATA *obj, const EWearFlag part) { return obj->get_wear_flag(part); }
inline bool CAN_WEAR_ANY(const OBJ_DATA* obj) { return obj->can_wear_any(); }
inline bool OBJ_FLAGGED(const OBJ_DATA* obj, const EExtraFlag flag) { return obj->get_extra_flag(flag); }
inline void SET_OBJ_AFF(OBJ_DATA* obj, const uint32_t packed_flag) { return obj->set_obj_aff(packed_flag); }
inline bool OBJ_AFFECT(const OBJ_DATA* obj, const uint32_t weapon_affect) { return obj->get_affect(weapon_affect); }

inline bool OBJ_AFFECT(const OBJ_DATA* obj, const EWeaponAffectFlag weapon_affect)
{
	return OBJ_AFFECT(obj, static_cast<uint32_t>(weapon_affect));
}

class CActionDescriptionWriter : public CCommonStringWriter
{
public:
	CActionDescriptionWriter(OBJ_DATA& object) : m_object(object) {}

	virtual const char* get_string() const { return m_object.get_action_description().c_str(); }
	virtual void set_string(const char* data) { m_object.set_action_description(data); }
	virtual void append_string(const char* data) { m_object.set_action_description(m_object.get_action_description() + data); }
	virtual size_t length() const { return m_object.get_action_description().length(); }
	virtual void clear() { m_object.clear_action_description(); }

private:
	OBJ_DATA& m_object;
};

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
