#include "affects_handler.h"
#include "handler.h"
#include "spells.h"
#include "utils.h"

#include <iostream>

//нужный Handler вызывается в зависимости от типа передаваемых параметров
void LackyAffectHandler::Handle( DamageActorParameters& params )
{
	if (params.damage>0) damFromMe_ = true;
	params.damage += params.damage*(round_*4)/100;
}

void LackyAffectHandler::Handle( DamageVictimParameters& params )
{
	if (params.damage>0)
	{
		damToMe_ = true;
	}
}

AFFECT_DATA<EApplyLocation>::shared_ptr find_affect(CHAR_DATA* ch, int afftype)
{
	for (const auto& aff : ch->affected)
	{
		if (aff->type == afftype)
		{
			return aff;
		}
	}

	return AFFECT_DATA<EApplyLocation>::shared_ptr();
}

void LackyAffectHandler::Handle(BattleRoundParameters& params)
{
	auto af = find_affect(params.ch, SPELL_LACKY);
	if (damFromMe_&&!damToMe_)
	{
		if (round_<5)
		{
			++round_;
		}
	}
	else
	{
		round_= 0;
	}
	if (af)
	{
		af->modifier = round_ * 2;
	}
	damToMe_=false;
	damFromMe_=false;
}
// тест
void LackyAffectHandler::Handle(StopFightParameters& params)
{
	auto af = find_affect(params.ch, SPELL_LACKY);
	if (af)
	{
		af->modifier = 0;
	}
	round_ = 0;
	damFromMe_ = damToMe_ = false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
