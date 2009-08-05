// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <fstream>
#include <string>
#include <sstream>
#include "house_exp.hpp"
#include "utils.h"
#include "house.h"

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
int ClanExp::get_exp() const
{
	return total_exp_;
}

/**
* Сохранение списка и буффера в отдельный файл клана (по аббревиатуре).
*/
void ClanExp::save(std::string abbrev) const
{
	for (unsigned i = 0; i != abbrev.length(); ++i)
		abbrev[i] = LOWER(AtoL(abbrev[i]));
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
void ClanExp::load(std::string abbrev)
{
	for (unsigned i = 0; i != abbrev.length(); ++i)
		abbrev[i] = LOWER(AtoL(abbrev[i]));
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
	int tmp_exp;
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
		(*clan)->last_exp.save((*clan)->get_abbrev());
	}
}
