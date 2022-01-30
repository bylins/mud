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

struct ClassSkillInfo {
	int min_level{kMaxPlayerLevel};
	int min_remort{kMaxRemort};
	long improve{1};
};

struct CharClassInfo {
	CharClassInfo() {
		skillls_info_ = std::make_unique<ClassSkillInfoRegister>();
	}
	CharClassInfo(CharClassInfo &s) = delete;
	void operator=(const CharClassInfo &s) = delete;

	/* Skills methods */
	bool IsKnown(const ESkill id) const;
	bool IsUnknonw(const ESkill id) const { return !IsKnown(id); };
	int GetMinRemort(const ESkill /*id*/) const { return 0; };
	int GetMinLevel(const ESkill /*id*/) const { return 1; };
	int GetImprove(const ESkill /*id*/) const { return 1; };
	int GetLevelDecrement(const ESkill /*id*/) const { return 1; };

	using ClassSkillInfoPtr = std::unique_ptr<ClassSkillInfo>;
	using ClassSkillInfoRegister = std::unordered_map<ESkill, ClassSkillInfoPtr>;
	using ClassSkillInfoRegisterPtr = std::unique_ptr<ClassSkillInfoRegister>;

	ClassSkillInfoRegisterPtr skillls_info_;
	int skills_level_decrement_{0};
};

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
