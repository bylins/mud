/**
\file region_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.regions).
\brief Loader + enum reflection for the region message container.
*/

#include "region_messages.h"

#include "engine/db/global_objects.h"
#include "engine/structs/info_container.h"

namespace {
const std::map<regions::ERegionMsg, std::string> kRegionMsgNames{
	{regions::ERegionMsg::kName, "kName"},
	{regions::ERegionMsg::kShortDesc, "kShortDesc"},
};
} // namespace

template<>
const std::string &NAME_BY_ITEM<regions::ERegionMsg>(const regions::ERegionMsg item) {
	return kRegionMsgNames.at(item);
}

template<>
const std::map<regions::ERegionMsg, std::string> &NAMES_OF<regions::ERegionMsg>() {
	return kRegionMsgNames;
}

template<>
regions::ERegionMsg ITEM_BY_NAME<regions::ERegionMsg>(const std::string &name) {
	static std::map<std::string, regions::ERegionMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kRegionMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace regions {

void RegionMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::RegionMessages().Init(data.Children());
}

void RegionMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::RegionMessages().Reload(data.Children());
}

std::string RegionMessagesLoader::EditableWhat() const {
	return "regionmsg";
}

std::vector<cfg_manager::EditableElement> RegionMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::RegionMessages()) {
		const int id = sheaf.GetId();
		const std::string id_str = (id == info_container::kUndefinedVnum) ? "kDefault" : std::to_string(id);
		const std::string label = (id == info_container::kUndefinedVnum)
			? "region messages (shared fallback)" : "region messages (vnum " + std::to_string(id) + ")";
		out.push_back({id_str, label});
	}
	return out;
}

cfg_manager::ValidationResult RegionMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::RegionMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Region-message data failed to parse (see syslog)."};
}

std::string RegionMessagesLoader::CanonicalElementId(const std::string &id) const {
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

parser_wrapper::DataNode RegionMessagesLoader::CreateElementNode(parser_wrapper::DataNode root,
																 const std::string &id) const {
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace regions

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
