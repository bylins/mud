/*
	Модуль составление списков целей для умений/зклинаний.
*/

#include "target_resolver.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/minions.h"

#include "engine/core/handler.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/core/comm.h"
#include "engine/network/descriptor_data.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "gameplay/fight/pk.h"
#include "engine/db/db.h"            // GetZoneRooms, world
#include "gameplay/clans/house.h"    // Clan::MayEnter, kHousePortal
#include "gameplay/mechanics/groups.h"
#include "utils/utils.h"

#include <cctype>
#include "gameplay/mechanics/sight.h"

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
	if (group::same_group(actor, target) || privilege::IsImmortal(target)) {
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

// Self-reference aliases. Legacy get_char_room_vis resolved these to the
// searcher up front, so every name-based char lookup (look/examine/diagnose/
// ... via ResolveChar) could target self. Bytes are KOI8-R ("я"/"меня"/"себя").
bool IsSelfAlias(const std::optional<std::string> &name) {
	if (!name) return false;
	const std::string &n = *name;
	return n == "self" || n == "me" || n == "я" || n == "меня" || n == "себя";
}

// Self resolves only for lookups that include the searcher's vicinity
// (room/self scope) -- get_char_vis always checked the room (and self) first.
bool ScopeAllowsSelf(const std::vector<Scope> &scopes) {
	for (const Scope s : scopes) {
		if (s == Scope::kRoom || s == Scope::kSelf) return true;
	}
	return false;
}

// Parse the legacy "N.name" ordinal prefix. "2.fido" -> {ordinal=2, raw="fido"};
// "fido" -> {1, "fido"}; "0.fido" -> {0, "fido"} (legacy "no match" sentinel);
// non-numeric prefix -> {1, "input as-is"}.
struct Ordinal {
	int n = 1;
	std::string raw;
};
Ordinal ParseOrdinal(const std::string &input) {
	Ordinal o{1, input};
	auto dot = input.find('.');
	if (dot == std::string::npos) return o;
	const std::string head = input.substr(0, dot);
	if (head.empty()) return o;
	for (char c : head) {
		if (!std::isdigit(static_cast<unsigned char>(c))) return o;
	}
	try {
		o.n = std::stoi(head);
		o.raw = input.substr(dot + 1);
	} catch (...) {
		// stoi can throw on overflow -- keep the {1, input} default.
	}
	return o;
}

// Tracks how many name-matched candidates have been seen so far, lets the
// ordinal-aware consider() pick the Nth one out of the stream without
// allocating a full match list.
struct OrdinalState {
	int target;     // the N from "N.name" (or 1 by default)
	int seen = 0;   // matches consumed so far
	bool done = false;
};

// Stage walking: for each scope in `q.scopes`, collect candidates and run the
// visibility / name / predicate gates; stop once `single` is satisfied. The
// helper takes a const std::string& name_filter so the ordinal-stripped form
// can be passed in without rebuilding the optional<std::string> per scope.
void CollectChars(CharData *searcher, const Query &q,
				  const std::optional<std::string> &name_filter,
				  OrdinalState &ord,
				  std::vector<CharData *> &out) {
	auto consider = [&](CharData *cand) {
		if (ord.done) return;
		if (!cand || cand->purged()) return;
		if (q.visible_only && !sight::CanSee(searcher, cand)) return;
		if (name_filter && !MatchesName(cand, *name_filter)) return;
		if (q.char_predicate && !q.char_predicate(cand)) return;
		// Ordinal: skip the first ord.target-1 matches when `single` is set;
		// for multi mode, take everything.
		if (q.single) {
			if (++ord.seen != ord.target) return;
			ord.done = true;
		}
		out.push_back(cand);
	};

	// Bail if "0.name" was specified (legacy no-match sentinel).
	if (q.single && ord.target == 0) return;

	for (const Scope scope : q.scopes) {
		if (ord.done) return;
		switch (scope) {
			case Scope::kSelf:
				consider(searcher);
				break;
			case Scope::kRoom: {
				const RoomRnum room = q.room_override.value_or(searcher ? searcher->in_room : kNowhere);
				if (room != kNowhere) {
					for (CharData *c : world[room]->people) {
						consider(c);
						if (ord.done) return;
					}
				}
				break;
			}
			case Scope::kWorld:
				// PC-priority: walk PCs first, then NPCs. Matches the legacy
				// get_char_vis ordering where get_player_vis fired before the
				// character_list sweep.
				for (const auto &shared : character_list) {
					CharData *c = shared.get();
					if (c && !c->IsNpc()) consider(c);
					if (ord.done) return;
				}
				for (const auto &shared : character_list) {
					CharData *c = shared.get();
					if (c && c->IsNpc()) consider(c);
					if (ord.done) return;
				}
				break;
			case Scope::kOnlinePcs:
				// Walk live descriptors; HERE() filters out menu/login states
				// where the character pointer is set but not in-world.
				for (DescriptorData *d = descriptor_list; d; d = d->next) {
					if (d->state != EConState::kPlaying) continue;
					CharData *c = d->character.get();
					if (!c || !HERE(c)) continue;
					consider(c);
					if (ord.done) return;
				}
				break;
			case Scope::kRnum:
				// Char-by-rnum lookup: defer until a consumer asks for it.
				// (Today's needs are obj-only; kRnum on chars is a no-op.)
				break;
			case Scope::kEquip:
			case Scope::kInventory:
				// No chars in equip / inventory in today's model.
				break;
		}
	}
}

// Forward declaration for the recursive container walk.
void ConsiderObjAndChildren(ObjData *cand, const Query &q,
							 const std::optional<std::string> &name_filter,
							 OrdinalState &ord,
							 std::vector<ObjData *> &out, CharData *searcher);

void ConsiderObjOne(ObjData *cand, const Query &q,
					 const std::optional<std::string> &name_filter,
					 OrdinalState &ord,
					 std::vector<ObjData *> &out, CharData *searcher) {
	if (ord.done) return;
	if (!cand) return;
	if (q.visible_only && searcher && !sight::CanSeeObj(searcher, cand)) return;
	if (name_filter && !MatchesName(cand, *name_filter)) return;
	if (q.obj_predicate && !q.obj_predicate(cand)) return;
	if (q.single) {
		if (++ord.seen != ord.target) return;
		ord.done = true;
	}
	out.push_back(cand);
}

void ConsiderObjAndChildren(ObjData *cand, const Query &q,
							 const std::optional<std::string> &name_filter,
							 OrdinalState &ord,
							 std::vector<ObjData *> &out, CharData *searcher) {
	ConsiderObjOne(cand, q, name_filter, ord, out, searcher);
	if (ord.done) return;
	if (!q.walk_containers || !cand) return;
	for (ObjData *child = cand->get_contains(); child; child = child->get_next_content()) {
		ConsiderObjAndChildren(child, q, name_filter, ord, out, searcher);
		if (ord.done) return;
	}
}

void CollectObjs(CharData *searcher, const Query &q,
				 const std::optional<std::string> &name_filter,
				 OrdinalState &ord,
				 std::vector<ObjData *> &out) {
	if (q.single && ord.target == 0) return;

	for (const Scope scope : q.scopes) {
		if (ord.done) return;
		switch (scope) {
			case Scope::kSelf:
				// Objs have no "self" -- skip.
				break;
			case Scope::kEquip:
				if (searcher) {
					for (int slot = 0; slot < EEquipPos::kNumEquipPos; ++slot) {
						ConsiderObjAndChildren(GET_EQ(searcher, slot), q, name_filter, ord, out, searcher);
						if (ord.done) return;
					}
				}
				break;
			case Scope::kInventory:
				if (searcher) {
					for (ObjData *o = searcher->carrying; o; o = o->get_next_content()) {
						ConsiderObjAndChildren(o, q, name_filter, ord, out, searcher);
						if (ord.done) return;
					}
				}
				break;
			case Scope::kRoom: {
				const RoomRnum room = q.room_override.value_or(searcher ? searcher->in_room : kNowhere);
				if (room != kNowhere) {
					for (ObjData *o : world[room]->contents) {
						ConsiderObjAndChildren(o, q, name_filter, ord, out, searcher);
						if (ord.done) return;
					}
				}
				break;
			}
			case Scope::kWorld:
				for (const auto &shared : world_objects) {
					if (ord.done) break;
					ConsiderObjAndChildren(shared.get(), q, name_filter, ord, out, searcher);
				}
				break;
			case Scope::kRnum:
				if (q.rnum_lookup) {
					const auto result = world_objects.find_first_by_rnum(*q.rnum_lookup);
					ConsiderObjOne(result.get(), q, name_filter, ord, out, searcher);
				}
				break;
			case Scope::kOnlinePcs:
				// Online-PC scope is char-only; no objs to collect.
				break;
		}
	}
}

}  // namespace

