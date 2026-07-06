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
#include <limits>
#include <optional>
#include <set>
#include <vector>

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
	// issue.potion-hotfix: a brewed potion's FROZEN brew-luck as a standard-normal z. When not NaN,
	// CalcNoisyAmount uses `mean + z*sd` instead of drawing, so every quaff of THIS potion delivers the
	// same amount -- and each of the potion's spells applies its OWN sigma (via sd) to the one shared z.
	// NaN (the default) means "no fixed noise -- draw randomly", i.e. an ordinary live cast.
	double noise_z{std::numeric_limits<double>::quiet_NaN()};
	// issue.potion-hotfix: a potion buff's DURATION scales off its MAKER's skill (stored on the potion),
	// never the drinker's. >=0 = a fixed maker skill (potion); <0 (default) = live cast, use the caster.
	int cast_skill{-1};
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
	CastContext(CharData *caster, ESpell spell_id, int level, const RollResult &potency)
		: level{level},
		  caster_{caster}, spell_id_{spell_id}, base_level_{level},
		  potency_{potency} {}

	// --- Immutable cast parameters ---
	[[nodiscard]] CharData *caster() const { return caster_; }
	[[nodiscard]] ESpell spell_id() const { return spell_id_; }
	[[nodiscard]] int base_level() const { return base_level_; }
	[[nodiscard]] const RollResult &potency() const { return potency_; }

	// --- Mutable working state ---
	// Working level: starts at base_level, decays per target in area casts.
	int level{0};
	// issue.area-cast: per-target effect/cast_success multiplier for area casts
	// (kUniform=1, kLinear/kStepped<=1). Set per target by CallMagicToArea; stays
	// 1.0 for every non-area cast, so the stage scalings below are identity there.
	double area_coeff{1.0};
	// issue.area-cast: targets that should react to the cast (retaliate). The whole spell is
	// ONE event, so reactions are recorded here per cast-upon target and fired once, after every
	// action has run -- a mid-spell retaliation must not kill the caster before later actions.
	std::vector<CharData *> reactions;
	// issue.side-spell: the spells currently in this cast chain (this spell + ancestors).
	// A side-spell stage refuses to cast a spell already here (cycle -> stack overflow).
	// Initialised to {spell_id} in BuildCastContext; a side cast inherits this set + itself.
	std::set<ESpell> casting;
	// issue.cast-chain: per-cast accumulators of what the actions have done so far, summed across
	// each action's targets. A later action can scale off one of these (its base= attribute)
	// instead of the caster's competence. Reset to 0 at cast start (fresh CastContext per cast).
	double damage_count{0.0};     // total (computed) HP damage dealt
	double points_count{0.0};     // total (computed) points restored, all categories
	double affects_potency{0.0};    // total potency of affects applied
	double dispelled_potency{0.0};  // total potency of affects removed
	// issue.cast-chain: true while the spell's FIRST action runs; CompetenceBase() forces real
	// competence then (the first action never chains off an accumulator). Set per action.
	bool is_entry_action{true};
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
	// issue.instant-death: the damage saving throw, rolled once per CastDamage (true = saved).
	// Shared by the instant-death gate (target must FAIL) and the save-for-half so the throw is
	// never rolled twice. Unset for non-<damage> casts / self-casts.
	std::optional<bool> last_saving_result;
	// Targets for the current cast. cvict will later become a richer target list
	// built by the target resolver (issue.spell-pipeline note).
	CharData *cvict{nullptr};
	ObjData  *ovict{nullptr};
	RoomData *rvict{nullptr};

	// --- Action cursor (issue.spell-pipeline multi-action) ---
	// The context owns the walk over the spell's <action> list so a stage cannot
	// swap the action mid-chain. RewindActions() must be called at the per-target
	// entry (CastToSingleTarget) -- area/group reuse one context across targets, so
	// the cursor has to restart per target. Usage:
	//   for (ctx.RewindActions(); ctx.HasPendingActions(); ctx.NextAction()) {...}
	// action() returns the current action, or nullptr when the spell has none --
	// a flag-only spell (no <action>) still gets ONE iteration so its in-code
	// stages fire; those stages then read their block via the spell-id getters.
	void RewindActions();
	void NextAction();
	[[nodiscard]] const talents_actions::Action *action() const;
	[[nodiscard]] bool HasPendingActions() const;
	// The action the current stage should read its block from: the cursor's
	// current action when the per-action loop is driving (CastToSingleTarget),
	// or the spell's primary action otherwise (bypass callers / rooms that call
	// a stage directly without rewinding the cursor). Stages use this instead of
	// MUD::Spell(spell_id).actions.GetX() so they read THEIR action, not always
	// the first one.
	[[nodiscard]] const talents_actions::Action &action_or_default() const;
	// issue.cast-chain: the scalar a stage's formula uses where competence (skill+stat) would go.
	// First action / base==kCompetence -> the potency roll's real competence; else the accumulator
	// named by the current action's base=.
	[[nodiscard]] double CompetenceBase() const;

 private:
	CharData *caster_{nullptr};
	ESpell spell_id_{ESpell::kUndefined};
	int base_level_{0};
	RollResult potency_;        // from SpellInfo::GetPotencyRoll()
	// Cursor state: the spell's action list + current index (set by RewindActions).
	const std::vector<talents_actions::Action> *actions_{nullptr};
	size_t action_idx_{0};
};

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
void ShowAffExpiredMsg(ESpell aff_type, CharData *ch);
// issue.npc-races: true if `ch` can speak/incant (players always; NPCs iff their race is <vocal/>).
bool IsAbleToSay(CharData *ch);

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

