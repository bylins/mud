/**
\file cities_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.cities).
\brief Loader + enum reflection for the city display-name container.
*/

#include "cities_messages.h"

#include "engine/db/global_objects.h"
#include "engine/structs/info_container.h"

namespace {
const std::map<cities::ECityMsg, std::string> kCityMsgNames{
	{cities::ECityMsg::kName, "kName"},
};
} // namespace

template<>
const std::string &NAME_BY_ITEM<cities::ECityMsg>(const cities::ECityMsg item) {
	return kCityMsgNames.at(item);
}

template<>
const std::map<cities::ECityMsg, std::string> &NAMES_OF<cities::ECityMsg>() {
	return kCityMsgNames;
}

template<>
cities::ECityMsg ITEM_BY_NAME<cities::ECityMsg>(const std::string &name) {
	static std::map<std::string, cities::ECityMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kCityMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace cities {

void CityMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::CityMessages().Init(data.Children());
}

void CityMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::CityMessages().Reload(data.Children());
}

std::string CityMessagesLoader::EditableWhat() const {
	return "citymsg";
}

std::vector<cfg_manager::EditableElement> CityMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::CityMessages()) {
		const int id = sheaf.GetId();
		const std::string id_str = (id == info_container::kUndefinedVnum) ? "kDefault" : std::to_string(id);
		const std::string label = (id == info_container::kUndefinedVnum)
			? "city name (shared fallback)" : "city name (vnum " + std::to_string(id) + ")";
		out.push_back({id_str, label});
	}
	return out;
}

cfg_manager::ValidationResult CityMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::CityMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "City-name data failed to parse (see syslog)."};
}

std::string CityMessagesLoader::CanonicalElementId(const std::string &id) const {
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

parser_wrapper::DataNode CityMessagesLoader::CreateElementNode(parser_wrapper::DataNode root,
															   const std::string &id) const {
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace cities

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
