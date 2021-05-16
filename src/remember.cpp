// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "remember.hpp"

#include "logger.hpp"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "chars/character.h"
#include "interpreter.h"
#include "screen.h"
#include "house.h"
#include "room.hpp"

#include <boost/format.hpp>

#include <algorithm>

namespace Remember
{

// болтать/орать
RememberListType gossip;
// оффтоп
RememberListType offtop;
// воззвания (иммам)
RememberListType imm_pray;
// гбогам/wiznet
RememberWiznetListType wiznet_;

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
	if (ch->get_name().empty())
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
	return str(boost::format("%1%%2% %3%%4% : '%5%'%6%\r\n")
			% (cmd == SCMD_GOSSIP ? CCYEL(vict, C_NRM) : CCIYEL(vict, C_NRM))
			% format_gossip_name(ch, vict).c_str()
			% (cmd == SCMD_GOSSIP ? "заметил" : "заорал")
			% GET_CH_VIS_SUF_1(ch, vict)
			% argument
			% CCNRM(vict, C_NRM));
}

// * Анти-копипаст для CharRemember::add_str.
void add_to_cont(RememberListType &cont, const std::string &text)
{
	cont.push_back(text);
	if (cont.size() > MAX_REMEMBER_NUM)
	{
		cont.erase(cont.begin());
	}
}

// * Анти-копипаст для CharRemember::get_text.
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

void add_to_flaged_cont(RememberWiznetListType &cont, const std::string &text, const int level)
{
	RememberMsgPtr temp_msg_ptr(new RememberMsg);
	temp_msg_ptr->Msg = time_format() + text;
	temp_msg_ptr->level = level;
	cont.push_back(temp_msg_ptr);
	if (cont.size() > MAX_REMEMBER_NUM)
	{
		cont.erase(cont.begin());
	}
}

std::string get_from_flaged_cont(const RememberWiznetListType &cont, unsigned int num_str, const int level)
{
	std::string text;
	RememberWiznetListType::const_iterator it = cont.begin();
	if (cont.size() > num_str)
	{
		std::advance(it, cont.size() - num_str);
	}
	for (; it != cont.end(); ++it)
	{
		if(level >= (*it)->level)
			text += (*it)->Msg;
	}
	if (text.empty())
	{
		text = "Вам нечего вспомнить.\r\n";
	}
	return text;
}

} // namespace Remember

using namespace Remember;

// * Добавление строки в список (flag).
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
	case GROUP:// added by WorM  групптелы 2010.10.13
		add_to_cont(group_, buffer);
		break;
	case GOSSIP:
		add_to_cont(gossip, buffer);
		break;
	case OFFTOP:
		add_to_cont(offtop, buffer);
		break;
	case PRAY:
		add_to_cont(imm_pray, buffer);
		break;
	case PRAY_PERSONAL:
		add_to_cont(pray_, buffer);
		add_to_cont(all_, buffer);
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
		return;
	}
}

// * Вывод списка (flag), ограниченного числом из режима вспомнить.
std::string CharRemember::get_text(int flag) const
{
	std::string buffer;

	switch (flag)
	{
	case ALL:
		buffer = get_from_cont(all_, num_str_);
		break;
	case PERSONAL:
		buffer = get_from_cont(personal_, num_str_);
		break;
	case GROUP:// added by WorM  групптелы 2010.10.13
		buffer = get_from_cont(group_, num_str_);
		break;
	case GOSSIP:
		buffer = get_from_cont(gossip, num_str_);
		break;
	case OFFTOP:
		buffer = get_from_cont(offtop, num_str_);
		break;
	case PRAY:
		buffer = get_from_cont(imm_pray, num_str_);
		break;
	case PRAY_PERSONAL:
		buffer = get_from_cont(pray_, num_str_);
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

void CharRemember::reset()
{
	all_.clear();
	personal_.clear();
	pray_.clear();
	group_.clear();// added by WorM  групптелы 2010.10.13
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

void do_remember_char(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char arg[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	// Если без аргумента - выдает личные теллы
	if (!*argument)
	{
		send_to_char(ch->remember_get(Remember::PERSONAL), ch);
		return;
	}

	argument = one_argument(argument, arg);

	if (is_abbrev(arg, "воззвать"))
	{
		if (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char(ch->remember_get(Remember::PRAY), ch);
		}
		else
		{
			send_to_char(ch->remember_get(Remember::PRAY_PERSONAL), ch);
		}
	}
	else if ((GET_LEVEL(ch) < LVL_IMMORT || IS_IMPL(ch)) && is_abbrev(arg, "оффтоп"))
	{
		if (!PRF_FLAGGED(ch, PRF_IGVA_PRONA))
		{
			send_to_char(ch->remember_get(Remember::OFFTOP), ch);
		}
		else
		{
			send_to_char(ch, "Вам нечего вспомнить.\r\n");
		}
	}
	else if (is_abbrev(arg, "болтать") || is_abbrev(arg, "орать"))
	{
		send_to_char(ch->remember_get(Remember::GOSSIP), ch);
	}
	else if (is_abbrev(arg, "группа") || is_abbrev(arg, "ггруппа"))// added by WorM  групптелы 2010.10.13
	{
		send_to_char(ch->remember_get(Remember::GROUP), ch);
	}
	else if (is_abbrev(arg, "клан") || is_abbrev(arg, "гдругам"))
	{
		if (CLAN(ch))
		{
			send_to_char(CLAN(ch)->get_remember(ch->remember_get_num(), Remember::CLAN), ch);
		}
		else
		{
			send_to_char(ch, "Вам нечего вспомнить.\r\n");
		}
		return;
	}
	else if (is_abbrev(arg, "союзники") || is_abbrev(arg, "альянс") || is_abbrev(arg, "гсоюзникам"))
	{
		if (CLAN(ch))
		{
			send_to_char(CLAN(ch)->get_remember(ch->remember_get_num(), Remember::ALLY), ch);
		}
		else
		{
			send_to_char(ch, "Вам нечего вспомнить.\r\n");
		}
		return;
	}
	else if (is_abbrev(arg, "гбогам") && IS_IMMORTAL(ch))
	{
		send_to_char(get_from_flaged_cont(wiznet_, ch->remember_get_num(), GET_LEVEL(ch)), ch);
		return;
	}
	else if (is_abbrev(arg, "все"))
	{
		send_to_char(ch->remember_get(Remember::ALL), ch);
		return;
	}
	else
	{
		if (IS_IMMORTAL(ch) && !IS_IMPL(ch))
			send_to_char("Формат команды: вспомнить [без параметров|болтать|воззвать|гг|гд|гс|гб|все]\r\n", ch);
		else
			send_to_char("Формат команды: вспомнить [без параметров|болтать|оффтоп|гг|гд|гс|все]\r\n", ch);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
