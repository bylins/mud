/* ************************************************************************
*   File: magic.h                                     Part of Bylins      *
*  Usage: header: low-level functions for magic; spell template code      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#ifndef MAGIC_H_
#define MAGIC_H_

#include "spells_info.h"

#include <cstdlib>

class CharData;
class ObjData;
struct RoomData;

// The "first-aid removable" spell list (moved here from char_data.h, issue.affect-dispell-flags).
// The order doubles as the cure-difficulty index used by do_first_aid (prob/10 > index). This list
// is slated to be replaced by the kAfCurable flag + a potency-based cure mechanic.
enum ERemovableSpell {
	kRemAbstinent = 0,
	kRemPoison,
	kRemMadness,
	kRemWeakness,
	kRemSlowdown,
	kRemMindless,
	kRemColdWind,
	kRemFever,
	kRemCurse,
	kRemDeafness,
	kRemSilence,
	kRemBlindness,
	kRemSleep,
	kRemHold,
	kRemVacuum,
	kRemHaemorrhage,
	kRemBattle,
	kMaxFirstaidRemove
};

inline ESpell GetRemovableSpellId(int num) {
	static const ESpell spell[kMaxFirstaidRemove] = {
		ESpell::kAbstinent, ESpell::kPoison, ESpell::kMadness,
		ESpell::kWeaknes, ESpell::kSlowdown, ESpell::kMindless, ESpell::kColdWind,
		ESpell::kFever, ESpell::kCurse, ESpell::kDeafness, ESpell::kSilence,
		ESpell::kBlindness, ESpell::kSleep, ESpell::kHold, ESpell::kVacuum,
		ESpell::kHaemorrhage, ESpell::kBattle
	};
	if (num < kMaxFirstaidRemove) {
		return spell[num];
	}
	return ESpell::kUndefined;
}

// One evaluation of a talents_actions::Roll for a specific caster (issue #3333).
struct RollResult {
	int dices{0};              // talents_actions::Roll::RollSkillDices()
	double skill_coeff{0.0};   // talents_actions::Roll::CalcSkillCoeff(caster)
	double stat_coeff{0.0};    // talents_actions::Roll::CalcBaseStatCoeff(caster)
	double low_skill_coeff{0.0};  // talents_actions::Roll::CalcLowSkillCoeff(caster): the
	                              // low-skill bonus only, fed to SetBattleLag as skill_bonus.
};

// The spell's success and potency rolls, computed once per cast in CallMagic and
// threaded to the cast-dispatch functions in place of (level, spell_id). The roll
// values do not depend on level; level is carried only to replace that parameter.
struct CastRollResult {
	ESpell spell_id{ESpell::kUndefined};
	int level{0};
	RollResult success;        // from SpellInfo::GetSuccessRoll()
	RollResult potency;        // from SpellInfo::GetPotencyRoll()
};

// Evaluates both rolls of spell_id against caster. Defined in magic_utils.cpp.
CastRollResult ComputeCastRoll(CharData *caster, ESpell spell_id, int level);

// VNUM'ы мобов для заклинаний, создающих мобов
const int kMobDouble = 3000; //внум прототипа для клона
const int kMobSkeleton = 3001;
const int kMobZombie = 3002;
const int kMobBonedog = 3003;
const int kMobBonedragon = 3004;
const int kMobBonespirit = 3005;
const int kMobNecrodamager = 3007;
const int kMobNecrotank = 3008;
const int kMobNecrobreather = 3009;
const int kMobNecrocaster = 3010;
const int kLastNecroMob = 3010;
// резерв для некротических забав
const int kMobMentalShadow = 3020;
const int kMobKeeper = 3021;
const int kMobFirekeeper = 3022;

const int kMaxSpellAffects = 16; // change it if you need more

bool is_room_forbidden(RoomData *room);
void mobile_affect_update();
void player_affect_update();
void print_rune_log();
void ShowAffExpiredMsg(ESpell aff_type, CharData *ch);

int CallMagicToGroup(CharData *ch, CastRollResult roll);
int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, CastRollResult roll);

int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

// Result of one cast stage (CastAffect/CastUnaffects/...). CastToSingleTarget runs the
// stages in order and stops the chain on kBreak. Today every stage returns kSuccess
// (behaviour unchanged); per-spell kBreak conditions are to be driven from spells.xml later.
// The list may grow.
enum class EStageResult {
	kBreak,		// stop the cast: skip the remaining stages.
	kSuccess	// stage handled; continue to the next stage.
};

int CastDamage(int level, CharData *ch, CharData *victim, ESpell spell_id);
// NB (issue.cast-affect): the <blocking>/<required> target gates now live in CastToSingleTarget,
// so prefer it over calling CastAffect directly. CastAffect stays declared here because
// SpellHolystrike (kHolystrike's manual room-dispatcher) must call it per target and cannot go
// through CastToSingleTarget (that would re-enter the manual stage -> infinite recursion).
EStageResult CastAffect(int level, CharData *ch, CharData *victim, ESpell spell_id, const RollResult &potency = {});
EStageResult CastToPoints(int level, CharData *ch, CharData *victim, ESpell spell_id);
EStageResult CastUnaffects(int, CharData *ch, CharData *victim, ESpell spell_id);
EStageResult CastToAlterObjs(int, CharData *ch, ObjData *obj, ESpell spell_id);
EStageResult CastCreation(int, CharData *ch, ESpell spell_id);
EStageResult CastCharRelocate(CharData *caster, CharData *cvict, ESpell spell_id);
int CastToSingleTarget(CharData *caster, CharData *cvict, ObjData *ovict, CastRollResult roll);
int CalcSaving(CharData *killer, CharData *victim, ESaving saving, bool need_log = false);
int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply);
int GetBasicSave(CharData *ch, ESaving saving, bool log = false);
EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id);
int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id);

#endif // MAGIC_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
