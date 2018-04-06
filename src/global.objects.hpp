#ifndef __GLOBAL_OBJECTS_HPP__
#define __GLOBAL_OBJECTS_HPP__

#include "heartbeat.hpp"
#include "speedwalks.hpp"
#include "shutdown.parameters.hpp"
#include "shops.implementation.hpp"
#include "world.objects.hpp"
#include "world.characters.hpp"
#include "act.wizard.hpp"

class BanList;	// to avoid inclusion of ban.hpp

class GlobalObjects
{
public:
	static WorldObjects& world_objects();
	static ShopExt::ShopListType& shop_list();
	static Characters& characters();
	static ShutdownParameters& shutdown_parameters();
	static speedwalks_t& speedwalks();
	static InspReqListType& inspect_list();
	static SetAllInspReqListType& setall_inspect_list();
	static BanList*& ban();
	static Heartbeat& heartbeat();
};

#endif // __GLOBAL_OBJECTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
