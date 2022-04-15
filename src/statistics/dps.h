// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DPS_HPP_INCLUDED
#define DPS_HPP_INCLUDED

#include <list>
#include <map>
#include <string>

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "utils/table_wrapper.h"

namespace DpsSystem {

void check_round(CharData *ch);

// режимы, чтобы не плодить оберток (себя, своих чармисов, чары из группы, чармисы чаров из группы)
enum { PERS_DPS, PERS_CHARM_DPS, GROUP_DPS, GROUP_CHARM_DPS };

class DpsNode {
 public:
	explicit DpsNode(long id = 0) : dmg_(0), over_dmg_(0), id_(id),
						   round_dmg_(0), buf_dmg_(0), rounds_(0) {};
	void add_dmg(int dmg, int over_dmg);
	void set_name(const char *name);
	[[nodiscard]] int get_stat() const;
	[[nodiscard]] unsigned get_dmg() const;
	[[nodiscard]] unsigned get_over_dmg() const;
	[[nodiscard]] long get_id() const;
	[[nodiscard]] const std::string &get_name() const;
	[[nodiscard]] unsigned get_round_dmg() const;
	void end_round();

 private:
	unsigned dmg_;			// нанесенный дамаг
	unsigned over_dmg_;		// часть дамага по уже мертвой целе
	std::string name_;		// имя для вывода в списке при отсутствии в игре
	long id_; 				// для идентификации
	unsigned round_dmg_;	// наибольший дамаг за раунд
	unsigned buf_dmg_;		// для сбора дамаги за раунд
	int rounds_;			// сколько всего раундов проведено в бою
};

typedef std::list<DpsNode> CharmListType;

// * Обертся на DpsNode со списком чармисов (для плеера).
class PlayerDpsNode : public DpsNode {
 public:
	void add_charm_dmg(CharData *ch, int dmg, int over_dmg);
	void print_charm_stats(table_wrapper::Table &table) const;
	void print_group_charm_stats(CharData *ch) const;
	void end_charm_round(CharData *ch);

 private:
	CharmListType::iterator find_charmice(CharData *ch);

	// список чармисов (MAX_DPS_CHARMICE)
	CharmListType charm_list_;
};

typedef std::map<long /* id */, PlayerDpsNode> GroupListType;

// * Внешний интефейс, видимый в Player.
class Dps {
 public:
	Dps() : exp_(0), battle_exp_(0), lost_exp_(0) {};
	Dps &operator=(const Dps &copy);

	void AddDmg(int type, CharData *ch, int dmg, int over_dmg);
	void Clear(int type);
	void PrintStats(CharData *ch, CharData *coder = nullptr);
	void PrintGroupStats(CharData *ch, CharData *coder = nullptr);
	void EndRound(int type, CharData *ch);

	void AddExp(int exp);
	void AddBattleExp(int exp);

 private:
	void AddTmpGroupList(CharData *ch);
	void AddGroupDmg(CharData *ch, int dmg, int over_dmg);
	void EndGroupRound(CharData *ch);
	void AddGroupCharmDmg(CharData *ch, int dmg, int over_dmg);
	void EndGroupCharmRound(CharData *ch);
	void PrintPersonalDpsStat(CharData *ch, std::ostringstream &out);
	void PrintPersonalExpStat(std::ostringstream &out) const;

	GroupListType group_dps_;	// групповая статистика
	PlayerDpsNode pers_dps_;	// персональная статистика и свои чармисы
	int exp_;					// набрано экспы, включая бэтл-экспу
	int battle_exp_;			// набрано бэтл-экспы
	int lost_exp_;				// потеряно экспы
};

} // namespace DpsSystem

#endif // DPS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
