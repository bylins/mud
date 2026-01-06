// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "obj_data.h"

#include "engine/db/obj_save.h"
#include "engine/db/world_objects.h"
#include "utils/parse.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/celebrates.h"
#include "gameplay/fight/pk.h"
#include "utils/cache.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/clans/house.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/mechanics/stable_objs.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <cmath>
#include <memory>
#include "engine/db/player_index.h"

//#include <sstream>

extern void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot);
extern void ExtractTrigger(Trigger *trig);
extern double CalcRemortRequirements(const CObjectPrototype *obj);

id_to_set_info_map ObjData::set_table;

ObjData::ObjData(const CObjectPrototype &other) :
	CObjectPrototype(other),
	m_unique_id(0),
	m_in_room(0),
	m_room_was_in(0),
	m_maker(DEFAULT_MAKER),
	m_owner(DEFAULT_OWNER),
	m_zone_from(0),
	m_is_rename(false),
	m_where_obj(EWhereObj::kNowhere),
	m_carried_by(nullptr),
	m_worn_by(nullptr),
	m_worn_on(0),
	m_in_obj(nullptr),
	m_contains(nullptr),
	m_next_content(nullptr),
	m_next(nullptr),
	m_craft_timer(0),
	m_id(0),
	m_script(new Script()),
	m_serial_number(0),
	m_purged(false),
	m_activator(false, 0) {
	caching::obj_cache.Add(this);
}

ObjData::~ObjData() {
	this->purge();
}

// * См. Character::zero_init()
void ObjData::zero_init() {
	CObjectPrototype::zero_init();
	set_weight(0);
	m_unique_id = 0;
	m_in_room = kNowhere;
	m_carried_by = nullptr;
	m_worn_by = nullptr;
	m_worn_on = kNowhere;
	m_in_obj = nullptr;
	m_contains = nullptr;
	m_id = 0;
	m_next_content = nullptr;
	m_next = nullptr;
	m_room_was_in = kNowhere;
	m_serial_number = 0;
	m_purged = false;
	m_activator.first = false;
	m_activator.second = 0;
	m_custom_label = nullptr;
}

void ObjData::detach_ex_description() {
	const auto old_description = get_ex_description();
	const auto new_description = std::make_shared<ExtraDescription>();
	if (nullptr != old_description->keyword) {
		new_description->keyword = str_dup(old_description->keyword);
	}
	if (nullptr != old_description->keyword) {
		new_description->description = str_dup(old_description->description);
	}
	set_ex_description(new_description);
}

// * См. Character::purge()
void ObjData::purge() {
	caching::obj_cache.Remove(this);
	//см. комментарий в структуре BloodyInfo из pk.cpp
	bloody::remove_obj(this);
	//weak_ptr тут бы был какраз в тему
	celebrates::RemoveFromObjLists(this->get_unique_id());
}

int ObjData::get_serial_num() {
	return m_serial_number;
}

void ObjData::set_serial_num(int num) {
	m_serial_number = num;
}

const std::string ObjData::activate_obj(const activation &__act) {
	if (get_rnum() >= 0) {
		SetWeaponAffectFlags(__act.get_affects());
		for (int i = 0; i < kMaxObjAffect; i++) {
			set_affected(i, __act.get_affected_i(i));
		}

		int weight = __act.get_weight();
		if (weight > 0) {
			set_weight(weight);
		}

		if (get_type() == kWeapon) {
			int nsides, ndices;
			__act.get_dices(ndices, nsides);
			// Типа такая проверка на то, устанавливались ли эти параметры.
			if (ndices > 0 && nsides > 0) {
				set_val(1, ndices);
				set_val(2, nsides);
			}
		}

		// Активируем умения.
		if (__act.has_skills()) {
			// У всех объектов с умениями skills указывает на один и тот же
			// массив. И у прототипов. Поэтому тут надо создавать новый,
			// если нет желания "активировать" сразу все такие объекты.
			// Умения, проставленные в сете, заменяют родные умения предмета.
			skills_t skills;
			__act.get_skills(skills);
			set_skills(skills);
		}

		return __act.get_actmsg() + "\n" + __act.get_room_actmsg();
	} else {
		return "\n";
	}
}

const std::string ObjData::deactivate_obj(const activation &__act) {
	if (get_rnum() >= 0) {
		SetWeaponAffectFlags(obj_proto[get_rnum()]->get_affect_flags());
		for (int i = 0; i < kMaxObjAffect; i++) {
			set_affected(i, obj_proto[get_rnum()]->get_affected(i));
		}

		set_weight(obj_proto[get_rnum()]->get_weight());

		if (get_type() == kWeapon) {
			set_val(1, obj_proto[get_rnum()]->get_val(1));
			set_val(2, obj_proto[get_rnum()]->get_val(2));
		}

		// Деактивируем умения.
		if (__act.has_skills()) {
			// При активации мы создавали новый массив с умениями. Его
			// можно смело удалять.
			set_skills(obj_proto[get_rnum()]->get_skills());
		}

		return __act.get_deactmsg() + "\n" + __act.get_room_deactmsg();
	} else {
		return "\n";
	}
}

void ObjData::remove_me_from_contains_list(ObjData *&head) {
	REMOVE_FROM_LIST(this, head, [](auto list) -> auto & { return list->m_next_content; });
}

void ObjData::remove_me_from_objects_list(ObjData *&head) {
	REMOVE_FROM_LIST(this, head, [](auto list) -> auto & { return list->m_next; });
}

void ObjData::set_id(const long _) {
	if (_ != m_id) {
		const auto old_id = m_id;

		m_id = _;
		for (const auto &observer : m_id_change_observers) {
			observer->notify(*this, old_id);
		}
	}
}

void ObjData::set_script(Script *_) {
	m_script.reset(_);
}

void ObjData::cleanup_script() {
	m_script->cleanup();
}

void ObjData::set_unique_id(const long _) {
	if (_ != m_unique_id) {
		const auto old_uid = m_unique_id;

		m_unique_id = _;

		for (const auto &observer : m_unique_id_change_observers) {
			observer->notify(*this, old_uid);
		}
	}
}

void CObjectPrototype::toggle_skill(const uint32_t skill) {
	TOGGLE_BIT(m_sparam, skill);
}

void CObjectPrototype::toggle_val_bit(const size_t index, const Bitvector bit) {
	TOGGLE_BIT(m_vals[index], bit);
}

void CObjectPrototype::toggle_wear_flag(const Bitvector flag) {
	TOGGLE_BIT(m_wear_flags, flag);
}

