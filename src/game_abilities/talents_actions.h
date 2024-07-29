/**
\authors Created by Sventovit
\date 5.06.2022.
\brief Модуль воздействий талантов.
\details Классы, описывающие активные воздействия различных талантов (заклинаний, умений, способностей) -
 урон, область применения, активные аффекты и т.п.
*/

#ifndef BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_
#define BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

#include "game_skills/skills.h"
#include "entities/entities_constants.h"
#include "utils/parser_wrapper.h"

class CharData;

namespace talents_actions {

enum class EAction {
	kDamage,
	kArea,
	kHeal
};

class IAction {
 public:
	virtual ~IAction() = default;

	virtual void Print(CharData *ch, std::ostringstream &buffer) const = 0;
};

class Damage : public IAction {
	int dice_num_{1};
	int dice_size_{1};
	int dice_add_{1};

	ESkill base_skill_{ESkill::kUndefined};
	double low_skill_bonus_{0.0};
	double hi_skill_bonus_{0.0};

	EBaseStat base_stat_{EBaseStat::kFirst};
	int base_stat_threshold_{10};
	double base_stat_weight_{0.0};

	ESaving saving_{ESaving::kReflex};
 public:
	explicit Damage(parser_wrapper::DataNode &node);

	[[nodiscard]] int RollSkillDices() const;
	[[nodiscard]] double CalcSkillCoeff(const CharData *ch) const;
	[[nodiscard]] double CalcBaseStatCoeff(const CharData *ch) const;

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

class Heal : public	Damage {
public:
	explicit Heal(parser_wrapper::DataNode &node);
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

using ActionPtr = std::shared_ptr<IAction>;

class Actions {
	using ActionsRoster = std::unordered_multimap<EAction, ActionPtr>;
	using ActionsRosterPtr = std::unique_ptr<ActionsRoster>;
	ActionsRosterPtr actions_;

	static void ParseAction(ActionsRosterPtr &info, parser_wrapper::DataNode node);
	static void ParseDamage(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseArea(ActionsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseHeal(ActionsRosterPtr &info, parser_wrapper::DataNode &node);

 public:
	Actions() {
		actions_ = std::make_unique<ActionsRoster>();
	};

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

	[[nodiscard]] const Damage &GetDmg() const;
	[[nodiscard]] const Area &GetArea() const;
	[[nodiscard]] const Heal &GetHeal() const;
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_ACTIONS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
