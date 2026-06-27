/**
\file punishments.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ADMINISTRATION_PUNISHMENTS_H_
#define BYLINS_SRC_ADMINISTRATION_PUNISHMENTS_H_

#include <memory>

class CharData;
#include <string>

namespace punishments {

// Which kind of punishment. Used to index a character's records universally.
enum class EType { kMute, kDumb, kHell, kName, kFreeze, kGcurse, kUnreg, kCount };

struct Punish {
  long duration = 0;
  std::string reason;
  int level = 0;
  long godid = 0;
};

// A character's punishment records, one Punish per EType.
class CharPunishments {
 public:
  Punish &operator[](EType t) { return items_[static_cast<int>(t)]; }
  const Punish &operator[](EType t) const { return items_[static_cast<int>(t)]; }
 private:
  Punish items_[static_cast<int>(EType::kCount)];
};

// The punishment record of `type` for `ch` -- read or write through the returned reference
// (replaces the MUTE_REASON/GET_HELL_LEV/... macros that used to live in utils.h).
Punish &Get(CharData *ch, EType type);
const Punish &Get(const CharData *ch, EType type);
inline Punish &Get(const std::shared_ptr<CharData> &ch, EType type) { return Get(ch.get(), type); }

bool SetMute(CharData *ch, CharData *vict, char *reason, long times);
bool SetDumb(CharData *ch, CharData *vict, char *reason, long times);
bool SetHell(CharData *ch, CharData *vict, char *reason, long times);
bool SetFreeze(CharData *ch, CharData *vict, char *reason, long times);
bool SetNameRoom(CharData *ch, CharData *vict, char *reason, long times);
bool SetUnregister(CharData *ch, CharData *vict, char *reason, long times);
bool SetRegister(CharData *ch, CharData *vict, char *reason);

}

#endif //BYLINS_SRC_ADMINISTRATION_PUNISHMENTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
