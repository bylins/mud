#ifndef SPELL_PARSER_HPP_
#define SPELL_PARSER_HPP_

#include <limits>
#include <string>

class CObjectPrototype;
#include <vector>

#include "gameplay/abilities/feats.h"
#include "engine/entities/entities_constants.h"
#include "spells.h"
#include "gameplay/skills/skills.h"
#include "gameplay/abilities/talents_actions.h"

#include "magic.h"
#include <cmath>
#include <algorithm>

class CharData;
struct RoomData;
class ObjData;

/**
 * Имена ищутся по сокращению: строка 'ог ша' находит 'огненный шар'.
 */
ESkill FindSkillId(const char *name);
ESkill FixNameAndFindSkillId(char *name);
ESkill FixNameFndFindSkillId(std::string &name);

ESpell FindSpellId(const std::string &name);
ESpell FindSpellId(const char *name);
ESpell FixNameAndFindSpellId(std::string &name);

EFeat FindFeatId(const char *name);
EFeat FixNameAndFindFeatId(const std::string &name);

abilities::EAbility FindAbilityId(const std::string &name);
abilities::EAbility FixNameAndFindAbilityId(const std::string &name);

ESpell FindSpellIdWithName(const std::string &name);

int FindCastTarget(ESpell spell_id, const char *t, CharData *ch, CharData **tch, ObjData **tobj, RoomData **troom,
		int *dir = nullptr);  // kTarDirection -> *dir = parsed direction
void SaySpell(CharData *ch, ESpell spell_id, CharData *tch, ObjData *tobj);

// True if `caster` bypasses the spell's room-level <blocking> (kNoMagic etc.).
// Greater gods always bypass; uncharmed NPCs (other than tutelars) bypass too -- they
// ignore the room block when ticking their own affects.
bool MayCastInForbiddenRoom(CharData *caster);

// True if `room` carries any of the room flags listed in the spell's
// <blocking><room_flags val="..."/></blocking>. Used in CallMagic and
// CallMagicToRoom to fizzle casts whose XML config forbids the room.
bool IsRoomBlocked(RoomData *room, const talents_actions::FlagCondition &cond);

// Cast-recorded potency: the scalar strength stored on every imposed affect
// (Affect::potency) for later dispel comparisons (DispelSucceeds). It is the
// deterministic competence skill_coeff + stat_coeff (no noise), captured at the
// moment of imposition, so every imposition path records the same value for the same cast.
float CalcCastPotency(const RollResult &potency);

// Apply-side modifier value used by every affect imposition path:
//     raw = min + ceil(beta·C·(1 + weight·d))   C = competence, d = potency.noise_dev
//     raw = min(raw, cap)        if cap > 0
//     return static_cast<int>(factor · raw)
// cap == 0 disables the clamp; factor is allowed to be negative (debuffs), with the clamp
// acting on the pre-factor magnitude. Templated so it serves a spell apply
// (TalentAffect::Apply) and an affect-owned apply (affects::AffectApply).
template<class ApplyT>
int ComputeApplyModifier(const ApplyT &apply, double competence, const RollResult &potency) {
	const double competencies = competence;
	double raw = apply.min + std::ceil(
			apply.beta * competencies * (1.0 + apply.weight * potency.noise_dev));
	if (apply.cap > 0) {
		raw = std::min(raw, static_cast<double>(apply.cap));
	}
	return static_cast<int>(apply.factor * raw);
}

// Multiplicative truncated-normal amount. mean = floor + scaled,
// std = sigma*scaled, so the coefficient of variation is ~sigma -- CONSTANT as `scaled` (= k*competence)
// grows, unlike additive dice noise (whose relative spread shrinks). Truncated to [floor, cap]; cap<=0 =
// no upper bound. Uses the seeded global RNG (GaussIntNumber), so simulator runs stay reproducible.
int CalcNoisyAmount(double floor_val, double scaled, double sigma, int cap,
		double fixed_z = std::numeric_limits<double>::quiet_NaN());

// A magic item's casting potency (competence C) -- SHARED by potions, scrolls, wands
// and staves. From the item's stored maker skill+stat (kMakerSkill/kMakerStat; absent -> the authored
// maker default), through the spell's potency_roll. A crafted potion may carry a brewed-in kPotionPotency
// (legacy saves), used when no maker skill is stored. Returns <=0 if the spell is undefined.
[[nodiscard]] float MagicItemPotency(const ObjData *item, ESpell spell_id);

// The maker's skill for a magic item's buff DURATION (brew skill for a crafted potion,
// else the authored maker). MagicItemStat is the paired key-stat. The user's own skill never matters --
// the item acts on its own.
[[nodiscard]] int MagicItemSkill(const ObjData *item);
[[nodiscard]] int MagicItemStat(const ObjData *item);

// Сила вещи, которую сделал игрок, посчитана по его умению (kMakerSkill/kMakerStat) и
// записана в самой вещи. У вещи из прототипа этих ключей нет, MagicItemPotency берет зашитые
// умолчания, и число выходит одинаковым у всех вещей с этим заклинанием -- свойством конкретной
// вещи оно не является. Такую силу помечаем в выводе, чтобы ее не принимали за настоящую.
[[nodiscard]] bool IsPotencyFromProto(const CObjectPrototype *item);

// Заклинания свитка/зелья/посоха с их силой, по одному на элемент,
// вида "исцеление (сила 42)" -- на выход в utils::OutWordsList. Пустой список, если
// заклинаний нет. Берутся из extra_values -- сырые val[] у этих типов нулевые.
[[nodiscard]] std::vector<std::string> SpellItemSpellsWithPotency(const ObjData *item);

int CalcCastSuccess(CharData *ch, CharData *victim, ESaving saving, ESpell spell_id);

void CastEquipmentAffect(CharData *ch, ESpell spell_id);

// The worn items (equipment slots) that carry any obj-affect
// suppression, in slot order. Used by the weave-restoration cast to choose a target item (it picks one
// at random). Never contains null; may be empty.
[[nodiscard]] std::vector<ObjData *> SuppressedWornItems(CharData *ch);

#endif // SPELL_PARSER_HPP_
