#include "abilities_items_set.h"

#include "entities/obj_data.h"
#include "utils/utils.h"

// TODO: Добавить учет типов ударов (уколол и проч).
bool TechniqueItem::operator==(const ObjData *item) const {
	return (item
		&& (type == GET_OBJ_TYPE(item))
		&& ((skill == ESkill::kAny) || (skill == static_cast<ESkill>(item->get_skill())))
		&& (flagged ? item->has_flag(flag) : true));
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :