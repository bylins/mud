// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_HPP_INCLUDED
#define OBJ_HPP_INCLUDED

#include "engine/ui/cmd/do_telegram.h"
#include "engine/core/conf.h"
#include "entities_constants.h"
#include "engine/db/id.h"
#include "gameplay/mechanics/obj_enchant.h"
#include "gameplay/magic/spells.h"
#include "gameplay/affects/obj_affects.h"   // obj_affects::ObjAffects (affects that live ON the item)
#include "gameplay/skills/skills.h"
#include "engine/structs/extra_description.h"
#include "engine/structs/flag_data.h"
#include "engine/core/sysdep.h"
#include "utils/utils_string.h"
#include "utils/grammar/cases.h"

#include <array>
#include <vector>
#include <string>
#include <map>
#include <list>

std::string print_obj_affects(const obj_affected_type &affect);
void print_obj_affects(CharData *ch, const obj_affected_type &affect);
void set_obj_eff(ObjData *itemobj, EApply type, int mod);
void set_obj_aff(ObjData *itemobj, EAffect bitv);

/// О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ GET_OBJ_VAL О©╫О©╫О©╫О©╫О©╫
/// О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫-О©╫О©╫О©╫О©╫ - О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ TextId::init_obj_vals()
/// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫/О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ remove_incorrect_keys()
class ObjVal {
 public:
	enum class EValueKey : uint32_t {
		// issue.potion-hotfix: magic-potion (kPotion) payload, also carried by a drink container.
		// Persisted BY NAME via the text_id kObjVals table (utils_parse.cpp), so the on-disk
		// strings stay stable even though the C++ identifiers follow the Google style.
		kPotionSpell1Num = 0,
		kPotionSpell1Lvl = 1,   // retired: no per-spell power (misnamed "level"). Kept only so old
		kPotionSpell2Num = 2,   // saves still load; strength is kPotionPotency, one per potion.
		kPotionSpell2Lvl = 3,   // retired
		kPotionSpell3Num = 4,   // the REAL 3rd spell again (had been hijacked to store the potency)
		kPotionSpell3Lvl = 5,   // retired
		kPotionProtoVnum = 6,   // vnum of the source potion poured into a container (0 = set via OLC)
		kPotionPotency = 7,     // the potion's single casting potency (competence C), replayed at cast
		kPotionBrewRoll = 8,    // the brew-success roll: the noise realization, replayed at cast
		kPotionSkill = 9,       // the MAKER's skill (brew skill for crafted, authored default else) --
		                        // stands in for the magic skill (non-mages brew too); drives DURATION
		                        // and, with kPotionStat, the competence (AMOUNT). Drinker never matters.
		kPotionStat = 10,       // the MAKER's key stat (Intelligence when brewing / authored default)
		// issue.potion-hotfix: drink/food CONTENTS state, split out of the historically overloaded
		// val[3] (which meant BOTH a freshness countdown AND, at exactly 1, "poisoned"). Persisted by
		// name via the kObjVals text_id table, like the potion keys above.
		kLiquidTimer = 11,      // contents freshness: counts down 1/mud-hour; reaching 0 spoils the potion
		kLiquidPoison = 12      // poison level applied on drinking (0 = harmless); set when it spoils
	};

	// issue.potion-hotfix: fixed-point scale for kPotionBrewRoll. The stored int is
	// lround((1 + eps) * kBrewRollScale), so the brewed noise factor (always > 0) equals
	// kPotionBrewRoll / kBrewRollScale -- a fractional noise realization living in the int store.
	static constexpr int kBrewRollScale = 1000;
	// issue.potion-hotfix: bias so a NEGATIVE brew-luck z still stores as a POSITIVE int (the value map
	// drops negatives). Stored = lround((z + kBrewRollBias) * kBrewRollScale); z = stored/scale - bias.
	// Draw range clamps |z| to <= kBrewRollBias (a standard normal essentially never exceeds it).
	static constexpr int kBrewRollBias = 5;

	class EValueKeyHash {
	 public:
		std::size_t operator()(const EValueKey value) const { return static_cast<std::size_t>(value); }
	};

	using values_t = std::unordered_map<EValueKey, int, EValueKeyHash>;
	using const_iterator = values_t::const_iterator;
	const_iterator begin() const { return m_values.begin(); }
	const_iterator end() const { return m_values.end(); }
	bool empty() const { return m_values.empty(); }

	// \return -1 - О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	int get(const EValueKey key) const;
	// О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫/О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	// \param val < 0 - О©╫О©╫О©╫О©╫О©╫О©╫ (О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫) О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	void set(const EValueKey key, int val);
	// О©╫О©╫О©╫О©╫ key О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	// \param val О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ +-
	void inc(const EValueKey key, int val = 1);
	// save/load О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	std::string print_to_file() const;
	bool init_from_file(const char *str);
	// О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫
	std::string print_to_zone() const;
	void init_from_zone(const char *str);
	// О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	bool operator==(const ObjVal &r) const {
		return m_values == r.m_values;
	}
	bool operator!=(const ObjVal &r) const {
		return m_values != r.m_values;
	}
	// О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ (О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫/О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫.О©╫.)
	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	void remove_incorrect_keys(int type);

 private:
	values_t m_values;
};

