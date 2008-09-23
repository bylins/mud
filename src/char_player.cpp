// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "char_player.hpp"
#include "utils.h"

Player::Player()
	:
	pfilepos_(-1),
	was_in_room_(NOWHERE)
{

}

int Player::get_pfilepos() const
{
	return pfilepos_;
}

void Player::set_pfilepos(int pfilepos)
{
	pfilepos_ = pfilepos;
}

room_rnum Player::get_was_in_room() const
{
	return was_in_room_;
}

void Player::set_was_in_room(room_rnum was_in_room)
{
	was_in_room_ = was_in_room;
}

std::string const & Player::get_passwd() const
{
	return passwd_;
}

void Player::set_passwd(std::string const & passwd)
{
	passwd_ = passwd;
}

void Player::remort()
{
	quested.clear();
}
