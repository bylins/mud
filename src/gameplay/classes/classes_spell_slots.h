#ifndef __SPELL_SLOTS_HPP__
#define __SPELL_SLOTS_HPP__

/*
	Константы, определяющие количество слотов под заклинания
	и прочий код для работы с ними.
*/

#include "engine/entities/char_data.h"
#include "classes_constants.h"

namespace classes {

int CalcCircleSlotsAmount(CharData *ch, int circle);

}; //namespace ClassPlayer

#endif // __SPELL_SLOTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
