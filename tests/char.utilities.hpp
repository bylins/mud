#ifndef __CHAR__UTILITIES_HPP__
#define __CHAR__UTILITIES_HPP__

// Backwards-compat shim: CharacterBuilder lives in src/engine/entities/ now,
// so it can be used by both tests and the headless balance simulator.
#include "engine/entities/character_builder.h"

namespace test_utils {
	using CharacterBuilder = entities::CharacterBuilder;
}

#endif // __CHAR__UTILITIES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