class CObjectPrototype;

class VNumChangeObserver {
 public:
	using shared_ptr = std::shared_ptr<VNumChangeObserver>;

	virtual ~VNumChangeObserver() {}

	virtual void notify(CObjectPrototype &object, const ObjVnum old_vnum) = 0;
};

class IDChangeObserver {
 public:
	using shared_ptr = std::shared_ptr<IDChangeObserver>;

	virtual ~IDChangeObserver() {}

	virtual void notify(ObjData &object, const object_id_t old_id) = 0;
};

class ObjectRNum_ChangeObserver {
 public:
	using shared_ptr = std::shared_ptr<ObjectRNum_ChangeObserver>;

	virtual ~ObjectRNum_ChangeObserver() {}

	virtual void notify(CObjectPrototype &object, const ObjRnum old_rnum) = 0;
};

class UIDChangeObserver {
 public:
	using shared_ptr = std::shared_ptr<UIDChangeObserver>;

	virtual ~UIDChangeObserver() {}

	virtual void notify(ObjData &object, const int old_uid) = 0;
};

class CObjectPrototype {
 public:
	using shared_ptr = std::shared_ptr<CObjectPrototype>;

	constexpr static int DEFAULT_MAXIMUM_DURABILITY = 100;
	constexpr static int DEFAULT_CURRENT_DURABILITY = DEFAULT_MAXIMUM_DURABILITY;
	constexpr static int DEFAULT_LEVEL = 0;
	constexpr static int DEFAULT_WEIGHT = INT_MAX;
	constexpr static EObjType DEFAULT_TYPE = kOther;
	constexpr static EObjMaterial DEFAULT_MATERIAL = kMaterialUndefined;

	constexpr static int DEFAULT_COST = 100;
	constexpr static int DEFAULT_RENT_ON = 100;
	constexpr static int DEFAULT_RENT_OFF = 100;
	constexpr static int UNLIMITED_GLOBAL_MAXIMUM = -1;
	constexpr static int DEFAULT_MAX_IN_WORLD = UNLIMITED_GLOBAL_MAXIMUM;
	constexpr static int DEFAULT_MINIMUM_REMORTS = 0;

	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	constexpr static int UNLIMITED_TIMER = 2147483647;
	constexpr static int ONE_DAY = 24 * 60;
	constexpr static int SEVEN_DAYS = 7 * ONE_DAY;
	constexpr static int DEFAULT_TIMER = SEVEN_DAYS;

	constexpr static int DEFAULT_DESTROYER = 60;
	constexpr static int DEFAULT_RNUM = kNothing;
	constexpr static int VALS_COUNT = 4;
	constexpr static int CORPSE_INDICATOR = 1; // container with value 3 set to 1 is a corpse

	using skills_t = std::map<ESkill, int>;
	using vals_t = std::array<int, VALS_COUNT>;
	using wear_flags_t = std::underlying_type<EWearFlag>::type;
	using pnames_t = std::array<std::string, grammar::ECase::kLastCase + 1>;
	using triggers_list_t = std::list<ObjVnum>;
	using triggers_list_ptr = std::shared_ptr<triggers_list_t>;
	using affected_t = std::array<obj_affected_type, kMaxObjAffect>;

