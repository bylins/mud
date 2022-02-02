#include "common.h"

#include "skills/parry.h"
#include "handler.h"

int isHaveNoExtraAttack(CharacterData *ch) {
	std::string message = "";
	parry_override(ch);
	if (ch->get_extra_victim()) {
		switch (ch->get_extra_attack_mode()) {
			case kExtraAttackBash:message = "Невозможно. Вы пытаетесь сбить $N3.";
				break;
			case kExtraAttackKick:message = "Невозможно. Вы пытаетесь пнуть $N3.";
				break;
			case kExtraAttackUndercut:message = "Невозможно. Вы пытаетесь подсечь $N3.";
				break;
			case kExtraAttackDisarm:message = "Невозможно. Вы пытаетесь обезоружить $N3.";
				break;
			case kExtraAttackThrow:message = "Невозможно. Вы пытаетесь метнуть оружие в $N3.";
				break;
			case kExtraAttackPick:
			case kExtraAttackCutShorts:message = "Невозможно. Вы пытаетесь провести боевой прием против $N1.";
				break;
			default:return false;
		}
	} else {
		return true;
	};

	act(message.c_str(), false, ch, 0, ch->get_extra_victim(), TO_CHAR);
	return false;
}

void set_wait(CharacterData *ch, int waittime, int victim_in_room) {
	if (!WAITLESS(ch) && (!victim_in_room || (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())))) {
		WAIT_STATE(ch, waittime * kPulseViolence);
	}
}

void setSkillCooldown(CharacterData *ch, ESkill skill, int cooldownInPulses) {
	if (ch->getSkillCooldownInPulses(skill) < cooldownInPulses) {
		ch->setSkillCooldown(skill, cooldownInPulses * kPulseViolence);
	}
}

void setSkillCooldownInFight(CharacterData *ch, ESkill skill, int cooldownInPulses) {
	if (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
		setSkillCooldown(ch, skill, cooldownInPulses);
	} else {
		WAIT_STATE(ch, kPulseViolence / 6);
	}
}

CharacterData *findVictim(CharacterData *ch, char *argument) {
	one_argument(argument, arg);
	CharacterData *victim = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	if (!victim) {
		if (!*arg && ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
			victim = ch->get_fighting();
		}
	}
	return victim;
}
