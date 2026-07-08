/**
\authors Created by Sventovit
\date 5.06.2022.
\brief Модуль воздействий талантов.
\details Классы, описывающие активные воздействия различных талантов (заклинаний, умений, способностей) -
 урон, область применения, активные аффекты и т.п.
*/

#ifndef BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_
#define BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

#include "gameplay/skills/skills.h"
#include "gameplay/affects/affect_contants.h"
#include "engine/entities/entities_constants.h"
#include "utils/parser_wrapper.h"

#include <string>
#include <vector>

class CharData;
enum class ESpell;

namespace talents_actions {

enum class EAction {
	kDamage,
	kArea,
	kPoints,   // hit / moves / thirst / cond restoration (was kHeal; issue.mag-points)
	kAffect,
	kUnaffect,
	kSummon,   // data-driven summon skeleton (issue.summon-pipeline); IAction-backed
	kCreation, // data-driven object creation (issue.obj-casting); IAction-backed
	kAlterObj, // data-driven object transform (issue.obj-casting); IAction-backed
	kManual,   // hand-coded handler (issue.manual-cast); not an IAction -- backed by manual_handler_
	kSideSpell // nested cast of another spell (issue.side-spell); not an IAction -- backed by side_spells_
};

// issue.area-cast per-action targets: how a NON-FIRST <action> picks its targets (the first
// action always uses the spell's own targeting flags). Set via the <action target="..."> attr.
enum class EActionTarget {
	kTarSame,        // reuse the previous action's resolved target list (default)
	kTarFightSelf,   // the caster
	kTarFightVict,   // the caster's current opponent
	kTarGroup,       // groupmates in the room (needs an <area> section)
	kTarFoes,        // all foes in the room (needs an <area> section)
	kTarRandomFoe,   // one random foe in the room
	kTarRandomAlly,  // one random ally in the room
	kTarMinions,     // the caster's charmed NPC followers in the room (minions)
	kTarRoomThis     // the room itself -- this action imposes its effect on the room
	                 // (the per-tick effect lives in the linked kService spell)
};

// issue.cast-chain: what a NON-FIRST <action> uses as the "base" of its formula instead of the
// caster's competence (skill+stat). kCompetence (default) keeps competence; the others substitute
// the matching per-cast accumulator (damage dealt / points restored / affects applied / removed).
// The first action ignores this -- it always uses competence. Set via <action base="...">.
enum class EActionBase { kCompetence, kDamage, kPoints, kAffects, kDispelled };

// (EAlign moved to engine/entities/entities_constants.h, issue.cast-dmg-migration: it's a
//  general alignment concept, not a talents-specific one. Used by <blocking>/<required>/<reflection>.)

// A set of mob flags (EMobFlag, from an NPC prototype), affect flags (EAffect), an optional
// alignment selector, and a set of room flags (ERoomFlag) used by the action-level
// <blocking>/<required>/<caster_blocking> tags (issue.cast-affect + issue.cast-dmg-migration
// + issue.room-affects). For <blocking> any matching flag/affect/align/room_flag blocks the
// cast; for <required> the target must have ALL of them. Mob flags are meaningful only on
// NPCs. align kAny means "no alignment check on this side". The room_flags check examines
// the CASTER's current room and is what drives the kNoMagic blocking for non-warcry spells.
struct FlagCondition {
	std::vector<EMobFlag> mob_flags;
	std::vector<EAffect> affect_flags;
	std::vector<ERoomFlag> room_flags;
	EAlign align{EAlign::kAny};
	[[nodiscard]] bool empty() const {
		return mob_flags.empty() && affect_flags.empty() && room_flags.empty()
				&& align == EAlign::kAny;
	}
};

// Action-level reflection (issue.cast-dmg-migration): when ch != cvict and the original target
// carries any of `affect_flags` OR matches the `align` selector, the spell may bounce back at
// the caster (a single percentile roll vs `prob`, default 20). The check lives in
// CastToSingleTarget so it runs once for the whole cast, not per stage. Reflection can't fire on
// self-casts. Potency/strength comparisons aren't checked here: mob/object affects are bare flags
// without a potency value, so a simple flag match + prob is the most we can do today.
struct Reflection {
	std::vector<EAffect> affect_flags;
	EAlign align{EAlign::kAny};
	int prob{20};
	[[nodiscard]] bool empty() const {
		return affect_flags.empty() && align == EAlign::kAny;
	}
};

class IAction {
 public:
	virtual ~IAction() = default;

	virtual void Print(CharData *ch, std::ostringstream &buffer) const = 0;
};

// Spell "potency roll" parameters (see issue #3332): dice plus a bonus from the
// relevant skill and a bonus from a base stat. A single potency roll is made per
// spell (stored in SpellInfo) and its result is fed to the individual effects
// (damage/heal). The same type is meant to be reused later for a spell "success
// roll", which takes a similar set of casting parameters.
class Roll {
	// Defaults are all zero (issue.dicerolls): an absent or all-zero <dices> block means "no
	// dice contribution". The previous defaults of 1/1/1 silently added a constant +2 to every
	// spell missing a <dices> block, which broke the principle of least surprise. See the
	// parser (Roll::Roll) for the matching change to drop the std::max(1, ...) clamps.
	int dice_num_{0};
	int dice_size_{0};
	int dice_add_{0};

