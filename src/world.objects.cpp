#include "world.objects.hpp"

#include "id.hpp"
#include "object.prototypes.hpp"
#include "logger.hpp"
#include "db.h"
#include "liquid.hpp"
#include "dg_scripts.h"
#include "utils.h"
#include "global.objects.hpp"

#include <algorithm>

void WorldObjects::WO_VNumChangeObserver::notify(CObjectPrototype& object, const obj_vnum old_vnum)
{
	if (old_vnum != object.get_vnum())
	{
		// find shared_ptr
		object_raw_ptr_to_object_ptr_t::iterator i = m_parent.m_object_raw_ptr_to_object_ptr.find(&object);
		if (i == m_parent.m_object_raw_ptr_to_object_ptr.end())
		{
			log("LOGIC ERROR: object with VNUM %d has not been found in world objects container. New VNUM is %d.",
				old_vnum, object.get_vnum());
			return;
		}
		OBJ_DATA::shared_ptr object_ptr = *i->second;

		// remove old index entry
		vnum_to_object_ptr_t::iterator vnum_to_object_i = m_parent.m_vnum_to_object_ptr.find(old_vnum);
		if (vnum_to_object_i != m_parent.m_vnum_to_object_ptr.end())
		{
			vnum_to_object_i->second.erase(object_ptr);
		}

		// insert new entry to the index
		const auto vnum = object_ptr->get_vnum();
		m_parent.m_vnum_to_object_ptr[vnum].insert(object_ptr);
	}
}

void WorldObjects::WO_RNumChangeObserver::notify(CObjectPrototype& object, const obj_rnum old_rnum)
{
	if (old_rnum != object.get_rnum())
	{
		// find shared_ptr
		object_raw_ptr_to_object_ptr_t::iterator i = m_parent.m_object_raw_ptr_to_object_ptr.find(&object);
		if (i == m_parent.m_object_raw_ptr_to_object_ptr.end())
		{
			log("LOGIC ERROR: object with RNUM %d has not been found in world objects container. New RNUM is %d.",
				old_rnum, object.get_rnum());
			return;
		}
		OBJ_DATA::shared_ptr object_ptr = *i->second;

		// remove old index entry
		rnum_to_object_ptr_t::iterator rnum_to_object_ptr_i = m_parent.m_rnum_to_object_ptr.find(old_rnum);
		if (rnum_to_object_ptr_i != m_parent.m_vnum_to_object_ptr.end())
		{
			rnum_to_object_ptr_i->second.erase(object_ptr);
		}

		// insert new entry to the index
		const auto rnum = object_ptr->get_rnum();
		m_parent.m_rnum_to_object_ptr[rnum].insert(object_ptr);
	}
}

void WorldObjects::WO_IDChangeObserver::notify(OBJ_DATA& object, const object_id_t old_id)
{
	if (old_id != object.get_id())
	{
		// find shared_ptr
		object_raw_ptr_to_object_ptr_t::iterator i = m_parent.m_object_raw_ptr_to_object_ptr.find(&object);
		if (i == m_parent.m_object_raw_ptr_to_object_ptr.end())
		{
			log("LOGIC ERROR: object with ID %ld has not been found in world objects container. New ID is %ld.",
				old_id, object.get_id());
			return;
		}
		OBJ_DATA::shared_ptr object_ptr = *i->second;

		// remove old index entry
		id_to_object_ptr_t::iterator id_to_object_ptr_i = m_parent.m_id_to_object_ptr.find(old_id);
		if (id_to_object_ptr_i != m_parent.m_id_to_object_ptr.end())
		{
			id_to_object_ptr_i->second.erase(object_ptr);
		}

		// insert new entry to the index
		const auto vnum = object_ptr->get_id();
		m_parent.m_id_to_object_ptr[vnum].insert(object_ptr);
	}
}

WorldObjects::WorldObjects() :
	m_id_change_observer(new WO_IDChangeObserver(*this)),
	m_rnum_change_observer(new WO_RNumChangeObserver(*this)),
	m_vnum_change_observer(new WO_VNumChangeObserver(*this))
{
}

OBJ_DATA::shared_ptr WorldObjects::create_blank(const std::string& alias)
{
	OBJ_DATA::shared_ptr new_object(new OBJ_DATA(-1));

	add(new_object);

	const auto id = max_id.allocate();
	new_object->set_id(id);
	new_object->set_extra_flag(EExtraFlag::ITEM_NORENT);
	new_object->set_aliases(alias);

	return std::move(new_object);
}

