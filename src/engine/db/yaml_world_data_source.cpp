// Part of Bylins http://www.mud.ru
// YAML world data source implementation

#ifdef HAVE_YAML

#include "yaml_world_data_source.h"
#include "utils/utils_encoding.h"
#include "dictionary_loader.h"
#include "db.h"
#include "obj_prototypes.h"
#include "global_objects.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/utils_time.h"
#include "utils/utils_string.h"
#include "utils/utils_parse.h"
#include "engine/entities/zone.h"
#include "engine/entities/room_data.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/db/description.h"
#include "engine/structs/extra_description.h"
#include "engine/structs/flag_data.h"
#include "gameplay/mechanics/dead_load.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/liquid.h"          // issue #3593: drinks[]/NUM_LIQ_TYPES
#include "gameplay/economics/currencies.h"       // issue #3593: MUD::Currency().GetName()
#include "gameplay/fight/fight_messages.h"        // issue #3593: GetAttackTypeDescription
#include "engine/scripting/dg_olc.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/skills/skills.h"
#include "utils/thread_pool.h"

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <utility>
#include <vector>
#include <regex>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>

#include "koi8r_yaml_emitter.h"

// External declarations
extern ZoneTable &zone_table;
extern IndexData **trig_index;
extern int top_of_trigt;
extern Rooms &world;
extern RoomRnum top_of_world;
extern IndexData *mob_index;
extern MobRnum top_of_mobt;
extern CharData *mob_proto;

namespace world_loader
{

namespace
{

TriggerScriptLanguage ParseTriggerScriptLanguage(const YAML::Node &root)
{
	std::string language = "dg";
	if (root["language"])
	{
		language = root["language"].as<std::string>();
	}
	else if (root["script_language"])
	{
		language = root["script_language"].as<std::string>();
	}
	std::transform(language.begin(), language.end(), language.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return language == "lua" ? TriggerScriptLanguage::Lua : TriggerScriptLanguage::Dg;
}

} // namespace

// Convert dictionary index to bitvector flag value.
// Delegates to the canonical flag_data_by_num() in engine/structs/flag_data.h
// to avoid drift (previous local copy mishandled plane 3: returned 1u<<idx
// instead of kIntThree | (1u << (idx - 90))).
inline Bitvector IndexToBitvector(long idx)
{
	return static_cast<Bitvector>(flag_data_by_num(static_cast<int>(idx)));
}

// ============================================================================
// Helper functions for YAML comments
// ============================================================================

// Get trigger name by vnum (for trigger comments)
std::string GetTriggerNameComment(int trigger_vnum) {
	int rnum = GetTriggerRnum(trigger_vnum);
	if (rnum >= 0 && rnum < top_of_trigt) {
		return trig_index[rnum]->proto->get_name();
	}
	return "";
}

// Get room name by rnum (for exit comments)
std::string GetRoomNameComment(RoomRnum room_rnum) {
	if (room_rnum >= 0 && room_rnum < static_cast<int>(world.size()) && world[room_rnum]) {
		return world[room_rnum]->name ? world[room_rnum]->name : "";
	}
	return "";
}

// Get skill name by ID (for skill comments)
std::string GetSkillNameComment(ESkill skill_id) {
	return MUD::Skill(skill_id).GetName();
}

// Get spell name by ID (for spell comments)
std::string GetSpellNameComment(ESpell spell_id) {
	return MUD::Spell(spell_id).GetName();
}

// Get feat name by ID (for feat comments)
std::string GetFeatNameComment(int feat_id) {
	return MUD::Feat(static_cast<EFeat>(feat_id)).GetName();
}

// issue #3593 (расширяет #3583): подпись values-слота по типу предмета -- смысл
// слота и, где уместно, расшифровка enum-значения (заклинание/тип атаки/жидкость/
// валюта/книга/класс компонента). Смысл слотов взят из OLC-меню
// (oedit_disp_val1..4_menu). slot 0-индексный (= val[0..3]); "" -- нет осмысленной
// подписи для данного типа/слота.
std::string GetObjValueComment(EObjType type, int slot, int value) {
	// заклинание по номеру (для спелл-слотов свитков/палочек/посохов/зелий)
	auto spell = [](int v) -> std::string {
		return (v > 0 && v <= to_underlying(ESpell::kLast)) ? GetSpellNameComment(static_cast<ESpell>(v)) : "";
	};
	switch (type) {
		case EObjType::kLightSource:
			return slot == 2 ? "длительность горения (0=погасла, -1=вечный)" : "";

		case EObjType::kScroll:
			return slot == 0 ? "уровень заклинания" : spell(value);   // val[1..3] -- заклинания

		case EObjType::kPotion:
			if (slot == 0) return "уровень заклинания";
			if (slot == 3) return "сила зелья (potency)";
			return spell(value);   // val[1..2] -- заклинания

		case EObjType::kWand:
		case EObjType::kStaff:
			switch (slot) {
				case 0: return "уровень заклинания";
				case 1: return "всего зарядов";
				case 2: return "осталось зарядов";
				case 3: return spell(value);
			}
			return "";

		case EObjType::kWeapon:
			switch (slot) {
				case 0: return "не используется";
				case 1: return "число бросков кубика";
				case 2: return "граней кубика";
				case 3: {
					// тип удара -- из кода (fight-сообщения); в oedit save all они загружены
					const std::string &n = fight::GetAttackTypeDescription(value);
					return n.empty() ? "тип атаки" : "тип атаки: " + n;
				}
			}
			return "";

		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor:
			if (slot == 0) return "изменяет AC";
			if (slot == 1) return "изменяет броню";
			return "";

		case EObjType::kContainer:
			switch (slot) {
				case 0: return "макс. вместимый вес";
				case 1: return "флаги контейнера";
				case 2: return "vnum ключа (-1 нет)";
				case 3: return "сложность замка (0-1000)";
			}
			return "";

		case EObjType::kLiquidContainer:
		case EObjType::kFountain:
			switch (slot) {
				case 0: return "объём, глотков";
				case 1: return "налито, глотков";
				case 2:
					return (value >= 0 && value < NUM_LIQ_TYPES)
						? "тип жидкости: " + std::string(drinks[value]) : "тип жидкости";
				case 3: return "отравлено (0 нет, 1 да, >1 таймер)";
			}
			return "";

		case EObjType::kFood:
			if (slot == 0) return "насыщает, часов";
			if (slot == 3) return "отравлено (0 нет, 1 да, >1 таймер)";
			return "";

		case EObjType::kMoney:
			if (slot == 0) return "сумма";
			if (slot == 1) {
				// валюта -- из кода: money val[1] == vnum валюты (kGoldVnum=0 и т.д.).
				const std::string &n = MUD::Currency(value).GetName();
				return (n.empty() || n == "!undefined!") ? "тип валюты" : n;
			}
			return "";

		case EObjType::kBook:
			// название типа книги -- из единого источника GetBookTypeName (enum EBook).
			if (slot == 0) {
				const char *n = GetBookTypeName(static_cast<EBook>(value));
				return (n && *n) ? n : "тип книги";
			}
			return "";

		case EObjType::kMagicIngredient:
			switch (slot) {
				case 0: return "лаг применения, сек + уровень (6 бит)";
				case 1: return "vnum прототипа";
				case 2: return "число использований";
			}
			return "";

		case EObjType::kMagicComponent:
			// Единственное авторское значение -- класс ингредиента; OLC пишет его в
			// val[3] (set_val(3) через меню класса), im.cpp читает GET_OBJ_VAL(...,3).
			// val[0..2] заполняются при загрузке из lib/misc/im.lst -- их не подписываем.
			if (slot == 3) {
				const char *n = GetIngredientClassName(static_cast<EIngredientClass>(value));
				return (n && *n) ? std::string("класс ингредиента: ") + n : "класс ингредиента";
			}
			return "";

		case EObjType::kCraftMaterial:
			switch (slot) {
				case 0: return "уровень игрока + морт*2";
				case 1: return "vnum прототипа";
				case 2: return "сила ингредиента";
				case 3: return "условный уровень";
			}
			return "";

		case EObjType::kBandage:
			return slot == 0 ? "хитов в секунду" : "";

		case EObjType::kEnchant:
			return slot == 0 ? "изменяет вес" : "";

		case EObjType::kMagicContaner:
			switch (slot) {
				case 0: return spell(value);
				case 1: return "объём колчана";
				case 2: return "количество стрел";
			}
			return "";

		case EObjType::kMagicArrow:
			switch (slot) {
				case 0: return spell(value);
				case 1: return "размер пучка";
				case 2: return "количество стрел";
			}
			return "";

		default:
			return "";
	}
}

// issue #3583: имя спелла для потионных ключей extra_values вида POTION_SPELL<n>_NUM
// (у зелий заклинания лежат в ObjVal-ключах, а не в val[]). Ключи *_LVL не трогаем.
std::string GetExtraValueSpellComment(const std::string &key, int value) {
	static const std::string kSuffix = "_NUM";
	// issue.magic-items: annotate the scroll/wand/staff SPELLITEM_SPELL<n>_NUM keys too, not just
	// the potion POTION_SPELL<n>_NUM keys. Both hold an ESpell number in a *_SPELL<n>_NUM key.
	const bool has_num_suffix = key.size() > kSuffix.size()
		&& key.compare(key.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0;
	// issue.magic-items: canonical SPELL<n>_NUM (unified) plus the legacy POTION_SPELL*/SPELLITEM_SPELL*
	// read-aliases -- rfind("SPELL",0) already covers SPELL<n>_NUM and SPELLITEM_SPELL<n>_NUM.
	const bool is_spell_key = has_num_suffix
		&& (key.rfind("SPELL", 0) == 0 || key.rfind("POTION_SPELL", 0) == 0);
	if (!is_spell_key || value <= 0 || value > to_underlying(ESpell::kLast)) {
		return "";
	}
	return GetSpellNameComment(static_cast<ESpell>(value));
}

// issue.magic-items-hotfix: подпись значения для extra_values-ключей, чей смысл --
// enum из кода. LIQUID_TYPE (бывший val[2] у питья/фонтанов, теперь ключ) -- название
// жидкости из drinks[] (enum LIQ_*). Прочее делегируем на спелл-подпись.
std::string GetExtraValueComment(const std::string &key, int value) {
	if (key == "LIQUID_TYPE") {
		return (value >= 0 && value < NUM_LIQ_TYPES) ? std::string(drinks[value]) : "";
	}
	return GetExtraValueSpellComment(key, value);
}

// Get material name by ID (for material comments)
std::string GetMaterialNameComment(int material_id) {
	return ::material_name[material_id];
}

// Get apply type name by ID (for apply location comments)
std::string GetApplyTypeNameComment(int apply_id) {
	return ::apply_types[apply_id];
}

// Strip MUD color codes (&X) from string
std::string StripMudColorCodes(const std::string &s)
{
	std::string result;
	result.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '&' && i + 1 < s.size())
		{
			++i;  // skip &X
		}
		else
		{
			result += s[i];
		}
	}
	return result;
}

// Get mob name (nominative) by vnum (for zone command comments)
std::string GetMobNameComment(int mob_vnum)
{
	MobRnum rnum = GetMobRnum(mob_vnum);
	if (rnum < 0 || !mob_proto)
	{
		return "";
	}
	const char *name = mob_proto[rnum].get_pad(0);
	if (!name || *name == '\0')
	{
		return "";
	}
	return StripMudColorCodes(name);
}

// Get room name by vnum (for zone command comments)
std::string GetRoomNameByVnum(int room_vnum)
{
	RoomRnum rnum = GetRoomRnum(room_vnum);
	return StripMudColorCodes(GetRoomNameComment(rnum));
}

// Get zone type name by type index (issue.ztypes-migrate: registry lookup).
std::string GetZoneTypeName(int type_id)
{
	if (type_id < 0 || !MUD::ZoneTypes().IsKnown(type_id))
	{
		return "";
	}
	return MUD::ZoneTypes()[type_id].GetName();
}

// Get object name (nominative) by vnum (for zone command comments)
std::string GetObjNameComment(int obj_vnum)
{
	int rnum = obj_proto.get_rnum(obj_vnum);
	if (rnum < 0)
	{
		return "";
	}
	auto obj = obj_proto[rnum];
	if (!obj)
	{
		return "";
	}
	return StripMudColorCodes(obj->get_PName(grammar::ECase::kNom));
}

// ============================================================================
// YamlWorldDataSource implementation
// ============================================================================

YamlWorldDataSource::YamlWorldDataSource(const std::string &world_dir)
	: m_world_dir(world_dir)
{
	// Load world configuration (required)
	if (!LoadWorldConfig()) {
		log("SYSERR: Failed to load world_config.yaml");
		fprintf(stderr, "\n");
		fprintf(stderr, "ERROR: World is in Legacy format, but server built with YAML support.\n");
		fprintf(stderr, "To convert world to YAML format, run from world directory (NOT world/world):\n");
		fprintf(stderr, "  cd %s\n", m_world_dir.c_str());
		fprintf(stderr, "  /path/to/convert_to_yaml.py -i . -o . --format yaml --type all\n");
		fprintf(stderr, "\n");
		exit(1);
	}
}

// Helper: get configured thread count from runtime config
size_t YamlWorldDataSource::GetConfiguredThreadCount() const
{
	size_t configured = runtime_config.thread_pools_loader();
	if (configured == 0)
	{
		// Default to physical cores (hw/2): world parsing is CPU- and
		// allocator-bound, so SMT siblings add little and can even hurt via
		// allocator contention. Matches the savers pool default. Set
		// <thread_pools><loader>N</loader> to override.
		size_t hw_threads = std::thread::hardware_concurrency();
		return std::max<size_t>(1, hw_threads / 2);
	}
	return configured;
}

// Load world configuration from world_config.yaml
bool YamlWorldDataSource::LoadWorldConfig()
{
	std::string config_path = m_world_dir + "/world_config.yaml";

	// Config file is required
	std::ifstream test_file(config_path);
	if (!test_file.good()) {
		log("SYSERR: World configuration file not found: %s", config_path.c_str());
		return false;
	}
	test_file.close();

	try {
		YAML::Node config = YAML::LoadFile(config_path);

		// Read line_endings setting (required)
		if (!config["line_endings"]) {
			log("SYSERR: Missing 'line_endings' in world_config.yaml");
			return false;
		}

		std::string line_endings = config["line_endings"].as<std::string>();
		if (line_endings == "dos") {
			m_convert_lf_to_crlf = true;
			log("World config: DOS line endings (LF -> CR+LF conversion enabled)");
		} else if (line_endings == "unix") {
			m_convert_lf_to_crlf = false;
			log("World config: Unix line endings (no conversion)");
		} else {
			log("SYSERR: Invalid line_endings value '%s' (expected 'dos' or 'unix')", line_endings.c_str());
			return false;
		}

		// Read layout setting (optional, defaults to flat). Controls only how
		// zones are WRITTEN; loading auto-detects the layout per zone regardless.
		if (config["layout"]) {
			std::string layout = config["layout"].as<std::string>();
			if (layout == "flat") {
				m_save_layout = YamlLayout::Flat;
				log("World config: flat layout (one file per entity type per zone)");
			} else if (layout == "per_file") {
				m_save_layout = YamlLayout::PerFile;
				log("World config: per-file layout (one file per entity)");
			} else {
				log("SYSERR: Invalid layout value '%s' (expected 'flat' or 'per_file')", layout.c_str());
				return false;
			}
		} else {
			m_save_layout = YamlLayout::Flat;
			log("World config: layout not set, defaulting to flat");
		}

		return true;
	} catch (const YAML::Exception &e) {
		log("SYSERR: Failed to parse world_config.yaml: %s", e.what());
		return false;
	}
}

bool YamlWorldDataSource::LoadDictionaries()
{
	if (m_dictionaries_loaded)
	{
		return true;
	}

	std::string dict_dir = m_world_dir + "/dictionaries";
	if (!DictionaryManager::Instance().LoadDictionaries(dict_dir))
	{
		log("SYSERR: Failed to load dictionaries from %s", dict_dir.c_str());
		return false;
	}

	m_dictionaries_loaded = true;
	return true;
}

std::vector<int> YamlWorldDataSource::GetZoneList()
{
	std::vector<int> zones;
	std::string index_path = m_world_dir + "/zones/index.yaml";

	try
	{
		YAML::Node root = YAML::LoadFile(index_path);
		if (root["zones"] && root["zones"].IsSequence())
		{
			for (const auto &zone_node : root["zones"])
			{
				zones.push_back(zone_node.as<int>());
			}
		}
	}
	catch (const YAML::Exception &e)
	{
		log("SYSERR: Failed to load zone index '%s': %s", index_path.c_str(), e.what());
	}

	return zones;
}

namespace
{
struct DiscoverResult
{
	std::vector<EntityFileTask> tasks;
	size_t flat_files_parsed = 0;
	size_t perfile_indexes_parsed = 0;
	int failures = 0;
};
} // namespace

std::vector<EntityFileTask> YamlWorldDataSource::DiscoverEntityFiles(const std::string &sub, const std::vector<int> *zone_filter)
{
	namespace fs = std::filesystem;

	// [yaml-timing] this used to run SERIALLY on the main thread, one zone at
	// a time. In flat layout, enumerating a zone's keys requires fully
	// parsing its file -- with 600+ zones that added up to 5-6x the actual
	// (already parallel) load step, for no reason: nothing here depends on
	// zone order (the load step re-sorts everything by vnum after parsing,
	// see the merge/post step below), so it's embarrassingly parallel across
	// zones exactly like the load step already is. Distributed across the
	// same thread pool here removes that serial bottleneck.
	utils::CExecutionTimer disc_timer;

	std::vector<int> zone_vnums = zone_filter ? *zone_filter : GetZoneList();
	std::sort(zone_vnums.begin(), zone_vnums.end());
	auto batches = utils::DistributeBatches(zone_vnums, m_num_threads);

	std::vector<std::future<DiscoverResult>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &sub]() -> DiscoverResult {
			namespace fs = std::filesystem;
			DiscoverResult result;
			for (int zone_vnum : batches[thread_id])
			{
				const std::string zone_dir = m_world_dir + "/zones/" + std::to_string(zone_vnum);
				const std::string flat_path = zone_dir + "/" + sub + ".yaml";
				const std::string index_path = zone_dir + "/" + sub + "/index.yaml";
				const bool has_flat = fs::exists(flat_path);
				const bool has_perfile = fs::exists(index_path);

				// Normally only one layout exists (saves are self-cleaning). If
				// both are present -- a half-finished migration or a leftover
				// from a failed cleanup -- the configured layout (m_save_layout,
				// from world_config.yaml) decides which one is authoritative;
				// the other is ignored.
				bool use_flat;
				if (has_flat && has_perfile)
				{
					use_flat = (m_save_layout == YamlLayout::Flat);
					log("WARNING: zone %d has both flat %s.yaml and per-file %s/ -- using "
						"%s per world_config layout; the other is ignored.",
						zone_vnum, sub.c_str(), sub.c_str(), use_flat ? "flat" : "per-file");
				}
				else
				{
					use_flat = has_flat;
				}

				if (use_flat)
				{
					// Flat layout: the file is a map of rel-number -> entity
					// node and doubles as its own index. Enumerating the keys
					// here already requires a full parse; stash that parsed
					// root in the task so the worker (below, in each Load*()
					// parallel section) reuses it instead of calling
					// YAML::LoadFile() on the same file a second time.
					EntityFileTask task;
					task.flat = true;
					task.path = flat_path;
					try
					{
						YAML::Node root = YAML::LoadFile(flat_path);
						++result.flat_files_parsed;
						for (auto it = root.begin(); it != root.end(); ++it)
						{
							int rel_num = it->first.as<int>();
							task.vnums.push_back(zone_vnum * 100 + rel_num);
						}
						task.parsed_root = root;
					}
					catch (const YAML::Exception &e)
					{
						log("SYSERR: Failed to load flat %s file for zone %d ('%s'): %s",
							sub.c_str(), zone_vnum, flat_path.c_str(), e.what());
						++result.failures;
						continue;
					}
					std::sort(task.vnums.begin(), task.vnums.end());
					if (!task.vnums.empty())
					{
						result.tasks.push_back(std::move(task));
					}
					continue;
				}

				// Per-file layout: one task per entity, listed in
				// <sub>/index.yaml. A missing index simply means the zone has
				// no entities of this type.
				if (!has_perfile)
				{
					continue;
				}
				try
				{
					YAML::Node root = YAML::LoadFile(index_path);
					++result.perfile_indexes_parsed;
					if (root[sub] && root[sub].IsSequence())
					{
						for (const auto &rel_node : root[sub])
						{
							int rel_num = rel_node.as<int>();
							EntityFileTask task;
							task.flat = false;
							std::ostringstream ss;
							ss << zone_dir << "/" << sub << "/"
							   << std::setfill('0') << std::setw(2) << rel_num << ".yaml";
							task.path = ss.str();
							task.vnums.push_back(zone_vnum * 100 + rel_num);
							result.tasks.push_back(std::move(task));
						}
					}
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load %s index for zone %d ('%s'): %s",
						sub.c_str(), zone_vnum, index_path.c_str(), e.what());
					++result.failures;
				}
			}
			return result;
		}));
	}

	std::vector<EntityFileTask> tasks;
	size_t flat_files_parsed = 0;
	size_t perfile_indexes_parsed = 0;
	int failures = 0;
	for (auto &f : futures)
	{
		DiscoverResult result = f.get();
		flat_files_parsed += result.flat_files_parsed;
		perfile_indexes_parsed += result.perfile_indexes_parsed;
		failures += result.failures;
		tasks.insert(tasks.end(), std::make_move_iterator(result.tasks.begin()),
			std::make_move_iterator(result.tasks.end()));
	}
	if (failures > 0)
	{
		fatal_log("SYSERR: Failed to load %d %s file(s) during discovery. Aborting.", failures, sub.c_str());
	}

	log("   [yaml-timing] discover '%s': %.1f ms parallel (threads=%zu, flat files fully parsed: %zu, "
		"per-file indexes: %zu, tasks: %zu)",
		sub.c_str(), disc_timer.delta().count() * 1000.0, batches.size(),
		flat_files_parsed, perfile_indexes_parsed, tasks.size());
	return tasks;
}

