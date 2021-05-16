#include "fight.penalties.hpp"

#include "logger.hpp"
#include "utils.h"
#include "chars/character.h"
#include "classes/constants.hpp"

int GroupPenaltyCalculator::get() const
{
	const bool leader_is_npc = IS_NPC(m_leader);
	const bool leader_in_group = AFF_FLAGGED(m_leader, EAffectFlag::AFF_GROUP);
	const bool leader_is_in_room = leader_in_group
		&& m_leader->in_room == IN_ROOM(m_killer);
	if (!leader_is_npc
		&& leader_is_in_room)
	{
		int penalty = 0;
		if (penalty_by_leader(m_leader, penalty))
		{
			return penalty;
		}
	}

	for (auto f = m_leader->followers; f; f = f->next)
	{
		const bool follower_is_npc = IS_NPC(f->follower);
		const bool follower_is_in_room = AFF_FLAGGED(f->follower, EAffectFlag::AFF_GROUP)
			&& f->follower->in_room == IN_ROOM(m_killer);

		if (follower_is_npc
			|| !follower_is_in_room)
		{
			continue;
		}

		int penalty = 0;
		if (penalty_by_leader(f->follower, penalty))
		{
			if (0 < penalty)
			{
				return penalty;
			}
		}
	}

	return 0;
}

bool GroupPenaltyCalculator::penalty_by_leader(const CHAR_DATA* player, int& penalty) const
{
	const int player_remorts = static_cast<int>(GET_REMORT(player));
	const int player_class = static_cast<int>(GET_CLASS(player));
	const int player_level = GET_LEVEL(player);

	if (IS_NPC(player))
	{
		log("LOGIC ERROR: try to get penalty for NPC [%s], VNum: %d\n",
			player->get_name().c_str(),
			GET_MOB_VNUM(player));
		penalty = DEFAULT_PENALTY;
		return true;
	}

	if (0 > player_class
		|| player_class > NUM_PLAYER_CLASSES)
	{
		log("LOGIC ERROR: wrong player class: %d for player [%s]",
			player_class,
			player->get_name().c_str());
		penalty = DEFAULT_PENALTY;
		return true;
	}

	if (0 > player_remorts
		|| player_remorts > MAX_REMORT)
	{
		log("LOGIC ERROR: wrong number of remorts: %d for player [%s]",
			player_remorts,
			player->get_name().c_str());
		penalty = DEFAULT_PENALTY;
		return true;
	}

	bool result = false;
	if (m_max_level - player_level > m_grouping[player_class][player_remorts])
	{
		penalty = 50 + 2 * (m_max_level - player_level - m_grouping[player_class][player_remorts]);
		result = true;
	}

	return result;
}

