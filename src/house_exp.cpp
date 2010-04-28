// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <fstream>
#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "house_exp.hpp"
#include "structs.h"
#include "utils.h"
#include "house.h"
#include "comm.h"
#include "room.hpp"

namespace
{

// количество нодов в списке экспы (зависит от CLAN_EXP_UPDATE_PERIOD)
const unsigned int MAX_LIST_NODES = 720;

} // namespace

/**
* Добавление экспы во временный буффер.
*/
void ClanExp::add_temp(int exp)
{
	buffer_exp_ += exp;
}

/**
* Добавление ноды в список экспы и выталкивание старой записи.
*/
void ClanExp::add_chunk()
{
	list_.push_back(buffer_exp_);
	if (list_.size() > MAX_LIST_NODES)
	{
		list_.erase(list_.begin());
	}
	buffer_exp_ = 0;
	update_total_exp();
}

/**
* Общее кол-во экспы в списке.
*/
long long ClanExp::get_exp() const
{
	return total_exp_;
}

/**
* Сохранение списка и буффера в отдельный файл клана (по аббревиатуре).
*/
void ClanExp::save(const std::string &abbrev) const
{
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + ".exp";
	std::ofstream file(filename.c_str());
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	std::stringstream out;
	out << buffer_exp_ << "\n";
	for (ExpListType::const_iterator it = list_.begin(); it != list_.end(); ++it)
	{
		out << *it << "\n";
	}
	file << out.rdbuf();
}

/**
* Загрузка списка экспы и буффера конкретного клана (по аббревиатуре).
*/
void ClanExp::load(const std::string &abbrev)
{
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + ".exp";
	std::ifstream file(filename.c_str());
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	if (!(file >> buffer_exp_))
	{
		log("Error read buffer_exp_: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	long long tmp_exp;
	while (file >> tmp_exp && list_.size() < MAX_LIST_NODES)
	{
		list_.push_back(tmp_exp);
	}
	update_total_exp();
}

void ClanExp::update_total_exp()
{
	total_exp_ = 0;
	for (ExpListType::const_iterator it = list_.begin(); it != list_.end(); ++it)
	{
		total_exp_ += *it;
	}
}

/**
* Добавление ноды (раз в час).
*/
void update_clan_exp()
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		(*clan)->last_exp.add_chunk();
	}
}

/**
* Сохранение списков (раз в час и на ребуте).
*/
void save_clan_exp()
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		(*clan)->last_exp.save((*clan)->get_file_abbrev());
		(*clan)->exp_history.save((*clan)->get_file_abbrev());
	}
}

////////////////////////////////////////////////////////////////////////////////
// ClanPkLog

namespace
{

// макс. кол-во записей в списке сражений
const unsigned MAX_PK_LOG = 15;

} // namespace


void ClanPkLog::add(const std::string &text)
{
	pk_log.push_back(text);
	if (pk_log.size() > MAX_PK_LOG)
	{
		pk_log.pop_front();
	}
	need_save = true;
}

void ClanPkLog::print(CHAR_DATA *ch) const
{
	std::string text;
	for (std::list<std::string>::const_iterator i = pk_log.begin(); i != pk_log.end(); ++i)
	{
		text += *i;
	}

	if (!text.empty())
	{
		send_to_char(ch, "Последние пк-события с участием членов дружины:\r\n%s", text.c_str());
	}
	else
	{
		send_to_char("Пусто.\r\n", ch);
	}
}

void ClanPkLog::save(const std::string &abbrev)
{
	if (!need_save)
	{
		return;
	}

	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + ".war";
	if (pk_log.empty())
	{
		remove(filename.c_str());
		return;
	}

	std::ofstream file(filename.c_str());
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	for (std::list<std::string>::const_iterator i = pk_log.begin(); i != pk_log.end(); ++i)
	{
		file << *i;
	}

	file.close();
	need_save = false;
}

void ClanPkLog::load(const std::string &abbrev)
{
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + ".war";

	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	while(std::getline(file, buffer))
	{
		boost::trim(buffer);
		buffer += "\r\n";
		pk_log.push_back(buffer);
	}
	file.close();
}

void ClanPkLog::check(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (!ch || !victim || ch->purged() || victim->purged()
		|| IS_NPC(victim) || !CLAN(victim) || ch == victim
		|| (ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA) && !RENTABLE(victim)))
	{
		return;
	}
	CHAR_DATA *killer = ch;
	if (IS_NPC(killer)
		&& killer->master && !IS_NPC(killer->master))
	{
		killer = killer->master;
	}
	if (!IS_NPC(killer) && CLAN(killer) != CLAN(victim))
	{
		char time_buf[17];
		time_t curr_time = time(0);
		strftime(time_buf, sizeof(time_buf), "%d-%m-%Y", localtime(&curr_time));
		std::stringstream out;
		out << time_buf << ": "
			<< GET_NAME(victim) << " убит" << GET_CH_SUF_6(victim) << " "
			<< GET_PAD(killer, 4) << "\r\n";

		CLAN(victim)->pk_log.add(out.str());
		if (CLAN(killer))
		{
			CLAN(killer)->pk_log.add(out.str());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// ClanExpHistory

void ClanExpHistory::add_exp(long exp)
{
	if (exp <= 0)
	{
		return;
	}
	char time_str[10];
	time_t curr_time = time(0);
	strftime(time_str, sizeof(time_str), "%Y.%m", localtime(&curr_time));
	std::string time_cpp_str(time_str);

	HistoryExpListType::iterator it = list_.find(time_cpp_str);
	if (it != list_.end())
	{
		it->second += exp;
	}
	else
	{
		list_[time_cpp_str] = exp;
	}
}

void ClanExpHistory::load(const std::string &abbrev)
{
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + "-history.exp";

	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	long long exp;
	while(file >> buffer >> exp)
	{
		list_[buffer] = exp;
	}
	file.close();
}

void ClanExpHistory::save(const std::string &abbrev) const
{
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + "-history.exp";
	if (list_.empty())
	{
		remove(filename.c_str());
		return;
	}

	std::ofstream file(filename.c_str());
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	for (HistoryExpListType::const_iterator i = list_.begin(); i != list_.end(); ++i)
	{
		file << i->first << " " << i->second << "\n";
	}
	file.close();
}

/**
* \param - количество последних месяцев, участвующих в расчете
* \return - набранная кланом экспа за последние num месяцев
*/
long long ClanExpHistory::get(int month) const
{
	long long exp = 0;
	int count = 0;
	for (HistoryExpListType::const_iterator i = list_.begin(), iend = list_.end(); i != iend; ++i, ++count)
	{
		if (count <= month)
		{
			exp += i->second;
		}
		else
		{
			break;
		}
	}
	return exp;
}
