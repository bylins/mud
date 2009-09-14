// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <sstream>
#include "dps.hpp"
#include "utils.h"
#include "comm.h"
#include "char.hpp"
#include "interpreter.h"
#include "db.h"
#include "handler.h"

namespace DpsSystem
{

// кол-во учитываемых чармисов с каждого чара
const unsigned MAX_DPS_CHARMICE = 5;
boost::format dps_stat_format(" %35s |  %13d |  %6d | %11d |\r\n");

/**
* Для расчета дпс, запускается при начале боя.
*/
void DpsNode::start_timer()
{
	curr_time_ = time(0);
}

/**
* В конце боя, минимум секунду.
*/
void DpsNode::stop_timer()
{
	time_t tmp_time = time(0) - curr_time_;
	tmp_time = MAX(1, tmp_time);
	total_time_ += tmp_time;
	curr_time_ = 0;
}

/**
* Добавление эффективной дамаги и овер-дамаги.
*/
void DpsNode::add_dmg(int dmg, int over_dmg)
{
	if (dmg >= 0 && over_dmg >= 0)
	{
		dmg_ += dmg;
		over_dmg_ += over_dmg;
	}
}

/**
* Имя для вывода в статистике при отсутствии онлайн.
* Пока осталось актуальным только для чармисов.
*/
void DpsNode::set_name(const char *name)
{
	if (name && *name)
	{
		name_ = name;
	}
}

/**
* Расчет дамаги в 2 секунды (раунд).
*/
int DpsNode::get_stat() const
{
	// идет бой
	if (curr_time_)
	{
		time_t tmp_time = total_time_ + (time(0) - curr_time_);
		if (tmp_time)
		{
			return (dmg_/tmp_time) * 2;
		}
		else
		{
			return 0;
		}
	}
	// тиха украинская ночь...
	if(total_time_)
	{
		return (dmg_/total_time_) * 2;
	}
	return 0;
}

unsigned DpsNode::get_dmg() const
{
	return dmg_;
}

unsigned DpsNode::get_over_dmg() const
{
	return over_dmg_;
}

const std::string & DpsNode::get_name() const
{
	return name_;
}

/**
* Используется для идентификации в списках как чармисов, так и чаров.
*/
long DpsNode::get_id() const
{
	return id_;
}

////////////////////////////////////////////////////////////////////////////////
// Dps

/**
* При старте таймера идет проверка присутствия чара/чармиса в списке
* и создание новой записи при необходимости.
*/
void Dps::start_timer(int type, CHAR_DATA *ch)
{
	switch (type)
	{
	case PERS_DPS:
		pers_dps_.start_timer();
		break;
	case PERS_CHARM_DPS:
		pers_dps_.start_charm_timer(GET_ID(ch), GET_NAME(ch));
		break;
	case GROUP_DPS:
		start_group_timer(GET_ID(ch), GET_NAME(ch));
		break;
	case GROUP_CHARM_DPS:
		if (ch && ch->master)
		{
			start_group_charm_timer(ch);
		}
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return;
	}
}

void Dps::stop_timer(int type, CHAR_DATA *ch)
{
	switch (type)
	{
	case PERS_DPS:
		pers_dps_.stop_timer();
		break;
	case PERS_CHARM_DPS:
		pers_dps_.stop_charm_timer(GET_ID(ch));
		break;
	case GROUP_DPS:
		stop_group_timer(GET_ID(ch));
		break;
	case GROUP_CHARM_DPS:
		if (ch && ch->master)
		{
			stop_group_charm_timer(GET_ID(ch->master), GET_ID(ch));
		}
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return;
	}
}

void Dps::add_dmg(int type, CHAR_DATA *ch, int dmg, int over_dmg)
{
	switch (type)
	{
	case PERS_DPS:
		pers_dps_.add_dmg(dmg, over_dmg);
		break;
	case PERS_CHARM_DPS:
		pers_dps_.add_charm_dmg(GET_ID(ch), dmg, over_dmg);
		break;
	case GROUP_DPS:
		add_group_dmg(GET_ID(ch), dmg, over_dmg);
		break;
	case GROUP_CHARM_DPS:
		if (ch && ch->master)
		{
			add_group_charm_dmg(GET_ID(ch->master), GET_ID(ch), dmg, over_dmg);
		}
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return;
	}
}

/**
* Очистка себя или группы.
*/
void Dps::clear(int type)
{
	switch (type)
	{
	case PERS_DPS:
	{
		PlayerDpsNode empty_dps;
		pers_dps_ = empty_dps;
		break;
	}
	case PERS_CHARM_DPS:
		break;
	case GROUP_DPS:
		group_dps_.clear();
		break;
	case GROUP_CHARM_DPS:
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return;
	}
}

/**
* Распечатка персональной статистики игрока и его чармисов.
* \param ch - игрок, которому идет распечатка.
*/
void Dps::print_stats(CHAR_DATA *ch)
{
	send_to_char("Персональная статистика:\r\n"
			"                                 Имя | Нанесено урона | В раунд | Лишний урон |\r\n"
			"-------------------------------------|----------------|---------|-------------|\r\n", ch);
	send_to_char(str(dps_stat_format
			% GET_NAME(ch) % pers_dps_.get_dmg() % pers_dps_.get_stat() % pers_dps_.get_over_dmg()), ch);
	send_to_char(pers_dps_.print_charm_stats(), ch);

	if (AFF_FLAGGED(ch, AFF_GROUP))
	{
		CHAR_DATA *leader = ch->master ? ch->master : ch;
		leader->dps_print_group_stats(ch);
	}
}

/**
* Для сортировки вывода по нанесенной дамаге.
*/
struct sort_node
{
	sort_node(std::string in_name, int in_dps, unsigned in_over_dmg)
			: name(in_name), dps(in_dps), over_dmg(in_over_dmg) {};

