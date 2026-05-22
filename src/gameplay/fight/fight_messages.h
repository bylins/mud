/**
\file fight_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue #3311).
\brief In-game message container for damage sources (weapon attacks + server damage).
\details Stores combat messages keyed by fight::EDamageSource (the damage source:
		 weapon hit type, or a server trap / bleeding) and fight::EFightMsg (the
		 per-type message kind). The XML source is
		 lib/cfg/hit_msg.xml, loaded through cfg_manager and exposed via
		 MUD::FightMessages(). The default sheaf (XML id "kDefault") maps to
		 EDamageSource::kUndefined.

		 Mirrors the spell message system (gameplay/magic/spell_messages.*, issue #3304).
		 The message kinds are named after the spell damage messages (ESpellMsg kFight*).

		 Issue #3311 only stores these messages; the fight system still emits combat
		 text from lib/misc/messages (the in-memory fight_messages[] table).
*/

#ifndef BYLINS_SRC_GAMEPLAY_FIGHT_FIGHT_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_FIGHT_FIGHT_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"
#include "fight_constants.h"

#include <string>

namespace fight {

/**
 * Per-damage-source message kinds. The container id type is EDamageSource; this enum
 * identifies the kind of message inside a single damage source's sheaf. Perspective is
 * encoded in the name (ToChar/ToVict/ToRoom), matching the act() targets. Named
 * after the spell damage messages (ESpellMsg kFight*) so the two read alike.
 */
enum class EFightMsg {
	kUndefined = 0,
	// Death / Miss / Hit / God x damager(ToChar) / damagee(ToVict) / onlookers(ToRoom).
	kFightDeathToChar, kFightDeathToVict, kFightDeathToRoom,
	kFightMissToChar, kFightMissToVict, kFightMissToRoom,
	kFightHitToChar, kFightHitToVict, kFightHitToRoom,
	kFightGodToChar, kFightGodToVict, kFightGodToRoom,
};

} // namespace fight

template<>
const std::string &NAME_BY_ITEM<fight::EDamageSource>(fight::EDamageSource item);
template<>
fight::EDamageSource ITEM_BY_NAME<fight::EDamageSource>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<fight::EFightMsg>(fight::EFightMsg item);
template<>
fight::EFightMsg ITEM_BY_NAME<fight::EFightMsg>(const std::string &name);

namespace fight {

using FightMessages = msg_container::MsgContainer<EDamageSource, EFightMsg>;

/**
 * Loads/reloads lib/cfg/hit_msg.xml into MUD::FightMessages().
 */
class FightMessagesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace fight

#endif // BYLINS_SRC_GAMEPLAY_FIGHT_FIGHT_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
