
/*
	Реализация кулдаунов для умений/способностей/спеллов/чего-еще-угодно.
*/

#include "char.cooldowns.hpp"
//#include "skills.h"
#include "logger.hpp"

template <typename IDType, IDType globalCooldownID>
class CooldownsType {
    map<IDType, short> cooldowns;

public:
	void setGlobalCooldown(unsigned cooldown);
	void setAbilityCooldown(IDType ability, short cooldown);
	void decreaseCooldowns(unsigned time);
	void resetCooldowns();
	bool haveCooldown(IDType ability);
	bool haveCooldown();
};

void CooldownsType::decreaseCooldowns(unsigned time) {
	for (auto& ability : cooldowns) {
		ability->second -= time;
		if (ability->second <= 0) {
			cooldowns.erase(ability);
		}
	}
};

void CooldownsType::resetCooldowns() {
	cooldowns.clear();
};

void CooldownsType::setGlobalCooldown(unsigned cooldown) {
	setAbilityCooldown(globalCooldownID, cooldown);
}

void CooldownsType::setAbilityCooldown(IDType ability, short cooldown) {

};

bool CooldownsType::haveCooldown() {
	return haveCooldown(globalCooldownID);
};

bool CooldownsType::haveCooldown(IDType ability) {
	auto abilityCooldown = cooldowns.find(ability);
	return (abilityCooldown != cooldowns.end());
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
