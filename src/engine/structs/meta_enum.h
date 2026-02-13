/**
\file meta_enum.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Здесь нужно будет разместить реализацию итерируемых и конвертируемых в строки enum, а пока так.
*/

#ifndef BYLINS_SRC_ENGINE_STRUCTS_META_ENUM_H_
#define BYLINS_SRC_ENGINE_STRUCTS_META_ENUM_H_

#include <string>

template<typename T>
struct Unimplemented {};

template<typename E>
const std::string &NAME_BY_ITEM(const E) {
	return Unimplemented<E>::FAIL;
}

template<typename E>
E ITEM_BY_NAME(const std::string &) {
	return Unimplemented<E>::FAIL;
}

template<typename E>
inline E ITEM_BY_NAME(const char *name) { return ITEM_BY_NAME<E>(std::string(name)); }

#endif //BYLINS_SRC_ENGINE_STRUCTS_META_ENUM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
