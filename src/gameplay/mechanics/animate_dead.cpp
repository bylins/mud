/**
\file animate_dead.cpp - a part of the Bylins engine.
\brief kAnimateDead mechanic implementation (issue.animate-dead / #3548).
*/

#include "animate_dead.h"

#include "engine/entities/char_data.h"
#include "gameplay/skills/skills.h"
#include "gameplay/abilities/abilities_constants.h"
#include "gameplay/magic/magic.h"   // kMobSkeleton .. kMobNecrocaster necro-mob vnums

#include <algorithm>
#include <cmath>
#include <iterator>

namespace animate_dead {

namespace {
// Potency scaling knobs (issue.animate-dead study, section 8; subject to a balance pass).
constexpr double kHpFactor = 0.08;      // HP        = proto_hp * (1 + kHpFactor * C)
constexpr double kDiceFactor = 1.2;     // damnodice += round(kDiceFactor * C)
constexpr int kMaxDamNoDice = 100;      // keep well inside the signed `byte` damnodice range

// Undead tier ladder, weakest -> strongest, with each type's skill-weight (cost). The weight is
// the Dark-Magic skill points one undead of that type consumes; floor(75/weight) is its count at
// the novice threshold (skeleton 6, zombie 5, bonedog 4, bonedragon 3, bonespirit/necrodamager 2,
// necrotank/breather/caster 1). Demotion walks DOWN this list from the desired tier.
struct UndeadTier { int vnum; int weight; };
constexpr UndeadTier kLadder[] = {
	{kMobSkeleton,      12},
	{kMobZombie,        15},
	{kMobBonedog,       16},
	{kMobBonedragon,    25},
	{kMobBonespirit,    37},
	{kMobNecrodamager,  37},
	{kMobNecrotank,     75},
	{kMobNecrobreather, 75},
	{kMobNecrocaster,   75},
};

int WeightOf(int vnum) {
	for (const auto &t : kLadder) {
		if (t.vnum == vnum) {
			return t.weight;
		}
	}
	return 0;   // non-necro (e.g. resurrection) followers do not consume the animate-dead budget
}

int LadderIndex(int vnum) {
	for (int i = 0; i < static_cast<int>(std::size(kLadder)); ++i) {
		if (kLadder[i].vnum == vnum) {
			return i;
		}
	}
	return static_cast<int>(std::size(kLadder)) - 1;   // unknown -> treat as the top tier
}

// Skill-weight already spent on the caster's currently-held charmed undead.
int UsedBudget(CharData *ch) {
	int used = 0;
	for (auto *k : ch->followers) {
		if (k && AFF_FLAGGED(k, EAffect::kCharmed) && k->get_master() == ch) {
			used += WeightOf(GET_MOB_VNUM(k));
		}
	}
	return used;
}
}  // namespace

int FitUndeadTier(CharData *ch, int desired) {
	// Budget = Dark-Magic skill up to the novice threshold (skill beyond it feeds potency, not
	// headcount), minus the weight of the undead already under control.
	const int budget = std::min(GetSkill(ch, ESkill::kDarkMagic), abilities::kNoviceSkillThreshold);
	const int remaining = budget - UsedBudget(ch);
	// Demote: from the corpse's natural tier, walk down to the strongest tier that still fits.
	for (int i = LadderIndex(desired); i >= 0; --i) {
		if (kLadder[i].weight <= remaining) {
			return kLadder[i].vnum;
		}
	}
	return 0;   // even a skeleton does not fit -> caller reports "too many minions"
}

void SetupUndeadStats(CharData * /*ch*/, CharData *mob, double competence) {
	const double c = std::max(0.0, competence);

	// HP: multiplicative on the tier prototype (keeps skeleton < ... < necrotank proportions,
	// mirrors the retired x(1+remort/10) boost). Never below the prototype value.
	const int hp = static_cast<int>(mob->get_max_hit() * (1.0 + kHpFactor * c));
	mob->set_max_hit(hp);
	mob->set_hit(hp);

	// Damage: additive on the dice COUNT (damsizedice kept from the prototype). Necromancer
	// undead are damage-first, so the damage weight (kDiceFactor) is heavier than the HP one.
	const int bonus = static_cast<int>(std::lround(kDiceFactor * c));
	const int dice = std::clamp(mob->mob_specials.damnodice + bonus, 1, kMaxDamNoDice);
	mob->mob_specials.damnodice = static_cast<byte>(dice);
}

}  // namespace animate_dead

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
