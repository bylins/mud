#ifndef __SPELL_SLOTS_HPP__
#define __SPELL_SLOTS_HPP__

/*
	Константы, определяющие количество слотов под заклинания
	и прочий код для работы с ними.
*/

#include "entities/char_data.h"
#include "classes_constants.h"

namespace PlayerClass {

int CalcCircleSlotsAmount(CharData *ch, int slot_num);
void mspell_slot(char *name, ESpell spell_id, int kin, int chclass, int slot);

class MaxClassSlot {
 public:
	MaxClassSlot();

	void init(int chclass, int kin, int slot);
	int get(ECharClass chclass) const;
	int get(const CharData *ch) const;

 private:
	int _max_class_slot[kNumPlayerClasses][kNumKins];
};

extern MaxClassSlot max_slots;

}; //namespace ClassPlayer

#endif // __SPELL_SLOTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
