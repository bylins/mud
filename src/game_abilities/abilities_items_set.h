#ifndef FEATURES_ITEMSET_HPP_INCLUDED_
#define FEATURES_ITEMSET_HPP_INCLUDED_

/*
	Класс наборов экипировки, требующейся для выполнения приема.
*/

#include "game_skills/skills.h"
#include "entities/entities_constants.h"

class ObjData;

struct TechniqueItem {
	EEquipPos wear_position{EEquipPos::kLight};
	EObjType type{EObjType::kItemUndefined};
	ESkill skill{ESkill::kAny};
	bool flagged{false};
	EObjFlag flag{EObjFlag::kGlow};

	TechniqueItem() = default;
	TechniqueItem(EEquipPos wear_position, EObjType obj_type)
		: wear_position{wear_position}, type(obj_type) {};
	TechniqueItem(EEquipPos wear_position, EObjType obj_type, ESkill obj_skill)
		: wear_position{wear_position}, type(obj_type), skill(obj_skill) {};
	TechniqueItem(EEquipPos wear_position, EObjType obj_type, ESkill obj_skill, EObjFlag extra_flag)
		: wear_position{wear_position}, type(obj_type), skill(obj_skill), flagged(true), flag(extra_flag) {};

	bool operator==(const ObjData *item) const;
};

using TechniqueItemKit = std::vector<TechniqueItem>;
using TechniqueItemKitPtr = std::unique_ptr<TechniqueItemKit>;
using TechniqueItemKitsGroup = std::vector<TechniqueItemKitPtr>;

#endif // FEATURES_ITEMSET_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
