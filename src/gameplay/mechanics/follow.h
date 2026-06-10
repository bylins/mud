#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_FOLLOW_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_FOLLOW_H_

#include <iosfwd>

class CharData;

// issue.chardata-cleaning: the character-following mechanic. The follow LOGIC/operations live here
// as free functions; CharData keeps only the raw state + trivial accessors (m_master / followers,
// get_master / has_master / set_master / get_followers_list). The do_follow command
// (engine/ui/cmd/do_follow.cpp) drives these.
namespace follow {

// Would making `master` the leader of `ch` create a cycle in the follower chain?
[[nodiscard]] bool MakesLoop(CharData *ch, CharData *master);

// Log a detailed follower-loop error (the leader chain + struct addresses).
void ReportLoopError(CharData *ch, CharData *master);

// Print ch's chain of leaders into ss (diagnostics).
void PrintLeadersChain(const CharData *ch, std::ostream &ss);

// Is ch a player who leads at least one PC follower?
[[nodiscard]] bool IsLeader(CharData *ch);

// Make ch follow leader (same-room check + the "you start following" messages).
void AddFollower(CharData *leader, CharData *ch);

// Low-level: attach ch to leader's follower list without checks/messages.
void AddFollowerSilently(CharData *leader, CharData *ch);


// Stop ch following its leader (mode = bitset of kSf* flags). Returns true if ch was extracted.
bool StopFollower(CharData *ch, int mode);
// A following/followed char died: detach from master and dismiss all followers.
void DieFollower(CharData *ch);
// Would making ch follow victim form an illegal follow loop?
[[nodiscard]] bool CircleFollow(CharData *ch, CharData *victim);

}  // namespace follow

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_FOLLOW_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