void CObjectPrototype::set_skill(ESkill skill_num, int percent) {
	if (!m_skills.empty()) {
		const auto skill = m_skills.find(skill_num);
		if (skill == m_skills.end()) {
			if (percent != 0) {
				m_skills.insert(std::make_pair(skill_num, percent));
			}
		} else {
			if (percent != 0) {
				skill->second = percent;
			} else {
				m_skills.erase(skill);
			}
		}
	} else {
		if (percent != 0) {
			m_skills.clear();
			m_skills.insert(std::make_pair(skill_num, percent));
		}
	}
}

void CObjectPrototype::set_wear_flag(const EWearFlag flag) {
	SET_BIT(m_wear_flags, flag);
}

void CObjectPrototype::clear_all_affected() {
	for (size_t i = 0; i < kMaxObjAffect; i++) {
		if (m_affected[i].location != EApply::kNone) {
			m_affected[i].location = EApply::kNone;
		}
	}
}

void CObjectPrototype::clear_proto_script() {
	m_proto_script.reset(new ObjData::triggers_list_t());
}

void CObjectPrototype::zero_init() {
	m_type = kItemUndefined;
	m_aliases.clear();
	m_description.clear();
	m_short_description.clear();
	m_action_description.clear();
	m_ex_description.reset();
	m_proto_script->clear();
	m_dgscript_field.clear();
	m_max_in_world = 0;
	m_skills.clear();
	m_timer = 0;
	m_minimum_remorts = 0;
	m_cost = 0;
	m_rent_on = 0;
	m_rent_off = 0;
	for (int i = 0; i < 6; i++) {
		m_pnames[i].clear();
	}
	m_ilevel = 0;
}

void CObjectPrototype::set_vnum(const ObjVnum vnum) {
	if (vnum != m_vnum) {
		const auto old_vnum = m_vnum;

		m_vnum = vnum;

		for (const auto &observer : m_vnum_change_observers) {
			observer->notify(*this, old_vnum);
		}
	}
}

void CObjectPrototype::tag_ex_description(const char *tag) {
	m_ex_description->description = str_add(m_ex_description->description, tag);
}

CObjectPrototype &CObjectPrototype::operator=(const CObjectPrototype &from) {
	MakeShallowCopy(from);
	return *this;
}

void CObjectPrototype::DungeonProtoCopy(const CObjectPrototype &from) {
	MakeShallowCopy(from);
}

void CObjectPrototype::MakeShallowCopy(const CObjectPrototype &from) {
	if (this != &from) {
		m_type = from.m_type;
		m_weight = from.m_weight;
		m_affected = from.m_affected;
		m_aliases = from.m_aliases;
		m_description = from.m_description;
		m_short_description = from.m_short_description;
		m_action_description = from.m_action_description;
		m_ex_description = from.m_ex_description;
		*m_proto_script = *from.m_proto_script;
		m_pnames = from.m_pnames;
		m_max_in_world = from.m_max_in_world;
		m_vals = from.m_vals;
		m_values = from.m_values;
		m_in_extracted_list = from.m_in_extracted_list;
		m_destroyer = from.m_destroyer;
		m_spell = from.m_spell;
		m_level = from.m_level;
		m_sparam = from.m_sparam;
		m_maximum_durability = from.m_maximum_durability;
		m_current_durability = from.m_current_durability;
		m_material = from.m_material;
		m_sex = from.m_sex;
		m_extra_flags = from.m_extra_flags;
		m_waffect_flags = from.m_waffect_flags;
		m_anti_flags = from.m_anti_flags;
		m_no_flags = from.m_no_flags;
		m_wear_flags = from.m_wear_flags;
		m_timer = from.m_timer;
		m_skills = from.m_skills;
		m_minimum_remorts = from.m_minimum_remorts;
		m_dgscript_field = from.m_dgscript_field;
		m_cost = from.m_cost;
		m_rent_on = from.m_rent_on;
		m_rent_off = from.m_rent_off;
		m_ilevel = from.m_ilevel;
		m_rnum = from.m_rnum;
		m_parent_proto = from.m_parent_proto;
	}
}

CObjectPrototype::CObjectPrototype(const CObjectPrototype &other): CObjectPrototype(other.m_vnum) {
	MakeShallowCopy(other);
}

int CObjectPrototype::get_skill(ESkill skill_num) const {
	const auto skill = m_skills.find(skill_num);
	if (skill != m_skills.end()) {
		return skill->second;
	}
	return 0;
}

bool CObjectPrototype::has_wear_flag(const EWearFlag part) const {
	return IS_SET(m_wear_flags, to_underlying(part));
}

bool CObjectPrototype::get_wear_mask(const wear_flags_t part) const {
	return IS_SET(m_wear_flags, part);
}

// * @warning Предполагается, что __out_skills.empty() == true.
void CObjectPrototype::get_skills(skills_t &out_skills) const {
	if (!m_skills.empty()) {
		out_skills.insert(m_skills.begin(), m_skills.end());
	}
}

bool CObjectPrototype::has_skills() const {
	return !m_skills.empty();
}

void CObjectPrototype::set_timer(int timer) {
	m_timer = std::max(0, timer);
}

int CObjectPrototype::get_timer() const {
	return m_timer;
}

//заколдование предмета
void ObjData::set_enchant(int skill) {
	int i = 0;

	for (i = 0; i < kMaxObjAffect; i++) {
		if (get_affected(i).location != EApply::kNone) {
			set_affected_location(i, EApply::kNone);
		}
	}

	set_affected_location(0, EApply::kHitroll);
	set_affected_location(1, EApply::kDamroll);

	if (skill <= 100)
		// 4 мортов (скил магия света 100)
	{
		set_affected_modifier(0, 1 + number(0, 1));
		set_affected_modifier(1, 1 + number(0, 1));
	} else if (skill <= 125)
		// 8 мортов (скил магия света 125)
	{
		set_affected_modifier(0, 1 + number(-3, 2));
		set_affected_modifier(1, 1 + number(-3, 2));
	} else if (skill <= 160)
		// 12 мортов (скил магия света 160)
	{
		set_affected_modifier(0, 1 + number(-4, 3));
		set_affected_modifier(1, 1 + number(-4, 3));
	} else if (skill <= 200)
		// 16 мортов (скил магия света 160+)
	{
		set_affected_modifier(0, 1 + number(-5, 5));
		set_affected_modifier(1, 1 + number(-5, 5));
	} else {
		set_affected_modifier(0, 1 + number(-4, 5));
		set_affected_modifier(1, 1 + number(-4, 5));
	}

	set_extra_flag(EObjFlag::kMagic);
	set_extra_flag(EObjFlag::kTransformed);
}

