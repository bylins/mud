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

// issue.character-affect-triggers: forward-decl so AffectActions can return it without pulling in the
// (heavy) talents_actions.h here.
namespace talents_actions { class Actions; }

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
	// issue.attack-ward: flavor shown when this affect reflects/absorbs an incoming attack (RunAttackWards).
	// $n = the attacker, $N = the warded (affect-bearing) char.
	kWardToChar,          // to the attacker (their cast was reflected/absorbed)
	kWardToVict,          // to the warded char (their affect reacted)
	kWardToRoom,          // to onlookers
	// issue.character-affect-triggers: flavor for damage dealt by this affect's own <damage> actions
	// (e.g. a kDispell trap, a DoT). When set, it FULLY REPLACES the generic combat hit + severity line.
	// $n = the char dealing/taking the damage (for affect self-damage they are the same), $N = the victim.
	kDamageToChar,        // to the char taking the affect's damage
	kDamageToVict,        // to a distinct victim (only when dealer != victim; self-damage omits it)
	kDamageToRoom,        // to onlookers
	// issue.character-affect-triggers: flavor shown when this affect's kDeath trigger fires (the bearer is
	// dying). Optional -- an empty slot shows nothing. $n = the dying bearer.
	kDeathToChar,         // to the dying bearer (e.g. "the curse tears you back from death")
	kDeathToRoom,         // to onlookers
	// issue.damage-change: flavor shown when this affect's <damage_change> (kWardDamage) transforms an
	// incoming attack -- e.g. a shield softening the blow. Optional; an empty slot shows nothing.
	// The affect BEARER is the damage victim ($N); the attacker is $n. Emitted with kToNoBriefShields.
	kTransformToChar,     // to the attacker (their hit was reduced/altered by the bearer's affect)
	kTransformToVict,     // to the affect bearer (their affect reacted to the incoming damage)
	kTransformToRoom,     // to onlookers
	// issue.damage-change: alternate transform flavor, selected by <damage_change msg="crit">. Lets one
	// affect carry two messages -- e.g. the ice shield's normal "softened the blow" (kTransform*) vs its
	// crit-absorb "the precise hit sank into the icy veil" (kTransformCrit*).
	kTransformCritToChar,
	kTransformCritToVict,
	kTransformCritToRoom,
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
[[nodiscard]] const talents_actions::Actions &AffectActions(EAffect affect_type);   // issue.character-affect-triggers: per-affect pulse <actions>
[[nodiscard]] bool AffectFlagsLoaded();   // issue.affect-migration: affects.xml flags loaded yet?

// issue.affect-migration: an affect's intrinsic buff classification -- the affect-side analog of a
// spell's <misc violent>. kYes = a buff (helpful), kNo = a debuff (harmful), kAmbiguous = depends on
// context. Declared per affect via <affect buff="Y|N|A"> in affects.xml; kAmbiguous when absent.
enum class EBuff { kNo, kYes, kAmbiguous };
[[nodiscard]] EBuff AffectBuffKind(EAffect affect_type);

// issue.damage-change: the affect's magic-shield selection weight from affects.xml (<shield weight=>).
// > 0 marks a mutually-exclusive elemental shield; 0 = not a shield. Used to weighted-randomly pick the
// one shield that reduces a given hit.
[[nodiscard]] int AffectShieldWeight(EAffect affect_type);
// issue.random-noise-rework: additive dispel-threshold modifier (percentage points, 0 = neutral).
[[nodiscard]] int AffectDispelMod(EAffect affect_type);

// issue.mob-flag-affect-materialization: affect types flagged kAfMaterialize, cached at load. These are
// the buffs realized as real (dispellable, duration=-1) affects on a flag-only NPC when its zone wakes.
[[nodiscard]] const std::vector<EAffect> &MaterializableAffects();

// issue.damage-change: affect types flagged kAfFullAbsorb -- grant total damage immunity. The engine's
// total-immunity block tests whether the victim has any of these AFF flags set (works for cast, worn and
// flag-only holders alike), instead of hard-coding the kGodsShield affect id.
[[nodiscard]] const std::vector<EAffect> &FullAbsorbAffects();

// issue.affects-improve (P2): one stat-change an affect imposes -- its location (EApply) plus the
// modifier-formula coefficients (same shape as a spell's <apply><modifier>). The affect, not the
// spell, owns these; the spell/skill supplies competence/dice/duration. Parsed from affects.xml
// <apply> children. (Built in P2; the impose path starts reading it in P3.)
struct AffectApply {
	EApply location{EApply::kNone};
	double min{0.0};
	double beta{0.0};
	double weight{0.0};  // issue.potency-noise: weight on the spell's shared noise draw (0 = deterministic)
	int factor{1};
	int cap{0};
	int stack{1};
	bool random{false};
};
[[nodiscard]] const std::vector<AffectApply> &AffectApplies(EAffect affect_type);

[[nodiscard]] std::string DescribeActive(const BitsetFlags<EAffect> &flags, const char *div);
[[nodiscard]] bool FindByShortDesc(const std::string &name, EAffect &out);
[[nodiscard]] bool MessagesLoaded();   // affect_messages cfg loaded? (boot guard)
[[nodiscard]] const std::vector<EAffect> &MenuOrder();   // ordered affects for the OLC editor

class AffectMessagesLoader : public cfg_manager::IEditableCfgLoader {  // cfg id "affect_messages"
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	// issue.unstable-hotfixes: in-game editing of affect messages (`vedun affectmsg`) -- an affect's
	// kShortDesc / kLook / lifecycle narration. Keyed by the <msg_sheaf id=> (EAffect token; "kDefault"
	// for the shared sheaf). Mirrors SpellMessagesLoader.
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
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
