/**
\file animate_dead.cpp - a part of the Bylins engine.
\brief kAnimateDead mechanic implementation (issue.animate-dead / #3548).
*/

#include "animate_dead.h"

#include "engine/entities/char_data.h"
#include "gameplay/skills/skills.h"
#include "gameplay/abilities/abilities_constants.h"

#include <algorithm>
#include <cmath>

namespace animate_dead {

namespace {
// Locked tuning knobs (issue.animate-dead study, section 8; subject to a balance pass).
constexpr double kHpFactor = 0.08;      // HP        = proto_hp * (1 + kHpFactor * C)
constexpr double kDiceFactor = 1.2;     // damnodice += round(kDiceFactor * C)
constexpr int kMaxUndead = 4;           // control cap (matches the old MaxCharmices ceiling)
constexpr int kCountDivisor = 25;       // 1 + min(skill,75)/25 -> 1..4 over skill 0..75
constexpr int kMaxDamNoDice = 100;      // keep well inside the signed `byte` damnodice range
}  // namespace

int MaxUndead(CharData *ch) {
	const int skill = std::min(GetSkill(ch, ESkill::kDarkMagic), abilities::kNoviceSkillThreshold);
	return std::min(kMaxUndead, 1 + skill / kCountDivisor);
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
