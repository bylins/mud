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
#include "engine/ui/system_messages.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/reset_stats.h"
#include "gameplay/core/remort.h"
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
#include "gameplay/magic/room_affects_loader.h"
#include "gameplay/magic/room_affect_messages.h"
#include "gameplay/affects/affect_messages.h"
#include "engine/core/common_messages.h"
#include "gameplay/mechanics/rune_stones_loaders.h"
#include "gameplay/mechanics/service_zones_loaders.h"
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
	// issue.cfg-loader-keys: единый ключ = имя файла (без .xml) = корневой элемент XML.
	std::string_view name;
	CfgDir dir;             // подпространство cfg/ (см. MakeCfgPath)
	LoaderPtr (*make)();
};

} // namespace

CfgManager::CfgManager() {
	const Registration registry[] = {
		{"entity_names", CfgDir::kMessagesRu, [] { return MakeLoader<entity_names::EntityNamesLoader>(); }},
		{"mob_races", CfgDir::kMechanics, [] { return MakeLoader<mob_races::MobRacesLoader>(); }},
		{"currency_msg", CfgDir::kMessagesRu, [] { return MakeLoader<currencies::CurrencyNamesLoader>(); }},
		{"shop_item_sets", CfgDir::kEconomics, [] { return MakeLoader<ShopExt::ShopItemSetsLoader>(); }},
		{"shops", CfgDir::kEconomics, [] { return MakeLoader<ShopExt::ShopsLoader>(); }},
		{"currencies", CfgDir::kEconomics, [] { return MakeLoader<currencies::CurrenciesLoader>(); }},
		{"class_msg", CfgDir::kMessagesRu, [] { return MakeLoader<classes::ClassNamesLoader>(); }},
		{"pc_classes", CfgDir::kClasses, [] { return MakeLoader<classes::ClassesLoader>(); }},
		{"experience_table", CfgDir::kRoot, [] { return MakeLoader<experience::ExperienceTableLoader>(); }},
		{"skills", CfgDir::kRoot, [] { return MakeLoader<SkillsLoader>(); }},
		{"skill_msg", CfgDir::kMessagesRu, [] { return MakeLoader<skills::SkillMessagesLoader>(); }},
		{"abilities", CfgDir::kRoot, [] { return MakeLoader<abilities::AbilitiesLoader>(); }},
		{"spells", CfgDir::kRoot, [] { return MakeLoader<spells::SpellsLoader>(); }},
		{"spell_msg", CfgDir::kMessagesRu, [] { return MakeLoader<spells::SpellMessagesLoader>(); }},
		{"points_intensity", CfgDir::kMessagesRu, [] { return MakeLoader<points_intensity::PointsIntensityLoader>(); }},
		{"hit_msg", CfgDir::kMessagesRu, [] { return MakeLoader<fight::FightMessagesLoader>(); }},
		{"feat_msg", CfgDir::kMessagesRu, [] { return MakeLoader<feats::FeatMessagesLoader>(); }},
		{"feats", CfgDir::kRoot, [] { return MakeLoader<feats::FeatsLoader>(); }},
		{"guild_msg", CfgDir::kMessagesRu, [] { return MakeLoader<guilds::GuildMessagesLoader>(); }},
		{"special_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::SpecialMessagesLoader>(); }},
		{"bank_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::BankMessagesLoader>(); }},
		{"mail_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::MailMessagesLoader>(); }},
		{"horse_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::HorseMessagesLoader>(); }},
		{"torc_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::TorcMessagesLoader>(); }},
		{"mercenary_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::MercMessagesLoader>(); }},
		{"exchange_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::ExchMessagesLoader>(); }},
		{"rent_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::RentMessagesLoader>(); }},
		{"shop_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::ShopMessagesLoader>(); }},
		{"board_msg", CfgDir::kMessagesRu, [] { return MakeLoader<specials::BoardMessagesLoader>(); }},
		{"specials", CfgDir::kRoot, [] { return MakeLoader<SpecialsLoader>(); }},
		{"affects", CfgDir::kRoot, [] { return MakeLoader<affects::AffectsLoader>(); }},
		{"room_affects", CfgDir::kRoot, [] { return MakeLoader<room_spells::RoomAffectsLoader>(); }},
		{"affect_msg", CfgDir::kMessagesRu, [] { return MakeLoader<affects::AffectMessagesLoader>(); }},
		{"room_affect_msg", CfgDir::kMessagesRu, [] { return MakeLoader<room_spells::RoomAffectMessagesLoader>(); }},
		{"common_msg", CfgDir::kMessagesRu, [] { return MakeLoader<CommonMessagesLoader>(); }},
		{"social_msg", CfgDir::kMessagesRu, [] { return MakeLoader<communication::social::SocialsLoader>(); }},
		{"cities_msg", CfgDir::kMessagesRu, [] { return MakeLoader<cities::CityMessagesLoader>(); }},
		{"cities", CfgDir::kMechanics, [] { return MakeLoader<cities::CitiesLoader>(); }},
		{"region_msg", CfgDir::kMessagesRu, [] { return MakeLoader<regions::RegionMessagesLoader>(); }},
		{"system_msg", CfgDir::kMessagesRu, [] { return MakeLoader<system_messages::SystemMessagesLoader>(); }},
		{"regions", CfgDir::kMechanics, [] { return MakeLoader<regions::RegionsLoader>(); }},
		{"stable_objs", CfgDir::kMechanics, [] { return MakeLoader<stable_objs::StableObjsLoader>(); }},
		{"global_drop", CfgDir::kMechanics, [] { return MakeLoader<GlobalDrop::GlobalDropLoader>(); }},
		{"group_exp_handicap", CfgDir::kMechanics, [] { return MakeLoader<GroupPenaltiesLoader>(); }},
		{"reset_stats", CfgDir::kMechanics, [] { return MakeLoader<stats_reset::ResetStatsLoader>(); }},
		{"remort", CfgDir::kRoot, [] { return MakeLoader<remort::RemortLoader>(); }},
		{"noob", CfgDir::kMechanics, [] { return MakeLoader<Noob::NoobLoader>(); }},
		{"digging", CfgDir::kMechanics, [] { return MakeLoader<mining::DiggingLoader>(); }},
		{"celebrates", CfgDir::kMechanics, [] { return MakeLoader<celebrates::CelebratesLoader>(); }},
		{"guards", CfgDir::kMechanics, [] { return MakeLoader<city_guards::CityGuardsLoader>(); }},
		{"daily_quest", CfgDir::kQuests, [] { return MakeLoader<DailyQuest::DailyQuestLoader>(); }},
		{"obj_sets", CfgDir::kMechanics, [] { return MakeLoader<obj_sets::ObjSetsLoader>(); }},
		{"cases", CfgDir::kMechanics, [] { return MakeLoader<treasure_cases::TreasureCasesLoader>(); }},
		{"jewelry", CfgDir::kCraft, [] { return MakeLoader<jewelry::JewelryLoader>(); }},
		{"item_creation", CfgDir::kCraft, [] { return MakeLoader<ItemCreationLoader>(); }},
		{"basic", CfgDir::kRoot, [] { return MakeLoader<BasicValuesLoader>(); }},
		{"pc_race_msg", CfgDir::kMessagesRu, [] { return MakeLoader<player_races::RaceMessagesLoader>(); }},
		{"pc_races", CfgDir::kMechanics, [] { return MakeLoader<player_races::PcRacesLoader>(); }},
		{"guilds", CfgDir::kMechanics, [] { return MakeLoader<guilds::GuildsLoader>(); }},
		{"zone_types", CfgDir::kRoot, [] { return MakeLoader<zone_types::ZoneTypesLoader>(); }},
		{"rune_spells", CfgDir::kMechanics, [] { return MakeLoader<rune_spells::RuneSpellsLoader>(); }},
		{"rune_stone_msg", CfgDir::kMessagesRu, [] { return MakeLoader<RuneStoneMessagesLoader>(); }},
		{"rune_stones", CfgDir::kMechanics, [] { return MakeLoader<RuneStonesLoader>(); }},
		{"service_zones", CfgDir::kMechanics, [] { return MakeLoader<service_zones::ServiceZonesLoader>(); }},
		{"mob_classes", CfgDir::kRoot, [] { return MakeLoader<mob_classes::MobClassesLoader>(); }},
		{"privilege", CfgDir::kRoot, [] { return MakeLoader<privilege::PrivilegeLoader>(); }},
	};
	for (const auto &reg : registry) {
		const auto [it, inserted] = loaders_.emplace(std::string(reg.name),
			LoaderInfo(MakeCfgPath(reg.dir, reg.name), reg.make()));
		(void) it;
		if (!inserted) {
			err_log("CfgManager: повторная регистрация загрузчика '%s' - пропущена.",
					std::string(reg.name).c_str());
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
	// Корень не тот, что ожидали - это лишь предупреждение: всё равно грузим (главное, чтобы
	// файл загрузился; кодовая база не всегда строго следует XML, корень может отличаться).
	// issue.cfg-loader-keys: ожидаемый корень == ключ конфига (== имя файла).
	if (id != data.GetName()) {
		err_log("Cfg %s [%s]: неожиданный корень <%s> (ожидался <%s>) - грузим как есть (%s)",
			reload ? "reload" : "load", id.c_str(), data.GetName(), id.c_str(),
			info.file.string().c_str());
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
