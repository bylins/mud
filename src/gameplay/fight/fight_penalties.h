#ifndef __FIGHT_PENALTIES_HPP__
#define __FIGHT_PENALTIES_HPP__

#include "gameplay/classes/pc_classes.h"

class CharData;    // to avoid inclusion of "char.hpp"

class GroupPenaltyCalculator {
 public:
	constexpr static int DEFAULT_PENALTY = 100;

	GroupPenaltyCalculator(const CharData *killer,
						   const CharData *leader,
						   const int max_level,
						   const GroupPenalties &grouping) :
		m_killer(killer),
		m_leader(leader),
		m_max_level(max_level),
		m_grouping(grouping) {
	}

	int get() const;

 private:
	const CharData *m_killer;
	const CharData *m_leader;
	const int m_max_level;
	const GroupPenalties &m_grouping;

	bool penalty_by_leader(const CharData *player, int &penalty) const;
};

#endif // __FIGHT_PENALTIES_HPP__
