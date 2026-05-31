#ifndef _TARGET_RESOLVER_HPP_INCLUDED_
#define _TARGET_RESOLVER_HPP_INCLUDED_

/*
	Класс, формирующий списки целей для массовых умений и заклинаний.
	Плюс несколько функций для проверки корректности целей.

	В перспективе сюда нужно перевести всю логику поиска и выбора целей для команд.
	При большом желании можно сделать класс универсальным, а не только для персонажей.
	Сейчас это не сделано, потому что при текущей логике исполнения команд это ничего особо не даст.
*/

#include "engine/structs/structs.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

class CharData;
class ObjData;
class RoomData;

namespace target_resolver {

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

// ---- Query-based resolver API (issue #3375 stage 2) -----------------------
//
// A unified descriptor for "find me targets that match X". The same struct
// covers char, obj and room lookups; the caller picks the kind via the
// matching ResolveChar/ResolveObj/ResolveRoom entry point and fills only the
// fields relevant to that kind (the others stay at their defaults).
//
// Design notes (see issue #3375 plan, stage 2):
//   * Scope is a list, walked in order. The first scope that returns at least
//     one match (when `single = true`) ends the search; otherwise all scopes
//     are visited and the union returned.
//   * `visible_only = true` gates char results through CAN_SEE and obj results
//     through CAN_SEE_OBJ. Set to false for scripted lookups that need to
//     locate hidden / wizinvis entities (the legacy SearchObjByRnum-style
//     paths).
//   * `name` is an optional substring (passed through isname semantics) --
//     when set, candidates must match before any predicate runs.
//   * `char_predicate` / `obj_predicate` are additional caller filters layered
//     on top. Compose them with the named filter factories shipped in
//     the stage-3 commit (MakeAlly / MakeEnemy / MakeFighting / etc.).
//
// The existing TargetsRosterType / FoesRosterType / FriendsRosterType classes
// above remain the right tool for "fan out across the room with a single
// filter" use cases (warcries, area damage). Query is for everything else --
// name lookups, single-target finds with custom filters, future <target>
// selectors on multi-action spells.

enum class Scope : std::uint8_t {
	kSelf,       // just the searcher
	kEquip,      // searcher's equipment slots
	kInventory,  // searcher's carrying list
	kRoom,       // people / objects in searcher's room
	kWorld,      // global character_list (chars only; obj-world lookup deferred)
};

struct Query {
	std::vector<Scope> scopes = {Scope::kRoom};
	std::optional<std::string> name;
	bool visible_only = true;
	bool single = true;

	std::function<bool(CharData *)> char_predicate;
	std::function<bool(ObjData *)>  obj_predicate;
};

// Single-result entry points. Return nullptr when no candidate matches.
CharData *ResolveChar(CharData *searcher, const Query &q);
ObjData  *ResolveObj (CharData *searcher, const Query &q);
RoomData *ResolveRoom(CharData *searcher, const Query &q);

// Multi-result entry points. `q.single` is ignored (always treated as false).
std::vector<CharData *> ResolveChars(CharData *searcher, const Query &q);
std::vector<ObjData *>  ResolveObjs (CharData *searcher, const Query &q);

}; // namespace target_resolver

#endif // _TARGET_RESOLVER_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
