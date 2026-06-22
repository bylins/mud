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

#include <map>
#include <string>
#include <vector>

/**
 * Per-spell message types. The id type of the container is ESpell; this enum
 * identifies the kind of message inside a single spell's sheaf. Perspective is
 * encoded in the type name (ToChar/ToRoom/ToVict), matching the act() targets.
 * Grow this enum as more functions are migrated to the container.
 */
enum class ESpellMsg {
	kUndefined = 0,
	// CastToPoints per-category narration (issue.point-bugs #2): one message per
	// non-zero amount, emitted in heal/moves/thirst/full order to the target via
	// act() with kToChar. The {intensity} placeholder substitutes from points_intensity.xml.
	// Replaced the old composite kPointsToVict key.
	kHealToVict,
	kMovesToVict,
	kThirstToVict,
	kFullToVict,
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
	kAffExpiredToChar,		// Affect wore off naturally, to the affected char.
	kAffExpiredToRoom,		// Affect wore off, to the room (only for affects whose expiry shows to others).
	// Reflection (issue.cast-dmg-migration): emitted by CastToSingleTarget when the original
	// target reflects the spell back at the caster ($n = caster, $N = original victim).
	kReflectedToChar,		// reflection: to the caster (whose spell bounced back).
	kReflectedToVict,		// reflection: to the original victim (who reflected the spell).
	kReflectedToRoom,		// reflection: to onlookers (not the original victim).
	// Cast forbidden (issue.room-affects): the spell fizzles before any effects run because the
	// caster's (or target room's) flags make casting impossible (ROOM_NOMAGIC, or e.g. kPeaceful
	// for kRuneLabel). Looked up in the cast spell's sheaf with kDefault fallback.
	kCastForbiddenToChar,	// fizzle: to the caster.
	kCastForbiddenToRoom,	// fizzle: to onlookers.
	// Room-affect description (issue.sight-fmt): per-spell room-aura text emitted by
	// show_room_affects when listing a room's active room-affects to a viewer. Three
	// variants per spell, picked by viewer state. Lookup is sheaf-direct with a chained
	// fallback Self->Invisible->Visible; missing keys stay silent (no kDefault fallback).
	kRoomAffectVisible,			// shown when the viewer lacks kDetectMagic; the "physical" trace.
	kRoomAffectInvisible,		// shown when the viewer has kDetectMagic and is NOT the caster.
	kRoomAffectSelfInvisible,	// shown when the viewer has kDetectMagic and IS the caster.
	// Pk override (issue.affect-flags): preferred over the regular kRoomAffect* keys
	// when the affect carries a non-zero pk_unique (set by SpellPortal when pkPortal
	// fires). Looked up before the regular chain; empty result falls back to it.
	// Used today by kPortalTimer to surface its blood-tinge variant.
	kRoomAffectPkVisible,		// pk variant: viewer lacks kDetectMagic.
	kRoomAffectPkInvisible,		// pk variant: viewer has kDetectMagic.
	// Material-component narration (issue.spellcomponents): emitted by
	// ProcessMatComponents around the <components>/<material> requirement
	// check. Looked up sheaf-directly on the cast spell -- a missing key
	// stays silent (no kDefault fallback), matching the kAffImposed*/
	// kAffDispelled* convention.
	kComponentUse,				// per matched item, when the requirement is satisfied (act $o).
	kComponentMissing,			// when no item matches; the cast aborts.
	kComponentExhausted,		// when val(2)-- drops the item to 0 and it's destroyed.
	// Verbal-component fizzle (issue.spellcomponents): caster is under kSilence
	// AND the spell carries <verbal/>. Looked up sheaf-direct on the cast spell
	// with kDefault fallback; default sheaf supplies the generic line.
	kCantCastSilenced,
	// Target-resolution narration (issue.spell-msg-improve): FindCastTarget couldn't
	// resolve the player's argument into a target the spell accepts (or the player
	// provided no argument and no default target applies). The default message
	// substitutes a {target} placeholder ("ЧТО" for object spells, "КОГО" for char
	// spells); warcry spells override with a louder variant, kControlWeather /
	// kCreateWeapon override with their specific "what kind?" variants.
	kNoTarget,
	// Cast-here narration (issue.spell-msg-improve): MayCastHere refused (e.g. peaceful
	// room + violent spell). Default key has the "magic dissolved into a flash" line;
	// warcry spells override with the air-shaking variant. ToChar/ToRoom mirror the
	// kCastForbidden pair pattern.
	kCantCastHereToChar,
	kCantCastHereToRoom,
	// Caster-side cast incantation banner (issue.spell-msg-improve): the "Вы произнесли
	// заклинание ..." / "Вы выкрикнули ..." line SaySpell prints to the caster, with
	// {color}/{name}/{nrm} placeholders substituted at emission. Warcry spells override
	// with the shouting variant.
	kCastIncantToChar,
	// CastCreation narration (issue.spell-msg-improve): item-conjuring spells (kCreateFood
	// / kCreateWater / kCreateLight / kCreateWeapon). kItemNoPrototype fires when the
	// world-objects factory can't build the obj (logged as SYSERR too). kItemCreatedTo*
	// are act()-style with $o = the new object; the kDefault sheaf carries generic text,
	// per-spell overrides can flavour the narration.
	kItemNoPrototype,
	kItemCreatedToChar,
	kItemCreatedToRoom,
	// SaySpell room narration (issue.spell-msg-improve): 6 per-situation keys covering
	// the cases SaySpell distinguishes (caster targeting self / another in room / object
	// in room / something out of room; and the side narration to a violent victim
	// / a friendly target). Each carries a printf %s slot that resolves to the spell's
	// canonical name (for viewers who Know the spell) or to the cast phrase (everyone
	// else). The kDefault sheaf carries TWO variants per key -- one PC-flavoured,
	// one humanoid-NPC-flavoured -- and the container picks one at random per
	// emission, so the verbal voice is mixed and stylistically varied. Per-spell
	// overrides can pin a specific line. kCastSaySound replaces all six lines when
	// the caster's race can't speak (default branch in SaySpell's NPC race switch).
	kCastSayToSelf,
	kCastSayToOther,
	kCastSayToObj,
	kCastSayToSomething,
	kCastSayDamageeToVict,
	kCastSayHelpeeToVict,
	kCastSaySound,
	// Controlled-cast swap narration (issue.spell-msg-improve): kMagNeedControl spells
	// drop the existing controlled affect before imposing their own. kCastInterruptedToChar
	// is looked up on the OLD spell's sheaf with the OLD spell's name in {name};
	// kCastPreparedToChar is looked up on the NEW spell's sheaf with the NEW spell's
	// name in {name}. Both fall back to kDefault.
	kCastInterruptedToChar,
	kCastPreparedToChar,
	// Owner notification when one of the caster's room/world affects gets removed
	// outside the dispel pipeline (issue.spell-msg-improve). RemoveSingleAffectFromWorld
	// is the current caller (kRuneLabel expiry on idle-quit, etc.). The kDefault sheaf
	// carries a generic "Ваша магия была развеяна" line; per-spell overrides flavour
	// the message (kRuneLabel says "Ваша рунная метка удалена.").
	kAfDispelledToOwner,
	// kCreateWater non-water-container reject (issue.spell-msg-improve): a generic
	// "creation failed" notice. Default kDefault sheaf carries the placeholder line;
	// kCreateWater overrides with "Прекратите, ради бога, химичить." for the
	// non-empty non-water container case. Other creation spells may override later.
	kItemCreationFailToChar,
	// "Wrong target type for this spell" (issue.spell-msg-improve). kDefault sheaf
	// carries a generic line; per-spell sheaves override (kIdentify / kFullIdentify
	// supply the "magic can't identify another creature" variant today).
	kWrongTarget,
	// Custom one-off slots for manual spells (issue.spell-msg-improve). These keys
	// carry NO fixed semantic -- each per-spell sheaf assigns its own meaning. Use
	// ONLY for messages that:
	//   1. are unique to a single manual spell (do_ManualSpell-style function),
	//   2. don't fit any existing semantically-named key, AND
	//   3. live in a spell slated for eventual migration to a Lua/Python scripting
	//      system.
	// Add a one-line XML comment on each per-spell usage describing what fires the
	// message -- the XML is the documentation source; the enum names stay anonymous
	// on purpose. Hitting the kCustomMsgTen cap on a single spell is a signal to
	// move that spell to scripts rather than extend the cap. Slots also carry room-affect
	// per-tick narration (cycled by Affect::apply_time -- one slot per tick phase). NEVER use these when
	// a clean semantically-named key fits (the SpellCharm migration that reused
	// kSummonWarhorse / kResurrectConsecrated etc. is the right pattern; reaching
	// for kCustomMsgOne would have hidden the semantic match).
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
	// Teleport-style relocation narration (issue.spell-msg-improve): shared by spells
	// where an actor leaves one room ($n is the actor leaving) and enters another
	// ($n is the actor arriving). kDefault carries generic "$n исчез$q." / "$n
	// появил$u." lines; per-spell overrides flavour the wording for kWorldOfRecall,
	// kTeleport, kRelocate, kSummon, and any future teleport-class spell.
	kCastDisappearToRoom,
	kCastAppearToRoom,
};

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(ESpellMsg item);
template<>
ESpellMsg ITEM_BY_NAME<ESpellMsg>(const std::string &name);
template<>
const std::map<ESpellMsg, std::string> &NAMES_OF<ESpellMsg>();  // issue.vedun-msg-editor

namespace spells {

using SpellMessages = msg_container::MsgContainer<ESpell, ESpellMsg>;

/**
 * Loads/reloads lib/cfg/spell_msg.xml into MUD::SpellMessages().
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
