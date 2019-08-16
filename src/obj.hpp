// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_HPP_INCLUDED
#define OBJ_HPP_INCLUDED

#include "id.hpp"
#include "obj_enchant.hpp"
#include "item.creation.hpp"
#include "spells.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <array>
#include <vector>
#include <string>
#include <map>

std::string print_obj_affects(const obj_affected_type &affect);
void print_obj_affects(CHAR_DATA *ch, const obj_affected_type &affect);

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

	class EValueKeyHash
	{
	public:
		std::size_t operator()(const EValueKey value) const { return static_cast<std::size_t>(value); }
	};

	using values_t = std::unordered_map<EValueKey, int, EValueKeyHash>;
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

class CObjectPrototype;

class VNumChangeObserver
{
public:
	using shared_ptr = std::shared_ptr<VNumChangeObserver>;

	virtual ~VNumChangeObserver() {}

	virtual void notify(CObjectPrototype& object, const obj_vnum old_vnum) = 0;
};

class IDChangeObserver
{
public:
	using shared_ptr = std::shared_ptr<IDChangeObserver>;

	virtual ~IDChangeObserver() {}

	virtual void notify(OBJ_DATA& object, const object_id_t old_id) = 0;
};

class ObjectRNum_ChangeObserver
{
public:
	using shared_ptr = std::shared_ptr<ObjectRNum_ChangeObserver>;

	virtual ~ObjectRNum_ChangeObserver() {}

	virtual void notify(CObjectPrototype& object, const obj_rnum old_rnum) = 0;
};

class UIDChangeObserver
{
public:
	using shared_ptr = std::shared_ptr<UIDChangeObserver>;

	virtual ~UIDChangeObserver() {}

	virtual void notify(OBJ_DATA& object, const int old_uid) = 0;
};

class CObjectPrototype
{
public:
	using shared_ptr = std::shared_ptr<CObjectPrototype>;

	enum EObjectType
	{
		ITEM_UNDEFINED = 0,
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
		ITEM_ENCHANT = 32,		// зачарование предмета
		ITEM_MAGIC_MATERIAL = 33,	// Item is a material related to craft system
		ITEM_MAGIC_ARROW = 34,	// Item is a material related to craft system
		ITEM_MAGIC_CONTAINER = 35,	// Item is a material related to craft system
		ITEM_CRAFT_MATERIAL = 36,	// Item is a material related to craft system
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

	constexpr static int DEFAULT_MAXIMUM_DURABILITY = 100;
	constexpr static int DEFAULT_CURRENT_DURABILITY = DEFAULT_MAXIMUM_DURABILITY;
	constexpr static int DEFAULT_LEVEL = 0;
	constexpr static int DEFAULT_WEIGHT = INT_MAX;
	constexpr static EObjectType DEFAULT_TYPE = ITEM_OTHER;
	constexpr static EObjectMaterial DEFAULT_MATERIAL = MAT_NONE;

	constexpr static int NUM_PADS = 6;

	constexpr static int DEFAULT_COST = 100;
	constexpr static int DEFAULT_RENT_ON = 100;
	constexpr static int DEFAULT_RENT_OFF = 100;
	constexpr static int UNLIMITED_GLOBAL_MAXIMUM = -1;
	constexpr static int DEFAULT_MAX_IN_WORLD = UNLIMITED_GLOBAL_MAXIMUM;
	constexpr static int DEFAULT_MINIMUM_REMORTS = 0;

	// бесконечный таймер
	constexpr static int UNLIMITED_TIMER = 2147483647;
	constexpr static int ONE_DAY = 24 * 60;
	constexpr static int SEVEN_DAYS = 7 * ONE_DAY;
	constexpr static int DEFAULT_TIMER = SEVEN_DAYS;

	constexpr static int DEFAULT_DESTROYER = 60;
	constexpr static int DEFAULT_RNUM = NOTHING;
	constexpr static int VALS_COUNT = 4;
	constexpr static int CORPSE_INDICATOR = 1; // container with value 3 set to 1 is a corpse

