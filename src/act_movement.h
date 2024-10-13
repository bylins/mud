#ifndef ACT_MOVEMENT_HPP_
#define ACT_MOVEMENT_HPP_

#include "engine/structs/structs.h"

class CharData;
class ObjData;

bool HasBoat(CharData *ch);
bool IsCorrectDirection(CharData *ch, int dir, bool check_specials, bool show_msg);

int skip_hiding(CharData *ch, CharData *vict);
int skip_sneaking(CharData *ch, CharData *vict);
int skip_camouflage(CharData *ch, CharData *vict);

#endif // ACT_MOVEMENT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
