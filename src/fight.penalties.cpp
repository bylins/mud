#include "fight.penalties.hpp"

#include "char.hpp"

int GroupPenalty::get() const
{
	const bool leader_is_npc = IS_NPC(m_leader);
	const bool leader_is_in_room = AFF_FLAGGED(m_leader, EAffectFlag::AFF_GROUP)
		&& m_leader->in_room == IN_ROOM(m_killer);
	if (!leader_is_npc
		&& leader_is_in_room)
	{
		return get_penalty(m_leader);
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

		const int penalty = get_penalty(f->follower);
		if (0 < penalty)
		{
			return penalty;
		}
	}

	return 0;
}

int GroupPenalty::get_penalty(const CHAR_DATA* player) const
{
	const int player_remorts = static_cast<int>(GET_REMORT(player));
	const int player_class = static_cast<int>(GET_CLASS(player));
	const int player_level = GET_LEVEL(player);

	if (IS_NPC(player))
	{
		log("LOGIC ERROR: try to get penalty for NPC [%s], VNum: %d\n",
			player->get_name().c_str(),
			GET_MOB_VNUM(player));
		return DEFAULT_PENALTY;
	}

	if (player_class > NUM_PLAYER_CLASSES)
	{
		log("LOGIC ERROR: wrong player class: %d for player [%s]",
			player_class,
			player->get_name().c_str());
		return DEFAULT_PENALTY;
	}

	if (player_remorts > MAX_REMORT)
	{
		log("LOGIC ERROR: wrong number of remorts: %d for player [%s]",
			player_remorts,
			player->get_name().c_str());
		return DEFAULT_PENALTY;
	}

	int penalty = 0;
	if (m_max_level - player_level > grouping[player_class][player_remorts])
	{
		penalty = 50 + 2 * (m_max_level - player_level - grouping[player_class][player_remorts]);
	}

	return penalty;
}