	CObjectPrototype(const ObjVnum vnum) : m_vnum(vnum),
										   m_type(DEFAULT_TYPE),
										   m_weight(DEFAULT_WEIGHT),
										   m_parent_proto(-1),
										   m_proto_script(new triggers_list_t()),
										   m_max_in_world(DEFAULT_MAX_IN_WORLD),
										   m_vals({0, 0, 0, 0}),
										   m_destroyer(DEFAULT_DESTROYER),
										   m_spell(ESpell::kUndefined),
										   m_level(DEFAULT_LEVEL),
										   m_sparam(-1),
										   m_maximum_durability(DEFAULT_MAXIMUM_DURABILITY),
										   m_current_durability(DEFAULT_CURRENT_DURABILITY),
										   m_material(DEFAULT_MATERIAL),
										   m_sex(EGender::kMale),
										   m_wear_flags(),
										   m_timer(DEFAULT_TIMER),
										   m_minimum_remorts(DEFAULT_MINIMUM_REMORTS),  // О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫. О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
											m_cost(DEFAULT_COST),
										   m_rent_on(DEFAULT_RENT_ON),
										   m_rent_off(DEFAULT_RENT_OFF),
										   m_ilevel(0),
										   m_in_extracted_list(false),
										   m_rnum(DEFAULT_RNUM) {}
	virtual ~CObjectPrototype() = default;
	ObjRnum get_parent_rnum() const {return m_parent_proto;}
	ObjVnum get_parent_vnum();
	void set_parent_rnum(ObjRnum _) {m_parent_proto = _;}
	auto &get_skills() const { return m_skills; }
	auto dec_val(size_t index) { return --m_vals[index]; }
	auto get_current_durability() const { return m_current_durability; }
	auto get_destroyer() const { return m_destroyer; }
	auto get_level() const { return m_level; }
	auto get_material() const { return m_material; }
	auto get_max_in_world() const { return m_max_in_world; }
	auto get_maximum_durability() const { return m_maximum_durability; }
	auto get_sex() const { return m_sex; }
	auto get_spec_param() const { return m_sparam; }
	auto get_spell() const { return m_spell; }
	auto get_type() const { return m_type; }
	auto get_val(size_t index) const { return m_vals[index]; }
	auto GetPotionValueKey(const ObjVal::EValueKey key) const { return m_values.get(key); }
	wear_flags_t get_wear_flags() const { return m_wear_flags.get_plane(0); }
	auto get_weight() const { return m_weight; }
	auto serialize_values() const { return m_values.print_to_file(); }
	bool can_wear_any() const { return m_wear_flags.any() && m_wear_flags.get_plane(0) != to_underlying(EWearFlag::kTake); }
	bool GetEEquipmentAffect(const EEquipmentAffect equipment_affect) const { return m_waffect_flags.get(equipment_affect); }
	bool GetEquipmentAffect(const Bitvector equipment_affect) const { return m_waffect_flags.get(static_cast<EEquipmentAffect>(equipment_affect)); }
	bool has_anti_flag(const EAntiFlag flag) const { return m_anti_flags.get(flag); }
	bool has_flag(const EObjFlag packed_flag) const { return m_extra_flags.get(packed_flag); }
	bool has_flag(const size_t plane, const Bitvector flag) const { return m_extra_flags.get_flag(plane, flag); }
	bool has_no_flag(const ENoFlag flag) const { return m_no_flags.get(flag); }
	bool has_wear_flag(const EWearFlag part) const;
	bool get_wear_mask(const wear_flags_t part) const;
	bool init_values_from_file(const char *str) { return m_values.init_from_file(str); }
	const auto &get_action_description() const { return m_action_description; }
	const auto &get_affect_flags() const { return m_waffect_flags; }
	const auto &get_affected(const size_t index) const { return m_affected[index]; }
	const auto &get_aliases() const { return m_aliases; }
	const auto &get_all_affected() const { return m_affected; }
	const auto &get_all_values() const { return m_values; }
	const auto &get_anti_flags() const { return m_anti_flags; }
	const auto &get_description() const { return m_description; }
	const auto &get_ex_description() const { return m_ex_description; }
	const auto &get_extra_flags() const { return m_extra_flags; }
	const auto &get_no_flags() const { return m_no_flags; }
	const auto &get_proto_script() const { return *m_proto_script; }
	const auto &get_proto_script_ptr() const { return m_proto_script; }
	const std::string &get_PName(const grammar::ECase name_case = grammar::ECase::kNom) const { return m_pnames[name_case]; }
	const std::string &get_short_description() const { return m_short_description; }
	void add_affect_flags(const BitsetFlags<EEquipmentAffect> &flags) { m_waffect_flags += flags; }
	void add_affected(const size_t index, const int amount) { m_affected[index].modifier += amount; }
	void add_anti_flags(const BitsetFlags<EAntiFlag> &flags) { m_anti_flags += flags; }
	void add_extra_flags(const BitsetFlags<EObjFlag> &flags) { m_extra_flags += flags; }
	void add_maximum(const int amount) { m_maximum_durability += amount; }
	void add_no_flags(const BitsetFlags<ENoFlag> &flags) { m_no_flags += flags; }
	void add_proto_script(const ObjVnum vnum) { m_proto_script->push_back(vnum); }
	void add_val(const size_t index, const int amount) { m_vals[index] += amount; }
	void add_weight(const int _) { m_weight += _; }
	void clear_action_description() { m_action_description.clear(); }
	void clear_affected(const size_t index) { m_affected[index].location = EApply::kNone; }
	void clear_all_affected();
	void clear_proto_script();
	void dec_affected_value(const size_t index) { --m_affected[index].modifier; }
	void dec_destroyer() { --m_destroyer; }
	void dec_weight() { --m_weight; }
	void gm_affect_flag(const char *subfield, const char **list, char *res) {
		m_extra_flags.gm_flag(subfield, list, res);
	}
	void gm_extra_flag(const char *subfield, const char **list, char *res) {
		m_extra_flags.gm_flag(subfield, list, res);
	}
	void inc_val(const size_t index) { ++m_vals[index]; }
	void init_values_from_zone(const char *str) { m_values.init_from_zone(str); }
	void load_affect_flags(const char *string) { m_waffect_flags.from_string(string); }
	void load_anti_flags(const char *string) { m_anti_flags.from_string(string); }
	void load_extra_flags(const char *string) { m_extra_flags.from_string(string); }
	void load_no_flags(const char *string) { m_no_flags.from_string(string); }
	void remove_incorrect_values_keys(const int type) { m_values.remove_incorrect_keys(type); }
	void set_action_description(const std::string &_) { m_action_description = _; }
	void SetEEquipmentAffectFlag(const EEquipmentAffect packed_flag) { m_waffect_flags.set(packed_flag); }
	void SetEquipmentAffectFlags(const BitsetFlags<EEquipmentAffect> &flags) { m_waffect_flags = flags; }
	void set_affected(const size_t index, const EApply location, const int modifier);
	void set_affected(const size_t index, const obj_affected_type &affect) { m_affected[index] = affect; }
	void set_affected_location(const size_t index, const EApply _) { m_affected[index].location = _; }
	void set_affected_modifier(const size_t index, const int _) { m_affected[index].modifier = _; }
	void set_aliases(const std::string &_) { m_aliases = _; }
	void set_all_affected(const affected_t &_) { m_affected = _; }
	void set_anti_flag(const EAntiFlag packed_flag) { m_anti_flags.set(packed_flag); }
	void set_anti_flags(const BitsetFlags<EAntiFlag> &flags) { m_anti_flags = flags; }
	void set_current_durability(const int _) { m_current_durability = _; }
	void set_description(const std::string &_) { m_description = _; }
	void set_destroyer(const int _) { m_destroyer = _; }
	void set_ex_description(const ExtraDescription::shared_ptr &_) { m_ex_description = _; }
	void set_ex_description(ExtraDescription *_) { m_ex_description.reset(_); }
	void set_extra_flag(const EObjFlag packed_flag) { m_extra_flags.set(packed_flag); }
	void set_extra_flag(const size_t plane, const Bitvector flag) { m_extra_flags.set_flag(plane, flag); }
	void set_extra_flags(const BitsetFlags<EObjFlag> &flags) { m_extra_flags = flags; }
	void set_level(const int _) { m_level = _; }
	void set_material(const EObjMaterial _) { m_material = _; }
	void set_max_in_world(const int _) { m_max_in_world = _; }
	void set_maximum_durability(const int _) { m_maximum_durability = _; }
	void set_no_flag(const ENoFlag flag) { m_no_flags.set(flag); }
	void set_no_flags(const BitsetFlags<ENoFlag> &flags) { m_no_flags = flags; }
	void set_obj_aff(const Bitvector packed_flag) { m_waffect_flags.set(static_cast<EEquipmentAffect>(packed_flag)); }
	void set_PName(const grammar::ECase index, const char *_) { m_pnames[index] = _; }
	void set_PName(const grammar::ECase index, const std::string &_) { m_pnames[index] = _; }
	void set_PNames(const pnames_t &_) { m_pnames = _; }
	void set_proto_script(const triggers_list_t &_) { *m_proto_script = _; }
	void set_short_description(const char *_) { m_short_description = _; }
	void set_short_description(const std::string &_) { m_short_description = _; }
	void set_spec_param(const int _) { m_sparam = _; }
	void set_spell(const int _) { m_spell = std::clamp(static_cast<ESpell>(_), ESpell::kUndefined, ESpell::kLast); }
	void set_type(const EObjType _) { m_type = _; }
	void set_sex(const EGender _) { m_sex = _; }
	void SetPotionValueKey(const ObjVal::EValueKey key, const int value) { return m_values.set(key, value); }
	void SetPotionValues(const ObjVal &_) { m_values = _; }
	void set_wear_flag(const EWearFlag flag);
	void set_wear_flags(const wear_flags_t _) { m_wear_flags.clear(); m_wear_flags.set_plane(0, _); }
	void set_weight(const int _) { m_weight = _; }
	void set_val(size_t index, int value) { m_vals[index] = value; }
	void sub_current(const int _) { m_current_durability -= _; }
	void sub_val(const size_t index, const int amount) { m_vals[index] -= amount; }
	void sub_weight(const int _) { m_weight -= _; }
	void swap_proto_script(triggers_list_t &_) { m_proto_script->swap(_); }
	void toggle_affect_flag(const size_t plane, const Bitvector flag) { m_waffect_flags.toggle_flag(plane, flag); }
	void toggle_anti_flag(const size_t plane, const Bitvector flag) { m_anti_flags.toggle_flag(plane, flag); }
	void toggle_extra_flag(const size_t plane, const Bitvector flag) { m_extra_flags.toggle_flag(plane, flag); }
	void toggle_no_flag(const size_t plane, const Bitvector flag) { m_no_flags.toggle_flag(plane, flag); }
	void toggle_skill(const uint32_t skill);
	void toggle_val_bit(const size_t index, const Bitvector bit);
	void toggle_wear_flag(const Bitvector flag);
	void unset_extraflag(const EObjFlag packed_flag) { m_extra_flags.unset(packed_flag); }
	void set_skill(ESkill skill_num, int percent);
	int get_skill(ESkill skill_num) const;
	void get_skills(skills_t &out_skills) const;
	bool has_skills() const;
	virtual void set_timer(int timer);
	virtual int get_timer() const;
	void set_skills(const skills_t &_) { m_skills = _; }
	auto get_minimum_remorts() const { return m_minimum_remorts; }
	auto get_dgscript_field() const { return m_dgscript_field; }
	auto get_cost() const { return m_cost; }
	void set_cost(int x);
	auto get_rent_off() const { return m_rent_off; }
	void set_rent_off(int x);
	auto get_rent_on() const { return m_rent_on; }
	void set_rent_on(int x);
	void set_ex_description(const char *keyword, const char *description);
	void set_minimum_remorts(const int _) { m_minimum_remorts = _; }
	void set_dgscript_field(const std::string _) { m_dgscript_field = _; }
	int get_auto_mort_req() const;
	double show_mort_req() const;
	double show_koef_obj() const;
	double get_ilevel() const;    ///< О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	void set_ilevel(double ilvl);
	auto get_rnum() const { return m_rnum; }
	void set_rnum(const ObjRnum _);
	auto get_vnum() const { return m_vnum; }
	auto get_extracted_list() const { return m_in_extracted_list; }
	// issue.obj-casting: mirrors CharData::purged() -- true once the object is on the deferred
	// extraction list (AddToExtractedList): logically destroyed but not yet freed (freed by
	// WorldObjects::PurgeExtractedList at heartbeat end). Cast stages guard ctx.ovict with this.
	[[nodiscard]] bool purged() const { return m_in_extracted_list; }
	void set_extracted_list(bool _);
	void subscribe_for_vnum_changes(const VNumChangeObserver::shared_ptr &observer) {
		m_vnum_change_observers.insert(observer);
	}
	void unsubscribe_from_vnum_changes(const VNumChangeObserver::shared_ptr &observer) {
		m_vnum_change_observers.erase(observer);
	}
	void subscribe_for_rnum_changes(const ObjectRNum_ChangeObserver::shared_ptr &observer) {
		m_rnum_change_observers.insert(observer);
	}
	void unsubscribe_from_rnum_changes(const ObjectRNum_ChangeObserver::shared_ptr &observer) {
		m_rnum_change_observers.erase(observer);
	}

