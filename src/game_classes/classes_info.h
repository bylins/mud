/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных классам скиллах и спеллах должна лежать здесь.
*/

#ifndef BYLINS_SRC_CLASSES_CLASSES_INFO_H_
#define BYLINS_SRC_CLASSES_CLASSES_INFO_H_

#include "boot/cfg_manager.h"
#include "game_classes/classes_constants.h"
#include "game_magic/spells.h"
#include "game_skills/skills.h"
#include "structs/info_container.h"

#include <optional>

namespace base_structs {
class ItemName;
}

namespace classes {

class ClassesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

struct CharClassInfo : public info_container::IItem<ECharClass> {

	template<class E, class T>
	class ClassTalentInfo {
	public:
		ClassTalentInfo() = delete;
		ClassTalentInfo(E id, int min_level, int min_remort)
			: id_{id}, min_level_{min_level}, min_remort_{min_remort} {};

		using Ptr = std::unique_ptr<T>;
		[[nodiscard]] bool HasItem(E item_id) const { return talents_.contains(item_id); };
		[[nodiscard]] bool HasNoItem(E item_id) const { return !HasItem(item_id); };

		[[nodiscard]] int GetMinRemort(E item_id) const {
			try {
				return talents_.at(item_id)->min_remort_;
			} catch (const std::out_of_range &) {
				return kMaxRemort + 1;
			}
		};

		[[nodiscard]] int GetMinLevel(E item_id) const {
			try {
				return talents_.at(item_id)->min_level_;
			} catch (const std::out_of_range &) {
				return kLvlImplementator;
			}
		};

	 protected:
		static inline std::unordered_map<E, Ptr> talents_;

		E id_{E::kIncorrect};
		int min_level_{kLvlImplementator};
		int min_remort_{kMaxRemort + 1};
	};

	class ClassSkillInfo : public ClassTalentInfo<ESkill, ClassSkillInfo> {
	 public:
		ClassSkillInfo() = delete;
		ClassSkillInfo(ESkill id, int min_level, int min_remort, long improve)
			: ClassTalentInfo(id, min_level, min_remort),
			improve_{improve} {
			talents_.try_emplace(id, std::make_unique<ClassSkillInfo>(*this));
		};

		[[nodiscard]] static long GetImprove(ESkill skill_id) ;
		[[nodiscard]] static int GetLevelDecrement() { return level_decrement_; };

	 private:
		long improve_{kMinImprove};
		static int level_decrement_;
	};

	class ClassSpellInfo : public ClassTalentInfo<ESpell, ClassSpellInfo> {
	 public:
		ClassSpellInfo() = delete;
		ClassSpellInfo(ESpell id, int min_level, int min_remort, int circle, int mem_mod, int cast_mod)
			: ClassTalentInfo(id, min_level, min_remort),
			circle_{circle}, mem_mod_{mem_mod}, cast_mod_{cast_mod} {
			talents_.try_emplace(id, std::make_unique<ClassSpellInfo>(*this));
		};

		[[nodiscard]] int GetCircle() const { return circle_; }; // \todo Не забыть реализовать
		[[nodiscard]] int GetMemMod() const { return mem_mod_; };
		[[nodiscard]] int GetCastMod() const { return cast_mod_; };

	 private:
		int circle_{kMaxMemoryCircle};
		int mem_mod_{0};
		int cast_mod_{0};
	};

	CharClassInfo() {
		names = std::make_unique<base_structs::ItemName>();
	}

	/* Базовые поля */
	ECharClass id{ECharClass::kFirst};
	EItemMode mode{EItemMode::kEnabled};

	[[nodiscard]] ECharClass GetId() const final { return id; };
	[[nodiscard]] EItemMode GetMode() const final { return mode; };

	/* Имена */
	std::unique_ptr<base_structs::ItemName> names;
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

	ClassSkillInfo skills;
	ClassSpellInfo spells;

	/* Прочее */
	void Print(std::stringstream &buffer) const;
};

class CharClassInfoBuilder : public info_container::IItemBuilder<CharClassInfo> {
 public:
	ItemOptional Build(parser_wrapper::DataNode &node) final;
 private:
	static parser_wrapper::DataNode SelectDataNode(parser_wrapper::DataNode &node);
	static std::optional<std::string> GetCfgFileName(parser_wrapper::DataNode &node);
	static void ParseClass(ItemOptional &info, parser_wrapper::DataNode &node);
	static void ParseName(ItemOptional &info, parser_wrapper::DataNode &node);
	static void ParseSkills(ItemOptional &info, parser_wrapper::DataNode &node);
	static int ParseSkillsLevelDecrement(ECharClass class_id, parser_wrapper::DataNode &node);
	static void ParseSingleSkill(ItemOptional &info, parser_wrapper::DataNode &node);
	static std::optional<ESkill> ParseSkillId(parser_wrapper::DataNode &node);
	static std::tuple<int, int, long> ParseSkillVals(ESkill id, parser_wrapper::DataNode &node);
};

using ClassesInfo = info_container::InfoContainer<ECharClass, CharClassInfo, CharClassInfoBuilder>;

} // namespace classes

#endif //BYLINS_SRC_CLASSES_CLASSES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