OBJ_DATA::shared_ptr WorldObjects::create_from_prototype_by_vnum(obj_vnum vnum)
{
	const auto rnum = real_object(vnum);
	if (rnum < 0)
	{
		log("Object (V) %d does not exist in database.", vnum);
		return nullptr;
	}

	return create_from_prototype_by_rnum(rnum);
}

OBJ_DATA::shared_ptr WorldObjects::create_from_prototype_by_rnum(obj_rnum rnum)
{
	const auto new_object = create_raw_from_prototype_by_rnum(rnum);
	if (new_object)
	{
		rnum = obj_proto.zone(rnum);
		if (rnum != -1 && zone_table[rnum].under_construction)
		{
			// модификация объектов тестовой зоны
			constexpr int TEST_OBJECT_TIMER = 30;
			new_object->set_timer(TEST_OBJECT_TIMER);
			new_object->set_extra_flag(EExtraFlag::ITEM_NOLOCATE);
			new_object->set_extra_flag(EExtraFlag::ITEM_NOT_UNLIMIT_TIMER);
		}

		new_object->clear_proto_script();

		const auto id = max_id.allocate();
		new_object->set_id(id);
		if (new_object->get_type() == OBJ_DATA::ITEM_DRINKCON)
		{
			if (new_object->get_val(1)
				&& new_object->get_val(2))
			{
				name_to_drinkcon(new_object.get(), new_object->get_val(2));
			}
		}

		assign_triggers(new_object.get(), OBJ_TRIGGER);
	}

	return std::move(new_object);
}

OBJ_DATA::shared_ptr WorldObjects::create_raw_from_prototype_by_rnum(obj_rnum rnum)
{
	// and obj_rnum
	if (rnum < 0)
	{
		log("SYSERR: Trying to create obj with negative (%d) num!", rnum);
		return nullptr;
	}

	const auto new_object = std::make_shared<OBJ_DATA>(*obj_proto[rnum]);
	obj_proto.inc_number(rnum);
	world_objects.add(new_object);

	return std::move(new_object);
}

void WorldObjects::add(const OBJ_DATA::shared_ptr& object)
{
	m_objects_list.push_front(object);
	add_to_index(m_objects_list.begin());

	object->subscribe_for_id_change(m_id_change_observer);
	object->subscribe_for_rnum_changes(m_rnum_change_observer);
	object->subscribe_for_vnum_changes(m_vnum_change_observer);
}

void WorldObjects::remove(OBJ_DATA* object)
{
	const auto object_i = m_object_raw_ptr_to_object_ptr.find(object);
	if (object_i == m_object_raw_ptr_to_object_ptr.end())
	{
		log("LOGIC ERROR: Couldn't find object at address %p. It cannot be removed.", object);
		return;
	}

	const OBJ_DATA::shared_ptr object_ptr = get_by_raw_ptr(object);

	object_ptr->unsubscribe_from_id_change(m_id_change_observer);
	object_ptr->unsubscribe_from_rnum_changes(m_rnum_change_observer);
	object_ptr->unsubscribe_from_vnum_changes(m_vnum_change_observer);

	m_id_to_object_ptr[object_ptr->get_id()].erase(object_ptr);
	m_vnum_to_object_ptr[object_ptr->get_vnum()].erase(object_ptr);
	m_rnum_to_object_ptr[object_ptr->get_rnum()].erase(object_ptr);
	m_objects_list.erase(object_i->second);
	m_object_raw_ptr_to_object_ptr.erase(object);

	m_purge_list.push_back(object_ptr);
}