	using skills_t = std::map<ESkill, int>;
	using vals_t = std::array<int, VALS_COUNT>;
	using wear_flags_t = std::underlying_type<EWearFlag>::type;
	using pnames_t = std::array<std::string, NUM_PADS>;
	using triggers_list_t = std::list<obj_vnum>;
	using triggers_list_ptr = std::shared_ptr<triggers_list_t>;
	using affected_t = std::array<obj_affected_type, MAX_OBJ_AFFECT>;
	
	CObjectPrototype(const obj_vnum vnum) : m_vnum(vnum),
		m_type(DEFAULT_TYPE),
		m_weight(DEFAULT_WEIGHT),
		m_proto_script(new triggers_list_t()),
		m_max_in_world(DEFAULT_MAX_IN_WORLD),
		m_vals({ 0, 0, 0, 0 }),
		m_destroyer(DEFAULT_DESTROYER),
		m_spell(SPELL_NO_SPELL),
		m_level(DEFAULT_LEVEL),
		m_skill(SKILL_INVALID),
		m_maximum_durability(DEFAULT_MAXIMUM_DURABILITY),
		m_current_durability(DEFAULT_CURRENT_DURABILITY),
		m_material(DEFAULT_MATERIAL),
		m_sex(DEFAULT_SEX),
		m_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_UNDEFINED)),
		m_timer(DEFAULT_TIMER),
		m_minimum_remorts(DEFAULT_MINIMUM_REMORTS),  // для хранения количеста мортов. если отричательное тогда до какого морта
		m_cost(DEFAULT_COST),
		m_rent_on(DEFAULT_RENT_ON),
		m_rent_off(DEFAULT_RENT_OFF),
		m_ilevel(0),
		m_rnum(DEFAULT_RNUM)
	{}
	virtual	~CObjectPrototype() {};

	auto& get_skills() const { return m_skills; }
	auto dec_val(size_t index) { return --m_vals[index]; }
	auto get_current_durability() const { return m_current_durability; }
	auto get_destroyer() const { return m_destroyer; }
	auto get_level() const { return m_level; }
	auto get_material() const { return m_material; }
	auto get_max_in_world() const { return m_max_in_world; }
	auto get_maximum_durability() const { return m_maximum_durability; }
	auto get_sex() const { return m_sex; }
	auto get_skill() const { return m_skill; }
	auto get_spell() const { return m_spell; }
	auto get_type() const { return m_type; }
	auto get_val(size_t index) const { return m_vals[index]; }
	auto get_value(const ObjVal::EValueKey key) const { return m_values.get(key); }
	auto get_wear_flags() const { return m_wear_flags; }
	auto get_weight() const { return m_weight; }
	auto serialize_values() const { return m_values.print_to_file(); }
	bool can_wear_any() const { return m_wear_flags > 0 && m_wear_flags != to_underlying(EWearFlag::ITEM_WEAR_TAKE); }
	bool get_affect(const EWeaponAffectFlag weapon_affect) const { return m_waffect_flags.get(weapon_affect); }
	bool get_affect(const uint32_t weapon_affect) const { return m_waffect_flags.get(weapon_affect); }
	bool get_anti_flag(const EAntiFlag flag) const { return m_anti_flags.get(flag); }
	bool get_extra_flag(const EExtraFlag packed_flag) const { return m_extra_flags.get(packed_flag); }
	bool get_extra_flag(const size_t plane, const uint32_t flag) const { return m_extra_flags.get_flag(plane, flag); }
	bool get_no_flag(const ENoFlag flag) const { return m_no_flags.get(flag); }
	bool has_wear_flag(const EWearFlag part) const;
	bool get_wear_mask(const wear_flags_t part) const;
	bool init_values_from_file(const char* str) { return m_values.init_from_file(str); }
	const auto& get_action_description() const { return m_action_description; }
	const auto& get_affect_flags() const { return m_waffect_flags; }
	const auto& get_affected(const size_t index) const { return m_affected[index]; }
	const auto& get_aliases() const { return m_aliases; }
	const auto& get_all_affected() const { return m_affected; }
	const auto& get_all_values() const { return m_values; }
	const auto& get_anti_flags() const { return m_anti_flags; }
	const auto& get_description() const { return m_description; }
	const auto& get_ex_description() const { return m_ex_description; }
	const auto& get_extra_flags() const { return m_extra_flags; }
	const auto& get_no_flags() const { return m_no_flags; }
	const auto& get_proto_script() const { return *m_proto_script; }
	const auto& get_proto_script_ptr() const { return m_proto_script; }
	const std::string& get_PName(const size_t index) const { return m_pnames[index]; }
	const std::string& get_short_description() const { return m_short_description; }
	void add_affect_flags(const FLAG_DATA& flags) { m_waffect_flags += flags; }
	void add_affected(const size_t index, const int amount) { m_affected[index].modifier += amount; }
	void add_anti_flags(const FLAG_DATA& flags) { m_anti_flags += flags; }
	void add_extra_flags(const FLAG_DATA& flags) { m_extra_flags += flags; }
	void add_maximum(const int amount) { m_maximum_durability += amount; }
	void add_no_flags(const FLAG_DATA& flags) { m_no_flags += flags; }
	void add_proto_script(const obj_vnum vnum) { m_proto_script->push_back(vnum); }
	void add_val(const size_t index, const int amount) { m_vals[index] += amount; }
	void add_weight(const int _) { m_weight += _; }
	void clear_action_description() { m_action_description.clear(); }
	void clear_affected(const size_t index) { m_affected[index].location = APPLY_NONE; }
	void clear_all_affected();
	void clear_proto_script();
	void dec_affected_value(const size_t index) { --m_affected[index].modifier; }
	void dec_destroyer() { --m_destroyer; }
	void dec_weight() { --m_weight; }
	void gm_affect_flag(const char *subfield, const char **list, char *res) { m_extra_flags.gm_flag(subfield, list, res); }
	void gm_extra_flag(const char *subfield, const char **list, char *res) { m_extra_flags.gm_flag(subfield, list, res); }
	void inc_val(const size_t index) { ++m_vals[index]; }
	void init_values_from_zone(const char* str) { m_values.init_from_zone(str); }
	void load_affect_flags(const char* string) { m_waffect_flags.from_string(string); }
	void load_anti_flags(const char* string) { m_anti_flags.from_string(string); }
	void load_extra_flags(const char* string) { m_extra_flags.from_string(string); }
	void load_no_flags(const char* string) { m_no_flags.from_string(string); }
	void remove_incorrect_values_keys(const int type) { m_values.remove_incorrect_keys(type); }
	void set_action_description(const std::string& _) { m_action_description = _; }
	void set_affect_flag(const EWeaponAffectFlag packed_flag) { m_waffect_flags.set(packed_flag); }
	void set_affect_flags(const FLAG_DATA& flags) { m_waffect_flags = flags; }
	void set_affected(const size_t index, const EApplyLocation location, const int modifier);
	void set_affected(const size_t index, const obj_affected_type& affect) { m_affected[index] = affect; }
	void set_affected_location(const size_t index, const EApplyLocation _) { m_affected[index].location = _; }
	void set_affected_modifier(const size_t index, const int _) { m_affected[index].modifier = _; }
	void set_aliases(const std::string& _) { m_aliases = _; }
	void set_all_affected(const affected_t& _) { m_affected = _; }
	void set_anti_flag(const EAntiFlag packed_flag) { m_anti_flags.set(packed_flag); }
	void set_anti_flags(const FLAG_DATA& flags) { m_anti_flags = flags; }
	void set_current_durability(const int _) { m_current_durability = _; }
	void set_description(const std::string& _) { m_description = _; }
	void set_destroyer(const int _) { m_destroyer = _; }
	void set_ex_description(const EXTRA_DESCR_DATA::shared_ptr& _) { m_ex_description = _; }
	void set_ex_description(EXTRA_DESCR_DATA* _) { m_ex_description.reset(_); }
	void set_extra_flag(const EExtraFlag packed_flag) { m_extra_flags.set(packed_flag); }
	void set_extra_flag(const size_t plane, const uint32_t flag) { m_extra_flags.set_flag(plane, flag); }
	void set_extra_flags(const FLAG_DATA& flags) { m_extra_flags = flags; }
	void set_level(const int _) { m_level = _; }
	void set_material(const EObjectMaterial _) { m_material = _; }
	void set_max_in_world(const int _) { m_max_in_world = _; }
	void set_maximum_durability(const int _) { m_maximum_durability = _; }
	void set_no_flag(const ENoFlag flag) { m_no_flags.set(flag); }
	void set_no_flags(const FLAG_DATA& flags) { m_no_flags = flags; }
	void set_obj_aff(const uint32_t packed_flag) { m_waffect_flags.set(packed_flag); }
	void set_PName(const size_t index, const char* _) { m_pnames[index] = _; }
	void set_PName(const size_t index, const std::string& _) { m_pnames[index] = _; }
	void set_PNames(const pnames_t& _) { m_pnames = _; }
	void set_proto_script(const triggers_list_t& _) { *m_proto_script = _; }
	void set_short_description(const char* _) { m_short_description = _; }
	void set_short_description(const std::string& _) { m_short_description = _; }
	void set_skill(const int _) { m_skill = _; }
	void set_spell(const int _) { m_spell = static_cast<ESpell>(_); }
	void set_type(const EObjectType _) { m_type = _; }
	void set_sex(const ESex _) { m_sex = _; }
	void set_value(const ObjVal::EValueKey key, const int value) { return m_values.set(key, value); }
	void set_values(const ObjVal&  _) { m_values = _; }
	void set_wear_flag(const EWearFlag flag);
	void set_wear_flags(const wear_flags_t _) { m_wear_flags = _; }
	void set_weight(const int _) { m_weight = _; }
	void set_val(size_t index, int value) { m_vals[index] = value; }
	void sub_current(const int _) { m_current_durability -= _; }
	void sub_val(const size_t index, const int amount) { m_vals[index] -= amount; }
	void sub_weight(const int _) { m_weight -= _; }
	void swap_proto_script(triggers_list_t& _) { m_proto_script->swap(_); }
	void toggle_affect_flag(const size_t plane, const uint32_t flag) { m_waffect_flags.toggle_flag(plane, flag); }
	void toggle_anti_flag(const size_t plane, const uint32_t flag) { m_anti_flags.toggle_flag(plane, flag); }
	void toggle_extra_flag(const size_t plane, const uint32_t flag) { m_extra_flags.toggle_flag(plane, flag); }
	void toggle_no_flag(const size_t plane, const uint32_t flag) { m_no_flags.toggle_flag(plane, flag); }
	void toggle_skill(const uint32_t skill);
	void toggle_val_bit(const size_t index, const uint32_t bit);
	void toggle_wear_flag(const uint32_t flag);
	void unset_extraflag(const EExtraFlag packed_flag) { m_extra_flags.unset(packed_flag); }
	void set_skill(int skill_num, int percent);
	int get_skill(int skill_num) const;
	void get_skills(skills_t& out_skills) const;
	bool has_skills() const;
	void set_timer(int timer);
	int get_timer() const;
	void set_skills(const skills_t& _) { m_skills = _; }
	auto get_minimum_remorts() const { return m_minimum_remorts; }
	auto get_cost() const { return m_cost; }
	void set_cost(int x);
	auto get_rent_off() const { return m_rent_off; }
	void set_rent_off(int x);
	auto get_rent_on() const { return m_rent_on; }
	void set_rent_on(int x);
	void set_ex_description(const char* keyword, const char* description);
	void set_minimum_remorts(const int _) { m_minimum_remorts = _; }
	int get_auto_mort_req() const;
	float show_mort_req();
	float show_koef_obj();
	float get_ilevel() const;	///< разные системы расчета привлекательности предмета
	void set_ilevel(float ilvl);
	auto get_rnum() const { return m_rnum; }
	void set_rnum(const obj_rnum _);
	auto get_vnum() const { return m_vnum; }

	void subscribe_for_vnum_changes(const VNumChangeObserver::shared_ptr& observer) { m_vnum_change_observers.insert(observer); }
	void unsubscribe_from_vnum_changes(const VNumChangeObserver::shared_ptr& observer) { m_vnum_change_observers.erase(observer); }

	void subscribe_for_rnum_changes(const ObjectRNum_ChangeObserver::shared_ptr& observer) { m_rnum_change_observers.insert(observer); }
	void unsubscribe_from_rnum_changes(const ObjectRNum_ChangeObserver::shared_ptr& observer) { m_rnum_change_observers.erase(observer); }

	std::string item_count_message(int num, int pad);

