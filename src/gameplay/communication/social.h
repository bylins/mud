/**
 \authors Created by Sventovit
 \date 20.01.2021.
 \brief Заголовок модуля социалов.
 \details issue.socials: socials are vnum-keyed data-driven entities (like guilds), loaded from
		  cfg/messages/<locale>/social_msg.xml via cfg_manager and exposed through MUD::Socials().
		  A social has a list of keyword synonyms, four EPosition gates, and up to six message slots.
*/

#ifndef BYLINS_SRC_COMMUNICATION_SOCIAL_H_
#define BYLINS_SRC_COMMUNICATION_SOCIAL_H_

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"
#include "engine/structs/info_container.h"
#include "engine/structs/meta_enum.h"

#include <map>
#include <string>
#include <vector>

class CharData;

// The six message slots of a social. Perspective is encoded in the name (ToChar/ToRoom/ToVict),
// matching the act() targets. Parsed from <message type="..."/> in social_msg.xml.
enum class ESocialMsg {
	kUndefined = 0,
	kCharNoArg,    // actor, no target
	kOthersNoArg,  // room, no target
	kCharFound,    // actor, with target  (absent => the social ignores arguments)
	kOthersFound,  // room, with target
	kVictFound,    // the target
	kNotFound,     // an arg was given but no target found (optional)
};

template<>
const std::string &NAME_BY_ITEM<ESocialMsg>(ESocialMsg item);
template<>
ESocialMsg ITEM_BY_NAME<ESocialMsg>(const std::string &name);
template<>
const std::map<ESocialMsg, std::string> &NAMES_OF<ESocialMsg>();

namespace communication::social {

class SocialInfo : public info_container::BaseItem<int> {
	friend class SocialInfoBuilder;

	std::vector<std::string> keywords_;
	EPosition ch_min_pos_ = EPosition::kStand;
	EPosition ch_max_pos_ = EPosition::kStand;
	EPosition vict_min_pos_ = EPosition::kStand;
	EPosition vict_max_pos_ = EPosition::kStand;
	std::map<ESocialMsg, std::string> messages_;   // stored already wrapped in &K...&n colour codes

 public:
	SocialInfo() = default;
	SocialInfo(int id, std::string &text_id, EItemMode mode) : BaseItem<int>(id, text_id, mode) {}

	[[nodiscard]] const std::vector<std::string> &GetKeywords() const { return keywords_; }
	[[nodiscard]] EPosition GetChMinPos() const { return ch_min_pos_; }
	[[nodiscard]] EPosition GetChMaxPos() const { return ch_max_pos_; }
	[[nodiscard]] EPosition GetVictMinPos() const { return vict_min_pos_; }
	[[nodiscard]] EPosition GetVictMaxPos() const { return vict_max_pos_; }
	// The message text for the slot, or an empty string if the social has no such slot.
	[[nodiscard]] const std::string &GetMsg(ESocialMsg type) const;
	[[nodiscard]] bool HasMsg(ESocialMsg type) const { return messages_.count(type) > 0; }
};

class SocialInfoBuilder : public info_container::IItemBuilder<SocialInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
};

using SocialsInfo = info_container::InfoContainer<int, SocialInfo, SocialInfoBuilder>;

// Loads cfg/messages/ru/social_msg.xml into MUD::Socials() and rebuilds the keyword->vnum search
// index. Editable in the Vedun editor by vnum (mirrors guilds::GuildsLoader).
class SocialsLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] parser_wrapper::DataNode FindElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace communication::social

// Prefix-match a typed command against every social keyword synonym; returns the social vnum or -1.
int find_action(char *cmd);
int do_social(CharData *ch, char *argument);

#endif //BYLINS_SRC_COMMUNICATION_SOCIAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