std::string YamlWorldDataSource::ConvertToKoi8r(const std::string &utf8_str) const
{
	if (utf8_str.empty())
	{
		return "";
	}

	static char buffer[65536];
	char *input = const_cast<char*>(utf8_str.c_str());
	char *output = buffer;
	codepages::utf8_to_koi(input, output);
	return buffer;
}

std::string YamlWorldDataSource::GetText(const YAML::Node &node, const std::string &key, const std::string &default_val) const
{
	if (node[key])
	{
		// YAML files are already in KOI8-R, no conversion needed
		std::string text = node[key].as<std::string>();

		// Convert line endings if configured for DOS format
		if (m_convert_lf_to_crlf) {
			std::string result;
			result.reserve(text.size() * 2);  // Reserve space for worst case
			for (size_t i = 0; i < text.size(); ++i) {
				if (text[i] == '\n' && (i == 0 || text[i-1] != '\r')) {
					result += "\r\n";
				} else {
					result += text[i];
				}
			}
			return result;
		}

		return text;
	}
	return default_val;
}

int YamlWorldDataSource::GetInt(const YAML::Node &node, const std::string &key, int default_val) const
{
	if (node[key])
	{
		return node[key].as<int>();
	}
	return default_val;
}

long YamlWorldDataSource::GetLong(const YAML::Node &node, const std::string &key, long default_val) const
{
	if (node[key])
	{
		return node[key].as<long>();
	}
	return default_val;
}

FlagData YamlWorldDataSource::ParseFlags(const YAML::Node &node, const std::string &dict_name) const
{
	FlagData flags;

	if (!node || !node.IsSequence())
	{
		return flags;
	}

	auto &dm = DictionaryManager::Instance();

	for (const auto &flag_node : node)
	{
		std::string flag_name = flag_node.as<std::string>();

		// Handle UNUSED_XX flags directly
		if (flag_name.rfind("UNUSED_", 0) == 0)
		{
			int bit = std::stoi(flag_name.substr(7));
			size_t plane = bit / 30;
			int bit_in_plane = bit % 30;
			flags.set_flag(plane, 1 << bit_in_plane);
			continue;
		}

		long bit_pos = dm.Lookup(dict_name, flag_name, -1);
		if (bit_pos >= 0)
		{
			size_t plane = bit_pos / 30;
			int bit = bit_pos % 30;
			flags.set_flag(plane, 1 << bit);
		}
	}

	return flags;
}

int YamlWorldDataSource::ParseEnum(const YAML::Node &node, const std::string &dict_name, int default_val) const
{
	if (!node)
	{
		return default_val;
	}

	std::string name = node.as<std::string>();
	// A raw integer value (the converter emits one whenever the enum index
	// has no symbolic name -- e.g. an out-of-range default_pos like 15) must
	// round-trip verbatim instead of collapsing to the default; otherwise
	// legacy<->yaml drift on such values, and the SQLite path (which stores
	// the int directly) would disagree too (issue #3384 class).
	if (!name.empty())
	{
		char *end = nullptr;
		const long as_int = std::strtol(name.c_str(), &end, 10);
		if (*end == '\0')
		{
			return static_cast<int>(as_int);
		}
	}
	auto &dm = DictionaryManager::Instance();
	long val = dm.Lookup(dict_name, name, default_val);
	return static_cast<int>(val);
}

int YamlWorldDataSource::ParsePosition(const YAML::Node &node) const
{
	return ParseEnum(node, "positions", static_cast<int>(EPosition::kStand));
}

int YamlWorldDataSource::ParseGender(const YAML::Node &node, int default_val) const
{
	return ParseEnum(node, "genders", default_val);
}

// ============================================================================
// Zone Loading
// ============================================================================

// Parse single zone file (thread-safe worker function)
ZoneData YamlWorldDataSource::ParseZoneFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);

	// Extract vnum from filepath (pattern: .../zones/VNUM/zone.yaml)
	size_t last_slash = file_path.rfind("/zone.yaml");
	size_t prev_slash = file_path.rfind('/', last_slash - 1);
	std::string vnum_str = file_path.substr(prev_slash + 1, last_slash - prev_slash - 1);
	int zone_vnum = std::atoi(vnum_str.c_str());

	ZoneData zone;
	// vnum is derived solely from the directory name (zones/<vnum>/zone.yaml),
	// which is the discovery authority via zones/index.yaml. The file no longer
	// stores a redundant vnum field (older files may still have one -- ignored).
	zone.vnum = zone_vnum;
	zone.name = GetText(root, "name", "Unknown Zone");
	zone.group = GetInt(root, "zone_group", 1);
	if (zone.group == 0) zone.group = 1;

	// Read metadata subfields
	if (root["metadata"])
	{
		zone.comment = GetText(root["metadata"], "comment");
		zone.location = GetText(root["metadata"], "location");
		zone.author = GetText(root["metadata"], "author");
		zone.description = GetText(root["metadata"], "description");
	}
	zone.top = GetInt(root, "top_room", zone.vnum * 100 + 99);
	zone.lifespan = GetInt(root, "lifespan", 30);
	zone.reset_mode = GetInt(root, "reset_mode", 2);
	zone.reset_idle = GetInt(root, "reset_idle", 0) != 0;
	zone.type = GetInt(root, "zone_type", 0);
	zone.level = GetInt(root, "mode", 0);
	zone.entrance = GetInt(root, "entrance", 0);

	// Initialize runtime fields (uses base class method)
	int under_construction = GetInt(root, "under_construction", 0);
	InitializeZoneRuntimeFields(zone, under_construction);

	// Load zone commands
	if (root["commands"])
	{
		LoadZoneCommands(zone, root["commands"]);
	}
	else
	{
		CREATE(zone.cmd, 1);
		zone.cmd[0].command = 'S';
	}

	// Load zone groups (typeA and typeB)
	if (root["typeA_zones"] && root["typeA_zones"].IsSequence())
	{
		zone.typeA_count = root["typeA_zones"].size();
		CREATE(zone.typeA_list, zone.typeA_count);
		int i = 0;
		for (const auto &z : root["typeA_zones"])
		{
			zone.typeA_list[i++] = z.as<int>();
		}
	}

	if (root["typeB_zones"] && root["typeB_zones"].IsSequence())
	{
		zone.typeB_count = root["typeB_zones"].size();
		CREATE(zone.typeB_list, zone.typeB_count);
		CREATE(zone.typeB_flag, zone.typeB_count);
		int i = 0;
		for (const auto &z : root["typeB_zones"])
		{
			zone.typeB_list[i] = z.as<int>();
			zone.typeB_flag[i] = false;
			i++;
		}
	}

	return zone;
}

// Parallel zone loading
void YamlWorldDataSource::LoadZonesParallel()
{
	utils::CExecutionTimer t_total;
	std::vector<int> zone_vnums = GetZoneList();
	if (zone_vnums.empty())
	{
		log("No zones found in YAML index.");
		return;
	}

	int zone_count = zone_vnums.size();
	zone_table.reserve(zone_count + dungeons::kNumberOfZoneDungeons);
	zone_table.resize(zone_count);
	log("   %d zones, %zd bytes.", zone_count, sizeof(ZoneData) * zone_count);

	// Sort zone vnums to match Legacy loader order (CRITICAL for checksums)
	std::sort(zone_vnums.begin(), zone_vnums.end());

	// Build vnum to index mapping
	std::map<int, size_t> vnum_to_idx;
	for (size_t i = 0; i < zone_vnums.size(); ++i)
	{
		vnum_to_idx[zone_vnums[i]] = i;
	}

	// Distribute zones into batches
	auto batches = utils::DistributeBatches(zone_vnums, m_num_threads);
	std::atomic<int> error_count{0};

	const double disc_ms = t_total.delta().count() * 1000.0;
	utils::CExecutionTimer t_par;

	// Launch parallel loading
	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &vnum_to_idx, &error_count]() {
			for (size_t i = 0; i < batches[thread_id].size(); ++i)
			{
				int zone_vnum = batches[thread_id][i];
				std::string zone_path = m_world_dir + "/zones/" + std::to_string(zone_vnum) + "/zone.yaml";

				try
				{
					size_t zone_idx = vnum_to_idx.at(zone_vnum);
					zone_table[zone_idx] = ParseZoneFile(zone_path);
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load zone %d from '%s': %s", zone_vnum, zone_path.c_str(), e.what());
					error_count++;
				}
				catch (const std::exception &e)
				{
					log("SYSERR: Failed to load zone %d from '%s': %s", zone_vnum, zone_path.c_str(), e.what());
					error_count++;
				}
			}
		}));
	}

	// Wait for all tasks
	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		fatal_log("FATAL: %d zone(s) failed to load. Aborting.", error_count.load());
	}

	const double par_ms = t_par.delta().count() * 1000.0;
	const double total_ms = t_total.delta().count() * 1000.0;
	log("   [yaml-timing] zones (threads=%zu): total=%.1f ms | discovery=%.1f | "
		"parallel-parse=%.1f | merge/post=%.1f",
		m_num_threads, total_ms, disc_ms, par_ms, total_ms - disc_ms - par_ms);
	log("Loaded %d zones from YAML (parallel).", zone_count);
}

void YamlWorldDataSource::EnsureThreadPool()
{
	// Same rationale as the LoadDictionaries() guard below: a composite may
	// call LoadTriggers/Rooms/Mobs/Objects on this instance without its own
	// LoadZones() ever having run (a different source won the index), so the
	// pool can't be assumed to exist yet. Idempotent -- cheap no-op once set.
	if (m_thread_pool)
	{
		return;
	}

	m_num_threads = GetConfiguredThreadCount();
	m_thread_pool = std::make_unique<utils::ThreadPool>(m_num_threads);
}

void YamlWorldDataSource::LoadZones()
{
	log("Loading zones from YAML files.");

	// Load dictionaries first (sequential, writes to singleton)
	if (!LoadDictionaries())
	{
		fatal_log("FATAL: Cannot continue without dictionaries. Aborting.");
	}

	// Get thread count and create thread pool
	EnsureThreadPool();
	log("YAML loading with %zu threads", m_num_threads);

	// Parallel load zones
	LoadZonesParallel();
}

static bool ParseCommandString(const std::string &line, struct reset_com &cmd)
{
	// Strip comment (everything after #)
	std::string s = line;
	auto hash_pos = s.find('#');
	if (hash_pos != std::string::npos)
	{
		s = s.substr(0, hash_pos);
	}
	// Trim whitespace
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s = s.substr(1);

	if (s.empty()) return false;

	std::istringstream iss(s);
	std::string keyword;
	iss >> keyword;

	cmd.if_flag = 0;
	cmd.arg1 = 0;
	cmd.arg2 = 0;
	cmd.arg3 = 0;
	cmd.arg4 = -1;
	cmd.sarg1 = nullptr;
	cmd.sarg2 = nullptr;
	cmd.line = 0;

	if (keyword == "MOB" || keyword == "M")
	{
		cmd.command = 'M';
		int if_flag, mob_vnum, max_world, room_vnum, max_room = -1;
		iss >> if_flag >> mob_vnum >> max_world >> room_vnum;
		iss >> max_room;  // optional
		cmd.if_flag = if_flag;
		cmd.arg1 = mob_vnum;
		cmd.arg2 = max_world;
		cmd.arg3 = room_vnum;
		cmd.arg4 = iss.fail() ? -1 : max_room;
	}
	else if (keyword == "OBJECT" || keyword == "O")
	{
		cmd.command = 'O';
		int if_flag, obj_vnum, max_val, room_vnum, load_prob = -1;
		iss >> if_flag >> obj_vnum >> max_val >> room_vnum;
		iss >> load_prob;
		cmd.if_flag = if_flag;
		cmd.arg1 = obj_vnum;
		cmd.arg2 = max_val;
		cmd.arg3 = room_vnum;
		cmd.arg4 = iss.fail() ? -1 : load_prob;
	}
	else if (keyword == "GIVE" || keyword == "G")
	{
		cmd.command = 'G';
		int if_flag, obj_vnum, max_val, load_prob = -1;
		iss >> if_flag >> obj_vnum >> max_val;
		iss >> load_prob;
		cmd.if_flag = if_flag;
		cmd.arg1 = obj_vnum;
		cmd.arg2 = max_val;
		cmd.arg3 = -1;
		cmd.arg4 = iss.fail() ? -1 : load_prob;
	}
	else if (keyword == "EQUIP" || keyword == "E")
	{
		cmd.command = 'E';
		int if_flag, obj_vnum, max_val, wear_pos, load_prob = -1;
		iss >> if_flag >> obj_vnum >> max_val >> wear_pos;
		iss >> load_prob;
		cmd.if_flag = if_flag;
		cmd.arg1 = obj_vnum;
		cmd.arg2 = max_val;
		cmd.arg3 = wear_pos;
		cmd.arg4 = iss.fail() ? -1 : load_prob;
	}
	else if (keyword == "PUT" || keyword == "P")
	{
		cmd.command = 'P';
		int if_flag, obj_vnum, max_val, container_vnum, load_prob = -1;
		iss >> if_flag >> obj_vnum >> max_val >> container_vnum;
		iss >> load_prob;
		cmd.if_flag = if_flag;
		cmd.arg1 = obj_vnum;
		cmd.arg2 = max_val;
		cmd.arg3 = container_vnum;
		cmd.arg4 = iss.fail() ? -1 : load_prob;
	}
	else if (keyword == "DOOR" || keyword == "D")
	{
		cmd.command = 'D';
		int if_flag, room_vnum, direction, state;
		iss >> if_flag >> room_vnum >> direction >> state;
		cmd.if_flag = if_flag;
		cmd.arg1 = room_vnum;
		cmd.arg2 = direction;
		cmd.arg3 = state;
	}
	else if (keyword == "REMOVE" || keyword == "R")
	{
		cmd.command = 'R';
		int if_flag, room_vnum, obj_vnum;
		iss >> if_flag >> room_vnum >> obj_vnum;
		cmd.if_flag = if_flag;
		cmd.arg1 = room_vnum;
		cmd.arg2 = obj_vnum;
	}
	else if (keyword == "TRIGGER" || keyword == "T")
	{
		cmd.command = 'T';
		int if_flag, trigger_type, trigger_vnum, room_vnum = -1;
		iss >> if_flag >> trigger_type >> trigger_vnum;
		iss >> room_vnum;
		cmd.if_flag = if_flag;
		cmd.arg1 = trigger_type;
		cmd.arg2 = trigger_vnum;
		cmd.arg3 = iss.fail() ? -1 : room_vnum;
	}
	else if (keyword == "VAR" || keyword == "V")
	{
		cmd.command = 'V';
		int if_flag, trigger_type, context, room_vnum;
		iss >> if_flag >> trigger_type >> context >> room_vnum;
		cmd.if_flag = if_flag;
		cmd.arg1 = trigger_type;
		cmd.arg2 = context;
		cmd.arg3 = room_vnum;
		std::string var_name, var_value;
		iss >> var_name;
		std::getline(iss, var_value);
		// Trim leading space from var_value
		if (!var_value.empty() && var_value[0] == ' ') var_value = var_value.substr(1);
		if (!var_name.empty()) cmd.sarg1 = str_dup(var_name.c_str());
		if (!var_value.empty()) cmd.sarg2 = str_dup(var_value.c_str());
	}
	else if (keyword == "EXTRACT" || keyword == "Q")
	{
		cmd.command = 'Q';
		int if_flag, mob_vnum;
		iss >> if_flag >> mob_vnum;
		cmd.if_flag = 0;  // forced to 0
		cmd.arg1 = mob_vnum;
	}
	else if (keyword == "FOLLOW" || keyword == "F")
	{
		cmd.command = 'F';
		int if_flag, room_vnum, leader_vnum, follower_vnum;
		iss >> if_flag >> room_vnum >> leader_vnum >> follower_vnum;
		cmd.if_flag = if_flag;
		cmd.arg1 = room_vnum;
		cmd.arg2 = leader_vnum;
		cmd.arg3 = follower_vnum;
	}
	else
	{
		return false;
	}

	return true;
}

