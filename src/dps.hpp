// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DPS_HPP_INCLUDED
#define DPS_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#include <list>
#include <map>
#include <string>

namespace DpsSystem
{

void check_round(CHAR_DATA *ch);

// режимы, чтобы не плодить оберток (себя, своих чармисов, чары из группы, чармисы чаров из группы)
enum { PERS_DPS, PERS_CHARM_DPS, GROUP_DPS, GROUP_CHARM_DPS };

class DpsNode
{
public:
	DpsNode(long id = 0) : dmg_(0), over_dmg_(0), id_(id),
			round_dmg_(0), buf_dmg_(0), rounds_(0) {};
	void add_dmg(int dmg, int over_dmg);
	void set_name(const char *name);
	int get_stat() const;
	unsigned get_dmg() const;
	unsigned get_over_dmg() const;
	long get_id() const;
	const std::string & get_name() const;
	unsigned get_round_dmg() const;
	void end_round();

private:
	// нанесенный дамаг
	unsigned dmg_;
	// часть дамага по уже мертвой целе
	unsigned over_dmg_;
	// имя для вывода в списке при отсутствии в игре
	std::string name_;
	// для идентификации
	long id_;
	// наибольший дамаг за раунд
	unsigned round_dmg_;
	// для сбора дамаги за раунд
	unsigned buf_dmg_;
	// сколько всего раундов проведено в бою
	int rounds_;
};

typedef std::list<DpsNode> CharmListType;

// * Обертся на DpsNode со списком чармисов (для плеера).
class PlayerDpsNode : public DpsNode
{
public:
	void add_charm_dmg(CHAR_DATA *ch, int dmg, int over_dmg);
	std::string print_charm_stats() const;
	void print_group_charm_stats(CHAR_DATA *ch) const;
	void end_charm_round(CHAR_DATA *ch);

private:
	CharmListType::iterator find_charmice(CHAR_DATA *ch);

	// список чармисов (MAX_DPS_CHARMICE)
	CharmListType charm_list_;
};

typedef std::map<long /* id */, PlayerDpsNode> GroupListType;

// * Внешний интефейс, видимый в Player.
class Dps
{
public:
	Dps() : exp_(0), battle_exp_(0), lost_exp_(0) {};
	Dps & operator= (const Dps &copy);

	void add_dmg(int type, CHAR_DATA *ch, int dmg, int over_dmg);
	void clear(int type);
	void print_stats(CHAR_DATA *ch, CHAR_DATA *coder = 0);
	void print_group_stats(CHAR_DATA *ch, CHAR_DATA *coder = 0);
	void end_round(int type, CHAR_DATA *ch);

	void add_exp(int exp);
	void add_battle_exp(int exp);

private:
	void add_tmp_group_list(CHAR_DATA *ch);
	void add_group_dmg(CHAR_DATA *ch, int dmg, int over_dmg);
	void end_group_round(CHAR_DATA *ch);
	void add_group_charm_dmg(CHAR_DATA *ch, int dmg, int over_dmg);
	void end_group_charm_round(CHAR_DATA *ch);

	// групповая статистика
	GroupListType group_dps_;
	// персональная статистика и свои чармисы
	PlayerDpsNode pers_dps_;
	// набрано экспы, включая бэтл-экспу
	int exp_;
	// набрано бэтл-экспы
	int battle_exp_;
	// потеряно экспы
	int lost_exp_;
};

} // namespace DpsSystem

#endif // DPS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