	std::string name;
	int dps;
	unsigned over_dmg;
};

typedef std::multimap<unsigned /* dmg */, sort_node> SortGroupType;
SortGroupType tmp_group_list;

/**
* Распечатка групповой статистики, находящейся у лидера группы.
* \param ch - игрок, которому идет распечатка.
*/
void Dps::print_group_stats(CHAR_DATA *ch)
{
	send_to_char("\r\nСтатистика Вашей группы:\r\n"
			"-------------------------------------|----------------|---------|-------------|\r\n", ch);
	CHAR_DATA *leader = ch->master ? ch->master : ch;
	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (f->follower && !IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_GROUP))
		{
			GroupListType::iterator it = group_dps_.find(GET_ID(f->follower));
			if (it != group_dps_.end())
			{
				sort_node tmp_node(it->second.get_name(), it->second.get_stat(), it->second.get_over_dmg());
				tmp_group_list.insert(std::make_pair(it->second.get_dmg(), tmp_node));
				it->second.print_group_charm_stats(f->follower);
			}
		}
	}
	std::string out;
	for (SortGroupType::reverse_iterator it = tmp_group_list.rbegin(); it != tmp_group_list.rend(); ++it)
	{
		out += (str(dps_stat_format % it->second.name % it->first % it->second.dps % it->second.over_dmg));
	}
	send_to_char(out, ch);
	tmp_group_list.clear();
}

void Dps::start_group_timer(int id, const char *name)
{
	GroupListType::iterator it = group_dps_.find(id);
	if (it != group_dps_.end())
	{
		it->second.start_timer();
	}
	else
	{
		PlayerDpsNode tmp_node;
		tmp_node.set_name(name);
		tmp_node.start_timer();
		group_dps_.insert(std::make_pair(id, tmp_node));
	}
}

void Dps::stop_group_timer(int id)
{
	GroupListType::iterator it = group_dps_.find(id);
	if (it != group_dps_.end())
	{
		it->second.stop_timer();
	}
}

void Dps::add_group_dmg(int id, int dmg, int over_dmg)
{
	GroupListType::iterator it = group_dps_.find(id);
	if (it != group_dps_.end())
	{
		it->second.add_dmg(dmg, over_dmg);
	}
}

void Dps::start_group_charm_timer(CHAR_DATA *ch)
{
	GroupListType::iterator it = group_dps_.find(GET_ID(ch->master));
	if (it != group_dps_.end())
	{
		it->second.start_charm_timer(GET_ID(ch), GET_NAME(ch));
	}
	else
	{
		PlayerDpsNode tmp_node;
		tmp_node.set_name(GET_NAME(ch->master));
		tmp_node.start_charm_timer(GET_ID(ch), GET_NAME(ch));
		group_dps_.insert(std::make_pair(GET_ID(ch->master), tmp_node));
	}
}

