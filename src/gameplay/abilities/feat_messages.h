/**
\file feat_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.thing-names).
\brief In-game message container for feats.
\details Stores feat-related player messages in a msg_container::MsgContainer keyed by
		 EFeat (the feat id) and EFeatMsg (the per-feat message type). The XML source is
		 lib/cfg/messages/ru/feat_msg.xml, loaded through cfg_manager and exposed via
		 MUD::FeatMessages(). For now the container carries only each feat's display name
		 (<name val="..."/>); feats are expected to gain activation/deactivation/ambience
		 messages later, which is why they get their own dedicated container rather than a
		 shared simple-name file (mirrors the skill message system, issue #3310).
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"
#include "feats.h"

#include <map>
#include <string>
#include <vector>

/**
 * Per-feat message types. The container id type is EFeat; this enum identifies the kind
 * of message inside a single feat's sheaf. Only kUndefined exists for now (the container
 * currently stores names only); grow this enum as feat activation/deactivation/ambience
 * messages are migrated.
 */
enum class EFeatMsg {
	kUndefined = 0,
};

template<>
const std::string &NAME_BY_ITEM<EFeatMsg>(EFeatMsg item);
template<>
EFeatMsg ITEM_BY_NAME<EFeatMsg>(const std::string &name);
template<>
const std::map<EFeatMsg, std::string> &NAMES_OF<EFeatMsg>();

namespace feats {

using FeatMessages = msg_container::MsgContainer<EFeat, EFeatMsg>;

/**
 * Loads/reloads lib/cfg/messages/ru/feat_msg.xml into MUD::FeatMessages().
 */
class FeatMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace feats

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