	ESkill base_skill_{ESkill::kUndefined};
	// Member defaults are 0 (no competence) so a wholly-absent <base_skill>/<base_stat> node -- e.g. a
	// spell with no <potency_roll> -- contributes nothing, as before. The hi-dominant rebalance default
	// (low=1, hi=7, stat weight=18; issue.random-noise-rework, weights are percentages /100) is applied
	// in the PARSER (Roll::Roll RollDoubleOr fallbacks) when a PRESENT node omits the attribute.
	double low_skill_bonus_{0.0};
	double hi_skill_bonus_{0.0};

	EBaseStat base_stat_{EBaseStat::kFirst};
	int base_stat_threshold_{10};
	double base_stat_weight_{0.0};
 public:
	Roll() = default;
	explicit Roll(parser_wrapper::DataNode &node);

	[[nodiscard]] int RollSkillDices() const;
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcSkillCoeffForValue(int skill) const;  // CalcSkillCoeff with an explicit skill value (item brewing)
	// The low-skill part of the skill coefficient only (no hi-skill term, no /100): used
	// as the skill bonus for the battle-lag formula (issue.cast-spell-lag).
	[[nodiscard]] double CalcLowSkillCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcBaseStatCoeffForValue(int stat) const;  // explicit key-stat value (authored potion)
	// The roll's key skill: also used as the affect's duration scaling skill (issue.calc-duration)
	// and the multi-hit count scaling skill (issue.extra-hits).
	[[nodiscard]] ESkill GetBaseSkill() const { return base_skill_; }

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

// issue.success-roll: per-spell success-roll config (sibling of <potency_roll>). Unlike Roll it has
// NO <dices> block -- success depends on randomness differently (the planned mechanic is a d100 roll
// vs a skill/stat-derived target with crit thresholds, mirroring the abilities roll system so the two
// can later be unified). base_skill / base_stat mirror potency_roll/Roll. <bonus> and <thresholds>
// are optional. Parsed and stored only for now; not yet consumed.
class SuccessRoll {
	ESkill base_skill_{ESkill::kUndefined};
	double low_skill_bonus_{0.0};
	double hi_skill_bonus_{0.0};

	EBaseStat base_stat_{EBaseStat::kFirst};
	int base_stat_threshold_{0};
	double base_stat_weight_{0.0};

	// <bonus> (optional): flat roll bonus + per-matchup bonus (caster x target; P=player, E=env/NPC).
	int bonus_roll_{0};
	int bonus_pvp_{0};
	int bonus_pve_{0};
	int bonus_evp_{0};
	int bonus_eve_{0};

	// <thresholds> (optional): d100 crit cutoffs; defaults match the abilities system (6 / 95).
	int critsuccess_{6};
	int critfail_{95};
 public:
	SuccessRoll() = default;
	explicit SuccessRoll(parser_wrapper::DataNode &node);

