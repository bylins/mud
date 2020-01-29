#ifndef _FEATURES_ITEMSET_HPP_INCLUDED_
#define _FEATURES_ITEMSET_HPP_INCLUDED_

/*
	Класс наборов экипировки, требующейся для выполнения приема.
*/

#include "obj.hpp"
#include "skills.h"
#include "structs.h"
#include "utils.h"

struct TechniqueItem {
	short _wearPosition;
	OBJ_DATA::EObjectType _type;
	ESkill _skill;
	bool _flagged;
	EExtraFlag _flag;

	// TODO: Добавить учет типов ударов (уколол и проч).
	bool operator== (const OBJ_DATA *item) const {
		return (item
				&& (_type == GET_OBJ_TYPE(item))
				&& ((_skill == SKILL_INDEFINITE) || (_skill == GET_OBJ_SKILL(item)))
				&& (_flagged && OBJ_FLAGGED(item, _flag)));
	};

	TechniqueItem():
		_wearPosition{-1},
		_type{OBJ_DATA::ITEM_UNDEFINED},
		_skill{SKILL_INDEFINITE},
		_flagged{false},
		_flag{EExtraFlag::ITEM_GLOW}
		{};

	TechniqueItem(short objectWearPosition, OBJ_DATA::EObjectType objectType):
		_wearPosition{objectWearPosition} {
			_type = objectType;
		};
	TechniqueItem(short objectWearPosition, OBJ_DATA::EObjectType objectType, ESkill objectSkill):
		TechniqueItem(objectWearPosition, objectType) {
			_skill = objectSkill;
		};
	TechniqueItem(short objectWearPosition, OBJ_DATA::EObjectType objectType, ESkill objectSkill, EExtraFlag flag):
		TechniqueItem(objectWearPosition, objectType, objectSkill) {
			_flag = flag;
			_flagged = true;
		};
};

using TechniqueItemKitType = std::vector<TechniqueItem>;
using TechniqueItemKitsGroupType = std::vector<TechniqueItemKitType*>;


#endif // _FEATURES_ITEMSET_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
