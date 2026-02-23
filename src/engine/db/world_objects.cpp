#include "world_objects.h"
#include "engine/core/handler.h"
#include "obj_prototypes.h"
#include "db.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/utils.h"
#include "global_objects.h"
#include "utils/backtrace.h"
#include "gameplay/mechanics/dungeons.h"

#include <algorithm>

void WorldObjects::WO_VNumChangeObserver::notify(CObjectPrototype &object, const ObjVnum old_vnum) {
	if (old_vnum != object.get_vnum()) {
		// find shared_ptr
		auto i = m_parent.m_object_raw_ptr_to_object_ptr.find(&object);
		if (i == m_parent.m_object_raw_ptr_to_object_ptr.end()) {
			log("LOGIC ERROR: object with VNUM %d has not been found in world objects container. New VNUM is %d.",
				old_vnum, object.get_vnum());
			return;
		}
		ObjData::shared_ptr object_ptr = *i->second;

		// remove old index entry
		auto vnum_to_object_i = m_parent.m_vnum_to_object_ptr.find(old_vnum);
		if (vnum_to_object_i != m_parent.m_vnum_to_object_ptr.end()) {
			vnum_to_object_i->second.erase(object_ptr);
		}

		// insert new entry to the index
		const auto vnum = object_ptr->get_vnum();
		m_parent.m_vnum_to_object_ptr[vnum].insert(object_ptr);
	}
}

void WorldObjects::WO_RNumChangeObserver::notify(CObjectPrototype &object, const ObjRnum old_rnum) {
	if (old_rnum != object.get_rnum()) {
		// find shared_ptr
		auto i = m_parent.m_object_raw_ptr_to_object_ptr.find(&object);
		if (i == m_parent.m_object_raw_ptr_to_object_ptr.end()) {
			log("LOGIC ERROR: object with RNUM %d has not been found in world objects container. New RNUM is %d.",
				old_rnum, object.get_rnum());
			return;
		}
		ObjData::shared_ptr object_ptr = *i->second;

		// remove old index entry
		if (m_parent.m_rnum_to_object_ptr.contains(old_rnum)) {
			m_parent.m_rnum_to_object_ptr[old_rnum].erase(object_ptr);
		}

		// insert new entry to the index
		const auto rnum = object_ptr->get_rnum();
		m_parent.m_rnum_to_object_ptr[rnum].insert(object_ptr);
	}
}

void WorldObjects::WO_IDChangeObserver::notify(ObjData &object, const object_id_t old_id) {
	if (old_id != object.get_id()) {
		// find shared_ptr
		auto i = m_parent.m_object_raw_ptr_to_object_ptr.find(&object);
		if (i == m_parent.m_object_raw_ptr_to_object_ptr.end()) {
			log("LOGIC ERROR: object with ID %ld has not been found in world objects container. New GetAbilityId is %ld.",
				old_id, object.get_id());
			return;
		}

		ObjData::shared_ptr object_ptr = *i->second;

		// remove old index entry
		auto id_to_object_ptr_i = m_parent.m_id_to_object_ptr.find(old_id);
		if (id_to_object_ptr_i != m_parent.m_id_to_object_ptr.end()) {
			m_parent.m_id_to_object_ptr.erase(id_to_object_ptr_i);
		}
		// insert new entry to the index
		const auto vnum = object_ptr->get_id();
		m_parent.m_id_to_object_ptr[vnum] = object_ptr;
	}
}

WorldObjects::WorldObjects() :
	m_id_change_observer(new WO_IDChangeObserver(*this)),
	m_rnum_change_observer(new WO_RNumChangeObserver(*this)),
	m_vnum_change_observer(new WO_VNumChangeObserver(*this)) {
}

ObjData::shared_ptr WorldObjects::create_blank() {
	const auto blank = create_from_prototype_by_rnum(0);
	blank->set_extra_flag(EObjFlag::kTransformed);
	blank->set_extra_flag(EObjFlag::kNorent);
	return blank; //вместо -1 вставим реальный объект
}

ObjData::shared_ptr WorldObjects::create_from_prototype_by_vnum(ObjVnum vnum) {
	const auto rnum = GetObjRnum(vnum);
	if (rnum < 0) {
		log("Object (V) %d does not exist in database.", vnum);
		return nullptr;
	}

	return create_from_prototype_by_rnum(rnum);
}