	[[nodiscard]] ESkill GetBaseSkill() const { return base_skill_; }
	[[nodiscard]] EBaseStat GetBaseStat() const { return base_stat_; }
	[[nodiscard]] int GetRollBonus() const { return bonus_roll_; }
	[[nodiscard]] int GetPvpBonus() const { return bonus_pvp_; }
	[[nodiscard]] int GetPveBonus() const { return bonus_pve_; }
	[[nodiscard]] int GetEvpBonus() const { return bonus_evp_; }
	[[nodiscard]] int GetEveBonus() const { return bonus_eve_; }
	[[nodiscard]] int GetCritsuccessThreshold() const { return critsuccess_; }
	[[nodiscard]] int GetCritfailThreshold() const { return critfail_; }

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

// Material component specification (issue.spellcomponents): one entry in a spell's
// <components> block. Both any_of and all_of are lists of object vnums; the cast
// proceeds only when EVERY vnum in all_of is present in the caster's reachable
// inventory/equip/room (subject to `where`) AND AT LEAST ONE of any_of is present.
// `where` is an EFind bitmask restricting the search locations; only the three
// flags kObjInventory / kObjRoom / kObjEquip are honoured, in the fixed search
// order equipment -> inventory -> room. A material with neither any_of nor
// all_of populated is meaningless (it can never be satisfied -- nothing to find);
// the parser logs a warning, and ProcessMatComponents silently skips such an
// entry rather than aborting the cast.
struct Material {
	std::vector<int> any_of;
	std::vector<int> all_of;
	// EFind bitmask. Set to the default (kObjInventory|kObjRoom|kObjEquip) in the
	// parser when the attribute is absent; 0 here means "explicitly empty" and
	// triggers a kBreak at consume time.
	Bitvector where{0};
	// Charge cost per cast, applied to each matched item's m_vals[2] (the
	// "charges remaining" field used by magic ingredients):
	//   cost  > 0 : val[2] -= cost; destroy the item when val[2] < 1.
	//   cost == 0 : presence required, but no charge spent (focus/catalyst).
	//   cost == -1: destroy the matched item in this single cast regardless
	//               of val[2] ("consumed whole" semantics).
	// Default 1 preserves the legacy dec_val(2) behaviour (one charge per
	// cast, destroy at zero). Game designers opt in to other behaviours via
	// the cost="N" attribute on <material>.
	int cost{1};
};

// The set of casting requirements for a spell (issue.spellcomponents). Lives at
// the <spell> level in spells.xml (sibling of <name>/<misc>/<potency_roll>),
// parsed by SpellInfoBuilder::ParseComponents and stored on SpellInfo.
// Today recognises <material> entries (the consumable / focus requirement),
// <verbal/> (the spell needs a spoken phrase to fire -- silenced casters can't
// cast it) and <weave/> (the action is actual magic -- can't be performed in a
// kNoMagic room). Forward-shaped for somatic / divine focus / sacrificial
// entries without further callsite churn.
class Components {
	std::vector<Material> materials_;
	// True if the spell has a <verbal/> child of <components>. A verbal spell
	// requires speech; kSilence on the caster blocks it (see do_cast,
	// CastSpell, process_player_attack, SaySpell). A non-verbal spell can be
	// cast under kSilence and SaySpell stays silent for it too.
	bool has_verbal_{false};
	// True if the spell has a <weave/> child of <components> (issue.weave-component).
	// A weave spell is actual magic and cannot be cast in a kNoMagic room -- the
	// CallMagic-side gate emits kCastForbiddenToChar / kCastForbiddenToRoom just
	// like the legacy data-driven <blocking><room_flags val="kNoMagic"/></blocking>
	// did; that block-shaped duplicate was removed from 228 spells in the same
	// migration, so weave is now the single source of truth for "is this magic".
	bool has_weave_{false};
	// True if the spell has a <sight/> child of <components> (issue.sight-component). Casting it
	// requires the caster to see; a blind caster is blocked by the CallMagic-side gate (HasSightComponent()).
	bool has_sight_{false};
 public:
	Components() = default;
	explicit Components(parser_wrapper::DataNode &node);
	[[nodiscard]] const std::vector<Material> &GetMaterials() const { return materials_; }
	[[nodiscard]] bool HasVerbalComponent() const { return has_verbal_; }
	[[nodiscard]] bool HasWeaveComponent() const { return has_weave_; }
	[[nodiscard]] bool HasSightComponent() const { return has_sight_; }
	[[nodiscard]] bool empty() const { return materials_.empty() && !has_verbal_ && !has_weave_ && !has_sight_; }
	void Print(std::ostringstream &buffer) const;
};

// The roll parameters (dice/skill/stat) used to live here; since issue #3332
// they belong to the spell-level potency roll (talents_actions::Roll, stored in
// SpellInfo). Damage now only carries its saving throw.
// Damage mirrors Heal's model (issue.damage-tag-improve): besides the saving throw it carries a
// completion probability and an <amount min= dices_weight= alpha= beta=> block. The damage amount
// follows the option-2 subquadratic model (issue.potency-formula): min + dice*dice_scale*(1 +
// alpha*C) + beta*C, where C = skill_coeff + stat_coeff. alpha=0 reduces to the legacy additive
// Formula A. The <amount> is optional; defaults are min=0, dices_weight=1.0, alpha=0, beta=1.0
// amount = dice + competencies).
class Damage : public IAction {
	ESaving saving_{ESaving::kReflex};
	int prob_{100};                          // percent chance the damage actually happens
	double amount_min_{0};                   // flat minimum damage
	double amount_dices_weight_{1.0};        // base scale on the potency roll's dice
	double amount_alpha_{0.0};               // dice-amplification: skill scales the dice (issue.potency-formula)
	double amount_beta_{1.0};                // additive skill+stat coefficient (was competencies_weight)
	// issue.random-noise-rework (P1): opt-in multiplicative truncated-normal noise. sigma>0 switches the
	// amount to GaussIntNumber(min + beta*C, sigma*beta*C, ...) -- mean scales with competence, relative
	// spread stays ~sigma (constant CV). 0 (default) keeps the legacy dice formula. beta is reused as k.
	double amount_sigma_{0.0};
	// Multi-hit support (issue.extra-hits): a <hits ...> child enables extra-hits computation via
	// CalcExtraHits. Absent tag -> has_hits_=false -> the spell deals exactly one hit (count=1).
	// When present, count = 1 + CalcExtraHits(caster, spell_id, base_skill, divisor, max, prob).
	bool has_hits_{false};
	int hits_skill_divisor_{25};             // CalcNoviceSkillBonus divisor for the extra-hits bonus
	int hits_max_{1};                        // upper bound on extra hits
	int hits_prob_{20};                      // percent chance the bonus fires (0 = random 0..extra)
	bool has_instant_death_{false};          // issue.instant-death: presence => the spell can kill outright
	int instant_death_prob_{100};            // percent chance the instant-death attempt is rolled
 public:
	explicit Damage(parser_wrapper::DataNode &node);
	[[nodiscard]] int GetProb() const { return prob_; }
	[[nodiscard]] double GetAmountMin() const { return amount_min_; }
	[[nodiscard]] double GetAmountDicesWeight() const { return amount_dices_weight_; }
	[[nodiscard]] double GetAmountAlpha() const { return amount_alpha_; }
	[[nodiscard]] double GetAmountBeta() const { return amount_beta_; }
	[[nodiscard]] double GetAmountSigma() const { return amount_sigma_; }
	[[nodiscard]] bool HasHits() const { return has_hits_; }
	[[nodiscard]] int GetHitsSkillDivisor() const { return hits_skill_divisor_; }
	[[nodiscard]] int GetHitsMax() const { return hits_max_; }
	[[nodiscard]] int GetHitsProb() const { return hits_prob_; }
	[[nodiscard]] ESaving GetSaving() const { return saving_; }
	[[nodiscard]] bool HasInstantDeath() const { return has_instant_death_; }
	[[nodiscard]] int GetInstantDeathProb() const { return instant_death_prob_; }

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// Points (issue.mag-points): the data-driven home for hit-point heals, move
// restores and thirst/cond adjustments. Each of the four categories lives in
// its own optional inner tag (<heal>, <moves>, <thirst>, <cond>); all four
// share the (min, dices_weight, alpha, beta) shape, only <heal>
// carries the NPC coefficient. The outer <points> tag carries the overheal-
// above-max flag (still heal-specific in semantics) and the per-cast
// probability that the whole points action fires.
//
// Sign convention for the four amounts (per issue.mag-points step 5):
//   heal:   positive = restore HP; negative is currently a no-op (the cast-
//           to-points path has no "damage" branch -- damage spells use
//           CastDamage). Reserved as 0 for safety.
//   moves:  positive = restore movement points; negative = drain them.
//   thirst: positive = restore (sate thirst); negative = make thirstier.
//           XML sign is the gameplay-natural one; the engine field stores
//           the opposite (0 = sated, higher = thirstier), so CastToPoints
//           negates the value before calling gain_condition().
//   cond:   same inversion as thirst, FULL slot.
//
// Heal class retired; the old <heal><amount/></heal> nesting collapsed into
// <points><heal/></points> and the inner Amount struct below is shared by
// every category.
class Points : public IAction {
 public:
	// One restoration / drain amount. For heal_, npc_coeff is parsed from
	// the XML (default 1.0 -- preserves the old <heal npc_coeff/> default);
	// for the other categories the parser leaves it at 0.0, which makes the
	// NPC-boost line in CastToPoints a no-op (v += v * 0 = v). present_ is
	// set by the parser when the corresponding inner tag is found.
	struct Amount {
		bool present{false};
		double min{0};
		double dices_weight{0};   // base scale on the potency dice
		double alpha{0};          // dice-amplification: skill scales the dice (issue.potency-formula)
		double beta{0};           // additive skill/stat coefficient (was competencies_weight)
		double npc_coeff{0};
		double sigma{0};          // issue.random-noise-rework: >0 -> multiplicative truncated-normal spread (heal/moves)
	};
 private:
	int extra_{0};        // overheal cap as percent ABOVE max_hp
	                      // (0 = no overheal; e.g. 20 lets HP reach 120% of max).
	int prob_{100};       // percent chance the whole points action fires
	Amount heal_;
	Amount moves_;
	Amount thirst_;
	// Hunger axis. Named `full_` (XML tag <full>) since the engine COND[FULL] field
	// is the hunger counter; the parent overall-state name "cond" is reserved for the
	// whole condition triplet, not just hunger (issue.point-bugs #3).
	Amount full_;
 public:
	explicit Points(parser_wrapper::DataNode &node);
	[[nodiscard]] int GetExtraPercent() const { return extra_; }
	[[nodiscard]] int GetProb() const { return prob_; }
	[[nodiscard]] const Amount &GetHeal()   const { return heal_; }
	[[nodiscard]] const Amount &GetMoves()  const { return moves_; }
	[[nodiscard]] const Amount &GetThirst() const { return thirst_; }
	[[nodiscard]] const Amount &GetFull()   const { return full_; }

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// issue.area-cast: per-target effect/cast_success falloff across area targets.
// kUniform = no falloff (full spell on every target, default); kLinear = (N-j+1)/N
// (target #1 full, linear down to 1/N); kStepped = free_targets full, then decay per step.
enum class EAreaDistribution { kUniform, kLinear, kStepped };

struct Area : public IAction {
	// <targets skill_divisor dice_size min max stat_weight/> (issue.area-cast): the count
	// keeps the historical skill-scaled-dice formula verbatim so target numbers match the
	// old values, plus an optional secondary-stat nudge (the cast potency roll's stat_coeff).
	int skill_divisor{1};
	int dice_size{1};
	int min_targets{1};
	int max_targets{1};
	double stat_weight{0.0};   // weight on the cast's potency stat_coeff (0 = exactly the old count)

