/* ****************************************************************************
* File: top.cpp                                                Part of Bylins *
* Usage: Топ игроков пошустрее                                                *
* (c) 2006 Krodo                                                              *
******************************************************************************/

#include <functional>
#include <sstream>
#include <iomanip>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "screen.h"
#include "top.h"

extern const char *class_name[];

TopListType TopPlayer::TopList (NUM_CLASSES);

// отдельное удаление из списка (для ренеймов, делетов и т.п.)
void TopPlayer::Remove(CHAR_DATA * ch)
{
	TopPlayer::TopList[static_cast<int>(GET_CLASS(ch))].remove_if(
		boost::bind(std::equal_to<long>(),
			boost::bind(&TopPlayer::unique, _1), GET_UNIQUE(ch)));
}

// проверяем надо-ли добавлять в топ и добавляем/обновляем при случае. reboot по дефолту 0 (1 для ребута)
void TopPlayer::Refresh(CHAR_DATA * ch, bool reboot)
{
	if (IS_NPC(ch) || IS_SET(PLR_FLAGS(ch, PLR_FROZEN), PLR_FROZEN) || IS_SET(PLR_FLAGS(ch, PLR_DELETED), PLR_DELETED) || IS_IMMORTAL(ch))
		return;

	int class_num = static_cast<int>(GET_CLASS(ch)); // а то уж больно плохо смотрится

	if (!reboot)
		TopPlayer::Remove(ch);

	// шерстим список по ремортам и экспе и смотрим куда воткнуться, TODO: обновить у ся буст и воткнуть тут find_if с трехэтажным биндом
	std::list<TopPlayer>::iterator it_exp;
	for (it_exp = TopPlayer::TopList[class_num].begin(); it_exp != TopPlayer::TopList[class_num].end(); ++it_exp)
		if (it_exp->remort < GET_REMORT(ch) || it_exp->remort == GET_REMORT(ch) && it_exp->exp < GET_EXP(ch))
			break;

	if (!GET_NAME(ch)) return; // у нас все может быть
	TopPlayer temp_player(GET_UNIQUE(ch), GET_NAME(ch), GET_EXP(ch), GET_REMORT(ch));

	if (it_exp != TopPlayer::TopList[class_num].end())
		TopPlayer::TopList[class_num].insert(it_exp, temp_player);
	else
		TopPlayer::TopList[class_num].push_back(temp_player);
}

const char * TopPlayer::TopFormat[NUM_CLASSES + 1] =
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
ACMD(DoBest)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	boost::trim(buffer);

	bool find = 0;
	int class_num = 0;
	// тут и далее <= для учета 'игроки' после классов
	for (; class_num <= NUM_CLASSES; ++class_num) {
		if (CompareParam(buffer, TopPlayer::TopFormat[class_num])) {
			find = 1;
			break;
		}
	}

	if (find) {
		std::ostringstream out;
		out << CCWHT(ch, C_NRM) << "Лучшие " << TopPlayer::TopFormat[class_num] << ":" << CCNRM(ch, C_NRM) << "\r\n";

		if (class_num < NUM_CLASSES) { // конкретная профа
			boost::format class_format("\t%-20s %-2d %s\r\n");
			int i = 0;
			for (std::list<TopPlayer>::const_iterator it = TopPlayer::TopList[class_num].begin(); it != TopPlayer::TopList[class_num].end() && i < MAX_TOP_CLASS; ++it, ++i)
				out << class_format % it->name % it->remort % desc_count(it->remort, WHAT_REMORT);
			send_to_char(ch, out.str().c_str());
		} else { // все профы
			int i = 0;
			boost::format all_format("\t%-20s %-2d %-17s %s\r\n");
			for (TopListType::const_iterator it = TopPlayer::TopList.begin(); it != TopPlayer::TopList.end(); ++it, ++i)
				if (!it->empty())
					out << all_format % it->begin()->name % it->begin()->remort % desc_count(it->begin()->remort, WHAT_REMORT) % class_name[i];
			send_to_char(ch, out.str().c_str());
		}
	} else {
		std::ostringstream out;
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << "Лучшими могут быть:\r\n";
		for (int i = 0, j = 1; i <= NUM_CLASSES; ++i, ++j)
			out << std::setw(15) << TopPlayer::TopFormat[i] << (j % 5 ? "" : "\r\n");
		out << "\r\n";
		send_to_char(ch, out.str().c_str());
		return;
	}
}
