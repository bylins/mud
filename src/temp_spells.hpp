#ifndef TEMP_SPELLS_HPP_INCLUDED
#define TEMP_SPELLS_HPP_INCLUDED

#include "char.hpp"

#include <vector>

namespace Temporary_Spells
{
	void add_spell(CHAR_DATA* ch, int spell, time_t set_time, time_t duration);
	void update_times();
	void update_char_times(CHAR_DATA* ch, time_t now);
	time_t spell_left_time(CHAR_DATA* ch, int spell);
}

#endif // TEMP_SPELLS_HPP_INCLUDED