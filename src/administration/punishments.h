/**
\file punishments.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ADMINISTRATION_PUNISHMENTS_H_
#define BYLINS_SRC_ADMINISTRATION_PUNISHMENTS_H_

class CharData;
namespace punishments {

struct Punish {
  long duration = 0;
  char *reason = nullptr;
  int level = 0;
  long godid = 0;
};

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
