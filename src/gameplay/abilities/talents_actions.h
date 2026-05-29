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

#include <vector>

class CharData;
enum class ESpell;

namespace talents_actions {

enum class EAction {
	kDamage,
	kArea,
	kHeal,
	kAffect,
	kUnaffect
};

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
	// The low-skill part of the skill coefficient only (no hi-skill term, no /100): used
	// as the skill bonus for the battle-lag formula (issue.cast-spell-lag).
	[[nodiscard]] double CalcLowSkillCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const;
	// The roll's key skill: also used as the affect's duration scaling skill (issue.calc-duration)
	// and the multi-hit count scaling skill (issue.extra-hits).
	[[nodiscard]] ESkill GetBaseSkill() const { return base_skill_; }

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
// Today recognises <material> entries (the consumable / focus requirement) and
// <verbal/> (the spell needs a spoken phrase to fire -- silenced casters can't
// cast it). Forward-shaped for somatic / divine focus / sacrificial entries
// without further callsite churn.
class Components {
	std::vector<Material> materials_;
	// True if the spell has a <verbal/> child of <components>. A verbal spell
	// requires speech; kSilence on the caster blocks it (see do_cast,
	// CastSpell, process_player_attack, SaySpell). A non-verbal spell can be
	// cast under kSilence and SaySpell stays silent for it too.
	bool has_verbal_{false};
 public:
	Components() = default;
	explicit Components(parser_wrapper::DataNode &node);
	[[nodiscard]] const std::vector<Material> &GetMaterials() const { return materials_; }
	[[nodiscard]] bool HasVerbal() const { return has_verbal_; }
	[[nodiscard]] bool empty() const { return materials_.empty() && !has_verbal_; }
	void Print(std::ostringstream &buffer) const;
};

// The roll parameters (dice/skill/stat) used to live here; since issue #3332
// they belong to the spell-level potency roll (talents_actions::Roll, stored in
// SpellInfo). Damage now only carries its saving throw.
// Damage mirrors Heal's model (issue.damage-tag-improve): besides the saving throw it carries a
// completion probability and an <amount min= dices_weight= competencies_weight=> block, so the
// damage amount is the roll's dice and competencies weighted by the damage's own factors. The
// <amount> is optional; its defaults are min=0 and BOTH weights 1.0 (so an omitted tag means
// amount = dice + competencies).
class Damage : public IAction {
	ESaving saving_{ESaving::kReflex};
	int prob_{100};                          // percent chance the damage actually happens
	double amount_min_{0};                   // flat minimum damage
	double amount_dices_weight_{1.0};        // weight applied to the potency roll's dice
	double amount_competencies_weight_{1.0}; // weight applied to the caster's skill+stat competencies
	// Multi-hit support (issue.extra-hits): a <hits ...> child enables extra-hits computation via
	// CalcExtraHits. Absent tag -> has_hits_=false -> the spell deals exactly one hit (count=1).
	// When present, count = 1 + CalcExtraHits(caster, spell_id, base_skill, divisor, max, prob).
	bool has_hits_{false};
	int hits_skill_divisor_{25};             // CalcNoviceSkillBonus divisor for the extra-hits bonus
	int hits_max_{1};                        // upper bound on extra hits
	int hits_prob_{20};                      // percent chance the bonus fires (0 = random 0..extra)
 public:
	explicit Damage(parser_wrapper::DataNode &node);
	[[nodiscard]] int GetProb() const { return prob_; }
	[[nodiscard]] double GetAmountMin() const { return amount_min_; }
	[[nodiscard]] double GetAmountDicesWeight() const { return amount_dices_weight_; }
	[[nodiscard]] double GetAmountCompetenciesWeight() const { return amount_competencies_weight_; }
	[[nodiscard]] bool HasHits() const { return has_hits_; }
	[[nodiscard]] int GetHitsSkillDivisor() const { return hits_skill_divisor_; }
	[[nodiscard]] int GetHitsMax() const { return hits_max_; }
	[[nodiscard]] int GetHitsProb() const { return hits_prob_; }

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// Heal no longer derives from Damage: it shares no parameters with it (healing has no saving
// throw). The heal amount is decoupled from the global potency roll: the roll only supplies the
// raw dice and competencies, while the <amount min= dices_weight= competencies_weight=> weights
// are the heal's own, so tuning a heal does not disturb the spell's other effects (which share
// the roll). Also carries an NPC coefficient, an "extra" overheal-above-max flag, and a
// completion probability.
class Heal : public IAction {
	double npc_coeff_{1};
	bool extra_{false};                    // may the heal raise hit points above the maximum?
	int prob_{100};                        // percent chance the healing actually happens
	double amount_min_{0};                 // minimum hit points restored
	double amount_dices_weight_{0};        // weight applied to the potency roll's dice
	double amount_competencies_weight_{0}; // weight applied to the caster's skill+stat competencies
 public:
	explicit Heal(parser_wrapper::DataNode &node);
	[[nodiscard]] double GetNpcCoeff() const { return npc_coeff_; }
	[[nodiscard]] bool IsExtra() const { return extra_; }
	[[nodiscard]] int GetProb() const { return prob_; }
	[[nodiscard]] double GetAmountMin() const { return amount_min_; }
	[[nodiscard]] double GetAmountDicesWeight() const { return amount_dices_weight_; }
	[[nodiscard]] double GetAmountCompetenciesWeight() const { return amount_competencies_weight_; }

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

struct Area : public IAction {
	double cast_decay{0.0};
	int level_decay{0};
	int free_targets{1};

