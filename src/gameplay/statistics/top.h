/* ****************************************************************************
* File: top.h                                                  Part of Bylins *
* Usage: Топ игроков пошустрее                                                *
* (c) 2006 Krodo                                                              *
******************************************************************************/

#ifndef TOP_H_
#define TOP_H_

#include <string>
#include <list>
#include <vector>

#include "engine/core/conf.h"
#include "gameplay/classes/classes_constants.h"

// кол-во отображаемых в топе игроков по профессии
const int kPlayerChartSize = 10;

class TopPlayer;
using PlayerChart = std::unordered_map<ECharClass, std::list<TopPlayer>>;

namespace Rating {
    void DoBest(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
}

class TopPlayer {
 public:
	TopPlayer(long unique, const char *name, long exp, int remort)
		: unique_(unique), name_(name), exp_(exp), remort_(remort) {};

	~TopPlayer() = default;

	static const PlayerChart &Chart();
	static void Remove(CharData *ch);
	static void Refresh(CharData *ch, bool reboot = false);

 private:
	long unique_;			// уид
	std::string name_;
	long exp_;
	int remort_;

	static PlayerChart chart_;

	static void PrintHelp(CharData *ch);
	static void PrintPlayersChart(CharData *ch);
	static void PrintClassChart(CharData *ch, ECharClass id);

	friend void Rating::DoBest(CharData *ch, char *argument, int cmd, int subcmd);
};

#endif // TOP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