void ObjData::set_enchant(int skill, ObjData *obj) {
	const auto negative_list = make_array<EAffect>(
		EAffect::kCurse, EAffect::kSleep, EAffect::kHold,
		EAffect::kSilence, EAffect::kCrying, EAffect::kBlind,
		EAffect::kSlow);

	//накидываем хитрол и дамрол
	set_enchant(skill);

	int enchant_count = 0;

	// 8 мортов (скил магия света 125)
	if (skill > 100 && skill <= 125) {
		enchant_count = 1;
	}
		// 12 мортов (скил магия света 160)
	else if (skill <= 160) {
		enchant_count = 2;
	}
		// 16 мортов (скил магия света 160+)
	else {
		enchant_count = 3;
	}

	for (int i = 0; i < enchant_count; ++i) {
		if (obj->get_affected(i).location != EApply::kNone) {
			set_obj_eff(this, obj->get_affected(i).location, obj->get_affected(i).modifier);
		}
	}

	//if (number(0, 3) == 0) //25% negative affect
	{
		::set_obj_aff(this, negative_list[number(0, static_cast<int>(negative_list.size() - 1))]);
	}

	add_affect_flags(obj->get_affect_flags());
	add_extra_flags(obj->get_extra_flags());
	add_no_flags(obj->get_no_flags());
}

void ObjData::unset_enchant() {
	int i = 0;
	for (i = 0; i < kMaxObjAffect; i++) {
		if (obj_proto.at(get_rnum())->get_affected(i).location != EApply::kNone) {
			set_affected(i, obj_proto.at(get_rnum())->get_affected(i));
		} else {
			set_affected_location(i, EApply::kNone);
		}
	}
	// Возврат эфектов
	SetWeaponAffectFlags(obj_proto[get_rnum()]->get_affect_flags());
	// поскольку все обнулилось можно втыкать слоты для ковки
	if (obj_proto.at(get_rnum()).get()->has_flag(EObjFlag::kHasThreeSlots)) {
		set_extra_flag(EObjFlag::kHasThreeSlots);
	} else if (obj_proto.at(get_rnum()).get()->has_flag(EObjFlag::kHasTwoSlots)) {
		set_extra_flag(EObjFlag::kHasTwoSlots);
	} else if (obj_proto.at(get_rnum()).get()->has_flag(EObjFlag::kHasOneSlot)) {
		set_extra_flag(EObjFlag::kHasOneSlot);
	}
	unset_extraflag(EObjFlag::kMagic);
	unset_extraflag(EObjFlag::kTransformed);
}

bool ObjData::clone_olc_object_from_prototype(const ObjVnum vnum) {
	const auto rnum = GetObjRnum(vnum);

	if (rnum < 0) {
		return false;
	}

	const auto obj_original = world_objects.create_from_prototype_by_rnum(rnum);
	const auto old_rnum = get_rnum();
	//const auto old_rnum = rnum;

	copy_from(obj_original.get());

	const auto proto_script_copy = ObjData::triggers_list_t(obj_proto.proto_script(rnum));
	set_proto_script(proto_script_copy);

	set_rnum(old_rnum);

	ExtractObjFromWorld(obj_original.get());

	return true;
}

//копирование имени
void ObjData::copy_name_from(const CObjectPrototype *src) {
	int i;

	//Копируем псевдонимы и дескрипшены
	set_aliases(!src->get_aliases().empty() ? src->get_aliases().c_str() : "нет");
	set_short_description(!src->get_short_description().empty() ? src->get_short_description().c_str()
																: "неопределено");
	set_description(!src->get_description().empty() ? src->get_description().c_str() : "неопределено");

	//Копируем имя по падежам
	for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
		auto name_case = static_cast<ECase>(i);
		set_PName(name_case, src->get_PName(name_case));
	}
}

void ObjData::copy_from(const CObjectPrototype *src) {
	// Копирую все поверх
	*this = *src;

	// Теперь нужно выделить собственные области памяти

	// Имена и названия
	set_aliases(!src->get_aliases().empty() ? src->get_aliases().c_str() : "нет");
	set_short_description(!src->get_short_description().empty() ? src->get_short_description().c_str()
																: "неопределено");
	set_description(!src->get_description().empty() ? src->get_description().c_str() : "неопределено");

	// Дополнительные описания, если есть
	{
		ExtraDescription::shared_ptr nd;
		auto *pddd = &nd;
		auto sdd = src->get_ex_description();
		while (sdd) {
			pddd->reset(new ExtraDescription());
			(*pddd)->keyword = str_dup(sdd->keyword);
			(*pddd)->description = str_dup(sdd->description);
			pddd = &(*pddd)->next;
			sdd = sdd->next;
		}

		if (nd) {
			set_ex_description(nd);
		}
	}
}

void ObjData::swap(ObjData &object, bool swap_trig) {
	if (this == &object) {
		return;
	}

	ObjData tmpobj(object);
	tmpobj.set_script(object.get_script());
	object = *this;
	*this = tmpobj;

	ObjVnum vnum = get_vnum();
	set_vnum(object.get_vnum());
	object.set_vnum(vnum);

	set_in_room(object.get_in_room());
	object.set_in_room(tmpobj.get_in_room());
	set_unique_id(object.get_unique_id());
	object.set_unique_id(tmpobj.get_unique_id());
	set_carried_by(object.get_carried_by());
	object.set_carried_by(tmpobj.get_carried_by());
	set_worn_by(object.get_worn_by());
	object.set_worn_by(tmpobj.get_worn_by());
	set_worn_on(object.get_worn_on());
	object.set_worn_on(tmpobj.get_worn_on());
	set_in_obj(object.get_in_obj());
	object.set_in_obj(tmpobj.get_in_obj());
	set_timer(object.get_timer());
	object.set_timer(tmpobj.get_timer());
	set_contains(object.get_contains());
	object.set_contains(tmpobj.get_contains());
	world_objects.swap_id(get_id(), object.get_id());
	if (swap_trig) {
		set_script(object.get_script());
		object.set_script(tmpobj.get_script());
	}
	set_next_content(object.get_next_content());
	object.set_next_content(tmpobj.get_next_content());
	set_next(object.get_next());
	object.set_next(tmpobj.get_next());
	// для name_list
	set_serial_num(object.get_serial_num());
	object.set_serial_num(tmpobj.get_serial_num());
	//копируем также инфу о зоне, вообще мне не совсем понятна замута с этой инфой об оригинальной зоне
	// см ZONE_DECAY (c) Стрибог
	set_vnum_zone_from((&object)->get_vnum_zone_from());
	object.set_vnum_zone_from((&tmpobj)->get_vnum_zone_from());
}

void ObjData::set_tag(const char *tag) {
	if (!get_ex_description()) {
		set_ex_description(get_aliases().c_str(), tag);
	} else {
		// По уму тут надо бы стереть старое описапние если оно не с прототипа
		detach_ex_description();
		tag_ex_description(tag);
	}
}

