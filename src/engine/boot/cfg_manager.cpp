/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер файлов конфигурации должен хранить информацию об именах и местоположении файлов конфигурации,
 порядке их загрузки и управлять процессом загрузки данных из файлов по контейнерам.
 */

#include "cfg_manager.h"

//#include  "feats.h"
#include "gameplay/abilities/abilities_info.h"
#include "gameplay/classes/pc_classes_info.h"
#include "gameplay/classes/mob_classes_info.h"
#include "gameplay/magic/spells_info.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/mechanics/guilds.h"
#include "engine/entities/zone_types.h"
#include "gameplay/mechanics/rune_spells.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/skills/skill_messages.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/points_intensity.h"
#include "gameplay/fight/fight_messages.h"

namespace cfg_manager {

CfgManager::CfgManager() {
/* loaders_.emplace("mobraces", "cfg/mob_races.xml");*/
	loaders_.emplace("currencies", LoaderInfo("cfg/economics/currencies.xml",
											  std::make_unique<currencies::CurrenciesLoader>(currencies::CurrenciesLoader())));
	loaders_.emplace("classes", LoaderInfo("cfg/classes/pc_classes.xml",
										   std::make_unique<classes::ClassesLoader>(classes::ClassesLoader())));
	loaders_.emplace("skills", LoaderInfo("cfg/skills.xml",
										  std::make_unique<SkillsLoader>(SkillsLoader())));
	loaders_.emplace("skill_messages", LoaderInfo("cfg/skill_msg.xml",
										  std::make_unique<skills::SkillMessagesLoader>(skills::SkillMessagesLoader())));
	loaders_.emplace("abilities", LoaderInfo("cfg/abilities.xml",
										  std::make_unique<abilities::AbilitiesLoader>(abilities::AbilitiesLoader())));
	loaders_.emplace("spells", LoaderInfo("cfg/spells.xml",
										  std::make_unique<spells::SpellsLoader>(spells::SpellsLoader())));
	loaders_.emplace("spell_messages", LoaderInfo("cfg/spell_msg.xml",
										  std::make_unique<spells::SpellMessagesLoader>(spells::SpellMessagesLoader())));
	loaders_.emplace("points_intensity", LoaderInfo("cfg/mechanics/points.xml",
										  std::make_unique<points_intensity::PointsIntensityLoader>(points_intensity::PointsIntensityLoader())));
	loaders_.emplace("fight_messages", LoaderInfo("cfg/hit_msg.xml",
										  std::make_unique<fight::FightMessagesLoader>(fight::FightMessagesLoader())));
	loaders_.emplace("feats", LoaderInfo("cfg/feats.xml",
										  std::make_unique<feats::FeatsLoader>(feats::FeatsLoader())));
	loaders_.emplace("guilds", LoaderInfo("cfg/guilds.xml",
										  std::make_unique<guilds::GuildsLoader>(guilds::GuildsLoader())));
	loaders_.emplace("zone_types", LoaderInfo("cfg/zone_types.xml",
										  std::make_unique<zone_types::ZoneTypesLoader>(zone_types::ZoneTypesLoader())));
	loaders_.emplace("rune_spells", LoaderInfo("cfg/mechanics/rune_spells.xml",
										  std::make_unique<rune_spells::RuneSpellsLoader>(rune_spells::RuneSpellsLoader())));
	loaders_.emplace("mob_classes", LoaderInfo("cfg/mob_classes.xml",
								  std::make_unique<mob_classes::MobClassesLoader>(mob_classes::MobClassesLoader())));
}

void CfgManager::ReloadCfg(const std::string &id) {
	if (!loaders_.contains(id)) {
		err_log("Неверный параметр перезагрузки файла конфигурации (%s)", id.c_str());
		return;
	}
	const auto &loader_info = loaders_.at(id);
	auto data = parser_wrapper::DataNode(loader_info.file);
	loader_info.loader->Reload(data);
}

void CfgManager::LoadCfg(const std::string &id) {
	if (!loaders_.contains(id)) {
		err_log("Неверный параметр загрузки файла конфигурации (%s)", id.c_str());
		return;
	}
	const auto &loader_info = loaders_.at(id);
	auto data = parser_wrapper::DataNode(loader_info.file);
	loader_info.loader->Load(data);
}

} // namespace cfg manager

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
