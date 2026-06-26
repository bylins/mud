/**
 \brief issue.ext-affects: affect short names + "look at character" aura text (cfg/messages/ru/
		affect_msg.xml), keyed by affect.
 \details A MsgContainer<EAffect, EAffectMsgType>: one sheaf per affect (kShortDesc + kLook[+kLookPoly])
		  plus the shared "kDefault" sheaf for the merged shield/aura group scaffolding (the line frames
		  + the count nouns). affected_bits is rebuilt from each affect's kShortDesc. Composition (which
		  affects, grouping, gender/poly/count selection) stays in code; the words and the line structure
		  live here. The EAffect enum itself stays in C++ (it is the identity, used in AFF_FLAGGED/switch).
*/

#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECT_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_AFFECT_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "affect_contants.h"   // EAffect

#include <map>
#include <vector>
#include <string>

namespace affects {

// Slots within an affect's sheaf. $a in any value is the act gender suffix of the looked-at character.
enum class EAffectMsgType {
	kUndefined = 0,
	kShortDesc,        // per-affect: short display name (affected_bits / score / identify)
	kLook,             // per-affect: ListOneChar aura -- a complete "...line" for standalone affects
	                   //             (cocoon/glow/status), or a bare list fragment for grouped ones
	                   //             (shields/auras) that the frame below joins via {list}
	kLookPoly,         // per-affect: poly (multi-form creature) variant of kLook
	kShieldFrame,      // kDefault: elemental-shield line frame; fmt template with {list} and {noun}
	kShieldNoun,       // kDefault: shield count-noun, singular  (one shield)
	kShieldNounMany,   // kDefault: shield count-noun, plural    (several)
	kAuraFrame,        // kDefault: detect-magic aura line frame; fmt template with {list} and {noun}
	kAuraNoun,         // kDefault: aura count-noun, singular
	kAuraNounMany,     // kDefault: aura count-noun, plural
	// Affect lifecycle messages (issue.affect-migration): moved here from ESpellMsg -- they belong
	// to the AFFECT, not the casting spell. CastAffect impose / CastUnaffects dispel / natural expiry
	// look these up by affect_type (with a transitional spell-message fallback for kUndefined affects).
	kAffImposedToRoom,    // CastAffect: affect landed, to the room.
	kAffImposedToChar,    // CastAffect: affect landed, to the affected char.
	kAffImposedToVict,    // impose SUCCESS, to the external target/opponent ($N; only when set & != affected).
	kAffImposeFailToChar,   // impose FAILURE (affect carries kAfFailed), to the affected char.
	kAffImposeFailToVict,   // impose FAILURE, to the external target/opponent ($N).
	kAffImposeFailToRoom,   // impose FAILURE, to the room.
	kAffDispelledToRoom,  // CastUnaffects: affect removed, to the room.
	kAffDispelledToChar,  // CastUnaffects: affect removed, to the cured char.
	kAffExpiredToChar,    // affect wore off naturally, to the affected char.
	kAffExpiredToRoom,    // affect wore off, to the room (only for affects whose expiry shows).
	kAffExpireSoon,       // mob affect ~1 min from expiry, to the room (mobile_affect_update heads-up).
};

// affect = the affect whose sheaf to read (EAffect::kUndefined => the shared "kDefault" sheaf).
[[nodiscard]] const std::string &AffectMsg(EAffect affect, EAffectMsgType slot);
// Sheaf-direct variant: returns the affect's own message or empty (NO kDefault/error fallback).
// Use where a missing message must stay silent (affect imposition narration).
[[nodiscard]] const std::string &AffectMsgRaw(EAffect affect, EAffectMsgType slot);
// Like AffectMsgRaw but for the armed/unarmed split: among the affect's variants of `slot`, when
// has_weapon prefer ones using $o (the weapon flourish), else fall back to non-$o; when unarmed
// return only non-$o variants (so $o never renders without a weapon). Empty if none apply.
[[nodiscard]] const std::string &AffectMsgWeapon(EAffect affect, EAffectMsgType slot, bool has_weapon);

// Direct affect-system queries (replacing the legacy affected_bits[] projection):
[[nodiscard]] EAffect AffectByIndex(std::size_t flat_index);   // set-bit index -> EAffect
[[nodiscard]] Bitvector AffectFlagsByType(EAffect affect_type);   // issue.affect-migration: per-affect behavior flags from affects.xml (0 if none)
[[nodiscard]] bool AffectFlagsLoaded();   // issue.affect-migration: affects.xml flags loaded yet?

// issue.affect-migration: an affect's intrinsic buff classification -- the affect-side analog of a
// spell's <misc violent>. kYes = a buff (helpful), kNo = a debuff (harmful), kAmbiguous = depends on
// context. Declared per affect via <affect buff="Y|N|A"> in affects.xml; kAmbiguous when absent.
enum class EBuff { kNo, kYes, kAmbiguous };
[[nodiscard]] EBuff AffectBuffKind(EAffect affect_type);

[[nodiscard]] std::string DescribeActive(const BitsetFlags<EAffect> &flags, const char *div);
[[nodiscard]] bool FindByShortDesc(const std::string &name, EAffect &out);
[[nodiscard]] bool MessagesLoaded();   // affect_messages cfg loaded? (boot guard)
[[nodiscard]] const std::vector<EAffect> &MenuOrder();   // ordered affects for the OLC editor

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
