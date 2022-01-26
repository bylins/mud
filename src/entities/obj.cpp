// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "obj.h"

#include "obj_save.h"
#include "world_objects.h"
#include "obj_prototypes.h"
#include "parse.h"
#include "handler.h"
#include "screen.h"
#include "game_mechanics/celebrates.h"
#include "fightsystem/pk.h"
#include "cache.h"
#include "depot.h"
#include "house.h"

extern void get_from_container(CharacterData *ch, ObjectData *cont, char *arg, int mode, int amount, bool autoloot);
void set_obj_eff(ObjectData *itemobj, EApplyLocation type, int mod);
void set_obj_aff(ObjectData *itemobj, EAffectFlag bitv);
extern void extract_trigger(Trigger *trig);

id_to_set_info_map ObjectData::set_table;

ObjectData::ObjectData(const ObjVnum vnum) :
	CObjectPrototype(vnum),
	m_uid(0),
	m_in_room(0),
	m_room_was_in(0),
	m_maker(DEFAULT_MAKER),
	m_owner(DEFAULT_OWNER),
	m_zone_from(0),
	m_parent(DEFAULT_PARENT),
	m_is_rename(false),
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
	this->zero_init();
	caching::obj_cache.add(this);
}

ObjectData::ObjectData(const CObjectPrototype &other) :
	CObjectPrototype(other),
	m_uid(0),
	m_in_room(0),
	m_room_was_in(0),
	m_maker(DEFAULT_MAKER),
	m_owner(DEFAULT_OWNER),
	m_zone_from(0),
	m_parent(DEFAULT_PARENT),
	m_is_rename(false),
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
	caching::obj_cache.add(this);
}

ObjectData::ObjectData(const ObjectData &other) : CObjectPrototype(other.get_vnum()) {
	*this = other;

	m_script.reset(new Script(*other.m_script));    // each object must have its own script. Just copy it

	caching::obj_cache.add(this);
}

ObjectData::~ObjectData() {
	this->purge();
}

