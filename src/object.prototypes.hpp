#ifndef __OBJECT_PROTOTYPES_HPP__
#define __OBJECT_PROTOTYPES_HPP__

#include "obj.hpp"
#include "logger.hpp"

#include <deque>

class CHAR_DATA;	// to avoid inclusion of the "char.hpp"
class TRIG_DATA;	// to avoid inclusion of the "dg_script.h"

class  CObjectPrototypes
{
private:
	struct SPrototypeIndex
	{
		SPrototypeIndex() : number(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}
		
		int number;		// number of existing units of this mob/obj //
		int stored;		// number of things in rent file            //
		int(*func)(CHAR_DATA*, void*, int, char*);
		char *farg;		// string argument for special function     //
		TRIG_DATA *proto;	// for triggers... the trigger     //
		int zone;			// mob/obj zone rnum //
		size_t set_idx; // индекс сета в obj_sets::set_list, если != -1
	};

public:
	~CObjectPrototypes() {};
	using prototypes_t = std::deque<CObjectPrototype::shared_ptr>;
	using const_iterator = prototypes_t::const_iterator;

	using index_t = std::deque<SPrototypeIndex>;

	/**
	** \name Proxy calls to std::vector member functions.
	**
	** This methods just perform proxy calls to corresponding functions of the m_prototype field.
	**
	** @{
	*/
	auto begin() const { return m_prototypes.begin(); }
	auto end() const { return m_prototypes.end(); }
	auto size() const { return m_prototypes.size(); }
	const auto& at(const size_t index) const {
	    return m_prototypes[index];
	}

	const auto& operator[](size_t index) const {
        if (index >= m_prototypes.size() )
        {
			mudlog("неизвестный прототип объекта", BRF, LVL_BUILDER, SYSLOG, 1);
        	return m_prototypes[0];
        }
	    return m_prototypes[index]; }
	/** @} */

	size_t add(CObjectPrototype* prototype, const obj_vnum vnum);
	size_t add(const CObjectPrototype::shared_ptr& prototype, const obj_vnum vnum);

	void zone(const size_t rnum, const size_t zone_rnum) { m_index[rnum].zone = static_cast<int>(zone_rnum); }

	auto stored(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].stored : -1; }
	auto stored(const CObjectPrototype::shared_ptr& object) const { return stored(object->get_rnum()); }
	void dec_stored(const size_t rnum) { --m_index[rnum].stored; }
	void inc_stored(const size_t rnum) { ++m_index[rnum].stored; }

	auto number(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].number : -1; }
	auto number(const CObjectPrototype::shared_ptr& object) const { return number(object->get_rnum()); }
	void dec_number(const size_t rnum);
	void inc_number(const size_t rnum) { ++m_index[rnum].number; }

	auto zone(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].zone : -1; }

	auto actual_count(const size_t rnum) const { return number(rnum) + stored(rnum); }

	auto func(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].func : nullptr; }
	void func(const size_t rnum, const decltype(SPrototypeIndex::func) function) { m_index[rnum].func = function; }

	auto spec(const size_t rnum) const { return func(rnum); }

	auto set_idx(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].set_idx : ~0; }
	void set_idx(const size_t rnum, const decltype(SPrototypeIndex::set_idx) value) { m_index[rnum].set_idx = value; }

	int rnum(const obj_vnum vnum) const;

	void set(const size_t index, CObjectPrototype* new_value);

	auto index_size() const { return m_index.size()*(sizeof(index_t::value_type) + sizeof(vnum2index_t::value_type)); }
	auto prototypes_size() const { return m_prototypes.size() * sizeof(prototypes_t::value_type); }

	const auto& proto_script(const size_t rnum) const { return m_prototypes.at(rnum)->get_proto_script(); }
	const auto& vnum2index() const { return m_vnum2index; }

private:
	using vnum2index_t = std::map<obj_vnum, size_t>;

	bool is_index_safe(const size_t index) const;

	prototypes_t m_prototypes;
	index_t m_index;
	vnum2index_t m_vnum2index;
};

inline bool CObjectPrototypes::is_index_safe(const size_t index) const
{
	return index < m_index.size();
}

extern CObjectPrototypes obj_proto;

// returns the real number of the object with given virtual number
inline obj_rnum real_object(obj_vnum vnum) { return obj_proto.rnum(vnum); }

inline auto GET_OBJ_SPEC(const CObjectPrototype* obj)
{
	return obj_proto.spec(obj->get_rnum());
}

#endif // __OBJECT_PROTOTYPES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
