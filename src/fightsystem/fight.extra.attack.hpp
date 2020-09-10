#ifndef __FIGHT_EXTRA_ATTACK_HPP__
#define __FIGHT_EXTRA_ATTACK_HPP__

#include "utils.h"
#include "spells.h"

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

class WeaponMagicalAttack
{
public:
	int get_count() const { return count_attack; }
	void set_count(const int _) { count_attack = _; }
	void start_count() { count_attack = 0; }
	void set_attack(CHAR_DATA *ch, CHAR_DATA *victim);
	bool set_count_attack(CHAR_DATA *ch);
	WeaponMagicalAttack(CHAR_DATA * ch); 
private:
	int count_attack = 0;
	CHAR_DATA *ch_;
};

#endif // __FIGHT_EXTRA_ATTACK_HPP__