void YamlWorldDataSource::LoadZoneCommands(ZoneData &zone, const YAML::Node &commands_node)
{
	if (!commands_node.IsSequence())
	{
		CREATE(zone.cmd, 1);
		zone.cmd[0].command = 'S';
		return;
	}

	int cmd_count = commands_node.size();
	CREATE(zone.cmd, cmd_count + 1);

	int idx = 0;
	for (const auto &cmd_node : commands_node)
	{
		struct reset_com &cmd = zone.cmd[idx];

		cmd.command = '*';
		cmd.if_flag = 0;
		cmd.arg1 = 0;
		cmd.arg2 = 0;
		cmd.arg3 = 0;
		cmd.arg4 = -1;
		cmd.sarg1 = nullptr;
		cmd.sarg2 = nullptr;
		cmd.line = 0;

		if (!cmd_node.IsScalar())
		{
			log("SYSERR: Zone %d: command is not a string", zone.vnum);
			continue;
		}
		std::string cmd_str = cmd_node.as<std::string>();
		if (!ParseCommandString(cmd_str, cmd))
		{
			// Skip unknown/empty commands
			continue;
		}

		idx++;
	}

	zone.cmd[idx].command = 'S';
}

// ============================================================================
// Trigger Loading
// ============================================================================

// Parse single trigger file (thread-safe worker function)
Trigger* YamlWorldDataSource::ParseTriggerFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);
	return ParseTriggerNode(root);
}

// Parse a single trigger from an already-loaded YAML node. Shared by the
// per-file layout (one node per file) and the flat layout (one node per
// rel-number entry in triggers.yaml).
Trigger* YamlWorldDataSource::ParseTriggerNode(const YAML::Node &root)
{
	std::string name = GetText(root, "name", "");
	int attach_type = ParseEnum(root["attach_type"], "attach_types", 0);

	// Parse trigger types
	long trigger_type = 0;
	if (root["trigger_types"] && root["trigger_types"].IsSequence())
	{
		for (const auto &type_node : root["trigger_types"])
		{
			std::string type_str = type_node.as<std::string>();
			auto &dm = DictionaryManager::Instance();
			long type_bit = dm.Lookup("trigger_types", type_str, -1);
			if (type_bit >= 0)
			{
				trigger_type |= (1L << type_bit);
			}
		}
	}

	int narg = GetInt(root, "narg", 0);
	std::string arglist = GetText(root, "arglist", "");
	std::string script = GetText(root, "script", "");
	const auto script_language = ParseTriggerScriptLanguage(root);

	// Create trigger (note: rnum will be assigned during merge)
	auto trig = new Trigger(-1, std::move(name), static_cast<byte>(attach_type), trigger_type);
	GET_TRIG_NARG(trig) = narg;
	trig->arglist = arglist;
	trig->set_script_language(script_language);

	if (script_language == TriggerScriptLanguage::Lua)
	{
		trig->set_lua_script_source(script);
	}
	else
	{
		ParseTriggerScript(trig, script);
	}

	// Note: vnum will be passed separately in thread results
	return trig;
}

// Parallel trigger loading
// The only trigger-loading entry point: `zone_filter` null means every zone.
// Parses in parallel across zones/tasks and returns the results WITHOUT
// touching trig_index/top_of_trigt -- the caller (GameLoader::BootWorld's
// orchestrator in db.cpp) does that placement once, after possibly combining
// results from other sources too (composite).
std::vector<LoadedTrigger> YamlWorldDataSource::LoadTriggers(const std::vector<int> *zone_filter)
{
	// Callers may invoke this without this instance's own LoadZones() ever
	// having run (a composite may have picked a DIFFERENT source to supply
	// zone_table/the index) -- LoadDictionaries() is otherwise only reached
	// from LoadZones(), so without this guard every ParseEnum/ParseFlags
	// lookup below would silently fall back to its default value (issue found
	// via per-zone round-trip testing: a room's sector and exits came back
	// wrong when SQLite, not YAML, won the index).
	if (!LoadDictionaries())
	{
		fatal_log("FATAL: Cannot continue without dictionaries. Aborting.");
	}
	EnsureThreadPool();

	utils::CExecutionTimer t_total;
	std::vector<EntityFileTask> tasks = DiscoverEntityFiles("triggers", zone_filter);
	if (tasks.empty())
	{
		log("No triggers found in YAML index.");
		return {};
	}

	// Distribute trigger files into batches (flat task = many vnums, per-file = one)
	auto batches = utils::DistributeBatches(tasks, m_num_threads);

	// Thread-local results (each thread collects its triggers)
	std::vector<std::vector<LoadedTrigger>> thread_results(batches.size());
	std::atomic<int> error_count{0};

	const double disc_ms = t_total.delta().count() * 1000.0;
	utils::CExecutionTimer t_par;

	// Launch parallel loading
	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &thread_results, &error_count]() {
			for (const auto &task : batches[thread_id])
			{
				YAML::Node file_root;
				if (task.flat && task.parsed_root)
				{
					// Already parsed by DiscoverEntityFiles() to enumerate
					// this task's vnums -- reuse it instead of parsing the
					// same file again.
					file_root = task.parsed_root;
				}
				else
				{
					try
					{
						file_root = YAML::LoadFile(task.path);
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to load triggers from '%s': %s", task.path.c_str(), e.what());
						error_count += static_cast<int>(task.vnums.size());
						continue;
					}
				}
				for (int vnum : task.vnums)
				{
					const YAML::Node node = task.flat ? file_root[vnum % 100] : file_root;
					try
					{
						Trigger* trig = ParseTriggerNode(node);
						thread_results[thread_id].push_back(LoadedTrigger{vnum, trig});
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to parse trigger %d from '%s': %s", vnum, task.path.c_str(), e.what());
						error_count++;
					}
				}
			}
		}));
	}

	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		fatal_log("FATAL: %d trigger(s) failed to load. Aborting.", error_count.load());
	}

	const double par_ms = t_par.delta().count() * 1000.0;

	std::vector<LoadedTrigger> all_triggers;
	for (auto &results : thread_results)
	{
		all_triggers.insert(all_triggers.end(), std::make_move_iterator(results.begin()),
			std::make_move_iterator(results.end()));
	}

	const double total_ms = t_total.delta().count() * 1000.0;
	log("   [yaml-timing] triggers (threads=%zu): total=%.1f ms | discovery=%.1f | "
		"parallel-parse=%.1f | merge/post=%.1f",
		m_num_threads, total_ms, disc_ms, par_ms, total_ms - disc_ms - par_ms);
	log("Loaded %zu triggers from YAML.", all_triggers.size());
	return all_triggers;
}

// ============================================================================
// Room Loading
// ============================================================================

// Parse single room file (thread-safe worker function)
RoomData* YamlWorldDataSource::ParseRoomFile(const std::string &file_path, int zone_rnum, LocalDescriptionIndex &local_index, size_t &local_desc_idx)
{
	YAML::Node root = YAML::LoadFile(file_path);

	// Extract vnum from filename
	size_t last_slash = file_path.find_last_of('/');
	size_t last_dot = file_path.find_last_of('.');

	std::string filename = file_path.substr(last_slash + 1, last_dot - last_slash - 1);
	int rel_num = std::atoi(filename.c_str());

	// Extract zone vnum from path
	size_t zone_start = file_path.rfind("/zones/") + 7;
	size_t zone_end = file_path.find("/", zone_start);
	int zone_vnum = std::atoi(file_path.substr(zone_start, zone_end - zone_start).c_str());
	int vnum = zone_vnum * 100 + rel_num;

	return ParseRoomNode(root, vnum, zone_rnum, local_index, local_desc_idx);
}

// Parse a single room from an already-loaded YAML node. Shared by the per-file
// layout (vnum derived from the filename) and the flat layout (vnum derived
// from the rel-number map key). local_desc_idx is an out-param.
RoomData* YamlWorldDataSource::ParseRoomNode(const YAML::Node &root, int vnum, int zone_rnum, LocalDescriptionIndex &local_index, size_t &local_desc_idx)
{
	auto room = new RoomData;
	room->vnum = vnum;
	room->zone_rn = zone_rnum;

	std::string name = GetText(root, "name", "Untitled Room");
	if (!name.empty()) { name[0] = UPPER(name[0]); }
	room->set_name(name);

	std::string description = GetText(root, "description", "");
	if (!description.empty())
	{
		// Add to thread-local index (no mutex needed!)
		local_desc_idx = local_index.add(description);
		// description_num will be set in merge phase
		room->description_num = 0;
	}
	else
	{
		local_desc_idx = 0;  // No description
		room->description_num = 0;
	}

	// Parse flags and sector
	if (root["flags"] && root["flags"].IsSequence())
	{
		auto &dm = DictionaryManager::Instance();
		for (const auto &flag_node : root["flags"])
		{
			std::string flag_name = flag_node.as<std::string>();
			long flag_val = dm.Lookup("room_flags", flag_name, -1);
			if (flag_val >= 0)
			{
				room->set_flag(static_cast<ERoomFlag>(IndexToBitvector(flag_val)));
			}
		}
	}
	room->sector_type = ParseEnum(root["sector"], "sectors", 0);

	// Load exits
	if (root["exits"])
	{
		LoadRoomExits(room, root["exits"], vnum);
	}

	// Load extra descriptions
	if (root["extra_descriptions"])
	{
		LoadRoomExtraDescriptions(room, root["extra_descriptions"]);
	}

	return room;
}

// Parallel room loading
// Parallel room loading
// The only room-loading entry point: `zone_filter` null means every zone.
// Parses in parallel across zones/tasks and returns the results WITHOUT
// touching world[]/top_of_world/zone_table[].RnumRoomsLocation or attaching
// triggers -- the caller (GameLoader::BootWorld's orchestrator in db.cpp)
// does all of that once, after possibly combining results from other
// sources too (composite). Room description dedup (GlobalObjects::
// descriptions()) still merges per-source here, since it's cheap and every
// source already funnels through the same global singleton either way.
std::vector<LoadedRoom> YamlWorldDataSource::LoadRooms(const std::vector<int> *zone_filter)
{
	if (!LoadDictionaries())
	{
		fatal_log("FATAL: Cannot continue without dictionaries. Aborting.");
	}
	EnsureThreadPool();

	utils::CExecutionTimer t_total;
	std::vector<EntityFileTask> tasks = DiscoverEntityFiles("rooms", zone_filter);
	if (tasks.empty())
	{
		log("No rooms found in YAML files.");
		return {};
	}

	// Distribute room files into batches (flat task = many vnums, per-file = one)
	auto batches = utils::DistributeBatches(tasks, m_num_threads);
	std::atomic<int> error_count{0};

	const double disc_ms = t_total.delta().count() * 1000.0;
	utils::CExecutionTimer t_par;

	// Launch parallel loading with thread-local description indices
	std::vector<std::future<ParsedRoomBatch>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &error_count]() -> ParsedRoomBatch {
			// Thread-local variables
			LocalDescriptionIndex local_index;
			std::vector<std::tuple<int, RoomData*, size_t>> parsed_rooms;
			std::map<int, std::vector<int>> local_triggers;

			for (const auto &task : batches[thread_id])
			{
				YAML::Node file_root;
				if (task.flat && task.parsed_root)
				{
					file_root = task.parsed_root;
				}
				else
				{
					try
					{
						file_root = YAML::LoadFile(task.path);
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to load rooms from '%s': %s", task.path.c_str(), e.what());
						error_count += static_cast<int>(task.vnums.size());
						continue;
					}
				}
				for (int vnum : task.vnums)
				{
					const YAML::Node node = task.flat ? file_root[vnum % 100] : file_root;
					try
					{
						// Calculate zone_rnum from vnum
						ZoneRnum zone_rn = 0;
						while (zone_rn < static_cast<ZoneRnum>(zone_table.size()) &&
						       vnum > zone_table[zone_rn].top)
						{
							zone_rn++;
						}

						size_t local_desc_idx = 0;
						RoomData* room = ParseRoomNode(node, vnum, zone_rn, local_index, local_desc_idx);

						// Load room triggers (if present)
						if (node["triggers"] && node["triggers"].IsSequence())
						{
							std::vector<int> trigger_list;
							for (const auto &trig_node : node["triggers"])
							{
								trigger_list.push_back(trig_node.as<int>());
							}
							if (!trigger_list.empty())
							{
								local_triggers[vnum] = std::move(trigger_list);
							}
						}

						// Store vnum, room, and local description index
						parsed_rooms.push_back(std::make_tuple(vnum, room, local_desc_idx));
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to parse room %d from '%s': %s", vnum, task.path.c_str(), e.what());
						error_count++;
					}
				}
			}

			return ParsedRoomBatch{std::move(local_index), std::move(parsed_rooms), std::move(local_triggers)};
		}));
	}

	// Collect results from all threads
	std::vector<ParsedRoomBatch> parsed_batches;
	parsed_batches.reserve(futures.size());
	for (auto &future : futures)
	{
		parsed_batches.push_back(future.get());
	}

	if (error_count > 0)
	{
		fatal_log("FATAL: %d room(s) failed to load. Aborting.", error_count.load());
	}

	const double par_ms = t_par.delta().count() * 1000.0;

	// Merge descriptions from all batches
	auto &global_descriptions = GlobalObjects::descriptions();
	std::vector<std::vector<size_t>> local_to_global(parsed_batches.size());

	for (size_t batch_id = 0; batch_id < parsed_batches.size(); ++batch_id)
	{
		local_to_global[batch_id] = global_descriptions.merge(parsed_batches[batch_id].descriptions);
	}

	std::vector<LoadedRoom> all_rooms;
	for (size_t batch_id = 0; batch_id < parsed_batches.size(); ++batch_id)
	{
		for (auto &[vnum, room, local_desc_idx] : parsed_batches[batch_id].rooms)
		{
			// local_desc_idx is 1-based (0 = no description, 1 = first description)
			// local_to_global is 0-indexed vector
			if (local_desc_idx > 0)
			{
				size_t local_idx_0based = local_desc_idx - 1;
				if (local_idx_0based < local_to_global[batch_id].size())
				{
					room->description_num = local_to_global[batch_id][local_idx_0based];
				}
			}

			LoadedRoom lr;
			lr.vnum = vnum;
			lr.room = room;
			auto trig_it = parsed_batches[batch_id].triggers.find(vnum);
			if (trig_it != parsed_batches[batch_id].triggers.end())
			{
				lr.triggers = trig_it->second;
			}
			all_rooms.push_back(std::move(lr));
		}
	}

	// Sort by vnum (CRITICAL for correct indexing -- see world_data_source.h)
	std::sort(all_rooms.begin(), all_rooms.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	log("   Merged %zu unique room descriptions from %zu threads.", global_descriptions.size(), parsed_batches.size());

	const double total_ms = t_total.delta().count() * 1000.0;
	log("   [yaml-timing] rooms (threads=%zu): total=%.1f ms | discovery=%.1f | "
		"parallel-parse=%.1f | merge/post=%.1f",
		m_num_threads, total_ms, disc_ms, par_ms, total_ms - disc_ms - par_ms);
	return all_rooms;
}

void YamlWorldDataSource::LoadRoomExits(RoomData *room, const YAML::Node &exits_node, int room_vnum)
{
	if (!exits_node.IsSequence())
	{
		return;
	}

	for (const auto &exit_node : exits_node)
	{
		int dir = ParseEnum(exit_node["direction"], "directions", 0);
		if (dir < 0 || dir >= EDirection::kMaxDirNum)
		{
			log("SYSERR: Room %d has invalid exit direction %d, skipping.", room_vnum, dir);
			continue;
		}

		auto exit_data = std::make_shared<ExitData>();

		exit_data->to_room(GetInt(exit_node, "to_room", -1));
		exit_data->key = GetInt(exit_node, "key", -1);
		exit_data->lock_complexity = GetInt(exit_node, "lock_complexity", 0);

		std::string desc = GetText(exit_node, "description", "");
		if (!desc.empty()) exit_data->general_description = desc;

		std::string keywords = GetText(exit_node, "keywords", "");
		if (!keywords.empty()) exit_data->set_keywords(keywords);

		exit_data->exit_info = GetInt(exit_node, "exit_flags", 0);

		// Дропаем полностью пустые D-блоки (симметрично с legacy/sqlite),
		// см. issue #3272.
		if (exit_data->is_empty()) continue;

		room->dir_option_proto[dir] = exit_data;
	}
}

void YamlWorldDataSource::LoadRoomExtraDescriptions(RoomData *room, const YAML::Node &extras_node)
{
	if (!extras_node.IsSequence())
	{
		return;
	}

	for (const auto &ed_node : extras_node)
	{
		std::string keywords = GetText(ed_node, "keywords", "");
		std::string description = GetText(ed_node, "description", "");

		ExtraDescription ex_desc;
		ex_desc.set_keyword(keywords);
		ex_desc.set_description(description);
		room->ex_description.push_back(std::move(ex_desc));
	}
}

// ============================================================================
// Mob Loading
// ============================================================================

// Parse single mob file (thread-safe worker function)
CharData YamlWorldDataSource::ParseMobFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);
	return ParseMobNode(root);
}

