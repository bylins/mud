/*
	Модуль составление списков целей для умений/зклинаний.
*/

#include "action_targeting.h"
#include "gameplay/fight/pk.h"
#include "gameplay/mechanics/groups.h"

/*
	2. Добавить возможность сформировать список группы без учета комнаты.
*/

namespace ActionTargeting {

/*
	isCorrectSingleFriend нет, потому что не имеется дружественных ареакастов
	Если таковые вводить - стоит добавить
*/
bool isIncorrectVictim(CharData *actor, CharData *target, char *arg) {
	if (actor == target) {
		return true;
	}
	if (!check_pkill(actor, target, arg)) {
		return true;
	}
	if (!may_kill_here(actor, target, NoArgument)) {
		return true;
	}
	return false;
};

// ===== list of filters ===
const FilterType emptyFilter;

const FilterType isNotCorrectTarget = [](CharData *actor, CharData *target) {
	return (!HERE(target) || target->in_room == kNowhere || !actor->isInSameRoom(target));
};

const FilterType isCorrectFriend = [](CharData *actor, CharData *target) {
	if (isNotCorrectTarget(actor, target)) {
		return false;
	};
	return (group::same_group(actor, target));
};

const FilterType isCorrectVictim = [](CharData *actor, CharData *target) {
	if (isNotCorrectTarget(actor, target)) {
		return false;
	};
	if (group::same_group(actor, target) || IS_IMMORTAL(target)) {
		return false;
	};
	if (!may_kill_here(actor, target, NoArgument)) {
		return false;
	};
	return true;
};

// ===== end list of filters ===

void TargetsRosterType::setFilter(const FilterType &baseFilter, const FilterType &extraFilter) {
	if (extraFilter) {
		_passesThroughFilter =
			[&baseFilter, &extraFilter](CharData *actor, CharData *target) {
				return (baseFilter(actor, target) && extraFilter(actor, target));
			};
	} else {
		_passesThroughFilter = baseFilter;
	};
};

void TargetsRosterType::fillRoster() {
	_roster.reserve(world[_actor->in_room]->people.size() + 1);
	std::copy_if(world[_actor->in_room]->people.begin(), world[_actor->in_room]->people.end(),
				 std::back_inserter(_roster), [this](CharData *ch) { return _passesThroughFilter(_actor, ch); });
};

void TargetsRosterType::setPriorityTarget(CharData *target) {
	if (!target) {
		return;
	}
	auto it = std::find(_roster.begin(), _roster.end(), target);
	if (it != _roster.end()) {
		_roster.erase(it);
	}
	_roster.push_back(target);
};

void TargetsRosterType::makeRosterOfFoes(CharData *priorityTarget,
										 const FilterType &baseFilter,
										 const FilterType &extraFilter) {
	setFilter(baseFilter, extraFilter);
	fillRoster();
	shuffle();
	setPriorityTarget(priorityTarget);
};

/*
	Svent TODO:
	Тут еще можно сделать проверку, составляем мы список для моба или для игрока.
	Внутри функции или вызовом отдельной функции - как эстетичней и правильней, надо думать.
	Для моба, естественно, по умолчанию все мобы - дружественные, если они не чармисы
	и ие слишком отличаются по наклонностям (по крайней мере до реализации фракций).
	В очень многих местах по коду (особенно в fight) идут проверки типа "если моб не непись, не ангел"
	и так далее, чтобы выбрать _дружественную_ цель для моба. Логично вынести это в одно место.
*/
void TargetsRosterType::makeRosterOfFriends(CharData *priorityTarget,
											const FilterType &baseFilter,
											const FilterType &extraFilter) {
	setFilter(baseFilter, extraFilter);
	fillRoster();
	setPriorityTarget(priorityTarget);
};

void TargetsRosterType::shuffle() {
	std::shuffle(_roster.begin(), _roster.end(), std::mt19937(std::random_device()()));
};

void TargetsRosterType::flip() {
	std::reverse(_roster.begin(), _roster.end());
};

int TargetsRosterType::count(const PredicateType &predicate) {
	return std::count_if(_roster.begin(), _roster.end(), predicate);
};

CharData *TargetsRosterType::getRandomItem(const PredicateType &predicate) {
	int amountOfItems = std::count_if(_roster.begin(), _roster.end(), predicate);
	if (amountOfItems == 0) {
		return nullptr;
	}
	amountOfItems = number(1, amountOfItems);
	auto item = _roster.begin();
	for (auto start = item; amountOfItems > 0; --amountOfItems, start = std::next(item, 1)) {
		item = std::find_if(start, _roster.end(), predicate);
	}
	return (*item);
};

CharData *TargetsRosterType::getRandomItem() {
	if (_roster.empty()) {
		return nullptr;
	}
	return _roster[number(0, static_cast<int>(_roster.size()) - 1)];
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