// Shared helper: ordinal-parses the q.name once, returns the stripped name
// (or std::nullopt if no name filter was set) and the requested ordinal.
namespace {
std::pair<std::optional<std::string>, int> NameAndOrdinal(const Query &q) {
	if (!q.name) return {std::nullopt, 1};
	Ordinal o = ParseOrdinal(*q.name);
	return {o.raw, o.n};
}
}  // namespace

CharData *ResolveChar(CharData *searcher, const Query &q) {
	if (searcher && !q.char_predicate && ScopeAllowsSelf(q.scopes) && IsSelfAlias(q.name)) {
		return searcher;
	}
	std::vector<CharData *> matches;
	Query qs = q;
	qs.single = true;
	auto [name, ordinal] = NameAndOrdinal(qs);
	OrdinalState ord{ordinal};
	CollectChars(searcher, qs, name, ord, matches);
	return matches.empty() ? nullptr : matches.front();
}

ObjData *ResolveObj(CharData *searcher, const Query &q) {
	std::vector<ObjData *> matches;
	Query qs = q;
	qs.single = true;
	auto [name, ordinal] = NameAndOrdinal(qs);
	OrdinalState ord{ordinal};
	CollectObjs(searcher, qs, name, ord, matches);
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
	if (searcher && !q.char_predicate && ScopeAllowsSelf(q.scopes) && IsSelfAlias(q.name)) {
		return {searcher};
	}
	std::vector<CharData *> matches;
	Query qm = q;
	qm.single = false;
	auto [name, ordinal] = NameAndOrdinal(qm);
	OrdinalState ord{ordinal};
	CollectChars(searcher, qm, name, ord, matches);
	return matches;
}

