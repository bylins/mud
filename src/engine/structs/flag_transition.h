// issue.flags-migration: TEMPORARY bridge between the legacy FlagData and the typed BitsetFlags,
// for subsystem boundaries that still speak FlagData (Lua bindings, crafting flag-combining, some
// admin-API paths) while the entity STORAGE has already moved to BitsetFlags. Both forms share the
// same 30-bit-plane numbering, so a plane-by-plane copy is byte-lossless. DELETE this header once
// FlagData is gone (issue.flags-migration P2).
#ifndef BYLINS_SRC_ENGINE_STRUCTS_FLAG_TRANSITION_H_
#define BYLINS_SRC_ENGINE_STRUCTS_FLAG_TRANSITION_H_

#include "engine/structs/flag_data.h"
#include "engine/structs/bitset_flags.h"

// FlagData -> BitsetFlags<E>. E must be supplied explicitly (the source is untyped).
template<class E>
BitsetFlags<E> ToBitset(const FlagData &f) {
	BitsetFlags<E> b;
	for (std::size_t i = 0; i < FlagData::kPlanesNumber; ++i) {
		b.set_plane(i, f.get_plane(i));
	}
	return b;
}

// BitsetFlags<E> -> FlagData (E inferred from the argument).
template<class B>
FlagData ToFlagData(const B &b) {
	FlagData f;
	for (std::size_t i = 0; i < FlagData::kPlanesNumber; ++i) {
		f.set_plane(i, b.get_plane(i));
	}
	return f;
}

#endif  // BYLINS_SRC_ENGINE_STRUCTS_FLAG_TRANSITION_H_
