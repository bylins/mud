#include "common.h"

#include "skills/parry.h"
#include "handler.h"

int isHaveNoExtraAttack(CHAR_DATA * ch) {
    std::string message = "";
    parry_override(ch);
    if (ch->get_extra_victim()) {
        switch (ch->get_extra_attack_mode()) {
            case EXTRA_ATTACK_BASH:
                message = "Невозможно. Вы пытаетесь сбить $N3.";
                break;
            case EXTRA_ATTACK_KICK:
                message = "Невозможно. Вы пытаетесь пнуть $N3.";
                break;
            case EXTRA_ATTACK_CHOPOFF:
                message = "Невозможно. Вы пытаетесь подсечь $N3.";
                break;
            case EXTRA_ATTACK_DISARM:
                message = "Невозможно. Вы пытаетесь обезоружить $N3.";
                break;
            case EXTRA_ATTACK_THROW:
                message = "Невозможно. Вы пытаетесь метнуть оружие в $N3.";
                break;
            case EXTRA_ATTACK_CUT_PICK:
            case EXTRA_ATTACK_CUT_SHORTS:
                message = "Невозможно. Вы пытаетесь провести боевой прием против $N1.";
                break;
            default:
                return false;
        }
    } else {
        return true;
    };

    act(message.c_str(), FALSE, ch, 0, ch->get_extra_victim(), TO_CHAR);
    return false;
}

void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room) {
    if (!WAITLESS(ch) && (!victim_in_room || (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())))) {
        WAIT_STATE(ch, waittime * PULSE_VIOLENCE);
    }
}

void setSkillCooldown(CHAR_DATA* ch, ESkill skill, int cooldownInPulses) {
    if (ch->getSkillCooldownInPulses(skill) < cooldownInPulses) {
        ch->setSkillCooldown(skill, cooldownInPulses*PULSE_VIOLENCE);
    }
}

void setSkillCooldownInFight(CHAR_DATA* ch, ESkill skill, int cooldownInPulses) {
    if (ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
        setSkillCooldown(ch, skill, cooldownInPulses);
    } else {
        WAIT_STATE(ch, PULSE_VIOLENCE/6);
    }
}

CHAR_DATA* findVictim(CHAR_DATA* ch, char* argument) {
    one_argument(argument, arg);
    CHAR_DATA* victim = get_char_vis(ch, arg, FIND_CHAR_ROOM);
    if (!victim) {
        if (!*arg && ch->get_fighting() && ch->isInSameRoom(ch->get_fighting())) {
            victim = ch->get_fighting();
        }
    }
    return victim;
}