std::vector<ObjData *> ResolveObjs(CharData *searcher, const Query &q) {
	std::vector<ObjData *> matches;
	Query qm = q;
	qm.single = false;
	auto [name, ordinal] = NameAndOrdinal(qm);
	OrdinalState ord{ordinal};
	CollectObjs(searcher, qm, name, ord, matches);
	return matches;
}

// ---- Convenience helpers --------------------------------------------------

CharData *FindCharInRoom(CharData *finder, std::string_view name) {
	Query q;
	q.scopes = {Scope::kRoom};
	q.name = std::string(name);
	return ResolveChar(finder, q);
}

CharData *FindCharInWorld(CharData *finder, std::string_view name) {
	Query q;
	q.scopes = {Scope::kRoom, Scope::kWorld};
	q.name = std::string(name);
	return ResolveChar(finder, q);
}

ObjData *FindObjAround(CharData *finder, std::string_view name) {
	Query q;
	q.scopes = {Scope::kEquip, Scope::kInventory, Scope::kRoom};
	q.name = std::string(name);
	q.walk_containers = true;
	return ResolveObj(finder, q);
}

// World-wide object lookup by name: local scopes first (так первым найдётся то,
// что под рукой), затем весь реестр world_objects. Восстанавливает поведение
// старого get_obj_vis -- нужно командам уровня бога вроде do_stat, которым важно
// найти предмет где угодно в мире (надет на другого игрока, в хранилище и т.п.).
ObjData *FindObjInWorld(CharData *finder, std::string_view name) {
	Query q;
	q.scopes = {Scope::kEquip, Scope::kInventory, Scope::kRoom, Scope::kWorld};
	q.name = std::string(name);
	q.walk_containers = true;
	return ResolveObj(finder, q);
}

