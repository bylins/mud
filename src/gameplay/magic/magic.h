/* ************************************************************************
*   File: magic.h                                     Part of Bylins      *
*  Usage: header: low-level functions for magic; spell template code      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#ifndef MAGIC_H_
#define MAGIC_H_

#include "spells_info.h"

#include <cstdlib>

class CharData;
class ObjData;
struct RoomData;

// The "first-aid removable" spell list (moved here from char_data.h, issue.affect-dispell-flags).
// The order doubles as the cure-difficulty index used by do_first_aid (prob/10 > index). This list
// is slated to be replaced by the kAfCurable flag + a potency-based cure mechanic.
enum ERemovableSpell {
	kRemAbstinent = 0,
	kRemPoison,
	kRemMadness,
	kRemWeakness,
	kRemSlowdown,
	kRemMindless,
	kRemColdWind,
	kRemFever,
	kRemCurse,
	kRemDeafness,
	kRemSilence,
	kRemBlindness,
	kRemSleep,
	kRemHold,
	kRemVacuum,
	kRemHaemorrhage,
	kRemBattle,
	kMaxFirstaidRemove
};

inline ESpell GetRemovableSpellId(int num) {
	static const ESpell spell[kMaxFirstaidRemove] = {
		ESpell::kAbstinent, ESpell::kPoison, ESpell::kMadness,
		ESpell::kWeaknes, ESpell::kSlowdown, ESpell::kMindless, ESpell::kColdWind,
		ESpell::kFever, ESpell::kCurse, ESpell::kDeafness, ESpell::kSilence,
		ESpell::kBlindness, ESpell::kSleep, ESpell::kHold, ESpell::kVacuum,
		ESpell::kHaemorrhage, ESpell::kBattle
	};
	if (num < kMaxFirstaidRemove) {
		return spell[num];
	}
	return ESpell::kUndefined;
}

// One evaluation of a talents_actions::Roll for a specific caster (issue #3333).
struct RollResult {
	int dices{0};              // talents_actions::Roll::RollSkillDices()
	double skill_coeff{0.0};   // talents_actions::Roll::CalcSkillCoeff(caster)
	double stat_coeff{0.0};    // talents_actions::Roll::CalcBaseStatCoeff(caster)
	double low_skill_coeff{0.0};  // talents_actions::Roll::CalcLowSkillCoeff(caster): the
	                              // low-skill bonus only, fed to SetBattleLag as skill_bonus.
};

// CastContext (issue.spell-pipeline): the single object threaded through the whole
// cast handler chain (CallMagic -> CastToSingleTarget -> the per-stage Cast* fns).
// It carries the cast parameters, the targets, and the accumulating results so that
// later <action>s can see what earlier ones did.
//
// Immutable parts (set once at construction, exposed via const getters): the caster,
// the spell, the base level and the two rolls -- these never change during a cast.
// Mutable parts (public fields): the current targets, the working `level` (which
// decays across area targets, starting from base_level()), and the action results.
class CastContext {
 public:
	CastContext(CharData *caster, ESpell spell_id, int level,
				const RollResult &success, const RollResult &potency)
		: level{level},
		  caster_{caster}, spell_id_{spell_id}, base_level_{level},
		  success_{success}, potency_{potency} {}

	// --- Immutable cast parameters ---
	[[nodiscard]] CharData *caster() const { return caster_; }
	[[nodiscard]] ESpell spell_id() const { return spell_id_; }
	[[nodiscard]] int base_level() const { return base_level_; }
	[[nodiscard]] const RollResult &success() const { return success_; }
	[[nodiscard]] const RollResult &potency() const { return potency_; }

	// --- Mutable working state ---
	// Working level: starts at base_level, decays per target in area casts.
	int level{0};
	// Results accumulated by the stage handlers, so later <action>s (and the
	// dispatcher) can read what earlier ones produced.
	struct ActionResult {
		// Per-cast damage from the last CastDamage: >=0 dealt; -1 means the target
		// died on this cast (kept as a sentinel -- see is_vict_dead for the boolean).
		long damage{0};
	};
	ActionResult result;
	// Set by CastDamage when the target died; the dispatcher stops the action chain.
	bool is_vict_dead{false};
	// Targets for the current cast. cvict will later become a richer target list
	// built by the target resolver (issue.spell-pipeline note).
	CharData *cvict{nullptr};
	ObjData  *ovict{nullptr};
	RoomData *rvict{nullptr};

 private:
	CharData *caster_{nullptr};
	ESpell spell_id_{ESpell::kUndefined};
	int base_level_{0};
	RollResult success_;        // from SpellInfo::GetSuccessRoll()
	RollResult potency_;        // from SpellInfo::GetPotencyRoll()
};

// VNUM'О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
const int kMobDouble = 3000; //О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
const int kMobSkeleton = 3001;
const int kMobZombie = 3002;
const int kMobBonedog = 3003;
const int kMobBonedragon = 3004;
const int kMobBonespirit = 3005;
const int kMobNecrodamager = 3007;
const int kMobNecrotank = 3008;
const int kMobNecrobreather = 3009;
const int kMobNecrocaster = 3010;
const int kLastNecroMob = 3010;
// О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
const int kMobMentalShadow = 3020;
const int kMobKeeper = 3021;
const int kMobFirekeeper = 3022;

const int kMaxSpellAffects = 16; // change it if you need more

bool IsRoomForbidden(RoomData *room);
void mobile_affect_update();
void player_affect_update();
void print_rune_log();
void ShowAffExpiredMsg(ESpell aff_type, CharData *ch);

// Outcome of a whole cast (the CallMagic / CastSpell / CastToSingleTarget chain).
// Replaces the old int return whose -1/0/1 meaning callers had to memorise.
enum class ECastResult {
	kNotCast,     // the cast did not take effect: blocked, fizzled, refused, no target.
	kSuccess,     // the cast took effect and the target is still alive.
	kTargetDied,  // the cast took effect and killed the target (don't re-cast at it).
};

// True if the cast produced an effect (alive OR dead target) -- the old `>= 0` ... note
// that the legacy idioms differed, so callers compare ECastResult directly; these helpers
// exist only where a plain "did anything happen" / "did the target die" check reads cleaner.
[[nodiscard]] inline bool CastTookEffect(ECastResult r) { return r != ECastResult::kNotCast; }
[[nodiscard]] inline bool CastTargetDied(ECastResult r) { return r == ECastResult::kTargetDied; }

ECastResult CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);
ECastResult CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Result of one cast stage (CastAffect/CastUnaffects/...). CastToSingleTarget runs the
// stages in order and stops the chain on kBreak. Today every stage returns kSuccess
// (behaviour unchanged); per-spell kBreak conditions are to be driven from spells.xml later.
// The list may grow.
enum class EStageResult {
	kBreak,		// stop the cast: skip the remaining stages.
	kSuccess	// stage handled; continue to the next stage.
};

// Stage functions reachable from outside the magic module. Prefer CallMagic / CastSpell --
// direct calls bypass CastToSingleTarget's blocking/required/reflection/caster_blocking gates,
// which is rarely what callers want. The two kept here have real external callers:
// fight_hit.cpp / spells.cpp run a per-target loop (CastDamage), and handler.cpp re-applies
// gear-borne effects (CastAffect). Everything else that used to live here -- CastUnaffects,
// CallMagicToArea/Group, ComputeCastRoll, ProcessMatComponents, CalcClassAntiSavingsMod, and
// the CastToPoints/CastToAlterObjs/CastCreation/CastSummon/CastManual/CastToSingleTarget
// dispatchers -- is module-internal and now lives in magic_internal.h.
EStageResult CastDamage(CastContext &ctx);
EStageResult CastAffect(CastContext &ctx);
int CalcSaving(CharData *killer, CharData *victim, ESaving saving, bool need_log = false);
int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply);
int GetBasicSave(CharData *ch, ESaving saving, bool log = false);

#endif // MAGIC_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
