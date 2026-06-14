// Part of Bylins http://www.mud.ru
// issue.player-races-rework: cfg-driven, Vedun-editable PC-race InfoContainer.

#include "player_races.h"

#include "pc_race_messages.h"
#include "engine/db/global_objects.h"
#include "utils/parse.h"
#include "utils/utils.h"

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

	for (auto &bp : node.Children("place_of_birth")) {
		race->birth_places_.push_back(parse::ReadAsInt(bp.GetValue("id")));
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

int BirthPlaceByMenuChoice(int race_vnum, const char *arg) {
	const int num = atoi(arg);
	const auto &places = MUD::PcRaces()[race_vnum].GetBirthPlaces();
	if (num >= 1 && static_cast<size_t>(num) <= places.size()) {
		return places[num - 1];
	}
	return kRaceUndefined;
}

} // namespace player_races

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
