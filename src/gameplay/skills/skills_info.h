#ifndef SKILLS_INFO_
#define SKILLS_INFO_

#include "skills.h"

#include "engine/boot/cfg_manager.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/classes/classes_constants.h"
#include "engine/structs/info_container.h"

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
