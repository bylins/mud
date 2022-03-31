#ifndef FEATURES_ITEMSET_HPP_INCLUDED_
#define FEATURES_ITEMSET_HPP_INCLUDED_

/*
	Класс наборов экипировки, требующейся для выполнения приема.
*/

#include "entities/obj_data.h"
#include "game_skills/skills.h"
#include "structs/structs.h"
#include "utils/utils.h"

struct TechniqueItem {
	int wear_position;
	EObjType type;
	ESkill skill;
	bool flagged;
	EObjFlag flag;

	// TODO: Добавить учет типов ударов (уколол и проч).
	bool operator==(const ObjData *item) const {
		return (item
			&& (type == GET_OBJ_TYPE(item))
			&& ((skill == ESkill::kAny) || (skill == static_cast<ESkill>(item->get_skill())))
			&& (flagged ? item->has_flag(flag) : true));
	};

	TechniqueItem() :
		wear_position{-1},
		type{EObjType::ITEM_UNDEFINED},
		skill{ESkill::kAny},
		flagged{false},
		flag{EObjFlag::kGlow} {};

	TechniqueItem(int wear_position, EObjType obj_type) :
		wear_position{wear_position}, skill{ESkill::kAny}, flagged{false}, flag{EObjFlag::kGlow} {
		type = obj_type;
	};
	TechniqueItem(int wear_position, EObjType obj_type, ESkill obj_skill) :
		TechniqueItem(wear_position, obj_type) {
		skill = obj_skill;
	};
	TechniqueItem(int wear_position, EObjType obj_type, ESkill obj_skill, EObjFlag extra_flag) :
		TechniqueItem(wear_position, obj_type, obj_skill) {
		flag = extra_flag;
		flagged = true;
	};
};

using TechniqueItemKit = std::vector<TechniqueItem>;
using TechniqueItemKitPtr = std::unique_ptr<TechniqueItemKit>;
using TechniqueItemKitsGroup = std::vector<TechniqueItemKitPtr>;

#endif // FEATURES_ITEMSET_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