	// <distribution type decay free_targets/>: per-target effect/cast_success falloff.
	// decay and free_targets matter only for kStepped.
	EAreaDistribution distribution{EAreaDistribution::kUniform};
	double decay{0.0};
	int free_targets{1};

	// Count = min + min(RollDices(skill/skill_divisor, dice_size)
	//                    + ceil(stat_weight*stat_coeff), max). The bonus (incl. the stat term)
	// is capped at max, so the total ranges [min, min+max] -- the historical semantics.
	[[nodiscard]] int CalcTargetsQuantity(int skill, double stat_coeff) const;
	// Per-target coefficient f_j (1-based j of n). decay_eff lets the caller pass the
	// kMultipleCast-softened rate. Always in [0,1] and non-increasing (never amplifies).
	[[nodiscard]] double DistributionCoeff(int j, int n, double decay_eff) const;
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// <summon base_fail min_fail handler><mob vnum competence_weight extra keeper from_corpse/></summon>
// (issue.summon-pipeline): the formalized common 80% of a summon -- the fail roll and the
// standard competence-scaled stats/affects -- with the spell-specific 20% in a named C++
// handler. Inert until the summon stage consumes it.
struct Summon : public IAction {
	int base_fail{0};               // base % failure chance; C*competence_weight is subtracted
	int min_fail{0};                // floor for the failure chance
	std::string handler;            // post-spawn handler name (looked up in the summon handler registry)
	// the single <mob> spec
	int mob_vnum{0};                // positive mob vnum (0 = the handler resolves it, e.g. from a corpse);
	                                // the summon loads it as a kSummoned instance (ReadMobile negates)
	double competence_weight{0.0};  // weight on C for this mob's fail/stat scaling
	bool extra{false};              // may summon extra abilities
	bool keeper{false};             // gets EAffect::kHelper + ESkill::kRescue
	bool from_corpse{false};        // resolve the mob from a corpse (animate dead / resurrection)
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// <obj_creation vnum handler/> (issue.obj-casting): create an object by vnum, then optionally
// run a named post-load handler to customize it (e.g. kCreateWeapon/kCreateArmor base items).
// The shared load/narrate/place skeleton lives in the creation stage. Inert until consumed.
struct Creation : public IAction {
	int vnum{0};               // prototype vnum to load
	std::string handler;       // optional post-load customizer (creation handler registry)
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// <alter_obj handler/> (issue.obj-casting): transform the target object via a named handler.
// The shared skeleton (resolve obj, kNoalter guard, no-effect message) lives in the alter-obj
// stage; only the per-spell transform is the handler. Inert until consumed.
struct AlterObj : public IAction {
	std::string handler;       // the per-spell transform (alter-obj handler registry)
	// issue.spells-hotfix: on a CHAR cast (no explicit object target) also alter a random item the
	// victim carries -- but ONLY when damage was dealt this cast (acid corroding gear). Default false
	// = never splash onto a char's items; cast the spell directly on an object to affect one.
	bool collateral_on_damage{false};
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// A spell affect described in data (issue #3334): the <affects> talent action.
// It stores the affect identity (spell)/saving/resist, the duration parameters
// (one duration for all applies), the affect flags (EAffFlag, common to all
// applies) and a list of applies. Each apply produces one Affect on the target,
// its modifier derived from the cast's potency roll (see CalcModifier callers).
class TalentAffect : public IAction {
 public:
	struct Apply {
		EAffect id{EAffect::kUndefined};
		EApply location{EApply::kNone};
		// Modifier = factor * cap(min + ceil(dices*dices_weight*(1+alpha*C) + beta*C)), C = skill+stat.
		// The cap (see below) is applied to the raw magnitude BEFORE the factor, so factor=-1
		// debuffs are bounded by [-cap, -min] when cap > 0.
		double min{0.0};
		double dices_weight{0.0};   // base scale on the potency dice
		double alpha{0.0};          // dice-amplification: skill scales the dice (issue.potency-formula)
		double beta{0.0};           // additive skill/stat coefficient (was competencies_weight)
		int factor{1};
		// Optional upper bound on the raw magnitude (i.e. on (min + ceil(comp*cw + dice*dw)))
		// before factor is applied. 0 (default) means "no cap" -- the modifier scales without
		// limit. Used by kForbidden (cap=100, mirroring OLD MIN(100, ...)) and by the elemental
		// auras (cap=30, recalibrated for ~R15 saturation). issue.modifier-cap.
		int cap{0};
		// Maximum number of stacks this affect may build up on a target (issue.affect-stacks).
		// Optional; defaults to 1 (no stacking). Values below 1 are clamped to 1. With stack > 1
		// re-applying adds a stack (accumulating the modifier) until the cap is reached.
		int stack{1};
		// When true this apply is not imposed unconditionally: all random-flagged applies of
		// the affect form one pool from which a single one is chosen at random per cast.
		bool random{false};
	};

