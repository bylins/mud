#include "affect_handler.h"

//нужный Handler вызывается в зависимости от типа передаваемых параметров
void CombatLuckAffectHandler::Handle(DamageActorParameters &params) {
	if (params.damage > 0) damFromMe_ = true;
	params.damage += params.damage * (round_ * 4) / 100;
}

void CombatLuckAffectHandler::Handle(DamageVictimParameters &params) {
	if (params.damage > 0) {
		damToMe_ = true;
	}
}

Affect<EApply>::shared_ptr find_affect(CharData *ch, ESpell aff_type) {
	for (const auto &aff : ch->affected) {
		if (aff->type == aff_type) {
			return aff;
		}
	}

	return Affect<EApply>::shared_ptr();
}

void CombatLuckAffectHandler::Handle(BattleRoundParameters &params) {
	auto af = find_affect(params.ch, ESpell::kCombatLuck);
	if (damFromMe_ && !damToMe_) {
		if (round_ < 5) {
			++round_;
		}
	} else {
		round_ = 0;
	}
	if (af) {
		af->modifier = round_ * 2;
	}
	damToMe_ = false;
	damFromMe_ = false;
}
// тест
void CombatLuckAffectHandler::Handle(StopFightParameters &params) {
	auto af = find_affect(params.ch, ESpell::kCombatLuck);
	if (af) {
		af->modifier = 0;
	}
	round_ = 0;
	damFromMe_ = damToMe_ = false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
