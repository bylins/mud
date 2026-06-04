/**
\file skill_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue #3310).
\brief In-game message container for skills.
\details Stores skill-related player messages in a msg_container::MsgContainer keyed
		 by ESkill (the skill id) and ESkillMsg (the per-skill message type). The XML
		 source is lib/cfg/skill_msg.xml, loaded through cfg_manager and exposed via
		 MUD::SkillMessages(). The default sheaf (XML id "kDefault") maps to
		 ESkill::kUndefined and supplies messages common to all skills; a per-skill
		 sheaf overrides the default only for the types it defines.

		 Mirrors the spell message system (gameplay/magic/spell_messages.*, issue #3304).

		 Message text is stored WITHOUT a trailing "\r\n": act()-style messages do not
		 need it, and SendMsgToChar() call sites append it explicitly.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_SKILL_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_SKILL_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"
#include "skills.h"

#include <map>
#include <string>
#include <vector>

/**
 * Per-skill message types. The id type of the container is ESkill; this enum
 * identifies the kind of message inside a single skill's sheaf. Perspective is
 * encoded in the type name (ToChar/ToRoom/ToVict) where a message has several
 * targets, matching the act() targets, as in ESpellMsg.
 *
 * The types below describe message *circumstances* that recur across many skills
 * (issue #3310 part 2). Their text lives in the kDefault sheaf and is overridden
 * per skill only where the wording genuinely differs. Grow this enum as more
 * skills are migrated.
 */
enum class ESkillMsg {
	kUndefined = 0,
	// Invariant precondition / failure messages (kDefault sheaf; per-skill override allowed).
	kDontKnowSkill,		// the character has not learned the skill.
	kOnCooldown,		// the skill is still recovering / the character has not rested.
	kCantWhileMounted,	// the skill cannot be used while mounted.
	kMustBeMounted,		// the skill can only be used while mounted.
	kGetOnFeet,			// position too low; the character must stand up first.
	kCantFightNow,		// the character is temporarily unable to fight.
	kNotFighting,		// the skill needs a fight, but the character is fighting no one.
	kPeacefulRoom,		// the room is peaceful and forbids violence.
	kNoTarget,			// no target was specified (!vict).
	kCantTargetSelf,	// the character cannot target themselves.
	kNeedWeapon,		// the skill requires a weapon to be wielded.
	kWrongWeapon,		// the wielded weapon is unsuitable for the skill.
	// Combat damage messages (originally from lib/misc/messages, issue #3310); served
	// to the fight system via MUD::SkillMessages() since issue #3316.
	// Death/Miss/Hit/God x damager(ToChar)/damagee(ToVict)/onlookers(ToRoom).
	kFightDeathToChar, kFightDeathToVict, kFightDeathToRoom,
	kFightMissToChar, kFightMissToVict, kFightMissToRoom,
	kFightHitToChar, kFightHitToVict, kFightHitToRoom,
	kFightGodToChar, kFightGodToVict, kFightGodToRoom,
};

template<>
const std::string &NAME_BY_ITEM<ESkillMsg>(ESkillMsg item);
template<>
ESkillMsg ITEM_BY_NAME<ESkillMsg>(const std::string &name);
template<>
const std::map<ESkillMsg, std::string> &NAMES_OF<ESkillMsg>();  // issue.vedun-msg-editor

namespace skills {

using SkillMessages = msg_container::MsgContainer<ESkill, ESkillMsg>;

/**
 * Loads/reloads lib/cfg/skill_msg.xml into MUD::SkillMessages().
 */
class SkillMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] bool IsValidElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace skills

#endif // BYLINS_SRC_GAMEPLAY_SKILLS_SKILL_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
