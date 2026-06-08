/**
 \brief issue.ext-affects: affect short names + "look at character" aura text (cfg/messages/ru/
		affect_msg.xml), keyed by affect.
 \details A MsgContainer<EAffect, EAffectMsgType>: one sheaf per affect (kShortDesc + kLook[+kLookPoly])
		  plus the shared "kDefault" sheaf for the merged shield/aura group scaffolding (prefix + the
		  noun forms). affected_bits is rebuilt from each affect's kShortDesc. The composition (which
		  affects, grouping, gender/poly/count selection) stays in code; only the words live here. The
		  EAffect enum itself stays in C++ (it is the identity, used in AFF_FLAGGED/switch everywhere).
*/

#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECT_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECT_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "affect_contants.h"   // EAffect

#include <map>
#include <string>

namespace affects {

// Slots within an affect's sheaf. kShortDesc/kLook/kLookPoly are per-affect; the kShield*/kAura* nouns
// and kShieldPrefix are the shared group scaffolding kept in the "kDefault" sheaf. $a in a value is the
// act gender suffix of the looked-at character.
enum class EAffectMsgType {
	kUndefined = 0,
	kShortDesc,        // short display name (affected_bits / score / identify); every affect
	kLook,             // ListOneChar aura line or fragment
	kLookPoly,         // poly (multi-form creature) variant of kLook
	kShieldPrefix,     // kDefault: "п╬п╨я─я┐п╤п╣п╫$a"
	kShieldNoun,       // kDefault: "я┴п╦я┌п╬п╪"  (one shield)
	kShieldNounMany,   // kDefault: "я┴п╦я┌п╟п╪п╦" (several)
	kAuraNoun,         // kDefault: "п╟я┐я─п╟"   (one aura)
	kAuraNounMany,     // kDefault: "п╟я┐я─я▀"   (several)
};

// affect = the affect whose sheaf to read (EAffect::kUndefined => the shared "kDefault" sheaf).
[[nodiscard]] const std::string &AffectMsg(EAffect affect, EAffectMsgType slot);

class AffectMessagesLoader : public cfg_manager::ICfgLoader {  // cfg id "affect_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

}  // namespace affects

template<>
const std::string &NAME_BY_ITEM<affects::EAffectMsgType>(affects::EAffectMsgType item);
template<>
affects::EAffectMsgType ITEM_BY_NAME<affects::EAffectMsgType>(const std::string &name);
template<>
const std::map<affects::EAffectMsgType, std::string> &NAMES_OF<affects::EAffectMsgType>();

#endif  // BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECT_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
