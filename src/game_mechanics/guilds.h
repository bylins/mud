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
#include "utils/table_wrapper.h"

class CharData;

int DoGuildLearn(CharData *ch, void *me, int cmd, char *argument);

namespace guilds {

using DataNode = parser_wrapper::DataNode;

class GuildsLoader : virtual public cfg_manager::ICfgLoader {
	static void AssignGuildsToTrainers();
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class GuildInfo : public info_container::BaseItem<int> {
	friend class GuildInfoBuilder;

	enum class ETalent { kSkill, kSpell, kFeat };
	enum class EGuildMsg {
		kGreeting,
		kCannotToChar,
		kCannotToRoom,
		kAskToChar,
		kAskToRoom,
		kLearnToChar,
		kLearnToRoom,
		kInquiry,
		kDidNotTeach,
		kAllSkills,
		kTalentEarned,
		kNothingLearned,
		kListEmpty,
		kIsInsolvent,
		kFree,
		kError};

	class IGuildTalent {
		ETalent talent_type_;
		Vnum currency_vnum_{0};
		int start_price_{0};
		int remort_percemt_{0};
		std::unordered_set<ECharClass> trained_classes_;

		public:
		explicit IGuildTalent(ETalent talent_type, DataNode &node);

		[[nodiscard]] ETalent GetTalentType() { return talent_type_; };
		[[nodiscard]] bool IsLearnable(CharData *ch) const;
		[[nodiscard]] bool IsUnlearnable(CharData *ch) const { return !IsLearnable(ch); };
		[[nodiscard]] std::string GetClassesList() const;
		[[nodiscard]] bool TakePayment(CharData *ch) const;
		[[nodiscard]] long CalcPrice(CharData *buyer) const;
		[[nodiscard]] std::string GetPriceCurrencyStr(uint64_t price) const;

		[[nodiscard]] virtual bool IsAvailable(CharData *ch) const = 0;
		[[nodiscard]] virtual const std::string &GetIdAsStr() const = 0;
		[[nodiscard]] virtual std::string_view GetName() const = 0;
		[[nodiscard]] virtual std::string GetTalentTypeName() const = 0;
		virtual void SetTalent(CharData *ch) const = 0;
	};

	class GuildSkill : public IGuildTalent {
		ESkill id_{ESkill::kUndefined};
		int amount_{10};
		int min_skill_{0};
		int max_skill_{10};

		void ParseSkillNode(DataNode &node);
	 public:
		explicit GuildSkill(DataNode &node)
			: IGuildTalent(ETalent::kSkill, node) {
			ParseSkillNode(node);
		};

		[[nodiscard]] ESkill GetId() const { return id_; };
		[[nodiscard]] bool IsAvailable(CharData *ch) const final;
		[[nodiscard]] const std::string &GetIdAsStr() const final;
		[[nodiscard]] std::string_view GetName() const final;
		[[nodiscard]] std::string GetTalentTypeName() const final { return "умение"; };
		void SetTalent(CharData *ch) const final;
	};

	class GuildSpell : public IGuildTalent {
		ESpell id_{ESpell::kUndefined};
		ESpellType spell_type_{ESpellType::kKnow};
		uint32_t spell_time_{1};

		void ParseSpellNode(DataNode &node);
	 public:
		explicit GuildSpell(DataNode &node)
		: IGuildTalent(ETalent::kSpell, node) {
			ParseSpellNode(node);
		};

		[[nodiscard]] ESpell GetId() const { return id_; };
		[[nodiscard]] bool IsAvailable(CharData *ch) const final;
		[[nodiscard]] const std::string &GetIdAsStr() const final;
		[[nodiscard]] std::string_view GetName() const final;
		[[nodiscard]] std::string GetTalentTypeName() const final { return "заклинание"; };
		void SetTalent(CharData *ch) const final;
	};

	class GuildFeat : public IGuildTalent {
		EFeat id_{EFeat::kUndefined};

		void ParseFeatNode(DataNode &node);
	 public:
		explicit GuildFeat(DataNode &node)
		: IGuildTalent(ETalent::kFeat, node) {
			ParseFeatNode(node);
		};

		[[nodiscard]] EFeat GetId() const { return id_; };
		[[nodiscard]] bool IsAvailable(CharData *ch) const final;
		[[nodiscard]] const std::string &GetIdAsStr() const final;
		[[nodiscard]] std::string_view GetName() const final;
		[[nodiscard]] std::string GetTalentTypeName() const final { return "способность"; };
		void SetTalent(CharData *ch) const final;
	};

	using TalentPtr = std::unique_ptr<IGuildTalent>;
	using TalentsRoster = std::vector<TalentPtr>;

	std::string name_;
	std::unordered_set<MobVnum> trainers_;
	TalentsRoster learning_talents_;

	static void Learn(CharData *trainer, CharData *ch, const TalentPtr &talent);
	[[nodiscard]] static const std::string &GetMessage(EGuildMsg msg_id);
	[[nodiscard]] static bool ProcessPayment(CharData *trainer, CharData *ch, const TalentPtr &talent);
	static void LearnSingle(CharData *trainer, CharData *ch, const TalentPtr &talent);

	const std::string &GetName() const { return name_; };
	void DisplayMenu(CharData *trainer, CharData *ch) const;
	void LearnAll(CharData *trainer, CharData *ch) const;
	void LearnWithTalentNum(CharData *trainer, CharData *ch, std::size_t talent_num) const;
	void LearnWithTalentName(CharData *trainer, CharData *ch, const std::string &talent_name) const;

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