// Parse a single mob from an already-loaded YAML node. Shared by the per-file
// and flat layouts; vnum is assigned by the caller during the merge phase.
CharData YamlWorldDataSource::ParseMobNode(const YAML::Node &root)
{
	CharData mob;
	mob.player_specials = player_special_data::s_for_mobiles;
	mob.SetNpcAttribute(true);
	// NameGod=1001 is initialized once on the shared singleton in
	// LoadMobsParallel before worker threads start -- see InitMobSharedDefaults().
	// Writing it here would race across threads (same value, but technically UB).
	mob.set_move(100);
	mob.set_max_move(100);

	// Names
	YAML::Node names = root["names"];
	if (names)
	{
		mob.SetCharAliases(GetText(names, "aliases"));
		mob.set_npc_name(GetText(names, "nominative"));
		mob.player_data.PNames[grammar::ECase::kNom] = GetText(names, "nominative");
		mob.player_data.PNames[grammar::ECase::kGen] = GetText(names, "genitive");
		mob.player_data.PNames[grammar::ECase::kDat] = GetText(names, "dative");
		mob.player_data.PNames[grammar::ECase::kAcc] = GetText(names, "accusative");
		mob.player_data.PNames[grammar::ECase::kIns] = GetText(names, "instrumental");
		mob.player_data.PNames[grammar::ECase::kPre] = GetText(names, "prepositional");
	}

	// Descriptions
	YAML::Node descs = root["descriptions"];
	if (descs)
	{
		mob.player_data.long_descr = utils::colorCAP(GetText(descs, "short_desc"));
		mob.player_data.description = GetText(descs, "long_desc");
	}

	// Base parameters
	alignment::SetAlignment(&mob, GetInt(root, "alignment", 0));

	// Stats
	YAML::Node stats = root["stats"];
	if (stats)
	{
		mob.set_level(GetInt(stats, "level", 1));
		// hitroll_penalty and armor are stored in the YAML in legacy file
		// units (i.e. before parse_simple_mob's `20 - x` and `10 * x`
		// transformations), matching what convert_to_yaml.py emits. Applying
		// the same conversions here is required for round-trip: otherwise
		// legacy -> yaml -> legacy oscillates the on-disk hitroll value
		// (e.g. 5 -> 15 -> 5 -> 15) every pass.
		GET_HR(&mob) = 20 - GetInt(stats, "hitroll_penalty", 20);
		GET_AC(&mob) = 10 * GetInt(stats, "armor", 10);

		YAML::Node hp = stats["hp"];
		if (hp)
		{
			mob.mem_queue.total = GetInt(hp, "dice_count", 1);
			mob.mem_queue.stored = GetInt(hp, "dice_size", 1);
			int hp_bonus = GetInt(hp, "bonus", 0);
			mob.set_hit(hp_bonus);
			mob.set_max_hit(0);
		}

		YAML::Node dmg = stats["damage"];
		if (dmg)
		{
			mob.mob_specials.damnodice = GetInt(dmg, "dice_count", 1);
			mob.mob_specials.damsizedice = GetInt(dmg, "dice_size", 1);
			mob.real_abils.damroll = GetInt(dmg, "bonus", 0);
		}
	}

	// Gold
	YAML::Node gold = root["gold"];
	if (gold)
	{
		mob.mob_specials.GoldNoDs = GetInt(gold, "dice_count", 0);
		mob.mob_specials.GoldSiDs = GetInt(gold, "dice_size", 0);
		currencies::SetHand(mob, currencies::kGold, GetInt(gold, "bonus", 0));
	}

	// Experience
	mob.set_exp(GetInt(root, "experience", 0));

	// Position
	YAML::Node pos = root["position"];
	if (pos)
	{
		mob.mob_specials.default_pos = static_cast<EPosition>(ParsePosition(pos["default"]));
		mob.SetPosition(static_cast<EPosition>(ParsePosition(pos["start"])));
	}
	else
	{
		mob.mob_specials.default_pos = EPosition::kStand;
		mob.SetPosition(EPosition::kStand);
	}

	// Sex
	mob.set_sex(static_cast<EGender>(ParseGender(root["sex"])));

	// Movement speed. parse_simple_mob leaves speed == -1 when the position
	// line has only three fields (the common case) and stores an explicit
	// value otherwise. The converter omits the -1 default, so fall back to
	// -1 here -- without it every such mob loads with speed 0 and walks on
	// the wrong cadence (same dropped-property class as #3384/#3386).
	mob.mob_specials.speed = GetInt(root, "speed", -1);

	// Race -- legacy clamps to [kBasic, kLastNpcRace] (boot_data_files.cpp).
	mob.player_data.Race = std::clamp(static_cast<ENpcRace>(GetInt(root, "race", ENpcRace::kBasic)),
									  ENpcRace::kBasic, ENpcRace::kLastNpcRace);

	// Physical attributes -- bounds mirror legacy interpret_espec.
	GET_SIZE(&mob) = std::clamp<byte>(GetInt(root, "size", 0), 0, 100);
	GET_HEIGHT(&mob) = std::clamp(GetInt(root, "height", 0), 0, 200);
	GET_WEIGHT(&mob) = std::clamp(GetInt(root, "weight", 0), 0, 200);

	// E-spec attributes - set defaults, then override
	mob.set_str(11);
	mob.set_dex(11);
	mob.set_int(11);
	mob.set_wis(11);
	mob.set_con(11);
	mob.set_cha(11);
	if (root["attributes"])
	{
		YAML::Node attrs = root["attributes"];
		mob.set_str(GetInt(attrs, "strength", 11));
		mob.set_dex(GetInt(attrs, "dexterity", 11));
		mob.set_int(GetInt(attrs, "intelligence", 11));
		mob.set_wis(GetInt(attrs, "wisdom", 11));
		mob.set_con(GetInt(attrs, "constitution", 11));
		mob.set_cha(GetInt(attrs, "charisma", 11));
	}

	// Flags
	// NOTE: dictionary returns the bit *index* (0..119); FlagData::set() expects a
	// packed value: (plane << 30) | (1 << bit_in_plane). Use flag_data_by_num()
	// to convert. Passing the raw index is the bug that corrupted action/affect
	// flags on mobs (e.g. set(13) ORed bits 0,2,3 instead of bit 13).
	auto &dm = DictionaryManager::Instance();
	if (root["action_flags"] && root["action_flags"].IsSequence())
	{
		for (const auto &flag_node : root["action_flags"])
		{
			std::string flag_name = flag_node.as<std::string>();
			long flag_val = dm.Lookup("action_flags", flag_name, -1);
			if (flag_val >= 0)
			{
				mob.SetFlag(static_cast<EMobFlag>(flag_data_by_num(flag_val)));
			}
		}
	}

	if (root["affect_flags"] && root["affect_flags"].IsSequence())
	{
		for (const auto &flag_node : root["affect_flags"])
		{
			std::string flag_name = flag_node.as<std::string>();
			long flag_val = dm.Lookup("affect_flags", flag_name, -1);
			if (flag_val >= 0)
			{
				AFF_FLAGS(&mob).set_index(flag_val);
			}
		}
	}

	// Skills
	if (root["skills"] && root["skills"].IsSequence())
	{
		for (const auto &skill_node : root["skills"])
		{
			int skill_id = GetInt(skill_node, "skill_id", 0);
			int value = GetInt(skill_node, "value", 0);
			SetSkill(&mob, static_cast<ESkill>(skill_id), value);
		}
	}

	// Enhanced E-spec fields
	if (root["enhanced"])
	{
		YAML::Node enhanced = root["enhanced"];

		// Clamps mirror MobileFile::interpret_espec in boot_data_files.cpp.
		// Без них старые данные (например, MaxFactor: 1000 у моба 200300)
		// попадали в YAML/SQLite как сырые значения и расходились с легаси.
		mob.set_str_add(GetInt(enhanced, "str_add", 0));
		mob.add_abils.hitreg = std::clamp(GetInt(enhanced, "hp_regen", 0), -200, 200);
		mob.add_abils.armour = std::clamp(GetInt(enhanced, "armour_bonus", 0), 0, 100);
		mob.add_abils.manareg = std::clamp(GetInt(enhanced, "mana_regen", 0), -200, 200);
		mob.add_abils.cast_success = std::clamp(GetInt(enhanced, "cast_success", 0), -200, 300);
		mob.add_abils.morale = std::clamp(GetInt(enhanced, "morale", 0), 0, 100);
		mob.add_abils.initiative_add = std::clamp(GetInt(enhanced, "initiative_add", 0), -200, 200);
		mob.add_abils.absorb = std::clamp(GetInt(enhanced, "absorb", 0), -200, 200);
		mob.add_abils.aresist = std::clamp(GetInt(enhanced, "aresist", 0), 0, 100);
		mob.add_abils.mresist = std::clamp(GetInt(enhanced, "mresist", 0), 0, 100);
		mob.add_abils.presist = std::clamp(GetInt(enhanced, "presist", 0), 0, 100);
		mob.mob_specials.attack_type = std::clamp(GetInt(enhanced, "bare_hand_attack", 0), 0, 99);
		mob.mob_specials.like_work = std::clamp<byte>(GetInt(enhanced, "like_work", 0), 0, 100);
		mob.mob_specials.MaxFactor = std::clamp<byte>(GetInt(enhanced, "max_factor", 0), 0, 127);
		mob.mob_specials.extra_attack = std::clamp<byte>(GetInt(enhanced, "extra_attack", 0), 0, 127);
		mob.set_remort(std::clamp<byte>(GetInt(enhanced, "mob_remort", 0), 0, 100));

		if (enhanced["special_bitvector"])
		{
			std::string special_bv = enhanced["special_bitvector"].as<std::string>();
			mob.mob_specials.npc_flags.from_string((char *)special_bv.c_str());
		}

		if (enhanced["role"])
		{
			std::string role_str = enhanced["role"].as<std::string>();
			CharData::role_t role(role_str);
			mob.set_role(role);
		}

		// Resistances. The current format is a named map (kFire: 10, kMind: 5,
		// ...) so the meaning of each value is explicit and the file order is
		// decoupled from the EResist enum order. A missing key means 0 (the
		// loader's zero-initialized default), so only non-zero entries appear.
		if (enhanced["resistances"])
		{
			const auto &node = enhanced["resistances"];
			if (node.IsMap())
			{
				for (const auto &kv : node)
				{
					const std::string key = kv.first.as<std::string>();
					try
					{
						const auto r = ITEM_BY_NAME<EResist>(key);
						mob.add_abils.apply_resistance[r] =
							std::clamp(kv.second.as<int>(), kMinResistance, kMaxNpcResist);
					}
					catch (const std::out_of_range &)
					{
						log("SYSERR: unknown EResist '%s' on mob '%s', skipped",
							key.c_str(), mob.get_npc_name().c_str());
					}
				}
			}
			else if (node.IsSequence())
			{
				// DEPRECATED: legacy positional list [v0, v1, ...]. Read for
				// backward compatibility with worlds generated before the named
				// map; remove once the new format has settled and no such worlds
				// remain (re-saving any world rewrites it as a map).
				int idx = 0;
				for (const auto &val_node : node)
				{
					int value = std::clamp(val_node.as<int>(), kMinResistance, kMaxNpcResist);
					if (idx < static_cast<int>(mob.add_abils.apply_resistance.size()))
					{
						mob.add_abils.apply_resistance[idx] = value;
					}
					idx++;
				}
			}
		}

		// Saves -- same named-map format and rationale as resistances.
		if (enhanced["saves"])
		{
			const auto &node = enhanced["saves"];
			if (node.IsMap())
			{
				for (const auto &kv : node)
				{
					const std::string key = kv.first.as<std::string>();
					try
					{
						const int s = to_underlying(ITEM_BY_NAME<ESaving>(key));
						if (s >= 0 && s < static_cast<int>(mob.add_abils.apply_saving_throw.size()))
						{
							mob.add_abils.apply_saving_throw[s] =
								std::clamp(kv.second.as<int>(), kMinSaving, kMaxSaving);
						}
					}
					catch (const std::out_of_range &)
					{
						log("SYSERR: unknown ESaving '%s' on mob '%s', skipped",
							key.c_str(), mob.get_npc_name().c_str());
					}
				}
			}
			else if (node.IsSequence())
			{
				// DEPRECATED: legacy positional list -- see the resistances
				// branch above. Kept for backward compatibility, remove once the
				// named map format has settled.
				int idx = 0;
				for (const auto &val_node : node)
				{
					int value = std::clamp(val_node.as<int>(), kMinSaving, kMaxSaving);
					if (idx < static_cast<int>(mob.add_abils.apply_saving_throw.size()))
					{
						mob.add_abils.apply_saving_throw[idx] = value;
					}
					idx++;
				}
			}
		}

		if (enhanced["feats"] && enhanced["feats"].IsSequence())
		{
			for (const auto &feat_node : enhanced["feats"])
			{
				int feat_id = feat_node.as<int>();
				if (feat_id >= 0 && feat_id < static_cast<int>(mob.real_abils.Feats.size()))
				{
					mob.real_abils.Feats.set(feat_id);
				}
			}
		}

		if (enhanced["spells"])
		{
			// Начисляет мобу count слотов спелла (SplMem), обновляя caster_level
			// и have_spell так же, как легаси-парсер `Spell: N`
			// (boot_data_files.cpp). SplKnw был yaml/sqlite-only сокращением,
			// терявшим кратность, из-за чего кастер после загрузки оставался
			// фактически без спеллов.
			const auto add_spell_mem = [&mob](int spell_id_int, int count) {
				if (count <= 0) { return; }
				if (spell_id_int < to_underlying(ESpell::kFirst)
					|| spell_id_int > to_underlying(ESpell::kLast))
				{
					log("SYSERR: Unknown spell id %d in mob yaml", spell_id_int);
					return;
				}
				const auto spell_id = static_cast<ESpell>(spell_id_int);
				GET_SPELL_MEM(&mob, spell_id) += count;
				mob.caster_level += (MUD::Spell(spell_id).IsFlagged(NPC_CALCULATE) ? count : 0);
				mob.mob_specials.have_spell = true;
			};
			const auto &spells_node = enhanced["spells"];
			if (spells_node.IsMap())
			{
				// Новый формат: `id: count` (по записи на спелл).
				for (const auto &kv : spells_node)
				{
					add_spell_mem(kv.first.as<int>(), kv.second.as<int>());
				}
			}
			else if (spells_node.IsSequence())
			{
				// Старый формат: id повторяется по разу на каждый слот.
				for (const auto &spell_node : spells_node)
				{
					add_spell_mem(spell_node.as<int>(), 1);
				}
			}
		}

		if (enhanced["helpers"] && enhanced["helpers"].IsSequence())
		{
			for (const auto &helper_node : enhanced["helpers"])
			{
				int helper_vnum = helper_node.as<int>();
				mob.summon_helpers.push_back(helper_vnum);
			}
		}

		if (enhanced["destinations"] && enhanced["destinations"].IsSequence())
		{
			int idx = 0;
			for (const auto &dest_node : enhanced["destinations"])
			{
				if (idx >= static_cast<int>(mob.mob_specials.dest.size()))
				{
					break;
				}
				mob.mob_specials.dest[idx] = dest_node.as<int>();
				idx++;
			}
			// dest_count drives GET_DEST and the whole movement-route logic;
			// without it the dest[] array we just filled stays invisible and
			// the mob never walks its patrol (issue #3384, matches the legacy
			// interpret_espec "Destination" handler in boot_data_files.cpp).
			mob.mob_specials.dest_count = idx;
		}
	}

	// Dead-load entries (legacy `L` lines, issue #3291). At top level of the
	// mob document, not under `enhanced`, so the YAML matches the legacy
	// position-after-E-section semantics.
	if (root["dead_load"] && root["dead_load"].IsSequence())
	{
		for (const auto &node : root["dead_load"])
		{
			dead_load::LoadingItem item;
			item.obj_vnum = GetInt(node, "obj_vnum", 0);
			item.load_prob = GetInt(node, "load_prob", 0);
			item.load_type = GetInt(node, "load_type", 0);
			item.spec_param = GetInt(node, "spec_param", 0);
			mob.dl_list.push_back(item);
		}
	}

	// Initialize test data if needed
	if (mob.GetLevel() == 0)
		SetTestData(&mob);

	return mob;
}

// Parallel mob loading
// The only mob-loading entry point: `zone_filter` null means every zone.
// Parses in parallel across zones/tasks and returns the results WITHOUT
// touching mob_proto[]/mob_index[]/top_of_mobt/zone_table[].RnumMobsLocation
// or attaching triggers -- the caller (GameLoader::BootWorld's orchestrator
// in db.cpp) does all of that once, after possibly combining results from
// other sources too (composite).
std::vector<LoadedMob> YamlWorldDataSource::LoadMobs(const std::vector<int> *zone_filter)
{
	if (!LoadDictionaries())
	{
		fatal_log("FATAL: Cannot continue without dictionaries. Aborting.");
	}
	EnsureThreadPool();

	utils::CExecutionTimer t_total;
	std::vector<EntityFileTask> tasks = DiscoverEntityFiles("mobs", zone_filter);
	if (tasks.empty())
	{
		log("No mobs found in YAML index.");
		return {};
	}

	// One-time init for shared mob singleton state -- done in the main thread
	// before workers start, so worker threads never write to globals. Cheap
	// and idempotent, so no harm in repeating it across sources/calls.
	player_special_data::s_for_mobiles->saved.NameGod = 1001;

	// Distribute mob files into batches (each task is one file; a flat file
	// carries many vnums, a per-file task exactly one).
	auto batches = utils::DistributeBatches(tasks, m_num_threads);
	std::atomic<int> error_count{0};

	// Two-phase loading: workers only parse into thread-local buffers; the
	// merge below builds the final sorted list in a single thread. This
	// prevents data races on (a) the shared player_specials singleton, (b)
	// ProtectedCharData::operator= observer callbacks, and (c)
	// caching::character_cache contention from CharData copy/destroy in
	// CharData::operator= side-effects across threads.
	std::vector<std::vector<LoadedMob>> thread_results(batches.size());

	const double disc_ms = t_total.delta().count() * 1000.0;
	utils::CExecutionTimer t_par;

	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &thread_results, &error_count]() {
			for (const auto &task : batches[thread_id])
			{
				YAML::Node file_root;
				if (task.flat && task.parsed_root)
				{
					file_root = task.parsed_root;
				}
				else
				{
					try
					{
						file_root = YAML::LoadFile(task.path);
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to load mobs from '%s': %s", task.path.c_str(), e.what());
						error_count += static_cast<int>(task.vnums.size());
						continue;
					}
				}
				for (int vnum : task.vnums)
				{
					// Flat: entity is the node at the rel-number key. Per-file:
					// the file root IS the entity node.
					const YAML::Node node = task.flat ? file_root[vnum % 100] : file_root;
					try
					{
						LoadedMob lm;
						lm.vnum = vnum;
						lm.mob = new CharData(ParseMobNode(node));

						if (node["triggers"] && node["triggers"].IsSequence())
						{
							for (const auto &trig_node : node["triggers"])
							{
								lm.triggers.push_back(trig_node.as<int>());
							}
						}
						thread_results[thread_id].push_back(std::move(lm));
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to parse mob %d from '%s': %s", vnum, task.path.c_str(), e.what());
						error_count++;
					}
				}
			}
		}));
	}

	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		fatal_log("FATAL: %d mob(s) failed to load. Aborting.", error_count.load());
	}

	const double par_ms = t_par.delta().count() * 1000.0;

	std::vector<LoadedMob> all_mobs;
	for (auto &results : thread_results)
	{
		all_mobs.insert(all_mobs.end(), std::make_move_iterator(results.begin()),
			std::make_move_iterator(results.end()));
	}
	std::sort(all_mobs.begin(), all_mobs.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	const double total_ms = t_total.delta().count() * 1000.0;
	log("   [yaml-timing] mobs (threads=%zu): total=%.1f ms | discovery=%.1f | "
		"parallel-parse=%.1f | merge/post=%.1f",
		m_num_threads, total_ms, disc_ms, par_ms, total_ms - disc_ms - par_ms);
	log("Loaded %zu mobs from YAML.", all_mobs.size());
	return all_mobs;
}

// Parse single object file (thread-safe worker function)
CObjectPrototype* YamlWorldDataSource::ParseObjectFile(const std::string &file_path, int vnum)
{
	YAML::Node root = YAML::LoadFile(file_path);
	return ParseObjectNode(root, vnum);
}

