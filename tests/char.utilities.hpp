#ifndef __CHAR__UTILITIES_HPP__
#define __CHAR__UTILITIES_HPP__

// Backwards-compat shim: CharacterBuilder lives in src/simulator/ -- it is a
// simulator-side helper that tests have historically reused via this alias.
#include "simulator/character_builder.h"

namespace test_utils {
	using CharacterBuilder = simulator::CharacterBuilder;
}

#endif // __CHAR__UTILITIES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
