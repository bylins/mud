/*
	Модуль составление списков целей для умений/зклинаний.
*/

#include "target_resolver.h"

#include "engine/core/handler.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/db/world_characters.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "gameplay/fight/pk.h"
#include "gameplay/mechanics/groups.h"
#include "utils/utils.h"

/*
	2. Добавить возможность сформировать список группы без учета комнаты.
*/

namespace target_resolver {

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
	if (group::same_group(actor, target) || target->IsImmortal()) {
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

// ---- Query-based resolver implementation (issue #3375 stage 2) ------------

namespace {

bool MatchesName(CharData *cand, const std::string &name) {
	return isname(name.c_str(), cand->GetCharAliases());
}

bool MatchesName(ObjData *cand, const std::string &name) {
	return isname(name.c_str(), cand->get_aliases().c_str());
}

// Stage walking: for each scope in `q.scopes`, collect candidates and run the
// visibility / name / predicate gates; stop once `single` is satisfied.
void CollectChars(CharData *searcher, const Query &q,
				  std::vector<CharData *> &out) {
	auto consider = [&](CharData *cand) {
		if (!cand || cand->purged()) return;
		if (q.visible_only && !CAN_SEE(searcher, cand)) return;
		if (q.name && !MatchesName(cand, *q.name)) return;
		if (q.char_predicate && !q.char_predicate(cand)) return;
		out.push_back(cand);
	};

	for (const Scope scope : q.scopes) {
		if (q.single && !out.empty()) return;
		switch (scope) {
			case Scope::kSelf:
				consider(searcher);
				break;
			case Scope::kRoom:
				if (searcher->in_room != kNowhere) {
					for (CharData *c : world[searcher->in_room]->people) {
						consider(c);
						if (q.single && !out.empty()) return;
					}
				}
				break;
			case Scope::kWorld:
				for (const auto &shared : character_list) {
					consider(shared.get());
					if (q.single && !out.empty()) return;
				}
				break;
			case Scope::kEquip:
			case Scope::kInventory:
				// Equip / inventory scopes carry no chars -- skip silently
				// (a future "find a charmed mob in the saddle" path can plug
				// container-walking in here).
				break;
		}
	}
}

void CollectObjs(CharData *searcher, const Query &q,
				 std::vector<ObjData *> &out) {
	auto consider = [&](ObjData *cand) {
		if (!cand) return;
		if (q.visible_only && !CAN_SEE_OBJ(searcher, cand)) return;
		if (q.name && !MatchesName(cand, *q.name)) return;
		if (q.obj_predicate && !q.obj_predicate(cand)) return;
		out.push_back(cand);
	};

	for (const Scope scope : q.scopes) {
		if (q.single && !out.empty()) return;
		switch (scope) {
			case Scope::kSelf:
			case Scope::kWorld:
				// Self / world obj lookups are out of scope for stage 2 --
				// add when a consumer needs them (a global obj table walk
				// is expensive; FindObjForLocate-style paths stay where
				// they are for now).
				break;
			case Scope::kEquip:
				for (int slot = 0; slot < EEquipPos::kNumEquipPos; ++slot) {
					consider(GET_EQ(searcher, slot));
					if (q.single && !out.empty()) return;
				}
				break;
			case Scope::kInventory:
				for (ObjData *o = searcher->carrying; o; o = o->get_next_content()) {
					consider(o);
					if (q.single && !out.empty()) return;
				}
				break;
			case Scope::kRoom:
				if (searcher->in_room != kNowhere) {
					for (ObjData *o : world[searcher->in_room]->contents) {
						consider(o);
						if (q.single && !out.empty()) return;
					}
				}
				break;
		}
	}
}

}  // namespace

CharData *ResolveChar(CharData *searcher, const Query &q) {
	std::vector<CharData *> matches;
	Query qs = q;
	qs.single = true;
	CollectChars(searcher, qs, matches);
	return matches.empty() ? nullptr : matches.front();
}

ObjData *ResolveObj(CharData *searcher, const Query &q) {
	std::vector<ObjData *> matches;
	Query qs = q;
	qs.single = true;
	CollectObjs(searcher, qs, matches);
	return matches.empty() ? nullptr : matches.front();
}

RoomData *ResolveRoom(CharData *searcher, const Query &q) {
	// Stage 2 placeholder: rooms have no scope/visibility model that maps
	// onto the char/obj walks, and there's no consumer asking for it yet.
	// Returning nullptr keeps the API symmetric for the day a consumer
	// shows up; the legacy FindRoomRnum is the right tool today.
	(void)searcher;
	(void)q;
	return nullptr;
}

std::vector<CharData *> ResolveChars(CharData *searcher, const Query &q) {
	std::vector<CharData *> matches;
	Query qm = q;
	qm.single = false;
	CollectChars(searcher, qm, matches);
	return matches;
}

std::vector<ObjData *> ResolveObjs(CharData *searcher, const Query &q) {
	std::vector<ObjData *> matches;
	Query qm = q;
	qm.single = false;
	CollectObjs(searcher, qm, matches);
	return matches;
}

// ---- Named filter factories (issue #3375 stage 3) -------------------------

namespace {

// Walk up the charm-chain so a charmed NPC resolves to its PC master for
// alliance purposes. Mirrors AllyRoot() in spells_info.cpp -- bounded at depth
// 4 so a pathological charm cycle can't hang the loop. (See
// bylins-issue-ambiguous-spells.)
const CharData *AllyRoot(const CharData *ch) {
	if (!ch) return nullptr;
	for (int depth = 0; depth < 4 && IS_CHARMICE(ch) && ch->has_master(); ++depth) {
		const auto *next = ch->get_master();
		if (!next || next == ch) break;
		ch = next;
	}
	return ch;
}

bool IsAllyOf(const CharData *root, const CharData *cand) {
	if (!root || !cand) return false;
	if (root == cand) return true;
	const auto *r = AllyRoot(root);
	const auto *c = AllyRoot(cand);
	if (!r || !c) return false;
	if (r == c) return true;
	// Two NPCs with no PC root: conditionally allied (mob-vs-mob rule).
	if (r->IsNpc() && c->IsNpc()) return true;
	return group::same_group(const_cast<CharData *>(r), const_cast<CharData *>(c));
}

}  // namespace

CharPredicate MakeAllyFilter(CharData *root) {
	return [root](CharData *cand) { return IsAllyOf(root, cand); };
}

CharPredicate MakeEnemyFilter(CharData *root) {
	return [root](CharData *cand) { return !IsAllyOf(root, cand); };
}

CharPredicate MakeNpcFilter() {
	return [](CharData *cand) { return cand && cand->IsNpc(); };
}

CharPredicate MakePcFilter() {
	return [](CharData *cand) { return cand && !cand->IsNpc(); };
}

CharPredicate MakeFightingFilter() {
	return [](CharData *cand) { return cand && cand->GetEnemy() != nullptr; };
}

CharPredicate MakeSameRoomFilter(CharData *root) {
	return [root](CharData *cand) {
		return root && cand && root->in_room == cand->in_room
			&& root->in_room != kNowhere;
	};
}

CharPredicate MakeVisibleFilter(CharData *viewer) {
	return [viewer](CharData *cand) {
		return viewer && cand && CAN_SEE(viewer, cand);
	};
}

CharPredicate MakeAffectFilter(EAffect flag) {
	return [flag](CharData *cand) { return cand && AFF_FLAGGED(cand, flag); };
}

CharPredicate MakeMobFlagFilter(EMobFlag flag) {
	return [flag](CharData *cand) {
		return cand && cand->IsNpc() && cand->IsFlagged(flag);
	};
}

CharPredicate MakeMobVnumFilter(MobVnum vnum) {
	return [vnum](CharData *cand) {
		return cand && cand->IsNpc() && GET_MOB_VNUM(cand) == vnum;
	};
}

ObjPredicate MakeObjVnumFilter(ObjVnum vnum) {
	return [vnum](ObjData *cand) {
		return cand && cand->get_vnum() == vnum;
	};
}

ObjPredicate MakeObjVisibleFilter(CharData *viewer) {
	return [viewer](ObjData *cand) {
		return viewer && cand && CAN_SEE_OBJ(viewer, cand);
	};
}

}; // namespace target_resolver

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
