/**
\authors Created by Sventovit
\date 26.01.2022.
\brief Модуль информации о классах персонажей.
\details Вся информация о лимитах статов, доступных скиллах и спеллах должна лежать здесь.
*/

#ifndef BYLINS_SRC_CLASSES_CLASSES_INFO_H_
#define BYLINS_SRC_CLASSES_CLASSES_INFO_H_

#include "skills_info.h"
#include "structs/structs.h"

#include <array>

namespace classes {

class CharClassInfo {
 private:
/*	struct ClassSkillInfo;
	using ClassSkillInfoPtr = std::unique_ptr<ClassSkillInfo>;*/

 public:
	/* Skill's mothods */
	int GetMinRemort(const ESkill /*id*/) const { return 0; };
	int GetMinLevel(const ESkill /*id*/) const { return 1; };
	int GetImprove(const ESkill /*id*/) const { return 1; };
	int GetLevelDecrement(const ESkill /*id*/) const { return 1; };
	bool Knows(const ESkill /*id*/) const { return true; };
	bool NotKnows(const ESkill /*id*/) const { return false; };
};

/*struct ClassSkillInfo {
	int min_level{kMaxPlayerLevel};
	int min_remort{kMaxRemort};
	int level_decrement{0};
	long improve{1};
};*/

class ClassesInfo {
 public:
	void Init();
	const CharClassInfo &operator[](ECharClass class_id) const;

 private:
	using CharClassPtr = std::unique_ptr<CharClassInfo>;
	using CharClassesRegister = std::unordered_map<ECharClass, CharClassPtr>;

	CharClassesRegister classes_;

	void InitCharClass(ECharClass class_id);
};

} // namespace classes

#endif //BYLINS_SRC_CLASSES_CLASSES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
