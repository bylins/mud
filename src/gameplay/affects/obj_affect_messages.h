/**
 \brief issue.obj-affects: obj-affect message system (cfg/messages/ru/obj_affect_msg.xml).
 \details Mirrors room_spells::RoomAffectMsg* but keyed by obj_affects::EObjAffect and with an
		  OBJECT-only slot set: an obj affect concerns the item and the character holding/wearing it.
		  Message templates that name the item carry a single "%s" placeholder; the runtime formats it
		  with the item's name in the grammatical case the message expects.
*/

#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_OBJ_AFFECT_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_OBJ_AFFECT_MESSAGES_H_

#include "obj_affects.h"               // obj_affects::EObjAffect (+ its NAME_BY_ITEM specializations)
#include "engine/boot/cfg_manager.h"

#include <map>
#include <string>

namespace obj_affects {

// Per-obj-affect message slots. Impose text may also come from the casting spell's own sheaf (the
// alter-* spells); these slots let the affect own its lifecycle text (wear-off / dispel / display).
enum class EObjAffectMsgType {
	kUndefined = 0,
	kShortDesc,             // display name in examine / identify (e.g. the affect's label)
	kDiagToChar,            // the "examine" diagnostic line for this affect on the item
	kAffImposedToChar,      // affect cast onto the item: to the caster
	kAffImposedToRoom,      // ... to onlookers
	kAffExpiredToChar,      // affect wore off: to the item's holder ("%s" = item name)
	kAffDispelledToChar,    // affect stripped by a player effect (remove curse / darkness / dispel)
	kTriggerToChar,         // a lifecycle/container trigger fired: flavor to the actor ($o = item)
	kTriggerToRoom,         // ... to onlookers ($n = the actor, $o = item)
};

// Message lookup keyed by the obj affect. ObjAffectMsg falls back to the shared kDefault sheaf;
// ObjAffectMsgRaw is sheaf-direct (empty string on miss -- use where a missing message stays silent).
[[nodiscard]] const std::string &ObjAffectMsg(EObjAffect affect, EObjAffectMsgType slot);
[[nodiscard]] const std::string &ObjAffectMsgRaw(EObjAffect affect, EObjAffectMsgType slot);

class ObjAffectMessagesLoader : public cfg_manager::ICfgLoader {  // cfg id "obj_affect_msg"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

}  // namespace obj_affects

template<>
const std::string &NAME_BY_ITEM<obj_affects::EObjAffectMsgType>(obj_affects::EObjAffectMsgType item);
template<>
obj_affects::EObjAffectMsgType ITEM_BY_NAME<obj_affects::EObjAffectMsgType>(const std::string &name);
template<>
const std::map<obj_affects::EObjAffectMsgType, std::string> &NAMES_OF<obj_affects::EObjAffectMsgType>();

#endif  // BYLINS_SRC_GAMEPLAY_AFFECTS_OBJ_AFFECT_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
