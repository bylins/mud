#ifndef GLOBAL_OBJECTS_HPP_
#define GLOBAL_OBJECTS_HPP_

#include "abilities/abilities_info.h"
#include "classes/classes_info.h"
#include "fightsystem/pk.h"
#include "game_mechanics/celebrates.h"
#include "utils/logger.h"
#include "heartbeat.h"
#include "speedwalks.h"
#include "cmd_god/shutdown_parameters.h"
#include "game_economics/shops_implementation.h"
#include "world_objects.h"
#include "entities/world_characters.h"
#include "act_wizard.h"
#include "influxdb.h"
#include "entities/zone.h"
#include "quests/daily_quest.h"
#include "skills_info.h"
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
	static abilities::AbilitiesInfo &Abilities();
	static SkillsInfo &Skills();
	static classes::ClassesInfo &Classes();
	static WorldObjects &world_objects();
	static ShopExt::ShopListType &Shops();
	static Characters &characters();
	static ShutdownParameters &shutdown_parameters();
	static Speedwalks &speedwalks();
	static InspReqListType &inspect_list();
	static SetAllInspReqListType &setall_inspect_list();
	static BanList *&ban();
	static Heartbeat &heartbeat();
	static influxdb::Sender &stats_sender();
	static OutputThread &output_thread();
	static ZoneTable &zone_table();

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

using MUD = GlobalObjects;

#endif // GLOBAL_OBJECTS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
