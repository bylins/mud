#ifndef SKILLS_INFO_
#define SKILLS_INFO_

#include "skills.h"

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/classes/classes_constants.h"
#include "engine/structs/info_container.h"
#include "engine/structs/bitset_flags.h"

// Флаги умения: <skill flags="kHidden|kObligatory">. Имена -- NAME_BY_ITEM/ITEM_BY_NAME<ESkillFlag>.
enum class ESkillFlag {
	// issue.self-train: умение качается и при применении на себя, когда цели-моба нет вовсе.
	// Без флага самокаст прока не дает: TrainSkill пропускает только моба или пустую цель.
	kSelfTrain,
	// issue #2689: игрок умения не видит -- ни в списке умений, ни в сообщении о росте. Боги видят.
	kHidden,
	// issue #2689: умение растет и в мирной комнате (храмы почти все мирные) и в замке; обычные нет.
	kTrainInPeaceful,
	kTrainInHouse,
	// issue #2689: умение есть у каждого персонажа. Кому его не хватает, получает при входе в игру
	// с уровнем kObligatorySkillStartLevel; класс должен его перечислять, иначе оно не сохранится.
	kObligatory,
	// issue #2689: умение не сбрасывается при перевоплощении, даже при обязательном сбросе умений.
	kKeepOnRemort,
	kCount
};

template<>
const std::string &NAME_BY_ITEM<ESkillFlag>(ESkillFlag item);
template<>
ESkillFlag ITEM_BY_NAME<ESkillFlag>(const std::string &name);

/*
 * Загрузчик конфига умений.
 */
class SkillsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

/*
 * Класс-описание конкретного умения.
 */
class SkillInfo : public info_container::BaseItem<ESkill> {
 public:
	SkillInfo() = default;
	SkillInfo(ESkill id, EItemMode mode)
		: BaseItem<ESkill>(id, mode) {};

	std::string name{"!undefined!"};
	std::string short_name{"!error"};
	ESaving save_type{ESaving::kFirst};
	int difficulty{200};
	int cap{1000};
	bool autosuccess{false};
	BitsetFlags<ESkillFlag> flags;

	[[nodiscard]] bool IsFlagged(ESkillFlag flag) const { return flags.get(flag); }

	/*
	 * Имя скилла в виде C-строки. По возможности используйте std::string
	 */
	[[nodiscard]] const char *GetName() const { return name.c_str(); };
	[[nodiscard]] const char *GetAbbr() const { return short_name.c_str(); };
	void Print(std::ostringstream &buffer) const;
};

/*
 * Класс-билдер описания отдельного умения.
 */
class SkillInfoBuilder : public info_container::IItemBuilder<SkillInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseObligatoryValues(parser_wrapper::DataNode &node);
	static void ParseDispensableValues(ItemPtr &item_ptr, parser_wrapper::DataNode &node);
};

using SkillsInfo = info_container::InfoContainer<ESkill, SkillInfo, SkillInfoBuilder>;


#endif // SKILLS_INFO_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
