// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "char_player.hpp"
#include "utils.h"

Player::Player()
	:
	pfilepos_(-1),
	was_in_room_(NOWHERE),
	from_room_(0),
	answer_id_(NOBODY)
{
	for (int i = 0; i < START_STATS_TOTAL; ++i)
		start_stats_.at(i) = 0;
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
	quested_.clear();
	mobmax_.clear();
}

void Player::reset()
{
	remember_.reset();
	last_tell_ = "";
	answer_id_ = NOBODY;
}

room_rnum Player::get_from_room() const
{
	return from_room_;
}

void Player::set_from_room(room_rnum from_room)
{
	from_room_ = from_room;
}

int Player::get_start_stat(int stat_num)
{
	int stat = 0;
	try
	{
		stat = start_stats_.at(stat_num);
	}
	catch (...)
	{
		log("SYSERROR : bad start_stat %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
	return stat;
}

void Player::set_start_stat(int stat_num, int number)
{
	try
	{
		start_stats_.at(stat_num) = number;
	}
	catch (...)
	{
		log("SYSERROR : bad start_stat num %d (%s %s %d)", stat_num, __FILE__, __func__, __LINE__);
	}
}

void Player::set_answer_id(int id)
{
	answer_id_ = id;
}

int Player::get_answer_id() const
{
	return answer_id_;
}

void Player::remember_add(std::string text, int flag)
{
	remember_.add_str(text, flag);
}

std::string Player::remember_get(int flag) const
{
	return remember_.get_text(flag);
}

bool Player::remember_set_num(unsigned int num)
{
	return remember_.set_num_str(num);
}

unsigned int Player::remember_get_num() const
{
	return remember_.get_num_str();
}

void Player::set_last_tell(const char *text)
{
	if (text)
	{
		last_tell_ = text;
	}
}

std::string const & Player::get_last_tell()
{
	return last_tell_;
}

void Player::quested_add(CHAR_DATA *ch, int vnum, char *text)
{
	quested_.add(ch, vnum, text);
}

bool Player::quested_remove(int vnum)
{
	return quested_.remove(vnum);
}

bool Player::quested_get(int vnum) const
{
	return quested_.get(vnum);
}

std::string Player::quested_get_text(int vnum) const
{
	return quested_.get_text(vnum);
}

std::string Player::quested_print() const
{
	return quested_.print();
}

void Player::quested_save(FILE *saved) const
{
	quested_.save(saved);
}

int Player::mobmax_get(int vnum) const
{
	return mobmax_.get_kill_count(vnum);
}

void Player::mobmax_add(CHAR_DATA *ch, int vnum, int count, int level)
{
	mobmax_.add(ch, vnum, count, level);
}

void Player::mobmax_load(CHAR_DATA *ch, int vnum, int count, int level)
{
	mobmax_.load(ch, vnum, count, level);
}

void Player::mobmax_remove(int vnum)
{
	mobmax_.remove(vnum);
}

void Player::mobmax_save(FILE *saved) const
{
	mobmax_.save(saved);
}

void Player::dps_start_timer(int type, CHAR_DATA *ch)
{
	dps_.start_timer(type, ch);
}

void Player::dps_stop_timer(int type, CHAR_DATA *ch)
{
	dps_.stop_timer(type, ch);
}

void Player::dps_add_dmg(int type, int dmg, int over_dmg, CHAR_DATA *ch)
{
	dps_.add_dmg(type, ch, dmg, over_dmg);
}

void Player::dps_clear(int type)
{
	dps_.clear(type);
}

void Player::dps_print_stats()
{
	dps_.print_stats(this);
}

void Player::dps_print_group_stats(CHAR_DATA *ch)
{
	dps_.print_group_stats(ch);
}

/**
* Для dps_copy.
*/
void Player::dps_set(DpsSystem::Dps *dps)
{
	dps_ = *dps;
}

/**
* Нужно только для копирования всего этого дела при передаче лидера.
*/
void Player::dps_copy(CHAR_DATA *ch)
{
	ch->dps_set(&dps_);
}

void Player::dps_end_round(int type, CHAR_DATA *ch)
{
	dps_.end_round(type, ch);
}

void Player::dps_add_exp(int exp, bool battle)
{
	if (battle)
	{
		dps_.add_battle_exp(exp);
	}
	else
	{
		dps_.add_exp(exp);
	}
}
