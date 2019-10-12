/* ****************************************************************************
* File: top.cpp                                                Part of Bylins *
* Usage: Топ игроков пошустрее                                                *
* (c) 2006 Krodo                                                              *
******************************************************************************/

#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "screen.h"
#include "top.h"
#include "glory_const.hpp"
#include "char.hpp"
#include "conf.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <functional>
#include <sstream>
#include <iomanip>

extern const char *class_name[];

TopListType TopPlayer::TopList(NUM_PLAYER_CLASSES);

// отдельное удаление из списка (для ренеймов, делетов и т.п.)
// данная функция работает в том числе и с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void TopPlayer::Remove(CHAR_DATA * short_ch)
{
	std::list<TopPlayer> &tmp_list = TopPlayer::TopList[static_cast<int>(GET_CLASS(short_ch))];

	std::list<TopPlayer>::iterator it = std::find_if(tmp_list.begin(), tmp_list.end(),
		[&](const TopPlayer& p)
	{
		return p.unique == GET_UNIQUE(short_ch);
	});

	if (it != tmp_list.end())
		tmp_list.erase(it);
}

// проверяем надо-ли добавлять в топ и добавляем/обновляем при случае. reboot по дефолту 0 (1 для ребута)
// данная функция работает в том числе и с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void TopPlayer::Refresh(CHAR_DATA * short_ch, bool reboot)
{
	if (IS_NPC(short_ch)
		|| PLR_FLAGS(short_ch).get(PLR_FROZEN)
		|| PLR_FLAGS(short_ch).get(PLR_DELETED)
		|| IS_IMMORTAL(short_ch))
	{
		return;
	}

	if (!reboot)
		TopPlayer::Remove(short_ch);

	// шерстим список по ремортам и экспе и смотрим куда воткнуться
	std::list<TopPlayer>::iterator it_exp;
	for (it_exp = TopPlayer::TopList[GET_CLASS(short_ch)].begin(); it_exp != TopPlayer::TopList[GET_CLASS(short_ch)].end(); ++it_exp)
		if (it_exp->remort < GET_REMORT(short_ch) || (it_exp->remort == GET_REMORT(short_ch) && it_exp->exp < GET_EXP(short_ch)))
			break;

	if (short_ch->get_name().empty())
	{
		return; // у нас все может быть
	}
	TopPlayer temp_player(GET_UNIQUE(short_ch), GET_NAME(short_ch), GET_EXP(short_ch), GET_REMORT(short_ch));

	if (it_exp != TopPlayer::TopList[GET_CLASS(short_ch)].end())
		TopPlayer::TopList[GET_CLASS(short_ch)].insert(it_exp, temp_player);
	else
		TopPlayer::TopList[GET_CLASS(short_ch)].push_back(temp_player);
}

const char * TopPlayer::TopFormat[NUM_PLAYER_CLASSES + 1] =
{
	"лекари",
	"колдуны",
	"тати",
	"богатыри",
	"наемники",
	"дружинники",
	"кудесники",
	"волшебники",
	"чернокнижники",
	"витязи",
	"охотники",
	"кузнецы",
	"купцы",
	"волхвы",
	"игроки"
};

// команда 'лучшие'
void DoBest(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	boost::trim(buffer);

	bool find = 0;
	int class_num = 0;
	// тут и далее <= для учета 'игроки' после классов
	for (; class_num <= NUM_PLAYER_CLASSES; ++class_num)
	{
		if (CompareParam(buffer, TopPlayer::TopFormat[class_num]))
		{
			find = 1;
			break;
		}
	}

	if (find)
	{
		std::ostringstream out;
		out << CCWHT(ch, C_NRM) << "Лучшие " << TopPlayer::TopFormat[class_num] << ":" << CCNRM(ch, C_NRM) << "\r\n";

		if (class_num < NUM_PLAYER_CLASSES)   // конкретная профа
		{
			boost::format class_format("\t%-20s %-2d %s\r\n");
			int i = 0;
			for (std::list<TopPlayer>::const_iterator it = TopPlayer::TopList[class_num].begin(); it != TopPlayer::TopList[class_num].end() && i < MAX_TOP_CLASS; ++it, ++i)
				out << class_format % it->name % it->remort % desc_count(it->remort, WHAT_REMORT);

			// если игрок участвует в данном топе - покажем ему, какой он неудачник
			int count = 1;
			std::list<TopPlayer>::iterator find_me = TopPlayer::TopList[class_num].begin();
			for (; find_me != TopPlayer::TopList[class_num].end(); ++find_me, ++count)
				if (find_me->unique == GET_UNIQUE(ch))
					break;
			if (find_me != TopPlayer::TopList[class_num].end())
				out << "Ваш текущий рейтинг: " << count << "\r\n";

			send_to_char(out.str().c_str(), ch);
		}
		else   // все профы
		{
			int i = 0;
			boost::format all_format("\t%-20s %-2d %-17s %s\r\n");
			for (TopListType::const_iterator it = TopPlayer::TopList.begin(); it != TopPlayer::TopList.end(); ++it, ++i)
				if (!it->empty())
					out << all_format % it->begin()->name % it->begin()->remort % desc_count(it->begin()->remort, WHAT_REMORT) % class_name[i];
			send_to_char(out.str().c_str(), ch);
		}
	}
	else
	{
		// топ славы
		if (CompareParam(buffer, "прославленные"))
		{
			GloryConst::print_glory_top(ch);
			return;
		}

		std::ostringstream out;
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << "Лучшими могут быть:\r\n";
		for (int i = 0, j = 1; i <= NUM_PLAYER_CLASSES; ++i, ++j)
			out << std::setw(15) << TopPlayer::TopFormat[i] << (j % 4 ? "" : "\r\n");

		out << std::setw(15) << "прославленные\r\n";
		out << "\r\n";
		send_to_char(out.str().c_str(), ch);
		return;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