protected:
	void zero_init();
	CObjectPrototype& operator=(const CObjectPrototype& from);	///< makes shallow copy of all fields except VNUM
	void set_vnum(const obj_vnum vnum);		///< allow inherited classes change VNUM (to make possible objects transformations)
	void tag_ex_description(const char* tag);

private:
	obj_vnum m_vnum;

	EObjectType m_type;
	int m_weight;

	affected_t m_affected;	// affects //

	std::string m_aliases;		// Title of object :get etc.        //
	std::string m_description;	// When in room                     //

	std::string m_short_description;	// when worn/carry/in cont.         //
	std::string m_action_description;	// What to write when used          //
	EXTRA_DESCR_DATA::shared_ptr m_ex_description;	// extra descriptions     //

	triggers_list_ptr m_proto_script;	// list of default triggers  //

	pnames_t m_pnames;
	int m_max_in_world;	///< Maximum in the world

	vals_t m_vals;		// old values
	ObjVal m_values;	// new values

	int m_destroyer;
	ESpell m_spell;
	int m_level;
	int m_skill;
	int m_maximum_durability;
	int m_current_durability;

	EObjectMaterial m_material;
	ESex m_sex;

	FLAG_DATA m_extra_flags;	// If it hums, glows, etc.      //
	FLAG_DATA m_waffect_flags;
	FLAG_DATA m_anti_flags;
	FLAG_DATA m_no_flags;

	wear_flags_t m_wear_flags;		// Where you can wear it     //

	int m_timer;	///< таймер (в минутах рл)

	skills_t m_skills;	///< если этот массив создался, то до выхода из программы уже не удалится. тут это вроде как "нормально"

	int m_minimum_remorts;	///< если > 0 - требование по минимальным мортам, проставленное в олц

	int m_cost;	///< цена шмотки при продаже
	int m_rent_on;	///< стоимость ренты, если надета
	int m_rent_off;	///< стоимость ренты, если в инве

	float m_ilevel;	///< расчетный уровень шмотки, не сохраняется
	obj_vnum m_rnum;	///< Where in data-base

	std::unordered_set<VNumChangeObserver::shared_ptr> m_vnum_change_observers;
	std::unordered_set<ObjectRNum_ChangeObserver::shared_ptr> m_rnum_change_observers;
};

