#ifndef GLOBAL_OBJECTS_HPP_
#define GLOBAL_OBJECTS_HPP_

#include "gameplay/abilities/abilities_info.h"
#include "gameplay/classes/pc_classes_info.h"
#include "gameplay/fight/pk.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/mechanics/mob_races.h"
#include "gameplay/magic/spells_info.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/points_intensity.h"
#include "gameplay/skills/skill_messages.h"
#include "gameplay/fight/fight_messages.h"
#include "gameplay/clans/ingr_chest_saver.h"
#include "gameplay/clans/chest_saver.h"
#include "gameplay/mechanics/celebrates.h"
#include "gameplay/mechanics/guilds.h"
#include "gameplay/communication/social.h"
#include "gameplay/mechanics/guild_messages.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/mechanics/regions.h"
#include "gameplay/mechanics/animate_dead.h"
#include "gameplay/mechanics/region_messages.h"
#include "engine/ui/system_messages.h"
#include "gameplay/mechanics/cities_messages.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/mechanics/pc_race_messages.h"
#include "gameplay/ai/special_messages.h"
#include "engine/entities/zone_types.h"
#include "gameplay/mechanics/rune_spells.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/abilities/feat_messages.h"
#include "utils/logger.h"
#include "engine/core/heartbeat.h"
#include "administration/shutdown_parameters.h"
#include "engine/ui/cmd_god/do_inspect.h"
#include "engine/scripting/dg_event.h"
#include "gameplay/economics/shops_implementation.h"
#include "engine/observability/provider.h"
#include "utils/logging/log_manager.h"
#include "world_objects.h"
#include "world_characters.h"
#include "engine/observability/provider.h"
#include "engine/entities/zone.h"
#include "gameplay/quests/daily_quest.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/skills/townportal.h"
#include "gameplay/mechanics/strengthening.h"
#include "engine/boot/cfg_manager.h"
#include "engine/ui/cmd_god/do_set_all.h"
#include "gameplay/classes/mob_classes_info.h"
#include "engine/db/player_index.h"
#include "engine/db/description.h"

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
	static cfg_manager::CfgManager &CfgManager();
	static abilities::AbilitiesInfo &Abilities();
	static const abilities::AbilityInfo &Ability(abilities::EAbility ability_id);
	static SkillsInfo &Skills();
	static const SkillInfo &Skill(ESkill skill_id);
	static spells::SpellsInfo &Spells();
	static const spells::SpellInfo &Spell(ESpell spell_id);
	static spells::SpellMessages &SpellMessages();
	static points_intensity::PointsIntensity &PointsIntensity();
	static skills::SkillMessages &SkillMessages();
    static fight::FightMessages &FightMessages();
	static feats::FeatsInfo &Feats();
	static feats::FeatMessages &FeatMessages();
	static const feats::FeatInfo &Feat(EFeat feat_id);
	static classes::ClassesInfo &Classes();
	static const classes::CharClassInfo &Class(ECharClass class_id);
	static mob_classes::MobClassesInfo &MobClasses();
	static const mob_classes:: MobClassInfo &MobClass(EMobClass mob_class_id);
	static guilds::GuildsInfo &Guilds();
	static communication::social::SocialsInfo &Socials();
	static guilds::GuildMessages &GuildMessages();
	static cities::CitiesInfo &Cities();
	static regions::RegionsInfo &Regions();
	static animate_dead::AnimateDeadInfo &AnimateDead();
	static regions::RegionMessages &RegionMessages();
	static system_messages::SystemMessages &SystemMessages();
	static cities::CityMessages &CityMessages();
	static player_races::PcRacesInfo &PcRaces();
	static player_races::RaceMessages &RaceMessages();
	static specials::SpecialMessages &SpecialMessages();
	static specials::BankMessages &BankMessages();
	static specials::MailMessages &MailMessages();
	static specials::HorseMessages &HorseMessages();
	static specials::TorcMessages &TorcMessages();
	static specials::MercMessages &MercMessages();
	static specials::ExchMessages &ExchMessages();
	static specials::RentMessages &RentMessages();
	static specials::ShopMessages &ShopMessages();
	static specials::BoardMessages &BoardMessages();
	static const guilds::GuildInfo &Guild(Vnum guild_vnum);
	static zone_types::ZoneTypesInfo &ZoneTypes();
	static const zone_types::ZoneTypeInfo &ZoneType(int type_vnum);
	static rune_spells::Registry &RuneSpells();
	static currencies::CurrenciesInfo &Currencies();
	static mob_races::MobRacesInfo &MobRaces();
	static const currencies::CurrencyInfo &Currency(Vnum currency_vnum);
	static WorldObjects &world_objects();
	static ShopExt::ShopListType &Shops();
	static Characters &characters();
	static ShutdownParameters &shutdown_parameters();
	static InspectRequestDeque &InspectRequests();
	static SetAllInspReqListType &setall_inspect_list();
	static BanList *&ban();
	static Heartbeat &heartbeat();
	static observability::OtelProvider &otel_provider();
	static OutputThread &output_thread();
	static logging::LogManager &log_manager();
	static ZoneTable &zone_table();
  	static RunestoneRoster &Runestones();

	static celebrates::CelebrateList &mono_celebrates();
	static celebrates::CelebrateList &poly_celebrates();
	static celebrates::CelebrateList &real_celebrates();
	static celebrates::CelebrateMobs &attached_mobs();
	static celebrates::CelebrateMobs &loaded_mobs();
	static celebrates::CelebrateObjs &attached_objs();
	static celebrates::CelebrateObjs &loaded_objs();

	static GlobalTriggersStorage &trigger_list();
	static TriggerEventList_t &trigger_event_list();
	static BloodyInfoMap &bloody_map();
	static Rooms &world();
	static PlayersIndex &player_table();
	static DailyQuest::DailyQuestMap &daily_quests();
	static Strengthening &strengthening();
	static obj2triggers_t &obj_triggers();
	static RoomDescriptions &descriptions();
	static ClanSystem::IngrChestSaver &ingr_chest_saver();
	static ClanSystem::ChestSaver &chest_saver();
};

using MUD = GlobalObjects;

#endif // GLOBAL_OBJECTS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
