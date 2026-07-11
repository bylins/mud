#ifndef BYLINS_OBJ_AFFECTS_H
#define BYLINS_OBJ_AFFECTS_H

// issue.obj-affects: affects that live ON an item (formerly the misnamed "TimedSpell on objects").
//
// These are NOT character affects. An affect on an item behaves differently from the same-named affect
// on a character: poison on a weapon poisons the strike target (it is not a damage-over-time on the
// item); bless/curse on an item change the item, not a creature. So obj affects carry their own
// identity enum (EObjAffect), independent of ESpell and of EAffect.
//
// The mechanic is materialized like every other affect in the engine (see Affect<> in affect_data.h and
// the room-affect subsystem in magic_rooms.h): an obj affect is a real Affect<EObjApply> with the full
// parameter set (duration -1 or computed, modifier, caster_id, potency, ...), stored in a per-item list.
//
// Each obj affect optionally OWNS an object extra-flag (EObjFlag), declared in cfg/affects/obj_affects.xml. On
// impose the affect sets that flag on the item instance; on expiry/dispel it is cleared ONLY if the
// item's PROTOTYPE does not carry the flag innately (an innate flag is a permanent property of the item
// and must survive). The flag is a separate entity from the affect: consumers keep reading the flag.

#include "gameplay/affects/affect_data.h"          // Affect<>, AffectFlagType
#include "engine/entities/entities_constants.h"    // EObjFlag
#include "gameplay/magic/spells.h"                 // ESpell (legacy on-disk migration + poison subtype)
#include "engine/structs/meta_enum.h"              // NAME_BY_ITEM / ITEM_BY_NAME / NAMES_OF
#include "utils/grammar/cases.h"                    // grammar::ECase (item-name case in messages)

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

class ObjData;
class CharData;
namespace parser_wrapper { class DataNode; }
namespace talents_actions { class Actions; }

namespace obj_affects {

// Obj-affect identity. Index-valued (like EAffect / ERoomAffect): kUndefined = 0 is the "no affect"
// sentinel, real affects are 1-based, kCount is the terminal counter.
enum class EObjAffect : Bitvector {
	kUndefined = 0,
	kInvisible = 1,		// item is invisible          (flag kInvisible)
	kCurse = 2,			// item is cursed / nodrop     (flag kNodrop)
	kPoisoned = 3,		// weapon is poisoned          (no flag; subtype poison ESpell in modifier)
	kBless = 4,			// item is blessed             (flag kBless)
	kFly = 5,			// item floats / does not sink (flag kFlying)
	kLight = 6,			// item glows                  (flag kGlow)
	kDartTrap = 7,		// a trap: on pick/open it blocks the action, damages + poisons the actor (no flag)
	kSuppressed = 8,	// an equipment affect this item confers is dispel-suppressed (no flag, not player-
						// dispellable): modifier = to_underlying(EAffect suppressed), duration = remaining game-hours
	kCount = 9,
};

// Apply-location placeholder for Affect<EObjApply>. Obj affects do not use EApply-style stat
// modifiers, but Affect<> is parameterized by a location kind, so provide the minimal one (mirrors
// room_spells::ERoomApply). kNone == 0 so Affect's default ctor is well-formed.
enum EObjApply {
	kNone = 0
};

}  // namespace obj_affects

// Obj affects store an EObjAffect in Affect::affect_type (see affect_data.h / AffectFlagType).
template<>
struct AffectFlagType<obj_affects::EObjApply> {
	using type = obj_affects::EObjAffect;
};