	explicit TalentAffect(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;

	[[nodiscard]] ESpell GetSpell() const { return spell_; }
	// Room-affect per-tick spell (a kService spell whose actions run each tick); kUndefined = none.
	[[nodiscard]] ESpell GetTickSpell() const { return tick_spell_; }
	// Room-affect per-tick code handler named by string (the manual-cast mechanism for ticks the
	// data can't express, e.g. weather); empty = none. Resolved via the room tick-handler registry.
	[[nodiscard]] const std::string &GetTickHandler() const { return tick_handler_; }
	[[nodiscard]] ESaving GetSaving() const { return saving_; }
	[[nodiscard]] EResist GetResist() const { return resist_; }
	[[nodiscard]] int GetProb() const { return prob_; }
	[[nodiscard]] Bitvector GetFlags() const { return flags_; }
	// Duration parameters (issue.calc-duration): base (flat duration in hours, PC unit-converted to
	// ticks) plus a skill-scaled bonus = min(skill, kNoviceSkillThreshold)/skill_divisor, optionally
	// clamped to [dur_min_, dur_max_] (0 means no clamp on that side, OLD-style). The "skill" is
	// the spell's potency-roll base_skill -- pulled at the call site from MUD::Spell(...).GetPotencyRoll().
	[[nodiscard]] int GetDurationBase() const { return dur_base_; }
	[[nodiscard]] int GetDurationSkillDivisor() const { return dur_skill_divisor_; }
	[[nodiscard]] int GetDurationMin() const { return dur_min_; }
	[[nodiscard]] int GetDurationMax() const { return dur_max_; }
	[[nodiscard]] const std::vector<Apply> &GetApplies() const { return applies_; }
	[[nodiscard]] bool HasLag() const { return has_lag_; }
	[[nodiscard]] unsigned GetLagBase() const { return lag_base_; }
	[[nodiscard]] double GetLagBonusDivisor() const { return lag_bonus_divisor_; }
	[[nodiscard]] bool HasReposition() const { return has_reposition_; }
	[[nodiscard]] EPosition GetRepositionPos() const { return reposition_pos_; }
	[[nodiscard]] bool GetRepositionStopFight() const { return reposition_stop_fight_; }
	// Scales the cast-potency value stored on each Affect this block imposes
	// (issue.affects-potency-weight). Default 1.0 (no change). Smaller values
	// (e.g. 0.5) record a weaker affect, which a future dispel contest in
	// DispelSucceeds finds easier to beat. Used to keep big-modifier spells
	// from becoming undispellable just because the same potency roll feeds
	// both modifier and stored potency. Symmetric in spirit with
	// TalentUnaffect::potency_weight_ on the dispel side.
	[[nodiscard]] float GetPotencyWeight() const { return potency_weight_; }
	// issue.random-noise-rework: additive per-affect dispel modifier (threshold percentage points) in
	// the dispel contest -- negative = harder to remove (critical buffs), positive = easier. Default 0;
	// the data-driven <affects dispel_mod=> attr (no code constants).
	[[nodiscard]] int GetDispelMod() const { return dispel_mod_; }

 private:
	ESpell spell_{static_cast<ESpell>(0)};
	ESpell tick_spell_{ESpell::kUndefined};
	std::string tick_handler_;
	ESaving saving_{ESaving::kReflex};
	EResist resist_{EResist::kFire};
	int prob_{100};                         // percent chance the affect block fires (default always)
	Bitvector flags_{0};
	int dur_base_{0};
	int dur_skill_divisor_{0};
	int dur_min_{0};
	int dur_max_{0};
	std::vector<Apply> applies_;
	// Battle lag applied to the victim when the affect lands (the <lag> tag,
	// issue.cast-spell-lag): lag = base + skill_bonus/bonus_divisor battle rounds, or just
	// base when bonus_divisor <= 0 (constant lag). has_lag_ marks that the tag was present.
	bool has_lag_{false};
	unsigned lag_base_{0};
	double lag_bonus_divisor_{0.0};
	// Forced reposition applied when the affect lands (the <reposition> tag, issue.cast-affect):
	// knock the victim to reposition_pos_ (kUndefined = no position change, only stop the fight)
	// and, if reposition_stop_fight_, stop everyone fighting the victim (the "peaceful" effect).
	bool has_reposition_{false};
	EPosition reposition_pos_{EPosition::kUndefined};
	bool reposition_stop_fight_{false};
	// Stored-potency scale (issue.affects-potency-weight); see GetPotencyWeight().
	float potency_weight_{1.0f};
	int dispel_mod_{0};  // issue.random-noise-rework: additive dispel-threshold modifier (<affects dispel_mod=>)
};

// The "unaffect" talent action (issue #3342): removes affects from the target and/or
// breaks the cast chain depending on which affects are present. Each of the four blocks
// holds two ESpell lists (affect types): any_of ("any one present") and all_of ("all
// present together"). Either list may instead be the wildcard "*", meaning "any/all
// affects matching the unaffect's affect_flags filter" -- see CastUnaffects for the
// processing order. With wildcard_any, one random eligible affect is picked; with
// wildcard_all, every eligible affect is queued for removal.
class TalentUnaffect : public IAction {
 public:
	// One <blocking>/<breaking>/<remove_anyway>/<remove> entry: its any_of/all_of lists.
	// breaking_by_failure (remove/remove_anyway only): if a dispel of any affect in this block
	// fails the potency check, the cast chain breaks (CastUnaffects returns kBreak).
	// wildcard_any/wildcard_all (set by any_of="*"/all_of="*"): match the unaffect's
	// affect_flags filter rather than a fixed spell list. Mutually exclusive with their
	// respective list; "*|kPoison" is rejected at parse time.
	struct Set {
		std::vector<ESpell> any_of;
		std::vector<ESpell> all_of;
		bool wildcard_any{false};
		bool wildcard_all{false};
		bool breaking_by_failure{false};
		[[nodiscard]] bool empty() const {
			return any_of.empty() && all_of.empty() && !wildcard_any && !wildcard_all;
		}
	};

