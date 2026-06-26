#include "affect_handler.h"

#include "gameplay/mechanics/condition.h"   // condition::kDrunk, kDrunked
#include "utils/utils.h"                     // GET_NAME, GET_COND, GET_DRUNK_STATE

#include <algorithm>

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

// issue.affect-migration: find by affect identity (EAffect), not the casting spell.
Affect<EApply>::shared_ptr find_affect(CharData *ch, EAffect affect_type) {
	for (const auto &aff : ch->affected) {
		if (aff->affect_type == affect_type) {
			return aff;
		}
	}

	return Affect<EApply>::shared_ptr();
}

void CombatLuckAffectHandler::Handle(BattleRoundParameters &params) {
	auto af = find_affect(params.ch, EAffect::kCombatLuck);
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
	auto af = find_affect(params.ch, EAffect::kCombatLuck);
	if (af) {
		af->modifier = 0;
	}
	round_ = 0;
	damFromMe_ = damToMe_ = false;
}


CharData::char_affects_list_t::iterator RemoveAffect(CharData *ch,
		const CharData::char_affects_list_t::iterator &affect_i) {
	if (ch->affected.empty()) {
		log("SYSERR: affect_remove(%s) when no affects...", GET_NAME(ch));
		return ch->affected.end();
	}
	const auto af = *affect_i;
	if (af->affect_type == EAffect::kAbstinent) {
		if (ch->player_specials) {
			GET_DRUNK_STATE(ch) = GET_COND(ch, condition::kDrunk) = std::min(GET_COND(ch, condition::kDrunk), kDrunked - 1);
		} else {
			log("SYSERR: player_specials is not set.");
		}
	}
	return ch->affected.erase(affect_i);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
