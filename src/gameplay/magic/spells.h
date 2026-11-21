/* ************************************************************************
*   File: spells.h                                      Part of Bylins    *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef SPELLS_H_
#define SPELLS_H_

#include "engine/entities/entities_constants.h"
#include "gameplay/skills/skills.h"
#include "gameplay/classes/classes_constants.h"
#include "spells_constants.h"
#include "engine/structs/structs.h"

class ActionContext;   // defined in magic.h
enum class EStageResult;  // defined in magic.h; manual handlers return it

#include <optional>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

void SpellInformation(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellEnchantWeapon(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellForbidden(int level, CharData *ch, CharData *victim, ObjData *obj);
// Helpers used by the manual spell handlers.
void CheckAutoNosummon(CharData *ch);
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int pk_action_type_summon(CharData *agressor, CharData *victim);
int pk_increment_revenge(CharData *agressor, CharData *victim);
extern int what_sky;
extern char cast_argument[];

ESpell FixNameAndFindSpellId(char *name);

bool CanGetSpell(CharData *ch, ESpell spell_id);
bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id);
int CalcMinRuneSpellLvl(const CharData *ch, ESpell spell_id);

const int kHoursPerWarcry = 4;
const int kHoursPerTurnUndead = 8;

#endif // SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
