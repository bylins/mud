/**
\authors Created by Sventovit
\date 30.05.2022.
\brief Модуль эффектов талантов.
\details Классы, описывающие пасивные эффекты различных талантов (заклинаний, умений, способностей) -
 бонусы к умениям, таймерам, пассивные аффекты и т.п.
*/

#ifndef BYLINS_SRC_STRUCTS_TALENTS_EFFECTS_H_
#define BYLINS_SRC_STRUCTS_TALENTS_EFFECTS_H_

#include "feats_constants.h"
#include "game_skills/skills.h"
#include "utils/parser_wrapper.h"

namespace talents_effects {

class Effects {
 public:
	Effects();
	~Effects();

	void ImposeApplies(CharData *ch) const;
	void ImposeSkillsMods(CharData *ch) const;
	[[nodiscard]] int GetTimerMod(ESkill skill_id) const;
	[[nodiscard]] int GetTimerMod(EFeat feat_id) const;

	void Build(parser_wrapper::DataNode &node);
	void Print(CharData *ch, std::ostringstream &buffer) const;

 private:
	class PassiveEffectsImpl;
	std::unique_ptr<PassiveEffectsImpl> pimpl_{nullptr};
};

}

#endif //BYLINS_SRC_STRUCTS_TALENTS_EFFECTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
