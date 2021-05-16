#ifndef TEMP_SPELLS_HPP_INCLUDED
#define TEMP_SPELLS_HPP_INCLUDED

#include "chars/character.h"

#include <vector>

namespace Temporary_Spells
{
	void add_spell(CHAR_DATA* ch, int spellnum, time_t set_time, time_t duration);
	void update_times();
	void update_char_times(CHAR_DATA* ch, time_t now);
	time_t spell_left_time(CHAR_DATA* ch, int spellnum);
}

#endif // TEMP_SPELLS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
