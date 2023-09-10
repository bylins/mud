#include "common.h"

#include "game_skills/parry.h"
#include "handler.h"

int IsHaveNoExtraAttack(CharData *ch) {
	std::string message = "";
	parry_override(ch);
	if (ch->GetExtraVictim()) {
		switch (ch->get_extra_attack_mode()) {
			case kExtraAttackBash: message = "Невозможно. Вы пытаетесь сбить $N3.";
				break;
			case kExtraAttackKick: message = "Невозможно. Вы пытаетесь пнуть $N3.";
				break;
			case kExtraAttackChopoff: message = "Невозможно. Вы пытаетесь подсечь $N3.";
				break;
			case kExtraAttackDisarm: message = "Невозможно. Вы пытаетесь обезоружить $N3.";
				break;
			case kExtraAttackThrow: message = "Невозможно. Вы пытаетесь метнуть оружие в $N3.";
				break;
			case kExtraAttackCut: message = "Невозможно. Вы пытаетесь провести боевой прием против $N1.";
				break;
			case kExtraAttackSlay: message = "Невозможно. Вы пытаетесь сразить $N3.";
				break;
			default: return false;
		}
	} else {
		return true;
	};

	act(message.c_str(), false, ch, nullptr, ch->GetExtraVictim(), kToChar);
	return false;
}

void SetWait(CharData *ch, int waittime, int wait_if_fight) {
	if (!IS_IMMORTAL(ch) && (!wait_if_fight || (ch->GetEnemy() && ch->isInSameRoom(ch->GetEnemy())))) {
		SetWaitState(ch, waittime * kBattleRound);
	}
}

void SetSkillCooldown(CharData *ch, ESkill skill, int pulses) {
	if (ch->getSkillCooldownInPulses(skill) < pulses) {
		ch->setSkillCooldown(skill, pulses * kBattleRound);
	}
}

void SetSkillCooldownInFight(CharData *ch, ESkill skill, int pulses) {
	if (ch->GetEnemy() && ch->isInSameRoom(ch->GetEnemy())) {
		SetSkillCooldown(ch, skill, pulses);
	} else {
		SetWaitState(ch, kBattleRound / 6);
	}
}

CharData *FindVictim(CharData *ch, char *argument) {
	one_argument(argument, arg);
	CharData *victim = get_char_vis(ch, arg, EFind::kCharInRoom);
	if (!victim) {
		if (!*arg && ch->GetEnemy() && ch->isInSameRoom(ch->GetEnemy())) {
			victim = ch->GetEnemy();
		}
	}
	return victim;
}
