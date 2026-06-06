/**
 * \brief Zone type registry implementation (issue.ztypes-migrate).
 * \details Replaces the hand-rolled lib/misc/ztypes.lst parser
 *          (legacy InitZoneTypes in db.cpp) with an info_container loader.
 *          The zone-file format is unchanged: zone.type carries the vnum,
 *          which is the int id of this registry. Authors maintain vnum
 *          uniqueness in cfg/zone_types.xml (no enforcement in code).
 */

#include "zone_types.h"

#include "engine/db/global_objects.h"
#include "utils/parse.h"
#include "utils/utils_string.h"

namespace zone_types {

using ItemPtr = ZoneTypeInfoBuilder::ItemPtr;

void ZoneTypesLoader::Load(DataNode data) {
	MUD::ZoneTypes().Init(data.Children());
}

void ZoneTypesLoader::Reload(DataNode data) {
	MUD::ZoneTypes().Reload(data.Children());
}

ItemPtr ZoneTypeInfoBuilder::Build(DataNode &node) {
	try {
		return ParseType(node);
	} catch (std::exception &e) {
		err_log("Zone type parsing error: '%s'", e.what());
		return nullptr;
	}
}

ItemPtr ZoneTypeInfoBuilder::ParseType(DataNode node) {
	auto vnum = parse::ReadAsInt(node.GetValue("vnum"));
	// v1: no <type mode=> attribute -- every entry is implicitly kEnabled.
	// info_container BaseItem<int> requires the mode arg in its ctor.
	auto mode = EItemMode::kEnabled;

	std::string text_id{"kUndefined"};
	std::string name{"undefined"};
	try {
		text_id = parse::ReadAsStr(node.GetValue("text_id"));
		name = parse::ReadAsStr(node.GetValue("name"));
	} catch (...) {
	}

	auto info = std::make_shared<ZoneTypeInfo>(vnum, text_id, name, mode);

	if (node.GoToChild("ingredients")) {
		ParseIngredients(info, node);
	}

	return info;
}

void ZoneTypeInfoBuilder::ParseIngredients(ItemPtr &info, DataNode &node) {
	const char *raw = node.GetValue("val");
	if (!raw || !*raw) {
		return;
	}
	try {
		for (const auto &token : utils::Split(raw, '|')) {
			info->ingredients_.push_back(parse::ReadAsInt(token.c_str()));
		}
	} catch (std::runtime_error &e) {
		err_log("ingredients parse error (%s) in zone type vnum=%d.",
				e.what(), info->GetId());
	}
}

}  // namespace zone_types

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
