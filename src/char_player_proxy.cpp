// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <string>
#include "char_player_proxy.hpp"
#include "comm.h"
#include "utils.h"

// дефолтные поля плеера для всех мобов разом
// TODO: на счет сетить или нет мобу в случае, если функция не должна ничего возвращать пока не уверен что лучше
Player PlayerProxy::mob;
char const *log_message = "SYSERR: Mob using 'player' at %s.";

int PlayerProxy::get_pfilepos() const
{
	if (player)
		return player->get_pfilepos();
	else
	{
		log(log_message, __func__);
		return mob.get_pfilepos();
	}
}

void PlayerProxy::set_pfilepos(int pfilepos)
{
	if (player)
		player->set_pfilepos(pfilepos);
	else
	{
		log(log_message, __func__);
		mob.set_pfilepos(pfilepos);
	}
}

room_rnum PlayerProxy::get_was_in_room() const
{
	if (player)
		return player->get_was_in_room();
	else
	{
		log(log_message, __func__);
		return mob.get_was_in_room();
	}
}

void PlayerProxy::set_was_in_room(room_rnum was_in_room)
{
	if (player)
		player->set_was_in_room(was_in_room);
	else
	{
		log(log_message, __func__);
		mob.set_was_in_room(was_in_room);
	}
}

std::string const & PlayerProxy::get_passwd() const
{
	if (player)
		return player->get_passwd();
	else
	{
		log(log_message, __func__);
		return mob.get_passwd();
	}
}

void PlayerProxy::set_passwd(std::string const & passwd)
{
	if (player)
		player->set_passwd(passwd);
	else
	{
		log(log_message, __func__);
		mob.set_passwd(passwd);
	}
}
