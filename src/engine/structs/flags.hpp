#pragma once

#include <iostream>
#include <limits>
#include <bitset>

template<typename EnumType, bool IsEnum = std::is_enum<EnumType>::value>
class BitFlag;

template<typename EnumType>
class BitFlag<EnumType, true>
{
public:
	constexpr const static int number_of_bits = std::numeric_limits<typename std::underlying_type<EnumType>::type>::digits;

	constexpr BitFlag() = default;
	constexpr BitFlag(EnumType value) : bits(std::size_t{1} << static_cast<std::size_t>(value)) {}
	constexpr BitFlag(const BitFlag& other) : bits(other.bits) {}

	constexpr BitFlag operator|(EnumType value) const { BitFlag result = *this; result.bits |= std::size_t{1} << static_cast<std::size_t>(value); return result; }
	constexpr BitFlag operator&(EnumType value) const { BitFlag result = *this; result.bits &= std::size_t{1} << static_cast<std::size_t>(value); return result; }
	constexpr BitFlag operator^(EnumType value) const { BitFlag result = *this; result.bits ^= std::size_t{1} << static_cast<std::size_t>(value); return result; }
	constexpr BitFlag operator~() const { BitFlag result = *this; result.bits.flip(); return result; }

	constexpr BitFlag operator|(BitFlag other) const { BitFlag result = *this; result.bits |= other.bits; return result; }
	constexpr BitFlag operator&(BitFlag other) const { BitFlag result = *this; result.bits &= other.bits; return result; }
	constexpr BitFlag operator^(BitFlag other) const { BitFlag result = *this; result.bits ^= other.bits; return result; }

	constexpr BitFlag& operator|=(EnumType value) { bits |= std::size_t{1} << static_cast<std::size_t>(value); return *this; }
	constexpr BitFlag& operator&=(EnumType value) { bits &= std::size_t{1} << static_cast<std::size_t>(value); return *this; }
	constexpr BitFlag& operator^=(EnumType value) { bits ^= std::size_t{1} << static_cast<std::size_t>(value); return *this; }

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

#define FLAGS_DECLARE_FROM_ENUM(FlagNewType, EnumType) using FlagNewType = BitFlag<EnumType>
#define FLAGS_DECLARE_OPERATORS(FlagType, EnumType) \
	inline FlagType operator|(const EnumType& a, const EnumType& b) { return FlagType(a) | b; } \
	inline FlagType operator&(const EnumType& a, const EnumType& b) { return FlagType(a) & b; } \
	inline FlagType operator^(const EnumType& a, const EnumType& b) { return FlagType(a) ^ b; }
