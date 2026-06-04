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

class CastContext;   // defined in magic.h (issue.spell-pipeline)
enum class EStageResult;  // defined in magic.h; manual handlers return it (issue.manual-cast)

#include <optional>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

#define NPC_CALCULATE 0xff << 16

// *** Extra attack bit flags //
constexpr Bitvector kEafParry = 1 << 0;
constexpr Bitvector kEafBlock = 1 << 1;
constexpr Bitvector kEafTouch = 1 << 2;
//constexpr Bitvector kEafProtect = 1 << 3;
constexpr Bitvector kEafDodge = 1 << 4;
constexpr Bitvector kEafHammer = 1 << 5;
constexpr Bitvector kEafOverwhelm = 1 << 6;
constexpr Bitvector kEafSlow = 1 << 7;
constexpr Bitvector kEafPunctual = 1 << 8;
constexpr Bitvector kEafAwake = 1 << 9;
constexpr Bitvector kEafFirst = 1 << 10;
constexpr Bitvector kEafSecond = 1 << 11;
constexpr Bitvector kEafStand = 1 << 13;
constexpr Bitvector kEafUsedright = 1 << 14;
constexpr Bitvector kEafUsedleft = 1 << 15;
constexpr Bitvector kEafMultyparry = 1 << 16;
constexpr Bitvector kEafSleep = 1 << 17;
constexpr Bitvector kEafIronWind = 1 << 18;
constexpr Bitvector kEafAutoblock = 1 << 19;	// автоматический блок щитом в осторожном стиле
constexpr Bitvector kEafPoisoned = 1 << 20;		// отравление с пушек раз в раунд
constexpr Bitvector kEafFirstPoison = 1 << 21;	// отравление цели первый раз за бой
constexpr Bitvector kEafInvisible = 1 << 22;	// одет автоинвиз

/// Flags for ingredient items (kMagicIngredient)
enum EIngredientFlag {
	kItemRunes = 1 << 0,
	kItemCheckUses = 1 << 1,
	kItemCheckLag = 1 << 2,
	kItemCheckLevel = 1 << 3,
	kItemDecayEmpty = 1 << 4
};

template<>
EIngredientFlag ITEM_BY_NAME<EIngredientFlag>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item);

constexpr Bitvector kMiLag1S = 1 << 0;
constexpr Bitvector kMiLag2S = 1 << 1;
constexpr Bitvector kMiLag4S = 1 << 2;
constexpr Bitvector kMiLag8S = 1 << 3;
constexpr Bitvector kMiLag16S = 1 << 4;
constexpr Bitvector kMiLag32S = 1 << 5;
constexpr Bitvector kMiLag64S = 1 << 6;
constexpr Bitvector kMiLag128S = 1 << 7;
constexpr Bitvector kMiLevel1 = 1 << 8;
constexpr Bitvector kMiLevel2 = 1 << 9;
constexpr Bitvector kMiLevel4 = 1 << 10;
constexpr Bitvector kMiLevel8 = 1 << 11;
constexpr Bitvector kMiLevel16 = 1 << 12;
constexpr Bitvector kMiLevel32 = 1 << 13;

// Бывшие kTypeHit/kTypeMagic/kType*-константы удалены (issue #3316): источник боевого
// урона теперь типизирован через fight::EDamageSource, выбор сообщения - по типу.

#define MANUAL_SPELL(spellname)    spellname(level, caster, cvict, ovict);

void SpellInformation(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellEnchantWeapon(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellForbidden(int level, CharData *ch, CharData *victim, ObjData *obj);
void SkillIdentify(int level, CharData *ch, CharData *victim, ObjData *obj);
void RemovePortalGate(RoomRnum rnum);
// issue.spellhandlers: helpers used by the extracted manual handlers (defs stay in spells.cpp).
int GetTeleportTargetRoom(CharData *ch, int rnum_start, int rnum_stop);
void CheckAutoNosummon(CharData *ch);
void AddPortalTimer(CharData *ch, RoomData *from_room, RoomRnum to_room, int time, long pk_unique = 0);
int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id);
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int CalcAntiSavings(CharData *ch);
int pk_action_type_summon(CharData *agressor, CharData *victim);
int pk_increment_revenge(CharData *agressor, CharData *victim);
void MortShowCharValues(CharData *victim, CharData *ch, int fullness);
extern int what_sky;
extern char cast_argument[];
// basic magic calling functions

ESpell FixNameAndFindSpellId(char *name);

bool CatchBloodyCorpse(ObjData *l);

// other prototypes //
bool CanGetSpell(CharData *ch, ESpell spell_id);
bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id);
int CalcMinRuneSpellLvl(const CharData *ch, ESpell spell_id);
ESkill GetMagicSkillId(ESpell spell_id);
int CheckRecipeValues(CharData *ch, ESpell spell_id, ESpellType spell_type, int showrecipe);
int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, CharData *tch = nullptr);
void MortShowObjValues(const ObjData *obj, CharData *ch, int fullness);

//#define CALC_SUCCESS(modi, perc)         ((modi)-100+(perc))

const int kHoursPerWarcry = 4;
const int kHoursPerTurnUndead = 8;

#endif // SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
