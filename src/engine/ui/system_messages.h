/**
\file system_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.sml-cdata-msg).
\brief Container for the game's static "system texts" (greetings, motd, immortal rules, ...).
\details The nine system texts formerly loaded from lib/text/* into global char* strings now live
		 in one cfg/messages/ru/system_msg.xml, inside a single msg_sheaf (id="kDefault"). Each
		 message carries its multi-line preformatted body in a <![CDATA[...]]> block (read verbatim
		 by MsgSheafBuilder). Loaded through cfg_manager (SystemMessagesLoader) and exposed via
		 MUD::SystemMessages(); read individual texts with system_messages::GetText(ESystemMsg).
		 The file is marked vedun="false" and its loader is intentionally non-editable, so the Vedun
		 editor never rewrites it (a whole-file save would flatten the CDATA formatting).
*/

#ifndef BYLINS_SRC_ENGINE_UI_SYSTEM_MESSAGES_H_
#define BYLINS_SRC_ENGINE_UI_SYSTEM_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"

#include <map>
#include <string>

namespace system_messages {
enum class ESystemMsg {
	kBackground,   // background story (shown in char-gen)
	kCredits,      // game credits
	kGreetings,    // connect/login greeting screen
	kImmList,      // list of gods
	kInfo,         // info page
	kMotd,         // message of the day (mortals)
	kNameRules,    // character-name rules (char-gen)
	kPolicies,     // policies page
	kImmRules,     // rules for immortals (was lib/text/rules)
	kWelcome,      // welcome shown after login (was config.cpp WELC_MESSG)
	kStartMessage, // first-login message for a new character (was config.cpp START_MESSG)
};
} // namespace system_messages

template<>
const std::string &NAME_BY_ITEM<system_messages::ESystemMsg>(system_messages::ESystemMsg item);
template<>
system_messages::ESystemMsg ITEM_BY_NAME<system_messages::ESystemMsg>(const std::string &name);
template<>
const std::map<system_messages::ESystemMsg, std::string> &NAMES_OF<system_messages::ESystemMsg>();

namespace system_messages {

// Single-sheaf container: the shared "kDefault" sheaf holds every system text, keyed by ESystemMsg.
using SystemMessages = msg_container::MsgContainer<int, ESystemMsg>;

// The text for one system message (from the single kDefault sheaf). Returns the container's
// fallback string if the message is missing.
[[nodiscard]] const std::string &GetText(ESystemMsg msg);

// Loads/reloads cfg/messages/ru/system_msg.xml into MUD::SystemMessages(). Deliberately a plain
// (non-editable) ICfgLoader: the Vedun editor must never rewrite this preformatted-text file.
class SystemMessagesLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) override;
	void Reload(parser_wrapper::DataNode data) override;
};

} // namespace system_messages

#endif // BYLINS_SRC_ENGINE_UI_SYSTEM_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
