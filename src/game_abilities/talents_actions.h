/**
\authors Created by Sventovit
\date 5.06.2022.
\brief Модуль воздействий талантов.
\details Классы, описывающие активные воздействия различных талантов (заклинаний, умений, способностей) -
 урон, область применения, активные аффекты и т.п.
*/

#ifndef BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_
#define BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

#include "game_affects/affect_contants.h"
#include "game_abilities/abilities_constants.h"
#include "game_skills/skills.h"
#include "entities/entities_constants.h"
#include "utils/parser_wrapper.h"

class CharData;

namespace talents_actions {

class Roll {
 protected:
	double resist_weight_{0.0};
	ESkill base_skill_{ESkill::kUndefined};
	double low_skill_bonus_{0.0};
	double hi_skill_bonus_{0.0};
	EBaseStat base_stat_{EBaseStat::kFirst};
	int base_stat_threshold_{10};
	double base_stat_weight_{0.0};

	void ParseBaseSkill(parser_wrapper::DataNode &node);
	void ParseBaseStat(parser_wrapper::DataNode &node);

 public:
	explicit Roll() = default;
	virtual ~Roll() = default;
	virtual void ParseRoll(parser_wrapper::DataNode &node);
	[[nodiscard]] double ResistWeight() const { return resist_weight_; };
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const;
	virtual void Print(std::ostringstream &buffer) const;
};

class PowerRoll : public Roll {
 private:
	int dice_num_{1};
	int dice_size_{1};
	int dice_add_{1};

	void ParseDices(parser_wrapper::DataNode &node);

 public:
	explicit PowerRoll() = default;
	~PowerRoll() override = default;
	void ParseRoll(parser_wrapper::DataNode &node) override;
	[[nodiscard]] int RollDices() const;
	int DoRoll(const CharData *ch) const;
	void Print(std::ostringstream &buffer) const override;
};

class SuccessRoll : public Roll {
	int critsuccess_threshold_{abilities::kDefaultCritsuccessThreshold};
	int critfail_threshold_{abilities::kDefaultCritfailThreshold};
	int roll_bonus_{abilities::kDefaultDifficulty};
	int pvp_penalty_{abilities::kDefaultPvPPenalty};
	int pve_penalty_{abilities::kDefaultPvEPenalty};
	int evp_penalty_{abilities::kDefaultEvPPenalty};

 public:
	explicit SuccessRoll() = default;
	~SuccessRoll() override = default;
	void ParseRoll(parser_wrapper::DataNode &node) override;
	void Print(std::ostringstream &buffer) const override;
};
/*
enum class ETalentEffect : Bitvector {
	kAffect			= 0u,
	kDamage			= 1u << 0,
	kArea			= 1u << 1,
	kDuration		= 1u << 2,
	kPointsChange	= 1u << 3,
	kPosChange		= 1u << 4
};
*/
class Damage {
	PowerRoll power_roll_;
	ESaving saving_{ESaving::kReflex};

 public:
	explicit Damage(parser_wrapper::DataNode &node);
	[[nodiscard]] int RollDices() const { return power_roll_.RollDices(); };
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const { return power_roll_.CalcSkillCoeff(ch); };
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const { return power_roll_.CalcBaseStatCoeff(ch); };
	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class Affect {
	PowerRoll power_roll_;
	ESaving saving_{ESaving::kReflex};
	EApply location_{EApply::kNone};
	int mod_{0};
	int cap_{0};
	bool accumulate_{false};
	Bitvector flags_{0u};
	Bitvector appplies_bits_{0u};
	std::unordered_set<ESpell> removes_spell_affects_;
	std::unordered_set<ESpell> replaces_spell_affects_;
	std::unordered_set<EAffect> applies_affects_;
	std::unordered_set<EAffect> blocked_by_affects_;
	std::unordered_set<EMobFlag> blocked_by_mob_flags_;

 public:
	explicit Affect(parser_wrapper::DataNode &node);
	[[nodiscard]] ESaving Saving()			const { return saving_; }
	[[nodiscard]] EApply Location()			const { return location_; }
	[[nodiscard]] Bitvector AffectBits()	const { return appplies_bits_; }
	[[nodiscard]] int Modifier()			const { return mod_; }
	[[nodiscard]] int Cap()					const { return cap_; }
	[[nodiscard]] bool Accumulate()	const { return accumulate_; }
	[[nodiscard]] Bitvector Flags()			const { return flags_; }
	[[nodiscard]] const auto &RemovedASpellffects() const { return removes_spell_affects_; }
	[[nodiscard]] const auto &ReplacesSpellAffects() const { return replaces_spell_affects_; }
	[[nodiscard]] const auto &BlockingAffects() const { return blocked_by_affects_; }
	[[nodiscard]] const auto &BlockingMobFlags() const { return blocked_by_mob_flags_; }
	[[nodiscard]] int RollDices() const { return power_roll_.RollDices(); };
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const { return power_roll_.CalcSkillCoeff(ch); };
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const { return power_roll_.CalcBaseStatCoeff(ch); };
	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class Duration {
	SuccessRoll success_roll_;
	int min_{1};
	int cap_{1};
	bool accumulate_{false};

 public:
	Duration() = default;
	explicit Duration(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;
	[[nodiscard]] bool Accumulate() const { return accumulate_; };
};

class Area {
 public:
	double cast_decay{0.0};
	int level_decay{0};
	int free_targets{1};

	int skill_divisor{1};
	int targets_dice_size{1};
	int min_targets{1};
	int max_targets{1};

	explicit Area(parser_wrapper::DataNode &node);
	[[nodiscard]] int CalcTargetsQuantity(int skill_level) const;
	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class TalentAction {
 public:
	using Affects = std::vector<Affect>;

	explicit TalentAction(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;
	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] bool DoesDamage() const { return (damage_ == nullptr); };
	[[nodiscard]] const Area &GetArea() const;
	[[nodiscard]] bool DoesAreaEffect() const { return (area_ == nullptr); };
	[[nodiscard]] const Duration &GetDuration() const;
	[[nodiscard]] bool DoesDurableEffect() const { return (duration_ == nullptr); };
	[[nodiscard]] const Affects &GetAffects() const;
	[[nodiscard]] bool DoesAffects() const { return affects_.empty(); };

 private:
	Affects affects_;
	std::unique_ptr<Damage> damage_;
	std::unique_ptr<Area> area_;
	std::unique_ptr<Duration> duration_;
};

class Actions {
	using ActionsRoster = std::vector<TalentAction>;
	using ActionsRosterPtr = std::unique_ptr<ActionsRoster>;
	ActionsRosterPtr actions_;

	static void ParseSingeAction(ActionsRosterPtr &info, parser_wrapper::DataNode node);

 public:
	Actions() {
		actions_ = std::make_unique<ActionsRoster>();
	};

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] const Area &GetArea() const;
	[[nodiscard]] const Duration &GetDuration() const;
	[[nodiscard]] const TalentAction::Affects &GetAffects() const;
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