ObjData *FindObjByRnum(ObjRnum rnum) {
	Query q;
	q.scopes = {Scope::kRnum};
	q.rnum_lookup = rnum;
	q.visible_only = false;
	return ResolveObj(nullptr, q);
}

CharData *FindPlayer(CharData *finder, std::string_view name) {
	Query q;
	q.scopes = {Scope::kWorld};
	q.name = std::string(name);
	q.visible_only = false;
	q.char_predicate = [](CharData *c) { return !c->IsNpc(); };
	return ResolveChar(finder, q);
}

CharData *FindCharInRoomOrSelf(CharData *finder, std::string_view name) {
	// Legacy self-aliases (do_look "self", DG-script "kill me", etc.).
	if (name == "self" || name == "me"
		|| name == "я" || name == "меня" || name == "себя") {
		return finder;
	}
	return FindCharInRoom(finder, name);
}

CharData *FindPlayerVis(CharData *finder, std::string_view name) {
	Query q;
	q.scopes = {Scope::kOnlinePcs};
	q.name = std::string(name);
	return ResolveChar(finder, q);
}

// ---- Named filter factories (issue #3375 stage 3) -------------------------

namespace {

// Walk up the charm-chain so a charmed NPC resolves to its PC master for
// alliance purposes. Mirrors AllyRoot() in spells_info.cpp -- bounded at depth
// 4 so a pathological charm cycle can't hang the loop. (See
// bylins-issue-ambiguous-spells.)
const CharData *AllyRoot(const CharData *ch) {
	if (!ch) return nullptr;
	for (int depth = 0; depth < 4 && IsCharmice(ch) && ch->has_master(); ++depth) {
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
		return viewer && cand && sight::CanSee(viewer, cand);
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
		return viewer && cand && sight::CanSeeObj(viewer, cand);
	};
}

RoomRnum GetRandomTeleportTargetInZone(CharData *ch, RoomRnum zone_room) {
	int rnum_start = 0, rnum_stop = 0;
	GetZoneRooms(world[zone_room]->zone_rn, &rnum_start, &rnum_stop);
	int n = rnum_stop - rnum_start + 1;
	if (n <= 0) {
		return kNowhere;
	}
	std::vector<int> rooms(n);
	for (int i = 0; i < n; ++i) {
		rooms[i] = rnum_start + i;
	}
	RoomRnum fnd_room = kNowhere;
	for (; n; --n) {
		const int j = number(0, n - 1);
		fnd_room = rooms[j];
		rooms[j] = rooms[n - 1];
		if (SECT(fnd_room) != ESector::kSecret
			&& !ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap)
			&& !ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel)
			&& !ROOM_FLAGGED(fnd_room, ERoomFlag::kNoTeleportIn)
			&& !ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap)
			&& !ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap)
			&& (!ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) || privilege::IsImmortal(ch))
			&& Clan::MayEnter(ch, fnd_room, kHousePortal)) {
			break;
		}
	}
	return n ? fnd_room : kNowhere;
}


// issue.handler-cleaning (Bucket 4): generic target-search helpers moved from handler.cpp.

ObjData *get_obj_vis_for_locate(CharData *ch, const char *name) {
	ObjData *i;
	int number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	// scan items carried //
	if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != nullptr) {
		return i;
	}

	// scan room //
	if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room]->contents)) != nullptr) {
		return i;
	}

	strcpy(tmp, name);
	number = get_number(&tmp);
	if (number != 1) {
		return nullptr;
	}

	// ok.. no luck yet. scan the entire obj list   //
	const WorldObjects::predicate_f locate_predicate = [&](const ObjData::shared_ptr &i) -> bool {
		const auto result = sight::CanSeeObj(ch, i.get())
			&& (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i.get(), ch))
			&& try_locate_obj(ch, i.get());
		return result;
	};

	return world_objects.find_if(locate_predicate).get();
}

