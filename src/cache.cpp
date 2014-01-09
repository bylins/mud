// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

// комментарий на русском в надежде починить кодировки bitbucket

#include "cache.hpp"
using namespace boost;

template class caching::Cache<Character*>;
template<class t>
caching::id_t caching::Cache<t>::max_id = 0;
caching::CharacterCache caching::character_cache;

template class caching::Cache<obj_data*>;
caching::ObjCache caching::obj_cache;

