/**
 \brief issue.affect-migration: room-affect message system (cfg/messages/ru/room_affect_msg.xml).
 \details Mirrors affects::AffectMsg* for char affects, but keyed by room_spells::ERoomAffect and with
		  a ROOM-only slot set: a room affect has no affected-char/opponent perspective the way a char
		  affect does -- everything is the caster ("char") or onlookers ("room"), plus the room-aura
		  listing variants picked by the viewer's kDetectMagic / caster identity / pk_unique.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MAGIC_ROOM_AFFECT_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_MAGIC_ROOM_AFFECT_MESSAGES_H_

#include "magic_rooms.h"            // room_spells::ERoomAffect (+ its NAME_BY_ITEM specializations)
#include "engine/boot/cfg_manager.h"

#include <map>
#include <string>

namespace room_spells {

// Per-room-affect message slots. (Cast-fizzle messages like kCastForbidden* stay in the SPELL message
// system: they describe a failed cast, not an affect.)
enum class ERoomAffectMsgType {
	kUndefined = 0,
	kShortDesc,                 // optional display name (stat / future OLC)
	kAffImposedToChar,          // cast landed: to the caster
	kAffImposedToRoom,          // cast landed: to onlookers
	kAffDispelledToChar,        // removed (dispel): to the caster
	kAffDispelledToRoom,        // removed (dispel): to onlookers
	kAffExpiredToChar,          // wore off: to the (former) caster if present
	kAffExpiredToRoom,          // wore off: to onlookers
	kAffInterruptedToChar,      // controlled effect replaced by a recast: to the (re)caster
	// issue.room-affect-trigger-improve: flavor shown when an EVENT trigger engages a character -- the
	// affect's own "why this happened" line, emitted before the action's effect runs. There is one
	// pair per trigger kind so a passage affect can word "walked through" / "opened" / "picked" /
	// "unlocked" distinctly; CastRoomEntryAction picks the pair from the firing trigger.
	kTriggerOnEntryToChar,      // kEnter family (moved through the passage): to the actor
	kTriggerOnEntryToRoom,      // ... to onlookers ($n0 = the actor)
	kTriggerOnOpenToChar,       // kOpen (opened the door): to the actor
	kTriggerOnOpenToRoom,       // ... to onlookers
	kTriggerOnPickToChar,       // kPick (picked the lock): to the actor
	kTriggerOnPickToRoom,       // ... to onlookers
	kTriggerOnUnlockToChar,     // kUnlock (unlocked the door): to the actor
	kTriggerOnUnlockToRoom,     // ... to onlookers
	kTriggerOnCloseToChar,      // kClose (closed the door): to the actor
	kTriggerOnCloseToRoom,      // ... to onlookers
	kTriggerOnLockToChar,       // kLock (locked the door): to the actor
	kTriggerOnLockToRoom,       // ... to onlookers
	// issue.room-affect-trigger-improve: shown room-wide when a trigger fired but its effect produced no
	// visible result (e.g. a violent side_spell suppressed by a peaceful room, or a refused cast) -- so a
	// triggered trap never reads as a silent no-op. Impersonal (no $-codes); sent to the whole room.
	kTriggerNoEffect,
	// issue.affects-improve (Phase B): per-pulse flavor lines, rotated by the affect's tick count.
	// Affect-native replacement for the spell sheaf's kCustomMsgOne..N that EmitRoomTickMessage read.
	kTickMsgOne,
	kTickMsgTwo,
	kTickMsgThree,
	kTickMsgFour,
	kTickMsgFive,
	kTickMsgSix,
	kTickMsgSeven,
	kTickMsgEight,
	kRoomAffectVisible,         // room-aura listing: viewer lacks kDetectMagic (the "physical" trace)
	kRoomAffectInvisible,       // viewer has kDetectMagic and is NOT the caster
	kRoomAffectSelfInvisible,   // viewer has kDetectMagic and IS the caster
	kRoomAffectPkVisible,       // pk variant: viewer lacks kDetectMagic
	kRoomAffectPkInvisible,     // pk variant: viewer has kDetectMagic
	// issue.character-affect-triggers: flavor for damage dealt by a ROOM/EXIT affect's own <damage> action
	// (e.g. a trap, a room/exit kDispell sting). When set it FULLY REPLACES the generic combat hit line,
	// mirroring the char-affect kDamageTo* keys. $n = the char dealing/taking the damage, $N = the victim.
	kDamageToChar,
	kDamageToVict,
	kDamageToRoom,
};

// Message lookup keyed by the room affect. RoomAffectMsg falls back to the shared kDefault sheaf;
// RoomAffectMsgRaw is sheaf-direct (empty string on miss -- use where a missing message stays silent).
[[nodiscard]] const std::string &RoomAffectMsg(ERoomAffect affect, ERoomAffectMsgType slot);
[[nodiscard]] const std::string &RoomAffectMsgRaw(ERoomAffect affect, ERoomAffectMsgType slot);

class RoomAffectMessagesLoader : public cfg_manager::IEditableCfgLoader {  // cfg id "room_affect_msg"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	// issue.unstable-hotfixes: in-game editing of room-affect messages (`vedun roomaffectmsg`). Keyed by
	// the <msg_sheaf id=> (ERoomAffect token; "kDefault" for the shared sheaf). Mirrors AffectMessagesLoader.
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

}  // namespace room_spells

template<>
const std::string &NAME_BY_ITEM<room_spells::ERoomAffectMsgType>(room_spells::ERoomAffectMsgType item);
template<>
room_spells::ERoomAffectMsgType ITEM_BY_NAME<room_spells::ERoomAffectMsgType>(const std::string &name);
template<>
const std::map<room_spells::ERoomAffectMsgType, std::string> &NAMES_OF<room_spells::ERoomAffectMsgType>();

#endif  // BYLINS_SRC_GAMEPLAY_MAGIC_ROOM_AFFECT_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
