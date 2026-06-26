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
	kRoomAffectVisible,         // room-aura listing: viewer lacks kDetectMagic (the "physical" trace)
	kRoomAffectInvisible,       // viewer has kDetectMagic and is NOT the caster
	kRoomAffectSelfInvisible,   // viewer has kDetectMagic and IS the caster
	kRoomAffectPkVisible,       // pk variant: viewer lacks kDetectMagic
	kRoomAffectPkInvisible,     // pk variant: viewer has kDetectMagic
};

// Message lookup keyed by the room affect. RoomAffectMsg falls back to the shared kDefault sheaf;
// RoomAffectMsgRaw is sheaf-direct (empty string on miss -- use where a missing message stays silent).
[[nodiscard]] const std::string &RoomAffectMsg(ERoomAffect affect, ERoomAffectMsgType slot);
[[nodiscard]] const std::string &RoomAffectMsgRaw(ERoomAffect affect, ERoomAffectMsgType slot);

class RoomAffectMessagesLoader : public cfg_manager::ICfgLoader {  // cfg id "room_affect_msg"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
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