	std::string item_count_message(int num, grammar::ECase name_case);
	void DungeonProtoCopy(const CObjectPrototype &from);

 protected:
	void zero_init();
	CObjectPrototype &operator=(const CObjectPrototype &from);    ///< makes shallow copy of all fields except VNUM
	CObjectPrototype(const CObjectPrototype &other);
	void set_vnum(const ObjVnum vnum);        ///< allow inherited classes change VNUM (to make possible objects transformations)
	void tag_ex_description(const char *tag);

 private:
	ObjVnum m_vnum;

	EObjType m_type;
	int m_weight;

	affected_t m_affected;    // affects //
	ObjRnum m_parent_proto;
	std::string m_aliases;        // Title of object :get etc.        //
	std::string m_description;    // When in room                     //

	std::string m_short_description;    // when worn/carry/in cont.         //
	std::string m_action_description;    // What to write when used          //
	ExtraDescription::shared_ptr m_ex_description;    // extra descriptions     //

	triggers_list_ptr m_proto_script;    // list of default triggers  //

	pnames_t m_pnames;
	int m_max_in_world;    ///< Maximum in the world

	vals_t m_vals;        // old values
	ObjVal m_values;    // new values
	int m_destroyer;
	ESpell m_spell;
	int m_level;
	int m_sparam;
	int m_maximum_durability;
	int m_current_durability;