bool try_locate_obj(CharData *ch, ObjData *i) {
	if (IS_CORPSE(i) || privilege::IsGod(ch)) //имм может локейтить и можно локейтить трупы
	{
		return true;
	} else if (i->has_flag(EObjFlag::kNolocate)) //если флаг !локейт и ее нет в комнате/инвентаре - пропустим ее
	{
		return false;
	} else if (i->get_carried_by() && i->get_carried_by()->IsNpc()) {
		if (world[i->get_carried_by()->in_room]->zone_rn
			== world[ch->in_room]->zone_rn) //шмотки у моба можно локейтить только в одной зоне
		{
			return true;
		} else {
			return false;
		}
	} else if (i->get_in_room() != kNowhere && i->get_in_room()) {
		if (world[i->get_in_room()]->zone_rn
			== world[ch->in_room]->zone_rn) //шмотки в клетке можно локейтить только в одной зоне
		{
			return true;
		} else {
			return false;
		}
	} else if (i->get_worn_by() && i->get_worn_by()->IsNpc()) {
		if (world[i->get_worn_by()->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
			return true;
		} else {
			return false;
		}
	} else if (i->get_in_obj()) {
		if (Clan::is_clan_chest(i->get_in_obj())) {
			return true;
		} else {
			const auto in_obj = i->get_in_obj();
			if (in_obj->get_carried_by()) {
				if (in_obj->get_carried_by()->IsNpc()) {
					if (world[in_obj->get_carried_by()->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
						return true;
					} else {
						return false;
					}
				} else {
					return true;
				}
			} else if (in_obj->get_in_room() != kNowhere && in_obj->get_in_room()) {
				if (world[in_obj->get_in_room()]->zone_rn == world[ch->in_room]->zone_rn) {
					return true;
				} else {
					return false;
				}
			} else if (in_obj->get_worn_by()) {
				const auto worn_by = i->get_in_obj()->get_worn_by();
				if (worn_by->IsNpc()) {
					if (world[worn_by->in_room]->zone_rn == world[ch->in_room]->zone_rn) {
						return true;
					} else {
						return false;
					}
				} else {
					return true;
				}
			} else {
				return true;
			}
		}
	} else {
		return true;
	}
}

int generic_find(char *arg, Bitvector bitvector, CharData *ch, CharData **tar_ch, ObjData **tar_obj) {
	char name[kMaxInputLength];

	*tar_ch = nullptr;
	*tar_obj = nullptr;

	ObjData *i;
	int l, number, j = 0;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	if (strlen(arg) > 256)
		return 0;
	one_argument(arg, name);

	if (!*name)
		return (0);

	if (IS_SET(bitvector, EFind::kCharInRoom))    // Find person in room
	{
		*tar_ch = target_resolver::FindCharInRoom(ch, name);
		if ((*tar_ch != nullptr))
			return (EFind::kCharInRoom);
	}
	if (IS_SET(bitvector, EFind::kCharInWorld)) {
		*tar_ch = target_resolver::FindCharInWorld(ch, name);
		if ((*tar_ch != nullptr))
			return (EFind::kCharInWorld);
	}
	if (IS_SET(bitvector, EFind::kObjWorld)) {
		if ((*tar_obj = target_resolver::FindObjAround(ch, name)))
			return (EFind::kObjWorld);
	}

	// Начало изменений. (с) Дмитрий ака dzMUDiST ака

// Переписан код, обрабатывающий параметры EFind::kObjEquip | EFind::kObjInventory | EFind::kObjRoom
// В итоге поиск объекта просиходит в " инветаре - комнате - экипировке" согласно
// общему количеству имеющихся "созвучных" предметов.
// Старый код закомментирован и подан в конце изменений.

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return 0;
	if (IS_SET(bitvector, EFind::kObjInventory)) {
		for (i = ch->carrying; i && (j <= number); i = i->get_next_content()) {
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, EFind::kObjExtraDesc)
					&& sight::find_exdesc(tmp, i->get_ex_description()))) {
				if (sight::CanSeeObj(ch, i)) {
					if (++j == number) {
						*tar_obj = i;
						return (EFind::kObjInventory);
					}
				}
			}
		}
	}
	if (IS_SET(bitvector, EFind::kObjRoom)) {
	for (auto i : world[ch->in_room]->contents) {
		if (j > number) break;
			if (isname(tmp, i->get_aliases())
				|| CHECK_CUSTOM_LABEL(tmp, i, ch)
				|| (IS_SET(bitvector, EFind::kObjExtraDesc)
					&& sight::find_exdesc(tmp, i->get_ex_description()))) {
				if (sight::CanSeeObj(ch, i)) {
					if (++j == number) {
						*tar_obj = i;
						return (EFind::kObjRoom);
					}
				}
			}
		}
	}
	if (IS_SET(bitvector, EFind::kObjEquip)) {
		for (l = 0; l < EEquipPos::kNumEquipPos; l++) {
			if (GET_EQ(ch, l) && sight::CanSeeObj(ch, GET_EQ(ch, l))) {
				if (isname(tmp, GET_EQ(ch, l)->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, GET_EQ(ch, l), ch)
					|| (IS_SET(bitvector, EFind::kObjExtraDesc)
						&& sight::find_exdesc(tmp, GET_EQ(ch, l)->get_ex_description()))) {
					if (++j == number) {
						*tar_obj = GET_EQ(ch, l);
						return (EFind::kObjEquip);
					}
				}
			}
		}
	}
	return (0);
}

