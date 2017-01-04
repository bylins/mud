// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

#ifndef CACHE_HPP_INCLUDED
#define CACHE_HPP_INCLUDED

#include <boost/unordered_map.hpp>

class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
class OBJ_DATA;	// forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.

namespace caching
{
typedef unsigned int id_t;
template <class t>
class Cache
{
public:
typedef t cached_t;
typedef boost::unordered_map<const id_t, cached_t> id_map_t;
typedef boost::unordered_map<cached_t, id_t> ptr_map_t;

inline void add(t obj)
{
	id_t id = ++max_id;
	id_map[id] = obj;
	ptr_map[obj] = id;
}

inline void remove(t obj)
{
	typename ptr_map_t::iterator it = ptr_map.find(obj);
	if (it != ptr_map.end())
{
		id_map.erase(it->second);
		ptr_map.erase(it);
	}
}

inline t get_obj(const id_t& id) const
{
	typename id_map_t::const_iterator it = id_map.find(id);
	if (it != id_map.end())
		return it->second;
	else return NULL; //dirty
}

inline id_t get_id(t& obj)
{
	typename ptr_map_t::iterator it = ptr_map.find(obj);
	if (it != ptr_map.end())
		return it->second;
	else return 0; //dirty
}

private:
id_map_t id_map;
ptr_map_t ptr_map;
static id_t max_id;
};
typedef Cache<CHAR_DATA*> CharacterCache;
extern CharacterCache character_cache;

typedef Cache<OBJ_DATA*> ObjCache;
extern ObjCache obj_cache;
}

#endif
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