void ObjData::attach_triggers(const triggers_list_t &trigs) {
	for (auto it = trigs.begin(); it != trigs.end(); ++it) {
		int rnum = GetTriggerRnum(*it);
		if (rnum != -1) {
			auto trig = read_trigger(rnum);
			if (!add_trigger(get_script().get(), trig, -1)) {
				ExtractTrigger(trig);
			}
		}
	}
}

/**
* Реальное старение шмотки (без всяких технических сетов таймера по коду).
* Помимо таймера самой шмотки снимается таймер ее временного обкаста.
* \param time по дефолту 1.
*/
void ObjData::dec_timer(int time, bool ignore_utimer, bool exchange) {
	*buf2 = '\0';
	if (!m_timed_spell.empty()) {
		m_timed_spell.dec_timer(this, time);
	}
	if (!ignore_utimer && stable_objs::IsTimerUnlimited(this)) {
		return;
	}
	std::stringstream buffer;

	if (get_timer() > 100000 && (this->get_type() == EObjType::kArmor
		|| this->get_type() == EObjType::kStaff
		|| this->get_type() == EObjType::kWorm
		|| this->get_type() == EObjType::kWeapon)) {
		buffer << "У предмета [" << GET_OBJ_VNUM(this)
			   << "] имя: " << this->get_PName(ECase::kNom).c_str() << ", id: " << get_id() << ", таймер > 100к равен: "
			   << get_timer();
		if (get_in_room() != kNowhere) {
			buffer << ", находится в комнате vnum: " << world[get_in_room()]->vnum;
		} else if (get_carried_by()) {
			buffer << ", затарено: " << GET_NAME(get_carried_by()) << "["
				   << GET_MOB_VNUM(get_carried_by()) << "] в комнате: [" << world[this->get_carried_by()->in_room]->vnum
				   << "]";
		} else if (get_worn_by()) {
			buffer << ", надет на перс: " << GET_NAME(get_worn_by()) << "[" << GET_MOB_VNUM(get_worn_by())
				   << "] в комнате: ["
				   << world[get_worn_by()->in_room]->vnum << "]";
		} else if (get_in_obj()) {
			buffer << ", находится в сумке: " << get_in_obj()->get_PName(ECase::kNom) << " в комнате: ["
				   << world[get_in_obj()->get_in_room()]->vnum << "]";
		}
		mudlog(buffer.str(), BRF, kLvlGod, SYSLOG, true);
	}
	if (time > 0) {
		set_timer(get_timer() - time);
	}
	if (!exchange) {
		if (((this->get_type() == EObjType::kLiquidContainer)
			|| (this->get_type() == EObjType::kFood))
			&& GET_OBJ_VAL(this, 3) > 1) //таймер у жижек и еды
		{
			dec_val(3);
		}
	}
}

ObjRnum CObjectPrototype::get_parent_vnum() {
	return obj_proto[this->get_parent_rnum()]->get_vnum();
}

double CObjectPrototype::show_mort_req() const {
	return CalcRemortRequirements(this);
}

double CObjectPrototype::show_koef_obj() const {
	return stable_objs::CountUnlimitedTimer(this);
}

double CObjectPrototype::get_ilevel() const {
	return m_ilevel;
}

void CObjectPrototype::set_ilevel(double ilvl) {
	m_ilevel = ilvl;
}

void CObjectPrototype::set_rnum(const ObjRnum _) {
	if (_ != m_rnum) {
		const auto old_rnum = m_rnum;

		m_rnum = _;

		for (const auto &observer : m_rnum_change_observers) {
			observer->notify(*this, old_rnum);
		}
	}
}

void CObjectPrototype::set_extracted_list(bool _) {
		m_in_extracted_list = _;
}

std::string CObjectPrototype::item_count_message(int num, ECase name_case) {
	if (num <= 0
		|| name_case < ECase::kFirstCase
		|| name_case > ECase::kLastCase
		|| get_PName(name_case).empty()) {
		log("SYSERROR : num=%d, pad=%d, pname=%s (%s:%d)", num, name_case, get_PName(name_case).c_str(), __FILE__, __LINE__);

		return "<ERROR>";
	}

	std::stringstream out;
	out << get_PName(name_case);
	if (num > 1) {
		out << " (x " << num << " " << GetDeclensionInNumber(num, EWhat::kThingA) << ")";
	}
	return out.str();
}

int CObjectPrototype::get_auto_mort_req() const {
	if (get_minimum_remorts() == -1)
		return 0;
	if (get_minimum_remorts() != 0) {
		return get_minimum_remorts();
	} else if (m_ilevel > 35) {
		return 12;
	} else if (m_ilevel > 33) {
		return 11;
	} else if (m_ilevel > 30) {
		return 9;
	} else if (m_ilevel > 25) {
		return 6;
	} else if (m_ilevel > 20) {
		return 3;
	}

	return 0;
}

void CObjectPrototype::set_cost(int x) {
	if (x >= 0) {
		m_cost = x;
	}
}

void CObjectPrototype::set_rent_off(int x) {
	if (x >= 0) {
		m_rent_off = x;
	}
}

void CObjectPrototype::set_rent_on(int x) {
	if (x >= 0) {
		m_rent_on = x;
	}
}

void ObjData::set_activator(bool flag, int num) {
	m_activator.first = flag;
	m_activator.second = num;
}

std::pair<bool, int> ObjData::get_activator() const {
	return m_activator;
}

void ObjData::add_timed_spell(const ESpell spell_id, const int time) {
	if (spell_id < ESpell::kFirst) {
		log("SYSERROR: func: %s, spell = %d, time = %d", __func__, to_underlying(spell_id), time);
		return;
	}
	m_timed_spell.add(this, spell_id, time);
}

void ObjData::del_timed_spell(const ESpell spell_id, const bool message) {
	m_timed_spell.del(this, spell_id, message);
}

void CObjectPrototype::set_ex_description(const char *keyword, const char *description) {
	ExtraDescription::shared_ptr d(new ExtraDescription());
	d->keyword = strdup(keyword);
	d->description = strdup(description);
	m_ex_description = d;
}

void set_obj_aff(ObjData *itemobj, const EAffect bitv) {
	for (const auto &i : weapon_affect) {
		if (i.aff_bitvector == static_cast<Bitvector>(bitv)) {
			SET_OBJ_AFF(itemobj, to_underlying(i.aff_pos));
		}
	}
}