	EObjMaterial m_material;
	EGender m_sex;

	BitsetFlags<EObjFlag> m_extra_flags;    // If it hums, glows, etc.      //
	BitsetFlags<EEquipmentAffect> m_waffect_flags;
	BitsetFlags<EAntiFlag> m_anti_flags;
	BitsetFlags<ENoFlag> m_no_flags;

	BitsetFlags<EWearFlag> m_wear_flags;        // Where you can wear it     //

	int m_timer;    ///< О©╫О©╫О©╫О©╫О©╫О©╫ (О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫)

	skills_t m_skills;    ///< О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫. О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫"

	int m_minimum_remorts;    ///< О©╫О©╫О©╫О©╫ > 0 - О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫
	std::string m_dgscript_field;
	int m_cost;    ///< О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	int m_rent_on;    ///< О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	int m_rent_off;    ///< О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫

	double m_ilevel;    ///< О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	bool m_in_extracted_list;
	ObjVnum m_rnum;    ///< Where in data-base

	std::unordered_set<VNumChangeObserver::shared_ptr> m_vnum_change_observers;
	std::unordered_set<ObjectRNum_ChangeObserver::shared_ptr> m_rnum_change_observers;

	void MakeShallowCopy(const CObjectPrototype &from);
};

inline auto GET_OBJ_VAL(const CObjectPrototype *obj, size_t index) {
	if (nullptr == obj) {
		return 0;
	}

	return obj->get_val(index);
}
inline auto GET_OBJ_VAL(const CObjectPrototype::shared_ptr &obj, size_t index) { return GET_OBJ_VAL(obj.get(), index); }

class activation {
	std::string actmsg, deactmsg, room_actmsg, room_deactmsg;
	BitsetFlags<EEquipmentAffect> affects;
	std::array<obj_affected_type, kMaxObjAffect> affected;
	int weight, ndices, nsides;
	CObjectPrototype::skills_t skills;

 public:
	activation() : affects(), weight(-1), ndices(-1), nsides(-1) {}

