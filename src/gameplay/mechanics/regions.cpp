/**
\file regions.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.regions).
\brief Loader for the geographic regions registry.
*/

#include "regions.h"

#include "cities.h"
#include "engine/db/global_objects.h"
#include "utils/parse.h"
#include "utils/utils.h"

using parser_wrapper::DataNode;

namespace regions {

RegionInfoBuilder::ItemPtr RegionInfoBuilder::Build(DataNode &node) {
	try {
		const int vnum = parse::ReadAsInt(node.GetValue("vnum"));
		std::string text_id = parse::ReadAsStr(node.GetValue("id"));
		auto mode = RegionInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
		// <capital> is intentionally not parsed yet (the format may still change).
		auto region = std::make_shared<RegionInfo>(vnum, text_id, mode);
		if (node.GoToChild("zones")) {
			for (auto &zone : node.Children("zone")) {
				region->zones_.push_back(parse::ReadAsInt(zone.GetValue("vnum")));
			}
			node.GoToParent();
		}
		return region;
	} catch (std::exception &e) {
		err_log("Region parsing error (incorrect value '%s').", e.what());
		return nullptr;
	}
}

void RegionsLoader::Load(DataNode data) {
	MUD::Regions().Init(data.Children());
}

void RegionsLoader::Reload(DataNode data) {
	MUD::Regions().Reload(data.Children());
}

std::string RegionsLoader::EditableWhat() const {
	return "region";
}

std::vector<cfg_manager::EditableElement> RegionsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &region : MUD::Regions()) {
		if (region.GetId() < 0) {   // skip the kUndefined sentinel
			continue;
		}
		out.push_back({std::to_string(region.GetId()), region.GetTextId()});
	}
	return out;
}

cfg_manager::ValidationResult RegionsLoader::Validate(DataNode &doc) const {
	if (MUD::Regions().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Region data failed to parse (see syslog for the offending region)."};
}

parser_wrapper::DataNode RegionsLoader::FindElementNode(DataNode root, const std::string &id) const {
	// A <region> is keyed by its integer vnum; iterate all children with a name check (a node copied
	// out of a keyed range would otherwise carry that range's filter -- see GuildsLoader).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "region" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string RegionsLoader::CanonicalElementId(const std::string &id) const {
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode RegionsLoader::CreateElementNode(DataNode root, const std::string &id) const {
	auto node = root.AddChild("region");
	node.SetValue("vnum", id);
	node.SetValue("id", "Undefined");
	return node;
}

// ---- lookups --------------------------------------------------------------------------------------

int RegionVnumByZone(int zone_vnum) {
	for (const auto &region : MUD::Regions()) {
		if (region.GetId() < 0) {   // skip the kUndefined sentinel
			continue;
		}
		if (region.HasZone(zone_vnum)) {
			return region.GetId();
		}
	}
	return kRegionUndefined;
}

int RegionVnumByCityId(const std::string &city_id) {
	const auto &city = MUD::Cities().FindItem(city_id);
	if (city.GetId() < 0) {   // unknown city
		return kRegionUndefined;
	}
	for (const auto &zone : city.GetZones()) {
		const int region = RegionVnumByZone(zone.vnum);
		if (region != kRegionUndefined) {
			return region;
		}
	}
	return kRegionUndefined;
}

} // namespace regions

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
