#ifndef __FIND_OBJ_ID_BY_VNUM_HPP__
#define __FIND_OBJ_ID_BY_VNUM_HPP__

#include "id.hpp"
#include "structs.h"

class CHAR_DATA;	// to avoid inclusion of "char.hpp"
class OBJ_DATA;		// to avoid inclusion of "obj.hpp"
struct ROOM_DATA;	// to avoid inclusion of "room.hpp"

class FindObjIDByVNUM
{
public:
	static constexpr object_id_t NOT_FOUND = -1;

	FindObjIDByVNUM(const obj_vnum vnum, const unsigned number) : m_vnum(vnum), m_number(number), m_result(NOT_FOUND) {}

	bool lookup_world_objects();
	bool lookup_inventory(const CHAR_DATA* character);
	bool lookup_worn(const CHAR_DATA* character);
	bool lookup_room(const room_rnum character);
	bool lookup_list(const OBJ_DATA* list);

	int lookup_for_caluid(const int type, const void* go);

	int result() const { return m_result; }

private:
	void add_seen(const object_id_t id) { m_seen.insert(id); }

	obj_vnum m_vnum;
	unsigned m_number;
	object_id_t m_result;
	object_id_set_t m_seen;
};


int find_obj_by_id_vnum__find_replacement(const obj_vnum vnum);
int find_obj_by_id_vnum__calcuid(const obj_vnum vnum, const unsigned number, const int type, const void* go);

#endif // __FIND_OBJ_ID_BY_VNUM_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
