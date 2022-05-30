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

namespace effects {

enum class EEffect {
	kDamage,
	kArea
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

struct Apply : public IEffect {
	std::unordered_map<EApply, int> applies;

	void Print(std::ostringstream &buffer) const override;
};

using EffectPtr = std::shared_ptr<IEffect>;
using Effects = std::unordered_map<EEffect, EffectPtr>;

}

#endif //BYLINS_SRC_STRUCTS_EFFECT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
