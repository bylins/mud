#ifndef _FEATURES_ITEMSET_HPP_INCLUDED_
#define _FEATURES_ITEMSET_HPP_INCLUDED_

/*
	Класс наборов экипировки, требующейся для выполнения приема.
*/

#include "obj.hpp"
#include "skills.h"
#include "structs.h"
#include "utils.h"

struct ExpedientItem {
	short _wearPosition;
	OBJ_DATA::EObjectType _type;
	ESkill _skill;
	bool _flagged;
	EExtraFlag _flag;

	// TODO: Добавить учет типов ударов (уколол и проч).
	bool operator== (const OBJ_DATA *item) const {
		return (item
				&& (_type == GET_OBJ_TYPE(item))
				&& ((_skill == SKILL_INVALID) || (_skill == GET_OBJ_SKILL(item)))
				&& (_flagged && OBJ_FLAGGED(item, _flag)));
	};

	ExpedientItem():
		_wearPosition{-1},
		_type{OBJ_DATA::ITEM_UNDEFINED},
		_skill{SKILL_INVALID},
		_flagged{false},
		_flag{0}
		{};

	ExpedientItem(short objectWearPosition, OBJ_DATA::EObjectType objectType):
		_wearPosition{objectWearPosition} {
			_type = objectType;
		};
	ExpedientItem(short objectWearPosition, OBJ_DATA::EObjectType objectType, ESkill objectSkill):
		ExpedientItem(objectWearPosition, objectType) {
			_skill = objectSkill;
		};
	ExpedientItem(short objectWearPosition, OBJ_DATA::EObjectType objectType, ESkill objectSkill, EExtraFlag flag):
		ExpedientItem(objectWearPosition, objectType, objectSkill) {
			_flag = flag;
			_flagged = true;
		};
};

using ExpedientItemKitType = std::vector<ExpedientItem>;
using ExpedientItemKitsGroupType = std::vector<ExpedientItemKitType*>;


#endif // _FEATURES_ITEMSET_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
