/*
 \authors Created by Sventovit
 \date 14.02.2022.
 \brief Менеджер файлов конфигурации.
 \details Менеджер хранит имена и расположение файлов конфигурации (декларативная таблица регистрации
 id -> путь -> фабрика загрузчика) и по запросу выполняет загрузку/перезагрузку/запись данных через
 соответствующий ICfgLoader.
 ВАЖНО: ПОРЯДОК загрузки менеджер пока НЕ задаёт - он определяется последовательностью вызовов
 LoadCfg(...) в BootMudDataBase(), где загрузка конфигов перемежается со сторонними загрузчиками
 (в т.ч. загрузкой игрового мира, которую в менеджер не вынести) и зависит от порядка инициализации
 подсистем (например, города/гильдии должны быть готовы раньше связанных с ними данных). Передача
 управления порядком самому менеджеру - отдельная нетривиальная задача (TODO).
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
// issue.cfg-manager: подпространства cfg/ - раскладка каталогов в одном месте, чтобы пути в
// таблице регистрации не дублировали префиксы, а переименование каталога было однострочным.
enum class CfgDir { kRoot, kClasses, kCraft, kEconomics, kMechanics, kMessagesRu, kQuests };

std::string MakeCfgPath(CfgDir dir, std::string_view file) {
	std::string_view sub;
	switch (dir) {
		case CfgDir::kRoot:       sub = "";             break;
		case CfgDir::kClasses:    sub = "classes/";     break;
		case CfgDir::kCraft:      sub = "craft/";       break;
		case CfgDir::kEconomics:  sub = "economics/";   break;
		case CfgDir::kMechanics:  sub = "mechanics/";   break;
		case CfgDir::kMessagesRu: sub = "messages/ru/"; break;
		case CfgDir::kQuests:     sub = "quests/";      break;
	}
	std::string out = "cfg/";
	out += sub;
	out += file;
	out += ".xml";   // issue.cfg-manager: расширение единое - добавляем здесь, в таблице только имя
	return out;
}

struct Registration {
	std::string_view id;
	CfgDir dir;              // подпространство cfg/ (см. MakeCfgPath)
	std::string_view leaf;  // имя файла без расширения (.xml добавит MakeCfgPath)
	std::string_view root;  // ожидаемый корневой элемент (для централизованной проверки разбора)
	LoaderPtr (*make)();
};

} // namespace

CfgManager::CfgManager() {
	const Registration registry[] = {
		{"entity_names",        CfgDir::kMessagesRu, "entity_names",       "entity_names",        [] { return MakeLoader<entity_names::EntityNamesLoader>(); }},
		{"mob_races",           CfgDir::kMechanics,  "mob_races",          "mob_races",           [] { return MakeLoader<mob_races::MobRacesLoader>(); }},
		{"currency_messages",   CfgDir::kMessagesRu, "currency_msg",       "msg_container",       [] { return MakeLoader<currencies::CurrencyNamesLoader>(); }},
		{"shop_item_sets",      CfgDir::kEconomics,  "shop_item_sets",     "shop_item_sets",      [] { return MakeLoader<ShopExt::ShopItemSetsLoader>(); }},
		{"shops",               CfgDir::kEconomics,  "shops",              "shop_list",           [] { return MakeLoader<ShopExt::ShopsLoader>(); }},
		{"currencies",          CfgDir::kEconomics,  "currencies",         "currencies",          [] { return MakeLoader<currencies::CurrenciesLoader>(); }},
		{"class_messages",      CfgDir::kMessagesRu, "class_msg",          "msg_container",       [] { return MakeLoader<classes::ClassNamesLoader>(); }},
		{"classes",             CfgDir::kClasses,    "pc_classes",         "classes",             [] { return MakeLoader<classes::ClassesLoader>(); }},
		{"experience_table",    CfgDir::kRoot,       "experience_table",   "experience_table",    [] { return MakeLoader<experience::ExperienceTableLoader>(); }},
		{"skills",              CfgDir::kRoot,       "skills",             "skills",              [] { return MakeLoader<SkillsLoader>(); }},
		{"skill_messages",      CfgDir::kMessagesRu, "skill_msg",          "msg_container",       [] { return MakeLoader<skills::SkillMessagesLoader>(); }},
		{"abilities",           CfgDir::kRoot,       "abilities",          "abilities",           [] { return MakeLoader<abilities::AbilitiesLoader>(); }},
		{"spells",              CfgDir::kRoot,       "spells",             "spells",              [] { return MakeLoader<spells::SpellsLoader>(); }},
		{"spell_messages",      CfgDir::kMessagesRu, "spell_msg",          "msg_container",       [] { return MakeLoader<spells::SpellMessagesLoader>(); }},
		{"points_intensity",    CfgDir::kMessagesRu, "points_intensity",   "points_intensity",    [] { return MakeLoader<points_intensity::PointsIntensityLoader>(); }},
		{"fight_messages",      CfgDir::kMessagesRu, "hit_msg",            "msg_container",       [] { return MakeLoader<fight::FightMessagesLoader>(); }},
		{"feat_messages",       CfgDir::kMessagesRu, "feat_msg",           "msg_container",       [] { return MakeLoader<feats::FeatMessagesLoader>(); }},
		{"feats",               CfgDir::kRoot,       "feats",              "feats",               [] { return MakeLoader<feats::FeatsLoader>(); }},
		{"guild_messages",      CfgDir::kMessagesRu, "guild_msg",          "msg_container",       [] { return MakeLoader<guilds::GuildMessagesLoader>(); }},
		{"special_messages",    CfgDir::kMessagesRu, "special_msg",        "msg_container",       [] { return MakeLoader<specials::SpecialMessagesLoader>(); }},
		{"bank_messages",       CfgDir::kMessagesRu, "bank_msg",           "msg_container",       [] { return MakeLoader<specials::BankMessagesLoader>(); }},
		{"mail_messages",       CfgDir::kMessagesRu, "mail_msg",           "msg_container",       [] { return MakeLoader<specials::MailMessagesLoader>(); }},
		{"horse_messages",      CfgDir::kMessagesRu, "horse_msg",          "msg_container",       [] { return MakeLoader<specials::HorseMessagesLoader>(); }},
		{"torc_messages",       CfgDir::kMessagesRu, "torc_msg",           "msg_container",       [] { return MakeLoader<specials::TorcMessagesLoader>(); }},
		{"mercenary_messages",  CfgDir::kMessagesRu, "mercenary_msg",      "msg_container",       [] { return MakeLoader<specials::MercMessagesLoader>(); }},
		{"exchange_messages",   CfgDir::kMessagesRu, "exchange_msg",       "msg_container",       [] { return MakeLoader<specials::ExchMessagesLoader>(); }},
		{"rent_messages",       CfgDir::kMessagesRu, "rent_msg",           "msg_container",       [] { return MakeLoader<specials::RentMessagesLoader>(); }},
		{"shop_messages",       CfgDir::kMessagesRu, "shop_msg",           "msg_container",       [] { return MakeLoader<specials::ShopMessagesLoader>(); }},
		{"board_messages",      CfgDir::kMessagesRu, "board_msg",          "msg_container",       [] { return MakeLoader<specials::BoardMessagesLoader>(); }},
		{"specials",            CfgDir::kRoot,       "specials",           "specials",            [] { return MakeLoader<SpecialsLoader>(); }},
		{"affects",             CfgDir::kRoot,       "affects",            "affects",             [] { return MakeLoader<affects::AffectsLoader>(); }},
		{"affect_messages",     CfgDir::kMessagesRu, "affect_msg",         "msg_container",       [] { return MakeLoader<affects::AffectMessagesLoader>(); }},
		{"common_messages",     CfgDir::kMessagesRu, "common_msg",         "msg_container",       [] { return MakeLoader<CommonMessagesLoader>(); }},
		{"socials",             CfgDir::kMessagesRu, "social_msg",         "socials",             [] { return MakeLoader<communication::social::SocialsLoader>(); }},
		{"city_messages",       CfgDir::kMessagesRu, "cities_msg",         "msg_container",       [] { return MakeLoader<cities::CityMessagesLoader>(); }},
		{"cities",              CfgDir::kMechanics,  "cities",             "cities",              [] { return MakeLoader<cities::CitiesLoader>(); }},
		{"region_messages",     CfgDir::kMessagesRu, "region_msg",         "msg_container",       [] { return MakeLoader<regions::RegionMessagesLoader>(); }},
		{"regions",             CfgDir::kMechanics,  "regions",            "regions",             [] { return MakeLoader<regions::RegionsLoader>(); }},
		{"stable_objs",         CfgDir::kMechanics,  "stable_objs",        "equipment_positions", [] { return MakeLoader<stable_objs::StableObjsLoader>(); }},
		{"global_drop",         CfgDir::kMechanics,  "global_drop",        "globaldrop",          [] { return MakeLoader<GlobalDrop::GlobalDropLoader>(); }},
		{"group_exp_handicap",  CfgDir::kMechanics,  "group_exp_handicap", "group_exp_handicap",  [] { return MakeLoader<GroupPenaltiesLoader>(); }},
		{"reset_stats",         CfgDir::kMechanics,  "reset_stats",        "reset_stats",         [] { return MakeLoader<stats_reset::ResetStatsLoader>(); }},
		{"noob",                CfgDir::kMechanics,  "noob",               "noob_help",           [] { return MakeLoader<Noob::NoobLoader>(); }},
		{"digging",             CfgDir::kMechanics,  "digging",            "digging",             [] { return MakeLoader<mining::DiggingLoader>(); }},
		{"celebrates",          CfgDir::kMechanics,  "celebrates",         "celebrates",          [] { return MakeLoader<celebrates::CelebratesLoader>(); }},
		{"guards",              CfgDir::kMechanics,  "guards",             "guardians",           [] { return MakeLoader<city_guards::CityGuardsLoader>(); }},
		{"daily_quest",         CfgDir::kQuests,     "daily_quest",        "daily_quest",         [] { return MakeLoader<DailyQuest::DailyQuestLoader>(); }},
		{"obj_sets",            CfgDir::kMechanics,  "obj_sets",           "obj_sets",            [] { return MakeLoader<obj_sets::ObjSetsLoader>(); }},
		{"treasure_cases",      CfgDir::kMechanics,  "cases",              "cases",               [] { return MakeLoader<treasure_cases::TreasureCasesLoader>(); }},
		{"jewelry",             CfgDir::kCraft,      "jewelry",            "jewelry",             [] { return MakeLoader<jewelry::JewelryLoader>(); }},
		{"item_creation",       CfgDir::kCraft,      "item_creation",      "item_creation",       [] { return MakeLoader<ItemCreationLoader>(); }},
		{"basic",               CfgDir::kRoot,       "basic",              "basic",               [] { return MakeLoader<BasicValuesLoader>(); }},
		{"pc_race_messages",    CfgDir::kMessagesRu, "pc_race_msg",        "msg_container",       [] { return MakeLoader<player_races::RaceMessagesLoader>(); }},
		{"pc_races",            CfgDir::kMechanics,  "pc_races",           "pc_races",            [] { return MakeLoader<player_races::PcRacesLoader>(); }},
		{"guilds",              CfgDir::kMechanics,  "guilds",             "guilds",              [] { return MakeLoader<guilds::GuildsLoader>(); }},
		{"zone_types",          CfgDir::kRoot,       "zone_types",         "zone_types",          [] { return MakeLoader<zone_types::ZoneTypesLoader>(); }},
		{"rune_spells",         CfgDir::kMechanics,  "rune_spells",        "rune_spells",         [] { return MakeLoader<rune_spells::RuneSpellsLoader>(); }},
		{"rune_stone_messages", CfgDir::kMessagesRu, "rune_stone_msg",     "msg_container",       [] { return MakeLoader<RuneStoneMessagesLoader>(); }},
		{"rune_stones",         CfgDir::kMechanics,  "rune_stones",        "rune_stones",         [] { return MakeLoader<RuneStonesLoader>(); }},
		{"mob_classes",         CfgDir::kRoot,       "mob_classes",        "mob_classes",         [] { return MakeLoader<mob_classes::MobClassesLoader>(); }},
		{"privilege",           CfgDir::kRoot,       "privilege",          "privilege",           [] { return MakeLoader<privilege::PrivilegeLoader>(); }},
	};
	for (const auto &reg : registry) {
		const auto [it, inserted] = loaders_.emplace(std::string(reg.id),
			LoaderInfo(MakeCfgPath(reg.dir, reg.leaf), std::string(reg.root), reg.make()));
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
	// issue.cfg-manager: централизованная проверка результата разбора. Если файл не разобран
	// (нет корня) или корневой элемент не тот, какой ждём, - НЕ зовём загрузчик, чтобы при сбое
	// текущие данные остались нетронутыми (DataNode уже залогировал ошибку чтения файла).
	if (data.IsEmpty()) {
		log("Cfg %s [%s]: ПРОПУЩЕН - файл не разобран/отсутствует, данные не изменены (%s)",
			reload ? "reload" : "load", id.c_str(), info.file.string().c_str());
		return;
	}
	if (!info.root.empty() && info.root != data.GetName()) {
		err_log("Cfg %s [%s]: ПРОПУЩЕН - корень <%s> вместо ожидаемого <%s> (%s)",
			reload ? "reload" : "load", id.c_str(), data.GetName(), info.root.c_str(),
			info.file.string().c_str());
		return;
	}
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