// Parse a single object from an already-loaded YAML node. Shared by the
// per-file layout (vnum from filename) and the flat layout (vnum from the
// rel-number map key).
CObjectPrototype* YamlWorldDataSource::ParseObjectNode(const YAML::Node &root, int vnum)
{
	// NOTE: This returns raw pointer - caller must wrap in shared_ptr
	auto obj_ptr = new CObjectPrototype(vnum);
	
			// Object created above

			// Names
			YAML::Node names = root["names"];
			if (names)
			{
				obj_ptr->set_aliases(GetText(names, "aliases"));
				obj_ptr->set_short_description(utils::colorLOW(GetText(names, "nominative")));
				obj_ptr->set_PName(grammar::ECase::kNom, utils::colorLOW(GetText(names, "nominative")));
				obj_ptr->set_PName(grammar::ECase::kGen, utils::colorLOW(GetText(names, "genitive")));
				obj_ptr->set_PName(grammar::ECase::kDat, utils::colorLOW(GetText(names, "dative")));
				obj_ptr->set_PName(grammar::ECase::kAcc, utils::colorLOW(GetText(names, "accusative")));
				obj_ptr->set_PName(grammar::ECase::kIns, utils::colorLOW(GetText(names, "instrumental")));
				obj_ptr->set_PName(grammar::ECase::kPre, utils::colorLOW(GetText(names, "prepositional")));
			}

			obj_ptr->set_description(utils::colorCAP(GetText(root, "short_desc")));
			obj_ptr->set_action_description(GetText(root, "action_desc"));

			// Type
			int obj_type_id = ParseEnum(root["type"], "obj_types", 0);
			obj_ptr->set_type(static_cast<EObjType>(obj_type_id));

			// Material
			obj_ptr->set_material(static_cast<EObjMaterial>(GetInt(root, "material", 0)));

			// Values
			YAML::Node values = root["values"];
			if (values && values.IsSequence() && values.size() >= 4)
			{
			// Match Legacy: negative val[0] becomes 0
			long val0 = values[0].as<long>(0);
			if (val0 < 0)
			{
				val0 = 0;
			}
			obj_ptr->set_val(0, val0);
				obj_ptr->set_val(1, values[1].as<long>(0));
				obj_ptr->set_val(2, values[2].as<long>(0));
				obj_ptr->set_val(3, values[3].as<long>(0));
			}

			// Physical properties
			obj_ptr->set_weight(GetInt(root, "weight", 0));
			if (obj_ptr->get_type() == EObjType::kLiquidContainer || obj_ptr->get_type() == EObjType::kFountain)
			{
				if (obj_ptr->get_weight() < obj_ptr->get_val(1))
				{
					obj_ptr->set_weight(obj_ptr->get_val(1) + 5);
				}
			}
			obj_ptr->set_cost(GetInt(root, "cost", 0));
			obj_ptr->set_rent_off(GetInt(root, "rent_off", 0));
			obj_ptr->set_rent_on(GetInt(root, "rent_on", 0));
			obj_ptr->set_spec_param(GetInt(root, "spec_param", 0));

			int max_dur = GetInt(root, "max_durability", 100);
			int cur_dur = GetInt(root, "cur_durability", 100);
			obj_ptr->set_maximum_durability(max_dur);
			obj_ptr->set_current_durability(std::min(max_dur, cur_dur));

			int timer = GetInt(root, "timer", -1);
			if (timer <= 0) { timer = ObjData::SEVEN_DAYS; }
			if (timer > 99999) timer = 99999;
			obj_ptr->set_timer(timer);

			// issue #3581: obj->spell и его уровень (level) -- мёртвая пара, из мировой сериализации выпилены; ключи "spell"/"level" в старых yaml игнорируются.
			obj_ptr->set_sex(static_cast<EGender>(ParseGender(root["sex"])));

			if (root["max_in_world"])
			{
				obj_ptr->set_max_in_world(root["max_in_world"].as<int>());
			}
			else
			{
				obj_ptr->set_max_in_world(-1);
			}

			obj_ptr->set_minimum_remorts(GetInt(root, "minimum_remorts", 0));

			// Flags
			auto &dm = DictionaryManager::Instance();

			if (root["extra_flags"] && root["extra_flags"].IsSequence())
			{
				for (const auto &flag_node : root["extra_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("extra_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->set_extra_flag(static_cast<EObjFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_extra_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			if (root["wear_flags"] && root["wear_flags"].IsSequence())
			{
				int wear_flags = 0;
				for (const auto &flag_node : root["wear_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("wear_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						wear_flags |= (1 << flag_val);
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						if (bit >= 0 && bit < 32)
							wear_flags |= (1 << bit);
					}
				}
				obj_ptr->set_wear_flags(wear_flags);
			}

			if (root["no_flags"] && root["no_flags"].IsSequence())
			{
				for (const auto &flag_node : root["no_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("no_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->set_no_flag(static_cast<ENoFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_no_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			if (root["anti_flags"] && root["anti_flags"].IsSequence())
			{
				for (const auto &flag_node : root["anti_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("anti_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->set_anti_flag(static_cast<EAntiFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_anti_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			if (root["affect_flags"] && root["affect_flags"].IsSequence())
			{
				// affect_flags на предмете -- бит EWeaponAffect (хранится в
				// m_waffect_flags), не EAffect. Раньше lookup шёл через dm
				// "affect_flags" (EAffect-нумерация), и kDetectPoison из
				// YAML попадал в бит 32, который в EWeaponAffect равен
				// kDisguising -- носитель получал маскировку вместо
				// определения яда. ITEM_BY_NAME<EWeaponAffect>(name) даёт
				// корректный EWeaponAffect-бит без посредника.
				//
				// Старые YAML, конвертированные ранее через AFFECT_FLAGS-
				// таблицу, могут содержать имена, которых нет в
				// EWeaponAffect (kDetectInvisible вместо kDetectInvisibility).
				// ITEM_BY_NAME .at() бросает out_of_range -- ловим, пишем
				// syslog, продолжаем. Лучше потерять один сомнительный
				// флаг, чем уронить boot всего мира.
				for (const auto &flag_node : root["affect_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_affect_flag(plane, 1 << bit_in_plane);
						continue;
					}
					try
					{
						const auto wa = ITEM_BY_NAME<EWeaponAffect>(flag_name);
						obj_ptr->SetEWeaponAffectFlag(wa);
					}
					catch (const std::out_of_range &)
					{
						log("SYSERR: unknown EWeaponAffect '%s' on obj vnum %d, skipped",
							flag_name.c_str(), obj_ptr->get_vnum());
					}
				}
			}

			// Match Legacy: remove transformed and ticktimer flags after loading
			obj_ptr->unset_extraflag(EObjFlag::kTransformed);
			obj_ptr->unset_extraflag(EObjFlag::kTicktimer);

			// Match Legacy: override max_in_world for zonedecay/repop_decay objects
			if (obj_ptr->has_flag(EObjFlag::kZonedecay) || obj_ptr->has_flag(EObjFlag::kRepopDecay))
			{
				obj_ptr->set_max_in_world(-1);
			}

			// Applies
			int apply_idx = 0;
			if (root["applies"] && root["applies"].IsSequence())
			{
				for (const auto &apply_node : root["applies"])
				{
					if (apply_idx >= kMaxObjAffect) break;
					int location = GetInt(apply_node, "location", 0);
					int modifier = GetInt(apply_node, "modifier", 0);
					obj_ptr->set_affected(apply_idx++, static_cast<EApply>(location), modifier);
				}
			}

			// Skills granted by the object (legacy `S` lines). Mirrors the
			// boot_data_files.cpp `case 'S'` handler; without it the object's
			// skill bonuses are silently dropped in the YAML world (issue #3386).
			if (root["skills"] && root["skills"].IsSequence())
			{
				for (const auto &skill_node : root["skills"])
				{
					int skill_id = GetInt(skill_node, "skill_id", 0);
					int value = GetInt(skill_node, "value", 0);
					obj_ptr->set_skill(static_cast<ESkill>(skill_id), value);
				}
			}

			// Extra values (V-строки): данные зелий и контейнеров с жидкостью.
			// Без чтения этой секции зелья в YAML-мире выходят без заклинаний (issue #3218).
			if (root["extra_values"] && root["extra_values"].IsMap())
			{
				for (const auto &kv : root["extra_values"])
				{
					std::string key = kv.first.as<std::string>();
					int value = kv.second.as<int>(0);
					std::string line = key + " " + std::to_string(value);
					obj_ptr->init_values_from_zone(line.c_str());
				}
			}

			// Extra descriptions
			if (root["extra_descriptions"] && root["extra_descriptions"].IsSequence())
			{
				for (const auto &ed_node : root["extra_descriptions"])
				{
					std::string keywords = GetText(ed_node, "keywords");
					std::string description = GetText(ed_node, "description");

					ExtraDescription ex_desc;
					ex_desc.set_keyword(keywords);
					ex_desc.set_description(description);
					obj_ptr->ex_descriptions().push_back(std::move(ex_desc));
				}
			}

			// Add to proto first to get rnum

	return obj_ptr;
}


// Parallel object loading
// The only object-loading entry point: `zone_filter` null means every zone.
// Parses in parallel across zones/tasks and returns the results WITHOUT
// touching obj_proto or attaching triggers -- the caller (GameLoader::
// BootWorld's orchestrator in db.cpp) does both once, after possibly
// combining results from other sources too (composite).
std::vector<LoadedObject> YamlWorldDataSource::LoadObjects(const std::vector<int> *zone_filter)
{
	if (!LoadDictionaries())
	{
		fatal_log("FATAL: Cannot continue without dictionaries. Aborting.");
	}
	EnsureThreadPool();

	utils::CExecutionTimer t_total;
	std::vector<EntityFileTask> tasks = DiscoverEntityFiles("objects", zone_filter);
	if (tasks.empty())
	{
		log("No objects found in YAML index.");
		return {};
	}

	// Distribute object files into batches (flat task = many vnums, per-file = one)
	auto batches = utils::DistributeBatches(tasks, m_num_threads);

	// Thread-local results (each thread collects its objects and triggers)
	std::vector<std::vector<LoadedObject>> thread_results(batches.size());
	std::atomic<int> error_count{0};

	const double disc_ms = t_total.delta().count() * 1000.0;
	utils::CExecutionTimer t_par;

	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &thread_results, &error_count]() {
			for (const auto &task : batches[thread_id])
			{
				YAML::Node file_root;
				if (task.flat && task.parsed_root)
				{
					file_root = task.parsed_root;
				}
				else
				{
					try
					{
						file_root = YAML::LoadFile(task.path);
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to load objects from '%s': %s", task.path.c_str(), e.what());
						error_count += static_cast<int>(task.vnums.size());
						continue;
					}
				}
				for (int vnum : task.vnums)
				{
					const YAML::Node node = task.flat ? file_root[vnum % 100] : file_root;
					try
					{
						LoadedObject lo;
						lo.vnum = vnum;
						lo.obj = std::shared_ptr<CObjectPrototype>(ParseObjectNode(node, vnum));

						if (node["triggers"] && node["triggers"].IsSequence())
						{
							for (const auto &trig_node : node["triggers"])
							{
								lo.triggers.push_back(trig_node.as<int>());
							}
						}
						thread_results[thread_id].push_back(std::move(lo));
					}
					catch (const std::exception &e)
					{
						log("SYSERR: Failed to parse object %d from '%s': %s", vnum, task.path.c_str(), e.what());
						error_count++;
					}
				}
			}
		}));
	}

	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		fatal_log("FATAL: %d object(s) failed to load. Aborting.", error_count.load());
	}

	const double par_ms = t_par.delta().count() * 1000.0;

	std::vector<LoadedObject> all_objects;
	for (auto &results : thread_results)
	{
		all_objects.insert(all_objects.end(), std::make_move_iterator(results.begin()),
			std::make_move_iterator(results.end()));
	}
	std::sort(all_objects.begin(), all_objects.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	const double total_ms = t_total.delta().count() * 1000.0;
	log("   [yaml-timing] objects (threads=%zu): total=%.1f ms | discovery=%.1f | "
		"parallel-parse=%.1f | merge/post=%.1f",
		m_num_threads, total_ms, disc_ms, par_ms, total_ms - disc_ms - par_ms);
	log("Loaded %zu objects from YAML.", all_objects.size());
	return all_objects;
}

// Helper methods for save operations

std::string YamlWorldDataSource::ConvertToUtf8(const std::string &koi8r_str) const
{
	if (koi8r_str.empty())
	{
		return "";
	}

	static char buffer[65536];
	char *input = const_cast<char*>(koi8r_str.c_str());
	char *output = buffer;
	codepages::koi_to_utf8(input, output);
	return buffer;
}

template<class FlagsT>
std::vector<std::string> YamlWorldDataSource::ConvertFlagsToNames(const FlagsT &flags, const std::string &dict_name) const
{
	std::vector<std::string> names;
	auto &dm = DictionaryManager::Instance();
	const Dictionary *dict = dm.GetDictionary(dict_name);

	if (!dict)
	{
		return names;
	}

	const auto &entries = dict->GetEntries();

	for (size_t plane = 0; plane < FlagData::kPlanesNumber; ++plane)
	{
		Bitvector plane_bits = flags.get_plane(plane);
		if (plane_bits == 0) continue;

		for (int bit = 0; bit < 30; ++bit)
		{
			if (plane_bits & (1 << bit))
			{
				int bit_index = plane * 30 + bit;

				std::string flag_name;
				for (const auto &[name, value] : entries)
				{
					if (value == bit_index)
					{
						flag_name = name;
						break;
					}
				}

				if (!flag_name.empty())
				{
					names.push_back(flag_name);
				}
				else
				{
					names.push_back("UNUSED_" + std::to_string(bit_index));
				}
			}
		}
	}

	return names;
}

std::string YamlWorldDataSource::ReverseLookupEnum(const std::string &dict_name, int value) const
{
	auto &dm = DictionaryManager::Instance();
	const Dictionary *dict = dm.GetDictionary(dict_name);

	if (!dict)
	{
		return std::to_string(value);
	}

	const auto &entries = dict->GetEntries();
	for (const auto &[name, dict_value] : entries)
	{
		if (dict_value == value)
		{
			return name;
		}
	}

	return std::to_string(value);
}

bool YamlWorldDataSource::WriteYamlAtomic(const std::string &filepath, const YAML::Node &node) const
{
	std::string temp_filepath = filepath + ".tmp";

	try
	{
		YAML::Emitter emitter;
		emitter << node;

		std::ofstream out(temp_filepath);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open temp file for writing: %s", temp_filepath.c_str());
			return false;
		}
		out << emitter.c_str();
		out.close();

		std::rename(temp_filepath.c_str(), filepath.c_str());
		return true;
	}
	catch (const std::exception &e)
	{
		log("SYSERR: Failed to write YAML file %s: %s", filepath.c_str(), e.what());

		return false;
	}
}

bool YamlWorldDataSource::WriteIndexYaml(const std::string &filepath,
										 const std::string &top_key,
										 const std::vector<int> &values) const
{
	namespace fs = std::filesystem;
	try
	{
		fs::create_directories(fs::path(filepath).parent_path());

		std::string temp_filepath = filepath + ".tmp";
		std::ofstream out(temp_filepath);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open temp file for index: %s", temp_filepath.c_str());
			return false;
		}
		// Match the layout the Python converter emits and the loader expects:
		//   <top_key>:
		//   - <v>
		// Or for an empty list, "<top_key>: []".
		if (values.empty())
		{
			out << top_key << ": []\n";
		}
		else
		{
			out << top_key << ":\n";
			for (int v : values)
			{
				out << "- " << v << "\n";
			}
		}
		out.close();
		std::rename(temp_filepath.c_str(), filepath.c_str());
		return true;
	}
	catch (const std::exception &e)
	{
		log("SYSERR: Failed to write index %s: %s", filepath.c_str(), e.what());
		return false;
	}
}

bool YamlWorldDataSource::RebuildPerZoneIndex(int zone_vnum, const std::string &sub) const
{
	// Collect rel-numbers (vnum % 100) of all in-memory prototypes that belong
	// to this zone. The set guarantees uniqueness + sorted order.
	std::set<int> rel_nums;

	if (sub == "mobs")
	{
		for (MobRnum rnum = 0; rnum < top_of_mobt; ++rnum)
		{
			const int v = mob_index[rnum].vnum;
			if (v / 100 == zone_vnum) rel_nums.insert(v % 100);
		}
	}
	else if (sub == "objects")
	{
		for (const auto &[v, rnum] : obj_proto.vnum2index())
		{
			if (v / 100 == zone_vnum) rel_nums.insert(v % 100);
		}
	}
	else if (sub == "rooms")
	{
		for (RoomRnum rnum = 0; rnum <= top_of_world; ++rnum)
		{
			if (!world[rnum]) continue;
			const int v = world[rnum]->vnum;
			if (v / 100 != zone_vnum) continue;
			// rel == 99 is reserved for the runtime-only "virtual room"
			// (AddVirtualRoomsToAllZones / "неизвестная комната"). SaveRooms
			// doesn't emit its file, so we mustn't list it in the index either.
			if (v % 100 == 99) continue;
			rel_nums.insert(v % 100);
		}
	}
	else if (sub == "triggers")
	{
		for (int rnum = 0; rnum < top_of_trigt; ++rnum)
		{
			if (!trig_index[rnum]) continue;
			const int v = trig_index[rnum]->vnum;
			if (v / 100 == zone_vnum) rel_nums.insert(v % 100);
		}
	}
	else
	{
		log("SYSERR: RebuildPerZoneIndex called with unknown sub '%s'", sub.c_str());
		return false;
	}

	const std::string filepath = m_world_dir + "/zones/" + std::to_string(zone_vnum)
		+ "/" + sub + "/index.yaml";
	return WriteIndexYaml(filepath, sub, {rel_nums.begin(), rel_nums.end()});
}
// ============================================================================

void YamlWorldDataSource::SaveZone(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveZone", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	std::string zone_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum);
	std::string zone_file = zone_dir + "/zone.yaml";
	std::string temp_file = zone_file + ".tmp";

	namespace fs = std::filesystem;
	if (!fs::exists(zone_dir))
	{
		fs::create_directories(zone_dir);
	}

	std::ofstream out(temp_file);
	if (!out.is_open())
	{
		log("SYSERR: Failed to open temp file for writing: %s", temp_file.c_str());
		return;
	}

	Koi8rYamlEmitter yaml(out);

	// Header comment
	yaml.Comment("Zone #" + std::to_string(zone.vnum));
	yaml.EmptyLine();

	yaml.Key("vnum");
	yaml.Value(zone.vnum);

	yaml.Key("name");
	yaml.Value(zone.name);

	// Metadata block (optional)
	if (!zone.comment.empty() || !zone.location.empty() || !zone.author.empty() || !zone.description.empty())
	{
		yaml.Key("metadata");
		yaml.BeginBlock();
		if (!zone.comment.empty())
		{
			yaml.Key("comment");
			yaml.Value(zone.comment);
		}
		if (!zone.location.empty())
		{
			yaml.Key("location");
			yaml.Value(zone.location);
		}
		if (!zone.author.empty())
		{
			yaml.Key("author");
			yaml.Value(zone.author);
		}
		if (!zone.description.empty())
		{
			yaml.Key("description");
			yaml.Value(zone.description, true);
		}
		yaml.EndBlock();
	}

	yaml.Key("top_room");
	yaml.Value(zone.top);

	yaml.Key("lifespan");
	yaml.Value(zone.lifespan);

	yaml.Key("reset_mode");
	yaml.Value(zone.reset_mode);

	yaml.Key("reset_idle");
	yaml.Value(zone.reset_idle ? 1 : 0);

	{
		std::string zone_type_name = GetZoneTypeName(zone.type);
		yaml.Key("zone_type");
		yaml.Value(zone.type, zone_type_name);
	}

	yaml.Key("mode");
	yaml.Value(zone.level, "level");

	if (zone.entrance != 0)
	{
		yaml.Key("entrance");
		yaml.Value(zone.entrance);
	}

	yaml.Key("under_construction");
	yaml.Value(zone.under_construction);

	if (zone.group > 0)
	{
		std::string group_comment = (zone.group <= 1) ? "solo" : ("group:" + std::to_string(zone.group));
		yaml.Key("zone_group");
		yaml.Value(zone.group, group_comment);
	}

	if (zone.typeA_count > 0)
	{
		yaml.Key("typeA_zones");
		yaml.BeginSequence();
		yaml.IncreaseIndent();
		for (int i = 0; i < zone.typeA_count; ++i)
		{
			yaml.SequenceItem(zone.typeA_list[i]);
		}
		yaml.DecreaseIndent();
	}

	if (zone.typeB_count > 0)
	{
		yaml.Key("typeB_zones");
		yaml.BeginSequence();
		yaml.IncreaseIndent();
		for (int i = 0; i < zone.typeB_count; ++i)
		{
			yaml.SequenceItem(zone.typeB_list[i]);
		}
		yaml.DecreaseIndent();
	}

	if (zone.cmd && zone.cmd[0].command != 'S')
	{
		yaml.Key("commands");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		// ResolveZoneCmdVnumArgsToRnums (db.cpp:1626) rewrites cmd.argN from
		// the on-disk vnums into in-memory rnums during boot. To round-trip
		// the zonefile we must invert that conversion here.
		auto mob_v = [](int rnum) -> int {
			if (rnum < 0 || rnum >= top_of_mobt) return rnum;
			return mob_index[rnum].vnum;
		};
		auto obj_v = [](int rnum) -> int {
			if (rnum < 0) return rnum;
			auto obj = obj_proto[rnum];
			return obj ? obj->get_vnum() : rnum;
		};
		auto room_v = [](int rnum) -> int {
			if (rnum < 0 || rnum > top_of_world || !world[rnum]) return rnum;
			return world[rnum]->vnum;
		};
		auto trig_v = [](int rnum) -> int {
			if (rnum < 0 || rnum >= top_of_trigt || !trig_index[rnum]) return rnum;
			return trig_index[rnum]->vnum;
		};

		for (int i = 0; zone.cmd[i].command != 'S'; ++i)
		{
			const struct reset_com &cmd = zone.cmd[i];
			std::ostringstream oss;
			std::string comment;

			switch (cmd.command)
			{
			case 'M':
			{
				int mob_vnum = mob_v(cmd.arg1);
				int room_vnum = room_v(cmd.arg3);
				oss << "MOB " << cmd.if_flag << " " << mob_vnum << " " << cmd.arg2 << " " << room_vnum;
				if (cmd.arg4 != -1) oss << " " << cmd.arg4;
				std::string mob_name = GetMobNameComment(mob_vnum);
				std::string room_name = GetRoomNameByVnum(room_vnum);
				std::ostringstream coss;
				if (!mob_name.empty()) coss << mob_name;
				if (!room_name.empty())
				{
					if (!coss.str().empty()) coss << " -> ";
					coss << room_name;
				}
				if (cmd.arg4 != -1)
				{
					if (!coss.str().empty()) coss << "; ";
					coss << "mr:" << cmd.arg4 << " mw:" << cmd.arg2;
				}
				comment = coss.str();
				break;
			}
			case 'O':
			{
				int obj_vnum = obj_v(cmd.arg1);
				// arg3 may legitimately be kNowhere (== -1 in some builds);
				// in that case ResolveZone... leaves it alone, so do the same.
				int room_vnum = (cmd.arg3 == kNowhere) ? cmd.arg3 : room_v(cmd.arg3);
				oss << "OBJECT " << cmd.if_flag << " " << obj_vnum << " " << cmd.arg2 << " " << room_vnum;
				if (cmd.arg4 != -1) oss << " " << cmd.arg4;
				std::string obj_name = GetObjNameComment(obj_vnum);
				std::string room_name = GetRoomNameByVnum(room_vnum);
				std::ostringstream coss;
				if (!obj_name.empty()) coss << obj_name;
				if (!room_name.empty())
				{
					if (!coss.str().empty()) coss << " -> ";
					coss << room_name;
				}
				comment = coss.str();
				break;
			}
			case 'G':
			{
				int obj_vnum = obj_v(cmd.arg1);
				oss << "GIVE " << cmd.if_flag << " " << obj_vnum << " " << cmd.arg2;
				if (cmd.arg4 != -1) oss << " " << cmd.arg4;
				comment = GetObjNameComment(obj_vnum);
				break;
			}
			case 'E':
			{
				int obj_vnum = obj_v(cmd.arg1);
				oss << "EQUIP " << cmd.if_flag << " " << obj_vnum << " " << cmd.arg2 << " " << cmd.arg3;
				if (cmd.arg4 != -1) oss << " " << cmd.arg4;
				std::string obj_name = GetObjNameComment(obj_vnum);
				std::ostringstream coss;
				if (!obj_name.empty()) coss << obj_name;
				coss << ", wear: " << cmd.arg3;
				comment = coss.str();
				break;
			}
			case 'P':
			{
				int obj_vnum = obj_v(cmd.arg1);
				int container_vnum = obj_v(cmd.arg3);
				oss << "PUT " << cmd.if_flag << " " << obj_vnum << " " << cmd.arg2 << " " << container_vnum;
				if (cmd.arg4 != -1) oss << " " << cmd.arg4;
				std::string obj_name = GetObjNameComment(obj_vnum);
				std::string cont_name = GetObjNameComment(container_vnum);
				std::ostringstream coss;
				if (!obj_name.empty()) coss << obj_name;
				if (!cont_name.empty())
				{
					if (!coss.str().empty()) coss << " -> ";
					coss << cont_name;
				}
				comment = coss.str();
				break;
			}
			case 'D':
			{
				int room_vnum = room_v(cmd.arg1);
				oss << "DOOR " << cmd.if_flag << " " << room_vnum << " " << cmd.arg2 << " " << cmd.arg3;
				std::string room_name = GetRoomNameByVnum(room_vnum);
				std::ostringstream coss;
				if (!room_name.empty()) coss << room_name;
				coss << ", dir: " << cmd.arg2;
				comment = coss.str();
				break;
			}
			case 'R':
			{
				int room_vnum = room_v(cmd.arg1);
				int obj_vnum = obj_v(cmd.arg2);
				oss << "REMOVE " << cmd.if_flag << " " << room_vnum << " " << obj_vnum;
				comment = GetRoomNameByVnum(room_vnum);
				break;
			}
			case 'T':
			{
				int trig_vnum = trig_v(cmd.arg2);
				// arg1 is the trigger-class enum (MOB/OBJ/WLD), not an rnum.
				// arg3 is a room rnum only when arg1 == WLD_TRIGGER.
				int third = cmd.arg3;
				if (cmd.arg1 == WLD_TRIGGER && cmd.arg3 != -1)
				{
					third = room_v(cmd.arg3);
				}
				oss << "TRIGGER " << cmd.if_flag << " " << cmd.arg1 << " " << trig_vnum;
				if (cmd.arg3 != -1) oss << " " << third;
				comment = GetTriggerNameComment(trig_vnum);
				break;
			}
			case 'V':
			{
				int second = cmd.arg2;
				if (cmd.arg1 == WLD_TRIGGER) second = room_v(cmd.arg2);
				oss << "VAR " << cmd.if_flag << " " << cmd.arg1 << " " << second << " " << cmd.arg3;
				if (cmd.sarg1) oss << " " << cmd.sarg1;
				if (cmd.sarg2) oss << " " << cmd.sarg2;
				if (cmd.sarg1 && cmd.sarg2)
				{
					comment = std::string(cmd.sarg1) + " = " + cmd.sarg2;
				}
				break;
			}
			case 'Q':
			{
				int mob_vnum = mob_v(cmd.arg1);
				oss << "EXTRACT 0 " << mob_vnum;
				comment = GetMobNameComment(mob_vnum);
				break;
			}
			case 'F':
			{
				int room_vnum = room_v(cmd.arg1);
				int leader_vnum = mob_v(cmd.arg2);
				int follower_vnum = mob_v(cmd.arg3);
				oss << "FOLLOW " << cmd.if_flag << " " << room_vnum << " " << leader_vnum << " " << follower_vnum;
				std::string leader_name = GetMobNameComment(leader_vnum);
				std::string follower_name = GetMobNameComment(follower_vnum);
				std::string room_name = GetRoomNameByVnum(room_vnum);
				std::ostringstream coss;
				if (!leader_name.empty()) coss << leader_name;
				if (!follower_name.empty())
				{
					if (!coss.str().empty()) coss << " -> ";
					coss << follower_name;
				}
				if (!room_name.empty())
				{
					if (!coss.str().empty()) coss << ", in ";
					coss << room_name;
				}
				comment = coss.str();
				break;
			}
			default:
				continue;
			}

			yaml.SequenceItem(oss.str(), comment);
		}

		yaml.DecreaseIndent();
	}

	out.close();
	std::rename(temp_file.c_str(), zone_file.c_str());

	log("Saved zone %d to YAML file", zone.vnum);

	// Top-level zones/index.yaml snapshot. Skip dungeon zones (vnum >=
	// kZoneStartDungeons) -- they are generated in-memory by
	// CreateBlankZoneDungeon and never persisted, matching the legacy
	// medit_save_to_disk guard at olc/medit.cpp:532.
	std::vector<int> zone_vnums;
	zone_vnums.reserve(zone_table.size());
	for (const auto &z : zone_table)
	{
		if (z.vnum < dungeons::kZoneStartDungeons)
		{
			zone_vnums.push_back(z.vnum);
		}
	}
	std::sort(zone_vnums.begin(), zone_vnums.end());
	WriteIndexYaml(m_world_dir + "/zones/index.yaml", "zones", zone_vnums);
}

// Remove the artifacts of the layout we did NOT just write, so a save migrates
// a zone's sub-type fully between layouts with no leftovers.
void YamlWorldDataSource::CleanupOtherLayout(int zone_vnum, const std::string &sub, YamlLayout written) const
{
	namespace fs = std::filesystem;
	const std::string base = m_world_dir + "/zones/" + std::to_string(zone_vnum);
	std::error_code ec;
	if (written == YamlLayout::Flat)
	{
		// We wrote <sub>.yaml -- drop the stale per-file directory (and index).
		fs::remove_all(base + "/" + sub, ec);
	}
	else
	{
		// We wrote the <sub>/ directory -- drop the stale flat file.
		fs::remove(base + "/" + sub + ".yaml", ec);
	}
}

void YamlWorldDataSource::EmitTriggerBody(Koi8rYamlEmitter &yaml, Trigger *trig)
{
	// Name
	yaml.Key("name");
	yaml.Value(GET_TRIG_NAME(trig));

	// Attach type
	yaml.Key("attach_type");
	yaml.Value(ReverseLookupEnum("attach_types", trig->get_attach_type()));

	if (trig->get_script_language() == TriggerScriptLanguage::Lua)
	{
		yaml.Key("language");
		yaml.Value("lua");
	}

	// Narg
	yaml.Key("narg");
	yaml.Value(GET_TRIG_NARG(trig));

	// Arglist (optional)
	if (!trig->arglist.empty())
	{
		yaml.Key("arglist");
		yaml.Value(trig->arglist);
	}

	// Trigger types
	bool has_trigger_types = false;
	for (int bit = 0; bit < 32; ++bit)
	{
		if (GET_TRIG_TYPE(trig) & (1L << bit))
		{
			has_trigger_types = true;
			break;
		}
	}

	if (has_trigger_types)
	{
		yaml.Key("trigger_types");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (int bit = 0; bit < 32; ++bit)
		{
			if (GET_TRIG_TYPE(trig) & (1L << bit))
			{
				std::string type_name = ReverseLookupEnum("trigger_types", bit);
				if (!type_name.empty() && type_name != std::to_string(bit))
				{
					yaml.SequenceItem(type_name);
				}
			}
		}

		yaml.DecreaseIndent();
	}

	if (trig->get_script_language() == TriggerScriptLanguage::Lua)
	{
		const auto &script = trig->get_lua_script_source();
		if (!script.empty())
		{
			yaml.Key("script");
			yaml.Value(script, true);  // literal=true
		}
		return;
	}

	// Script (multiline literal block). ParseTriggerScript keeps
	// whitespace-only lines from the source as cmd_list nodes whose
	// cmd is "" after TrimRight; we must round-trip the node count or
	// the world checksum shifts. The original whitespace text is no
	// longer available, so we serialise empty cmds as a single space:
	// on reload, ParseTriggerScript sees a non-empty line, TrimRights
	// it back to "", and the node count is preserved.
	std::string script;
	for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next)
	{
		script += cmd->cmd.empty() ? std::string(" ") : cmd->cmd;
		if (cmd->next)
		{
			script += "\n";
		}
	}

	if (!script.empty())
	{
		yaml.Key("script");
		yaml.Value(script, true);  // literal=true
	}
}

bool YamlWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	(void)notify_level; // YAML saves don't use mudlog - errors go to log file
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveTriggers", zone_rnum);
		return false;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	// Collect this zone's triggers, sorted by vnum.
	std::vector<std::pair<int, Trigger *>> entries;  // (vnum, proto)
	for (TrgRnum trig_rnum = 0; trig_rnum < top_of_trigt; ++trig_rnum)
	{
		if (!trig_index[trig_rnum]) continue;
		int trig_vnum = trig_index[trig_rnum]->vnum;
		if (trig_vnum < zone.vnum * 100 || trig_vnum > zone.top) continue;
		Trigger *trig = trig_index[trig_rnum]->proto;
		if (!trig) continue;
		entries.emplace_back(trig_vnum, trig);
	}
	std::sort(entries.begin(), entries.end(),
		[](const auto &a, const auto &b) { return a.first < b.first; });

	namespace fs = std::filesystem;

	if (m_save_layout == YamlLayout::Flat)
	{
		// Flat layout ignores specific_vnum: the whole zone file is rewritten
		// from the in-memory prototypes (which already reflect the edit).
		const std::string flat_path = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/triggers.yaml";
		const std::string temp_file = flat_path + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			return false;
		}
		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Triggers for zone " + std::to_string(zone.vnum));
		for (const auto &[trig_vnum, trig] : entries)
		{
			yaml.EmptyLine();
			yaml.Comment("Trigger #" + std::to_string(trig_vnum));
			out << yaml.GetIndent() << (trig_vnum % 100) << ":" << std::endl;
			yaml.IncreaseIndent();
			EmitTriggerBody(yaml, trig);
			yaml.DecreaseIndent();
		}
		out.close();
		if (std::rename(temp_file.c_str(), flat_path.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), flat_path.c_str());
			return false;
		}
		log("Saved %zu triggers (flat) for zone %d", entries.size(), zone.vnum);
		CleanupOtherLayout(zone.vnum, "triggers", YamlLayout::Flat);
		return true;
	}

	// Per-file layout: one file per trigger.
	int saved_count = 0;
	for (const auto &[trig_vnum, trig] : entries)
	{
		if (specific_vnum != -1 && trig_vnum != specific_vnum) continue;

		int rel_num = trig_vnum % 100;
		const std::string trig_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/triggers";
		if (!fs::exists(trig_dir))
		{
			fs::create_directories(trig_dir);
		}
		std::ostringstream trig_file_ss;
		trig_file_ss << trig_dir << "/" << std::setfill('0') << std::setw(2) << rel_num << ".yaml";
		const std::string trig_file = trig_file_ss.str();
		const std::string temp_file = trig_file + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			continue;
		}

		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Trigger #" + std::to_string(trig_vnum));
		yaml.EmptyLine();
		EmitTriggerBody(yaml, trig);

		out.close();
		if (std::rename(temp_file.c_str(), trig_file.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), trig_file.c_str());
			continue;
		}
		++saved_count;
	}

	log("Saved %d triggers for zone %d", saved_count, zone.vnum);
	RebuildPerZoneIndex(zone.vnum, "triggers");
	CleanupOtherLayout(zone.vnum, "triggers", YamlLayout::PerFile);
	return true;
}

