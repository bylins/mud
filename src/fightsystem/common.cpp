#include "common.h"

#include "skills/parry.h"
#include "handler.h"

int IsHaveNoExtraAttack(CharData *ch) {
	std::string message = "";
	parry_override(ch);
	if (ch->get_extra_victim()) {
		switch (ch->get_extra_attack_mode()) {
			case kExtraAttackBash: message = "Невозможно. Вы пытаетесь сбить $N3.";
				break;
			case kExtraAttackKick: message = "Невозможно. Вы пытаетесь пнуть $N3.";
				break;
			case kExtraAttackUndercut: message = "Невозможно. Вы пытаетесь подсечь $N3.";
				break;
			case kExtraAttackDisarm: message = "Невозможно. Вы пытаетесь обезоружить $N3.";
				break;
			case kExtraAttackThrow: message = "Невозможно. Вы пытаетесь метнуть оружие в $N3.";
				break;
			case kExtraAttackCut: message = "Невозможно. Вы пытаетесь провести боевой прием против $N1.";
				break;
			default: return false;
		}
	} else {
		return true;
	};

	act(message.c_str(), false, ch, nullptr, ch->get_extra_victim(), kToChar);
	return false;
}

void SetWait(CharData *ch, int waittime, int victim_in_room) {
	if (!WAITLESS(ch) && (!victim_in_room || (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())))) {
		WAIT_STATE(ch, waittime * kPulseViolence);
	}
}

void SetSkillCooldown(CharData *ch, ESkill skill, int cooldownInPulses) {
	if (ch->getSkillCooldownInPulses(skill) < cooldownInPulses) {
		ch->setSkillCooldown(skill, cooldownInPulses * kPulseViolence);
	}
}

void SetSkillCooldownInFight(CharData *ch, ESkill skill, int cooldownInPulses) {
	if (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
		SetSkillCooldown(ch, skill, cooldownInPulses);
	} else {
		WAIT_STATE(ch, kPulseViolence / 6);
	}
}

CharData *FindVictim(CharData *ch, char *argument) {
	one_argument(argument, arg);
	CharData *victim = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	if (!victim) {
		if (!*arg && ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
			victim = ch->get_fighting();
		}
	}
	return victim;
}
