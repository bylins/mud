/**
\authors Created by Sventovit
\date 30.05.2022.
\brief Модуль эффектов талантов.
\details Классы, описывающие эффекты различных активных и пассивных талантов (заклинаний, умений, способностей) -
 урон, накладываемые аффекты, область применения и все такое прочее.
*/

#ifndef BYLINS_SRC_STRUCTS_EFFECT_H_
#define BYLINS_SRC_STRUCTS_EFFECT_H_

#include "game_affects/affect_contants.h" // возможно, апплаи стоит перенести как раз сбда..
#include "entities/entities_constants.h"
#include "utils/parser_wrapper.h"

namespace effects {

enum class EEffect {
	kDamage,
	kArea,
	kApply
};

class IEffect {
 public:
	virtual ~IEffect() = default;

	virtual void Print(std::ostringstream &buffer) const = 0;
};

struct Damage : public IEffect {
	ESaving saving{ESaving::kReflex};
	int dice_num{1};
	int dice_size{1};
	int dice_add{1};
	double low_skill_bonus{0.0};
	double hi_skill_bonus{0.0};

	void Print(std::ostringstream &buffer) const override;
};

struct Area : public IEffect {
	double cast_decay{0.0};
	int level_decay{0};
	int free_targets{1};

	int skill_divisor{1};
	int targets_dice_size{1};
	int min_targets{1};
	int max_targets{1};

	void Print(std::ostringstream &buffer) const override;
};

struct Applies : public IEffect {
	struct Mod {
		int mod{0};
		int cap{0};
		double lvl_bonus{0.0};
		double remort_bonus{0.0};

		Mod() = default;
		Mod(int mod, int cap, double lvl_bonus, double remort_bonus)
			: mod(mod), cap(cap), lvl_bonus(lvl_bonus), remort_bonus(remort_bonus) {};
	};

	using AppliesRoster = std::unordered_map<EApply, Mod>;
	AppliesRoster applies_roster;

	void Print(std::ostringstream &buffer) const override;
	void ImposeApplies(CharData *ch) const;
};

using EffectPtr = std::shared_ptr<IEffect>;

class Effects {
	using EffectsRoster = std::unordered_multimap<EEffect, EffectPtr>;
	using EffectsRosterPtr = std::unique_ptr<EffectsRoster>;
	EffectsRosterPtr effects_;

	static void ParseEffect(EffectsRosterPtr &info, parser_wrapper::DataNode node);
	static void ParseDamageEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseAreaEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseAppliesEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node);
 public:
	Effects() {
		effects_ = std::make_unique<EffectsRoster>();
	};

	void Build(parser_wrapper::DataNode &node);
	void Print(std::ostringstream &buffer) const;

	void ImposeApplies(CharData *ch) const;
	[[nodiscard]] std::optional<Damage> GetDmg() const;
	[[nodiscard]] std::optional<Area> GetArea() const;
};

}

#endif //BYLINS_SRC_STRUCTS_EFFECT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
