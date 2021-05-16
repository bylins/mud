#include "find.obj.id.by.vnum.hpp"

#include "logger.hpp"
#include "dg_script/dg_scripts.h"
#include "world.objects.hpp"
#include "chars/character.h"
#include "obj.hpp"
#include "room.hpp"

class TriggerLookup
{
public:
	using shared_ptr = std::shared_ptr<TriggerLookup>;

	TriggerLookup(FindObjIDByVNUM& finder) : m_finder(finder) {}
	~TriggerLookup() {}

	static TriggerLookup::shared_ptr create(FindObjIDByVNUM& finder, const int type, const void* go);

	virtual int lookup() = 0;

protected:
	FindObjIDByVNUM& finder() { return m_finder; }

private:
	FindObjIDByVNUM& m_finder;
};

class MobTriggerLookup : public TriggerLookup
{
public:
	MobTriggerLookup(FindObjIDByVNUM& finder, const CHAR_DATA* mob) : TriggerLookup(finder), m_mob(mob) {}

	virtual int lookup() override;

private:
	const CHAR_DATA* m_mob;
};

class ObjTriggerLookup : public TriggerLookup
{
public:
	ObjTriggerLookup(FindObjIDByVNUM& finder, const OBJ_DATA* object) : TriggerLookup(finder), m_object(object) {}

	virtual int lookup() override;

private:
	const OBJ_DATA* m_object;
};

class WldTriggerLookup : public TriggerLookup
{
public:
	WldTriggerLookup(FindObjIDByVNUM& finder, const ROOM_DATA* room) : TriggerLookup(finder), m_room(room) {}

	virtual int lookup() override;

private:
	const ROOM_DATA* m_room;
};

int WldTriggerLookup::lookup()
{
	auto result = false;
	if (m_room)
	{
		const auto room_rnum = real_room(m_room->number);
		result = finder().lookup_room(room_rnum);
	}

	if (!result)
	{
		finder().lookup_world_objects();
	}
	return finder().result();
}

int ObjTriggerLookup::lookup()
{
	if (!m_object)
	{
		return FindObjIDByVNUM::NOT_FOUND;
	}

	const auto owner = m_object->get_worn_by() ? m_object->get_worn_by() : m_object->get_carried_by();
	if (owner)
	{
		const auto mob_lookuper = std::make_shared<MobTriggerLookup>(finder(), owner);
		return mob_lookuper->lookup();
	}

	const auto object_room = m_object->get_in_room();
	const auto result = finder().lookup_room(object_room);
	if (!result)
	{
		finder().lookup_world_objects();
	}

	return finder().result();
}

int MobTriggerLookup::lookup()
{
	auto result = false;
	if (m_mob)
	{
		result = finder().lookup_inventory(m_mob)
			|| finder().lookup_room(m_mob->in_room);
	}

	if (!result)
	{
		finder().lookup_world_objects();
	}

	return finder().result();
}

// * Аналогично find_char_vnum, только для объектов.
bool FindObjIDByVNUM::lookup_world_objects()
{
	OBJ_DATA::shared_ptr object = world_objects.find_by_vnum_and_dec_number(m_vnum, m_number, m_seen);

	if (object)
	{
		m_result = object->get_id();
		return true;
	}

	return false;
}

bool FindObjIDByVNUM::lookup_inventory(const CHAR_DATA* character)
{
	if (!character)
	{
		return false;
	}

	return lookup_list(character->carrying);
}

bool FindObjIDByVNUM::lookup_worn(const CHAR_DATA* character)
{
	if (!character)
	{
		return false;
	}

	for (int i = 0; i < NUM_WEARS; ++i)
	{
		const auto equipment = character->equipment[i];
		if (equipment
			&& equipment->get_vnum() == m_vnum)
		{
			if (0 == m_number)
			{
				m_result = equipment->get_id();
				return true;
			}

			add_seen(equipment->get_id());
			--m_number;
		}
	}

	return false;
}

bool FindObjIDByVNUM::lookup_room(const room_rnum room)
{
	const auto room_contents = world[room]->contents;
	if (!room_contents)
	{
		return false;
	}

	return lookup_list(room_contents);
}

bool FindObjIDByVNUM::lookup_list(const OBJ_DATA* list)
{
	while (list)
	{
		if (list->get_vnum() == m_vnum)
		{
			if (0 == m_number)
			{
				m_result = list->get_id();
				return true;
			}

			add_seen(list->get_id());
			--m_number;
		}

		list = list->get_next_content();
	}

	return false;
}

int FindObjIDByVNUM::lookup_for_caluid(const int type, const void* go)
{
	const auto lookuper = TriggerLookup::create(*this, type, go);

	if (lookuper)
	{
		return lookuper->lookup();
	}

	return NOT_FOUND;
}

TriggerLookup::shared_ptr TriggerLookup::create(FindObjIDByVNUM& finder, const int type, const void* go)
{
	switch (type)
	{
	case WLD_TRIGGER:
		return std::make_shared<WldTriggerLookup>(finder, static_cast<const ROOM_DATA*>(go));

	case OBJ_TRIGGER:
		return std::make_shared<ObjTriggerLookup>(finder, static_cast<const OBJ_DATA*>(go));

	case MOB_TRIGGER:
		return std::make_shared<MobTriggerLookup>(finder, static_cast<const CHAR_DATA*>(go));
	}

	log("SYSERR: Logic error trigger type %d is not valid. Valid values are %d, %d, %d",
		type, MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER);

	return nullptr;
}

int find_obj_by_id_vnum__find_replacement(const obj_vnum vnum)
{
	FindObjIDByVNUM finder(vnum, 0);

	finder.lookup_world_objects();
	const auto result = finder.result();

	return result;
}

int find_obj_by_id_vnum__calcuid(const obj_vnum vnum, const unsigned number, const int type, const void* go)
{
	FindObjIDByVNUM finder(vnum, number);

	finder.lookup_for_caluid(type, go);
	const auto result = finder.result();

	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