// * См. Character::zero_init()
void ObjectData::zero_init() {
	CObjectPrototype::zero_init();
	set_weight(0);
	m_uid = 0;
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

void ObjectData::detach_ex_description() {
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
void ObjectData::purge() {
	caching::obj_cache.remove(this);
	//см. комментарий в структуре BloodyInfo из pk.cpp
	bloody::remove_obj(this);
	//weak_ptr тут бы был какраз в тему
	Celebrates::remove_from_obj_lists(this->get_uid());
}

int ObjectData::get_serial_num() {
	return m_serial_number;
}

void ObjectData::set_serial_num(int num) {
	m_serial_number = num;
}

const std::string ObjectData::activate_obj(const activation &__act) {
	if (get_rnum() >= 0) {
		set_affect_flags(__act.get_affects());
		for (int i = 0; i < kMaxObjAffect; i++) {
			set_affected(i, __act.get_affected_i(i));
		}

		int weight = __act.get_weight();
		if (weight > 0) {
			set_weight(weight);
		}

		if (get_type() == ITEM_WEAPON) {
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

const std::string ObjectData::deactivate_obj(const activation &__act) {
	if (get_rnum() >= 0) {
		set_affect_flags(obj_proto[get_rnum()]->get_affect_flags());
		for (int i = 0; i < kMaxObjAffect; i++) {
			set_affected(i, obj_proto[get_rnum()]->get_affected(i));
		}

		set_weight(obj_proto[get_rnum()]->get_weight());

		if (get_type() == ITEM_WEAPON) {
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

void ObjectData::remove_me_from_contains_list(ObjectData *&head) {
	REMOVE_FROM_LIST(this, head, [](auto list) -> auto & { return list->m_next_content; });
}

void ObjectData::remove_me_from_objects_list(ObjectData *&head) {
	REMOVE_FROM_LIST(this, head, [](auto list) -> auto & { return list->m_next; });
}

void ObjectData::set_id(const long _) {
	if (_ != m_id) {
		const auto old_id = m_id;

		m_id = _;

		for (const auto &observer : m_id_change_observers) {
			observer->notify(*this, old_id);
		}
	}
}

void ObjectData::set_script(Script *_) {
	m_script.reset(_);
}

void ObjectData::cleanup_script() {
	m_script->cleanup();
}

void ObjectData::set_uid(const unsigned _) {
	if (_ != m_uid) {
		const auto old_uid = m_uid;

		m_uid = _;

		for (const auto &observer : m_uid_change_observers) {
			observer->notify(*this, old_uid);
		}
	}
}

void CObjectPrototype::toggle_skill(const uint32_t skill) {
	TOGGLE_BIT(m_skill, skill);
}

void CObjectPrototype::toggle_val_bit(const size_t index, const uint32_t bit) {
	TOGGLE_BIT(m_vals[index], bit);
}

void CObjectPrototype::toggle_wear_flag(const uint32_t flag) {
	TOGGLE_BIT(m_wear_flags, flag);
}

void CObjectPrototype::set_skill(int skill_num, int percent) {
	if (!m_skills.empty()) {
		const auto skill = m_skills.find(static_cast<ESkill>(skill_num));
		if (skill == m_skills.end()) {
			if (percent != 0) {
				m_skills.insert(std::make_pair(static_cast<ESkill>(skill_num), percent));
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
			m_skills.insert(std::make_pair(static_cast<ESkill>(skill_num), percent));
		}
	}
}

void CObjectPrototype::set_wear_flag(const EWearFlag flag) {
	SET_BIT(m_wear_flags, flag);
}

void CObjectPrototype::clear_all_affected() {
	for (size_t i = 0; i < kMaxObjAffect; i++) {
		if (m_affected[i].location != APPLY_NONE) {
			m_affected[i].location = APPLY_NONE;
		}
	}
}

void CObjectPrototype::clear_proto_script() {
	m_proto_script.reset(new ObjectData::triggers_list_t());
}

void CObjectPrototype::zero_init() {
	m_type = ITEM_UNDEFINED;
	m_aliases.clear();
	m_description.clear();
	m_short_description.clear();
	m_action_description.clear();
	m_ex_description.reset();
	m_proto_script->clear();
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
		m_destroyer = from.m_destroyer;
		m_spell = from.m_spell;
		m_level = from.m_level;
		m_skill = from.m_skill;
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
		m_cost = from.m_cost;
		m_rent_on = from.m_rent_on;
		m_rent_off = from.m_rent_off;
		m_ilevel = from.m_ilevel;
		m_rnum = from.m_rnum;
	}

	return *this;
}

int CObjectPrototype::get_skill(int skill_num) const {
	const auto skill = m_skills.find(static_cast<ESkill>(skill_num));
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
	m_timer = MAX(0, timer);
}

int CObjectPrototype::get_timer() const {
	return m_timer;
}

//заколдование предмета
void ObjectData::set_enchant(int skill) {
	int i = 0;

	for (i = 0; i < kMaxObjAffect; i++) {
		if (get_affected(i).location != APPLY_NONE) {
			set_affected_location(i, APPLY_NONE);
		}
	}

	set_affected_location(0, APPLY_HITROLL);
	set_affected_location(1, APPLY_DAMROLL);

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

	set_extra_flag(EExtraFlag::ITEM_MAGIC);
	set_extra_flag(EExtraFlag::ITEM_TRANSFORMED);
}

void ObjectData::set_enchant(int skill, ObjectData *obj) {
	const auto negative_list = make_array<EAffectFlag>(
		EAffectFlag::AFF_CURSE, EAffectFlag::AFF_SLEEP, EAffectFlag::AFF_HOLD,
		EAffectFlag::AFF_SILENCE, EAffectFlag::AFF_CRYING, EAffectFlag::AFF_BLIND,
		EAffectFlag::AFF_SLOW);

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
		if (obj->get_affected(i).location != APPLY_NONE) {
			set_obj_eff(this, obj->get_affected(i).location, obj->get_affected(i).modifier);
		}
	}

	//if (number(0, 3) == 0) //25% negative affect
	{
		::set_obj_aff(this, negative_list[number(0, static_cast<int>(negative_list.size() - 1))]);
	}

	add_affect_flags(GET_OBJ_AFFECTS(obj));
	add_extra_flags(GET_OBJ_EXTRA(obj));
	add_no_flags(GET_OBJ_NO(obj));
}

void ObjectData::unset_enchant() {
	int i = 0;
	for (i = 0; i < kMaxObjAffect; i++) {
		if (obj_proto.at(get_rnum())->get_affected(i).location != APPLY_NONE) {
			set_affected(i, obj_proto.at(get_rnum())->get_affected(i));
		} else {
			set_affected_location(i, APPLY_NONE);
		}
	}
	// Возврат эфектов
	set_affect_flags(obj_proto[get_rnum()]->get_affect_flags());
	// поскольку все обнулилось можно втыкать слоты для ковки
	if (OBJ_FLAGGED(obj_proto.at(get_rnum()).get(), EExtraFlag::ITEM_WITH3SLOTS)) {
		set_extra_flag(EExtraFlag::ITEM_WITH3SLOTS);
	} else if (OBJ_FLAGGED(obj_proto.at(get_rnum()).get(), EExtraFlag::ITEM_WITH2SLOTS)) {
		set_extra_flag(EExtraFlag::ITEM_WITH2SLOTS);
	} else if (OBJ_FLAGGED(obj_proto.at(get_rnum()).get(), EExtraFlag::ITEM_WITH1SLOT)) {
		set_extra_flag(EExtraFlag::ITEM_WITH1SLOT);
	}
	unset_extraflag(EExtraFlag::ITEM_MAGIC);
	unset_extraflag(EExtraFlag::ITEM_TRANSFORMED);
}

bool ObjectData::clone_olc_object_from_prototype(const ObjVnum vnum) {
	const auto rnum = real_object(vnum);

	if (rnum < 0) {
		return false;
	}

	const auto obj_original = world_objects.create_from_prototype_by_rnum(rnum);
	const auto old_rnum = get_rnum();
	//const auto old_rnum = rnum;

	copy_from(obj_original.get());

	const auto proto_script_copy = ObjectData::triggers_list_t(obj_proto.proto_script(rnum));
	set_proto_script(proto_script_copy);

	set_rnum(old_rnum);

	extract_obj(obj_original.get());

	return true;
}

//копирование имени
void ObjectData::copy_name_from(const CObjectPrototype *src) {
	int i;

	//Копируем псевдонимы и дескрипшены
	set_aliases(!src->get_aliases().empty() ? src->get_aliases().c_str() : "нет");
	set_short_description(!src->get_short_description().empty() ? src->get_short_description().c_str()
																: "неопределено");
	set_description(!src->get_description().empty() ? src->get_description().c_str() : "неопределено");

	//Копируем имя по падежам
	for (i = 0; i < NUM_PADS; i++)
		set_PName(i, src->get_PName(i));
}

void ObjectData::copy_from(const CObjectPrototype *src) {
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

void ObjectData::swap(ObjectData &object) {
	if (this == &object) {
		return;
	}

	ObjectData tmpobj(object);
	object = *this;
	*this = tmpobj;

	ObjVnum vnum = get_vnum();
	set_vnum(object.get_vnum());
	object.set_vnum(vnum);

	set_in_room(object.get_in_room());
	object.set_in_room(tmpobj.get_in_room());

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
	set_id(object.get_id());
	object.set_id(tmpobj.get_id());
	set_script(object.get_script());
	object.set_script(tmpobj.get_script());
	set_next_content(object.get_next_content());
	object.set_next_content(tmpobj.get_next_content());
	set_next(object.get_next());
	object.set_next(tmpobj.get_next());
	// для name_list
	set_serial_num(object.get_serial_num());
	object.set_serial_num(tmpobj.get_serial_num());
	//копируем также инфу о зоне, вообще мне не совсем понятна замута с этой инфой об оригинальной зоне
	// см ZONE_DECAY (c) Стрибог
	set_vnum_zone_from(GET_OBJ_VNUM_ZONE_FROM(&object));
	object.set_vnum_zone_from(GET_OBJ_VNUM_ZONE_FROM(&tmpobj));
}

void ObjectData::set_tag(const char *tag) {
	if (!get_ex_description()) {
		set_ex_description(get_aliases().c_str(), tag);
	} else {
		// По уму тут надо бы стереть старое описапние если оно не с прототипа
		detach_ex_description();
		tag_ex_description(tag);
	}
}

void ObjectData::attach_triggers(const triggers_list_t &trigs) {
	for (auto it = trigs.begin(); it != trigs.end(); ++it) {
		int rnum = real_trigger(*it);
		if (rnum != -1) {
			auto trig = read_trigger(rnum);
			if (!add_trigger(get_script().get(), trig, -1)) {
				extract_trigger(trig);
			}
		}
	}
}

float count_mort_requred(const CObjectPrototype *obj);
float count_unlimited_timer(const CObjectPrototype *obj);

/**
* Реальное старение шмотки (без всяких технических сетов таймера по коду).
* Помимо таймера самой шмотки снимается таймер ее временного обкаста.
* \param time по дефолту 1.
*/
void ObjectData::dec_timer(int time, bool ignore_utimer, bool exchange) {
	*buf2 = '\0';
	if (!m_timed_spell.empty()) {
		m_timed_spell.dec_timer(this, time);
	}
	if (!ignore_utimer && check_unlimited_timer(this)) {
		return;
	}
	if (get_timer()  > 100000 && (GET_OBJ_TYPE(this) == ObjectData::ITEM_ARMOR
			|| GET_OBJ_TYPE(this) == ObjectData::ITEM_STAFF
			|| GET_OBJ_TYPE(this) == ObjectData::ITEM_WORN
			|| GET_OBJ_TYPE(this) == ObjectData::ITEM_WEAPON)) {
	if (get_in_room() != kNowhere) {
		sprintf(buf2, "Находится в комнате vnum: %d", world[get_in_room()]->room_vn);
	} else if (get_carried_by()) {
		sprintf(buf2, "Затарено %s[%d] в комнате [%d]", GET_NAME(get_carried_by()),
				GET_MOB_VNUM(get_carried_by()),
				world[this->get_carried_by()->in_room]->room_vn);
	} else if (get_worn_by()) {
		sprintf(buf2, "надет на %s[%d] в комнате [%d]",
				GET_NAME(get_worn_by()),
				GET_MOB_VNUM(get_worn_by()),
				world[this->get_worn_by()->in_room]->room_vn);
	} else if (get_in_obj()) {
		sprintf(buf2, "Находится в сумке %s", GET_OBJ_PNAME(get_in_obj(), 0).c_str());
	}
		snprintf(buf, kMaxStringLength, "У предмета [%d] имя: %s, id: %ld, таймер > 100к равен %d. %s", GET_OBJ_VNUM(this), 
				GET_OBJ_PNAME(this, 0).c_str(), get_id(), get_timer(), buf2);
		mudlog(buf, BRF, kLevelGod, SYSLOG, true);
	}

	if (time > 0) {

		set_timer(get_timer() - time);
	}

	if (!exchange) {
		if (((GET_OBJ_TYPE(this) == CObjectPrototype::ITEM_DRINKCON)
			|| (GET_OBJ_TYPE(this) == CObjectPrototype::ITEM_FOOD))
			&& GET_OBJ_VAL(this, 3) > 1) //таймер у жижек и еды
		{
			dec_val(3);
		}
	}
}

float CObjectPrototype::show_mort_req() {
	return count_mort_requred(this);
}

float CObjectPrototype::show_koef_obj() {
	return count_unlimited_timer(this);
}

float CObjectPrototype::get_ilevel() const {
	return m_ilevel;
}

void CObjectPrototype::set_ilevel(float ilvl) {
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

std::string CObjectPrototype::item_count_message(int num, int pad) {
	if (num <= 0
		|| pad < 0
		|| pad > 5
		|| get_PName(pad).empty()) {
		log("SYSERROR : num=%d, pad=%d, pname=%s (%s:%d)", num, pad, get_PName(pad).c_str(), __FILE__, __LINE__);

		return "<ERROR>";
	}

	std::stringstream out;
	out << get_PName(pad);
	if (num > 1) {
		out << " (x " << num << " " << desc_count(num, WHAT_THINGa) << ")";
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

void ObjectData::set_activator(bool flag, int num) {
	m_activator.first = flag;
	m_activator.second = num;
}

std::pair<bool, int> ObjectData::get_activator() const {
	return m_activator;
}

void ObjectData::add_timed_spell(const int spell, const int time) {
	if (spell < 1 || spell >= SPELLS_COUNT) {
		log("SYSERROR: func: %s, spell = %d, time = %d", __func__, spell, time);
		return;
	}
	m_timed_spell.add(this, spell, time);
}

void ObjectData::del_timed_spell(const int spell, const bool message) {
	m_timed_spell.del(this, spell, message);
}

void CObjectPrototype::set_ex_description(const char *keyword, const char *description) {
	ExtraDescription::shared_ptr d(new ExtraDescription());
	d->keyword = strdup(keyword);
	d->description = strdup(description);
	m_ex_description = d;
}

void set_obj_aff(ObjectData *itemobj, const EAffectFlag bitv) {
	for (const auto &i : weapon_affect) {
		if (i.aff_bitvector == static_cast<bitvector_t>(bitv)) {
			SET_OBJ_AFF(itemobj, to_underlying(i.aff_pos));
		}
	}
}

void set_obj_eff(ObjectData *itemobj, const EApplyLocation type, int mod) {
	for (auto i = 0; i < kMaxObjAffect; i++) {
		if (itemobj->get_affected(i).location == type) {
			const auto current_mod = itemobj->get_affected(i).modifier;
			itemobj->set_affected(i, type, current_mod + mod);
			break;
		} else if (itemobj->get_affected(i).location == APPLY_NONE) {
			itemobj->set_affected(i, type, mod);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

namespace {

const float SQRT_MOD = 1.7095f;
const int AFF_SHIELD_MOD = 30;
const int AFF_BLINK_MOD = 10;

} // namespace

////////////////////////////////////////////////////////////////////////////////

namespace ObjSystem {

float count_affect_weight(const CObjectPrototype * /*obj*/, int num, int mod) {
	float weight = 0;

	switch (num) {
		case APPLY_STR: weight = mod * 7.5;
			break;
		case APPLY_DEX: weight = mod * 10.0;
			break;
		case APPLY_INT: weight = mod * 10.0;
			break;
		case APPLY_WIS: weight = mod * 10.0;
			break;
		case APPLY_CON: weight = mod * 10.0;
			break;
		case APPLY_CHA: weight = mod * 10.0;
			break;
		case APPLY_HIT: weight = mod * 0.3;
			break;
		case APPLY_AC: weight = mod * -0.5;
			break;
		case APPLY_HITROLL: weight = mod * 2.3;
			break;
		case APPLY_DAMROLL: weight = mod * 3.3;
			break;
		case APPLY_SAVING_WILL: weight = mod * -0.5;
			break;
		case APPLY_SAVING_CRITICAL: weight = mod * -0.5;
			break;
		case APPLY_SAVING_STABILITY: weight = mod * -0.5;
			break;
		case APPLY_SAVING_REFLEX: weight = mod * -0.5;
			break;
		case APPLY_CAST_SUCCESS: weight = mod * 1.5;
			break;
		case APPLY_MANAREG: weight = mod * 0.2;
			break;
		case APPLY_MORALE: weight = mod * 1.0;
			break;
		case APPLY_INITIATIVE: weight = mod * 2.0;
			break;
		case APPLY_ABSORBE: weight = mod * 1.0;
			break;
		case APPLY_AR: weight = mod * 1.5;
			break;
		case APPLY_MR: weight = mod * 1.5;
			break;
	}

	return weight;
}

bool is_armor_type(const CObjectPrototype *obj) {
	switch (GET_OBJ_TYPE(obj)) {
		case ObjectData::ITEM_ARMOR:
		case ObjectData::ITEM_ARMOR_LIGHT:
		case ObjectData::ITEM_ARMOR_MEDIAN:
		case ObjectData::ITEM_ARMOR_HEAVY: return true;

		default: return false;
	}
}

bool is_mob_item(const CObjectPrototype *obj) {
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MALE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_FEMALE)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_MONO)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_POLY)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_RUSICHI)
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE)) {
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
		&& IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_NO(obj, ENoFlag::ITEM_NO_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MALE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_FEMALE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_MONO)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLY)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_RUSICHI)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE)) {
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
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE)) {
		return true;
	}
	if (IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_SEVERANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_POLANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_KRIVICHI)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_VATICHI)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_VELANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_DREVLANE)
		&& IS_OBJ_ANTI(obj, EAntiFlag::ITEM_AN_CHARMICE)) {
		return true;
	}

	return false;
}

void init_ilvl(CObjectPrototype *obj) {
	if (is_mob_item(obj)
		|| obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF)
		|| obj->get_minimum_remorts() > 0) {
		obj->set_ilevel(0);
		return;
	}

	float total_weight = count_mort_requred(obj);

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
	PURSE_RNUM = real_object(PURSE_VNUM);
	PERS_CHEST_RNUM = real_object(PERS_CHEST_VNUM);
}

ObjectData *create_purse(CharacterData *ch, int/* gold*/) {
	const auto obj = world_objects.create_from_prototype_by_rnum(PURSE_RNUM);
	if (!obj) {
		return obj.get();
	}

	obj->set_aliases("тугой кошелек");
	obj->set_short_description("тугой кошелек");
	obj->set_description("Кем-то оброненный тугой кошелек лежит здесь.");
	obj->set_PName(0, "тугой кошелек");
	obj->set_PName(1, "тугого кошелька");
	obj->set_PName(2, "тугому кошельку");
	obj->set_PName(3, "тугой кошелек");
	obj->set_PName(4, "тугим кошельком");
	obj->set_PName(5, "тугом кошельке");

	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_),
			 "--------------------------------------------------\r\n"
			 "Владелец: %s\r\n"
			 "В случае потери просьба вернуть за вознаграждение.\r\n"
			 "--------------------------------------------------\r\n", ch->get_name().c_str());
	obj->set_ex_description(obj->get_PName(0).c_str(), buf_);

	obj->set_type(ObjectData::ITEM_CONTAINER);
	obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));

	obj->set_val(0, 0);
	// CLOSEABLE + CLOSED
	obj->set_val(1, 5);
	obj->set_val(2, -1);
	obj->set_val(3, ch->get_uid());

	obj->set_rent_off(0);
	obj->set_rent_on(0);
	// чтобы скавенж мобов не трогать
	obj->set_cost(2);
	obj->set_extra_flag(EExtraFlag::ITEM_NODONATE);
	obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);

	return obj.get();
}

