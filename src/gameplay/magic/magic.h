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
#include "gameplay/affects/affect_contants.h"
#include "gameplay/abilities/talents_actions.h"   // issue.character-affect-triggers: EActionTrigger (EventContext)

#include <cstdlib>
#include <limits>
#include <optional>
#include <set>
#include <string_view>   // issue.character-affect-triggers: EventContext::GetTag
#include <vector>
#include <deque>   // issue.perk-action-patching: ActionContext::patch_scratch_

class CharData;
class ObjData;
struct RoomData;
namespace feats { struct TalentPatch; }   // issue.perk-action-patching
template<typename> class Affect;   // issue.mob-flag-affect-materialization: BuildMaterializedAffect return

// (issue.affect-migration) The old ESpell "first-aid removable" list + GetRemovableSpellId were
// removed: curability is now the kAfCurable battleflag -- see Affect::removable(), DoFirstaid, and
// the data-driven dispel/cure filter (AffectMatchesFlags).

// One evaluation of a talents_actions::Roll for a specific caster (issue #3333).
struct RollResult {
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
	// issue.potency-noise: the spell's ONE realized relative noise deviation d = sigma*z for THIS cast
	// (sigma from potency_roll, z the shared truncated-normal draw). Every manifestation scales it by its
	// own weight: value = min + beta*C*(1 + weight*noise_dev). 0 = deterministic. Potions freeze z, so d
	// is realized per spell with that spell's sigma.
	double noise_dev{0.0};
};

// issue.mob-flag-affect-materialization: build the real affect node(s) that realize an intrinsic mob
// buff flag. The affect and its granting spell share a name (kSanctuary affect <- kSanctuary spell,
// etc.), so we roll THAT spell's <potency_roll> for the mob (its skill + stat + the spell's dice),
// deriving BOTH the stored potency (so the buff resists dispel like a real cast, not a free strip) and
// each stat apply's modifier (competence = skill_coeff+stat_coeff, as an entry action's CompetenceBase).
// An affect with <apply> children (kCloudly, kBlink) yields one node per apply WITH its computed
// modifier; an applies-less buff (sanctuary/shields/prism/glass) yields one bare flag-carrier node.
// duration is -1 (permanent) on every node. If no same-named spell exists the roll is zero (flat mins,
// potency 0).
[[nodiscard]] std::vector<Affect<EApply>> BuildMaterializedAffect(const CharData *mob, EAffect affect_type);

// ActionContext (issue.spell-pipeline): the single object threaded through the whole
// cast handler chain (CallMagic -> CastToSingleTarget -> the per-stage Cast* fns).
// It carries the cast parameters, the targets, and the accumulating results so that
// later <action>s can see what earlier ones did.
//
// Immutable parts (set once at construction, exposed via const getters): the caster,
// the spell, the base level and the two rolls -- these never change during a cast.
// Mutable parts (public fields): the current targets, the working `level` (which
// decays across area targets, starting from base_level()), and the action results.
// issue.character-affect-triggers: the "why this action ran" payload for a trigger-launched action chain.
// Built at the trigger's fire-site and threaded through the context (see ActionContext::Event) so a
// manual_cast handler can read rich per-event data that has no XML grammar. trigger == kCount means "no
// event" (an ordinary cast, not trigger-launched). Members are filled per trigger kind at the fire-site:
//   kPreHit         -> weapon, skill                          (pre-damage swing)
//   kPostHit -> amount, weapon, skill, actor (= victim) (post-damage, on a landed hit)
//   kKill    -> amount (= fatal dmg), weapon, skill, actor (= the killed victim, already dead --
//              exposed for reads only; do NOT cast on it)   (fired from Damage::ProcessDeath)
struct EventContext {
	talents_actions::EActionTrigger trigger{talents_actions::EActionTrigger::kCount};
	int amount{0};                       // damage dealt (kPostHit) / points restored (kPoints)
	ObjData *weapon{nullptr};            // weapon used for the hit
	ESkill skill{ESkill::kUndefined};    // skill used for the hit
	CharData *actor{nullptr};            // the other party in the event (e.g. the hit victim / the healer)
	int points_category{-1};             // kPoints: which points_intensity::ECategory was restored (-1 = n/a)
	[[nodiscard]] bool valid() const { return trigger != talents_actions::EActionTrigger::kCount; }
	// issue.character-affect-triggers: numeric value of a well-known event tag, for <action base="tag"
	// tag="NAME">. Known tags are backed by the typed fields above; unknown -> 0. (A general variant
	// tag-bag for arbitrary tags is deferred until a custom event needs one.)
	[[nodiscard]] long GetTag(std::string_view name) const {
		if (name == "amount") { return amount; }
		return 0;
	}
};

