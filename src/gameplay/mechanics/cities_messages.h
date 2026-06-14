/**
\file cities_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.cities).
\brief In-game display-name container for cities.
\details Cities are keyed by an integer vnum, so their localised display names live in a vnum-keyed
		 msg_container::MsgContainer<int, cities::ECityMsg>. The name is the sheaf's <name> field;
		 the ECityMsg message types are reserved for future per-city messages. XML source is
		 lib/cfg/messages/ru/cities_msg.xml, loaded through cfg_manager (before "cities") and exposed
		 via MUD::CityMessages(). Mirrors the guild message system.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_CITIES_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_CITIES_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"

#include <map>
#include <string>
#include <vector>

namespace cities {
// Reserved for future per-city messages; today only the sheaf <name> is used.
enum class ECityMsg { kName };
} // namespace cities

template<>
const std::string &NAME_BY_ITEM<cities::ECityMsg>(cities::ECityMsg item);
template<>
cities::ECityMsg ITEM_BY_NAME<cities::ECityMsg>(const std::string &name);
template<>
const std::map<cities::ECityMsg, std::string> &NAMES_OF<cities::ECityMsg>();

namespace cities {

// Vnum-keyed: each sheaf is one city's display name, keyed by the city vnum from cities.xml.
// id="kDefault" (kUndefinedVnum) holds the shared fallback name.
using CityMessages = msg_container::MsgContainer<int, cities::ECityMsg>;

/**
 * Loads/reloads lib/cfg/messages/ru/cities_msg.xml into MUD::CityMessages().
 */
class CityMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace cities

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_CITIES_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