	explicit TalentUnaffect(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;

	[[nodiscard]] const Set &GetBlocking() const { return blocking_; }
	[[nodiscard]] const Set &GetBreaking() const { return breaking_; }
	[[nodiscard]] const Set &GetRemoveAnyway() const { return remove_anyway_; }
	[[nodiscard]] const Set &GetRemove() const { return remove_; }
	// Base dispel win chance in percent at competence parity (the <unaffect dispel_bonus=> attr,
	// default 50). The dispeller's competence advantage over the affect shifts it up/down; see the
	// d100 contest in DispelSucceeds. issue.random-noise-rework: replaced the potency_weight multiplier.
	[[nodiscard]] int GetDispelBonus() const { return dispel_bonus_; }
	// Which affects this unaffect may remove: an affect is eligible only if it carries at least one
	// of these EAffFlag bits (issue.affect-dispell-flags). Default kAfCurable|kAfDispellable -- a
	// generic unaffect removes anything curable or dispellable. kRemovePoison narrows it to
	// kAfCurable (cures, doesn't dispel); kDispellMagic to kAfDispellable (dispels, doesn't cure).
	[[nodiscard]] Bitvector GetAffectFlags() const { return affect_flags_; }
	[[nodiscard]] int GetProb() const { return prob_; }
	// issue.debuff-decay: percent of THIS dispel's potency by which a SURVIVING affect's potency
	// shifts when a removal attempt fails (positive weakens it, negative strengthens it; 0 = none).
	[[nodiscard]] int GetDecay() const { return decay_; }

 private:
	Set blocking_;       // present -> removal is blocked (chain not affected)
	Set breaking_;       // present -> the cast chain breaks (EStageResult::kBreak)
	Set remove_anyway_;  // dispelled even when blocking is true
	Set remove_;         // dispelled only when blocking is false
	int dispel_bonus_{50};  // issue.random-noise-rework: % win at competence parity (was potency_weight multiplier)
	Bitvector affect_flags_{kAfCurable | kAfDispellable};
	int prob_{100};      // percent chance the unaffect block fires at all (default always)
	int decay_{0};       // issue.debuff-decay: % of dispel potency to shift a surviving affect on a failed removal
};

using ActionPtr = std::shared_ptr<IAction>;

// One <action> block (issue.spell-pipeline multi-action): the manifestations it
// contains (damage / area / points / affects / unaffect, keyed by EAction) plus
// its own target/caster gates. A spell's <talent_actions> is an ordered list of
// these; today every spell has exactly one. The manifestation getters throw if
// the action lacks that kind -- callers guard with Contains() first, exactly as
// the old Actions getters did.
class Action {
	using ActionsRoster = std::unordered_multimap<EAction, ActionPtr>;
	ActionsRoster manifestations_;
	// Target gates (issue.cast-affect): the cast is refused on this action unless the target has
	// none of blocking_ and all of required_. Checked in CastToSingleTarget, not inside a stage.
	FlagCondition blocking_;
	FlagCondition required_;
	// Reflection (issue.cast-dmg-migration): checked in CastToSingleTarget; redirects the cast
	// back at the caster on a successful prob roll.
	Reflection reflection_;
	// Per-action target selector (issue.area-cast): how a non-first action picks its targets.
	// Ignored for the first action (uses the spell's targeting flags). Default kTarSame.
	EActionTarget target_{EActionTarget::kTarSame};
	// Per-action formula base (issue.cast-chain): kCompetence = caster competence (default);
	// otherwise the matching accumulator. Ignored for the first action. reset_ zeroes the base
	// accumulator after this action runs (no-op for the first action / kCompetence).
	EActionBase base_{EActionBase::kCompetence};
	bool reset_{false};
	// issue.manual-cast: name of the hand-coded handler (<manual_cast><handler val=>) this action
	// runs as its manual stage; resolved against the registry in magic.cpp. Empty = no manual stage.
	std::string manual_handler_;
	// issue.side-spell: spells to cast (full nested pipeline) on this action's target(s), in order.
	// Parsed from <side_spell id="kHold"/> (several allowed). Empty = no side-spell stage.
	std::vector<ESpell> side_spells_;