void WorldObjects::foreach(const foreach_f function) const
{
	const list_t& list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

void WorldObjects::foreach_on_copy(const foreach_f function) const
{
	const list_t list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

void WorldObjects::foreach_on_copy_while(const foreach_while_f function) const
{
	const list_t list = get_list();
	for (list_t::const_iterator i = list.begin(); i != list.end(); ++i)
	{
		if (!function(*i))
		{
			break;
		}
	}
}

void WorldObjects::foreach_with_vnum(const obj_vnum vnum, const foreach_f function) const
{
	const auto set_i = m_vnum_to_object_ptr.find(vnum);
	if (set_i != m_vnum_to_object_ptr.end())
	{
		std::for_each(set_i->second.begin(), set_i->second.end(), function);
	}
}

void WorldObjects::foreach_with_rnum(const obj_rnum rnum, const foreach_f function) const
{
	const auto set_i = m_rnum_to_object_ptr.find(rnum);
	if (set_i != m_rnum_to_object_ptr.end())
	{
		std::for_each(set_i->second.begin(), set_i->second.end(), function);
	}
}

void WorldObjects::foreach_with_id(const object_id_t id, const foreach_f function) const
{
	const auto set_i = m_id_to_object_ptr.find(id);
	if (set_i != m_id_to_object_ptr.end())
	{
		std::for_each(set_i->second.begin(), set_i->second.end(), function);
	}
}

OBJ_DATA::shared_ptr WorldObjects::find_if(const predicate_f predicate) const
{
	return find_if(predicate, 0);
}

OBJ_DATA::shared_ptr WorldObjects::find_if(const predicate_f predicate, unsigned number) const
{
	return find_if_and_dec_number(predicate, number);
}

OBJ_DATA::shared_ptr WorldObjects::find_if_and_dec_number(const predicate_f predicate, unsigned& number) const
{
	list_t::const_iterator result_i = std::find_if(m_objects_list.begin(), m_objects_list.end(), predicate);

	while (result_i != m_objects_list.end()
		&& 0 < number)
	{
		result_i = std::find_if(++result_i, m_objects_list.end(), predicate);
		--number;
	}

	OBJ_DATA::shared_ptr result;
	if (result_i != m_objects_list.end())
	{
		result = *result_i;
	}

	return std::move(result);
}

OBJ_DATA::shared_ptr WorldObjects::find_by_name(const char* name) const
{
	return find_if([&](const OBJ_DATA::shared_ptr& obj) -> bool
	{
		return isname(name, obj->get_aliases());
	});
}

OBJ_DATA::shared_ptr WorldObjects::find_by_id(const object_id_t id, unsigned number) const
{
	const auto set_i = m_id_to_object_ptr.find(id);
	if (set_i != m_id_to_object_ptr.end())
	{
		for (const auto& object : set_i->second)
		{
			if (0 == number)
			{
				return object;
			}

			--number;
		}
	}

	return nullptr;
}

OBJ_DATA::shared_ptr WorldObjects::find_by_vnum(const obj_vnum vnum, unsigned number) const
{
	return find_by_vnum_and_dec_number(vnum, number);
}

OBJ_DATA::shared_ptr WorldObjects::find_by_vnum_and_dec_number(const obj_vnum vnum, unsigned& number) const
{
	object_id_set_t except;
	return find_by_vnum_and_dec_number(vnum, number, except);
}

OBJ_DATA::shared_ptr WorldObjects::find_by_vnum_and_dec_number(const obj_vnum vnum, unsigned& number, const object_id_set_t& except) const
{
	const auto set_i = m_vnum_to_object_ptr.find(vnum);
	if (set_i != m_vnum_to_object_ptr.end())
	{
		for (const auto& object : set_i->second)
		{
			if (except.find(object->get_id()) == except.end())
			{
				if (0 == number)
				{
					return object;
				}

				--number;
			}
		}
	}

	return nullptr;
}

OBJ_DATA::shared_ptr WorldObjects::find_by_rnum(const obj_rnum rnum, unsigned number) const
{
	const auto set_i = m_rnum_to_object_ptr.find(rnum);
	if (set_i != m_rnum_to_object_ptr.end())
	{
		for (const auto& object : set_i->second)
		{
			if (0 == number)
			{
				return object;
			}

			--number;
		}
	}

	return nullptr;
}

OBJ_DATA::shared_ptr WorldObjects::get_by_raw_ptr(OBJ_DATA* object) const
{
	OBJ_DATA::shared_ptr result;

	const auto result_i = m_object_raw_ptr_to_object_ptr.find(object);
	if (result_i != m_object_raw_ptr_to_object_ptr.end())
	{
		result = *result_i->second;
	}

	return std::move(result);
}

void WorldObjects::add_to_index(const list_t::iterator& object_i)
{
	const auto object = *object_i;
	const auto vnum = object->get_vnum();
	m_vnum_to_object_ptr[vnum].insert(object);
	m_id_to_object_ptr[object->get_id()].insert(object);
	m_rnum_to_object_ptr[object->get_rnum()].insert(object);
	m_object_raw_ptr_to_object_ptr.emplace(object.get(), object_i);
}

WorldObjects& world_objects = GlobalObjects::world_objects();	// contains all objects in the world

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
