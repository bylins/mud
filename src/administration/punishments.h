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

int set_punish(CharData *ch, CharData *vict, int punish, char *reason, long times);

}

#endif //BYLINS_SRC_ADMINISTRATION_PUNISHMENTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
