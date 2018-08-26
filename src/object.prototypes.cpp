#include "object.prototypes.hpp"

#include "logger.hpp"

size_t CObjectPrototypes::add(CObjectPrototype* prototype, const obj_vnum vnum)
{
	return add(CObjectPrototype::shared_ptr(prototype, [&](auto ptr) { delete (OBJ_DATA *)ptr; }), vnum);
}

size_t CObjectPrototypes::add(const CObjectPrototype::shared_ptr& prototype, const obj_vnum vnum)
{
	const auto index = m_index.size();
	prototype->set_rnum(static_cast<int>(index));
	m_vnum2index[vnum] = index;
	m_prototypes.push_back(prototype);
	m_index.push_back(SPrototypeIndex());
	return index;
}

void CObjectPrototypes::dec_number(const size_t rnum)
{
	if (0 == m_index[rnum].number)
	{
		log("SYSERR: Attempt to decrement number of objects that does not exist at all (0 == number).");
		return;
	}
	--m_index[rnum].number;
}

int CObjectPrototypes::rnum(const obj_vnum vnum) const
{
	vnum2index_t::const_iterator i = m_vnum2index.find(vnum);
	return i == m_vnum2index.end() ? -1 : static_cast<int>(i->second);
}

void CObjectPrototypes::set(const size_t index, CObjectPrototype* new_value)
{
	new_value->set_rnum(static_cast<int>(index));
	m_prototypes[index].reset(new_value);
}

CObjectPrototypes obj_proto;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
