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
* Формирование имени болтающего/орущего при записе во 'вспом все' в обход act().
* Иммы видны всегда, кто-ты с большой буквы.
*/
std::string format_gossip_name(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (!GET_NAME(ch))
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return "";
	}
	std::string name = IS_IMMORTAL(ch) ? GET_NAME(ch) : PERS(ch, vict, 0);
	name[0] = UPPER(name[0]);
	return name;
}

/**
* Болтовня ch, пишущаяся во вспом все к vict'иму. Изврат конечно, но переделывать
* систему в do_gen_comm чет облом пока, а возвращать сформированную строку из act() не хочется.
*/
std::string format_gossip(CHAR_DATA *ch, CHAR_DATA *vict, int cmd, const char *argument)
{
	snprintf(buf, MAX_STRING_LENGTH, "%s%s %s%s : '%s'%s\r\n",
			cmd == SCMD_GOSSIP ? CCYEL(vict, C_NRM) : CCIYEL(vict, C_NRM),
			format_gossip_name(ch, vict).c_str(),
			cmd == SCMD_GOSSIP ? "заметил" : "заорал",
			GET_CH_VIS_SUF_1(ch, vict), argument,
			CCNRM(vict, C_NRM));
	return buf;
}

void add_to_cont(RememberListType &cont, const std::string &text)
{
	cont.push_back(text);
	if (cont.size() > MAX_REMEMBER_NUM)
	{
		cont.erase(cont.begin());
	}
}

std::string get_from_cont(const RememberListType &cont, unsigned int num_str)
{
	std::string text;
	RememberListType::const_iterator it = cont.begin();
	if (cont.size() > num_str)
	{
		std::advance(it, cont.size() - num_str);
	}
	for (; it != cont.end(); ++it)
	{
		text += *it;
	}
	return text;
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
		add_to_cont(all_, buffer);
		break;
	case PERSONAL:
		add_to_cont(personal_, buffer);
		break;
	case GOSSIP:
		add_to_cont(gossip, buffer);
		break;
	case OFFTOP:
		add_to_cont(offtop, buffer);
		break;
	case PRAY:
		add_to_cont(pray, buffer);
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
		buffer = get_from_cont(all_, num_str_);
		break;
	}
	case PERSONAL:
	{
		buffer = get_from_cont(personal_, num_str_);
		break;
	}
	case GOSSIP:
	{
		buffer = get_from_cont(gossip, num_str_);
		break;
	}
	case OFFTOP:
	{
		buffer = get_from_cont(offtop, num_str_);
		break;
	}
	case PRAY:
	{
		buffer = get_from_cont(pray, num_str_);
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
