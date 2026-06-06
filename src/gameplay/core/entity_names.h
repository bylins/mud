/**
\file entity_names.h - a part of the Bylins engine.
\authors Created by Claude (issue.thing-names).
\brief Shared display-name registry for several simple-name entity types.
\details Mob classes, mob races and zone types each have only a plain (non-declined) display
		 name and are unlikely to grow their own message types, so their names share a single
		 file, cfg/messages/ru/entity_names.xml, instead of one message container per type. The
		 file has one section per entity type, each listing <name id="..." val="..."/> entries;
		 EntityNamesLoader fills a string-keyed registry per section, consulted by that entity's
		 builder at load time (so entity_names loads BEFORE those entities). Mirrors the bespoke
		 currency/class name registries, minus the grammatical-case machinery.
*/

#ifndef BYLINS_SRC_GAMEPLAY_CORE_ENTITY_NAMES_H_
#define BYLINS_SRC_GAMEPLAY_CORE_ENTITY_NAMES_H_

#include "engine/boot/cfg_manager.h"

#include <string>

namespace entity_names {

class EntityNamesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

// Look up a display name by id token / text_id; returns an empty string if absent.
const std::string &FindMobClassName(const std::string &id);
const std::string &FindMobRaceName(const std::string &id);
const std::string &FindZoneTypeName(const std::string &id);

} // namespace entity_names

#endif // BYLINS_SRC_GAMEPLAY_CORE_ENTITY_NAMES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
