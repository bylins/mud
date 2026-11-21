/**
 \file antisaving.cpp - a part of the Bylins engine.
 \brief Anti-saving ("колдовство"): the caster's side of a saving throw.
*/

#include "gameplay/mechanics/antisaving.h"

#include "administration/privilege.h"
#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/mechanics/condition.h"
#include "gameplay/skills/skills.h"
#include "utils/utils.h"

#include <algorithm>
#include <array>

namespace antisaving {

namespace {

// Anti-saving bonus by real wisdom 1..kMobBaseStatCap (index = wisdom - 1).
constexpr std::array<int, kMobBaseStatCap> kWisdomBonus{
	-9, -8, -7, -6, -5, -4, -3, -2, -1, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 21, 22, 22, 23, 23, 24, 24, 25, 25,
	26, 26, 27, 27, 28, 28, 29, 29, 30, 30,
	31, 31, 32, 32, 33, 33, 34, 34, 35, 35,
	36, 36, 37, 37, 38, 38, 39, 39, 40, 40,
	41, 41, 41, 42, 42, 42, 43, 43, 43, 44,
	44, 44, 45, 45, 45, 46, 46, 46, 47, 47,
	47, 48, 48, 48, 49, 49, 49, 50, 50, 50,
};

int WisdomBonus(int wisdom) {
	const int idx = std::clamp(wisdom, kLeastBaseStat, kMobBaseStatCap) - 1;
	return kWisdomBonus[idx];
}

}  // namespace

int CalcAntiSavings(CharData *ch) {
	int modi = 0;
	if (privilege::IsImmortal(ch)) {
		modi = 1000;
	} else if (GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		modi = 250;
	} else if (GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		modi = -250;
	} else {
		modi = ch->add_abils.cast_success;
	}
	modi += WisdomBonus(GetRealWis(ch));
	if (!ch->IsNpc()) {
		modi *= condition::GetCondPenalty(ch, condition::kCast);
	}
	return modi;
}

int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id) {
	auto mod = MUD::Class(ch->GetClass()).spells[spell_id].GetCastMod();
	auto skill = GetSkill(ch, MUD::Spell(spell_id).GetSuccessRoll().GetBaseSkill());
	return static_cast<int>(mod * skill);
}

}  // namespace antisaving

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
