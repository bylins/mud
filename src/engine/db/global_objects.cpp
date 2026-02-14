#include "global_objects.h"

#include <memory>

#include "administration/ban.h"

namespace {
// This struct defines order of creating and destroying global objects
struct GlobalObjectsStorage {
	GlobalObjectsStorage();

	/// This object should be destroyed last because it serves all output operations. So I define it first.
	std::shared_ptr<OutputThread> output_thread;

	celebrates::CelebrateList mono_celebrates;
	celebrates::CelebrateList poly_celebrates;
	celebrates::CelebrateList real_celebrates;
	celebrates::CelebrateMobs attached_mobs;
	celebrates::CelebrateMobs loaded_mobs;
	celebrates::CelebrateObjs attached_objs;
	celebrates::CelebrateObjs loaded_objs;

	// to be destroyed last to avoid crashes in other destructors during shutdown
	TriggerEventList_t trigger_event_list;

	GlobalTriggersStorage trigger_list;
	Rooms world;
	BloodyInfoMap bloody_map;

	abilities::AbilitiesInfo abilities_info;
	SkillsInfo skills_info;
	spells::SpellsInfo spells_info;
	feats::FeatsInfo feats_info;
	cfg_manager::CfgManager cfg_mngr;
	classes::ClassesInfo classes_info;
	mob_classes::MobClassesInfo mob_classes_info;
	guilds::GuildsInfo guilds_info;
	currencies::CurrenciesInfo currencies_info;
  	RunestoneRoster runestone_roster;
	WorldObjects world_objects;
	ShopExt::ShopListType shop_list;
	PlayersIndex player_table;
	Characters characters;
	ShutdownParameters shutdown_parameters;
	SetAllInspReqListType setall_inspect_list;
  	InspectRequestDeque inspect_request_deque;
	BanList *ban;
	Heartbeat heartbeat;
	std::shared_ptr<influxdb::Sender> stats_sender;
	ZoneTable zone_table;
	DailyQuest::DailyQuestMap daily_quests;
	Strengthening strengthening;
	obj2triggers_t obj2triggers;
	RoomDescriptions room_descriptions;
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

const abilities::AbilityInfo &GlobalObjects::Ability(abilities::EAbility ability_id) {
	return global_objects().abilities_info[ability_id];
}

SkillsInfo &GlobalObjects::Skills() {
	return global_objects().skills_info;
}

const SkillInfo &GlobalObjects::Skill(ESkill skill_id) {
	return global_objects().skills_info[skill_id];
}

spells::SpellsInfo &GlobalObjects::Spells() {
	return global_objects().spells_info;
}

const spells::SpellInfo &GlobalObjects::Spell(ESpell spell_id) {
	return global_objects().spells_info[spell_id];
}

feats::FeatsInfo &GlobalObjects::Feats() {
	return global_objects().feats_info;
}

const feats::FeatInfo &GlobalObjects::Feat(const EFeat feat_id) {
	return global_objects().feats_info[feat_id];
}

cfg_manager::CfgManager &GlobalObjects::CfgManager() {
	return global_objects().cfg_mngr;
}

classes::ClassesInfo &GlobalObjects::Classes() {
	return global_objects().classes_info;
}

const classes::CharClassInfo &GlobalObjects::Class(ECharClass class_id) {
	return global_objects().classes_info[class_id];
}

mob_classes::MobClassesInfo &GlobalObjects::MobClasses() {
	return global_objects().mob_classes_info;
}

const mob_classes::MobClassInfo &GlobalObjects::MobClass(EMobClass mob_class_id) {
	return global_objects().mob_classes_info[mob_class_id];
}

guilds::GuildsInfo &GlobalObjects::Guilds() {
	return global_objects().guilds_info;
}

const guilds::GuildInfo &GlobalObjects::Guild(Vnum guild_vnum) {
	return global_objects().guilds_info[guild_vnum];
}

currencies::CurrenciesInfo &GlobalObjects::Currencies() {
	return global_objects().currencies_info;
};

const currencies::CurrencyInfo &GlobalObjects::Currency(Vnum currency_vnum) {
	return global_objects().currencies_info[currency_vnum];
};

RunestoneRoster &GlobalObjects::Runestones() {
	return global_objects().runestone_roster;
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

InspectRequestDeque &GlobalObjects::InspectRequests() {
	return global_objects().inspect_request_deque;
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
		global_objects().stats_sender = std::make_shared<influxdb::Sender>(
			runtime_config.statistics().host(), runtime_config.statistics().port());
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

celebrates::CelebrateList &GlobalObjects::mono_celebrates() {
	return global_objects().mono_celebrates;
}

celebrates::CelebrateList &GlobalObjects::poly_celebrates() {
	return global_objects().poly_celebrates;
}

celebrates::CelebrateList &GlobalObjects::real_celebrates() {
	return global_objects().real_celebrates;
}

celebrates::CelebrateMobs &GlobalObjects::attached_mobs() {
	return global_objects().attached_mobs;
}

celebrates::CelebrateMobs &GlobalObjects::loaded_mobs() {
	return global_objects().loaded_mobs;
}

celebrates::CelebrateObjs &GlobalObjects::attached_objs() {
	return global_objects().attached_objs;
}

celebrates::CelebrateObjs &GlobalObjects::loaded_objs() {
	return global_objects().loaded_objs;
}

GlobalTriggersStorage &GlobalObjects::trigger_list() {
	return global_objects().trigger_list;
}

TriggerEventList_t &GlobalObjects::trigger_event_list() {
	return global_objects().trigger_event_list;
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

RoomDescriptions &GlobalObjects::descriptions() {
	return global_objects().room_descriptions;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
