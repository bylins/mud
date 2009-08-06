// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <algorithm>
#include "remember.hpp"
#include "utils.h"

namespace Remember
{

// кол-во запоминаемых строк в каждом списке
const unsigned int MAX_REMEMBER_NUM = 100;

} // namespace Remember

using namespace Remember;

void CharRemember::add_str(std::string text, int flag)
{
	switch (flag)
	{
	case PERSONAL:
		personal_.push_back(text);
		if (personal_.size() > MAX_REMEMBER_NUM)
		{
			personal_.erase(personal_.begin());
		}
	case GROUP:
	case PRAY:
	default:
		log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
		break;
	}
}

std::string CharRemember::get_text(int flag) const
{
	std::string buffer;

	switch (flag)
	{
	case PERSONAL:
	{
		RememberListType::const_iterator it = personal_.begin();
		if (personal_.size() > num_str_)
		{
			std::advance(it, personal_.size() - num_str_);
		}
		for (; it != personal_.end(); ++it)
		{
			buffer += *it + "\r\n";
		}
	}
	case GROUP:
	case PRAY:
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
