#include "global_objects.h"

#include "cmd_god/ban.h"

namespace {
// This struct defines order of creating and destroying global objects
struct GlobalObjectsStorage {
	GlobalObjectsStorage();

	/// This object should be destroyed last because it serves all output operations. So I define it first.
	std::shared_ptr<OutputThread> output_thread;

	Celebrates::CelebrateList mono_celebrates;
	Celebrates::CelebrateList poly_celebrates;
	Celebrates::CelebrateList real_celebrates;
	Celebrates::CelebrateMobs attached_mobs;
	Celebrates::CelebrateMobs loaded_mobs;
	Celebrates::CelebrateObjs attached_objs;
	Celebrates::CelebrateObjs loaded_objs;

	GlobalTriggersStorage trigger_list;
	Rooms world;
	BloodyInfoMap bloody_map;

	abilities::AbilitiesInfo abilities_info;
	SkillsInfo skills_info;
	cfg_manager::CfgManager cfg_mngr;
	classes::ClassesInfo classes_info;
	WorldObjects world_objects;
	ShopExt::ShopListType shop_list;
	PlayersIndex player_table;
	Characters characters;
	ShutdownParameters shutdown_parameters;
	Speedwalks speedwalks;
	SetAllInspReqListType setall_inspect_list;
	InspReqListType inspect_list;
	BanList *ban;
	Heartbeat heartbeat;
	std::shared_ptr<influxdb::Sender> stats_sender;
	ZoneTable zone_table;
	DailyQuest::DailyQuestMap daily_quests;
	Strengthening strengthening;
	obj2triggers_t obj2triggers;
};

GlobalObjectsStorage::GlobalObjectsStorage() :
	ban(nullptr) {
}

// This function ensures that global objects will be created at the moment of getting access to them
GlobalObjectsStorage &global_objects() {
	static GlobalObjectsStorage storage;

	return storage;
}
}

abilities::AbilitiesInfo &GlobalObjects::Abilities() {
	return global_objects().abilities_info;
}

SkillsInfo &GlobalObjects::Skills() {
	return global_objects().skills_info;
}

cfg_manager::CfgManager &GlobalObjects::CfgManager() {
	return global_objects().cfg_mngr;
}

classes::ClassesInfo &GlobalObjects::Classes() {
	return global_objects().classes_info;
}

WorldObjects &GlobalObjects::world_objects() {
	return global_objects().world_objects;
}

ShopExt::ShopListType &GlobalObjects::Shops() {
	return global_objects().shop_list;
}

Characters &GlobalObjects::characters() {
	return global_objects().characters;
}

ShutdownParameters &GlobalObjects::shutdown_parameters() {
	return global_objects().shutdown_parameters;
}

Speedwalks &GlobalObjects::speedwalks() {
	return global_objects().speedwalks;
}

InspReqListType &GlobalObjects::inspect_list() {
	return global_objects().inspect_list;
}

SetAllInspReqListType &GlobalObjects::setall_inspect_list() {
	return global_objects().setall_inspect_list;
}

BanList *&GlobalObjects::ban() {
	return global_objects().ban;
}

Heartbeat &GlobalObjects::heartbeat() {
	return global_objects().heartbeat;
}

influxdb::Sender &GlobalObjects::stats_sender() {
	if (!global_objects().stats_sender) {
		global_objects().stats_sender.reset(new influxdb::Sender(
			runtime_config.statistics().host(), runtime_config.statistics().port()));
	}

	return *global_objects().stats_sender;
}

OutputThread &GlobalObjects::output_thread() {
	if (!global_objects().output_thread) {
		global_objects().output_thread.reset(new OutputThread(runtime_config.output_queue_size()));
	}

	return *global_objects().output_thread;
}

ZoneTable &GlobalObjects::zone_table() {
	return global_objects().zone_table;
}

Celebrates::CelebrateList &GlobalObjects::mono_celebrates() {
	return global_objects().mono_celebrates;
}

Celebrates::CelebrateList &GlobalObjects::poly_celebrates() {
	return global_objects().poly_celebrates;
}

Celebrates::CelebrateList &GlobalObjects::real_celebrates() {
	return global_objects().real_celebrates;
}

Celebrates::CelebrateMobs &GlobalObjects::attached_mobs() {
	return global_objects().attached_mobs;
}

Celebrates::CelebrateMobs &GlobalObjects::loaded_mobs() {
	return global_objects().loaded_mobs;
}

Celebrates::CelebrateObjs &GlobalObjects::attached_objs() {
	return global_objects().attached_objs;
}

Celebrates::CelebrateObjs &GlobalObjects::loaded_objs() {
	return global_objects().loaded_objs;
}

GlobalTriggersStorage &GlobalObjects::trigger_list() {
	return global_objects().trigger_list;
}

BloodyInfoMap &GlobalObjects::bloody_map() {
	return global_objects().bloody_map;
}

Rooms &GlobalObjects::world() {
	return global_objects().world;
}

PlayersIndex &GlobalObjects::player_table() {
	return global_objects().player_table;
}

DailyQuest::DailyQuestMap &GlobalObjects::daily_quests() {
	return global_objects().daily_quests;
}

Strengthening &GlobalObjects::strengthening() {
	return global_objects().strengthening;
}

obj2triggers_t &GlobalObjects::obj_triggers() {
	return global_objects().obj2triggers;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