bool is_purse(ObjectData *obj) {
	return obj->get_rnum() == PURSE_RNUM;
}

/// вываливаем и пуржим кошелек при попытке открыть или при взятии хозяином
void process_open_purse(CharacterData *ch, ObjectData *obj) {
	auto value = obj->get_val(1);
	REMOVE_BIT(value, CONT_CLOSED);
	obj->set_val(1, value);

	char buf_[kMaxInputLength];
	snprintf(buf_, sizeof(buf_), "all");
	get_from_container(ch, obj, buf_, FIND_OBJ_INV, 1, false);
	act("$o рассыпал$U в ваших руках...", false, ch, obj, 0, TO_CHAR);
	extract_obj(obj);
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

	for (auto i = m_values.begin(), iend = m_values.end(); i != iend; ++i) {
		std::string key_str = TextId::to_str(TextId::OBJ_VALS, to_underlying(i->first));
		if (!key_str.empty()) {
			out << key_str << " " << i->second << "\n";
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
		const int key = TextId::to_num(TextId::OBJ_VALS, key_str);
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
		std::string key_str = TextId::to_str(TextId::OBJ_VALS, to_underlying(i.first));
		if (!key_str.empty()) {
			m_val_vec.push_back(std::make_pair(key_str, i.second));
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
		const int key = TextId::to_num(TextId::OBJ_VALS, key_str);
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
			case ObjectData::ITEM_DRINKCON:
			case ObjectData::ITEM_FOUNTAIN:
				if (!is_valid_drinkcon(i->first)) {
					i = m_values.erase(i);
					erased = true;
				}
				break;
		} // switch

		if (!erased) {
			++i;
		}
	}
}

std::string print_obj_affects(const obj_affected_type &affect) {
	sprinttype(affect.location, apply_types, buf2);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++) {
		if (!str_cmp(buf2, apply_negative[j])) {
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0) {
		negative = true;
	} else if (negative && affect.modifier < 0) {
		negative = false;
	}

	snprintf(buf, kMaxStringLength, "%s%s%s%s%s%d%s\r\n",
			 KCYN, buf2, KNRM,
			 KCYN, (negative ? " ухудшает на " : " улучшает на "),
			 abs(affect.modifier), KNRM);

	return std::string(buf);
}

void print_obj_affects(CharacterData *ch, const obj_affected_type &affect) {
	sprinttype(affect.location, apply_types, buf2);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++) {
		if (!str_cmp(buf2, apply_negative[j])) {
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0) {
		negative = true;
	} else if (negative && affect.modifier < 0) {
		negative = false;
	}
	snprintf(buf, kMaxStringLength, "   %s%s%s%s%s%d%s\r\n",
			 CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM),
			 CCCYN(ch, C_NRM),
			 negative ? " ухудшает на " : " улучшает на ", abs(affect.modifier), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
}

typedef std::map<ObjectData::EObjectType, std::string> EObjectType_name_by_value_t;
typedef std::map<const std::string, ObjectData::EObjectType> EObjectType_value_by_name_t;
EObjectType_name_by_value_t EObjectType_name_by_value;
EObjectType_value_by_name_t EObjectType_value_by_name;
void init_EObjectType_ITEM_NAMES() {
	EObjectType_value_by_name.clear();
	EObjectType_name_by_value.clear();

	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_LIGHT] = "ITEM_LIGHT";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_SCROLL] = "ITEM_SCROLL";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_WAND] = "ITEM_WAND";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_STAFF] = "ITEM_STAFF";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_WEAPON] = "ITEM_WEAPON";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_FIREWEAPON] = "ITEM_FIREWEAPON";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MISSILE] = "ITEM_MISSILE";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_TREASURE] = "ITEM_TREASURE";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_ARMOR] = "ITEM_ARMOR";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_POTION] = "ITEM_POTION";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_WORN] = "ITEM_WORN";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_OTHER] = "ITEM_OTHER";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_TRASH] = "ITEM_TRASH";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_TRAP] = "ITEM_TRAP";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_CONTAINER] = "ITEM_CONTAINER";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_NOTE] = "ITEM_NOTE";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_DRINKCON] = "ITEM_DRINKCON";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_KEY] = "ITEM_KEY";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_FOOD] = "ITEM_FOOD";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MONEY] = "ITEM_MONEY";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_PEN] = "ITEM_PEN";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_BOAT] = "ITEM_BOAT";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_FOUNTAIN] = "ITEM_FOUNTAIN";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_BOOK] = "ITEM_BOOK";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_INGREDIENT] = "ITEM_INGREDIENT";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MING] = "ITEM_MING";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MATERIAL] = "ITEM_MATERIAL";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_BANDAGE] = "ITEM_BANDAGE";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_ARMOR_LIGHT] = "ITEM_ARMOR_LIGHT";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_ARMOR_MEDIAN] = "ITEM_ARMOR_MEDIAN";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_ARMOR_HEAVY] = "ITEM_ARMOR_HEAVY";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_ENCHANT] = "ITEM_ENCHANT";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MAGIC_MATERIAL] = "ITEM_MAGIC_MATERIAL";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MAGIC_ARROW] = "ITEM_MAGIC_ARROW";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_MAGIC_CONTAINER] = "ITEM_MAGIC_CONTAINER";
	EObjectType_name_by_value[ObjectData::EObjectType::ITEM_CRAFT_MATERIAL] = "ITEM_CRAFT_MATERIAL";

	for (const auto &i : EObjectType_name_by_value) {
		EObjectType_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ObjectData::EObjectType>(const ObjectData::EObjectType item) {
	if (EObjectType_name_by_value.empty()) {
		init_EObjectType_ITEM_NAMES();
	}
	return EObjectType_name_by_value.at(item);
}

template<>
ObjectData::EObjectType ITEM_BY_NAME(const std::string &name) {
	if (EObjectType_name_by_value.empty()) {
		init_EObjectType_ITEM_NAMES();
	}
	return EObjectType_value_by_name.at(name);
}

typedef std::map<ObjectData::EObjectMaterial, std::string> EObjectMaterial_name_by_value_t;
typedef std::map<const std::string, ObjectData::EObjectMaterial> EObjectMaterial_value_by_name_t;
EObjectMaterial_name_by_value_t EObjectMaterial_name_by_value;
EObjectMaterial_value_by_name_t EObjectMaterial_value_by_name;
void init_EObjectMaterial_ITEM_NAMES() {
	EObjectMaterial_value_by_name.clear();
	EObjectMaterial_name_by_value.clear();

	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_NONE] = "MAT_NONE";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_BULAT] = "MAT_BULAT";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_BRONZE] = "MAT_BRONZE";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_IRON] = "MAT_IRON";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_STEEL] = "MAT_STEEL";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_SWORDSSTEEL] = "MAT_SWORDSSTEEL";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_COLOR] = "MAT_COLOR";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_CRYSTALL] = "MAT_CRYSTALL";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_WOOD] = "MAT_WOOD";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_SUPERWOOD] = "MAT_SUPERWOOD";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_FARFOR] = "MAT_FARFOR";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_GLASS] = "MAT_GLASS";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_ROCK] = "MAT_ROCK";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_BONE] = "MAT_BONE";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_MATERIA] = "MAT_MATERIA";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_SKIN] = "MAT_SKIN";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_ORGANIC] = "MAT_ORGANIC";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_PAPER] = "MAT_PAPER";
	EObjectMaterial_name_by_value[ObjectData::EObjectMaterial::MAT_DIAMOND] = "MAT_DIAMOND";

	for (const auto &i : EObjectMaterial_name_by_value) {
		EObjectMaterial_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ObjectData::EObjectMaterial>(const ObjectData::EObjectMaterial item) {
	if (EObjectMaterial_name_by_value.empty()) {
		init_EObjectMaterial_ITEM_NAMES();
	}
	return EObjectMaterial_name_by_value.at(item);
}

template<>
ObjectData::EObjectMaterial ITEM_BY_NAME(const std::string &name) {
	if (EObjectMaterial_name_by_value.empty()) {
		init_EObjectMaterial_ITEM_NAMES();
	}
	return EObjectMaterial_value_by_name.at(name);
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
	for (id_to_set_info_map::const_iterator i = ObjectData::set_table.begin(),
			 iend = ObjectData::set_table.end(); i != iend; ++i) {
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
	for (std::vector<SetNode>::iterator i = set_list.begin(),
			 iend = set_list.end(); i != iend; ++i) {
		i->obj_vnum.clear();
	}
}

// * Проверка шмотки на принадлежность к сетам из set_list.
void check_item(int vnum) {
	for (std::vector<SetNode>::iterator i = set_list.begin(),
			 iend = set_list.end(); i != iend; ++i) {
		std::set<int>::const_iterator k = i->set_vnum.find(vnum);
		if (k != i->set_vnum.end()) {
			i->obj_vnum.push_back(vnum);
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
				log("[TO] Player %s : set-item %d deleted", player_table[pt_num].name(), i->vnum);
				i->timer = -1;
				int rnum = real_object(i->vnum);
				if (rnum >= 0) {
					obj_proto.dec_stored(rnum);
				}
				need_save = true;
			}
		}
	}
	if (need_save) {
		if (!Crash_write_timer(pt_num)) {
			log("SYSERROR: [TO] Error writing timer file for %s", player_table[pt_num].name());
		}
		return;
	}
	// перс.хран
	Depot::delete_set_item(player_table[pt_num].unique, vnum);
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
		Depot::check_rented(player_table[i].unique);
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
	if (!obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF)) {
		return false;
	}
	for (id_to_set_info_map::const_iterator i = ObjectData::set_table.begin(),
			 iend = ObjectData::set_table.end(); i != iend; ++i) {
		if (i->second.find(GET_OBJ_VNUM(obj)) != i->second.end()
			&& i->second.size() > sets_items) {
			return true;
		}
	}
	return false;
}

