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
#include "game_skills/skills.h"
#include "entities/entities_constants.h"
#include "utils/parser_wrapper.h"

class CharData;

namespace talents_actions {

class Roll {
 private:
	ESkill base_skill_{ESkill::kUndefined};
	double low_skill_bonus_{0.0};
	double hi_skill_bonus_{0.0};
	EBaseStat base_stat_{EBaseStat::kFirst};
	int base_stat_threshold_{10};
	double base_stat_weight_{0.0};
	int dice_num_{1};
	int dice_size_{1};
	int dice_add_{1};

	void ParseDices(parser_wrapper::DataNode &node);
	void ParseBaseSkill(parser_wrapper::DataNode &node);
	void ParseBaseStat(parser_wrapper::DataNode &node);

 public:
	explicit Roll() = default;
	~Roll() = default;
	void ParseRoll(parser_wrapper::DataNode &node);
	[[nodiscard]] int RollDices() const;
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const;
	void Print(std::ostringstream &buffer) const;
};

enum class ETalentEffect : Bitvector {
	kAffect = 0u,
	kDamage = 1u << 0,
	kArea = 1u << 1,
	kPointsChange = 1u << 2,
	kPosChange = 1u << 3
};

class TalentEffect {
 private:
	Roll power_roll_;

 protected:
	TalentEffect() = default;
	explicit TalentEffect(parser_wrapper::DataNode &node);

 public:
	virtual ~TalentEffect() = default;
	void ParseBaseFields(parser_wrapper::DataNode &node);
	[[nodiscard]] int RollDices() const { return power_roll_.RollDices(); };
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const { return power_roll_.CalcSkillCoeff(ch); };
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const { return power_roll_.CalcBaseStatCoeff(ch); };
	virtual void Print(CharData *ch, std::ostringstream &buffer) const;
};

class Damage : public TalentEffect {
	ESaving saving_{ESaving::kReflex};

 public:
	explicit Damage(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

class Duration : public TalentEffect {
	int min_{1};
	int cap_{1};
	bool accumulate_{false};

 public:
	Duration() = default;
	explicit Duration(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

class Affect : public TalentEffect {
	ESaving saving_{ESaving::kReflex};
	Duration duration_;
	EApply location_{EApply::kNone};
	EAffect affect_{EAffect::kUndefinded};
	int mod_{0};
	int cap_{0};
	bool accumulate_{false};

 public:
	explicit Affect(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

class Area : public TalentEffect {
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
	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

//class TalentAction {
//	Roll success_roll_;
//	ESaving saving_{ESaving::kReflex};
//
// public:
//	explicit TalentAction(parser_wrapper::DataNode &node);
//};

class Actions {
	using EffectPtr = std::shared_ptr<TalentEffect>;
	using ActionsRoster = std::unordered_multimap<ETalentEffect, EffectPtr>;
	using ActionsRosterPtr = std::unique_ptr<ActionsRoster>;
	ActionsRosterPtr actions_;

	static void ParseAction(ActionsRosterPtr &info, parser_wrapper::DataNode node);

 public:
	Actions() {
		actions_ = std::make_unique<ActionsRoster>();
	};

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] const Area &GetArea() const;
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
