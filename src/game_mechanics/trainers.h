/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_TRAINERS_H_
#define BYLINS_SRC_GAME_MECHANICS_TRAINERS_H_

#include "boot/cfg_manager.h"
//#include "game_skills/skills.h"
//#include "structs/info_container.h"

class CharData;

void init_guilds();
int guild_mono(CharData *ch, void *me, int cmd, char *argument);
int guild_poly(CharData *ch, void *me, int cmd, char *argument);
/*
namespace trainers {

class ConditionSRoster {};

class TrainersLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class TrainerInfo : public info_container::IItem<int> {
	int id_{-1};
	EItemMode mode_{EItemMode::kDisabled};

	std::map<ESkill, ConditionSRoster> taught_skills_;
//	std::map<ESpell, ConditionSRoster> taught_spells_;
//	std::map<EFeat, ConditionSRoster> taught_feats_;

 public:

	[[nodiscard]] int GetId() const final { return id_; };
	[[nodiscard]] EItemMode GetMode() const final { return mode_; };
};

class TrainerInfoBuilder : public info_container::IItemBuilder<int> {
 public:
	ItemOptional Build(parser_wrapper::DataNode &node) final;
 private:
	static void ParseTrainer(ItemOptional &info, parser_wrapper::DataNode &node);
};

using TrainersInfo = info_container::InfoContainer<int, TrainerInfo, TrainerInfoBuilder>;

}
*/
#endif //BYLINS_SRC_GAME_MECHANICS_TRAINERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