	int skill_divisor{1};
	int targets_dice_size{1};
	int min_targets{1};
	int max_targets{1};

	[[nodiscard]] int CalcTargetsQuantity(int skill_level) const;
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
		EAffect id{EAffect::kUndefinded};
		EApply location{EApply::kNone};
		// Modifier = factor * cap(min + ceil(competencies*competencies_weight + dices*dices_weight)).
		// The cap (see below) is applied to the raw magnitude BEFORE the factor, so factor=-1
		// debuffs are bounded by [-cap, -min] when cap > 0.
		double min{0.0};
		double dices_weight{0.0};
		double competencies_weight{0.0};
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

 private:
	ESpell spell_{static_cast<ESpell>(0)};
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
	// Weight applied to this dispel's potency roll when checked against an affect's recorded
	// potency (issue: potency-gated dispel). Default 1.0 (the <unaffect potency_weight=> attr).
	[[nodiscard]] float GetPotencyWeight() const { return potency_weight_; }
	// Which affects this unaffect may remove: an affect is eligible only if it carries at least one
	// of these EAffFlag bits (issue.affect-dispell-flags). Default kAfCurable|kAfDispellable -- a
	// generic unaffect removes anything curable or dispellable. kRemovePoison narrows it to
	// kAfCurable (cures, doesn't dispel); kDispellMagic to kAfDispellable (dispels, doesn't cure).
	[[nodiscard]] Bitvector GetAffectFlags() const { return affect_flags_; }
	[[nodiscard]] int GetProb() const { return prob_; }

 private:
	Set blocking_;       // present -> removal is blocked (chain not affected)
	Set breaking_;       // present -> the cast chain breaks (EStageResult::kBreak)
	Set remove_anyway_;  // dispelled even when blocking is true
	Set remove_;         // dispelled only when blocking is false
	float potency_weight_{1.0f};
	Bitvector affect_flags_{kAfCurable | kAfDispellable};
	int prob_{100};      // percent chance the unaffect block fires at all (default always)
};

using ActionPtr = std::shared_ptr<IAction>;

class Actions {
	using ActionsRoster = std::unordered_multimap<EAction, ActionPtr>;
	using ActionsRosterPtr = std::unique_ptr<ActionsRoster>;
	ActionsRosterPtr actions_;
	// Action-level target gates (issue.cast-affect): the cast is refused unless the target has
	// none of blocking_ and all of required_. They apply to the whole action (damage, affects,
	// ...), so they are checked in CastToSingleTarget, not inside a single stage.
	FlagCondition blocking_;
	FlagCondition required_;
	// Action-level caster gate (issue.cast-dmg-migration): mirrors blocking_, but examines the
	// CASTER instead of the victim. Used by kDispelEvil / kDispelGood to refuse the cast when
	// the caster carries the incompatible alignment (replaces their old in-code "wrath" branch).
	// A future caster_required_ could be added by the same pattern.
	FlagCondition caster_blocking_;
	// Reflection (issue.cast-dmg-migration): also checked in CastToSingleTarget; redirects the
	// cast back at the caster on a successful prob roll (see Reflection comment above).
	Reflection reflection_;

	void ParseAction(ActionsRosterPtr &info, parser_wrapper::DataNode node);
	static void ParseDamage(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseArea(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseHeal(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseAffect(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseUnaffect(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseFlagCondition(FlagCondition &cond, parser_wrapper::DataNode &node);
	static void ParseReflection(Reflection &refl, parser_wrapper::DataNode &node);

 public:
	Actions() {
		actions_ = std::make_unique<ActionsRoster>();
	};

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

	[[nodiscard]] bool Contains(EAction action) const;
	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] const Area &GetArea() const;
	[[nodiscard]] const Heal &GetHeal() const;
	[[nodiscard]] const TalentAffect &GetAffect() const;
	[[nodiscard]] const TalentUnaffect &GetUnaffect() const;
	[[nodiscard]] const FlagCondition &GetBlocking() const { return blocking_; }
	[[nodiscard]] const FlagCondition &GetRequired() const { return required_; }
	[[nodiscard]] const FlagCondition &GetCasterBlocking() const { return caster_blocking_; }
	[[nodiscard]] const Reflection &GetReflection() const { return reflection_; }
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
