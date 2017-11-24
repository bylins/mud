#ifndef __GLOBAL_OBJECTS_HPP__
#define __GLOBAL_OBJECTS_HPP__

#include "shops.implementation.hpp"
#include "world.objects.hpp"

class GlobalObjects
{
public:
	static WorldObjects& world_objects();
	static ShopExt::ShopListType& shop_list();
};

#endif // __GLOBAL_OBJECTS_HPP__