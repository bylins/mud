#ifndef ACT_MOVEMENT_HPP_
#define ACT_MOVEMENT_HPP_

#include "engine/structs/structs.h"
#include "engine/entities/entities_constants.h"

enum EMoveType : int {
  kDefault,
  kFlee,
  kThrowOut
};

class CharData;
class ObjData;

int PerformMove(CharData *ch, int dir, int need_specials_check, int checkmob, CharData *master);
int PerformSimpleMove(CharData *ch, int dir, int following, CharData *leader, EMoveType move_type);
bool HasBoat(CharData *ch);
bool IsCorrectDirection(CharData *ch, int dir, bool check_specials, bool show_msg);

int SkipHiding(CharData *ch, CharData *vict);
int SkipSneaking(CharData *ch, CharData *vict);
int SkipCamouflage(CharData *ch, CharData *vict);
EDirection SelectRndDirection(CharData *ch, int fail_chance);

#endif // ACT_MOVEMENT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
