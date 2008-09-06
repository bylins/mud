// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "char.hpp"
#include "char_player.hpp"
#include "comm.h"
#include "utils.h"

Player::Player()
	: pfilepos(-1)
{

}

// дефолтные поля плеера для всех мобов разом
Player PlayerProxy::mob;
char const *log_message = "SYSERR: Mob using 'player' at %s.";

int Player::get_pfilepos()
{
	return pfilepos;
}

int PlayerProxy::get_pfilepos()
{
	if (player)
		return player->get_pfilepos();
	else
	{
		log(log_message, __func__);
		return mob.get_pfilepos();
	}
}

void Player::set_pfilepos(int num)
{
	pfilepos = num;
}

void PlayerProxy::set_pfilepos(int num)
{
	if (player)
		player->set_pfilepos(num);
	else
	{
		log(log_message, __func__);
		mob.set_pfilepos(num);
	}
}
