// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <algorithm>
#include "remember.hpp"
#include "utils.h"
#include "comm.h"
#include "db.h"

namespace Remember
{

std::string time_format()
{
	char time_buf[9];
	time_t tmp_time = time(0);
	strftime(time_buf, sizeof(time_buf), "[%H:%M] ", localtime(&tmp_time));
	return time_buf;
}

} // namespace Remember

using namespace Remember;

void CharRemember::add_str(std::string text, int flag)
{
	std::string buffer = time_format();
	buffer += text;

	switch (flag)
	{
	case ALL:
		all_.push_back(buffer);
		if (all_.size() > MAX_REMEMBER_NUM)
		{
			all_.erase(all_.begin());
		}
		break;
	case PERSONAL:
		personal_.push_back(buffer);
		if (personal_.size() > MAX_REMEMBER_NUM)
		{
			personal_.erase(personal_.begin());
		}
		break;
	case GROUP:
		break;
	case PRAY:
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
		return;
	}
}

std::string CharRemember::get_text(int flag) const
{
	std::string buffer;

	switch (flag)
	{
	case ALL:
	{
		RememberListType::const_iterator it = all_.begin();
		if (all_.size() > num_str_)
		{
			std::advance(it, all_.size() - num_str_);
		}
		for (; it != all_.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	case PERSONAL:
	{
		RememberListType::const_iterator it = personal_.begin();
		if (personal_.size() > num_str_)
		{
			std::advance(it, personal_.size() - num_str_);
		}
		for (; it != personal_.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	case GROUP:
		break;
	case PRAY:
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
		break;
	}
	if (buffer.empty())
	{
		buffer = "Вам нечего вспомнить.\r\n";
	}
	return buffer;
}

void CharRemember::set_answer_id(int id)
{
	answer_id_ = id;
}

int CharRemember::get_answer_id() const
{
	return answer_id_;
}

void CharRemember::reset()
{
	personal_.clear();
	group_.clear();
	pray_.clear();
	answer_id_ = NOBODY;
	last_tell_ = "";
}

bool CharRemember::set_num_str(unsigned int num)
{
	if (num >= 1 && num <= MAX_REMEMBER_NUM)
	{
		num_str_ = num;
		return true;
	}
	return false;
}

unsigned int CharRemember::get_num_str() const
{
	return num_str_;
}