	friend class Actions;   // Actions builds these via the Parse* helpers.

 public:
	void Print(CharData *ch, std::ostringstream &buffer) const;

	[[nodiscard]] bool Contains(EAction action) const;
	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] const Area &GetArea() const;
	[[nodiscard]] const Summon &GetSummon() const;
	[[nodiscard]] const Creation &GetCreation() const;
	[[nodiscard]] const AlterObj &GetAlterObj() const;
	[[nodiscard]] const Points &GetPoints() const;
	[[nodiscard]] const TalentAffect &GetAffect() const;
	[[nodiscard]] const TalentUnaffect &GetUnaffect() const;
	[[nodiscard]] const FlagCondition &GetBlocking() const { return blocking_; }
	[[nodiscard]] const FlagCondition &GetRequired() const { return required_; }
	[[nodiscard]] const Reflection &GetReflection() const { return reflection_; }
	[[nodiscard]] EActionTarget GetTarget() const { return target_; }
	[[nodiscard]] EActionBase GetBase() const { return base_; }
	[[nodiscard]] bool GetReset() const { return reset_; }
	[[nodiscard]] const std::string &GetManualHandler() const { return manual_handler_; }
	[[nodiscard]] const std::vector<ESpell> &GetSideSpells() const { return side_spells_; }
};

// Spell-level caster gate (issue.spell-unification): a spell is castable by the caster
// or not -- this is a whole-spell concern, not per-action, so it lives on SpellInfo
// (after potency_roll/success_roll), checked once in CallMagic. Both sections reuse
// FlagCondition; only the <align> and <affect_flags> axes are meaningful for a caster.
struct CasterConditions {
	FlagCondition blocking;   // caster carrying any of these is refused the spell
	FlagCondition required;   // caster must satisfy all of these to cast
	[[nodiscard]] bool empty() const { return blocking.empty() && required.empty(); }
};

class Actions {
	// Ordered list of <action> blocks, in XML order. Empty for spells with no
	// <talent_actions> (flag-only spells: summon / creation / manual / ...).
	std::vector<Action> list_;

