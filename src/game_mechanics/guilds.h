/**
\authors Created by Sventovit
\date 14.05.2022.
\brief Модуль механики учителей умений/заклинаний/способностей.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_GUILDS_H_
#define BYLINS_SRC_GAME_MECHANICS_GUILDS_H_

#include <set>
#include <feats.h>

#include "boot/cfg_manager.h"
#include "game_skills/skills.h"
#include "structs/info_container.h"

class CharData;

int DoGuildLearn(CharData *ch, void *me, int cmd, char *argument);

namespace guilds {

class ConditionsRoster {};

class GuildsLoader : virtual public cfg_manager::ICfgLoader {
	static void AssignGuildsToTrainers();
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class GuildInfo : public info_container::BaseItem<int> {
	friend class GuildInfoBuilder;

	enum class ETalent { kSkill, kSpell, kFeat };
	enum class EGuildMsg { kGreeting, kCannot, kSkill, kSpell, kFeat, kError };

	class IGuildTalent {
		ETalent talent_type_;
		public:
		explicit IGuildTalent(ETalent talent_type)
			: talent_type_(talent_type) {};

		[[nodiscard]] ETalent GetTalentType() { return talent_type_; };
		[[nodiscard]] virtual const std::string &GetIdAsStr() const = 0;
		[[nodiscard]] virtual std::string_view GetName() const = 0;
	};

	class GuildSkill : public IGuildTalent {
		ESkill id_;
	 public:
		explicit GuildSkill(ETalent talent_type, ESkill id)
			: IGuildTalent(talent_type), id_(id) {};

		[[nodiscard]] ESkill GetId() const { return id_; };
		[[nodiscard]] const std::string &GetIdAsStr() const final;
		[[nodiscard]] std::string_view GetName() const final;
	};

	class GuildSpell : public IGuildTalent {
		ESpell id_;
	 public:
		explicit GuildSpell(ETalent talent_type, ESpell id)
		: IGuildTalent(talent_type), id_(id) {};

		[[nodiscard]] ESpell GetId() const { return id_; };
		[[nodiscard]] const std::string &GetIdAsStr() const final;
		[[nodiscard]] std::string_view GetName() const final;
	};

	class GuildFeat : public IGuildTalent {
		EFeat id_;
	 public:
		explicit GuildFeat(ETalent talent_type, EFeat id)
		: IGuildTalent(talent_type), id_(id) {};

		[[nodiscard]] EFeat GetId() const { return id_; };
		[[nodiscard]] const std::string &GetIdAsStr() const final;
		[[nodiscard]] std::string_view GetName() const final;
	};

	using TalentPtr = std::unique_ptr<IGuildTalent>;
	using TalentsRoster = std::vector<TalentPtr>;

	std::string name_;
	std::set<MobVnum> trainers_;
	TalentsRoster learning_talents_;

	const std::string &GetName() { return name_; };
	static const std::string & GetMessage(EGuildMsg msg_id);
	void DisplayMenu(CharData *trainer, CharData *ch) const;

 public:
	GuildInfo() = default;
	GuildInfo(int id, std::string &text_id, std::string &name, EItemMode mode)
	: BaseItem<int>(id, text_id, mode), name_{name} {};

	void Print(CharData *ch, std::ostringstream &buffer) const;
	void AssignToTrainers() const;

	void Process(CharData *trainer, CharData *ch, const std::string &argument) const;
};

class GuildInfoBuilder : public info_container::IItemBuilder<GuildInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseGuild(parser_wrapper::DataNode node);
	static void ParseTalents(ItemPtr &info, parser_wrapper::DataNode &node);
};

using GuildsInfo = info_container::InfoContainer<int, GuildInfo, GuildInfoBuilder>;

}
#endif //BYLINS_SRC_GAME_MECHANICS_GUILDS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