int find_all_dots(char *arg) {
	char tmpname[kMaxInputLength];

	if (!str_cmp(arg, "all") || !str_cmp(arg, "все")) {
		return (kFindAll);
	} else if (!strn_cmp(arg, "all.", 4) || !strn_cmp(arg, "все.", 4)) {
		strl_cpy(tmpname, arg + 4, kMaxInputLength);
		strl_cpy(arg, tmpname, kMaxInputLength);
		return (kFindAlldot);
	} else {
		return (kFindIndiv);
	}
}

RoomRnum FindRoomRnum(CharData *ch, char *rawroomstr, int trig) {
	RoomVnum tmp;
	RoomRnum location;
	CharData *target_mob;
	ObjData *target_obj;
	char roomstr[kMaxInputLength];

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		SendMsgToChar("Укажите номер или название комнаты.\r\n", ch);
		return (kNowhere);
	}
	if (a_isdigit(*roomstr) && !strchr(roomstr, '.')) {
		tmp = atoi(roomstr);
		if ((location = GetRoomRnum(tmp)) == kNowhere) {
			SendMsgToChar("Нет комнаты с таким номером.\r\n", ch);
			return (kNowhere);
		}
	} else if ((target_mob = target_resolver::FindCharInWorld(ch, roomstr)) != nullptr) {
		location = target_mob->in_room;
	} else if ((target_obj = target_resolver::FindObjInWorld(ch, roomstr)) != nullptr) {
		if (target_obj->get_in_room() != kNowhere) {
			location = target_obj->get_in_room();
		} else {
			SendMsgToChar("Этот объект вам недоступен.\r\n", ch);
			return (kNowhere);
		}
	} else {
		SendMsgToChar("В округе нет похожего предмета или создания.\r\n", ch);
		return (kNowhere);
	}

	// a location has been found -- if you're < GRGOD, check restrictions.
	if (!privilege::IsGrGod(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		if (ROOM_FLAGGED(location, ERoomFlag::kGodsRoom) && GetRealLevel(ch) < kLvlGreatGod) {
			SendMsgToChar("Вы не столь божественны, чтобы получить доступ в эту комнату!\r\n", ch);
			return (kNowhere);
		}
		if (ROOM_FLAGGED(location, ERoomFlag::kNoTeleportIn) && trig != 1) {
			SendMsgToChar("В комнату не телепортировать!\r\n", ch);
			return (kNowhere);
		}
		if (!Clan::MayEnter(ch, location, kHousePortal)) {
			SendMsgToChar("Частная собственность - посторонним в ней делать нечего!\r\n", ch);
			return (kNowhere);
		}
	}
	return (location);
}

}; // namespace target_resolver


// issue.handler-cleaning (split): entity finders moved from handler.cpp. Global (not in
// namespace target_resolver) so existing unqualified callers keep working via handler.h.

ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 ObjData *list,
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number);
ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 const ObjData::obj_list_t &list,
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number);
ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 ObjData *equip[],
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number);