ObjData::shared_ptr WorldObjects::create_from_prototype_by_rnum(ObjRnum rnum) {
	auto new_object = create_raw_from_prototype_by_rnum(rnum);
	if (new_object) {
		rnum = obj_proto.zone(rnum);
		if (rnum != -1 && zone_table[rnum].under_construction) {
			// модификация объектов тестовой зоны
			constexpr int TEST_OBJECT_TIMER = 30;
			new_object->set_timer(TEST_OBJECT_TIMER);
			new_object->set_extra_flag(EObjFlag::kNolocate);
			new_object->set_extra_flag(EObjFlag::KLimitedTimer);
			new_object->set_extra_flag(EObjFlag::kTransformed);
			new_object->set_extra_flag(EObjFlag::kNosell);
		}
		new_object->clear_proto_script();
		if (new_object->get_type() == EObjType::kLiquidContainer) {
			if (new_object->get_val(1) > 0) {
				name_from_drinkcon(new_object.get());
				name_to_drinkcon(new_object.get(), new_object->get_val(2));
				}
		}

		assign_triggers(new_object.get(), OBJ_TRIGGER);
	}

	return new_object;
}

ObjData::shared_ptr WorldObjects::create_raw_from_prototype_by_rnum(ObjRnum rnum) {
	// and ObjRnum
	if (rnum < 0) {
		log("SYSERR: Trying to create obj with negative (%d) num!", rnum);
		return nullptr;
	}

	auto new_object = std::make_shared<ObjData>(*obj_proto[rnum]);
	const auto id = max_id.allocate();

	obj_proto.inc_number(rnum);
	new_object->set_id(id);
	world_objects.add(new_object);
	if (new_object->get_vnum() / 100  >= dungeons::kZoneStartDungeons && new_object->get_parent_rnum() == -1) {
		ZoneVnum zvn = new_object->get_vnum() / 100;
		mudlog(fmt::format("ERROR: в данж #{} оригинал #{} загружен несуществующий в оригинале предмет #{}", zvn, zone_table[GetZoneRnum(zvn)].copy_from_zone, new_object->get_vnum()));
	}
	return new_object;
}

void WorldObjects::add(const ObjData::shared_ptr &object) {
	m_objects_list.push_front(object);
	add_to_index(m_objects_list.begin());

	object->subscribe_for_id_change(m_id_change_observer);
	object->subscribe_for_rnum_changes(m_rnum_change_observer);
	object->subscribe_for_vnum_changes(m_vnum_change_observer);
}

void WorldObjects::remove(ObjData *object) {
	const auto object_i = m_object_raw_ptr_to_object_ptr.find(object);
	if (object_i == m_object_raw_ptr_to_object_ptr.end()) {
		log("LOGIC ERROR: Couldn't find object at address %p. It cannot be removed.", object);
		return;
	}

	const ObjData::shared_ptr object_ptr = get_by_raw_ptr(object);
	obj_proto.dec_number(object->get_rnum());
	object_ptr->unsubscribe_from_id_change(m_id_change_observer);
	object_ptr->unsubscribe_from_rnum_changes(m_rnum_change_observer);
	object_ptr->unsubscribe_from_vnum_changes(m_vnum_change_observer);

	m_id_to_object_ptr.erase(object_ptr->get_id());
	m_vnum_to_object_ptr[object_ptr->get_vnum()].erase(object_ptr);
	m_rnum_to_object_ptr[object_ptr->get_rnum()].erase(object_ptr);
	m_objects_list.erase(object_i->second);
	m_object_raw_ptr_to_object_ptr.erase(object);

	m_purge_list.push_back(object_ptr);
}

