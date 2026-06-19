/**
\file guild_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.thing-names).
\brief Guild message container: loader + guilds::EMsg name registration.
*/

#include "guild_messages.h"

#include "engine/structs/info_container.h"   // info_container::kUndefinedVnum
#include "engine/db/global_objects.h"

#include <map>
#include <string>
#include <vector>

namespace {
// File-scope so NAMES_OF can expose it to the Vedun editor (mirrors skill_messages).
const std::map<guilds::EMsg, std::string> kGuildMsgNames{
		{guilds::EMsg::kGreeting, "kGreeting"},
		{guilds::EMsg::kDischarge, "kDischarge"},
		{guilds::EMsg::kCannotToChar, "kCannotToChar"},
		{guilds::EMsg::kCannotToRoom, "kCannotToRoom"},
		{guilds::EMsg::kAskToChar, "kAskToChar"},
		{guilds::EMsg::kAskToRoom, "kAskToRoom"},
		{guilds::EMsg::kLearnToChar, "kLearnToChar"},
		{guilds::EMsg::kLearnToRoom, "kLearnToRoom"},
		{guilds::EMsg::kInquiry, "kInquiry"},
		{guilds::EMsg::kDidNotTeach, "kDidNotTeach"},
		{guilds::EMsg::kAllSkills, "kAllSkills"},
		{guilds::EMsg::kTalentEarned, "kTalentEarned"},
		{guilds::EMsg::kNothingLearned, "kNothingLearned"},
		{guilds::EMsg::kListEmpty, "kListEmpty"},
		{guilds::EMsg::kIsInsolvent, "kIsInsolvent"},
		{guilds::EMsg::kFree, "kFree"},
		{guilds::EMsg::kTemporary, "kTemporary"},
		{guilds::EMsg::kYouGiveMoney, "kYouGiveMoney"},
		{guilds::EMsg::kSomeoneGivesMoney, "kSomeoneGivesMoney"},
		{guilds::EMsg::kFailToChar, "kFailToChar"},
		{guilds::EMsg::kFailToRoom, "kFailToRoom"},
		{guilds::EMsg::kError, "kError"},
	};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<guilds::EMsg>(const guilds::EMsg item) {
	return kGuildMsgNames.at(item);
}

template<>
const std::map<guilds::EMsg, std::string> &NAMES_OF<guilds::EMsg>() {
	return kGuildMsgNames;
}

template<>
guilds::EMsg ITEM_BY_NAME<guilds::EMsg>(const std::string &name) {
	static std::map<std::string, guilds::EMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kGuildMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name); // throws std::out_of_range on unknown name
}

namespace guilds {

void GuildMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::GuildMessages().Init(data.Children());
}

void GuildMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::GuildMessages().Reload(data.Children());
}

// issue.vedun-msg-editor: editor discovery + safe-commit validation.
std::string GuildMessagesLoader::EditableWhat() const {
	return "guildmsg";
}

std::vector<cfg_manager::EditableElement> GuildMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::GuildMessages()) {
		const int id = sheaf.GetId();
		const std::string id_str = (id == info_container::kUndefinedVnum) ? "kDefault" : std::to_string(id);
		const std::string label = (id == info_container::kUndefinedVnum)
				? "guild messages (shared)" : "guild messages (vnum " + std::to_string(id) + ")";
		out.push_back({id_str, label});
	}
	return out;
}

cfg_manager::ValidationResult GuildMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::GuildMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Guild-message data failed to parse (see syslog for the offending sheaf/message)."};
}

std::string GuildMessagesLoader::CanonicalElementId(const std::string &id) const {
	if (id == "kDefault") {
		return "kDefault";
	}
	// A per-guild override sheaf is keyed by a non-negative guild vnum.
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

parser_wrapper::DataNode GuildMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// An empty sheaf for `id`; the editor then adds <message> children.
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace guilds

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
