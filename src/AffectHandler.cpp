#include "AffectHandler.hpp"
#include <iostream>
#include "utils.h"
#include "handler.h"
#include "spells.h"

//нужный Handler вызывается в зависимости от типа передаваемых параметров
void LackyAffectHandler::Handle( DamageActorParameters& params )
{
	if (params.damage>0) damFromMe_ = true;
	params.damage += params.damage*(round_*4)/100;
}

void LackyAffectHandler::Handle( DamageVictimParameters& params )
{
	if (params.damage>0) damToMe_ = true;
}

AFFECT_DATA*  find_affect(CHAR_DATA* ch, int afftype)
{
	AFFECT_DATA* aff;
	for(aff=ch->affected;aff;aff=aff->next)
	{
		if (aff->type == afftype)
			return aff;
	}
	return 0;
}

void LackyAffectHandler::Handle( BattleRoundParameters& params )
{
	AFFECT_DATA* af = find_affect(params.ch, SPELL_LACKY);
	if (damFromMe_&&!damToMe_){
		if (round_<5) {
			++round_;
		}
	}
	else {
		round_= 0;
	}
	if (af) af->modifier=round_*2;
	damToMe_=false;
	damFromMe_=false;
}

void LackyAffectHandler::Handle( StopFightParameters& params )
{
	AFFECT_DATA* af = find_affect(params.ch, SPELL_LACKY);
	if (af) af->modifier = 0;
	round_=0;
	damFromMe_=damToMe_=false;
}
