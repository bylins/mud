/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных классам скиллах и спеллах должна лежать здесь.
*/

#ifndef BYLINS_SRC_CLASSES_CLASSES_INFO_H_
#define BYLINS_SRC_CLASSES_CLASSES_INFO_H_

#include "engine/boot/cfg_manager.h"
#include "classes_constants.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/magic/spells.h"
#include "gameplay/skills/skills.h"
#include "engine/structs/info_container.h"
#include "utils/grammar/cases.h"

#include <optional>

namespace grammar {
class ItemName;
}

namespace classes {

class ClassesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

class CharClassInfo : public info_container::BaseItem<ECharClass> {
 public:
	template<typename TalentIndexEnum>
	class TalentInfo : public info_container::BaseItem<TalentIndexEnum> {
	public:
		TalentInfo() = default;
		TalentInfo(TalentIndexEnum id, int min_level, int min_remort, EItemMode mode)
			: BaseItem<TalentIndexEnum>(id, mode), min_level_{min_level}, min_remort_{min_remort} {};

		[[nodiscard]] int GetMinLevel() const { return min_level_; };
		[[nodiscard]] int GetMinRemort() const { return min_remort_; };

	 protected:
		int min_level_{kMinCharLevel};
		int min_remort_{kMinRemort};
	};

	class SkillInfo : public TalentInfo<ESkill> {
	 public:
		SkillInfo() = default;
		SkillInfo(ESkill id, int min_level, int min_remort, long improve, EItemMode mode)
			: TalentInfo(id, min_level, min_remort, mode), improve_{improve} {};

		[[nodiscard]] long GetImprove() const { return improve_; };

	 private:
		long improve_{kMinImprove};
	};

	class SkillInfoBuilder : public info_container::IItemBuilder<SkillInfo> {
	 public:
		ItemPtr Build(parser_wrapper::DataNode &node) final;
	};

	using Skills = info_container::InfoContainer<ESkill, SkillInfo, SkillInfoBuilder>;

	class SpellInfo : public TalentInfo<ESpell> {
	 public:
		SpellInfo() = default;
		SpellInfo(ESpell id, int min_level, int min_remort, int circle, int mem_mod, double cast_mod, EItemMode mode)
			: TalentInfo(id, min_level, min_remort, mode), circle_{circle}, mem_mod_{mem_mod}, cast_mod_{cast_mod} {};

		[[nodiscard]] int GetCircle() const { return circle_; };
		[[nodiscard]] int GetMemMod() const { return mem_mod_; };
		[[nodiscard]] double GetCastMod() const { return cast_mod_; };

	 private:
		int circle_{kMaxMemoryCircle};
		int mem_mod_{0};
		double cast_mod_{0.0};
	};

	class SpellInfoBuilder : public info_container::IItemBuilder<SpellInfo> {
	 public:
		ItemPtr Build(parser_wrapper::DataNode &node) final;
	};

	using Spells = info_container::InfoContainer<ESpell, SpellInfo, SpellInfoBuilder>;

	class FeatInfo : public TalentInfo<EFeat> {
	 public:
		FeatInfo() = default;
		FeatInfo(EFeat id, int min_level, int min_remort, int slot, bool inborn, EItemMode mode)
			: TalentInfo(id, min_level, min_remort, mode), slot_{slot}, inborn_{inborn} {};

		[[nodiscard]] int GetSlot() const { return slot_; };
		[[nodiscard]] bool IsInborn() const { return inborn_; };

	 private:
		int slot_{0};
		bool inborn_{false};
	};

	class FeatInfoBuilder : public info_container::IItemBuilder<FeatInfo> {
	 public:
		ItemPtr Build(parser_wrapper::DataNode &node) final;
	};

	using Feats = info_container::InfoContainer<EFeat, FeatInfo, FeatInfoBuilder>;

// =====================================================================================================================

	CharClassInfo()
		: BaseItem<ECharClass>() {
		InnerInit();
	};

	CharClassInfo(ECharClass id, EItemMode mode)
		: BaseItem<ECharClass>(id, mode) {
		InnerInit();
	};

	/* Имена */
	std::unique_ptr<grammar::ItemName> names;
	std::string abbr;
	[[nodiscard]] const std::string &GetName(ECase name_case = ECase::kNom) const;
	[[nodiscard]] const std::string &GetPluralName(ECase name_case = ECase::kNom) const;
	[[nodiscard]] const std::string &GetAbbr() const;

