/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_GUILDS_H_
#define BYLINS_SRC_GAME_MECHANICS_GUILDS_H_

#include <set>

#include "boot/cfg_manager.h"
#include "game_skills/skills.h"
#include "structs/info_container.h"

class CharData;

void init_guilds();
int guild_mono(CharData *ch, void *me, int cmd, char *argument);
int guild_poly(CharData *ch, void *me, int cmd, char *argument);


namespace guilds {

class Condition {};

class GuildsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class GuildInfo : public info_container::BaseItem<int> {
	friend class GuildInfoBuilder;
	class GuildTalent;

	std::string name_;
	std::set<MobVnum> trainers_;
	std::map<ESkill, Condition> taught_skills_;
//	std::map<ESpell, ConditionSRoster> taught_spells_;
//	std::map<EFeat, ConditionSRoster> taught_feats_;

 public:
	GuildInfo() = default;
	GuildInfo(int id, std::string &text_id, std::string &name, EItemMode mode)
	: BaseItem<int>(id, text_id, mode), name_{name} {};

	const std::string &GetName() { return name_; };
	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class GuildInfoBuilder : public info_container::IItemBuilder<GuildInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseGuild(parser_wrapper::DataNode node);
	static void ParseSkills(ItemPtr &info, parser_wrapper::DataNode &node);
};

using GuildsInfo = info_container::InfoContainer<int, GuildInfo, GuildInfoBuilder>;

class GuildInfo::GuildTalent {
	std::vector<Condition> conditions_roster_;
 public:
	virtual std::string_view GetDisplayStr() = 0;
	virtual bool CheckName(const std::string &talent_name) = 0;
	virtual bool SoftCheck(CharData *ch) = 0;
	virtual bool TryLearn(CharData *ch) = 0;
};

}
#endif //BYLINS_SRC_GAME_MECHANICS_GUILDS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
