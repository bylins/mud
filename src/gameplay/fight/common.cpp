#include "common.h"

#include "gameplay/skills/parry.h"
#include "engine/core/handler.h"
#include "engine/core/target_resolver.h"

int IsHaveNoExtraAttack(CharData *ch) {
	std::string message = "";
	CheckParryOverride(ch);
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
	if (!ch->IsImmortal() && (!wait_if_fight || (ch->GetEnemy() && ch->isInSameRoom(ch->GetEnemy())))) {
		SetBattleLag(ch, waittime);
	}
}

void SetSkillCooldown(CharData *ch, ESkill skill, int pulses) {
	if (ch->Skills().GetCooldownInPulses(skill) < pulses) {
		ch->setSkillCooldown(skill, pulses * kBattleRound + 1);
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
	CharData * victim = nullptr;
	victim = target_resolver::FindCharInRoom(ch, arg);
	if (!victim) {
		if (!*arg && ch->GetEnemy() && ch->isInSameRoom(ch->GetEnemy())) {
			victim = ch->GetEnemy();
		}
	}
	return victim;
}