	/**
	 *  Строка в C-стиле. По возможности используйте std::string.
	 */
	[[nodiscard]] const char *GetCName(ECase name_case = ECase::kNom) const;

	/**
	 *  Строка в C-стиле. По возможности используйте std::string.
	 */
	[[nodiscard]] const char *GetPluralCName(ECase name_case = ECase::kNom) const;

	/* Всякого рода таланты класса - скиллы, спеллы и проч. */
	Skills skills;
	int skill_level_decrement_{kMinTalentLevelDecrement};
	[[nodiscard]] int GetSkillLvlDecrement() const { return skill_level_decrement_; };
	void PrintSkillsTable(CharData *ch, std::ostringstream &buffer) const;

	Spells spells;
	int spell_level_decrement_{kMinTalentLevelDecrement};
	[[nodiscard]] int GetSpellLvlDecrement() const { return spell_level_decrement_; };
	[[nodiscard]] int GetMaxCircle() const;
	void PrintSpellsTable(CharData *ch, std::ostringstream &buffer) const;

	Feats feats;
	int remorts_for_feat_slot_{kMaxRemort};
	[[nodiscard]] int GetRemortsNumForFeatSlot() const { return remorts_for_feat_slot_; };
	void PrintFeatsTable(CharData *ch, std::ostringstream &buffer) const;

	/* базовые параметры */
	struct BaseStatLimits {
		int gen_min{kDefaultBaseStatMin};
		int gen_max{kDefaultBaseStatMax};
		int gen_auto{kDefaultBaseStatAutoGen};
		int cap{kDefaultBaseStatCap};
	};
	std::unordered_map<EBaseStat, BaseStatLimits> base_stats;
	void PrintBaseStatsTable(CharData *ch, std::ostringstream &buffer) const;
	auto GetBaseStatGenMin(EBaseStat stat_id) const { return base_stats.at(stat_id).gen_min; };
	auto GetBaseStatGenMax(EBaseStat stat_id) const { return base_stats.at(stat_id).gen_max; };
	auto GetBaseStatGenAuto(EBaseStat stat_id) const { return base_stats.at(stat_id).gen_auto; };
	auto GetBaseStatCap(EBaseStat stat_id) const { return base_stats.at(stat_id).cap; };

	/* вторичные параметры */
	/* Какая-то вторичная дичь, определяющая бонусы-штрафы */
	// \todo Нужно будет посмотреть, можно ли это переработать или вообще избавиться
	struct ClassApplies {
		ClassApplies() = default;
		ClassApplies(int koef_con, int base_con, int min_con, int max_con)
			: koef_con{koef_con}, base_con{base_con}, min_con{min_con}, max_con{max_con} {};
		int koef_con{0};
		int base_con{0};
		int min_con{0};
		int max_con{0};
	};
	ClassApplies applies;

	/* Прочее */
	void Print(CharData *ch, std::ostringstream &buffer) const;
	void PrintHeader(std::ostringstream &buffer) const;

 private:
	void InnerInit() {
		names = std::make_unique<grammar::ItemName>();
		for (auto stat = EBaseStat::kFirst; stat <= EBaseStat::kLast; ++stat) {
			base_stats[stat] = CharClassInfo::BaseStatLimits();
		}
	}
};

class CharClassInfoBuilder : public info_container::IItemBuilder<CharClassInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static parser_wrapper::DataNode SelectDataNode(parser_wrapper::DataNode &node);
	static std::optional<std::string> GetCfgFileName(parser_wrapper::DataNode &node);
	static ItemPtr ParseClass(parser_wrapper::DataNode &node);
	static void ParseStats(ItemPtr &info, parser_wrapper::DataNode &node);
	static void ParseBaseStats(ItemPtr &info, parser_wrapper::DataNode &node);
	static void ParseName(ItemPtr &info, parser_wrapper::DataNode &node);
	static void ParseSkills(ItemPtr &info, parser_wrapper::DataNode &node);
	static void ParseSpells(ItemPtr &info, parser_wrapper::DataNode &node);
	static void ParseFeats(ItemPtr &info, parser_wrapper::DataNode &node);
	// временная функция
	static void TemporarySetStat(ItemPtr &info);
};

using ClassesInfo = info_container::InfoContainer<ECharClass, CharClassInfo, CharClassInfoBuilder>;

} // namespace classes

#endif //BYLINS_SRC_CLASSES_CLASSES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