	activation(const std::string &__actmsg, const std::string &__deactmsg,
			   const std::string &__room_actmsg, const std::string &__room_deactmsg,
			   const BitsetFlags<EEquipmentAffect> &__affects, const obj_affected_type *__affected,
			   int __weight, int __ndices, int __nsides) :
		actmsg(__actmsg), deactmsg(__deactmsg), room_actmsg(__room_actmsg),
		room_deactmsg(__room_deactmsg), affects(__affects), weight(__weight),
		ndices(__ndices), nsides(__nsides) {
		for (int i = 0; i < kMaxObjAffect; i++)
			affected[i] = __affected[i];
	}

	bool has_skills() const {
		return !skills.empty();
	}

	// * @warning О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫ __out_skills.empty() == true.
	void get_skills(CObjectPrototype::skills_t &__skills) const {
		__skills.insert(skills.begin(), skills.end());
	}

	activation &set_skill(const ESkill __skillnum, const int __percent) {
		CObjectPrototype::skills_t::iterator skill = skills.find(__skillnum);
		if (skill == skills.end()) {
			if (__percent != 0)
				skills.insert(std::make_pair(__skillnum, __percent));
		} else {
			if (__percent != 0)
				skill->second = __percent;
			else
				skills.erase(skill);
		}

		return *this;
	}

	void get_dices(int &__ndices, int &__nsides) const {
		__ndices = ndices;
		__nsides = nsides;
	}

	activation &
	set_dices(int __ndices, int __nsides) {
		ndices = __ndices;
		nsides = __nsides;
		return *this;
	}

	int get_weight() const {
		return weight;
	}

	activation &
	set_weight(int __weight) {
		weight = __weight;
		return *this;
	}

	const std::string &
	get_actmsg() const {
		return actmsg;
	}

	activation &
	set_actmsg(const std::string &__actmsg) {
		actmsg = __actmsg;
		return *this;
	}

	const std::string &
	get_deactmsg() const {
		return deactmsg;
	}

	activation &
	set_deactmsg(const std::string &__deactmsg) {
		deactmsg = __deactmsg;
		return *this;
	}

	const std::string &
	get_room_actmsg() const {
		return room_actmsg;
	}

	activation &
	set_room_actmsg(const std::string &__room_actmsg) {
		room_actmsg = __room_actmsg;
		return *this;
	}

	const std::string &
	get_room_deactmsg() const {
		return room_deactmsg;
	}

	activation &
	set_room_deactmsg(const std::string &__room_deactmsg) {
		room_deactmsg = __room_deactmsg;
		return *this;
	}

	const BitsetFlags<EEquipmentAffect> &
	get_affects() const {
		return affects;
	}

	activation &
	set_affects(const BitsetFlags<EEquipmentAffect> &__affects) {
		affects = __affects;
		return *this;
	}

	const std::array<obj_affected_type, kMaxObjAffect> &
	get_affected() const {
		return affected;
	}

	activation &
	set_affected(const obj_affected_type *__affected) {
		for (int i = 0; i < kMaxObjAffect; i++)
			affected[i] = __affected[i];
		return *this;
	}

	const obj_affected_type &
	get_affected_i(int __i) const {
		return __i < 0 ? affected[0] :
			   __i < kMaxObjAffect ? affected[__i] : affected[kMaxObjAffect - 1];
	}

	activation &
	set_affected_i(int __i, const obj_affected_type &__affected) {
		if (__i >= 0 && __i < kMaxObjAffect)
			affected[__i] = __affected;

		return *this;
	}
};

typedef std::map<unique_bit_flag_data, activation> class_to_act_map;
typedef std::map<unsigned int, class_to_act_map> qty_to_camap_map;

class set_info : public std::map<ObjVnum, qty_to_camap_map> {
	std::string name;
	std::string alias;

 public:
	typedef std::map<ObjVnum, qty_to_camap_map> ovnum_to_qamap_map;

	set_info() {}

	set_info(const ovnum_to_qamap_map &__base, const std::string &__name) : ovnum_to_qamap_map(__base), name(__name) {}

	const std::string &
	get_name() const {
		return name;
	}

	set_info &
	set_name(const std::string &__name) {
		name = __name;
		return *this;
	}

	const std::string &get_alias() const {
		return alias;
	}

	void set_alias(const std::string &_alias) {
		alias = _alias;
	}
};

typedef std::map<int, set_info> id_to_set_info_map;

// issue.obj-affects: the former "TimedSpell on objects" mechanic is now the ObjAffect subsystem
// (gameplay/affects/obj_affects.h) -- affects that live ON an item, keyed by obj_affects::EObjAffect.
// ObjData stores them as obj_affects::ObjAffects (see m_obj_affects); runtime lives in obj_affects.cpp.


// О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫"
struct custom_label {
 public:
	custom_label() : text_label(nullptr), clan_abbrev(nullptr), author(-2), author_mail(nullptr) {}
	~custom_label();

	char *text_label; // О©╫О©╫О©╫О©╫О©╫
	char *clan_abbrev;       // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
	int author;       // О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫: О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ ch->get_idnum(), О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -2
	char *author_mail;// О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫
};

inline custom_label::~custom_label() {
	if (nullptr != text_label) {
		free(text_label);
	}

	if (nullptr != clan_abbrev) {
		free(clan_abbrev);
	}

	if (nullptr != author_mail) {
		free(author_mail);
	}
}

