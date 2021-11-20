#pragma once

#include <iostream>
#include <limits>
#include <bitset>

template<typename EnumType, bool IsEnum = std::is_enum<EnumType>::value>
class bitflag;

template<typename EnumType>
class bitflag<EnumType, true>
{
public:
	constexpr const static int number_of_bits = std::numeric_limits<typename std::underlying_type<EnumType>::type>::digits;

	constexpr bitflag() = default;
	constexpr bitflag(EnumType value) : bits(1 << static_cast<std::size_t>(value)) {}
	constexpr bitflag(const bitflag& other) : bits(other.bits) {}

	constexpr bitflag operator|(EnumType value) const { bitflag result = *this; result.bits |= 1 << static_cast<std::size_t>(value); return result; }
	constexpr bitflag operator&(EnumType value) const { bitflag result = *this; result.bits &= 1 << static_cast<std::size_t>(value); return result; }
	constexpr bitflag operator^(EnumType value) const { bitflag result = *this; result.bits ^= 1 << static_cast<std::size_t>(value); return result; }
	constexpr bitflag operator~() const { bitflag result = *this; result.bits.flip(); return result; }

	constexpr bitflag operator|(bitflag other) const { bitflag result = *this; result.bits |= other.bits; return result; }
	constexpr bitflag operator&(bitflag other) const { bitflag result = *this; result.bits &= other.bits; return result; }
	constexpr bitflag operator^(bitflag other) const { bitflag result = *this; result.bits ^= other.bits; return result; }

	constexpr bitflag& operator|=(EnumType value) { bits |= 1 << static_cast<std::size_t>(value); return *this; }
	constexpr bitflag& operator&=(EnumType value) { bits &= 1 << static_cast<std::size_t>(value); return *this; }
	constexpr bitflag& operator^=(EnumType value) { bits ^= 1 << static_cast<std::size_t>(value); return *this; }

	constexpr bool any() const { return bits.any(); }
	constexpr bool all() const { return bits.all(); }
	constexpr bool none() const { return bits.none(); }
	constexpr operator bool() const { return any(); }

	constexpr bool test(EnumType value) const { return bits.test(static_cast<std::size_t>(value)); }
	constexpr void set(EnumType value) { bits.set(static_cast<std::size_t>(value)); }
	constexpr void unset(EnumType value) { bits.reset(static_cast<std::size_t>(value)); }

private:
	std::bitset<number_of_bits> bits;
};

#define FLAGS_DECLARE_FROM_ENUM(FlagNewType, EnumType) using FlagNewType = bitflag<EnumType>
#define FLAGS_DECLARE_OPERATORS(FlagType, EnumType) \
	inline FlagType operator|(const EnumType& a, const EnumType& b) { return FlagType(a) | b; } \
	inline FlagType operator&(const EnumType& a, const EnumType& b) { return FlagType(a) & b; } \
	inline FlagType operator^(const EnumType& a, const EnumType& b) { return FlagType(a) ^ b; }
