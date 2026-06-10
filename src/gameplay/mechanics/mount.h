#ifndef BYLINS_MOUNT_H
#define BYLINS_MOUNT_H

#include <memory>

class CharData;

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

}  // namespace mount

void make_horse(CharData *horse, CharData *ch);

void do_horseon(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseoff(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_horseget(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseput(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horsetake(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_givehorse(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stophorse(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_MOUNT_H
