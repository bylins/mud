#ifndef BYLINS_MOUNT_H
#define BYLINS_MOUNT_H

#include <memory>

class CharData;
enum class ESaving : int;

// issue.mount-mechanics: the mounting/horse mechanic. Per-character horse helpers (moved off
// CharData) live in the `mount` namespace; the do_horse* command handlers stay free functions
// (command-table convention). The "horse" special proc (buy/sell) is a separate mechanic that
// merely calls into here.
namespace mount {

// ch's horse: the NPC follower carrying the kHorse affect, or nullptr.
[[nodiscard]] CharData *GetHorse(CharData *ch);

// Does ch own a horse? With same_room, only counts a horse in ch's room.
[[nodiscard]] bool HasHorse(const CharData *ch, bool same_room);

// Is ch currently mounted (carries kHorse and the horse is in the room)?
[[nodiscard]] bool IsOnHorse(const CharData *ch);

// Is ch itself a mount (a charmed NPC follower carrying kHorse)? shared_ptr overload for convenience.
[[nodiscard]] bool IsHorse(const CharData *ch);
[[nodiscard]] bool IsHorse(const std::shared_ptr<CharData> &ch);

// Throw the rider off (call on rider or horse): unhorses the rider, lags & seats them. Returns
// false if ch is neither a rider nor a mount.
bool DropFromHorse(CharData *ch);

// Voluntarily get off the horse (clears kHorse, emits the messages). No-op if not mounted.
void Dismount(CharData *ch);

// True if ch is mounted and the horse is in the way of the attempted action (also tells ch so).
bool IsBlockedByHorse(CharData *ch);

// Turn the NPC `horse` into ch's mount (sets kHorse, makes it ch's follower, clears stray flags).
void MakeHorse(CharData *horse, CharData *ch);

// --- "What being mounted does" in combat (Riding-skill based). The fight_hit callers keep the
//     melee guard (non-NPC, not throw/backstab) and call these to apply the mount deltas. ---
// To-hit penalty for a mounted attacker (also trains Riding).
void ApplyRiderToHit(CharData *ch, CharData *victim, int &calc_thaco);
// Mounted attacker's damage bonus + to-hit adjustment vs the Riding difficulty.
void ApplyRiderHitAndDamage(CharData *ch, int &dam, int &calc_thaco);
// High-Riding (>100) mounted damage multiplier. Self-guarded (no-op if afoot or Riding<=100).
void ApplyRiderDamageMult(CharData *ch, int &dam);

// Mounted save modifier added to a basic saving throw: +20 Reflex (worse), -20-Riding/25 Stability
// (better). 0 when afoot or for other saves.
[[nodiscard]] int SavingModifier(const CharData *ch, ESaving saving);

}  // namespace mount

void do_horseon(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseoff(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_horseget(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseput(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horsetake(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_givehorse(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stophorse(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_MOUNT_H
