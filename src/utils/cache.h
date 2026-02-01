// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

#ifndef CACHE_HPP_INCLUDED
#define CACHE_HPP_INCLUDED

#include <unordered_map>
#include <mutex>

class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
class ObjData;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.

namespace caching {

using IdType = unsigned int;

template<class CashedType>
class Cache {
 public:
	using IdMap = std::unordered_map<IdType, CashedType>;
	using PtrMap = std::unordered_map<CashedType, IdType>;

	Cache() = default;
	~Cache() = default;

	inline void Add(CashedType obj) {
		std::lock_guard<std::mutex> lock{m_mutex};
		IdType id = ++max_id;
		id_map[id] = obj;
		ptr_map[obj] = id;
	}

	inline void Remove(CashedType obj) {
		std::lock_guard<std::mutex> lock{m_mutex};
		auto it = ptr_map.find(obj);
		if (it != ptr_map.end()) {
			id_map.erase(it->second);
			ptr_map.erase(it);
		}
	}

	inline CashedType GetItem(const IdType &id) const {
		auto it = id_map.find(id);
		if (it != id_map.end())
			return it->second;
		else return nullptr;
	}

	inline IdType GetId(CashedType &obj) {
		auto it = ptr_map.find(obj);
		if (it != ptr_map.end())
			return it->second;
		else return 0;
	}

 private:
	IdMap id_map;
	PtrMap ptr_map;
	static IdType max_id;
	mutable std::mutex m_mutex;  // Protects id_map, ptr_map, and max_id from concurrent access
};

using CharacterCache = Cache<CharData *>;
extern CharacterCache character_cache;

using ObjCache = Cache<ObjData *>;
extern ObjCache obj_cache;

}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