	// Parse one <action> into `out` (its manifestations + gates).
	static void ParseAction(Action &out, parser_wrapper::DataNode node);
	static void ParseDamage(Action &out, parser_wrapper::DataNode &node);
	static void ParseArea(Action &out, parser_wrapper::DataNode &node);
	static void ParseSummon(Action &out, parser_wrapper::DataNode &node);
	static void ParseCreation(Action &out, parser_wrapper::DataNode &node);
	static void ParseAlterObj(Action &out, parser_wrapper::DataNode &node);
	static void ParsePoints(Action &out, parser_wrapper::DataNode &node);
	static void ParseAffect(Action &out, parser_wrapper::DataNode &node);
	static void ParseUnaffect(Action &out, parser_wrapper::DataNode &node);
	static void ParseFlagCondition(FlagCondition &cond, parser_wrapper::DataNode &node);
	// Parse a <target_conditions> block (issue.spell-unification): its <blocking> and
	// <required> children fill the action's blocking_/required_ via ParseFlagCondition.
	static void ParseTargetConditions(Action &out, parser_wrapper::DataNode &node);
	static void ParseReflection(Reflection &refl, parser_wrapper::DataNode &node);

	// Empty fallback for the back-compat delegating getters when list_ is empty
	// (a spell with no <action>): preserves the old "default-constructed gates"
	// behaviour for GetBlocking()/GetRequired()/... on such spells.
	[[nodiscard]] static const Action &EmptyAction();

 public:
	void Build(parser_wrapper::DataNode &node);
	// Parse a spell-level <caster_conditions> block (issue.spell-unification) into
	// `out`: <blocking>/<required> children with <align>/<affect_flags> tags. Static
	// because it fills a SpellInfo-level member, not an Actions instance.
	static void ParseCasterConditions(CasterConditions &out, parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

	// Ordered access to the action list (issue.spell-pipeline). The cast loop
	// walks this in order.
	[[nodiscard]] const std::vector<Action> &list() const { return list_; }

	// The action a single-action (or bypass / room) caller should use: the first
	// action, or a static empty one when the spell has none. Underpins the
	// back-compat getters and CastContext::action_or_default().
	[[nodiscard]] const Action &primary() const {
		return list_.empty() ? EmptyAction() : list_.front();
	}

	// Back-compat single-action API: delegates to the first action (or the empty
	// fallback when there is none). Every spell has exactly one action today, so
	// these stay behaviour-identical; the dispatch migrates to list()/the cursor
	// in later stages.
	[[nodiscard]] bool Contains(EAction action) const;
	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] const Area &GetArea() const;
	[[nodiscard]] const Summon &GetSummon() const;
	[[nodiscard]] const Creation &GetCreation() const;
	[[nodiscard]] const AlterObj &GetAlterObj() const;
	[[nodiscard]] const Points &GetPoints() const;
	[[nodiscard]] const TalentAffect &GetAffect() const;
	[[nodiscard]] const TalentUnaffect &GetUnaffect() const;
	[[nodiscard]] const FlagCondition &GetBlocking() const;
	[[nodiscard]] const FlagCondition &GetRequired() const;
	[[nodiscard]] const Reflection &GetReflection() const;
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