ECastResult CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level,
		float fixed_potency = -1.0f,   // fixed_potency>=0 (item/potion): use it as the whole cast potency, skip caster roll
		double fixed_noise_z = std::numeric_limits<double>::quiet_NaN(),  // not-NaN (brewed potion): frozen brew-luck z replayed at cast
		int fixed_skill = -1);  // >=0 (potion): the MAKER skill used for buff DURATION
ECastResult CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Result of one cast stage (CastAffect/CastUnaffects/...). With the per-action loop
// (issue.spell-pipeline) the dispatcher walks each spell action and runs its stages:
//   kBreak    -- stop the whole cast: skip this action's remaining stages AND all
//                following actions.
//   kContinue -- stop only the current action's remaining stages; advance to the
//                next action. (No stage produces this yet; until the per-action
//                loop lands it is treated like kSuccess -- i.e. "not kBreak".)
//   kSuccess  -- stage handled; run the next stage of the current action.
// Today every stage returns kSuccess; per-spell kBreak/kContinue conditions are to be
// driven from spells.xml later.
enum class EStageResult {
	kBreak,		// stop the whole cast (this action's rest + all later actions).
	kContinue,	// stop this action's remaining stages; go to the next action.
	kFail,		// issue.side-spell: stage ran but its effect failed; flow continues (status only).
	kSuccess	// stage handled; continue to the next stage of this action.
};

// Stage functions reachable from outside the magic module. Prefer CallMagic / CastSpell --
// direct calls bypass CastToSingleTarget's blocking/required/reflection gates (and the
// spell-level caster_conditions gate in CallMagic),
// which is rarely what callers want. The two kept here have real external callers:
// fight_hit.cpp / spells.cpp run a per-target loop (CastDamage), and handler.cpp re-applies
// gear-borne effects (CastAffect). Everything else that used to live here -- CastUnaffects,
// CallMagicToArea/Group, BuildCastContext, ProcessMatComponents, CalcClassAntiSavingsMod, and
// the CastToPoints/CastToAlterObjs/CastCreation/CastSummon/CastManual/CastToSingleTarget
// dispatchers -- is module-internal and now lives in magic_internal.h.
EStageResult CastDamage(CastContext &ctx);
EStageResult CastAffect(CastContext &ctx);

// issue.vedun-hotfix #5: registered handler-name lists, for the Vedun editor's handler pick-lists.
std::vector<std::string> SummonHandlerNames();
std::vector<std::string> AlterObjHandlerNames();
std::vector<std::string> ObjCreationHandlerNames();
std::vector<std::string> ManualHandlerNames();

#endif // MAGIC_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