void set_obj_eff(ObjData *itemobj, const EApply type, int mod) {
	for (auto i = 0; i < kMaxObjAffect; i++) {
		if (itemobj->get_affected(i).location == type) {
			const auto current_mod = itemobj->get_affected(i).modifier;
			itemobj->set_affected(i, type, current_mod + mod);
			break;
		} else if (itemobj->get_affected(i).location == EApply::kNone) {
			itemobj->set_affected(i, type, mod);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

namespace ObjSystem {

float count_affect_weight(const CObjectPrototype * /*obj*/, int num, int mod) {
	float weight = 0;

	switch (num) {
		case EApply::kStr: weight = mod * 7.5;
			break;
		case EApply::kDex: weight = mod * 10.0;
			break;
		case EApply::kInt: weight = mod * 10.0;
			break;
		case EApply::kWis: weight = mod * 10.0;
			break;
		case EApply::kCon: weight = mod * 10.0;
			break;
		case EApply::kCha: weight = mod * 10.0;
			break;
		case EApply::kHp: weight = mod * 0.3;
			break;
		case EApply::kAc: weight = mod * -0.5;
			break;
		case EApply::kHitroll: weight = mod * 2.3;
			break;
		case EApply::kDamroll: weight = mod * 3.3;
			break;
		case EApply::kSavingWill: weight = mod * -0.5;
			break;
		case EApply::kSavingCritical: weight = mod * -0.5;
			break;
		case EApply::kSavingStability: weight = mod * -0.5;
			break;
		case EApply::kSavingReflex: weight = mod * -0.5;
			break;
		case EApply::kCastSuccess: weight = mod * 1.5;
			break;
		case EApply::kManaRegen: weight = mod * 0.2;
			break;
		case EApply::kMorale: weight = mod * 1.0;
			break;
		case EApply::kInitiative: weight = mod * 2.0;
			break;
		case EApply::kAbsorbe: weight = mod * 1.0;
			break;
		case EApply::kAffectResist: weight = mod * 1.5;
			break;
		case EApply::kMagicResist: weight = mod * 1.5;
			break;
		default: break;
	}

	return weight;
}

bool is_armor_type(const CObjectPrototype *obj) {
	switch (obj->get_type()) {
		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor: return true;

		default: return false;
	}
}

bool is_mob_item(const CObjectPrototype *obj) {
	if (obj->has_no_flag(ENoFlag::kMale)
		&& obj->has_no_flag(ENoFlag::kFemale)
		&& obj->has_no_flag(ENoFlag::kCharmice)) {
		return true;
	}
	if (obj->has_no_flag(ENoFlag::kMono)
		&& obj->has_no_flag(ENoFlag::kPoly)
		&& obj->has_no_flag(ENoFlag::kCharmice)) {
		return true;
	}
	if (obj->has_no_flag(ENoFlag::kCharmice)) {
		return true;
	}
	if (obj->has_no_flag(ENoFlag::kSorcerer)
		&& obj->has_no_flag(ENoFlag::kThief)
		&& obj->has_no_flag(ENoFlag::kWarrior)
		&& obj->has_no_flag(ENoFlag::kAssasine)
		&& obj->has_no_flag(ENoFlag::kGuard)
		&& obj->has_no_flag(ENoFlag::kPaladine)
		&& obj->has_no_flag(ENoFlag::kRanger)
		&& obj->has_no_flag(ENoFlag::kVigilant)
		&& obj->has_no_flag(ENoFlag::kMerchant)
		&& obj->has_no_flag(ENoFlag::kMagus)
		&& obj->has_no_flag(ENoFlag::kConjurer)
		&& obj->has_no_flag(ENoFlag::kCharmer)
		&& obj->has_no_flag(ENoFlag::kWIzard)
		&& obj->has_no_flag(ENoFlag::kNecromancer)
		&& obj->has_no_flag(ENoFlag::kCharmice)) {
		return true;
	}
	if (obj->has_no_flag(ENoFlag::kCharmice)) {
		return true;
	}
	if (obj->has_anti_flag(EAntiFlag::kMale)
		&& obj->has_anti_flag(EAntiFlag::kFemale)
		&& obj->has_anti_flag(EAntiFlag::kCharmice)) {
		return true;
	}
	if (obj->has_anti_flag(EAntiFlag::kMono)
		&& obj->has_anti_flag(EAntiFlag::kPoly)
		&& obj->has_anti_flag(EAntiFlag::kCharmice)) {
		return true;
	}

	if (obj->has_anti_flag(EAntiFlag::kSorcerer)
		&& obj->has_anti_flag(EAntiFlag::kThief)
		&& obj->has_anti_flag(EAntiFlag::kWarrior)
		&& obj->has_anti_flag(EAntiFlag::kAssasine)
		&& obj->has_anti_flag(EAntiFlag::kGuard)
		&& obj->has_anti_flag(EAntiFlag::kPaladine)
		&& obj->has_anti_flag(EAntiFlag::kRanger)
		&& obj->has_anti_flag(EAntiFlag::kVigilant)
		&& obj->has_anti_flag(EAntiFlag::kMerchant)
		&& obj->has_anti_flag(EAntiFlag::kMagus)
		&& obj->has_anti_flag(EAntiFlag::kConjurer)
		&& obj->has_anti_flag(EAntiFlag::kCharmer)
		&& obj->has_anti_flag(EAntiFlag::kWizard)
		&& obj->has_anti_flag(EAntiFlag::kNecromancer)
		&& obj->has_anti_flag(EAntiFlag::kCharmice)) {
		return true;
	}

	return false;
}

void init_ilvl(CObjectPrototype *obj) {
	if (is_mob_item(obj)
		|| obj->has_flag(EObjFlag::KSetItem)
		|| obj->get_minimum_remorts() > 0) {
		obj->set_ilevel(0);
		return;
	}

	auto total_weight = CalcRemortRequirements(obj);

	obj->set_ilevel(total_weight);
}

void init_item_levels() {
	for (const auto &i : obj_proto) {
		init_ilvl(i.get());
	}
}

} // namespace ObjSystem

namespace system_obj {
/// кошелек для кун с игрока
const int PURSE_VNUM = 1924;
int PURSE_RNUM = -1;
/// персональное хранилище
const int PERS_CHEST_VNUM = 331;
int PERS_CHEST_RNUM = -1;
TelegramBot *bot = new TelegramBot();

/// при старте сразу после лоада зон
void init() {
	PURSE_RNUM = GetObjRnum(PURSE_VNUM);
	PERS_CHEST_RNUM = GetObjRnum(PERS_CHEST_VNUM);
}

ObjData *create_purse(CharData *ch, int/* gold*/) {
	const auto obj = world_objects.create_from_prototype_by_rnum(PURSE_RNUM);
	if (!obj) {
		return obj.get();
	}

	obj->set_aliases("тугой кошелек");
	obj->set_short_description("тугой кошелек");
	obj->set_description("Кем-то оброненный тугой кошелек лежит здесь.");
	obj->set_PName(ECase::kNom, "тугой кошелек");
	obj->set_PName(ECase::kGen, "тугого кошелька");
	obj->set_PName(ECase::kDat, "тугому кошельку");
	obj->set_PName(ECase::kAcc, "тугой кошелек");
	obj->set_PName(ECase::kIns, "тугим кошельком");
	obj->set_PName(ECase::kPre, "тугом кошельке");

	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_),
			 "--------------------------------------------------\r\n"
			 "Владелец: %s\r\n"
			 "В случае потери просьба вернуть за вознаграждение.\r\n"
			 "--------------------------------------------------\r\n", ch->get_name().c_str());
	obj->set_ex_description(obj->get_PName(ECase::kNom).c_str(), buf_);

	obj->set_type(EObjType::kContainer);
	obj->set_wear_flags(to_underlying(EWearFlag::kTake));

	obj->set_val(0, 0);
	// CLOSEABLE + CLOSED
	obj->set_val(1, 5);
	obj->set_val(2, -1);
	obj->set_val(3, ch->get_uid());

	obj->set_rent_off(0);
	obj->set_rent_on(0);
	// чтобы скавенж мобов не трогать
	obj->set_cost(2);
	obj->set_extra_flag(EObjFlag::kNodonate);
	obj->set_extra_flag(EObjFlag::kNosell);

	return obj.get();
}

bool is_purse(ObjData *obj) {
	return obj->get_rnum() == PURSE_RNUM;
}

/// вываливаем и пуржим кошелек при попытке открыть или при взятии хозяином
void process_open_purse(CharData *ch, ObjData *obj) {
	auto value = obj->get_val(1);
	REMOVE_BIT(value, EContainerFlag::kShutted);
	obj->set_val(1, value);

	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_), "all");
	get_from_container(ch, obj, buf_, EFind::kObjInventory, 1, false);
	act("$o рассыпал$U в ваших руках...", false, ch, obj, 0, kToChar);
	ExtractObjFromWorld(obj);
}

} // namespace system_obj

