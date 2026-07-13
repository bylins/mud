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
#include "engine/structs/structs.h"    // there was defined type "byte" if it had been missing

class ActionContext;   // defined in magic.h (issue.spell-pipeline)
enum class EStageResult;  // defined in magic.h; manual handlers return it (issue.manual-cast)

#include <optional>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

// Бывшие kTypeHit/kTypeMagic/kType*-константы удалены (issue #3316): источник боевого
// урона теперь типизирован через fight::EDamageSource, выбор сообщения - по типу.

void SpellInformation(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellEnchantWeapon(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellForbidden(int level, CharData *ch, CharData *victim, ObjData *obj);
// issue.spellhandlers: helpers used by the extracted manual handlers (defs stay in spells.cpp).
void CheckAutoNosummon(CharData *ch);
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int CalcAntiSavings(CharData *ch);
int pk_action_type_summon(CharData *agressor, CharData *victim);
int pk_increment_revenge(CharData *agressor, CharData *victim);
extern int what_sky;
extern char cast_argument[];
// basic magic calling functions

ESpell FixNameAndFindSpellId(char *name);

// other prototypes //
bool CanGetSpell(CharData *ch, ESpell spell_id);
bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id);
int CalcMinRuneSpellLvl(const CharData *ch, ESpell spell_id);

//#define CALC_SUCCESS(modi, perc)         ((modi)-100+(perc))

const int kHoursPerWarcry = 4;
const int kHoursPerTurnUndead = 8;

#endif // SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
