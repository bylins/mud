#ifndef _ACTION_TARGETING_HPP_INCLUDED_
#define _ACTION_TARGETING_HPP_INCLUDED_

/*
	Класс, формирующий списки целей для массовых умений и заклинаний.
	Плюс несколько функций для проверки корректности целей.

	В перспективе сюда нужно перевести всю логику поиска и выбора целей для команд.
	При большом желании можно сделать класс универсальным, а не только для персонажей.
	Сейчас это не сделано, потому что при текущей логике исполнения команд это ничего особо не даст.
*/

#include "char.hpp"
#include "structs.h"

#include <functional>

namespace ActionTargeting {

	using FilterType = std::function<bool (CHAR_DATA*, CHAR_DATA*)>;
	using PredicateType = std::function<bool (CHAR_DATA*)>;
	extern const FilterType emptyFilter;
	extern const FilterType isCorrectFriend;
	extern const FilterType isCorrectVictim;

	bool isNotCorrectSingleVictim(CHAR_DATA* actor, CHAR_DATA* target, char* arg);

	class TargetsRosterType {
	protected:
		CHAR_DATA* _actor;
		std::vector<CHAR_DATA*> _roster;
		FilterType _passesThroughFilter;

		void setFilter(const FilterType& baseFilter, const FilterType& extraFilter);
		void fillRoster();
		void shuffle();
		void setPriorityTarget(CHAR_DATA* target);
		void makeRosterOfFoes(CHAR_DATA* priorityTarget, const FilterType& baseFilter, const FilterType& extraFilter);
		void makeRosterOfFriends(CHAR_DATA* priorityTarget, const FilterType& baseFilter, const FilterType& extraFilter);

		TargetsRosterType();
		TargetsRosterType(CHAR_DATA* actor) :
			_actor{actor}
			{};
	public:
		auto begin() const noexcept {return std::make_reverse_iterator(std::end(_roster));};
		auto end() const noexcept {return std::make_reverse_iterator(std::begin(_roster));};
		CHAR_DATA* getRandomItem(const PredicateType& predicate);
		CHAR_DATA* getRandomItem();
		int amount() {return _roster.size();};
		int count(const PredicateType& predicate);
	};

//  -------------------------------------------------------

	class FoesRosterType : public TargetsRosterType {
	protected:
		FoesRosterType();

	public:
		FoesRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget, const FilterType& extraFilter) :
			TargetsRosterType(actor) {
				makeRosterOfFoes(priorityTarget, isCorrectVictim, extraFilter);
			};
		FoesRosterType(CHAR_DATA* actor, const FilterType& extraFilter) :
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
		FriendsRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget, const FilterType& extraFilter) :
			TargetsRosterType(actor) {
				makeRosterOfFriends(priorityTarget, isCorrectFriend, extraFilter);
			};
		FriendsRosterType(CHAR_DATA* actor, const FilterType& extraFilter) :
			FriendsRosterType(actor, nullptr, extraFilter)
			{};
		FriendsRosterType(CHAR_DATA* actor, CHAR_DATA* priorityTarget) :
			FriendsRosterType(actor, priorityTarget, emptyFilter)
			{};
		FriendsRosterType(CHAR_DATA* actor) :
			FriendsRosterType(actor, nullptr, emptyFilter)
			{};
	};

}; // namespace ActionTargeting

#endif // _ACTION_TARGETING_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
