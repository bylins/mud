/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер файлов конфигурации должен хранить информацию об именах и местоположении файлов конфигурации,
 порядке их загрузки и управлять процессом загрузки данных из файлов по контейнерам.
 */

#include "cfg_manager.h"

#include <string_view>

#include "utils/utils_time.h"   // issue.xml-cfg-parsing: reload timing
#include "utils/logger.h"

//#include  "feats.h"
#include "gameplay/abilities/abilities_info.h"
#include "gameplay/classes/pc_classes_info.h"
#include "gameplay/core/experience.h"
#include "gameplay/economics/shop_ext.h"
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
#include "gameplay/mechanics/cities.h"
#include "gameplay/mechanics/regions.h"
#include "gameplay/mechanics/region_messages.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/reset_stats.h"
#include "gameplay/mechanics/noob.h"
#include "gameplay/mechanics/celebrates.h"
#include "gameplay/mechanics/city_guards.h"
#include "gameplay/quests/daily_quest.h"
#include "gameplay/mechanics/obj_sets.h"
#include "gameplay/mechanics/treasure_cases.h"
#include "gameplay/mechanics/cities_messages.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/mechanics/pc_race_messages.h"
#include "gameplay/ai/special_messages.h"
#include "gameplay/affects/affects_loader.h"
#include "gameplay/affects/affect_messages.h"
#include "engine/core/common_messages.h"
#include "gameplay/mechanics/rune_stones_loaders.h"
#include "administration/privilege_db.h"
#include "gameplay/crafting/mining.h"
#include "gameplay/crafting/jewelry.h"
#include "gameplay/crafting/item_creation.h"
#include "gameplay/core/basic_values.h"

