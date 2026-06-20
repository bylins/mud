/**
\file char_equip_flags.h - a part of the Bylins engine.
\brief Flags controlling equip/unequip behaviour (CharEquipFlag / CharEquipFlags).
\details Extracted from handler.h (issue.handler-cleaning) into a tiny, dependency-light
 header so that modules implementing the equip/unequip mechanics (equipment, obj_sets)
 can take these flags by reference in their public API without pulling the heavy
 handler.h (whose inline helpers require a complete CharData and would otherwise create
 include cycles through obj_sets.h).
*/

#ifndef BYLINS_SRC_ENGINE_CORE_CHAR_EQUIP_FLAGS_H_
#define BYLINS_SRC_ENGINE_CORE_CHAR_EQUIP_FLAGS_H_

#include <cstdint>

#include "engine/structs/flags.hpp"

enum class CharEquipFlag : uint8_t {
	no_cast,	// no spell casting
	skip_total,	// no total affect update
	show_msg	// show wear and activation messages
};

FLAGS_DECLARE_FROM_ENUM(CharEquipFlags, CharEquipFlag);
FLAGS_DECLARE_OPERATORS(CharEquipFlags, CharEquipFlag);

#endif // BYLINS_SRC_ENGINE_CORE_CHAR_EQUIP_FLAGS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
