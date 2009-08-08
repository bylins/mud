// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <algorithm>
#include "remember.hpp"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "char.hpp"
#include "interpreter.h"
#include "screen.h"

namespace Remember
{

// болтать/орать
RememberListType gossip;
// оффтоп
RememberListType offtop;
// воззвания (иммам)
RememberListType pray;

std::string time_format()
{
	char time_buf[9];
	time_t tmp_time = time(0);
	strftime(time_buf, sizeof(time_buf), "[%H:%M] ", localtime(&tmp_time));
	return time_buf;
}

/**
* Болтовня ch, пишущаяся во вспом все к vict'иму. Изврат конечно, но переделывать
* систему в do_gen_comm чет облом пока, а возвращать сформированную строку из act() не хочется.
*/
std::string format_gossip(CHAR_DATA *ch, CHAR_DATA *vict, int cmd, const char *argument)
{
	snprintf(buf, MAX_STRING_LENGTH, "%s%s %s%s : '%s'%s\r\n",
			cmd == SCMD_GOSSIP ? CCYEL(vict, C_NRM) : CCIYEL(vict, C_NRM),
			PERS(ch, vict, 0), cmd == SCMD_GOSSIP ? "заметил" : "заорал",
			GET_CH_VIS_SUF_1(ch, vict), argument,
			CCNRM(vict, C_NRM));
	return buf;
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
	case GOSSIP:
		gossip.push_back(buffer);
		if (gossip.size() > MAX_REMEMBER_NUM)
		{
			gossip.erase(gossip.begin());
		}
		break;
	case OFFTOP:
		offtop.push_back(buffer);
		if (offtop.size() > MAX_REMEMBER_NUM)
		{
			offtop.erase(offtop.begin());
		}
		break;
	case PRAY:
		pray.push_back(buffer);
		if (pray.size() > MAX_REMEMBER_NUM)
		{
			pray.erase(pray.begin());
		}
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
	case GOSSIP:
	{
		RememberListType::const_iterator it = gossip.begin();
		if (gossip.size() > num_str_)
		{
			std::advance(it, gossip.size() - num_str_);
		}
		for (; it != gossip.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	case OFFTOP:
	{
		RememberListType::const_iterator it = offtop.begin();
		if (offtop.size() > num_str_)
		{
			std::advance(it, offtop.size() - num_str_);
		}
		for (; it != offtop.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	case PRAY:
	{
		RememberListType::const_iterator it = pray.begin();
		if (pray.size() > num_str_)
		{
			std::advance(it, pray.size() - num_str_);
		}
		for (; it != pray.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
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
	all_.clear();
	personal_.clear();
//	group_.clear();
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
