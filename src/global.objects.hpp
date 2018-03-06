#ifndef __GLOBAL_OBJECTS_HPP__
#define __GLOBAL_OBJECTS_HPP__

#include "shops.implementation.hpp"
#include "world.objects.hpp"
#include "world.characters.hpp"

class GlobalObjects
{
public:
	static WorldObjects& world_objects();
	static ShopExt::ShopListType& shop_list();
	static Characters& characters();
};

#endif // __GLOBAL_OBJECTS_HPP__