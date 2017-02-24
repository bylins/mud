#ifndef TEMP_SPELLS_HPP_INCLUDED
#define TEMP_SPELLS_HPP_INCLUDED

#include "char.hpp"

#include <vector>

#define TEMP_SPELLS_REFRESH_PERIOD 1

namespace Temporary_Spells
{
	void add_spell(CHAR_DATA* ch, int spell, time_t set_time, time_t duration);
	void update_times();
}

#endif // TEMP_SPELLS_HPP_INCLUDED