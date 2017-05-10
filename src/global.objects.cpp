#include "global.objects.hpp"

#include "world.objects.hpp"
#include "shop_ext.hpp"

// This struct defines order of creating and destroying global objects
struct GlobalObjectsStorage
{
	ShopExt::ShopListType m_shop_list;
	WorldObjects m_world_objects;
};

// This function ensures that global objects will be created at the moment of getting access to them
GlobalObjectsStorage& global_objects()
{
	static GlobalObjectsStorage storage;

	return storage;
}

WorldObjects& GlobalObjects::world_objects()
{
	return global_objects().m_world_objects;
}

ShopExt::ShopListType& GlobalObjects::shop_list()
{
	return global_objects().m_shop_list;
}