int ObjVal::get(const EValueKey key) const {
	auto i = m_values.find(key);
	if (i != m_values.end()) {
		return i->second;
	}
	return -1;
}

void ObjVal::set(const EValueKey key, int val) {
	if (val >= 0) {
		m_values[key] = val;
	} else {
		auto i = m_values.find(key);
		if (i != m_values.end()) {
			m_values.erase(i);
		}
	}
}

void ObjVal::inc(const ObjVal::EValueKey key, int val) {
	auto i = m_values.find(key);
	if (i == m_values.end() || val == 0) return;

	if (val < 0 && i->second + val <= 0) {
		i->second = 0;
		return;
	} else if (val >= std::numeric_limits<int>::max() - i->second) {
		i->second = std::numeric_limits<int>::max();
		return;
	}

	i->second += val;
}

std::string ObjVal::print_to_file() const {
	if (m_values.empty()) return "";

	std::stringstream out;
	out << "Vals:\n";

	for (auto m_value : m_values) {
		std::string key_str = text_id::ToStr(text_id::kObjVals, to_underlying(m_value.first));
		if (!key_str.empty()) {
			out << key_str << " " << m_value.second << "\n";
		}
	}
	out << "~\n";

	return out.str();
}

bool ObjVal::init_from_file(const char *str) {
	m_values.clear();
	std::stringstream text(str);
	std::string key_str;
	bool result = true;
	int val = -1;

	while (text >> key_str >> val) {
		const int key = text_id::ToNum(text_id::kObjVals, key_str);
		if (key >= 0 && val >= 0) {
			m_values.emplace(static_cast<EValueKey>(key), val);
			key_str.clear();
			val = -1;
		} else {
			err_log("invalid key=%d or val=%d (%s %s:%d)",
					key, val, __func__, __FILE__, __LINE__);
		}
	}

	return result;
}

std::string ObjVal::print_to_zone() const {
	if (m_values.empty()) {
		return "";
	}

	std::stringstream out;

	//sort before output
	std::vector<std::pair<std::string, int>> m_val_vec;

	for (auto const i : m_values) {
		std::string key_str = text_id::ToStr(text_id::kObjVals, to_underlying(i.first));
		if (!key_str.empty()) {
			m_val_vec.emplace_back(key_str, i.second);
		}
	}

	std::sort(m_val_vec.begin(), m_val_vec.end(),
			  [](std::pair<std::string, int> &a, std::pair<std::string, int> &b) {
				return a.first < b.first;
			  });

	for (auto const &i : m_val_vec) {
		out << "V " << i.first << " " << i.second << "\n";
	}

	return out.str();
}

void ObjVal::init_from_zone(const char *str) {
	std::stringstream text(str);
	std::string key_str;
	int val = -1;

	if (text >> key_str >> val) {
		const int key = text_id::ToNum(text_id::kObjVals, key_str);
		if (key >= 0 && val >= 0) {
			m_values.emplace(static_cast<EValueKey>(key), val);
		} else {
			err_log("invalid key=%d or val=%d (%s %s:%d)",
					key, val, __func__, __FILE__, __LINE__);
		}
	}
}

bool is_valid_drinkcon(const ObjVal::EValueKey key) {
	switch (key) {
		case ObjVal::EValueKey::POTION_SPELL1_NUM:
		case ObjVal::EValueKey::POTION_SPELL1_LVL:
		case ObjVal::EValueKey::POTION_SPELL2_NUM:
		case ObjVal::EValueKey::POTION_SPELL2_LVL:
		case ObjVal::EValueKey::POTION_SPELL3_NUM:
		case ObjVal::EValueKey::POTION_SPELL3_LVL:
		case ObjVal::EValueKey::POTION_PROTO_VNUM: return true;
	}
	return false;
}

void ObjVal::remove_incorrect_keys(int type) {
	for (auto i = m_values.begin(); i != m_values.end(); /* empty */) {
		bool erased = false;
		switch (type) {
			case EObjType::kLiquidContainer:
			case EObjType::kFountain:
				if (!is_valid_drinkcon(i->first)) {
					i = m_values.erase(i);
					erased = true;
				}
				break;
			default: break;
		} // switch

		if (!erased) {
			++i;
		}
	}
}

std::string print_obj_affects(const obj_affected_type &affect) {
	sprinttype(affect.location, apply_types, buf2);
	if (buf2[0] == '*')
		memmove(buf2, buf2 + 1, strlen(buf2) - 1);
	bool negative = IsNegativeApply(affect.location);
	if (!negative && affect.modifier < 0) {
		negative = true;
	} else if (negative && affect.modifier < 0) {
		negative = false;
	}

	snprintf(buf, kMaxStringLength, "%s%s%s%s%s%d%s\r\n",
			 kColorCyn, buf2, kColorNrm,
			 kColorCyn, (negative ? " ухудшает на " : " улучшает на "),
			 abs(affect.modifier), kColorNrm);

	return std::string(buf);
}

