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
boost::format dps_stat_format(" %25s |  %15d | %5d |  %5d | %11d |\r\n");
boost::format dps_group_stat_format(" %25s |  %8d | %3.0f%% | %5d |  %5d | %11d |\r\n");

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
	total_time_ += MAX(1, tmp_time);
	curr_time_ = 0;
	end_round();
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
		buf_dmg_ += dmg;
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
		if (name_.size() > 25)
		{
			name_ = name_.substr(0, 25);
		}
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

void DpsNode::end_round()
{
	if (round_dmg_ < buf_dmg_)
	{
		round_dmg_ = buf_dmg_;
	}
	buf_dmg_ = 0;
}

unsigned DpsNode::get_round_dmg() const
{
	return round_dmg_;
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
			stop_group_charm_timer(ch);
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
			add_group_charm_dmg(ch, dmg, over_dmg);
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
		exp_ = 0;
		battle_exp_ = 0;
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

void Dps::end_round(int type, CHAR_DATA *ch)
{
	switch (type)
	{
	case PERS_DPS:
		pers_dps_.end_round();
		break;
	case PERS_CHARM_DPS:
		pers_dps_.end_charm_round(GET_ID(ch));
		break;
	case GROUP_DPS:
		end_group_round(GET_ID(ch));
		break;
	case GROUP_CHARM_DPS:
		end_group_charm_round(ch);
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
		return;
	}
}

/**
* Для сортировки вывода по нанесенной дамаге.
*/
struct sort_node
{
	sort_node(std::string in_name, int in_dps, unsigned in_round_dmg, unsigned in_over_dmg)
			: dps(in_dps), round_dmg(in_round_dmg), over_dmg(in_over_dmg)
	{
		name = in_name.substr(0, 25);
	};

	std::string name;
	int dps;
	unsigned round_dmg;
	unsigned over_dmg;
};

typedef std::multimap<unsigned /* dmg */, sort_node> SortGroupType;
SortGroupType tmp_group_list;
// суммарный дамаг группы при распечатке статистики
unsigned tmp_total_dmg = 0;

void Dps::add_tmp_group_list(CHAR_DATA *ch)
{
	GroupListType::iterator it = group_dps_.find(GET_ID(ch));
	if (it != group_dps_.end())
	{
		sort_node tmp_node(it->second.get_name(), it->second.get_stat(),
				it->second.get_round_dmg(), it->second.get_over_dmg());
		tmp_group_list.insert(std::make_pair(it->second.get_dmg(), tmp_node));
		tmp_total_dmg += it->second.get_dmg();
		it->second.print_group_charm_stats(ch);
	}
}

/**
* Распечатка персональной статистики игрока и его чармисов.
* \param ch - игрок, которому идет распечатка.
*/
void Dps::print_stats(CHAR_DATA *ch)
{
	send_to_char("Персональная статистика:\r\n"
			"                       Имя |   Нанесено урона | В раунд (макс) | Лишний урон |\r\n"
			"---------------------------|------------------|----------------|-------------|\r\n", ch);
	send_to_char(str(dps_stat_format
			% GET_NAME(ch) % pers_dps_.get_dmg()
			% pers_dps_.get_stat() % pers_dps_.get_round_dmg()
			% pers_dps_.get_over_dmg()), ch);
	send_to_char(pers_dps_.print_charm_stats(), ch);
	double percent = exp_ ? battle_exp_ * 100.0 / exp_ : 0.0;
	send_to_char(ch, "\r\nВсего получено опыта: %d, за удары: %d (%.2f%%)\r\n", exp_, battle_exp_, percent);

	if (AFF_FLAGGED(ch, AFF_GROUP))
	{
		tmp_total_dmg = 0;
		CHAR_DATA *leader = ch->master ? ch->master : ch;
		leader->dps_print_group_stats(ch);
	}
}

/**
* Распечатка групповой статистики, находящейся у лидера группы.
* \param ch - игрок, которому идет распечатка.
*/
void Dps::print_group_stats(CHAR_DATA *ch)
{
	send_to_char("\r\nСтатистика Вашей группы:\r\n"
			"---------------------------|------------------|----------------|-------------|\r\n", ch);

	CHAR_DATA *leader = ch->master ? ch->master : ch;
	for (follow_type *f = leader->followers; f; f = f->next)
	{
		if (f->follower && !IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_GROUP))
		{
			add_tmp_group_list(f->follower);
		}
	}
	add_tmp_group_list(leader);

	std::string out;
	for (SortGroupType::reverse_iterator it = tmp_group_list.rbegin(); it != tmp_group_list.rend(); ++it)
	{
		double percent = tmp_total_dmg ? it->first * 100.0 / tmp_total_dmg : 0.0;
		out += (str(dps_group_stat_format % it->second.name % it->first % percent
				% it->second.dps % it->second.round_dmg % it->second.over_dmg));
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
	else
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
	}
}