class ActionContext {
 public:
	ActionContext(CharData *caster, ESpell spell_id, int level, const RollResult &potency)
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
	// Initialised to {spell_id} in BuildActionContext; a side cast inherits this set + itself.
	std::set<ESpell> casting;
	// issue.cast-chain: per-cast accumulators of what the actions have done so far, summed across
	// each action's targets. A later action can scale off one of these (its base= attribute)
	// instead of the caster's competence. Reset to 0 at cast start (fresh ActionContext per cast).
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
	// issue.affect-migration: run an external action list (an affect's own <actions>) through this
	// context instead of the spell's. nullptr (the default) = use the spell's actions.
	void UseExternalActions(const std::vector<talents_actions::Action> *list) { external_actions_ = list; }
	// issue.affect-migration: per-tick channel for room-affect manual_cast handlers -- the tick runner
	// seeds the ticking affect's current duration here and reads it back, so a manual handler (e.g.
	// HandleThunderstormTick) can branch on / modify it via this ActionContext (no room-affect type in
	// magic.h). -1 = not a tick cast.
	void SetTickDuration(int d) { tick_duration_ = d; }
	[[nodiscard]] int GetTickDuration() const { return tick_duration_; }
	// issue.room-affect-trigger-improve: a manual_cast handler's override of an event trigger's
	// return value. The <trigger return="N"/> tag is the default; a handler that calls this wins.
	// std::nullopt = handler set nothing -> the tag/default applies. Threaded like tick_duration_.
	void SetTriggerReturn(int v) { trigger_return_ = v; }
	[[nodiscard]] std::optional<int> GetTriggerReturn() const { return trigger_return_; }
	// issue.character-affect-triggers: the trigger event that launched this action chain (default = an
	// invalid/kCount event = ordinary cast). Threaded so manual_cast handlers can read the event's rich
	// data (damage amount, weapon, skill, the other party). Copied into side_spell sub-contexts.
	void SetEvent(const EventContext &e) { event_ = e; }
	[[nodiscard]] const EventContext &Event() const { return event_; }
	// issue.attack-ward: whole-cast defender-ward outcome, decided at the is_entry gate by
	// RunWholeCastWards. `reflect` is applied directly as cvict = caster (no flag); ward_stop_ = the
	// whole cast was absorbed (scope="all" / shield -> caller returns kNotCast). Scoped damage/affect
	// absorbs are NOT stored here -- they roll per-manifest in CastDamage/CastAffect (TryScopedAbsorb).
	void SetWardStop() { ward_stop_ = true; }
	[[nodiscard]] bool WardStop() const { return ward_stop_; }
	// issue.attack-ward: this cast is a side-spell sub-cast -- part of the same incoming spell, so the
	// whole-cast wards (reflect / scope="all") are NOT re-run for it (see CastSideSpell / is_entry gate).
	void SetNested() { nested_ = true; }
	[[nodiscard]] bool Nested() const { return nested_; }
	// issue.character-affect-triggers: affect-owned damage flavor for <damage> stages run under this
	// context (set by the affect-trigger runners). When present it FULLY REPLACES the generic combat
	// message + severity line in Damage::Process. Copied into side_spell sub-contexts like the event.
	void SetAffectDamageMsg(const std::string &to_char, const std::string &to_vict, const std::string &to_room) {
		aff_dmg_msg_char_ = to_char; aff_dmg_msg_vict_ = to_vict; aff_dmg_msg_room_ = to_room;
	}
	[[nodiscard]] const std::string &AffectDamageMsgChar() const { return aff_dmg_msg_char_; }
	[[nodiscard]] const std::string &AffectDamageMsgVict() const { return aff_dmg_msg_vict_; }
	[[nodiscard]] const std::string &AffectDamageMsgRoom() const { return aff_dmg_msg_room_; }
	// issue.damage-over-time: author (uid) credited for a <damage source="poison"> tick -- the poisoner
	// (affect caster_id), so a poison kill counts for them. 0 = no author.
	void SetDamageAuthorUid(long uid) { damage_author_uid_ = uid; }
	[[nodiscard]] long DamageAuthorUid() const { return damage_author_uid_; }
	[[nodiscard]] const talents_actions::Action *action() const;
	[[nodiscard]] bool HasPendingActions() const;
	// issue.perk-action-patching: build this cast's patched action chain from the caster's usable perks
	// (SpellInfo::talent_patches). Fast no-op when the spell has no patches. Call once in CallMagic before
	// the action walk; every other cast (and everyone without the perk) walks the spell unchanged.
	void ApplyTalentPatches();
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
	// issue.perk-action-patching: apply one resolved patch to action_view_/patch_scratch_.
	void ApplyOnePatch(const feats::TalentPatch &p);
	CharData *caster_{nullptr};
	ESpell spell_id_{ESpell::kUndefined};
	int base_level_{0};
	RollResult potency_;        // from SpellInfo::GetPotencyRoll()
	// Cursor state: the spell's action list + current index (set by RewindActions).
	const std::vector<talents_actions::Action> *actions_{nullptr};
	// issue.affect-migration: affect-owned actions; when set, override the spell's action list.
	const std::vector<talents_actions::Action> *external_actions_{nullptr};
	size_t action_idx_{0};
	int tick_duration_{-1};   // issue.affect-migration: see SetTickDuration (-1 = not a tick cast)
	std::optional<int> trigger_return_;   // issue.room-affect-trigger-improve: see SetTriggerReturn
	EventContext event_;   // issue.character-affect-triggers: see SetEvent/Event
	bool ward_stop_{false};   // issue.attack-ward: whole cast absorbed (scope="all"/shield) or refused
	bool nested_{false};      // issue.attack-ward: this is a side-spell sub-cast (skip whole-cast wards)
	std::string aff_dmg_msg_char_;   // issue.character-affect-triggers: see SetAffectDamageMsg
	std::string aff_dmg_msg_vict_;
	std::string aff_dmg_msg_room_;
	long damage_author_uid_{0};      // issue.damage-over-time: poison author for <damage source="poison">
	// issue.perk-action-patching: the per-cast patched chain (non-owning ptrs into the spell's own blocks
	// + perk-owned blocks + patch_scratch_). patched_ (not emptiness) selects it, so a perk that removed
	// every block still overrides. patch_scratch_ owns per-cast block copies for manifestation-level edits
	// (deque -> stable pointers). All empty/false on an ordinary unpatched cast.
	std::vector<const talents_actions::Action *> action_view_;
	std::deque<talents_actions::Action> patch_scratch_;
	bool patched_{false};
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
void ShowAffExpiredMsg(EAffect affect_type, CharData *ch);
// issue.affect-migration: emit an affect's imposition messages (data-driven, from affect_msg.xml).
// Picks the success or FAIL set by `failed` (the skill derives it from the affect's kAfFailed flag),
// across char/caster/room perspectives, with the armed/unarmed $o rule. Silent on missing keys.
// ch = applier, victim = the affected (ch == victim for self-affects).
// affected = the char the affect is on ($n); other = the externally-supplied target/opponent ($N,
// nullptr for a pure self-affect). Perspectives: ToChar->affected, ToVict->other, ToRoom->others.
void EmitAffectImpose(CharData *affected, CharData *other, EAffect affect_type, bool failed);
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
		int fixed_skill = -1,  // >=0 (potion): the MAKER skill used for buff DURATION
		int dir = -1);  // issue.room-affect-trigger-improve: dir>=0 + kMagRoom -> cast on EXIT(caster,dir) (door affect)
ECastResult CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst,
		int dir = -1);  // dir>=0: kTarDirection cast on the exit in that direction
// issue.character-affect-triggers: run the bearer's affect actions whose trigger matches event.trigger,
// threading `event` onto the context so manual_cast handlers can read its rich data. Called from trigger
// fire-sites (the per-hit kPreHit / post-damage kPostHit points in fight_hit.cpp). True if any ran.
bool RunCharEventTriggers(CharData *ch, const EventContext &event);

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
// CallMagicToArea/Group, BuildActionContext, ProcessMatComponents, CalcClassAntiSavingsMod, and
// the CastToPoints/CastToAlterObjs/CastCreation/CastSummon/CastManual/CastToSingleTarget
// dispatchers -- is module-internal and now lives in magic_internal.h.
EStageResult CastDamage(ActionContext &ctx);
EStageResult CastAffect(ActionContext &ctx);

// issue.vedun-hotfix #5: registered handler-name lists, for the Vedun editor's handler pick-lists.
std::vector<std::string> SummonHandlerNames();
std::vector<std::string> AlterObjHandlerNames();
std::vector<std::string> ObjCreationHandlerNames();
std::vector<std::string> ManualHandlerNames();

#endif // MAGIC_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
