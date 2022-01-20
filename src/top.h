/* ****************************************************************************
* File: top.h                                                  Part of Bylins *
* Usage: Топ игроков пошустрее                                                *
* (c) 2006 Krodo                                                              *
******************************************************************************/

#ifndef _TOP_H_
#define _TOP_H_

#include "conf.h"
#include <string>
#include <list>
#include <vector>

// кол-во отображаемых в топе игроков по профессии
#define MAX_TOP_CLASS 10

class TopPlayer;
typedef std::vector<std::list<TopPlayer>> TopListType;

class TopPlayer {
 public:
	TopPlayer(long _unique, const char *_name, long _exp, int _remort)
		: unique(_unique), name(_name), exp(_exp), remort(_remort) {};

	~TopPlayer() = default;

	static const char *TopFormat[];

	static void Remove(CharacterData *ch);
	static void Refresh(CharacterData *ch, bool reboot = false);

 private:
	long unique;      // уид
	std::string name; // имя
	long exp;         // опыта
	int remort;       // ремортов

	static TopListType TopList; // собсна топ

	friend void DoBest(CharacterData *ch, char *argument, int cmd, int subcmd);
};

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