bool find_set_item(ObjectData *obj) {
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
	for (id_to_set_info_map::const_iterator i = ObjectData::set_table.begin(),
			 iend = ObjectData::set_table.end(); i != iend; ++i) {
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
bool house_find_set_item(CharacterData *ch, const std::set<int> &vnum_list) {
	// храны у нас через задницу сделаны
	for (ObjectData *chest = world[real_room(CLAN(ch)->get_chest_room())]->contents; chest;
		 chest = chest->get_next_content()) {
		if (Clan::is_clan_chest(chest)) {
			for (ObjectData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
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
bool is_norent_set(CharacterData *ch, ObjectData *obj, bool clan_chest) {
	if (!obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF)) {
		return false;
	}

	init_vnum_list(obj_sets::normalize_vnum(GET_OBJ_VNUM(obj)));

	if (vnum_list.empty()) {
		return false;
	}

	// экипировка
	for (int i = 0; i < NUM_WEARS; ++i) {
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
		for (struct Follower *k = ch->followers; k; k = k->next) {
			if (!IS_CHARMICE(k->ch)
				|| !k->ch->has_master()) {
				continue;
			}

			for (int j = 0; j < NUM_WEARS; j++) {
				if (find_set_item(GET_EQ(k->ch, j))) {
					return false;
				}
			}

			if (find_set_item(k->ch->carrying)) {
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
