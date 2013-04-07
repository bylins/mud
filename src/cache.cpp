// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

// комментарий на русском в надежде починить кодировки bitbucket

#include "cache.hpp"
using namespace boost;

template class caching::Cache<Character*>;
template<>
caching::id_t caching::CharacterCache::max_id = 0;
caching::CharacterCache caching::character_cache;

template class caching::Cache<obj_data*>;
template<>
caching::id_t caching::ObjCache::max_id = 0;
caching::ObjCache caching::obj_cache;

