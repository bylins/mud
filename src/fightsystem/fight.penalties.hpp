#ifndef __FIGHT_PENALTIES_HPP__
#define __FIGHT_PENALTIES_HPP__

#include "classes/class.hpp"

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

class GroupPenaltyCalculator
{
public:
	constexpr static int DEFAULT_PENALTY = 100;

	GroupPenaltyCalculator(const CHAR_DATA* killer, const CHAR_DATA* leader, const int max_level, const GroupPenalties& grouping):
		m_killer(killer),
		m_leader(leader),
		m_max_level(max_level),
		m_grouping(grouping)
	{
	}

	int get() const;

private:
	const CHAR_DATA* m_killer;
	const CHAR_DATA* m_leader;
	const int m_max_level;
	const GroupPenalties& m_grouping;

	bool penalty_by_leader(const CHAR_DATA* player, int& penalty) const;
};

#endif // __FIGHT_PENALTIES_HPP__
