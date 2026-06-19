/**
\file entity_names.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.thing-names).
\brief Loader + registries for the shared simple-name file (mob classes / mob races / zone types).
*/

#include "entity_names.h"

#include "utils/parser_wrapper.h"

#include <unordered_map>

namespace entity_names {

namespace {
using DataNode = parser_wrapper::DataNode;
using Registry = std::unordered_map<std::string, std::string>;

Registry g_mob_class_names;
Registry g_mob_race_names;
Registry g_zone_type_names;

// Fill `reg` from every <name id="..." val="..."/> under the <section> child of the root.
void LoadSection(DataNode &root, const char *section, Registry &reg) {
	reg.clear();
	for (auto &section_node : root.Children(section)) {       // 0 or 1 such section
		for (auto &name : section_node.Children("name")) {
			const char *id = name.GetValue("id");
			if (!id || !*id) {
				continue;
			}
			const char *val = name.GetValue("val");
			reg[id] = val ? val : "";
		}
	}
}

void LoadAll(DataNode data) {
	LoadSection(data, "mob_classes", g_mob_class_names);
	LoadSection(data, "mob_races", g_mob_race_names);
	LoadSection(data, "zone_types", g_zone_type_names);
}

const std::string &Find(const Registry &reg, const std::string &id) {
	static const std::string kEmpty;
	const auto it = reg.find(id);
	return it == reg.end() ? kEmpty : it->second;
}
} // namespace

void EntityNamesLoader::Load(DataNode data) { LoadAll(std::move(data)); }
void EntityNamesLoader::Reload(DataNode data) { LoadAll(std::move(data)); }

const std::string &FindMobClassName(const std::string &id) { return Find(g_mob_class_names, id); }
const std::string &FindMobRaceName(const std::string &id) { return Find(g_mob_race_names, id); }
const std::string &FindZoneTypeName(const std::string &id) { return Find(g_zone_type_names, id); }

} // namespace entity_names

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
