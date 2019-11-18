
/*
	Реализация кулдаунов для умений.
*/

#include "char.cooldowns.hpp"
#include "skills.h"

class AbilityCooldownType {
    short globalCoolDown;
    map<ESkill, short> abilityCooldowns;

public:
	void setGlobalCooldown(short cooldown);
	void setAbilityCooldown(ESkill ability, short cooldown);
	short getAbilityCooldown(ESkill ability);
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
