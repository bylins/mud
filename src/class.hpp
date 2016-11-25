#ifndef __CLASS_HPP__
#define __CLASS_HPP__

#include "structs.h"

#include <array>

class GroupPenalties
{
public:
	using class_penalties_t = std::array<int, MAX_REMORT + 1>;
	using penalties_t = std::array<class_penalties_t, NUM_PLAYER_CLASSES>;

	int init();
	const auto& operator[](const size_t character_class) const { return m_grouping[character_class]; }

private:
	penalties_t m_grouping;
};

extern GroupPenalties grouping;

#endif // __CLASS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
