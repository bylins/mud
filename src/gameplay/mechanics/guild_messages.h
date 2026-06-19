/**
\file guild_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.thing-names).
\brief In-game message container for guilds.
\details Guilds are keyed by an integer vnum, so guild messages live in a vnum-keyed
		 msg_container::MsgContainer<int, guilds::EMsg>. Today every message is shared by all
		 guilds and lives in the default sheaf (XML id "kDefault" => info_container::kUndefinedVnum);
		 the structure allows per-guild overrides keyed by a guild vnum later. The XML source is
		 lib/cfg/messages/ru/guild_msg.xml, loaded through cfg_manager (before "guilds") and exposed
		 via MUD::GuildMessages(). Mirrors the skill/spell message systems.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_GUILD_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_GUILD_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"
#include "guilds.h"   // guilds::EMsg

#include <map>
#include <string>
#include <vector>

template<>
const std::string &NAME_BY_ITEM<guilds::EMsg>(guilds::EMsg item);
template<>
guilds::EMsg ITEM_BY_NAME<guilds::EMsg>(const std::string &name);
template<>
const std::map<guilds::EMsg, std::string> &NAMES_OF<guilds::EMsg>();

namespace guilds {

// Vnum-keyed: the default sheaf (kUndefinedVnum) holds the shared messages; future per-guild
// overrides would be keyed by a guild vnum.
using GuildMessages = msg_container::MsgContainer<int, guilds::EMsg>;

/**
 * Loads/reloads lib/cfg/messages/ru/guild_msg.xml into MUD::GuildMessages().
 */
class GuildMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace guilds

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_GUILD_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
