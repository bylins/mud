#ifndef __DAILY_QUEST_HPP__
#define __DAILY_QUEST_HPP__

#include <string>
#include <unordered_map>

struct DailyQuest
{
	// id
	int id;
	// desk
	std::string desk;
	// награда
	int reward;
};

using DailyQuestMap = std::unordered_map<int, DailyQuest>;
extern DailyQuestMap& d_quest;

#endif // __DAILY_QUEST_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
