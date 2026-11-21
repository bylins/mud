/**
\file spell_messages.h - a part of the Bylins engine.
\authors Created by Claude.
\brief In-game message container for spells.
\details Stores spell-related player messages in a msg_container::MsgContainer keyed
		 by ESpell (the spell id) and ESpellMsg (the per-spell message type). The XML
		 source is lib/cfg/messages/ru/spell_msg.xml, loaded through cfg_manager and exposed
		 via MUD::SpellMessages(). The default sheaf (XML id "kDefault") maps to
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

#include <map>
#include <string>
#include <vector>

/**
 * Per-spell message types. The id type of the container is ESpell; this enum
 * identifies the kind of message inside a single spell's sheaf. Perspective is
 * encoded in the type name (ToChar/ToRoom/ToVict), matching the act() targets.
 */
enum class ESpellMsg {
	kUndefined = 0,
	// CastToPoints narration: one message per non-zero amount, emitted in
	// heal/moves/thirst/full order to the target via act() with kToChar. The
	// {intensity} placeholder substitutes from points_intensity.xml.
	kHealToVict,
	kMovesToVict,
	kThirstToVict,
	kFullToVict,
	kAreaToChar,		// area cast: cast message to the caster.
	kAreaToRoom,		// area cast: cast message to the room.
	kAreaToVict,		// area cast: message to each affected target.
	// CastSpell cast-precondition messages (common to all spells -> kDefault sheaf,
	// overridable per spell). All shown to the caster.
	kCantCastSleeping,
	kCantCastResting,
	kCantCastSitting,
	kCantCastFighting,
	kCantCastPosition,	// position: other too-low position.
	kCantCastMaster,	// target is the caster's charmer.
	kCantCastSelfOnly,
	kCantCastNotSelf,
	kCantCastNotAlly,	// spell may only be cast on self or a groupmate (kTarAllyOnly).
	kCantCastNotMinion,	// spell may only be cast on the caster's own NPC follower (kTarMinionsOnly).
	kTargetUnavailable,
	kCantCastPeaceful,	// caster is peaceful and refuses to do harm.
	// Generic "the spell had no effect" outcome (kDefault sheaf).
	kNoeffect,
	// CastUnaffects: the target had nothing matching the <unaffect> filter
	// (affect_flags/debuff_only) at all -- distinct from kNoeffect, which means
	// "something matched but the dispel roll failed", so players can tell "nothing
	// to cure" from "cure failed". Generic wording in kDefault; kRemovePoison overrides it.
	kUnaffectNoMatch,
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
	kKnockdownToRoom,		// shared knockdown -> kDefault (to the room).
	kKnockdownToChar,		// shared knockdown -> kDefault (to the victim).
	kAcidCorrodeObj,		// kAcid: corrodes the victim's object.
	// Incantation phrase spoken when casting (SaySpell), chosen by religion.
	kCastPhraseHeathen,		// phrase for a heathen (poly) caster.
	kCastPhraseChristian,	// phrase for a Christian (mono) caster.
	// Combat damage messages, rendered by SendSkillMessages for spell damage.
	// Death/Miss/Hit/God x damager(ToChar)/damagee(ToVict)/onlookers(ToRoom).
	kFightDeathToChar, kFightDeathToVict, kFightDeathToRoom,
	kFightMissToChar, kFightMissToVict, kFightMissToRoom,
	kFightHitToChar, kFightHitToVict, kFightHitToRoom,
	kFightGodToChar, kFightGodToVict, kFightGodToRoom,
	// Affect imposition (CastAffect) messages, keyed by the cast spell id; ToChar = the
	// affected char, ToRoom = onlookers. Looked up sheaf-directly with no kDefault
	// fallback, so a spell without the key shows nothing.
	kAffImposedToRoom,
	kAffImposedToChar,
	// Emitted by MaybeReflectToCaster when the original target reflects the spell back
	// at the caster ($n = caster, $N = original victim).
	kReflectedToChar,		// to the caster (whose spell bounced back).
	kReflectedToVict,		// to the original victim (who reflected the spell).
	kReflectedToRoom,		// to onlookers (not the original victim).
	// Cast forbidden: the spell fizzles before any effects run because the caster's
	// (or target room's) flags make casting impossible (ROOM_NOMAGIC, or e.g. kPeaceful
	// for kRuneLabel). Looked up in the cast spell's sheaf with kDefault fallback.
	kCastForbiddenToChar,
	kCastForbiddenToRoom,
	// Material-component narration, emitted by ProcessMatComponents around the
	// <components>/<material> requirement check. Looked up sheaf-directly on the cast
	// spell -- a missing key stays silent (no kDefault fallback).
	kComponentUse,				// per matched item, when the requirement is satisfied (act $o).
	kComponentMissing,			// when no item matches; the cast aborts.
	kComponentExhausted,		// when val(2)-- drops the item to 0 and it's destroyed.
	// Verbal-component fizzle: caster is under kSilence AND the spell carries <verbal/>.
	// Looked up on the cast spell with kDefault fallback.
	kCantCastSilenced,
	// FindCastTarget couldn't resolve the player's argument into a target the spell
	// accepts (or no argument was given and no default target applies). The default
	// message substitutes a {target} placeholder ("ЧТО" for object spells, "КОГО" for
	// char spells); warcry spells override with a louder variant, kControlWeather /
	// kCreateWeapon with their specific "what kind?" variants.
	kNoTarget,
	// MayCastHere refused (e.g. peaceful room + violent spell). Warcry spells override
	// the default with the air-shaking variant.
	kCantCastHereToChar,
	kCantCastHereToRoom,
	// The "Вы произнесли заклинание ..." / "Вы выкрикнули ..." line SaySpell prints to
	// the caster, with {color}/{name}/{nrm} placeholders substituted at emission. Warcry
	// spells override with the shouting variant.
	kCastIncantToChar,
	// CastCreation narration (kCreateFood / kCreateWater / kCreateLight / kCreateWeapon).
	// kItemNoPrototype fires when the world-objects factory can't build the obj (also
	// logged as SYSERR). kItemCreatedTo* are act()-style with $o = the new object.
	kItemNoPrototype,
	kItemCreatedToChar,
	kItemCreatedToRoom,
	// SaySpell room narration: one key per cast situation (caster targeting self /
	// another in room / object in room / something out of room; and the side narration
	// to a violent victim / a friendly target). Each carries a printf %s slot that
	// resolves to the spell's canonical name (for viewers who know the spell) or to the
	// cast phrase (everyone else). The kDefault sheaf carries several variants per key and
	// the container picks one at random per emission. kCastSaySound replaces all six
	// lines when the caster can't speak (!IsAbleToSay).
	kCastSayToSelf,
	kCastSayToOther,
	kCastSayToObj,
	kCastSayToSomething,
	kCastSayDamageeToVict,
	kCastSayHelpeeToVict,
	kCastSaySound,
	// Controlled-cast swap: looked up on the NEW spell's sheaf with its name in {name},
	// falling back to kDefault. (The interruption line is room_affects' kAffInterruptedToChar.)
	kCastPreparedToChar,
	// Generic "creation failed" notice; kCreateWater overrides it for the non-empty
	// non-water container case.
	kItemCreationFailToChar,
	// "Wrong target type for this spell"; kIdentify / kFullIdentify override with the
	// "magic can't identify another creature" variant.
	kWrongTarget,
	// Custom one-off slots for manual spells. These keys carry NO fixed semantic --
	// each per-spell sheaf assigns its own meaning. Use ONLY for messages that:
	//   1. are unique to a single manual spell (do_ManualSpell-style function),
	//   2. don't fit any existing semantically-named key, AND
	//   3. live in a spell slated for eventual migration to a Lua/Python scripting
	//      system.
	// Add a one-line XML comment on each per-spell usage describing what fires the
	// message -- the XML is the documentation source; the enum names stay anonymous
	// on purpose. Hitting the kCustomMsgTen cap on a single spell is a signal to
	// move that spell to scripts rather than extend the cap. Slots also carry room-affect
	// per-tick narration (one slot per tick phase). NEVER use these when a clean
	// semantically-named key fits (e.g. SpellCharm reuses kSummonWarhorse /
	// kResurrectConsecrated rather than hiding the semantic match behind kCustomMsgOne).
	kCustomMsgOne,
	kCustomMsgTwo,
	kCustomMsgThree,
	kCustomMsgFour,
	kCustomMsgFive,
	kCustomMsgSix,
	kCustomMsgSeven,
	kCustomMsgEight,
	kCustomMsgNine,
	kCustomMsgTen,
	// Teleport-style relocation narration: the actor ($n) leaves one room and enters
	// another. kDefault carries generic "$n исчез$q." / "$n появил$u." lines; per-spell
	// sheaves (kWorldOfRecall, kTeleport, kRelocate, kSummon, ...) flavour the wording.
	kCastDisappearToRoom,
	kCastAppearToRoom,
};

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(ESpellMsg item);
template<>
ESpellMsg ITEM_BY_NAME<ESpellMsg>(const std::string &name);
template<>
const std::map<ESpellMsg, std::string> &NAMES_OF<ESpellMsg>();

namespace spells {

using SpellMessages = msg_container::MsgContainer<ESpell, ESpellMsg>;

/**
 * Loads/reloads lib/cfg/messages/ru/spell_msg.xml into MUD::SpellMessages().
 */
class SpellMessagesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

} // namespace spells

#endif // BYLINS_SRC_GAMEPLAY_MAGIC_SPELL_MESSAGES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
