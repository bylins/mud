
/*
	Реализация кулдаунов для умений.
*/

#include "char.cooldowns.hpp"
#include "skills.h"

class AbilityCooldownsType {
    short globalCoolDown;
    map<ESkill, short> abilityCooldowns;

public:
	void setGlobalCooldown(unsigned cooldown);
	void setAbilityCooldown(ESkill ability, short cooldown);
	void decreaseCooldowns(unsigned time);
	bool haveCooldown(ESkill ability);
	bool haveCooldown();
};

void AbilityCooldownsType::setGlobalCooldown(unsigned cooldown) {
	globalCoolDown = MAX(cooldown, globalCoolDown);
}

void AbilityCooldownsType::setAbilityCooldown(ESkill ability, short cooldown) {
};

void AbilityCooldownsType::decreaseCooldowns(unsigned time) {
};

bool AbilityCooldownsType::haveCooldown(ESkill ability) {
};

bool AbilityCooldownsType::haveCooldown() {
	return (globalCoolDown > 0);
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