void YamlWorldDataSource::EmitRoomBody(Koi8rYamlEmitter &yaml, std::ostream &out, RoomData *room)
{
	// Name
	if (room->name)
	{
		yaml.Key("name");
		yaml.Value(room->name);
	}

	// Description
	std::string desc = GlobalObjects::descriptions().get(room->description_num);
	if (!desc.empty())
	{
		yaml.Key("description");
		yaml.Value(desc, true);  // literal=true
	}

	// Sector
	yaml.Key("sector");
	yaml.Value(ReverseLookupEnum("sectors", static_cast<int>(room->sector_type)));

	// Flags
	FlagData room_flags = room->read_flags();
	auto flag_names = ConvertFlagsToNames(room_flags, "room_flags");
	if (!flag_names.empty())
	{
		yaml.Key("flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : flag_names)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Exits (with to_room comments)
	bool has_exits = false;
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
	{
		if (room->dir_option_proto[dir])
		{
			has_exits = true;
			break;
		}
	}

	if (has_exits)
	{
		yaml.Key("exits");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
		{
			if (!room->dir_option_proto[dir])
			{
				continue;
			}

			out << yaml.GetIndent() << "- direction: ";
			out << ReverseLookupEnum("directions", dir) << std::endl;

			// to_room (with comment)
			RoomRnum to_rnum = kNowhere;
			int to_vnum = -1;
			if (room->dir_option_proto[dir]->to_room() != kNowhere)
			{
				to_rnum = room->dir_option_proto[dir]->to_room();
				if (to_rnum >= 0 && to_rnum <= top_of_world && world[to_rnum])
				{
					to_vnum = world[to_rnum]->vnum;
				}
			}

			out << yaml.GetIndent() << "  to_room: " << to_vnum;
			if (to_vnum != -1 && to_rnum != kNowhere)
			{
				std::string room_name = GetRoomNameComment(to_rnum);
				if (!room_name.empty())
				{
					out << "  # " << room_name;
				}
			}
			out << std::endl;

			// Description (optional). Delegate to Koi8rYamlEmitter::Value
			// for proper clip-vs-strip chomping based on trailing newline
			// in the source string (otherwise round-trip silently appends
			// a CR/LF that wasn't in memory).
			if (!room->dir_option_proto[dir]->general_description.empty())
			{
				out << yaml.GetIndent() << "  description:";
				yaml.IncreaseIndent();
				yaml.Value(room->dir_option_proto[dir]->general_description, true);
				yaml.DecreaseIndent();
			}

			// Keywords (optional). ExitData splits the load value on '|':
			// keyword = nominative form, vkeyword = accusative. Recombine
			// them on save with the same delimiter so the next load sees
			// both forms (otherwise the accusative form is silently lost
			// and "открыть решетку" no longer matches the door).
			if (room->dir_option_proto[dir]->keyword)
			{
				std::string kws = room->dir_option_proto[dir]->keyword;
				const char *vk = room->dir_option_proto[dir]->vkeyword;
				if (vk && *vk && kws != vk)
				{
					kws += "|";
					kws += vk;
				}
				out << yaml.GetIndent() << "  keywords:";
				// IncreaseIndent so yaml.Value's literal-block branch emits
				// content lines at parent_indent + 2 -- the indicator "2"
				// matches actual column position. Without this content sits
				// one indent shy of where "|+2"/"|2" promises it lives.
				yaml.IncreaseIndent();
				yaml.Value(kws);
				yaml.DecreaseIndent();
			}

			// Exit flags (optional)
			if (room->dir_option_proto[dir]->exit_info != 0)
			{
				out << yaml.GetIndent() << "  exit_flags: ";
				out << static_cast<int>(room->dir_option_proto[dir]->exit_info) << std::endl;
			}

			// Key (optional)
			if (room->dir_option_proto[dir]->key != -1)
			{
				out << yaml.GetIndent() << "  key: ";
				out << room->dir_option_proto[dir]->key << std::endl;
			}

			// Lock complexity (optional)
			if (room->dir_option_proto[dir]->lock_complexity != 0)
			{
				out << yaml.GetIndent() << "  lock_complexity: ";
				out << static_cast<int>(room->dir_option_proto[dir]->lock_complexity) << std::endl;
			}
		}

		yaml.DecreaseIndent();
	}

	// Extra descriptions.
	// issue #3596: экстра-дескриптор без описания (0 символов/одни пробелы, напр.
	// после очистки в OLC через /c) считаем несуществующим -- не пишем; если
	// валидных дескрипторов не осталось, секцию опускаем.
	std::vector<const ExtraDescription *> exdescs;
	for (const auto &exdesc : room->ex_description)
	{
		if (!exdesc.keyword.empty()
			&& exdesc.description.find_first_not_of(" \t\r\n") != std::string::npos)
		{
			exdescs.push_back(&exdesc);
		}
	}
	if (!exdescs.empty())
	{
		yaml.Key("extra_descriptions");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		// Вектор хранит описания в порядке файла (load делает push_back), пишем вперёд.
		for (const auto *exdesc : exdescs)
		{
			// keywords may start with '-' (legitimately a single dash);
			// Koi8rYamlEmitter::Value handles leading-indicator quoting.
			out << yaml.GetIndent() << "- keywords:";
			// IncreaseIndent so yaml.Value's literal-block branch emits content
			// lines at the correct column for indicator "2" (parent_indent + 2).
			yaml.IncreaseIndent();
			yaml.Value(exdesc->keyword);
			yaml.DecreaseIndent();
			out << yaml.GetIndent() << "  description:";
			yaml.IncreaseIndent();
			yaml.Value(exdesc->description, true);
			yaml.DecreaseIndent();
		}

		yaml.DecreaseIndent();
	}

	// Triggers (with comments)
	if (room->proto_script && !room->proto_script->empty())
	{
		yaml.Key("triggers");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (auto trig_vnum : *room->proto_script)
		{
			std::string trig_comment = GetTriggerNameComment(trig_vnum);
			yaml.SequenceItem(trig_vnum, trig_comment);
		}

		yaml.DecreaseIndent();
	}
}

void YamlWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveRooms", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	// Collect this zone's rooms, sorted by vnum. The cached RnumRoomsLocation
	// range misses rooms added via OLC/Admin API, so scan the whole world.
	// rel == 99 is the runtime-only virtual room (AddVirtualRoomsToAllZones);
	// it is never persisted (matches RebuildPerZoneIndex), so skip it -- in the
	// flat layout writing it would resurrect a phantom room on reload.
	std::vector<std::pair<int, RoomData *>> entries;
	for (RoomRnum room_rnum = 0; room_rnum <= top_of_world; ++room_rnum)
	{
		RoomData *room = world[room_rnum];
		if (!room || room->vnum < zone.vnum * 100 || room->vnum > zone.top) continue;
		if (room->vnum % 100 == 99) continue;
		entries.emplace_back(room->vnum, room);
	}
	std::sort(entries.begin(), entries.end(),
		[](const auto &a, const auto &b) { return a.first < b.first; });

	namespace fs = std::filesystem;

	if (m_save_layout == YamlLayout::Flat)
	{
		const std::string flat_path = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/rooms.yaml";
		const std::string temp_file = flat_path + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			return;
		}
		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Rooms for zone " + std::to_string(zone.vnum));
		for (const auto &[vnum, room] : entries)
		{
			yaml.EmptyLine();
			yaml.Comment("Room #" + std::to_string(vnum));
			out << yaml.GetIndent() << (vnum % 100) << ":" << std::endl;
			yaml.IncreaseIndent();
			EmitRoomBody(yaml, out, room);
			yaml.DecreaseIndent();
		}
		out.close();
		if (std::rename(temp_file.c_str(), flat_path.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), flat_path.c_str());
			return;
		}
		log("Saved %zu rooms (flat) for zone %d", entries.size(), zone.vnum);
		CleanupOtherLayout(zone.vnum, "rooms", YamlLayout::Flat);
		return;
	}

	// Per-file layout: one file per room.
	const std::string rooms_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/rooms";
	if (!fs::exists(rooms_dir))
	{
		fs::create_directories(rooms_dir);
	}
	int saved_count = 0;
	for (const auto &[vnum, room] : entries)
	{
		if (specific_vnum != -1 && vnum != specific_vnum) continue;

		int rel_num = vnum % 100;
		std::string room_file = rooms_dir + "/" + fmt::format("{:02d}", rel_num) + ".yaml";
		std::string temp_file = room_file + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			continue;
		}

		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Room #" + std::to_string(vnum));
		yaml.EmptyLine();
		EmitRoomBody(yaml, out, room);

		out.close();
		if (std::rename(temp_file.c_str(), room_file.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), room_file.c_str());
			continue;
		}
		++saved_count;
	}

	log("Saved %d rooms for zone %d", saved_count, zone.vnum);
	RebuildPerZoneIndex(zone.vnum, "rooms");
	CleanupOtherLayout(zone.vnum, "rooms", YamlLayout::PerFile);
}

