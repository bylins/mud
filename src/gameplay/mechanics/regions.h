/**
\file regions.h - a part of the Bylins engine.
\authors Created by Claude (issue.regions).
\brief Geographic regions registry (rudimentary).
\details A region is, for now, just a vnum + string id -- the seed of a future geography layer where
		 world zones belong to regions, player activity feeds reputation in a region's capital city,
		 and road zones act as transport/trade arteries. The <capital> tag (a cities.xml city id) is
		 intentionally NOT parsed yet, since the format may still change. A vnum-keyed,
		 Vedun-editable InfoContainer (cfg/mechanics/regions.xml), exposed via MUD::Regions().
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_REGIONS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_REGIONS_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/info_container.h"

#include <string>
#include <vector>

namespace regions {

class RegionInfo : public info_container::BaseItem<int> {
 public:
	RegionInfo() = default;
	RegionInfo(int id, std::string &text_id, EItemMode mode) : BaseItem<int>(id, text_id, mode) {};
};

class RegionInfoBuilder : public info_container::IItemBuilder<RegionInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
};

using RegionsInfo = info_container::InfoContainer<int, RegionInfo, RegionInfoBuilder>;

class RegionsLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] parser_wrapper::DataNode FindElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace regions

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_REGIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
