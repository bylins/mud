/**
\file pc_race_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue.player-races-rework).
\brief Display-name container for player races (PC tribes).
\details A race's name has four gendered forms; they live in a vnum-keyed
		 msg_container::MsgContainer<int, EGender> -- the message type IS the grammar gender, so
		 MUD::RaceMessages().GetMessage(race_vnum, ch->get_sex()) returns the form for that character.
		 The sheaf <name> holds the menu label. XML source is lib/cfg/messages/ru/pc_race_msg.xml,
		 loaded through cfg_manager (before "pc_races") and exposed via MUD::RaceMessages().
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_PC_RACE_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_PC_RACE_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"   // EGender + its meta-enum reflection
#include "engine/structs/msg_container.h"
#include "utils/grammar/gender.h"                  // EGender

#include <string>
#include <vector>

namespace player_races {

// Vnum-keyed: each sheaf is one race, keyed by the race vnum from pc_races.xml. The message type is
// EGender, so a race's name forms are stored as <message type="kMale" .../> etc. and selected by sex.
using RaceMessages = msg_container::MsgContainer<int, EGender>;

/**
 * Loads/reloads lib/cfg/messages/ru/pc_race_msg.xml into MUD::RaceMessages().
 */
class RaceMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace player_races

#endif // BYLINS_SRC_GAMEPLAY_MECHANICS_PC_RACE_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