void WorldObjects::foreach(const foreach_f &function) const {
	const list_t &list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

void WorldObjects::foreach_on_copy(const foreach_f &function) const {
	const list_t list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

void WorldObjects::foreach_on_copy_while(const foreach_while_f &function) const {
	const list_t list = get_list();
	for (const auto &it : list) {
		if (!function(it)) {
			break;
		}
	}
}

void WorldObjects::foreach_with_vnum(const ObjVnum vnum, const foreach_f &function) const {
	const auto set_i = m_vnum_to_object_ptr.find(vnum);
	if (set_i != m_vnum_to_object_ptr.end()) {
		std::for_each(set_i->second.begin(), set_i->second.end(), function);
	}
}

void WorldObjects::foreach_with_rnum(const ObjRnum rnum, const foreach_f &function) const {
	const auto set_i = m_rnum_to_object_ptr.find(rnum);
	if (set_i != m_rnum_to_object_ptr.end()) {
		std::for_each(set_i->second.begin(), set_i->second.end(), function);
	}
}

ObjData::shared_ptr WorldObjects::find_if(const predicate_f &predicate) const {
	return find_if(predicate, 0);
}

ObjData::shared_ptr WorldObjects::find_if(const predicate_f &predicate, int number) const {
	return find_if_and_dec_number(predicate, number);
}

ObjData::shared_ptr WorldObjects::find_if_and_dec_number(const predicate_f &predicate, int &number) const {
	auto result_i = std::find_if(m_objects_list.begin(), m_objects_list.end(), predicate);

	while (result_i != m_objects_list.end() && number > 1) {
		result_i = std::find_if(++result_i, m_objects_list.end(), predicate);
		--number;
	}

	ObjData::shared_ptr result;
	if (result_i != m_objects_list.end()) {
		result = *result_i;
	}

	return result;
}

ObjData::shared_ptr WorldObjects::find_by_name(const char *name) const {
	return find_if([&](const ObjData::shared_ptr &obj) -> bool {
		return isname(name, obj->get_aliases());
	});
}

void WorldObjects::swap_id(const object_id_t from_id, const object_id_t to_id) {
	std::swap(m_id_to_object_ptr[from_id], m_id_to_object_ptr[to_id]);
}

ObjData::shared_ptr WorldObjects::find_by_id(const object_id_t id) const {
	const auto it = m_id_to_object_ptr.find(id);
	if (it != m_id_to_object_ptr.end()) {
		return it->second;
	}
	return nullptr;
}

ObjData::shared_ptr WorldObjects::find_by_vnum(const ObjVnum vnum, unsigned number) const {
	return find_by_vnum_and_dec_number(vnum, number);
}

ObjData::shared_ptr WorldObjects::find_by_vnum_and_dec_number(const ObjVnum vnum, unsigned &number) const {
	object_id_set_t except;
	return find_by_vnum_and_dec_number(vnum, number, except);
}

ObjData::shared_ptr WorldObjects::find_by_vnum_and_dec_number(const ObjVnum vnum,
															  unsigned &number,
															  const object_id_set_t &except) const {
	const auto set_i = m_vnum_to_object_ptr.find(vnum);
	if (set_i != m_vnum_to_object_ptr.end()) {
		for (const auto &object : set_i->second) {
			if (except.find(object->get_id()) == except.end()) {
				if (0 == number) {
					return object;
				}

				--number;
			}
		}
	}

	return nullptr;
}

ObjData::shared_ptr WorldObjects::find_by_rnum(const ObjRnum rnum, unsigned number) const {
	const auto set_i = m_rnum_to_object_ptr.find(rnum);
	if (set_i != m_rnum_to_object_ptr.end()) {
		for (const auto &object : set_i->second) {
			if (0 == number) {
				return object;
			}

			--number;
		}
	}

	return nullptr;
}

ObjData::shared_ptr WorldObjects::get_by_raw_ptr(ObjData *object) const {
	ObjData::shared_ptr result;

	const auto result_i = m_object_raw_ptr_to_object_ptr.find(object);
	if (result_i != m_object_raw_ptr_to_object_ptr.end()) {
		result = *result_i->second;
	}

	return result;
}

void WorldObjects::GetObjListByRnum(const ObjRnum rnum, std::list<ObjData *> &result) {
	result.clear();
	foreach_with_rnum(rnum, [&result](const ObjData::shared_ptr &obj) {
		result.push_back(obj.get());
	});
}

void WorldObjects::GetObjListByVnum(const ObjVnum vnum, std::list<ObjData *> &result) {
	result.clear();
	foreach_with_vnum(vnum, [&result](const ObjData::shared_ptr &obj) {
		result.push_back(obj.get());
	});
}

void WorldObjects::AddToExtractedList(ObjData *obj) {
	const ObjData::shared_ptr object_ptr = get_by_raw_ptr(obj);

	object_ptr->unsubscribe_from_rnum_changes(m_rnum_change_observer);
	m_rnum_to_object_ptr[object_ptr->get_rnum()].erase(object_ptr);
	log("add obj to extracted list %s %d", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_VNUM(obj));
//	if (obj->get_vnum() == 33516) {
//		debug::backtrace(runtime_config.logs(SYSLOG).handle());
//	}
	obj->set_extracted_list(true);
//	obj->get_script()->set_purged(true);
	m_extracted_list.insert(obj);
}

void WorldObjects::PurgeExtractedList() {
	if (!m_extracted_list.empty()) {
		log("Start obj PurgeExtractedList");
		for (auto it : m_extracted_list) {
			ExtractObjFromWorld(it, false);
		}
		m_extracted_list.clear();
	}
}

void WorldObjects::add_to_index(const list_t::iterator &object_i) {
	const auto object = *object_i;
	const auto vnum = object->get_vnum();
	m_vnum_to_object_ptr[vnum].insert(object);
	m_id_to_object_ptr[object->get_id()] = object;
	m_rnum_to_object_ptr[object->get_rnum()].insert(object);
	m_object_raw_ptr_to_object_ptr.emplace(object.get(), object_i);
}

WorldObjects &world_objects = GlobalObjects::world_objects();    // contains all objects in the world

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
