// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "char_player.hpp"
#include "utils.h"

boost::shared_ptr<Player> Player::shared_mob(new Player);

Player::Player()
	:
	pfilepos_(-1),
	was_in_room_(NOWHERE),
	from_room_(0)
{

}

bool check_mob_guard(Player const *plr, char const *fnc)
{
	// да, это можно сунуть без обертки в шаред-птр
	// и здесь будет красивее, но так удобнее присваивать мобу
	if (plr == &(*(Player::shared_mob)))
	{
		log("SYSERR: Mob using '%s'.", fnc);
		return false;
	}
	return true;
}

int Player::get_pfilepos() const
{
	check_mob_guard(this, __func__);
	return pfilepos_;
}

void Player::set_pfilepos(int pfilepos)
{
	check_mob_guard(this, __func__);
	pfilepos_ = pfilepos;
}

room_rnum Player::get_was_in_room() const
{
	check_mob_guard(this, __func__);
	return was_in_room_;
}

void Player::set_was_in_room(room_rnum was_in_room)
{
	check_mob_guard(this, __func__);
	was_in_room_ = was_in_room;
}

std::string const & Player::get_passwd() const
{
	check_mob_guard(this, __func__);
	return passwd_;
}

void Player::set_passwd(std::string const & passwd)
{
	check_mob_guard(this, __func__);
	passwd_ = passwd;
}

void Player::remort()
{
	check_mob_guard(this, __func__);
	quested.clear();
	mobmax.clear();
}

room_rnum Player::get_from_room() const
{
	check_mob_guard(this, __func__);
	return from_room_;
}

void Player::set_from_room(room_rnum from_room)
{
	check_mob_guard(this, __func__);
	from_room_ = from_room;
}
