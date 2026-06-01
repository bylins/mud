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

// Builds a CastContext for spell_id cast by caster at level (evaluates both rolls).
// Targets stay null here; callers fill cvict/ovict/rvict. Defined in magic_utils.cpp.
CastContext ComputeCastRoll(CharData *caster, ESpell spell_id, int level);

// VNUM'ы мобов для заклинаний, создающих мобов
const int kMobDouble = 3000; //внум прототипа для клона
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
// резерв для некротических забав
const int kMobMentalShadow = 3020;
const int kMobKeeper = 3021;
const int kMobFirekeeper = 3022;

const int kMaxSpellAffects = 16; // change it if you need more

bool IsRoomForbidden(RoomData *room);
void mobile_affect_update();
void player_affect_update();
void print_rune_log();
void ShowAffExpiredMsg(ESpell aff_type, CharData *ch);

int CallMagicToGroup(CharData *ch, CastContext roll);
int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, CastContext roll);

int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Result of one cast stage (CastAffect/CastUnaffects/...). CastToSingleTarget runs the
// stages in order and stops the chain on kBreak. Today every stage returns kSuccess
// (behaviour unchanged); per-spell kBreak conditions are to be driven from spells.xml later.
// The list may grow.
enum class EStageResult {
	kBreak,		// stop the cast: skip the remaining stages.
	kSuccess	// stage handled; continue to the next stage.
};

// Stage functions called from outside the magic module. Prefer CallMagic / CastSpell when
// possible -- direct calls bypass CastToSingleTarget's blocking/required/reflection/
// caster_blocking gates, which is rarely what callers want. fight_hit.cpp and spells.cpp
// (SpellHolystrike etc.) call CastDamage/CastAffect/CastUnaffects directly because they run a
// per-target loop outside the normal cast pipeline; handler.cpp calls CastAffect to re-apply
// gear-borne effects. The remaining stage functions (CastToPoints / CastToAlterObjs /
// CastCreation / CastSummon / CastManual / CastToSingleTarget) live in magic_internal.h --
// they're only called from CallMagic and the dispatcher in magic.cpp.
int CastDamage(CastContext &ctx);
EStageResult CastAffect(CastContext &ctx);
EStageResult CastUnaffects(CastContext &ctx);
int CalcSaving(CharData *killer, CharData *victim, ESaving saving, bool need_log = false);
int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply);
int GetBasicSave(CharData *ch, ESaving saving, bool log = false);
EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id);
int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id);

#endif // MAGIC_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