void print_obj_affects(CharData *ch, const obj_affected_type &affect) {
	sprinttype(affect.location, apply_types, buf2);
	if (buf2[0] == '*')
		memmove(buf2, buf2 + 1, strlen(buf2) - 1);
	bool negative = IsNegativeApply(affect.location);
	if (!negative && affect.modifier < 0) {
		negative = true;
	} else if (negative && affect.modifier < 0) {
		negative = false;
	}
	snprintf(buf, kMaxStringLength, "   %s%s%s%s%s%d%s\r\n",
			 kColorCyn, buf2, kColorNrm,
			 kColorCyn,
			 negative ? " ухудшает на " : " улучшает на ", abs(affect.modifier), kColorNrm);
	SendMsgToChar(buf, ch);
}

namespace SetSystem {
struct SetNode {
  // список шмоток по конкретному сету для сверки
  // инится один раз при ребуте и больше не меняется
  std::set<int> set_vnum;
  // список шмоток из данного сета у текущего чара
  // если после заполнения в списке только 1 предмет
  // значит удаляем его как единственный у чара
  std::vector<int> obj_vnum;
};

std::vector<SetNode> set_list;

constexpr unsigned BIG_SET_ITEMS = 9;
constexpr unsigned MINI_SET_ITEMS = 3;

// для проверок при попытке ренты
std::set<int> vnum_list;

// * Заполнение списка фулл-сетов для последующих сверок.
void init_set_list() {
	for (id_to_set_info_map::const_iterator i = ObjData::set_table.begin(),
			 iend = ObjData::set_table.end(); i != iend; ++i) {
		if (i->second.size() > BIG_SET_ITEMS) {
			SetNode node;
			for (set_info::const_iterator k = i->second.begin(),
					 kend = i->second.end(); k != kend; ++k) {
				node.set_vnum.insert(k->first);
			}
			set_list.push_back(node);
		}
	}
}

// * Удаление инфы от последнего сверявшегося чара.
void reset_set_list() {
	for (auto &i : set_list) {
		i.obj_vnum.clear();
	}
}

// * Проверка шмотки на принадлежность к сетам из set_list.
void check_item(int vnum) {
	for (auto &i : set_list) {
		auto k = i.set_vnum.find(vnum);
		if (k != i.set_vnum.end()) {
			i.obj_vnum.push_back(vnum);
		}
	}
}

// * Обнуление таймера шмотки в ренте или перс.хране.
void delete_item(const std::size_t pt_num, int vnum) {
	bool need_save = false;
	// рента
	if (player_table[pt_num].timer) {
		for (std::vector<SaveTimeInfo>::iterator i = player_table[pt_num].timer->time.begin(),
				 iend = player_table[pt_num].timer->time.end(); i != iend; ++i) {
			if (i->vnum == vnum) {
				log("[TO] Player %s : set-item %d deleted", player_table[pt_num].name().c_str(), i->vnum);
				i->timer = -1;
				int rnum = GetObjRnum(i->vnum);
				if (rnum >= 0) {
					obj_proto.dec_stored(rnum);
				}
				need_save = true;
			}
		}
	}
	if (need_save) {
		if (!Crash_write_timer(pt_num)) {
			log("SYSERROR: [TO] Error writing timer file for %s", player_table[pt_num].name().c_str());
		}
		return;
	}
	// перс.хран
	Depot::delete_set_item(player_table[pt_num].uid(), vnum);
}

// * Проверка при ребуте всех рент и перс.хранилищ чаров.
void check_rented() {
	init_set_list();

	for (std::size_t i = 0; i < player_table.size(); i++) {
		reset_set_list();
		// рента
		if (player_table[i].timer) {
			for (std::vector<SaveTimeInfo>::iterator it = player_table[i].timer->time.begin(),
					 it_end = player_table[i].timer->time.end(); it != it_end; ++it) {
				if (it->timer >= 0) {
					check_item(it->vnum);
				}
			}
		}
		// перс.хран
		Depot::check_rented(player_table[i].uid());
		// проверка итогового списка
		for (std::vector<SetNode>::iterator it = set_list.begin(),
				 iend = set_list.end(); it != iend; ++it) {
			if (it->obj_vnum.size() == 1) {
				delete_item(i, it->obj_vnum[0]);
			}
		}
	}
}

/**
* Почта, базар.
* Предметы сетов из BIG_SET_ITEMS и более предметов не принимаются.
*/
bool is_big_set(const CObjectPrototype *obj, bool is_mini) {
	unsigned int sets_items = is_mini ? MINI_SET_ITEMS : BIG_SET_ITEMS;
	if (!obj->has_flag(EObjFlag::KSetItem)) {
		return false;
	}
	for (id_to_set_info_map::const_iterator i = ObjData::set_table.begin(),
			 iend = ObjData::set_table.end(); i != iend; ++i) {
		if (i->second.find(GET_OBJ_VNUM(obj)) != i->second.end()
			&& i->second.size() > sets_items) {
			return true;
		}
	}
	return false;
}

bool find_set_item(ObjData *obj) {
	for (; obj; obj = obj->get_next_content()) {
		std::set<int>::const_iterator i = vnum_list.find(obj_sets::normalize_vnum(GET_OBJ_VNUM(obj)));
		if (i != vnum_list.end()) {
			return true;
		}
		if (find_set_item(obj->get_contains())) {
			return true;
		}
	}
	return false;
}

// * Генерация списка сетин из того же набора, что и vnum (исключая ее саму).
void init_vnum_list(int vnum) {
	vnum_list.clear();
	for (id_to_set_info_map::const_iterator i = ObjData::set_table.begin(),
			 iend = ObjData::set_table.end(); i != iend; ++i) {
		if (i->second.find(vnum) != i->second.end())
			//&& i->second.size() > BIG_SET_ITEMS)
		{
			for (set_info::const_iterator k = i->second.begin(),
					 kend = i->second.end(); k != kend; ++k) {
				if (k->first != vnum) {
					vnum_list.insert(k->first);
				}
			}
		}
	}

	if (vnum_list.empty()) {
		vnum_list = obj_sets::vnum_list_add(vnum);
	}
}

/* проверяем сетину в массиве внумоB*/
bool is_norent_set(int vnum, std::vector<int> objs) {
	if (objs.empty())
		return true;
	// нормализуем внумы
	vnum = obj_sets::normalize_vnum(vnum);
	for (unsigned int i = 0; i < objs.size(); i++) {
		objs[i] = obj_sets::normalize_vnum(objs[i]);
	}
	init_vnum_list(obj_sets::normalize_vnum(vnum));
	for (const auto &it : objs) {
		for (const auto &it1 : vnum_list)
			if (it == it1)
				return false;
	}
	return true;
}

// * Поиск в хране из списка vnum_list.
bool house_find_set_item(CharData *ch, const std::set<int> &vnum_list) {
	// храны у нас через задницу сделаны
	for (ObjData *chest = world[GetRoomRnum(CLAN(ch)->get_chest_room())]->contents; chest;
		 chest = chest->get_next_content()) {
		if (Clan::is_clan_chest(chest)) {
			for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
				if (vnum_list.find(obj_sets::normalize_vnum(temp->get_vnum())) != vnum_list.end()) {
					return true;
				}
			}
		}
	}
	return false;
}