void Dps::add_group_dmg(int id, int dmg, int over_dmg)
{
	GroupListType::iterator it = group_dps_.find(id);
	if (it != group_dps_.end())
	{
		it->second.add_dmg(dmg, over_dmg);
	}
	else
	{
		log("dps %s : %d %d %d", __func__, id, dmg, over_dmg);
	}
}

void Dps::end_group_round(int id)
{
	GroupListType::iterator it = group_dps_.find(id);
	if (it != group_dps_.end())
	{
		it->second.end_round();
	}
	else
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
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

void Dps::stop_group_charm_timer(CHAR_DATA *ch)
{
	GroupListType::iterator it = group_dps_.find(GET_ID(ch->master));
	if (it != group_dps_.end())
	{
		it->second.stop_charm_timer(GET_ID(ch));
	}
	else
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
	}
}

void Dps::add_group_charm_dmg(CHAR_DATA *ch, int dmg, int over_dmg)
{
	GroupListType::iterator it = group_dps_.find(GET_ID(ch->master));
	if (it != group_dps_.end())
	{
		it->second.add_charm_dmg(GET_ID(ch), dmg, over_dmg);
	}
	else
	{
		log("dps %s : %s %d %d", __func__, GET_NAME(ch), dmg, over_dmg);
	}
}

void Dps::end_group_charm_round(CHAR_DATA *ch)
{
	GroupListType::iterator it = group_dps_.find(GET_ID(ch->master));
	if (it != group_dps_.end())
	{
		it->second.end_charm_round(GET_ID(ch));
	}
	else
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
	}
}

/**
* Чтобы не морочить голову в dps_copy, заменяем только груп.статистику.
*/
Dps & Dps::operator= (const Dps &copy)
{
	if (this != &copy)
	{
		group_dps_ = copy.group_dps_;
	}
	return *this;
}

void Dps::add_exp(int exp)
{
	exp_ += exp;
}

void Dps::add_battle_exp(int exp)
{
	battle_exp_ += exp;
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
	else
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
	}
}

void PlayerDpsNode::add_charm_dmg(int id, int dmg, int over_dmg)
{
	CharmListType::iterator it = find_charmice(id);
	if (it != charm_list_.end())
	{
		it->add_dmg(dmg, over_dmg);
	}
	else
	{
		log("dps %s : %d %d %d", __func__, id, dmg, over_dmg);
	}
}

void PlayerDpsNode::end_charm_round(int id)
{
	CharmListType::iterator it = find_charmice(id);
	if (it != charm_list_.end())
	{
		it->end_round();
	}
	else
	{
		log("SYSERROR: мы не должны были сюда попасть, func: %s", __func__);
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
		text << dps_stat_format % it->get_name() % it->get_dmg()
				% it->get_stat() % it->get_round_dmg() % it->get_over_dmg();
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
			sort_node tmp_node(it->get_name(), it->get_stat(), it->get_round_dmg(), it->get_over_dmg());
			tmp_group_list.insert(std::make_pair(it->get_dmg(), tmp_node));
			tmp_total_dmg += it->get_dmg();
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
