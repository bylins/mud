#include "global.objects.hpp"

#include "ban.hpp"

namespace
{
	// This struct defines order of creating and destroying global objects
	struct GlobalObjectsStorage
	{
		GlobalObjectsStorage();

		WorldObjects world_objects;
		ShopExt::ShopListType shop_list;
		Characters characters;
		ShutdownParameters shutdown_parameters;
		speedwalks_t speedwalks;
		SetAllInspReqListType setall_inspect_list;
		InspReqListType inspect_list;
		BanList* ban;
		Heartbeat heartbeat;
	};

	GlobalObjectsStorage::GlobalObjectsStorage() :
		ban(nullptr)
	{
	}

	// This function ensures that global objects will be created at the moment of getting access to them
	GlobalObjectsStorage& global_objects()
	{
		static GlobalObjectsStorage storage;

		return storage;
	}
}

WorldObjects& GlobalObjects::world_objects()
{
	return global_objects().world_objects;
}

ShopExt::ShopListType& GlobalObjects::shop_list()
{
	return global_objects().shop_list;
}

Characters& GlobalObjects::characters()
{
	return global_objects().characters;
}

ShutdownParameters& GlobalObjects::shutdown_parameters()
{
	return global_objects().shutdown_parameters;
}

speedwalks_t& GlobalObjects::speedwalks()
{
	return global_objects().speedwalks;
}

InspReqListType& GlobalObjects::inspect_list()
{
	return global_objects().inspect_list;
}

SetAllInspReqListType& GlobalObjects::setall_inspect_list()
{
	return global_objects().setall_inspect_list;
}

BanList*& GlobalObjects::ban()
{
	return global_objects().ban;
}

Heartbeat& GlobalObjects::heartbeat()
{
	return global_objects().heartbeat;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
