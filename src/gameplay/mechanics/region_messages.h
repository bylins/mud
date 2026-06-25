/**
\file region_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.regions).
\brief Message container for geographic regions.
\details A vnum-keyed msg_container::MsgContainer<int, ERegionMsg>, keyed by the region vnum from
		 regions.xml. For now each region has two messages -- kName (full name) and kShortDesc -- but
		 the container is extensible. XML source is lib/cfg/messages/ru/region_msg.xml, loaded through
		 cfg_manager (before "regions") and exposed via MUD::RegionMessages().
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_REGION_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_REGION_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"

#include <map>
#include <string>
#include <vector>

namespace regions {
enum class ERegionMsg {
	kName,        // full region name (e.g. the Kievan principality)
	kShortDesc,   // short label (e.g. the capital's name)
};
} // namespace regions

template<>
const std::string &NAME_BY_ITEM<regions::ERegionMsg>(regions::ERegionMsg item);
template<>
regions::ERegionMsg ITEM_BY_NAME<regions::ERegionMsg>(const std::string &name);
template<>
const std::map<regions::ERegionMsg, std::string> &NAMES_OF<regions::ERegionMsg>();

namespace regions {

// Vnum-keyed: each sheaf is one region, keyed by the region vnum from regions.xml.
using RegionMessages = msg_container::MsgContainer<int, regions::ERegionMsg>;

/**
 * Loads/reloads lib/cfg/messages/ru/region_msg.xml into MUD::RegionMessages().
 */
class RegionMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace regions

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_REGION_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