inline auto GET_OBJ_VAL(const CObjectPrototype* obj, size_t index)
{
        if (nullptr == obj)
        {
                return 0;
        }

        return obj->get_val(index);
}
inline auto GET_OBJ_VAL(const CObjectPrototype::shared_ptr& obj, size_t index) { return GET_OBJ_VAL(obj.get(), index); }

class activation
{
	std::string actmsg, deactmsg, room_actmsg, room_deactmsg;
	FLAG_DATA affects;
	std::array<obj_affected_type, MAX_OBJ_AFFECT> affected;
	int weight, ndices, nsides;
	CObjectPrototype::skills_t skills;

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
	void get_skills(CObjectPrototype::skills_t& __skills) const
	{
		__skills.insert(skills.begin(), skills.end());
	}

	activation& set_skill(const ESkill __skillnum, const int __percent)
	{
		CObjectPrototype::skills_t::iterator skill = skills.find(__skillnum);
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

class SCRIPT_DATA;	// to avoid inclusion of "dg_scripts.h"

class OBJ_DATA : public CObjectPrototype
{
public:
	using shared_ptr = std::shared_ptr<OBJ_DATA>;

	constexpr static const int DEFAULT_MAKER = 0;
	constexpr static const int DEFAULT_OWNER = 0;
	constexpr static const int DEFAULT_PARENT = 0;

	OBJ_DATA(const obj_vnum vnum);
	OBJ_DATA(const OBJ_DATA&);
	OBJ_DATA(const CObjectPrototype&);
	~OBJ_DATA();

	const std::string activate_obj(const activation& __act);
	const std::string deactivate_obj(const activation& __act);

	int get_serial_num();
	void set_serial_num(int num);

	void dec_timer(int time = 1, bool ingore_utimer = false, bool exchange = false);

	static id_to_set_info_map set_table;
	static void init_set_table();

	void purge();

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
	auto get_craft_timer() const { return m_craft_timer; }
	auto get_crafter_uid() const { return m_maker; }
	auto get_id() const { return m_id; }
	auto get_in_obj() const { return m_in_obj; }
	auto get_in_room() const { return m_in_room; }
	auto get_is_rename() const { return m_is_rename; }
	auto get_next() const { return m_next; }
	auto get_next_content() const { return m_next_content; }
	auto get_owner() const { return m_owner; }
	auto get_parent() const { return m_parent; }
	auto get_room_was_in() const { return m_room_was_in; }
	auto get_uid() const { return m_uid; }
	auto get_worn_by() const { return m_worn_by; }
	auto get_worn_on() const { return m_worn_on; }
	auto get_zone() const { return m_zone; }
	auto serialize_enchants() const { return m_enchants.print_to_file(); }
	const auto& get_enchants() const { return m_enchants; }
	const auto& get_custom_label() const { return m_custom_label; }
	const auto& get_script() const { return m_script; }
	void add_enchant(const obj::enchant& _) { m_enchants.add(_); }
	void remove_custom_label() { m_custom_label.reset(); }
	void remove_me_from_contains_list(OBJ_DATA*& head);
	void remove_me_from_objects_list(OBJ_DATA*& head);
	void remove_set_bonus() { m_enchants.remove_set_bonus(this); }
	void set_carried_by(CHAR_DATA* _) { m_carried_by = _; }
	void set_contains(OBJ_DATA* _) { m_contains = _; }
	void set_craft_timer(const int _) { m_craft_timer = _; }
	void set_crafter_uid(const int _) { m_maker = _; }
	void set_custom_label(const std::shared_ptr<custom_label>& _) { m_custom_label = _; }
	void set_custom_label(custom_label* _) { m_custom_label.reset(_); }
	void set_id(const long _);
	void set_in_obj(OBJ_DATA* _) { m_in_obj = _; }
	void set_in_room(const room_rnum _) { m_in_room = _; }
	void set_is_rename(const bool _) { m_is_rename = _; }
	void set_next(OBJ_DATA* _) { m_next = _; }
	void set_next_content(OBJ_DATA* _) { m_next_content = _; }
	void set_owner(const int _) { m_owner = _; }
	void set_parent(const int _) { m_parent = _; }
	void set_room_was_in(const int _) { m_room_was_in = _; }
	void set_script(const std::shared_ptr<SCRIPT_DATA>& _) { m_script = _; }
	void set_script(SCRIPT_DATA* _);
	void cleanup_script();
	void set_uid(const unsigned _);
	void set_worn_by(CHAR_DATA* _) { m_worn_by = _; }
	void set_worn_on(const short _) { m_worn_on = _; }
	void set_zone(const int _) { m_zone = _; }
	void update_enchants_set_bonus(const obj_sets::ench_type& _) { m_enchants.update_set_bonus(this, _); }
	void set_enchant(int skill);
	void set_enchant(int skill, OBJ_DATA *obj);
	void unset_enchant();

	void copy_name_from(const CObjectPrototype* src);
	
	bool clone_olc_object_from_prototype(const obj_vnum vnum);
	void copy_from(const CObjectPrototype* src);
	
	void swap(OBJ_DATA& object);
	void set_tag(const char* tag);

	void subscribe_for_id_change(const IDChangeObserver::shared_ptr& observer) { m_id_change_observers.insert(observer); }
	void unsubscribe_from_id_change(const IDChangeObserver::shared_ptr& observer) { m_id_change_observers.erase(observer); }

	void subscribe_for_uid_change(const UIDChangeObserver::shared_ptr& observer) { m_uid_change_observers.insert(observer); }
	void unsubscribe_from_uid_change(const UIDChangeObserver::shared_ptr& observer) { m_uid_change_observers.erase(observer); }

	void attach_triggers(const triggers_list_t& trigs);

	// Полель крафт           //
	void set_create_type(AbstractCreateObjectType *CreateType);
	void set_craft(CHAR_DATA* ch);
	
private:
	void zero_init();

	void detach_ex_description();

	unsigned int m_uid;
	room_rnum m_in_room;	// In what room -1 when conta/carr //
	int m_room_was_in;

	int m_maker;
	int m_owner;
	int m_zone;
	int m_parent;		// Vnum for object parent //
	bool m_is_rename;

	CHAR_DATA *m_carried_by;	// Carried by :NULL in room/conta   //
	CHAR_DATA *m_worn_by;	// Worn by?              //
	short int m_worn_on;		// Worn where?          //

	std::shared_ptr<custom_label> m_custom_label;		// наносимая чаром метка //

	OBJ_DATA *m_in_obj;	// In what object NULL when none    //
	OBJ_DATA *m_contains;	// Contains objects                 //
	OBJ_DATA *m_next_content;	// For 'contains' lists             //
	OBJ_DATA *m_next;		// For the object list              //

	obj::Enchants m_enchants;

	AbstractCreateObjectType *CreateObjectType;	// Полель крафт           //
	int m_craft_timer;

	TimedSpell m_timed_spell;    ///< временный обкаст

	object_id_t m_id;			// used by DG triggers              //
	std::shared_ptr<SCRIPT_DATA> m_script;	// script info for the object       //
	
	// порядковый номер в списке чаров (для name_list)
	int m_serial_number;
	// true - объект спуржен и ждет вызова delete для оболочки
	bool m_purged;
	// для сообщений сетов <активировано или нет, размер активатора>
	std::pair<bool, int> m_activator;

	std::unordered_set<IDChangeObserver::shared_ptr> m_id_change_observers;
	std::unordered_set<UIDChangeObserver::shared_ptr> m_uid_change_observers;
};

template <> const std::string& NAME_BY_ITEM<OBJ_DATA::EObjectType>(const OBJ_DATA::EObjectType item);
template <> OBJ_DATA::EObjectType ITEM_BY_NAME<OBJ_DATA::EObjectType>(const std::string& name);

template <> const std::string& NAME_BY_ITEM<OBJ_DATA::EObjectMaterial>(const OBJ_DATA::EObjectMaterial item);
template <> OBJ_DATA::EObjectMaterial ITEM_BY_NAME<OBJ_DATA::EObjectMaterial>(const std::string& name);

inline void CObjectPrototype::set_affected(const size_t index, const EApplyLocation location, const int modifier)
{
	m_affected[index].location = location;
	m_affected[index].modifier = modifier;
}

inline bool CAN_WEAR(const CObjectPrototype* obj, const EWearFlag part) { return obj->has_wear_flag(part); }
inline bool CAN_WEAR_ANY(const CObjectPrototype* obj) { return obj->can_wear_any(); }
inline bool OBJ_FLAGGED(const CObjectPrototype* obj, const EExtraFlag flag) { return obj->get_extra_flag(flag); }
inline void SET_OBJ_AFF(CObjectPrototype* obj, const uint32_t packed_flag) { return obj->set_obj_aff(packed_flag); }
inline bool OBJ_AFFECT(const CObjectPrototype* obj, const uint32_t weapon_affect) { return obj->get_affect(weapon_affect); }

inline bool OBJ_AFFECT(const CObjectPrototype* obj, const EWeaponAffectFlag weapon_affect)
{
	return OBJ_AFFECT(obj, static_cast<uint32_t>(weapon_affect));
}

class CActionDescriptionWriter : public AbstractStringWriter
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

float count_affect_weight(const CObjectPrototype* obj, int num, int mod);
bool is_armor_type(const CObjectPrototype *obj);
void release_purged_list();
void init_item_levels();
void init_ilvl(CObjectPrototype *obj);
bool is_mob_item(const CObjectPrototype *obj);

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

namespace SetSystem
{
	void check_item(int vnum);
	void check_rented();
	void init_vnum_list(int vnum);
	bool find_set_item(OBJ_DATA *obj);
	bool is_big_set(const CObjectPrototype *obj, bool is_mini = false);
	bool is_norent_set(CHAR_DATA *ch, OBJ_DATA *obj, bool clan_chest = false);
	bool is_norent_set(int vnum, std::vector<int> objs);
} // namespace SetSystem

#endif // OBJ_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
