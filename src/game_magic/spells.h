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

#include "entities/entities_constants.h"
#include "game_skills/skills.h"
#include "game_classes/classes_constants.h"
#include "spells_constants.h"
#include "structs/structs.h"    // there was defined type "byte" if it had been missing

#include <optional>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

#define NPC_CALCULATE 0xff << 16

// *** Extra attack bit flags //
constexpr Bitvector kEafParry = 1 << 0;
constexpr Bitvector kEafBlock = 1 << 1;
constexpr Bitvector kEafTouch = 1 << 2;
constexpr Bitvector kEafProtect = 1 << 3;
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

// PLAYER SPELLS TYPES //
enum ESpellType {
	kUnknowm = 0,
	kKnow = 1 << 0,
	kTemp = 1 << 1,
	kPotionCast = 1 << 2,
	kWandCast = 1 << 3,
	kScrollCast = 1 << 4,
	kItemCast = 1 << 5,
	kRunes = 1 << 6
};

/// Flags for ingredient items (kIngredient)
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

const int kTypeHit = 400;
const int kTypeMagic = 420;
// new attack types can be added here - up to TYPE_SUFFERING
const int kTypeTriggerdeath = 495;
const int kTypeTunnerldeath = 496;
const int kTypeWaterdeath = 497;
const int kTypeRoomdeath = 498;
const int kTypeSuffering = 499;

struct AttackHitType {
	const char *singular;
	const char *plural;
};

#define MANUAL_SPELL(spellname)    spellname(level, caster, cvict, ovict);

void SpellCreateWater(int/* level*/, CharData *ch, CharData *victim, ObjData *obj);
void SpellRecall(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/);
void SpellTeleport(int /* level */, CharData *ch, CharData */*victim*/, ObjData */*obj*/);
void SpellSummon(int /*level*/, CharData *ch, CharData *victim, ObjData */*obj*/);
void SpellRelocate(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/);
void SpellPortal(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/);
void SpellLocateObject(int level, CharData *ch, CharData* /*victim*/, ObjData *obj);
void SpellCharm(int/* level*/, CharData *ch, CharData *victim, ObjData* /* obj*/);
void SpellInformation(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellIdentify(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellFullIdentify(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellEnchantWeapon(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellControlWeather(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellCreateWeapon(int/* level*/, CharData* /*ch*/, CharData* /*victim*/, ObjData* /* obj*/);
void SpellEnergydrain(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/);
void SpellFear(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/);
void SpellSacrifice(int/* level*/, CharData *ch, CharData *victim, ObjData* /*obj*/);
void SpellForbidden(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellHolystrike(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/);
void SkillIdentify(int level, CharData *ch, CharData *victim, ObjData *obj);
void SpellSummonAngel(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/);
void SpellVampirism(int/* level*/, CharData* /*ch*/, CharData* /*victim*/, ObjData* /*obj*/);
void SpellMentalShadow(int/* level*/, CharData *ch, CharData* /*victim*/, ObjData* /*obj*/);

// basic magic calling functions

ESpell FixNameAndFindSpellId(char *name);

bool CatchBloodyCorpse(ObjData *l);

// other prototypes //
void InitSpellLevels();
const char *GetSpellName(ESpell spell_id);
int CalcSaving(CharData *killer, CharData *victim, ESaving saving, int ext_apply);
int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply);
bool CanGetSpell(CharData *ch, ESpell spell_id);
bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl);
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id);
ESkill GetMagicSkillId(ESpell spell_id);
int CheckRecipeValues(CharData *ch, ESpell spell_id, ESpellType spell_type, int showrecipe);
int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, const CharData *targ = nullptr);

//Polud статистика использования заклинаний
typedef std::map<ESpell, int> SpellCountType;

namespace SpellUsage {
	extern bool is_active;
	extern time_t start;
	void AddSpellStat(ECharClass char_class, ESpell spell_id);
	void save();
	void clear();
};
//-Polud

#define CALC_SUCCESS(modi, perc)         ((modi)-100+(perc))

const int kHoursPerWarcry = 4;
const int kHoursPerTurnUndead = 8;

#endif // SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