/**
* Экипировка, инвентарь, чармисы, перс. хран.см
* Требуется наличие двух и более предметов, если сетина из большого сета.
* Перс. хран, рента.
*/
bool is_norent_set(CharData *ch, ObjData *obj, bool clan_chest) {
	if (!obj->has_flag(EObjFlag::KSetItem)) {
		return false;
	}

	init_vnum_list(obj_sets::normalize_vnum(GET_OBJ_VNUM(obj)));

	if (vnum_list.empty()) {
		return false;
	}

	// экипировка
	for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		if (find_set_item(GET_EQ(ch, i))) {
			return false;
		}
	}
	// инвентарь
	if (find_set_item(ch->carrying)) {
		return false;
	}

	// чармисы
	if (ch->followers) {
		for (struct FollowerType *k = ch->followers; k; k = k->next) {
			if (!IS_CHARMICE(k->follower)
				|| !k->follower->has_master()) {
				continue;
			}

			for (int j = 0; j < EEquipPos::kNumEquipPos; j++) {
				if (find_set_item(GET_EQ(k->follower, j))) {
					return false;
				}
			}

			if (find_set_item(k->follower->carrying)) {
				return false;
			}
		}
	}

	if (!clan_chest) {
		// перс. хранилище
		if (Depot::find_set_item(ch, vnum_list)) {
			return false;
		}
	} else {
		if (house_find_set_item(ch, vnum_list)) {
			return false;
		}
	}
	return true;
}

} // namespace SetSystem

int GetObjMIW(ObjRnum rnum) {
	if (rnum < 0)
		return 0;
	return obj_proto[rnum]->get_max_in_world();
}

double CalcRemortRequirements(const CObjectPrototype *obj) {
	const float SQRT_MOD = 1.7095f;
	const int AFF_SHIELD_MOD = 30;
	const int AFF_MAGICGLASS_MOD = 10;
	const int AFF_BLINK_MOD = 10;
	const int AFF_CLOUDLY_MOD = 10;

	double result{0.0};
	if (ObjSystem::is_mob_item(obj) || obj->has_flag(EObjFlag::KSetItem)) {
		return result;
	}

	double total_weight{0.0};
	// аффекты APPLY_x
	for (int k = 0; k < kMaxObjAffect; k++) {
		if (obj->get_affected(k).location == 0) continue;

		// случай, если один аффект прописан в нескольких полях
		for (int kk = 0; kk < kMaxObjAffect; kk++) {
			if (obj->get_affected(k).location == obj->get_affected(kk).location
				&& k != kk) {
				log("SYSERROR: double affect=%d, ObjVnum=%d",
					obj->get_affected(k).location, GET_OBJ_VNUM(obj));
				return 1000000;
			}
		}
		if ((obj->get_affected(k).modifier > 0) && ((obj->get_affected(k).location != EApply::kAc) &&
			(obj->get_affected(k).location != EApply::kSavingWill) &&
			(obj->get_affected(k).location != EApply::kSavingCritical) &&
			(obj->get_affected(k).location != EApply::kSavingStability) &&
			(obj->get_affected(k).location != EApply::kSavingReflex))) {
			auto weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
			total_weight += pow(weight, SQRT_MOD);
		}
			// савесы которые с минусом должны тогда понижать вес если в +
		else if ((obj->get_affected(k).modifier > 0) && ((obj->get_affected(k).location == EApply::kAc) ||
			(obj->get_affected(k).location == EApply::kSavingWill) ||
			(obj->get_affected(k).location == EApply::kSavingCritical) ||
			(obj->get_affected(k).location == EApply::kSavingStability) ||
			(obj->get_affected(k).location == EApply::kSavingReflex))) {
			auto weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, 0 - obj->get_affected(k).modifier);
			total_weight -= pow(weight, -SQRT_MOD);
		}
			//Добавленый кусок учет савесов с - значениями
		else if ((obj->get_affected(k).modifier < 0)
			&& ((obj->get_affected(k).location == EApply::kAc) ||
				(obj->get_affected(k).location == EApply::kSavingWill) ||
				(obj->get_affected(k).location == EApply::kSavingCritical) ||
				(obj->get_affected(k).location == EApply::kSavingStability) ||
				(obj->get_affected(k).location == EApply::kSavingReflex))) {
			auto weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
			total_weight += pow(weight, SQRT_MOD);
		}
			//Добавленый кусок учет отрицательного значения но не савесов
		else if ((obj->get_affected(k).modifier < 0)
			&& ((obj->get_affected(k).location != EApply::kAc) &&
				(obj->get_affected(k).location != EApply::kSavingWill) &&
				(obj->get_affected(k).location != EApply::kSavingCritical) &&
				(obj->get_affected(k).location != EApply::kSavingStability) &&
				(obj->get_affected(k).location != EApply::kSavingReflex))) {
			auto weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, 0 - obj->get_affected(k).modifier);
			total_weight -= pow(weight, -SQRT_MOD);
		}
	}
	// аффекты AFF_x через weapon_affect
	for (const auto &m : weapon_affect) {
		if (obj->GetEWeaponAffect(m.aff_pos)) {
			auto obj_affects = static_cast<EAffect>(m.aff_bitvector);
			if (obj_affects == EAffect::kAirShield ||
				obj_affects == EAffect::kFireShield ||
				obj_affects == EAffect::kIceShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (obj_affects == EAffect::kMagicGlass) {
				total_weight += pow(AFF_MAGICGLASS_MOD, SQRT_MOD);
			} else if (obj_affects == EAffect::kBlink) {
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			} else if (obj_affects == EAffect::kCloudly) {
				total_weight += pow(AFF_CLOUDLY_MOD, SQRT_MOD);
			}
		}
	}

	if (total_weight < 1) {
		return result;
	} else {
		return ceil(pow(total_weight, 1 / SQRT_MOD));
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
