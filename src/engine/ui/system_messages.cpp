/**
\file system_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue.sml-cdata-msg).
\brief Loader + enum reflection for the system-text message container.
*/

#include "system_messages.h"

#include "engine/db/global_objects.h"
#include "engine/structs/info_container.h"

namespace {
const std::map<system_messages::ESystemMsg, std::string> kSystemMsgNames{
	{system_messages::ESystemMsg::kBackground, "kBackground"},
	{system_messages::ESystemMsg::kCredits, "kCredits"},
	{system_messages::ESystemMsg::kGreetings, "kGreetings"},
	{system_messages::ESystemMsg::kImmList, "kImmList"},
	{system_messages::ESystemMsg::kInfo, "kInfo"},
	{system_messages::ESystemMsg::kMotd, "kMotd"},
	{system_messages::ESystemMsg::kNameRules, "kNameRules"},
	{system_messages::ESystemMsg::kPolicies, "kPolicies"},
	{system_messages::ESystemMsg::kImmRules, "kImmRules"},
};
} // namespace

template<>
const std::string &NAME_BY_ITEM<system_messages::ESystemMsg>(const system_messages::ESystemMsg item) {
	return kSystemMsgNames.at(item);
}

template<>
const std::map<system_messages::ESystemMsg, std::string> &NAMES_OF<system_messages::ESystemMsg>() {
	return kSystemMsgNames;
}

template<>
system_messages::ESystemMsg ITEM_BY_NAME<system_messages::ESystemMsg>(const std::string &name) {
	static std::map<std::string, system_messages::ESystemMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kSystemMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace system_messages {

namespace {
// XML line-end normalization (pugixml parse_eol, per the XML spec) collapses the CDATA bodies to
// bare '\n', but the game's output layer needs CRLF (every literal in the codebase uses "\r\n", and
// the old file loader appended "\r\n" per line). Convert lazily and cache; the cache is dropped on
// (re)load so it always reflects the current file. Map references stay valid across inserts, so the
// const& returned to callers is stable until the next reload.
std::map<ESystemMsg, std::string> g_text_cache;

std::string ToCrlf(const std::string &s) {
	std::string out;
	out.reserve(s.size() + s.size() / 32 + 2);
	for (const char c : s) {
		if (c == '\r') {
			continue;          // drop stray CR; the '\n' below re-adds the pair
		}
		if (c == '\n') {
			out += "\r\n";
		} else {
			out += c;
		}
	}
	return out;
}
} // namespace

const std::string &GetText(const ESystemMsg msg) {
	auto it = g_text_cache.find(msg);
	if (it == g_text_cache.end()) {
		// All system texts live in the single shared "kDefault" sheaf (kUndefinedVnum).
		it = g_text_cache.emplace(
			msg, ToCrlf(MUD::SystemMessages().GetMessage(info_container::kUndefinedVnum, msg))).first;
	}
	return it->second;
}

void SystemMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::SystemMessages().Init(data.Children());
	g_text_cache.clear();
}

void SystemMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::SystemMessages().Reload(data.Children());
	g_text_cache.clear();
}

} // namespace system_messages

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
