#ifndef __FIGHT_EXTRA_ATTACK_HPP__
#define __FIGHT_EXTRA_ATTACK_HPP__

#include "utils/utils.h"
#include "game_magic/spells.h"

class CharacterData;    // to avoid inclusion of "char.hpp"

class WeaponMagicalAttack {
 public:
	int get_count() const { return count_attack; }
	void set_count(const int _) { count_attack = _; }
	void start_count() { count_attack = 0; }
	void set_attack(CharacterData *ch, CharacterData *victim);
	bool set_count_attack(CharacterData *ch);
	WeaponMagicalAttack(CharacterData *ch);
 private:
	int count_attack = 0;
	CharacterData *ch_;
};

#endif // __FIGHT_EXTRA_ATTACK_HPP__
