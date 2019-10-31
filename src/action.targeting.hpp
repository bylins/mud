#ifndef _ACTION_TARGETING_HPP_INCLUDED_
#define _ACTION_TARGETING_HPP_INCLUDED_

/*
	Класс, формирующий списки целей для массовых умений и заклинаний.
	Плюс несколько функций для проверки корректности целей.

	В перспективе сюда нужно перевести всю логику поиска и выбора целей для команд.
	Причем при большом желании можно сделать класс универсальным, а не только для персонажей.
	Сейчас это не сделано, потому что при текущей логике исполнения команд это ничего не даст.
*/

#include "char.hpp"
#include "structs.h"

#include <functional>

namespace ActionTargeting {

	using FilterType = std::function<bool (CHAR_DATA*, CHAR_DATA*)>;
	extern FilterType emptyFilter;
	extern FilterType isCorrectFriend;
	extern FilterType isCorrectVictim;
	extern FilterType isNotCorrectTarget;

	bool isNotCorrectSingleVictim(CHAR_DATA* actor, CHAR_DATA* target, char* arg);

	class TargetsRosterType : public std::vector<CHAR_DATA*> {
	protected:
		CHAR_DATA* _actor;
		FilterType _passesThroughFilter;
		std::reverse_iterator<std::vector<CHAR_DATA*>::iterator> _currentTarget;

		void setFilter(FilterType &baseFilter, FilterType &extraFilter);
		void fillRoster();
		void shuffle();
		void setPriorityTarget(CHAR_DATA* target);
		void makeRosterOfFoes(CHAR_DATA* priorityTarget, FilterType &baseFilter, FilterType &extraFilter);
		void makeRosterOfFriends(CHAR_DATA* priorityTarget, FilterType &baseFilter, FilterType &extraFilter);
		void prepareRosterForUse(CHAR_DATA* priorityTarget);

		TargetsRosterType();
		TargetsRosterType(CHAR_DATA* actor) :
			_actor{actor}
			{};
	public:
		CHAR_DATA* shift();
	};

//  -------------------------------------------------------

	class FoesRosterType : public TargetsRosterType {
	protected:
		FoesRosterType();

	public:
		FoesRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget, FilterType &extraFilter) :
			TargetsRosterType(actor) {
				makeRosterOfFoes(priorityTarget, isCorrectVictim, extraFilter);
			};
		FoesRosterType(CHAR_DATA* actor, FilterType &extraFilter) :
			FoesRosterType(actor, nullptr, extraFilter)
			{};
		FoesRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget) :
			FoesRosterType(actor, priorityTarget, emptyFilter)
			{};
		FoesRosterType(CHAR_DATA* actor) :
			FoesRosterType(actor, nullptr, emptyFilter)
			{};
	};

//  -------------------------------------------------------

	class FriendsRosterType : public TargetsRosterType {
	protected:
		FriendsRosterType();

	public:
		FriendsRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget = nullptr, FilterType &extraFilter = emptyFilter) :
			TargetsRosterType(actor) {
				makeRosterOfFriends(priorityTarget, isCorrectFriend, extraFilter);
			};/*
		FriendsRosterType(CHAR_DATA* actor, FilterType &extraFilter) :
			FriendsRosterType(actor, nullptr, extraFilter)
			{};
		FriendsRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget) :
			FriendsRosterType(actor, priorityTarget, emptyFilter)
			{};
		FriendsRosterType(CHAR_DATA* actor) :
			FriendsRosterType(actor, nullptr, emptyFilter)
			{};*/
	};

}; // namespace ActionTargeting

#endif // _ACTION_TARGETING_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
