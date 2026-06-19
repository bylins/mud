#ifndef TEMP_SPELLS_HPP_INCLUDED
#define TEMP_SPELLS_HPP_INCLUDED

#include "gameplay/magic/spells_constants.h"

#include <ctime>

#include <vector>

class CharData;

// issue.chardata-cleaning: a temporary spell record (was TemporarySpell in char_data.h).
struct TemporarySpell {
	ESpell spell{ESpell::kUndefined};
	time_t set_time{0};
	time_t duration{0};
};

namespace temporary_spells {
void AddSpell(CharData *ch, ESpell spell_id, time_t set_time, time_t duration);
void update_times();
void update_char_times(CharData *ch, time_t now);
time_t GetSpellLeftTime(CharData *ch, ESpell spell_id);
}

#endif // TEMP_SPELLS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
