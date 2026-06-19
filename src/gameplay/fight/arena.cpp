/**
\file arena.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Arena combat helpers.
*/

#include "arena.h"

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "utils/grammar/cases.h"
#include "gameplay/mechanics/sight.h"
#include "utils/utils.h"

namespace arena {

const char *VisibleName(const CharData *ch, const CharData *viewer, int pad, bool arena) {
	return arena ? GET_PAD(ch, pad) : sight::PersonName(ch, viewer, pad);
}

const char *VisibleObjShort(const ObjData *obj, const CharData *viewer, bool arena) {
	return (arena || sight::CanSeeObj(viewer, obj)) ? obj->get_short_description().c_str()
											  : grammar::SomethingInCase(grammar::ECase::kNom);
}

const char *VisibleObjName(const ObjData *obj, const CharData *viewer, grammar::ECase pad, bool arena) {
	if (arena || sight::CanSeeObj(viewer, obj)) {
		return !obj->get_PName(pad).empty() ? obj->get_PName(pad).c_str()
											: obj->get_short_description().c_str();
	}
	return grammar::SomethingInCase(pad);
}

}  // namespace arena

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
