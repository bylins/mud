/**
\file spell_messages.h - a part of the Bylins engine.
\authors Created by Claude (issue #3304).
\brief In-game message container for spells.
\details Stores spell-related player messages in a msg_container::MsgContainer keyed
		 by ESpell (the spell id) and ESpellMsg (the per-spell message type). The XML
		 source is lib/cfg/spell_msg.xml, loaded through cfg_manager and exposed via
		 MUD::SpellMessages(). The default sheaf (XML id "kDefault") maps to
		 ESpell::kUndefined and supplies messages common to all spells.

		 Message text is stored WITHOUT a trailing "\r\n": act()-style messages do not
		 need it, and SendMsgToChar() call sites append it explicitly.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_MESSAGES_H_
#define BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_MESSAGES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/meta_enum.h"
#include "engine/structs/msg_container.h"
#include "spells_constants.h"

#include <string>

/**
 * Per-spell message types. The id type of the container is ESpell; this enum
 * identifies the kind of message inside a single spell's sheaf. Perspective is
 * encoded in the type name (ToChar/ToRoom/ToVict), matching the act() targets.
 * Grow this enum as more functions are migrated to the container.
 */
enum class ESpellMsg {
	kUndefined = 0,
	kPointsToVict,		// CastToPoints: effect felt by the target of a points/heal spell.
	kAreaToChar,		// CallMagicToArea: cast message to the caster.
	kAreaToRoom,		// CallMagicToArea: cast message to the room.
	kAreaToVict,		// CallMagicToArea: message to each affected target.
	// CastSpell cast-precondition messages (common to all spells -> kDefault sheaf,
	// overridable per spell). All shown to the caster.
	kCantCastSleeping,	// position: sleeping.
	kCantCastResting,	// position: resting.
	kCantCastSitting,	// position: sitting.
	kCantCastFighting,	// position: fighting.
	kCantCastPosition,	// position: other too-low position.
	kCantCastMaster,	// target is the caster's charmer.
	kCantCastSelfOnly,	// spell may only be cast on self.
	kCantCastNotSelf,	// spell may not be cast on self.
	kCantCastNotAlly,	// spell may only be cast on self or a groupmate (kTarAllyOnly).
	kCantCastNotMinion,	// spell may only be cast on the caster's own NPC follower (kTarMinionsOnly).
	kTargetUnavailable,	// spell target is unavailable.
	kCantCastPeaceful,	// caster is peaceful and refuses to do harm.
	// Generic "the spell had no effect" outcome, shown across CastAffect/CastUnaffects/
	// etc. (issue no-effect-msg). Lives in the kDefault sheaf; replaced the old global
	// NOEFFECT literal.
	kNoeffect,
	// CastToAlterObjs messages (to the caster).
	kObjResist,			// object is no-alter (common -> kDefault sheaf).
	kAlterObjToChar,	// per-spell "object transformed" message.
	kEnchantNotWeapon,	// kEnchantWeapon: target is not a weapon.
	kEnchantMagic,		// kEnchantWeapon: target is already magical.
	kEnchantSetItem,	// kEnchantWeapon: target is a set item.
	kEnchantMono,		// kEnchantWeapon: success, mono religion.
	kEnchantPoly,		// kEnchantWeapon: success, poly religion.
	kEnchantOther,		// kEnchantWeapon: success, no religion.
	kRemovePoisonUnknown,	// kRemovePoison: object has no prototype.
	kRemovePoisonRotten,	// kRemovePoison: contents have rotted.
	// CastSummon messages.
	kSummonToRoom,			// per-spell "summoned" message to the room (act).
	kSummonFail,			// random summon failure (kDefault variants; kClone overrides).
	kSummonNoCorpse,		// animate/resurrect with no suitable corpse (act).
	kSummonCharmed,			// caster is charmed and cannot summon.
	kSummonNoProto,			// summoned mob prototype is missing.
	kSummonWarhorse,		// target corpse was a warhorse.
	kResurrectBadCorpse,	// resurrection: corpse has no valid mob.
	kResurrectConsecrated,	// resurrection: mob is consecrated/protected.
	kResurrectNoPower,		// resurrection: mob cannot be controlled.
	kResurrectProtected,	// resurrection: mob has gods' shield.
	// CastDamage spell-keyed flavour (caster-centric ToChar/ToNotVict/ToVict).
	kDamageToChar,			// to the caster.
	kDamageToNotVict,		// to onlookers (not the victim).
	kDamageToVict,			// to the victim.
	kLaggedToRoom,			// lagged, to the room.
	kLaggedToChar,			// lagged, to the victim.
	kKnockdownToRoom,		// shared knockdown -> kDefault (to the room).
	kKnockdownToChar,		// shared knockdown -> kDefault (to the victim).
	kAcidCorrodeObj,		// kAcid: corrodes the victim's object.
	// Incantation phrase spoken when casting (SaySpell), chosen by religion.
	kCastPhraseHeathen,		// phrase for a heathen (poly) caster.
	kCastPhraseChristian,	// phrase for a Christian (mono) caster.
	// Combat damage messages (originally from lib/misc/messages, issue #3304); served
	// to the fight system via MUD::SpellMessages() since issue #3316.
	// Death/Miss/Hit/God x damager(ToChar)/damagee(ToVict)/onlookers(ToRoom).
	kFightDeathToChar, kFightDeathToVict, kFightDeathToRoom,
	kFightMissToChar, kFightMissToVict, kFightMissToRoom,
	kFightHitToChar, kFightHitToVict, kFightHitToRoom,
	kFightGodToChar, kFightGodToVict, kFightGodToRoom,
	// Affect imposition (CastAffect) and dispel (CastUnaffects) messages, issue #3335.
	// Keyed by the cast spell id; perspective is in the name (ToChar = the affected
	// char, ToRoom = onlookers). A spell with no such message omits the key (or stores
	// an empty val), so nothing is shown -- these are looked up sheaf-directly with no
	// kDefault fallback, so a missing key never leaks a default message.
	kAffImposedToRoom,		// CastAffect: affect landed, to the room.
	kAffImposedToChar,		// CastAffect: affect landed, to the affected char.
	kAffDispelledToRoom,	// CastUnaffects: affect removed, to the room.
	kAffDispelledToChar,	// CastUnaffects: affect removed, to the cured char.
};

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(ESpellMsg item);
template<>
ESpellMsg ITEM_BY_NAME<ESpellMsg>(const std::string &name);

namespace spells {

using SpellMessages = msg_container::MsgContainer<ESpell, ESpellMsg>;

/**
 * Loads/reloads lib/cfg/spell_msg.xml into MUD::SpellMessages().
 */
class SpellMessagesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

} // namespace spells

#endif // BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