class Script;    // to avoid inclusion of "dg_scripts.h"

class ObjData : public CObjectPrototype {
 public:
	using shared_ptr = std::shared_ptr<ObjData>;
	using obj_list_t = std::list<ObjData*>;

	constexpr static const int DEFAULT_MAKER = 0;
	constexpr static const int DEFAULT_OWNER = 0;
	constexpr static const int DEFAULT_PARENT = 0;

	ObjData(const CObjectPrototype &);
	~ObjData();

	const std::string activate_obj(const activation &__act);
	const std::string deactivate_obj(const activation &__act);

	int get_serial_num();
	void set_serial_num(int num);

	void set_timer(int timer) override;
	int get_timer() const override;
	void dec_timer(int time = 1, bool ingore_utimer = false, bool exchange = false);
	void process_periodic_effects();

	static id_to_set_info_map set_table;

	void purge();

	void set_activator(bool flag, int num);
	std::pair<bool, int> get_activator() const;

	// issue.obj-affects: affects that live ON this item (obj_affects::EObjAffect). Runtime helpers
	// (impose/remove/tick/query) are free functions in gameplay/affects/obj_affects.h.
	const obj_affects::ObjAffects &get_obj_affects() const { return m_obj_affects; }
	obj_affects::ObjAffects &get_obj_affects() { return m_obj_affects; }
	bool has_obj_affects() const { return !m_obj_affects.empty(); }

	// issue.obj-suppressor-affect: equipment affects this item confers but that are temporarily suppressed
	// by a dispel (affect_total skips them while suppressed). Backed by kSuppressed obj-affects, so they
	// persist across rent/relog and tick with every other obj-affect; the API lives in obj_affects.h.
	bool is_affect_suppressed(EAffect aff) const { return obj_affects::IsEquipAffectSuppressed(this, aff); }
	bool has_suppressed_affects() const { return !obj_affects::SuppressedEquipAffects(this).empty(); }
	std::vector<std::pair<EAffect, int>> suppressed_equip_affects() const { return obj_affects::SuppressedEquipAffects(this); }
	// issue.affect-suppression-dispell: lift the suppression of `aff` NOW (weave-restoration success) --
	// erase the kSuppressed and, if worn, re-materialize the equipment affect on the wearer (same as the
	// timer auto-return in process_periodic_effects). No-op if `aff` was not suppressed.
	void release_suppression(EAffect aff);

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
	auto get_room_was_in() const { return m_room_was_in; }
	auto get_unique_id() const { return m_unique_id; }
	auto get_worn_by() const { return m_worn_by; }
	auto get_worn_on() const { return m_worn_on; }
	auto get_where_obj() const { return m_where_obj; }
	auto get_vnum_zone_from() const { return m_zone_from; }
	auto serialize_enchants() const { return m_enchants.print_to_file(); }
	const auto &get_enchants() const { return m_enchants; }
	const auto &get_custom_label() const { return m_custom_label; }
	const auto &get_script() const { return m_script; }
	void add_enchant(const ObjectEnchant::enchant &_) { m_enchants.add(_); }
	void remove_custom_label() { m_custom_label.reset(); }
	void remove_me_from_contains_list(ObjData *&head);
	void remove_me_from_objects_list(ObjData *&head);
	void remove_set_bonus() { m_enchants.remove_set_bonus(this); }
	void set_carried_by(CharData *_) { m_carried_by = _; }
	void set_contains(ObjData *_) { m_contains = _; }
	void set_craft_timer(const int _) { m_craft_timer = _; }
	void set_crafter_uid(const int _) { m_maker = _; }
	void set_custom_label(const std::shared_ptr<custom_label> &_) { m_custom_label = _; }
	void set_custom_label(custom_label *_) { m_custom_label.reset(_); }
	void set_id(const long _);
	void set_in_obj(ObjData *_) { m_in_obj = _; }
	void set_in_room(const RoomRnum _) { m_in_room = _; }
	void set_is_rename(const bool _) { m_is_rename = _; }
	void set_next(ObjData *_) { m_next = _; }
	void set_next_content(ObjData *_) { m_next_content = _; }
	void set_owner(const int _) { m_owner = _; }
	void set_room_was_in(const int _) { m_room_was_in = _; }
	void set_script(const std::shared_ptr<Script> &_) { m_script = _; }
	void set_script(Script *_);
	void cleanup_script();
	void set_unique_id(const long _);
	void set_worn_by(CharData *_) { m_worn_by = _; }
	void set_worn_on(const short _) { m_worn_on = _; }
	void set_where_obj(const EWhereObj _) { m_where_obj = _; }
	void set_vnum_zone_from(const int _) { m_zone_from = _; }
	void update_enchants_set_bonus(const obj_sets::ench_type &_) { m_enchants.update_set_bonus(this, _); }
	void set_enchant(int skill);
	void set_enchant(int skill, ObjData *obj);
	void unset_enchant();
	void copy_name_from(const CObjectPrototype *src);

	bool clone_olc_object_from_prototype(const ObjVnum vnum);
	void copy_from(const CObjectPrototype *src);

