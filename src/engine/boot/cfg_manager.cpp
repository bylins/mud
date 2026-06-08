/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер файлов конфигурации должен хранить информацию об именах и местоположении файлов конфигурации,
 порядке их загрузки и управлять процессом загрузки данных из файлов по контейнерам.
 */

#include "cfg_manager.h"

#include "utils/utils_time.h"   // issue.xml-cfg-parsing: reload timing
#include "utils/logger.h"

//#include  "feats.h"
#include "gameplay/abilities/abilities_info.h"
#include "gameplay/classes/pc_classes_info.h"
#include "gameplay/classes/mob_classes_info.h"
#include "gameplay/magic/spells_info.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/mechanics/guilds.h"
#include "gameplay/communication/social.h"
#include "gameplay/ai/spec_assign.h"
#include "gameplay/mechanics/mob_races.h"
#include "engine/entities/zone_types.h"
#include "gameplay/mechanics/rune_spells.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/skills/skill_messages.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/points_intensity.h"
#include "gameplay/fight/fight_messages.h"
#include "gameplay/abilities/feat_messages.h"
#include "gameplay/core/entity_names.h"
#include "gameplay/mechanics/guild_messages.h"
#include "gameplay/ai/special_messages.h"

namespace cfg_manager {

CfgManager::CfgManager() {
	loaders_.emplace("entity_names", LoaderInfo("cfg/messages/ru/entity_names.xml",
										  std::make_unique<entity_names::EntityNamesLoader>(entity_names::EntityNamesLoader())));
	loaders_.emplace("mob_races", LoaderInfo("cfg/mechanics/mob_races.xml",
											  std::make_unique<mob_races::MobRacesLoader>(mob_races::MobRacesLoader())));
	loaders_.emplace("currency_messages", LoaderInfo("cfg/messages/ru/currency_msg.xml",
											  std::make_unique<currencies::CurrencyNamesLoader>(currencies::CurrencyNamesLoader())));
	loaders_.emplace("currencies", LoaderInfo("cfg/economics/currencies.xml",
											  std::make_unique<currencies::CurrenciesLoader>(currencies::CurrenciesLoader())));
	loaders_.emplace("class_messages", LoaderInfo("cfg/messages/ru/class_msg.xml",
										  std::make_unique<classes::ClassNamesLoader>(classes::ClassNamesLoader())));
	loaders_.emplace("classes", LoaderInfo("cfg/classes/pc_classes.xml",
										   std::make_unique<classes::ClassesLoader>(classes::ClassesLoader())));
	loaders_.emplace("skills", LoaderInfo("cfg/skills.xml",
										  std::make_unique<SkillsLoader>(SkillsLoader())));
	loaders_.emplace("skill_messages", LoaderInfo("cfg/messages/ru/skill_msg.xml",
										  std::make_unique<skills::SkillMessagesLoader>(skills::SkillMessagesLoader())));
	loaders_.emplace("abilities", LoaderInfo("cfg/abilities.xml",
										  std::make_unique<abilities::AbilitiesLoader>(abilities::AbilitiesLoader())));
	loaders_.emplace("spells", LoaderInfo("cfg/spells.xml",
										  std::make_unique<spells::SpellsLoader>(spells::SpellsLoader())));
	loaders_.emplace("spell_messages", LoaderInfo("cfg/messages/ru/spell_msg.xml",
										  std::make_unique<spells::SpellMessagesLoader>(spells::SpellMessagesLoader())));
	loaders_.emplace("points_intensity", LoaderInfo("cfg/messages/ru/points_intensity.xml",
										  std::make_unique<points_intensity::PointsIntensityLoader>(points_intensity::PointsIntensityLoader())));
	loaders_.emplace("fight_messages", LoaderInfo("cfg/messages/ru/hit_msg.xml",
										  std::make_unique<fight::FightMessagesLoader>(fight::FightMessagesLoader())));
	loaders_.emplace("feat_messages", LoaderInfo("cfg/messages/ru/feat_msg.xml",
										  std::make_unique<feats::FeatMessagesLoader>(feats::FeatMessagesLoader())));
	loaders_.emplace("feats", LoaderInfo("cfg/feats.xml",
										  std::make_unique<feats::FeatsLoader>(feats::FeatsLoader())));
	loaders_.emplace("guild_messages", LoaderInfo("cfg/messages/ru/guild_msg.xml",
										  std::make_unique<guilds::GuildMessagesLoader>(guilds::GuildMessagesLoader())));
	loaders_.emplace("special_messages", LoaderInfo("cfg/messages/ru/special_msg.xml",
										  std::make_unique<specials::SpecialMessagesLoader>(specials::SpecialMessagesLoader())));
	loaders_.emplace("bank_messages", LoaderInfo("cfg/messages/ru/bank_msg.xml",
										  std::make_unique<specials::BankMessagesLoader>(specials::BankMessagesLoader())));
	loaders_.emplace("mail_messages", LoaderInfo("cfg/messages/ru/mail_msg.xml",
										  std::make_unique<specials::MailMessagesLoader>(specials::MailMessagesLoader())));
	loaders_.emplace("horse_messages", LoaderInfo("cfg/messages/ru/horse_msg.xml",
										  std::make_unique<specials::HorseMessagesLoader>(specials::HorseMessagesLoader())));
	loaders_.emplace("torc_messages", LoaderInfo("cfg/messages/ru/torc_msg.xml",
										  std::make_unique<specials::TorcMessagesLoader>(specials::TorcMessagesLoader())));
	loaders_.emplace("mercenary_messages", LoaderInfo("cfg/messages/ru/mercenary_msg.xml",
										  std::make_unique<specials::MercMessagesLoader>(specials::MercMessagesLoader())));
	loaders_.emplace("exchange_messages", LoaderInfo("cfg/messages/ru/exchange_msg.xml",
										  std::make_unique<specials::ExchMessagesLoader>(specials::ExchMessagesLoader())));
	loaders_.emplace("specials", LoaderInfo("cfg/specials.xml",
										  std::make_unique<SpecialsLoader>(SpecialsLoader())));
	loaders_.emplace("socials", LoaderInfo("cfg/messages/ru/social_msg.xml",
										  std::make_unique<communication::social::SocialsLoader>(communication::social::SocialsLoader())));
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
	// issue.xml-cfg-parsing: time the two phases separately -- DOM parse (file read + pugixml) vs
	// build+swap (constructing the new register) -- so we know where the hot-reload lag really is.
	utils::CExecutionTimer parse_timer;
	auto data = parser_wrapper::DataNode(loader_info.file);
	const double parse_ms = parse_timer.delta().count() * 1000.0;
	utils::CExecutionTimer build_timer;
	loader_info.loader->Reload(data);
	const double build_ms = build_timer.delta().count() * 1000.0;
	log("Cfg reload [%s]: parse %.2f ms + build/swap %.2f ms = %.2f ms (%s)",
		id.c_str(), parse_ms, build_ms, parse_ms + build_ms, loader_info.file.string().c_str());
}

void CfgManager::LoadCfg(const std::string &id) {
	if (!loaders_.contains(id)) {
		err_log("Неверный параметр загрузки файла конфигурации (%s)", id.c_str());
		return;
	}
	const auto &loader_info = loaders_.at(id);
	// issue.xml-cfg-parsing: boot-time baseline of the same two phases.
	utils::CExecutionTimer parse_timer;
	auto data = parser_wrapper::DataNode(loader_info.file);
	const double parse_ms = parse_timer.delta().count() * 1000.0;
	utils::CExecutionTimer build_timer;
	loader_info.loader->Load(data);
	const double build_ms = build_timer.delta().count() * 1000.0;
	log("Cfg load [%s]: parse %.2f ms + build %.2f ms = %.2f ms (%s)",
		id.c_str(), parse_ms, build_ms, parse_ms + build_ms, loader_info.file.string().c_str());
}

// issue.vedun-editor: enumerate loaders that opted into editing (implement IEditableCfgLoader).
std::vector<CfgManager::EditableEntry> CfgManager::EditableEntries() const {
	std::vector<EditableEntry> out;
	for (const auto &[id, info] : loaders_) {
		if (auto *editable = dynamic_cast<IEditableCfgLoader *>(info.loader.get())) {
			out.push_back({editable->EditableWhat(), info.file, editable});
		}
	}
	return out;
}

std::optional<CfgManager::EditableEntry> CfgManager::FindEditable(const std::string &what) const {
	for (auto &entry : EditableEntries()) {
		if (entry.what == what) {
			return entry;
		}
	}
	return std::nullopt;
}

} // namespace cfg manager

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