namespace cfg_manager {

namespace {
// issue.cfg-manager: фабрика загрузчика без сырого new (unique_ptr<Derived> -> LoaderPtr).
template<class T>
LoaderPtr MakeLoader() {
	return std::make_unique<T>();
}

// issue.cfg-manager: единая декларативная таблица регистрации загрузчиков - id, путь к файлу
// конфига и фабрика. Заменяет ~60 повторяющихся loaders_.emplace(...): все id и пути собраны
// здесь, в одном месте (внутренние константы менеджера).
struct Registration {
	std::string_view id;
	std::string_view path;
	LoaderPtr (*make)();
};

} // namespace

CfgManager::CfgManager() {
	const Registration registry[] = {
		{"entity_names",        "cfg/messages/ru/entity_names.xml",     [] { return MakeLoader<entity_names::EntityNamesLoader>(); }},
		{"mob_races",           "cfg/mechanics/mob_races.xml",          [] { return MakeLoader<mob_races::MobRacesLoader>(); }},
		{"currency_messages",   "cfg/messages/ru/currency_msg.xml",     [] { return MakeLoader<currencies::CurrencyNamesLoader>(); }},
		{"shop_item_sets",      "cfg/economics/shop_item_sets.xml",     [] { return MakeLoader<ShopExt::ShopItemSetsLoader>(); }},
		{"shops",               "cfg/economics/shops.xml",              [] { return MakeLoader<ShopExt::ShopsLoader>(); }},
		{"currencies",          "cfg/economics/currencies.xml",         [] { return MakeLoader<currencies::CurrenciesLoader>(); }},
		{"class_messages",      "cfg/messages/ru/class_msg.xml",        [] { return MakeLoader<classes::ClassNamesLoader>(); }},
		{"classes",             "cfg/classes/pc_classes.xml",           [] { return MakeLoader<classes::ClassesLoader>(); }},
		{"experience_table",    "cfg/experience_table.xml",             [] { return MakeLoader<experience::ExperienceTableLoader>(); }},
		{"skills",              "cfg/skills.xml",                       [] { return MakeLoader<SkillsLoader>(); }},
		{"skill_messages",      "cfg/messages/ru/skill_msg.xml",        [] { return MakeLoader<skills::SkillMessagesLoader>(); }},
		{"abilities",           "cfg/abilities.xml",                    [] { return MakeLoader<abilities::AbilitiesLoader>(); }},
		{"spells",              "cfg/spells.xml",                       [] { return MakeLoader<spells::SpellsLoader>(); }},
		{"spell_messages",      "cfg/messages/ru/spell_msg.xml",        [] { return MakeLoader<spells::SpellMessagesLoader>(); }},
		{"points_intensity",    "cfg/messages/ru/points_intensity.xml", [] { return MakeLoader<points_intensity::PointsIntensityLoader>(); }},
		{"fight_messages",      "cfg/messages/ru/hit_msg.xml",          [] { return MakeLoader<fight::FightMessagesLoader>(); }},
		{"feat_messages",       "cfg/messages/ru/feat_msg.xml",         [] { return MakeLoader<feats::FeatMessagesLoader>(); }},
		{"feats",               "cfg/feats.xml",                        [] { return MakeLoader<feats::FeatsLoader>(); }},
		{"guild_messages",      "cfg/messages/ru/guild_msg.xml",        [] { return MakeLoader<guilds::GuildMessagesLoader>(); }},
		{"special_messages",    "cfg/messages/ru/special_msg.xml",      [] { return MakeLoader<specials::SpecialMessagesLoader>(); }},
		{"bank_messages",       "cfg/messages/ru/bank_msg.xml",         [] { return MakeLoader<specials::BankMessagesLoader>(); }},
		{"mail_messages",       "cfg/messages/ru/mail_msg.xml",         [] { return MakeLoader<specials::MailMessagesLoader>(); }},
		{"horse_messages",      "cfg/messages/ru/horse_msg.xml",        [] { return MakeLoader<specials::HorseMessagesLoader>(); }},
		{"torc_messages",       "cfg/messages/ru/torc_msg.xml",         [] { return MakeLoader<specials::TorcMessagesLoader>(); }},
		{"mercenary_messages",  "cfg/messages/ru/mercenary_msg.xml",    [] { return MakeLoader<specials::MercMessagesLoader>(); }},
		{"exchange_messages",   "cfg/messages/ru/exchange_msg.xml",     [] { return MakeLoader<specials::ExchMessagesLoader>(); }},
		{"rent_messages",       "cfg/messages/ru/rent_msg.xml",         [] { return MakeLoader<specials::RentMessagesLoader>(); }},
		{"shop_messages",       "cfg/messages/ru/shop_msg.xml",         [] { return MakeLoader<specials::ShopMessagesLoader>(); }},
		{"board_messages",      "cfg/messages/ru/board_msg.xml",        [] { return MakeLoader<specials::BoardMessagesLoader>(); }},
		{"specials",            "cfg/specials.xml",                     [] { return MakeLoader<SpecialsLoader>(); }},
		{"affects",             "cfg/affects.xml",                      [] { return MakeLoader<affects::AffectsLoader>(); }},
		{"affect_messages",     "cfg/messages/ru/affect_msg.xml",       [] { return MakeLoader<affects::AffectMessagesLoader>(); }},
		{"common_messages",     "cfg/messages/ru/common_msg.xml",       [] { return MakeLoader<CommonMessagesLoader>(); }},
		{"socials",             "cfg/messages/ru/social_msg.xml",       [] { return MakeLoader<communication::social::SocialsLoader>(); }},
		{"city_messages",       "cfg/messages/ru/cities_msg.xml",       [] { return MakeLoader<cities::CityMessagesLoader>(); }},
		{"cities",              "cfg/mechanics/cities.xml",             [] { return MakeLoader<cities::CitiesLoader>(); }},
		{"region_messages",     "cfg/messages/ru/region_msg.xml",       [] { return MakeLoader<regions::RegionMessagesLoader>(); }},
		{"regions",             "cfg/mechanics/regions.xml",            [] { return MakeLoader<regions::RegionsLoader>(); }},
		{"stable_objs",         "cfg/mechanics/stable_objs.xml",        [] { return MakeLoader<stable_objs::StableObjsLoader>(); }},
		{"global_drop",         "cfg/mechanics/global_drop.xml",        [] { return MakeLoader<GlobalDrop::GlobalDropLoader>(); }},
		{"group_exp_handicap",  "cfg/mechanics/group_exp_handicap.xml", [] { return MakeLoader<GroupPenaltiesLoader>(); }},
		{"reset_stats",         "cfg/mechanics/reset_stats.xml",        [] { return MakeLoader<stats_reset::ResetStatsLoader>(); }},
		{"noob",                "cfg/mechanics/noob.xml",               [] { return MakeLoader<Noob::NoobLoader>(); }},
		{"digging",             "cfg/mechanics/digging.xml",            [] { return MakeLoader<mining::DiggingLoader>(); }},
		{"celebrates",          "cfg/mechanics/celebrates.xml",         [] { return MakeLoader<celebrates::CelebratesLoader>(); }},
		{"guards",              "cfg/mechanics/guards.xml",             [] { return MakeLoader<city_guards::CityGuardsLoader>(); }},
		{"daily_quest",         "cfg/quests/daily_quest.xml",           [] { return MakeLoader<DailyQuest::DailyQuestLoader>(); }},
		{"obj_sets",            "cfg/mechanics/obj_sets.xml",           [] { return MakeLoader<obj_sets::ObjSetsLoader>(); }},
		{"treasure_cases",      "cfg/mechanics/cases.xml",              [] { return MakeLoader<treasure_cases::TreasureCasesLoader>(); }},
		{"jewelry",             "cfg/craft/jewelry.xml",                [] { return MakeLoader<jewelry::JewelryLoader>(); }},
		{"item_creation",       "cfg/craft/item_creation.xml",          [] { return MakeLoader<ItemCreationLoader>(); }},
		{"basic",               "cfg/basic.xml",                        [] { return MakeLoader<BasicValuesLoader>(); }},
		{"pc_race_messages",    "cfg/messages/ru/pc_race_msg.xml",      [] { return MakeLoader<player_races::RaceMessagesLoader>(); }},
		{"pc_races",            "cfg/mechanics/pc_races.xml",           [] { return MakeLoader<player_races::PcRacesLoader>(); }},
		{"guilds",              "cfg/mechanics/guilds.xml",             [] { return MakeLoader<guilds::GuildsLoader>(); }},
		{"zone_types",          "cfg/zone_types.xml",                   [] { return MakeLoader<zone_types::ZoneTypesLoader>(); }},
		{"rune_spells",         "cfg/mechanics/rune_spells.xml",        [] { return MakeLoader<rune_spells::RuneSpellsLoader>(); }},
		{"rune_stone_messages", "cfg/messages/ru/rune_stone_msg.xml",   [] { return MakeLoader<RuneStoneMessagesLoader>(); }},
		{"rune_stones",         "cfg/mechanics/rune_stones.xml",        [] { return MakeLoader<RuneStonesLoader>(); }},
		{"mob_classes",         "cfg/mob_classes.xml",                  [] { return MakeLoader<mob_classes::MobClassesLoader>(); }},
		{"privilege",           "cfg/privilege.xml",                    [] { return MakeLoader<privilege::PrivilegeLoader>(); }},
	};
	for (const auto &reg : registry) {
		const auto [it, inserted] = loaders_.emplace(std::string(reg.id),
			LoaderInfo(std::string(reg.path), reg.make()));
		(void) it;
		if (!inserted) {
			err_log("CfgManager: повторная регистрация загрузчика '%s' - пропущена.",
					std::string(reg.id).c_str());
		}
	}
}

// issue.cfg-manager: общая реализация загрузки/перезагрузки (раньше LoadCfg и ReloadCfg
// дублировали друг друга). Один поиск по карте; тайминг разбора DOM и сборки логируется.
void CfgManager::Apply(const std::string &id, bool reload) {
	const auto it = loaders_.find(id);
	if (it == loaders_.end()) {
		err_log("Неверный параметр %s файла конфигурации (%s)",
				reload ? "перезагрузки" : "загрузки", id.c_str());
		return;
	}
	const auto &info = it->second;
	// issue.xml-cfg-parsing: time the two phases separately -- DOM parse (file read + pugixml)
	// vs build/swap (constructing the new register) -- so we know where the lag really is.
	utils::CExecutionTimer parse_timer;
	auto data = parser_wrapper::DataNode(info.file);
	const double parse_ms = parse_timer.delta().count() * 1000.0;
	utils::CExecutionTimer build_timer;
	if (reload) {
		info.loader->Reload(data);
	} else {
		info.loader->Load(data);
	}
	const double build_ms = build_timer.delta().count() * 1000.0;
	log("Cfg %s [%s]: parse %.2f ms + %s %.2f ms = %.2f ms (%s)",
		reload ? "reload" : "load", id.c_str(), parse_ms, reload ? "build/swap" : "build",
		build_ms, parse_ms + build_ms, info.file.string().c_str());
}

void CfgManager::ReloadCfg(const std::string &id) {
	Apply(id, /*reload=*/true);
}

void CfgManager::LoadCfg(const std::string &id) {
	Apply(id, /*reload=*/false);
}

// issue.cfg-manager: атомарная запись готового DOM в файл, зарегистрированный за `id`
// (во временный .new + rename). Вызывающий не знает ни пути, ни имени файла.
bool CfgManager::Save(const std::string &id, const parser_wrapper::DataNode &doc) {
	const auto it = loaders_.find(id);
	if (it == loaders_.end()) {
		err_log("CfgManager::Save: неизвестный id конфига (%s)", id.c_str());
		return false;
	}
	const auto &file = it->second.file;
	std::filesystem::path tmp = file;
	tmp += ".new";
	if (!doc.Save(tmp)) {
		err_log("CfgManager::Save: не удалось записать %s", tmp.string().c_str());
		return false;
	}
	std::error_code ec;
	std::filesystem::rename(tmp, file, ec);
	if (ec) {
		std::filesystem::remove(tmp, ec);
		err_log("CfgManager::Save: не удалось заменить %s (%s)", file.string().c_str(), ec.message().c_str());
		return false;
	}
	return true;
}

// issue.cfg-manager: пересборка конфига из памяти - строим пустой документ, даём загрузчику
// заполнить его (loader->Save) и пишем по зарегистрированному пути.
bool CfgManager::SaveCfg(const std::string &id) {
	const auto it = loaders_.find(id);
	if (it == loaders_.end()) {
		err_log("CfgManager::SaveCfg: неизвестный id конфига (%s)", id.c_str());
		return false;
	}
	auto doc = parser_wrapper::DataNode::NewDocument();
	it->second.loader->Save(doc);
	return Save(id, doc);
}

// issue.vedun-editor: enumerate loaders that opted into editing (implement IEditableCfgLoader).
std::vector<CfgManager::EditableEntry> CfgManager::EditableEntries() const {
	std::vector<EditableEntry> out;
	for (const auto &[id, info] : loaders_) {
		if (auto *editable = dynamic_cast<IEditableCfgLoader *>(info.loader.get())) {
			out.push_back({id, editable->EditableWhat(), info.file, editable});
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
