#ifndef __GLOBAL_OBJECTS_HPP__
#define __GLOBAL_OBJECTS_HPP__

#include "fightsystem/pk.h"
#include "game_mechanics/celebrates.h"
#include "logger.h"
#include "heartbeat.h"
#include "speedwalks.h"
#include "cmd_god/shutdown_parameters.h"
#include "shops_implementation.h"
#include "world_objects.h"
#include "entities/world.characters.h"
#include "act_wizard.h"
#include "influxdb.h"
#include "zone.table.h"
#include "quests/daily_quest.h"
#include "strengthening.h"
class BanList;    // to avoid inclusion of ban.hpp

/**
* This class is going to be very large. Probably instead of declaring all global objects as a static functions
* and include *all* necessary headers we should consider hiding all these functions in .cpp file and declare them
* where they are needed. For example:
*
* > global.objects.cpp:
* CelebrateList& mono_celebrates_getted() { return global_objects().mono_celebrates; }
*
* > file.where.mono_celebrates.is.needed.cpp:
* extern CelebrateList& mono_celebrates_getted();
* CelebrateList& mono_celebrates = mono_celebrates_getted();
*/
class GlobalObjects {
 public:
	static WorldObjects &world_objects();
	static ShopExt::ShopListType &shop_list();
	static Characters &characters();
	static ShutdownParameters &shutdown_parameters();
	static speedwalks_t &speedwalks();
	static InspReqListType &inspect_list();
	static SetAllInspReqListType &setall_inspect_list();
	static BanList *&ban();
	static Heartbeat &heartbeat();
	static influxdb::Sender &stats_sender();
	static OutputThread &output_thread();
	static zone_table_t &zone_table();

	static Celebrates::CelebrateList &mono_celebrates();
	static Celebrates::CelebrateList &poly_celebrates();
	static Celebrates::CelebrateList &real_celebrates();
	static Celebrates::CelebrateMobs &attached_mobs();
	static Celebrates::CelebrateMobs &loaded_mobs();
	static Celebrates::CelebrateObjs &attached_objs();
	static Celebrates::CelebrateObjs &loaded_objs();

	static GlobalTriggersStorage &trigger_list();
	static BloodyInfoMap &bloody_map();
	static Rooms &world();
	static PlayersIndex &player_table();
	static DailyQuest::DailyQuestMap &daily_quests();
	static Strengthening &strengthening();
	static obj2triggers_t &obj_triggers();
};

#endif // __GLOBAL_OBJECTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