namespace obj_affects {

using ObjAffect = Affect<EObjApply>;
using ObjAffects = std::list<ObjAffect::shared_ptr>;
using ObjAffectIt = ObjAffects::iterator;

// --- per-affect registry (cfg/affects/obj_affects.xml, filled by ObjAffectsLoader; built-in defaults until) ---
// The extra-flag this affect owns while active. Returns false / kGlow(unused) when the affect owns no
// flag (e.g. kPoisoned). Consumers should gate on HasFlag first.
[[nodiscard]] bool HasFlag(EObjAffect affect);
[[nodiscard]] EObjFlag Flag(EObjAffect affect);
// Whether a player effect (remove curse / dispel / darkness) may strip this affect. (Innate prototype
// flags are handled separately by the prototype check on removal, not by this bit.)
[[nodiscard]] bool Dispellable(EObjAffect affect);
// Grammatical case the affect's lifecycle messages expect the item name ("%s") to be in.
[[nodiscard]] grammar::ECase MsgCase(EObjAffect affect);
// The affect the inspecting character must carry to SEE this affect on a normal examine (kUndefined =
// always visible, e.g. fly/light). Immortals and the god `stat` command bypass the gate.
[[nodiscard]] EAffect SeeAffect(EObjAffect affect);
// The affect's own <actions> (each gated by its <trigger>), parsed from cfg/affects/obj_affects.xml; empty when
// the affect declares none. Mirrors room_spells::RoomAffectActions. Fired by the trigger dispatchers.
[[nodiscard]] const talents_actions::Actions &ObjAffectActions(EObjAffect affect);
// Rebuild the registry from a parsed cfg/affects/obj_affects.xml document (see ObjAffectsLoader).
void BuildRegistry(parser_wrapper::DataNode data);

// --- legacy on-disk migration (old obj save format stored ESpell in the "TSpl" tag) -------------------
// Map an old TimedSpell ESpell to its obj-affect identity (kUndefined if the spell was never an
// item affect). The 4 poison spells all map to kPoisoned; the caster stores the specific poison in the
// affect modifier so on-hit combat keeps its distinct effect.
[[nodiscard]] EObjAffect BySpell(ESpell spell);
[[nodiscard]] bool IsPoisonSpell(ESpell spell);

// --- runtime (obj_affects.cpp) -----------------------------------------------------------------------
// Impose/refresh an obj affect on the item. duration: -1 = permanent, > 0 = timer in minutes (the
// obj-affect tick cadence, one per MUD hour). modifier: for kPoisoned the poison ESpell's underlying
// value (its combat subtype); 0 for the flag affects. Sets the affect's owned flag on the instance and
// registers a timed affect for ticking. Re-imposing an existing affect refreshes it; imposing any
// poison first clears an existing poison (one poison per weapon).
void Impose(ObjData *obj, EObjAffect type, int duration, int modifier = 0,
			long caster_id = 0, float potency = 0.0f, int charges = -1);

// Remove a specific obj affect. message = emit the wear-off / dispel line to the holder. Clears the
// affect's owned flag ONLY if the item prototype lacks it (an innate prototype flag survives).
void Remove(ObjData *obj, EObjAffect type, bool message);

// Tick every timed obj affect down by `minutes`, expiring (with message) any that reach <= 0.
// tick_suppressions=false skips kSuppressed affects (they pause while offline -- "N hours of PLAY");
// the offline catch-up in ObjData::dec_timer passes false, the online periodic tick passes true.
void Tick(ObjData *obj, int minutes = 1, bool tick_suppressions = true);

// Spend one trigger charge on an affect after its triggered action ran. charges == -1 (the default) is
// unlimited -> no-op; at 0 the affect is consumed (removed like an expiry). Used by the trigger dispatch.
void SpendCharge(ObjData *obj, EObjAffect type);

// Queries.
[[nodiscard]] ObjAffectIt Find(ObjData *obj, EObjAffect type);
[[nodiscard]] bool Has(const ObjData *obj, EObjAffect type);
// The poison ESpell on a weapon (kUndefined if none) -- the kPoisoned affect's combat subtype.
[[nodiscard]] ESpell PoisonSpell(const ObjData *obj);

// --- equipment-affect suppression (issue.obj-suppressor-affect) ------------------------------------
// A dispel that lands on an equipment-conferred affect drops it on the source item for a time, then it
// auto-returns. Modelled as one kSuppressed obj-affect per suppressed EAffect (modifier = the EAffect,
// duration = game-hours), so it serializes/ticks with every other obj-affect. Multi-instance is keyed by
// modifier -- kept separate from the one-per-type Impose/Find used by the flag affects.
// Add/refresh a suppression of `aff` for `hours` game-hours (no-op for hours <= 0).
void SuppressEquipAffect(ObjData *obj, EAffect aff, int hours);
// Whether `aff` is currently suppressed on this item.
[[nodiscard]] bool IsEquipAffectSuppressed(const ObjData *obj, EAffect aff);
// (EAffect, remaining game-hours) for every suppression on the item (for identify display).
[[nodiscard]] std::vector<std::pair<EAffect, int>> SuppressedEquipAffects(const ObjData *obj);

// Examine/identify diagnostic text for all affects on the item (one line per affect, timer appended).
// `viewer` gates which affects are shown (see SeeAffect); pass nullptr to show them all (god stat).
[[nodiscard]] std::string Diag(const ObjData *obj, const CharData *viewer);

// Serialization: one "OAff: <EObjAffect token> <timer> <modifier>" line per affect (+ a legacy "TSpl:
// <ESpell> <timer>" reader in obj_save.cpp maps old saves via BySpell). Empty string if none.
[[nodiscard]] std::string Serialize(const ObjData *obj);

}  // namespace obj_affects

// meta_enum string maps (definitions in obj_affects.cpp).
template<>
const std::string &NAME_BY_ITEM<obj_affects::EObjAffect>(obj_affects::EObjAffect item);
template<>
obj_affects::EObjAffect ITEM_BY_NAME<obj_affects::EObjAffect>(const std::string &name);
template<>
const std::map<obj_affects::EObjAffect, std::string> &NAMES_OF<obj_affects::EObjAffect>();

#endif  // BYLINS_OBJ_AFFECTS_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