	void swap(ObjData &object, bool swap_trig = true);
	void set_tag(const char *tag);

	void subscribe_for_id_change(const IDChangeObserver::shared_ptr &observer) { m_id_change_observers.insert(observer); }
	void unsubscribe_from_id_change(const IDChangeObserver::shared_ptr &observer) { m_id_change_observers.erase(observer); }

	void subscribe_for_unique_id_change(const UIDChangeObserver::shared_ptr &observer) {
		m_unique_id_change_observers.insert(observer);
	}
	void unsubscribe_from_unique_id_change(const UIDChangeObserver::shared_ptr &observer) {
		m_unique_id_change_observers.erase(observer);
	}

	void attach_triggers(const triggers_list_t &trigs);
	bool is_unrentable();

 private:
	void zero_init();

	void detach_ex_description();

	long m_unique_id;
	RoomRnum m_in_room;    // In what room -1 when conta/carr //
	int m_room_was_in;

	int m_maker;
	int m_owner;
	int m_zone_from;
	bool m_is_rename;
	EWhereObj m_where_obj;
	CharData *m_carried_by;    // Carried by :NULL in room/conta   //
	CharData *m_worn_by;    // Worn by?              //
	short int m_worn_on;        // Worn where?          //

	std::shared_ptr<custom_label> m_custom_label;        // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ //

	ObjData *m_in_obj;    // In what object NULL when none    //
	ObjData *m_contains;    // Contains objects                 //
	ObjData *m_next_content;    // For 'contains' lists             //
	ObjData *m_next;        // For the object list              //

	ObjectEnchant::Enchants m_enchants;

	int m_craft_timer;

	obj_affects::ObjAffects m_obj_affects;    ///< issue.obj-affects: affects on this item

	long  m_id;            // used by DG triggers              //
	std::shared_ptr<Script> m_script;    // script info for the object       //

	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ (О©╫О©╫О©╫ name_list)
	int m_serial_number;
	// true - О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ delete О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	bool m_purged;
	// О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ <О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫>
	std::pair<bool, int> m_activator;

	std::unordered_set<IDChangeObserver::shared_ptr> m_id_change_observers;
	std::unordered_set<UIDChangeObserver::shared_ptr> m_unique_id_change_observers;
};

inline void CObjectPrototype::set_affected(const size_t index, const EApply location, const int modifier) {
	m_affected[index].location = location;
	m_affected[index].modifier = modifier;
}
//void delete_item(const std::size_t pt_num, int vnum);
inline bool CAN_WEAR(const CObjectPrototype *obj, const EWearFlag part) { return obj->has_wear_flag(part); }
inline bool CAN_WEAR_ANY(const CObjectPrototype *obj) { return obj->can_wear_any(); }
inline void SET_OBJ_AFF(CObjectPrototype *obj, const Bitvector packed_flag) { return obj->set_obj_aff(packed_flag); }
inline bool OBJ_AFFECT(const CObjectPrototype *obj,
					   const Bitvector equipment_affect) { return obj->GetEquipmentAffect(equipment_affect); }

inline bool OBJ_AFFECT(const CObjectPrototype *obj, const EEquipmentAffect equipment_affect) {
	return OBJ_AFFECT(obj, static_cast<Bitvector>(equipment_affect));
}
int GetObjMIW(ObjRnum rnum);

class CActionDescriptionWriter : public utils::AbstractStringWriter {
 public:
	CActionDescriptionWriter(ObjData &object) : m_object(object) {}

	virtual const char *get_string() const { return m_object.get_action_description().c_str(); }
	virtual void set_string(const char *data) { m_object.set_action_description(data); }
	virtual void append_string(const char *data) {
		m_object.set_action_description(m_object.get_action_description() + data);
	}
	virtual size_t length() const { return m_object.get_action_description().length(); }
	virtual void clear() { m_object.clear_action_description(); }

 private:
	ObjData &m_object;
};

namespace ObjSystem {
float count_affect_weight(const CObjectPrototype *obj, int num, int mod);
bool is_armor_type(const CObjectPrototype *obj);
void release_purged_list();
void init_item_levels();
void init_ilvl(CObjectPrototype *obj);
bool is_mob_item(const CObjectPrototype *obj);
} // namespace ObjSystem

std::string char_get_custom_label(ObjData *obj, CharData *ch);

namespace system_obj {
/// О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
extern int PURSE_RNUM;
/// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
extern int PERS_CHEST_RNUM;
void init();
ObjData *create_purse(CharData *ch, int gold);
bool is_purse(ObjData *obj);
void process_open_purse(CharData *ch, ObjData *obj);
// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫-О©╫О©╫О©╫
extern TelegramBot *bot;
} // namespace system_obj

namespace SetSystem {
void check_item(int vnum);
void check_rented();
void init_vnum_list(int vnum);
bool find_set_item(ObjData *obj);
bool is_big_set(const CObjectPrototype *obj, bool is_mini = false);
bool is_norent_set(CharData *ch, ObjData *obj, bool clan_chest = false);
bool is_norent_set(int vnum, std::vector<int> objs);
} // namespace SetSystem

#endif // OBJ_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
