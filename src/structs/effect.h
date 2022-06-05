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
#include "feats_constants.h"
#include "game_skills/skills.h"
#include "entities/entities_constants.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"
#include "utils/table_wrapper.h"

namespace effects {

enum class EEffect {
	kDamage,
	kArea
};

// ====================================================================================================================
// ====================================================================================================================
// ====================================================================================================================

class PassiveEffects {
 public:
	using SkillMods = std::map<ESkill, int>;

	PassiveEffects();
	~PassiveEffects();

	void ImposeApplies(CharData *ch) const;
	[[nodiscard]] std::optional<SkillMods> GetSkillMods() const;
	[[nodiscard]] int GetSkillMod(ESkill skill_id) const;
	[[nodiscard]] int GetTimerMod(ESkill skill_id) const;
	[[nodiscard]] int GetTimerMod(EFeat feat_id) const;

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

 private:
	class PassiveEffectsImpl;
	std::unique_ptr<PassiveEffectsImpl> pimpl_{nullptr};
};

// ====================================================================================================================
// ====================================================================================================================
// ====================================================================================================================

class IEffect {
 public:
	virtual ~IEffect() = default;

	virtual void Print(CharData *ch, std::ostringstream &buffer) const = 0;
};

struct Damage : public IEffect {
	ESaving saving{ESaving::kReflex};
	int dice_num{1};
	int dice_size{1};
	int dice_add{1};
	double low_skill_bonus{0.0};
	double hi_skill_bonus{0.0};

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

struct Area : public IEffect {
	double cast_decay{0.0};
	int level_decay{0};
	int free_targets{1};

	int skill_divisor{1};
	int targets_dice_size{1};
	int min_targets{1};
	int max_targets{1};

	void Print(CharData *ch, std::ostringstream &buffer) const override;
};

using EffectPtr = std::shared_ptr<IEffect>;

class Effects {
	using EffectsRoster = std::unordered_multimap<EEffect, EffectPtr>;
	using EffectsRosterPtr = std::unique_ptr<EffectsRoster>;
	EffectsRosterPtr effects_;

	static void ParseEffect(EffectsRosterPtr &info, parser_wrapper::DataNode node);
	static void ParseDamageEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node);
	static void ParseAreaEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node);

 public:
	Effects() {
		effects_ = std::make_unique<EffectsRoster>();
	};

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

	[[nodiscard]] std::optional<Damage> GetDmg() const;
	[[nodiscard]] std::optional<Area> GetArea() const;
};

}

#endif //BYLINS_SRC_STRUCTS_EFFECT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
