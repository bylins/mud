/**
\file pc_race_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.player-races-rework).
\brief Loader for the player-race display-name container (keyed by race vnum, typed by EGender).
*/

#include "pc_race_messages.h"

#include "engine/db/global_objects.h"
#include "engine/structs/info_container.h"

namespace player_races {

void RaceMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::RaceMessages().Init(data.Children());
}

void RaceMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::RaceMessages().Reload(data.Children());
}

std::string RaceMessagesLoader::EditableWhat() const {
	return "pcracemsg";
}

std::vector<cfg_manager::EditableElement> RaceMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::RaceMessages()) {
		const int id = sheaf.GetId();
		const std::string id_str = (id == info_container::kUndefinedVnum) ? "kDefault" : std::to_string(id);
		const std::string label = (id == info_container::kUndefinedVnum)
			? "race name (shared fallback)" : "race name (vnum " + std::to_string(id) + ")";
		out.push_back({id_str, label});
	}
	return out;
}

cfg_manager::ValidationResult RaceMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::RaceMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "PC-race name data failed to parse (see syslog)."};
}

std::string RaceMessagesLoader::CanonicalElementId(const std::string &id) const {
	if (id == "kDefault") {
		return "kDefault";
	}
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

parser_wrapper::DataNode RaceMessagesLoader::CreateElementNode(parser_wrapper::DataNode root,
															   const std::string &id) const {
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace player_races

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
