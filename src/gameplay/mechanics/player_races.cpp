// Part of Bylins http://www.mud.ru
// issue.player-races-rework: cfg-driven, Vedun-editable PC-race InfoContainer.

#include "player_races.h"

#include "pc_race_messages.h"
#include "cities.h"
#include "regions.h"
#include "region_messages.h"
#include "engine/db/global_objects.h"
#include "engine/structs/structs.h"
#include "utils/parse.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <fmt/format.h>

using parser_wrapper::DataNode;

namespace player_races {

PcRaceInfoBuilder::ItemPtr PcRaceInfoBuilder::Build(DataNode &node) {
	try {
		return ParseRace(node);
	} catch (std::exception &e) {
		err_log("PC race parsing error (incorrect value '%s').", e.what());
		return nullptr;
	}
}

PcRaceInfoBuilder::ItemPtr PcRaceInfoBuilder::ParseRace(DataNode node) {
	const int vnum = parse::ReadAsInt(node.GetValue("vnum"));
	std::string text_id = parse::ReadAsStr(node.GetValue("id"));
	auto mode = PcRaceInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	auto race = std::make_shared<PcRaceInfo>(vnum, text_id, mode);

	if (node.GoToChild("place_of_birth")) {
		for (auto &city : node.Children("city")) {
			race->birth_cities_.push_back(parse::ReadAsStr(city.GetValue("id")));
		}
		node.GoToParent();
	}
	if (node.GoToChild("race_features")) {
		for (auto &feat : node.Children("feature")) {
			race->features_.push_back(parse::ReadAsConstant<EFeat>(feat.GetValue("id")));
		}
		node.GoToParent();
	}
	return race;
}

void PcRacesLoader::Load(DataNode data) {
	MUD::PcRaces().Init(data.Children());
}

void PcRacesLoader::Reload(DataNode data) {
	MUD::PcRaces().Reload(data.Children());
}

std::string PcRacesLoader::EditableWhat() const {
	return "pcrace";
}

std::vector<cfg_manager::EditableElement> PcRacesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &race : MUD::PcRaces()) {
		if (race.GetId() < 0) {   // skip the kUndefined sentinel
			continue;
		}
		out.push_back({std::to_string(race.GetId()),
					   race.GetTextId() + " " + MUD::RaceMessages().GetName(race.GetId())});
	}
	return out;
}

cfg_manager::ValidationResult PcRacesLoader::Validate(DataNode &doc) const {
	if (MUD::PcRaces().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "PC race data failed to parse (see syslog for the offending race)."};
}

parser_wrapper::DataNode PcRacesLoader::FindElementNode(DataNode root, const std::string &id) const {
	// A <race> is keyed by its integer vnum; iterate all children with a name check (a node copied
	// out of a keyed range would otherwise carry that range's filter -- see GuildsLoader).
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "race" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string PcRacesLoader::CanonicalElementId(const std::string &id) const {
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

parser_wrapper::DataNode PcRacesLoader::CreateElementNode(DataNode root, const std::string &id) const {
	auto node = root.AddChild("race");
	node.SetValue("vnum", id);
	node.SetValue("id", "Undefined");
	node.SetValue("mode", "kEnabled");
	node.AddChild("race_features");
	return node;
}

// ---- public API ----------------------------------------------------------------------------------

std::string FormatRacesMenu() {
	std::string out;
	for (const auto &race : MUD::PcRaces()) {
		if (race.GetId() < 0 || !race.IsAvailable()) {
			continue;
		}
		out += fmt::format(" {}) {}\r\n", race.GetId() + 1, MUD::RaceMessages().GetName(race.GetId()));
	}
	return out;
}

int RaceVnumByMenuChoice(const char *arg) {
	const int num = atoi(arg);
	if (num < 1) {
		return kRaceUndefined;
	}
	const int vnum = num - 1;
	if (MUD::PcRaces().IsAvailable(vnum)) {
		return vnum;
	}
	return kRaceUndefined;
}

// ---- start-region selection -----------------------------------------------------------------------

std::vector<int> StartRegionsForRace(int race_vnum) {
	std::vector<int> result;
	for (const auto &city_id : MUD::PcRaces()[race_vnum].GetBirthCities()) {
		const int region = regions::RegionVnumByCityId(city_id);
		if (region == regions::kRegionUndefined) {
			continue;
		}
		if (std::find(result.begin(), result.end(), region) == result.end()) {
			result.push_back(region);
		}
	}
	return result;
}

std::string FormatStartRegionsMenu(int race_vnum) {
	std::string out;
	int n = 1;
	for (const int region : StartRegionsForRace(race_vnum)) {
		out += fmt::format(" {}) {}\r\n", n++,
						   MUD::RegionMessages().GetMessage(region, regions::ERegionMsg::kName));
	}
	return out;
}

std::string StartCityForRaceRegion(int race_vnum, int region_vnum) {
	for (const auto &city_id : MUD::PcRaces()[race_vnum].GetBirthCities()) {
		if (regions::RegionVnumByCityId(city_id) == region_vnum) {
			return city_id;
		}
	}
	return "";
}

int StartRegionByMenuChoice(int race_vnum, const char *arg) {
	const auto regions_list = StartRegionsForRace(race_vnum);
	// (1) a 1-based number over this race's region list.
	const int num = atoi(arg);
	if (num >= 1 && static_cast<size_t>(num) <= regions_list.size()) {
		return regions_list[num - 1];
	}
	// (2) a region short name, possibly abbreviated (needle is a prefix of the short name).
	std::string needle = arg;
	utils::ConvertToLow(needle);
	if (!needle.empty()) {
		for (const int region : regions_list) {
			std::string short_desc = MUD::RegionMessages().GetMessage(region, regions::ERegionMsg::kShortDesc);
			utils::ConvertToLow(short_desc);
			if (short_desc.rfind(needle, 0) == 0) {   // needle is a prefix of the short name
				return region;
			}
		}
	}
	return kRaceUndefined;
}

int StartRoomForRaceRegion(int race_vnum, int region_vnum) {
	const std::string city_id = StartCityForRaceRegion(race_vnum, region_vnum);
	if (city_id.empty()) {
		return kNowhere;
	}
	const auto &rents = MUD::Cities().FindItem(city_id).GetRentVnums();
	if (rents.empty()) {
		return kNowhere;
	}
	return *rents.begin();
}

} // namespace player_races

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
