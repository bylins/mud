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
	int dice_num_{1};
	int dice_size_{1};
	int dice_add_{1};

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

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

// The roll parameters (dice/skill/stat) used to live here; since issue #3332
// they belong to the spell-level potency roll (talents_actions::Roll, stored in
// SpellInfo). Damage now only carries its saving throw.
class Damage : public IAction {
	ESaving saving_{ESaving::kReflex};
 public:
	explicit Damage(parser_wrapper::DataNode &node);

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

// Heal no longer derives from Damage: it shares no parameters with it (the roll
// moved to the potency roll, and healing has no saving throw). It only keeps the
// NPC healing coefficient.
class Heal : public IAction {
	double npc_coeff_{1};
 public:
	explicit Heal(parser_wrapper::DataNode &node);
	[[nodiscard]] double GetNpcCoeff() const;

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
		// Modifier = factor * min(min + ceil(competencies*competencies_weight
		//                                      + dices*dices_weight), min*stack).
		double min{0.0};
		double dices_weight{0.0};
		double competencies_weight{0.0};
		int factor{1};
		int stack{0};
		// When true this apply is not imposed unconditionally: all random-flagged applies of
		// the affect form one pool from which a single one is chosen at random per cast.
		bool random{false};
	};

	explicit TalentAffect(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;

	[[nodiscard]] ESpell GetSpell() const { return spell_; }
	[[nodiscard]] ESaving GetSaving() const { return saving_; }
	[[nodiscard]] EResist GetResist() const { return resist_; }
	[[nodiscard]] Bitvector GetFlags() const { return flags_; }
	[[nodiscard]] int GetDurationConst() const { return dur_const_; }
	[[nodiscard]] int GetDurationLevelDivisor() const { return dur_level_divisor_; }
	[[nodiscard]] int GetDurationMin() const { return dur_min_; }
	[[nodiscard]] int GetDurationMax() const { return dur_max_; }
	[[nodiscard]] const std::vector<Apply> &GetApplies() const { return applies_; }
	[[nodiscard]] const std::vector<EMobFlag> &GetBlockingMobFlags() const { return blocking_mob_flags_; }
	[[nodiscard]] const std::vector<EAffect> &GetBlockingAffectFlags() const { return blocking_affect_flags_; }
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
	Bitvector flags_{0};
	int dur_const_{0};
	int dur_level_divisor_{0};
	int dur_min_{0};
	int dur_max_{0};
	std::vector<Apply> applies_;
	// Flags that make the target wholly immune to this affect, checked before the cast
	// even tries to build affects / roll saves (the <blocking> tag, issue.aff-flagged-check).
	// Mob flags (EMobFlag) come from an NPC prototype, so they are tested for NPCs only;
	// affect flags (EAffect) may also be granted by equipment, so they are tested for any
	// target via AFF_FLAGGED.
	std::vector<EMobFlag> blocking_mob_flags_;
	std::vector<EAffect> blocking_affect_flags_;
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
// present together"). See CastUnaffects for the processing order.
class TalentUnaffect : public IAction {
 public:
	// One <blocking>/<breaking>/<remove_anyway>/<remove> entry: its any_of/all_of lists.
	struct Set {
		std::vector<ESpell> any_of;
		std::vector<ESpell> all_of;
		[[nodiscard]] bool empty() const { return any_of.empty() && all_of.empty(); }
	};

	explicit TalentUnaffect(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;

	[[nodiscard]] const Set &GetBlocking() const { return blocking_; }
	[[nodiscard]] const Set &GetBreaking() const { return breaking_; }
	[[nodiscard]] const Set &GetRemoveAnyway() const { return remove_anyway_; }
	[[nodiscard]] const Set &GetRemove() const { return remove_; }

 private:
	Set blocking_;       // present -> removal is blocked (chain not affected)
	Set breaking_;       // present -> the cast chain breaks (EStageResult::kBreak)
	Set remove_anyway_;  // dispelled even when blocking is true
	Set remove_;         // dispelled only when blocking is false
};

using ActionPtr = std::shared_ptr<IAction>;

class Actions {
	using ActionsRoster = std::unordered_multimap<EAction, ActionPtr>;
	using ActionsRosterPtr = std::unique_ptr<ActionsRoster>;
	ActionsRosterPtr actions_;

	static void ParseAction(ActionsRosterPtr &info, parser_wrapper::DataNode node);
	static void ParseDamage(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseArea(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseHeal(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseAffect(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseUnaffect(ActionsRosterPtr &info, parser_wrapper::DataNode &node);

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
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
