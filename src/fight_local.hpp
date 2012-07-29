// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_LOCAL_HPP_
#define _FIGHT_LOCAL_HPP_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "AffectHandler.hpp"

#define RIGHT_WEAPON 1
#define LEFT_WEAPON  2

//Polud функция, вызывающая обработчики аффектов, если они есть
template <class S> void handle_affects( S& params ) //тип params определяется при вызове функции
{
	AFFECT_DATA* aff;
	for (aff=params.ch->affected; aff; aff = aff->next)
	{
		if (aff->handler)
			aff->handler->Handle(params); //в зависимости от типа params вызовется нужный Handler
	}
}

/** fight.cpp */

int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
void set_battle_pos(CHAR_DATA * ch);

/** fight_hit.cpp */

int calc_leadership(CHAR_DATA * ch);
void exthit(CHAR_DATA * ch, int type, int weapon);

/** fight_stuff.cpp */

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam);
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);

#endif // _FIGHT_LOCAL_HPP_