void YamlWorldDataSource::EmitMobBody(Koi8rYamlEmitter &yaml, std::ostream &out, CharData &mob)
{
	// Names
	yaml.Key("names");
	out << std::endl;
	yaml.IncreaseIndent();

	// GetCharAliases() returns the full keyword list (name_), matching
	// what LoadMobs reads via SetCharAliases(GetText(names, "aliases"))
	// at line 1557. get_npc_name() returns short_descr_ (== nominative),
	// which would truncate "костяк скелет" -> "скелет" on round-trip.
	const std::string &aliases = mob.GetCharAliases();
	if (!aliases.empty())
	{
		yaml.Key("aliases");
		yaml.Value(aliases);
	}
	yaml.Key("nominative");
	yaml.Value(mob.player_data.PNames[grammar::ECase::kNom]);
	yaml.Key("genitive");
	yaml.Value(mob.player_data.PNames[grammar::ECase::kGen]);
	yaml.Key("dative");
	yaml.Value(mob.player_data.PNames[grammar::ECase::kDat]);
	yaml.Key("accusative");
	yaml.Value(mob.player_data.PNames[grammar::ECase::kAcc]);
	yaml.Key("instrumental");
	yaml.Value(mob.player_data.PNames[grammar::ECase::kIns]);
	yaml.Key("prepositional");
	yaml.Value(mob.player_data.PNames[grammar::ECase::kPre]);

	yaml.DecreaseIndent();

	// Descriptions
	yaml.Key("descriptions");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("short_desc");
	yaml.Value(mob.player_data.long_descr, true);  // literal=true

	yaml.Key("long_desc");
	yaml.Value(mob.player_data.description, true);  // literal=true

	yaml.DecreaseIndent();

	// Alignment
	yaml.Key("alignment");
	yaml.Value(alignment::GetAlignment(&mob));

	// Stats
	yaml.Key("stats");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("level");
	yaml.Value(mob.GetLevel());

	// Write in legacy file units to stay symmetric with LoadMob() and
	// matching convert_to_yaml.py output -- otherwise yaml -> legacy
	// -> yaml round-trip flips the on-disk hitroll value every pass.
	yaml.Key("hitroll_penalty");
	yaml.Value(20 - GET_HR(&mob));

	yaml.Key("armor");
	yaml.Value(GET_AC(&mob) / 10);

	// HP
	yaml.Key("hp");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("dice_count");
	yaml.Value(static_cast<int>(mob.mem_queue.total));  // byte -> int

	yaml.Key("dice_size");
	yaml.Value(static_cast<int>(mob.mem_queue.stored));  // byte -> int

	yaml.Key("bonus");
	yaml.Value(mob.get_hit());

	yaml.DecreaseIndent();

	// Damage
	yaml.Key("damage");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("dice_count");
	yaml.Value(static_cast<int>(mob.mob_specials.damnodice));  // byte -> int

	yaml.Key("dice_size");
	yaml.Value(static_cast<int>(mob.mob_specials.damsizedice));  // byte -> int

	yaml.Key("bonus");
	yaml.Value(mob.real_abils.damroll);

	yaml.DecreaseIndent();
	yaml.DecreaseIndent();  // stats

	// Gold
	yaml.Key("gold");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("dice_count");
	yaml.Value(static_cast<int>(mob.mob_specials.GoldNoDs));  // byte -> int

	yaml.Key("dice_size");
	yaml.Value(static_cast<int>(mob.mob_specials.GoldSiDs));  // byte -> int

	yaml.Key("bonus");
	yaml.Value(currencies::GetHand(mob, currencies::kGold));

	yaml.DecreaseIndent();

	// Experience
	yaml.Key("experience");
	yaml.Value(mob.get_exp());

	// Position
	yaml.Key("position");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("default");
	yaml.Value(ReverseLookupEnum("positions", static_cast<int>(mob.mob_specials.default_pos)));

	yaml.Key("start");
	yaml.Value(ReverseLookupEnum("positions", static_cast<int>(mob.GetPosition())));

	yaml.DecreaseIndent();

	// Sex
	yaml.Key("sex");
	yaml.Value(ReverseLookupEnum("genders", static_cast<int>(mob.get_sex())));

	// Size, height, weight
	yaml.Key("size");
	yaml.Value(GET_SIZE(&mob));

	// NPC race, between `size` and `height` as the converter writes it.
	yaml.Key("race");
	yaml.Value(static_cast<int>(mob.player_data.Race));

	yaml.Key("height");
	yaml.Value(static_cast<int>(GET_HEIGHT(&mob)));  // ubyte -> int

	yaml.Key("weight");
	yaml.Value(static_cast<int>(GET_WEIGHT(&mob)));  // ubyte -> int

	// Attributes (skip if all six are at default 11). LoadMobs sets every
	// attribute to 11 unconditionally and then overrides from `attributes`
	// if present; treating "all 11" as "not specified" matches the Python
	// converter, which only emits this block when legacy data actually
	// carried per-attribute values.
	const bool attributes_set =
		mob.get_str() != 11 || mob.get_dex() != 11 || mob.get_int() != 11 ||
		mob.get_wis() != 11 || mob.get_con() != 11 || mob.get_cha() != 11;
	if (attributes_set)
	{
		yaml.Key("attributes");
		out << std::endl;
		yaml.IncreaseIndent();

		yaml.Key("strength");
		yaml.Value(mob.get_str());

		yaml.Key("dexterity");
		yaml.Value(mob.get_dex());

		yaml.Key("intelligence");
		yaml.Value(mob.get_int());

		yaml.Key("wisdom");
		yaml.Value(mob.get_wis());

		yaml.Key("constitution");
		yaml.Value(mob.get_con());

		yaml.Key("charisma");
		yaml.Value(mob.get_cha());

		yaml.DecreaseIndent();
	}

	// Action flags
	auto act_flags = ConvertFlagsToNames(mob.char_specials.saved.act, "action_flags");
	if (!act_flags.empty())
	{
		yaml.Key("action_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : act_flags)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Affect flags
	auto aff_flags = ConvertFlagsToNames(AFF_FLAGS(&mob), "affect_flags");
	if (!aff_flags.empty())
	{
		yaml.Key("affect_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : aff_flags)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Skills (with comments). Use the raw trained-skill map (skill_level),
	// NOT GetSkill(): GetSkill() layers on instance context -- equipment,
	// affects and a kDominationArena clamp keyed off in_room -- which is
	// meaningless for a prototype (in_room is NOWHERE) and would zero mob
	// skills on resave. Mirrors WorldChecksum::SerializeMob and the loader
	// so skills survive the round-trip (issue #3391).
	std::vector<std::pair<int, int>> mob_skills;
	for (const auto &kv : mob.GetCharSkills())
	{
		if (kv.second.skill_level > 0)
		{
			mob_skills.emplace_back(static_cast<int>(kv.first), kv.second.skill_level);
		}
	}
	std::sort(mob_skills.begin(), mob_skills.end());

	if (!mob_skills.empty())
	{
		yaml.Key("skills");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &kv : mob_skills)
		{
			out << yaml.GetIndent() << "- skill_id: " << kv.first;
			out << "  # " << GetSkillNameComment(static_cast<ESkill>(kv.first)) << std::endl;
			out << yaml.GetIndent() << "  value: " << kv.second << std::endl;
		}

		yaml.DecreaseIndent();
	}

	// Triggers (with comments)
	if (mob.proto_script && !mob.proto_script->empty())
	{
		yaml.Key("triggers");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (auto trig_vnum : *mob.proto_script)
		{
			std::string trig_comment = GetTriggerNameComment(trig_vnum);
			yaml.SequenceItem(trig_vnum, trig_comment);
		}

		yaml.DecreaseIndent();
	}

	// Enhanced E-spec block. Always emitted because mob_type is always
	// "E" in this code path and the Python converter writes the block
	// unconditionally (carrying at least resistances/saves as zero-arrays).
	// `tascii` appends into the buffer (uses strlen(ascii) + strncat) and
	// does NOT clear it -- callers must zero-init or its output becomes
	// the concatenation of whatever was in stack memory from prior mob
	// iterations. That's the root cause of the "special_bitvector accretes
	// across mobs" diff: 'f1 0' on mob 1, 'f1 0 0 0' on mob 2, etc.
	char special_buf[kMaxStringLength];
	special_buf[0] = '\0';
	mob.mob_specials.npc_flags.tascii(FlagData::kPlanesNumber, special_buf, sizeof(special_buf));
	std::string role_str = mob.get_role().to_string();
	{
		{
			yaml.Key("enhanced");
			out << std::endl;
			yaml.IncreaseIndent();

			if (mob.get_str_add() != 0)
			{
				yaml.Key("str_add");
				yaml.Value(mob.get_str_add());
			}
			if (mob.add_abils.hitreg != 0)
			{
				yaml.Key("hp_regen");
				yaml.Value(mob.add_abils.hitreg);
			}
			if (mob.add_abils.armour != 0)
			{
				yaml.Key("armour_bonus");
				yaml.Value(mob.add_abils.armour);
			}
			if (mob.add_abils.manareg != 0)
			{
				yaml.Key("mana_regen");
				yaml.Value(mob.add_abils.manareg);
			}
			if (mob.add_abils.cast_success != 0)
			{
				yaml.Key("cast_success");
				yaml.Value(mob.add_abils.cast_success);
			}
			if (mob.add_abils.morale != 0)
			{
				yaml.Key("morale");
				yaml.Value(mob.add_abils.morale);
			}
			if (mob.add_abils.initiative_add != 0)
			{
				yaml.Key("initiative_add");
				yaml.Value(mob.add_abils.initiative_add);
			}
			if (mob.add_abils.absorb != 0)
			{
				yaml.Key("absorb");
				yaml.Value(mob.add_abils.absorb);
			}
			if (mob.add_abils.aresist != 0)
			{
				yaml.Key("aresist");
				yaml.Value(mob.add_abils.aresist);
			}
			if (mob.add_abils.mresist != 0)
			{
				yaml.Key("mresist");
				yaml.Value(mob.add_abils.mresist);
			}
			if (mob.add_abils.presist != 0)
			{
				yaml.Key("presist");
				yaml.Value(mob.add_abils.presist);
			}
			if (mob.mob_specials.attack_type != 0)
			{
				yaml.Key("bare_hand_attack");
				yaml.Value(mob.mob_specials.attack_type);
			}
			if (mob.mob_specials.like_work != 0)
			{
				yaml.Key("like_work");
				yaml.Value(mob.mob_specials.like_work);
			}
			if (mob.mob_specials.MaxFactor != 0)
			{
				yaml.Key("max_factor");
				yaml.Value(mob.mob_specials.MaxFactor);
			}
			if (mob.mob_specials.extra_attack != 0)
			{
				yaml.Key("extra_attack");
				yaml.Value(mob.mob_specials.extra_attack);
			}
			if (mob.get_remort() != 0)
			{
				yaml.Key("mob_remort");
				yaml.Value(mob.get_remort());
			}

			// tascii leaves a trailing " " (or "0 " for empty) -- strip it
			// to match the Python converter, which emits "f1" not "f1 ".
			{
				size_t len = std::strlen(special_buf);
				while (len > 0 && special_buf[len - 1] == ' ')
				{
					special_buf[--len] = '\0';
				}
			}
			if (special_buf[0] != '0' || special_buf[1] != '\0')
			{
				yaml.Key("special_bitvector");
				yaml.Value(std::string(special_buf));
			}

			if (!role_str.empty() && role_str != "000000000")
			{
				yaml.Key("role");
				yaml.Value(role_str);
			}

			// Resistances. Emitted as a named map (kFire: 10, kMind: 5, ...)
			// so each value is self-describing and the file order is
			// independent of the EResist enum order. Only non-zero entries are
			// written; a missing key -- or a missing block -- means 0, matching
			// the loader's zero-initialized defaults.
			bool has_resistances = false;
			for (int i = EResist::kFirstResist; i <= EResist::kLastResist; ++i)
			{
				if (mob.add_abils.apply_resistance[i] != 0)
				{
					has_resistances = true;
					break;
				}
			}
			if (has_resistances)
			{
				yaml.Key("resistances");
				yaml.BeginBlock();
				for (int i = EResist::kFirstResist; i <= EResist::kLastResist; ++i)
				{
					const int value = mob.add_abils.apply_resistance[i];
					if (value != 0)
					{
						yaml.Key(NAME_BY_ITEM<EResist>(static_cast<EResist>(i)));
						yaml.Value(value);
					}
				}
				yaml.EndBlock();
			}

			// Saves -- same named-map format and rationale as resistances.
			bool has_saves = false;
			for (int i = to_underlying(ESaving::kFirst); i <= to_underlying(ESaving::kLast); ++i)
			{
				if (mob.add_abils.apply_saving_throw[i] != 0)
				{
					has_saves = true;
					break;
				}
			}
			if (has_saves)
			{
				yaml.Key("saves");
				yaml.BeginBlock();
				for (int i = to_underlying(ESaving::kFirst); i <= to_underlying(ESaving::kLast); ++i)
				{
					const int value = mob.add_abils.apply_saving_throw[i];
					if (value != 0)
					{
						yaml.Key(NAME_BY_ITEM<ESaving>(static_cast<ESaving>(i)));
						yaml.Value(value);
					}
				}
				yaml.EndBlock();
			}

			// Feats
			bool has_feats = false;
			for (size_t i = 0; i < mob.real_abils.Feats.size(); ++i)
			{
				if (mob.real_abils.Feats.test(i)) { has_feats = true; break; }
			}
			if (has_feats)
			{
				yaml.Key("feats");
				yaml.BeginSequence();
				yaml.IncreaseIndent();

				for (size_t i = 0; i < mob.real_abils.Feats.size(); ++i)
				{
					if (mob.real_abils.Feats.test(i))
					{
						yaml.SequenceItem(static_cast<int>(i), GetFeatNameComment(static_cast<int>(i)));
					}
				}

				yaml.DecreaseIndent();
			}

			// Spells (memorized slot counts) как map `id: count` c подписью
			// имени спелла. Раньше писался повторяющийся список (N копий id) --
			// рудимент легаси-формата `Spell: N` по строке на слот. Загрузчик
			// принимает и map, и старый список (см. ParseMobNode).
			bool has_spells = false;
			for (size_t i = 0; i < mob.real_abils.SplMem.size(); ++i)
			{
				if (mob.real_abils.SplMem[i] > 0) { has_spells = true; break; }
			}
			if (has_spells)
			{
				yaml.Key("spells");
				yaml.BeginBlock();

				for (size_t i = 0; i < mob.real_abils.SplMem.size(); ++i)
				{
					const int mem = mob.real_abils.SplMem[i];
					if (mem > 0)
					{
						yaml.Key(std::to_string(i));
						yaml.Value(mem, GetSpellNameComment(static_cast<ESpell>(i)));
					}
				}

				yaml.EndBlock();
			}

			// Helpers
			if (!mob.summon_helpers.empty())
			{
				yaml.Key("helpers");
				yaml.BeginSequence();
				yaml.IncreaseIndent();

				for (int helper_vnum : mob.summon_helpers)
				{
					yaml.SequenceItem(helper_vnum);
				}

				yaml.DecreaseIndent();
			}

			// Destinations: emit only the real route (dest[0..dest_count-1]),
			// NOT the full kMaxDest array. dest_count is the count the
			// legacy loader and the converter use; padding the sequence
			// with the array's trailing zeros makes the loader read
			// dest_count too large with bogus 0 destinations, breaking the
			// round-trip for every patrolling mob (issue #3384/#3391).
			if (mob.mob_specials.dest_count > 0)
			{
				yaml.Key("destinations");
				yaml.BeginSequence();
				yaml.IncreaseIndent();

				for (int d = 0; d < mob.mob_specials.dest_count
					&& d < static_cast<int>(mob.mob_specials.dest.size()); ++d)
				{
					yaml.SequenceItem(mob.mob_specials.dest[d]);
				}

				yaml.DecreaseIndent();
			}

			yaml.DecreaseIndent();  // enhanced
		}
	}

	// Dead-load list (legacy L-lines, issue #3291). Top-level, after the
	// enhanced block so the document layout matches the converter output.
	// Emitted as raw "- key: value" sequence-of-maps because the
	// Koi8rYamlEmitter has no Begin/EndMappingItem helpers (the same
	// pattern is used by the skills block above).
	if (!mob.dl_list.empty())
	{
		yaml.Key("dead_load");
		yaml.BeginSequence();
		yaml.IncreaseIndent();
		for (const auto &dl : mob.dl_list)
		{
			out << yaml.GetIndent() << "- obj_vnum: " << dl.obj_vnum << std::endl;
			out << yaml.GetIndent() << "  load_prob: " << dl.load_prob << std::endl;
			out << yaml.GetIndent() << "  load_type: " << dl.load_type << std::endl;
			out << yaml.GetIndent() << "  spec_param: " << dl.spec_param << std::endl;
		}
		yaml.DecreaseIndent();
	}

}

void YamlWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveMobs", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	// Collect this zone's mobs, sorted by vnum.
	std::vector<std::pair<int, CharData *>> entries;
	for (MobRnum mob_rnum = 0; mob_rnum <= top_of_mobt; ++mob_rnum)
	{
		if (!mob_index[mob_rnum].vnum) continue;
		int mob_vnum = mob_index[mob_rnum].vnum;
		if (mob_vnum < zone.vnum * 100 || mob_vnum > zone.top) continue;
		entries.emplace_back(mob_vnum, &mob_proto[mob_rnum]);
	}
	std::sort(entries.begin(), entries.end(),
		[](const auto &a, const auto &b) { return a.first < b.first; });

	namespace fs = std::filesystem;

	if (m_save_layout == YamlLayout::Flat)
	{
		const std::string flat_path = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/mobs.yaml";
		const std::string temp_file = flat_path + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			return;
		}
		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Mobs for zone " + std::to_string(zone.vnum));
		for (const auto &[vnum, mob] : entries)
		{
			yaml.EmptyLine();
			yaml.Comment("Mob #" + std::to_string(vnum));
			out << yaml.GetIndent() << (vnum % 100) << ":" << std::endl;
			yaml.IncreaseIndent();
			EmitMobBody(yaml, out, *mob);
			yaml.DecreaseIndent();
		}
		out.close();
		if (std::rename(temp_file.c_str(), flat_path.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), flat_path.c_str());
			return;
		}
		log("Saved %zu mobs (flat) for zone %d", entries.size(), zone.vnum);
		CleanupOtherLayout(zone.vnum, "mobs", YamlLayout::Flat);
		return;
	}

	// Per-file layout: one file per mob.
	const std::string mobs_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/mobs";
	if (!fs::exists(mobs_dir))
	{
		fs::create_directories(mobs_dir);
	}
	int saved_count = 0;
	for (const auto &[vnum, mob] : entries)
	{
		if (specific_vnum != -1 && vnum != specific_vnum) continue;

		int rel_num = vnum % 100;
		std::ostringstream mob_file_ss;
		mob_file_ss << mobs_dir << "/" << std::setfill('0') << std::setw(2) << rel_num << ".yaml";
		std::string mob_file = mob_file_ss.str();
		std::string temp_file = mob_file + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			continue;
		}

		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Mob #" + std::to_string(vnum));
		yaml.EmptyLine();
		EmitMobBody(yaml, out, *mob);

		out.close();
		if (std::rename(temp_file.c_str(), mob_file.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), mob_file.c_str());
			continue;
		}
		++saved_count;
	}

	log("Saved %d mobs for zone %d", saved_count, zone.vnum);
	RebuildPerZoneIndex(zone.vnum, "mobs");
	CleanupOtherLayout(zone.vnum, "mobs", YamlLayout::PerFile);
}
void YamlWorldDataSource::EmitObjectBody(Koi8rYamlEmitter &yaml, std::ostream &out, CObjectPrototype *obj)
{
	// Names
	yaml.Key("names");
	out << std::endl;
	yaml.IncreaseIndent();

	yaml.Key("aliases");
	yaml.Value(obj->get_aliases());

	yaml.Key("nominative");
	yaml.Value(obj->get_PName(grammar::ECase::kNom));

	yaml.Key("genitive");
	yaml.Value(obj->get_PName(grammar::ECase::kGen));

	yaml.Key("dative");
	yaml.Value(obj->get_PName(grammar::ECase::kDat));

	yaml.Key("accusative");
	yaml.Value(obj->get_PName(grammar::ECase::kAcc));

	yaml.Key("instrumental");
	yaml.Value(obj->get_PName(grammar::ECase::kIns));

	yaml.Key("prepositional");
	yaml.Value(obj->get_PName(grammar::ECase::kPre));

	yaml.DecreaseIndent();

	// Long description ("when in room"). Yaml convention: stored under
	// `short_desc:` to mirror the legacy file's 8th string, which is the
	// long-form description. m_short_description is the inventory name
	// (== nominative) and is already serialised via names/nominative.
	yaml.Key("short_desc");
	yaml.Value(obj->get_description(), true);  // literal=true

	// Action description (optional)
	if (!obj->get_action_description().empty())
	{
		yaml.Key("action_desc");
		yaml.Value(obj->get_action_description(), true);  // literal=true
	}

	// Type
	yaml.Key("type");
	yaml.Value(ReverseLookupEnum("obj_types", static_cast<int>(obj->get_type())));

	// Material (with comment)
	int material_id = static_cast<int>(obj->get_material());
	yaml.Key("material");
	yaml.Value(material_id, GetMaterialNameComment(material_id));

	// issue.magic-items-hotfix (point 5): scrolls/wands/staves/potions/drink-containers/fountains keep
	// their payload in extra_values; the raw val[0..3] just duplicate it, so skip them (YAML). Loaders
	// reconstruct val[] from the keys (ConvertObjValues / obj_save).
	const auto obj_type = obj->get_type();
	const bool hotfix_extended = obj_type == EObjType::kScroll || obj_type == EObjType::kWand
		|| obj_type == EObjType::kStaff || obj_type == EObjType::kPotion
		|| obj_type == EObjType::kLiquidContainer || obj_type == EObjType::kFountain;
	if (!hotfix_extended) {
		yaml.Key("values");
		yaml.BeginSequence();
		yaml.IncreaseIndent();
		yaml.SequenceItem(obj->get_val(0), GetObjValueComment(obj_type, 0, obj->get_val(0)));
		yaml.SequenceItem(obj->get_val(1), GetObjValueComment(obj_type, 1, obj->get_val(1)));
		yaml.SequenceItem(obj->get_val(2), GetObjValueComment(obj_type, 2, obj->get_val(2)));
		yaml.SequenceItem(obj->get_val(3), GetObjValueComment(obj_type, 3, obj->get_val(3)));
		yaml.DecreaseIndent();
	}

	// Weight, cost, rent
	yaml.Key("weight");
	yaml.Value(obj->get_weight());

	yaml.Key("cost");
	yaml.Value(obj->get_cost());

	yaml.Key("rent_off");
	yaml.Value(obj->get_rent_off());

	yaml.Key("rent_on");
	yaml.Value(obj->get_rent_on());

	// Spec param (optional)
	if (obj->get_spec_param() != 0)
	{
		yaml.Key("spec_param");
		yaml.Value(obj->get_spec_param());
	}

	// Durability and timer
	yaml.Key("max_durability");
	yaml.Value(obj->get_maximum_durability());

	yaml.Key("cur_durability");
	yaml.Value(obj->get_current_durability());

	yaml.Key("timer");
	yaml.Value(obj->get_timer());

	// issue #3581: obj->spell и его уровень (level) -- мёртвая пара, в yaml больше не пишем.

	yaml.Key("sex");
	yaml.Value(ReverseLookupEnum("genders", static_cast<int>(obj->get_sex())));

	// Max in world (optional)
	if (obj->get_max_in_world() != -1)
	{
		yaml.Key("max_in_world");
		yaml.Value(obj->get_max_in_world());
	}

	// Minimum remorts (optional)
	if (obj->get_minimum_remorts() != 0)
	{
		yaml.Key("minimum_remorts");
		yaml.Value(obj->get_minimum_remorts());
	}

	// Extra flags
	auto extra_flags = ConvertFlagsToNames(obj->get_extra_flags(), "extra_flags");
	if (!extra_flags.empty())
	{
		yaml.Key("extra_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : extra_flags)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Wear flags
	int wear_flags = obj->get_wear_flags();
	if (wear_flags != 0)
	{
		yaml.Key("wear_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (int bit = 0; bit < 32; ++bit)
		{
			if (wear_flags & (1 << bit))
			{
				std::string flag_name = ReverseLookupEnum("wear_flags", bit);
				if (!flag_name.empty() && flag_name != std::to_string(bit))
				{
					yaml.SequenceItem(flag_name);
				}
				else
				{
					yaml.SequenceItem("UNUSED_" + std::to_string(bit));
				}
			}
		}

		yaml.DecreaseIndent();
	}

	// No flags
	auto no_flags = ConvertFlagsToNames(obj->get_no_flags(), "no_flags");
	if (!no_flags.empty())
	{
		yaml.Key("no_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : no_flags)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Anti flags
	auto anti_flags = ConvertFlagsToNames(obj->get_anti_flags(), "anti_flags");
	if (!anti_flags.empty())
	{
		yaml.Key("anti_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : anti_flags)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Affect flags. m_waffect_flags хранит биты EWeaponAffect, имена
	// берём из EWeaponAffect-таблицы (не EAffect), иначе round-trip
	// конвертера сломает item-affects: загружено как kDetectPoison,
	// сохранено как kHorse (бит 27 в EAffect-словаре). Идём по битам
	// сами и берём имена через NAME_BY_ITEM<EWeaponAffect>.
	std::vector<std::string> affect_flags;
	{
		const auto &fld = obj->get_affect_flags();
		for (size_t plane = 0; plane < FlagData::kPlanesNumber; ++plane)
		{
			Bitvector plane_bits = fld.get_plane(plane);
			if (plane_bits == 0) continue;
			for (int bit = 0; bit < 30; ++bit)
			{
				if (!(plane_bits & (1u << bit))) continue;
				const Bitvector v = (plane == 0) ? (1u << bit) :
					((plane == 1) ? (kIntOne | (1u << bit)) :
					(plane == 2) ? (kIntTwo | (1u << bit)) :
					(kIntThree | (1u << bit)));
				try
				{
					const auto &name = NAME_BY_ITEM<EWeaponAffect>(static_cast<EWeaponAffect>(v));
					if (!name.empty()) affect_flags.push_back(name);
				}
				catch (const std::out_of_range &)
				{
					const int idx = static_cast<int>(plane) * 30 + bit;
					affect_flags.push_back("UNUSED_" + std::to_string(idx));
				}
			}
		}
	}
	if (!affect_flags.empty())
	{
		yaml.Key("affect_flags");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &flag : affect_flags)
		{
			yaml.SequenceItem(flag);
		}

		yaml.DecreaseIndent();
	}

	// Applies (with location comments)
	bool has_applies = false;
	for (int i = 0; i < kMaxObjAffect; ++i)
	{
		if (obj->get_affected(i).location != EApply::kNone)
		{
			has_applies = true;
			break;
		}
	}

	if (has_applies)
	{
		yaml.Key("applies");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (int i = 0; i < kMaxObjAffect; ++i)
		{
			if (obj->get_affected(i).location != EApply::kNone)
			{
				int location = static_cast<int>(obj->get_affected(i).location);
				int modifier = obj->get_affected(i).modifier;

				out << yaml.GetIndent() << "- location: " << location;
				out << "  # " << GetApplyTypeNameComment(location) << std::endl;
				out << yaml.GetIndent() << "  modifier: " << modifier << std::endl;
			}
		}

		yaml.DecreaseIndent();
	}

	// Умения предмета (legacy S-строки), issue #3386.
	if (obj->has_skills())
	{
		yaml.Key("skills");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (const auto &[skill_id, value] : obj->get_skills())
		{
			out << yaml.GetIndent() << "- skill_id: " << static_cast<int>(skill_id);
			out << "  # " << GetSkillNameComment(skill_id) << std::endl;
			out << yaml.GetIndent() << "  value: " << value << std::endl;
		}

		yaml.DecreaseIndent();
	}

	// Extra descriptions.
	// issue #3596: экстра-дескриптор без описания (0 символов/одни пробелы, напр.
	// после очистки в OLC через /c) считаем несуществующим -- не пишем ни keyword,
	// ни блок description; если валидных дескрипторов не осталось, секцию опускаем.
	std::vector<const ExtraDescription *> exdescs;
	for (const auto &exdesc : obj->get_ex_description())
	{
		if (!exdesc.keyword.empty()
			&& exdesc.description.find_first_not_of(" \t\r\n") != std::string::npos)
		{
			exdescs.push_back(&exdesc);
		}
	}
	if (!exdescs.empty())
	{
		yaml.Key("extra_descriptions");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		// Вектор хранит описания в порядке файла (load делает push_back), пишем вперёд.
		for (const auto *exdesc : exdescs)
		{
			// keywords may start with '-' (legitimately a single dash);
			// Koi8rYamlEmitter::Value handles leading-indicator quoting.
			out << yaml.GetIndent() << "- keywords:";
			// IncreaseIndent so yaml.Value's literal-block branch emits content
			// lines at the correct column for indicator "2" (parent_indent + 2).
			yaml.IncreaseIndent();
			yaml.Value(exdesc->keyword);
			yaml.DecreaseIndent();
			out << yaml.GetIndent() << "  description:";
			yaml.IncreaseIndent();
			yaml.Value(exdesc->description, true);
			yaml.DecreaseIndent();
		}

		yaml.DecreaseIndent();
	}

	// Extra values (V-строки): данные зелий и прочих контейнеров с жидкостью.
	// Без сохранения этой секции данные о заклинаниях зелий пропадут (issue #3218).
	if (!obj->get_all_values().empty())
	{
		std::vector<std::pair<std::string, int>> sorted_vals;
		for (const auto &kv : obj->get_all_values())
		{
			std::string key_str = text_id::ToStr(text_id::kObjVals, to_underlying(kv.first));
			if (!key_str.empty())
			{
				sorted_vals.emplace_back(std::move(key_str), kv.second);
			}
		}

		if (!sorted_vals.empty())
		{
			std::sort(sorted_vals.begin(), sorted_vals.end(),
				[](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
					return a.first < b.first;
				});

			yaml.Key("extra_values");
			yaml.BeginBlock();
			for (const auto &kv : sorted_vals)
			{
				yaml.Key(kv.first);
				yaml.Value(kv.second, GetExtraValueComment(kv.first, kv.second));
			}
			yaml.EndBlock();
		}
	}

	// Triggers (with comments)
	if (obj->get_proto_script_ptr() && !obj->get_proto_script().empty())
	{
		yaml.Key("triggers");
		yaml.BeginSequence();
		yaml.IncreaseIndent();

		for (auto trig_vnum : obj->get_proto_script())
		{
			std::string trig_comment = GetTriggerNameComment(trig_vnum);
			yaml.SequenceItem(trig_vnum, trig_comment);
		}

		yaml.DecreaseIndent();
	}

}

void YamlWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveObjects", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	// Collect this zone's objects, sorted by vnum.
	std::vector<std::pair<int, CObjectPrototype *>> entries;
	for (const auto &[obj_vnum, obj_rnum] : obj_proto.vnum2index())
	{
		if (obj_vnum < zone.vnum * 100 || obj_vnum > zone.top) continue;
		auto obj = obj_proto[obj_rnum];
		if (!obj) continue;
		entries.emplace_back(obj_vnum, obj.get());
	}
	std::sort(entries.begin(), entries.end(),
		[](const auto &a, const auto &b) { return a.first < b.first; });

	namespace fs = std::filesystem;

	if (m_save_layout == YamlLayout::Flat)
	{
		const std::string flat_path = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/objects.yaml";
		const std::string temp_file = flat_path + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			return;
		}
		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Objects for zone " + std::to_string(zone.vnum));
		for (const auto &[vnum, obj] : entries)
		{
			yaml.EmptyLine();
			yaml.Comment("Object #" + std::to_string(vnum));
			out << yaml.GetIndent() << (vnum % 100) << ":" << std::endl;
			yaml.IncreaseIndent();
			EmitObjectBody(yaml, out, obj);
			yaml.DecreaseIndent();
		}
		out.close();
		if (std::rename(temp_file.c_str(), flat_path.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), flat_path.c_str());
			return;
		}
		log("Saved %zu objects (flat) for zone %d", entries.size(), zone.vnum);
		CleanupOtherLayout(zone.vnum, "objects", YamlLayout::Flat);
		return;
	}

	// Per-file layout: one file per object.
	const std::string objs_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/objects";
	if (!fs::exists(objs_dir))
	{
		fs::create_directories(objs_dir);
	}
	int saved_count = 0;
	for (const auto &[vnum, obj] : entries)
	{
		if (specific_vnum != -1 && vnum != specific_vnum) continue;

		int rel_num = vnum % 100;
		std::ostringstream obj_file_ss;
		obj_file_ss << objs_dir << "/" << std::setfill('0') << std::setw(2) << rel_num << ".yaml";
		std::string obj_file = obj_file_ss.str();
		std::string temp_file = obj_file + ".tmp";
		std::ofstream out(temp_file);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open %s for writing", temp_file.c_str());
			continue;
		}

		Koi8rYamlEmitter yaml(out);
		yaml.Comment("Object #" + std::to_string(vnum));
		yaml.EmptyLine();
		EmitObjectBody(yaml, out, obj);

		out.close();
		if (std::rename(temp_file.c_str(), obj_file.c_str()) != 0)
		{
			log("SYSERR: Failed to rename %s to %s", temp_file.c_str(), obj_file.c_str());
			continue;
		}
		++saved_count;
	}

	log("Saved %d objects for zone %d", saved_count, zone.vnum);
	RebuildPerZoneIndex(zone.vnum, "objects");
	CleanupOtherLayout(zone.vnum, "objects", YamlLayout::PerFile);
}
// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<IWorldDataSource> CreateYamlDataSource(const std::string &world_dir)
{
	return std::make_unique<YamlWorldDataSource>(world_dir);
}

} // namespace world_loader

#endif // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
