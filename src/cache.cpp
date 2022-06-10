// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru


#include "cache.h"

namespace caching {

template
class Cache<CharData *>;

template<class CashedType>
IdType Cache<CashedType>::max_id = 0;

CharacterCache character_cache;

template
class Cache<ObjData *>;

ObjCache obj_cache;

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