ObjData *GetObjByRnumInContent(ObjRnum rnum, ObjData *list) {
	ObjData *i;
	for (i = list; i; i = i->get_next_content()) {
		if (i->get_rnum() == rnum) {
			return (i);
		}
	}

	return nullptr;
}

ObjData *GetObjByRnumInContent(ObjRnum rnum, const ObjData::obj_list_t &list) {
	for (auto i : list) {
		if (i->get_rnum() == rnum) {
			return i;
		}
	}
	return nullptr;
}

ObjData *GetObjByVnumInContent(ObjVnum vnum, ObjData *list) {
	ObjData *i;
	for (i = list; i; i = i->get_next_content()) {
		if (i->get_vnum() == vnum) {
			return (i);
		}
	}

	return nullptr;
}

CharData *get_player_of_name(const char *name) {
	for (const auto &i : character_list) {
		if (i->IsNpc()) {
			continue;
		}
		if (!isname(name, i->GetCharAliases())) {
			continue;
		}
		return i.get();
	}
	return nullptr;
}

ObjData *get_obj_in_list_vis(CharData *ch, const char *name, const ObjData::obj_list_t &list, bool locate_item) {
	int j = 0, number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return (nullptr);

	//Запретим локейт 2. 3. n. стафин
	if (number > 1 && locate_item)
		return (nullptr);

	for (auto i : list) {
		if (j > number) break;
		if (i->get_extracted_list()) {
			continue;
		}
		if (isname(tmp, i->get_aliases())
			|| CHECK_CUSTOM_LABEL(tmp, i, ch)) {
			if (sight::CanSeeObj(ch, i)) {
				// sprintf(buf,"Show obj %d %s %x ", number, i->name, i);
				// SendMsgToChar(buf,ch);
				if (!locate_item) {
					if (++j == number)
						return (i);
				} else {
					if (try_locate_obj(ch, i))
						return (i);
					else
						continue;
				}
			}
		}
	}

	return (nullptr);
}

ObjData *get_obj_in_list_vis(CharData *ch, const char *name, ObjData *list, bool locate_item) {
	ObjData::obj_list_t tmp_list;
	for (auto obj = list; obj; obj = obj->get_next_content()) {
		tmp_list.push_back(obj);
	}
	return get_obj_in_list_vis(ch, name, tmp_list, locate_item);
}

ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 ObjData *list,
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number) {
	for (auto item = list; item != nullptr; item = item->get_next_content()) {
		if (sight::CanSeeObj(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}
	return nullptr;
}

ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 const ObjData::obj_list_t &list,
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number) {
	for (auto item : list) {
		if (sight::CanSeeObj(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

ObjData *get_obj_vis_and_dec_num(CharData *ch,
								 const char *name,
								 ObjData *equip[],
								 std::unordered_set<unsigned int> &id_obj_set,
								 int &number) {
	for (auto i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		auto item = equip[i];
		if (item && sight::CanSeeObj(ch, item)) {
			if (isname(name, item->get_aliases())
				|| CHECK_CUSTOM_LABEL(name, item, ch)) {
				if (--number == 0) {
					return item;
				}
				id_obj_set.insert(item->get_id());
			}
		}
	}

	return nullptr;
}

ObjData *get_object_in_equip_vis(CharData *ch, const char *arg, ObjData *equipment[], int *j) {
	int l, number;
	char tmpname[kMaxInputLength];
	char *tmp = tmpname;

	strcpy(tmp, arg);
	if (!(number = get_number(&tmp)))
		return (nullptr);

	for ((*j) = 0, l = 0; (*j) < EEquipPos::kNumEquipPos; (*j)++) {
		if (equipment[(*j)]) {
			if (sight::CanSeeObj(ch, equipment[(*j)])) {
				if (isname(tmp, equipment[(*j)]->get_aliases())
					|| CHECK_CUSTOM_LABEL(tmp, equipment[(*j)], ch)) {
					if (++l == number) {
						return equipment[(*j)];
					}
				}
			}
		}
	}

	return (nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
