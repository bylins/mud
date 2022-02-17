#ifndef _FEATURES_ITEMSET_HPP_INCLUDED_
#define _FEATURES_ITEMSET_HPP_INCLUDED_

/*
	Класс наборов экипировки, требующейся для выполнения приема.
*/

#include "entities/obj_data.h"
#include "skills.h"
#include "structs/structs.h"
#include "utils/utils.h"

struct TechniqueItem {
	short _wearPosition;
	ObjData::EObjectType _type;
	ESkill _skill;
	bool _flagged;
	EExtraFlag _flag;

	// TODO: Добавить учет типов ударов (уколол и проч).
	bool operator==(const ObjData *item) const {
		return (item
			&& (_type == GET_OBJ_TYPE(item))
			&& ((_skill == ESkill::kAny) || (_skill == static_cast<ESkill>(item->get_skill())))
			&& (_flagged && OBJ_FLAGGED(item, _flag)));
	};

	TechniqueItem() :
		_wearPosition{-1},
		_type{ObjData::ITEM_UNDEFINED},
		_skill{ESkill::kAny},
		_flagged{false},
		_flag{EExtraFlag::ITEM_GLOW} {};

	TechniqueItem(short objectWearPosition, ObjData::EObjectType objectType) :
		_wearPosition{objectWearPosition} {
		_type = objectType;
	};
	TechniqueItem(short objectWearPosition, ObjData::EObjectType objectType, ESkill objectSkill) :
		TechniqueItem(objectWearPosition, objectType) {
		_skill = objectSkill;
	};
	TechniqueItem(short objectWearPosition, ObjData::EObjectType objectType, ESkill objectSkill, EExtraFlag flag) :
		TechniqueItem(objectWearPosition, objectType, objectSkill) {
		_flag = flag;
		_flagged = true;
	};
};

using TechniqueItemKitType = std::vector<TechniqueItem>;
using TechniqueItemKitPtr = std::unique_ptr<TechniqueItemKitType>;
//using TechniqueItemKitsGroupType = std::vector<TechniqueItemKitType *>;
using TechniqueItemKitsGroupType = std::vector<TechniqueItemKitPtr>;

#endif // _FEATURES_ITEMSET_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
