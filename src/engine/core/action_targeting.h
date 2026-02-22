#ifndef _ACTION_TARGETING_HPP_INCLUDED_
#define _ACTION_TARGETING_HPP_INCLUDED_

/*
	Класс, формирующий списки целей для массовых умений и заклинаний.
	Плюс несколько функций для проверки корректности целей.

	В перспективе сюда нужно перевести всю логику поиска и выбора целей для команд.
	При большом желании можно сделать класс универсальным, а не только для персонажей.
	Сейчас это не сделано, потому что при текущей логике исполнения команд это ничего особо не даст.
*/

#include "engine/structs/structs.h"

#include <functional>

#include <random>

class CharData;

namespace ActionTargeting {

using FilterType = std::function<bool(CharData *, CharData *)>;
using PredicateType = std::function<bool(CharData *)>;
extern const FilterType emptyFilter;
extern const FilterType isCorrectFriend;
extern const FilterType isCorrectVictim;

bool isIncorrectVictim(CharData *actor, CharData *target, char *arg);

class TargetsRosterType {
 protected:
	CharData *_actor;
	std::vector<CharData *> _roster;
	FilterType _passesThroughFilter;

	void setFilter(const FilterType &baseFilter, const FilterType &extraFilter);
	void fillRoster();
	void shuffle();
	void setPriorityTarget(CharData *target);
	void makeRosterOfFoes(CharData *priorityTarget, const FilterType &baseFilter, const FilterType &extraFilter);
	void makeRosterOfFriends(CharData *priorityTarget, const FilterType &baseFilter, const FilterType &extraFilter);

	TargetsRosterType();
	TargetsRosterType(CharData *actor) :
		_actor{actor} {};
 public:
	auto begin() const noexcept { return std::make_reverse_iterator(std::end(_roster)); };
	auto end() const noexcept { return std::make_reverse_iterator(std::begin(_roster)); };
	CharData *getRandomItem(const PredicateType &predicate);
	CharData *getRandomItem();
	int amount() { return static_cast<int>(_roster.size()); };
	int count(const PredicateType &predicate);
	void flip();
};

//  -------------------------------------------------------

class FoesRosterType : public TargetsRosterType {
 protected:
	FoesRosterType();

 public:
	FoesRosterType(CharData *actor, CharData *priorityTarget, const FilterType &extraFilter) :
		TargetsRosterType(actor) {
		makeRosterOfFoes(priorityTarget, isCorrectVictim, extraFilter);
	};
	FoesRosterType(CharData *actor, const FilterType &extraFilter) :
		FoesRosterType(actor, nullptr, extraFilter) {};
	FoesRosterType(CharData *actor, CharData *priorityTarget) :
		FoesRosterType(actor, priorityTarget, emptyFilter) {};
	FoesRosterType(CharData *actor) :
		FoesRosterType(actor, nullptr, emptyFilter) {};
};

//  -------------------------------------------------------

class FriendsRosterType : public TargetsRosterType {
 protected:
	FriendsRosterType();

 public:
	FriendsRosterType(CharData *actor, CharData *priorityTarget, const FilterType &extraFilter) :
		TargetsRosterType(actor) {
		makeRosterOfFriends(priorityTarget, isCorrectFriend, extraFilter);
	};
	FriendsRosterType(CharData *actor, const FilterType &extraFilter) :
		FriendsRosterType(actor, nullptr, extraFilter) {};
	FriendsRosterType(CharData *actor, CharData *priorityTarget) :
		FriendsRosterType(actor, priorityTarget, emptyFilter) {};
	FriendsRosterType(CharData *actor) :
		FriendsRosterType(actor, nullptr, emptyFilter) {};
};

}; // namespace ActionTargeting

#endif // _ACTION_TARGETING_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
