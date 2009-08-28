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
	for (int i = 0; i < START_STATS_TOTAL; ++i)
		start_stats_.at(i) = 0;
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

int Player::get_start_stat(int stat_num)
{
	check_mob_guard(this, __func__);
	int stat = 0;
	try
	{
		stat = start_stats_.at(stat_num);
	}
	catch (...)
	{
		log("SYSERROR : ban start_stat %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
	return stat;
}

void Player::set_start_stat(int stat_num, int number)
{
	check_mob_guard(this, __func__);
	try
	{
		start_stats_.at(stat_num) = number;
	}
	catch (...)
	{
		log("SYSERROR : ban start_stat num %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
}

void Player::set_answer_id(int id)
{
	check_mob_guard(this, __func__);
	remember_.set_answer_id(id);
}

int Player::get_answer_id() const
{
	check_mob_guard(this, __func__);
	return remember_.get_answer_id();
}

void Player::add_remember(std::string text, int flag)
{
	check_mob_guard(this, __func__);
	remember_.add_str(text, flag);
}

std::string Player::get_remember(int flag) const
{
	check_mob_guard(this, __func__);
	return remember_.get_text(flag);
}

bool Player::set_remember_num(unsigned int num)
{
	check_mob_guard(this, __func__);
	return remember_.set_num_str(num);
}

unsigned int Player::get_remember_num() const
{
	check_mob_guard(this, __func__);
	return remember_.get_num_str();
}

void Player::reset()
{
	check_mob_guard(this, __func__);
	remember_.reset();
	last_tell_ = "";
}

void Player::set_last_tell(const char *text)
{
	check_mob_guard(this, __func__);
	if (text)
	{
		last_tell_ = text;
	}
}

std::string & Player::get_last_tell()
{
	check_mob_guard(this, __func__);
	return last_tell_;
}
