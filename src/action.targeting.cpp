
/*
	Модуль составление списков целей для умений/зклинаний.
*/

#include "action.targeting.hpp"
#include "pk.h"

#include <algorithm>

namespace ActionTargeting {

	/*
		isCorrectSingleFriend нет, потому что не имеется дружественных ареакастов
		Если таковые вводить - стоит добавить
	*/
	bool isNotCorrectSingleVictim(CHAR_DATA *actor, CHAR_DATA *target, char* arg) {
		if (actor = target) {
			return true;
		}
		if (!check_pkill(actor, target, arg)) {
			return true;
		}
		if (!may_kill_here(actor, target)) {
			return true;
		}
		return false;
	};

// ===== list of filters ===
	FilterType emptyFilter;

	FilterType isCorrectFriend = [](CHAR_DATA *actor, CHAR_DATA *target) {
		if (isNotCorrectTarget(actor, target)) {
			return false;
		};
		if (!same_group(actor, target)) {
			return false;
		};
		return true;
	};

	FilterType isCorrectVictim = [](CHAR_DATA *actor, CHAR_DATA *target) {
		if (isNotCorrectTarget(actor, target)) {
			return false;
		};
		if (same_group(actor, target) || IS_IMMORTAL(target)) {
			return false;
		};
		if (!may_kill_here(actor, target)) {
			return false;
		};
		return true;
	};

	FilterType isNotCorrectTarget = [](CHAR_DATA* /* actor */, CHAR_DATA* target) {
			return (!HERE(target) || (IN_ROOM(target) == NOWHERE));
	};
// ===== end list of filters ===

	void TargetsRosterType::setFilter(FilterType &baseFilter, FilterType &extraFilter) {
		if (extraFilter) {
			_passesThroughFilter =
				[&baseFilter, &extraFilter](CHAR_DATA* actor, CHAR_DATA* target) {
					return (baseFilter(actor, target) && extraFilter(actor, target));
					};
		} else {
			_passesThroughFilter = baseFilter;
		};
	};

	void TargetsRosterType::fillRoster() {
        this->reserve(world[_actor->in_room]->people.size()+1);
        std::copy_if(world[_actor->in_room]->people.begin(), world[_actor->in_room]->people.end(),
						 std::back_inserter(*this), [this](CHAR_DATA* ch) {return _passesThroughFilter(_actor, ch);});
	};

	void TargetsRosterType::setPriorityTarget(CHAR_DATA* target) {
		auto it = std::find(this->begin(), this->end(), target);
        if (it != this->end()) {
            this->erase(it);
        }
		this->push_back(target);
	};

	void TargetsRosterType::makeRosterOfFoes(CHAR_DATA* priorityTarget, FilterType &baseFilter, FilterType &extraFilter) {
		setFilter(baseFilter, extraFilter);
		fillRoster();
		shuffle();
		prepareRosterForUse(priorityTarget);
	};

	void TargetsRosterType::makeRosterOfFriends(CHAR_DATA* priorityTarget, FilterType &baseFilter, FilterType &extraFilter) {
		setFilter(baseFilter, extraFilter);
		fillRoster();
		prepareRosterForUse(priorityTarget);
	};

	void TargetsRosterType::prepareRosterForUse(CHAR_DATA* priorityTarget) {
		if (priorityTarget) {
			setPriorityTarget(priorityTarget);
		};
		_currentTarget = this->rbegin();
	};

	CHAR_DATA* TargetsRosterType::shift() {
		auto item = std::find_if(_currentTarget, this->rend(), [](CHAR_DATA* ch) {return (!ch->purged());});
		if (item == this->rend()) {
			return nullptr;
		};
		_currentTarget = std::next(item, 1);
		return *item;
	};

	void TargetsRosterType::shuffle() {
		std::random_shuffle(this->begin(), this->end());
	};

	/*
		В принципе тут можно добавить еще функцию сортировки списка через
		std::partial_sort или просто std::sort
		Тогда будет легко сделать, к примеру, лечащий скилл, который в первую очередь
		лечит наиболее тяжело раненых членов группы.
		По покамест таких абилок нет и смысла в них тоже, потому воздержался.
	*/

}; // namespace ActionTargeting

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