void Dps::stop_group_charm_timer(int master_id, int charm_id)
{
	GroupListType::iterator it = group_dps_.find(master_id);
	if (it != group_dps_.end())
	{
		it->second.stop_charm_timer(charm_id);
	}
}

void Dps::add_group_charm_dmg(int master_id, int charm_id, int dmg, int over_dmg)
{
	GroupListType::iterator it = group_dps_.find(master_id);
	if (it != group_dps_.end())
	{
		it->second.add_charm_dmg(charm_id, dmg, over_dmg);
	}
}

// Dps
////////////////////////////////////////////////////////////////////////////////
// PlayerDpsNode

CharmListType::iterator PlayerDpsNode::find_charmice(long id)
{
	return std::find_if(charm_list_.begin(), charm_list_.end(),
			boost::bind(std::equal_to<int>(),
			boost::bind(&DpsNode::get_id, _1), id));
}

void PlayerDpsNode::start_charm_timer(int id, const char *name)
{
	CharmListType::iterator it = find_charmice(id);
	if (it != charm_list_.end())
	{
		it->start_timer();
	}
	else
	{
		DpsNode tmp_node(id);
		tmp_node.set_name(name);
		tmp_node.start_timer();
		charm_list_.push_back(tmp_node);

		if (charm_list_.size() > MAX_DPS_CHARMICE)
		{
			charm_list_.erase(charm_list_.begin());
		}
	}
}

void PlayerDpsNode::stop_charm_timer(int id)
{
	CharmListType::iterator it = find_charmice(id);
	if (it != charm_list_.end())
	{
		it->stop_timer();
	}
}

void PlayerDpsNode::add_charm_dmg(int id, int dmg, int over_dmg)
{
	CharmListType::iterator it = find_charmice(id);
	if (it != charm_list_.end())
	{
		it->add_dmg(dmg, over_dmg);
	}
}

/**
* Чармисы в персональной статистике выводятся без сортировки.
*/
std::string PlayerDpsNode::print_charm_stats() const
{
	std::ostringstream text;
	for (CharmListType::const_reverse_iterator it = charm_list_.rbegin(); it != charm_list_.rend(); ++it)
	{
		text << dps_stat_format % it->get_name() % it->get_dmg() % it->get_stat() % it->get_over_dmg();
	}
	return text.str();
}

/**
* Распечатка групповой статистики живых чармисов по данному игроку.
*/
void PlayerDpsNode::print_group_charm_stats(CHAR_DATA *ch) const
{
	std::ostringstream text;
	for (follow_type *f = ch->followers; f; f = f->next)
	{
		if (!IS_CHARMICE(f->follower))
		{
			continue;
		}
		CharmListType::const_iterator it = std::find_if(charm_list_.begin(), charm_list_.end(),
				boost::bind(std::equal_to<int>(),
				boost::bind(&DpsNode::get_id, _1), GET_ID(f->follower)));
		if (it != charm_list_.end())
		{
			sort_node tmp_node(it->get_name(), it->get_stat(), it->get_over_dmg());
			tmp_group_list.insert(std::make_pair(it->get_dmg(), tmp_node));
		}
	}
}

// PlayerDpsNode
////////////////////////////////////////////////////////////////////////////////

} // namespace DpsSystem

/**
* 'дметр' - вывод своей статистики и групповой, если в группе.
* 'дметр очистить' - очистка своей статистики.
* 'дметр очистить группа' - очистка групповой статистики (только лидер).
*/
ACMD(do_dmeter)
{
	if (IS_NPC(ch)) return;

	char name[MAX_INPUT_LENGTH];
	two_arguments(argument, arg, name);

	if (!*arg)
	{
		ch->dps_print_stats();
	}
	if (isname(arg, "очистить"))
	{
		if (!*name)
		{
			ch->dps_clear(DpsSystem::PERS_DPS);
		}
		if (isname(name, "группа"))
		{
			if (!AFF_FLAGGED(ch, AFF_GROUP))
			{
				send_to_char("Вы не состоите в группе.\r\n", ch);
				return;
			}
			if (ch->master)
			{
				send_to_char("Вы не являетесь лидеров группы.\r\n", ch);
				return;
			}
			ch->dps_clear(DpsSystem::GROUP_DPS);
		}
	}
}
