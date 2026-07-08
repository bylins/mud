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
#include "engine/entities/entities_constants.h"   // EMobFlag
#include "gameplay/affects/affect_contants.h"     // EAffect
#include "engine/entities/obj_data.h"          // ObjData::obj_list_t for the finder decls

#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

class CharData;
class ObjData;
struct RoomData;

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
//   * `visible_only = true` gates char results through CanSee and obj results
//     through CanSeeObj. Set to false for scripted lookups that need to
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
	kSelf,        // just the searcher
	kEquip,       // searcher's equipment slots
	kInventory,   // searcher's carrying list
	kRoom,        // people / objects in searcher's room (or room_override if set)
	kWorld,       // global character_list (PC-priority order) / world_objects registry
	kOnlinePcs,   // active PCs (descriptor_list state=kPlaying, HERE() gated)
	kRnum,        // direct registry lookup -- pairs with rnum_lookup (chars/objs)
};

struct Query {
	std::vector<Scope> scopes = {Scope::kRoom};

	// Name filter. Accepts the legacy "N.name" syntax -- the resolver strips
	// the "N." prefix off and treats it as the ordinal (returns the N-th match
	// instead of the first). "2.guard" picks the second visible guard. An
	// ordinal of 0 (e.g. "0.foo") is treated as "no match" by the legacy
	// semantics this preserves.
	std::optional<std::string> name;

	// kRoom uses this room when set; otherwise falls back to searcher->in_room.
	// Lets a caller search a room the searcher isn't in (legacy
	// SearchCharInRoomByName / scripted lookups).
	std::optional<RoomRnum> room_override;

	// Direct registry-key lookup for kRnum. Mob rnum for ResolveChar (deferred
	// -- no caller asks for it today), obj rnum for ResolveObj (replaces
	// SearchObjByRnum).
	std::optional<int> rnum_lookup;

	// Walk INTO containers when scanning equip / inventory / room obj lists
	// (legacy get_obj_vis behaviour). Off by default since most callers want
	// a flat scope walk.
	bool walk_containers = false;

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

// ---- Convenience helpers --------------------------------------------------
//
// Thin shorthands over Query + Resolve* that cover the recurring shapes the
// legacy get_char_vis / get_obj_vis / SearchObjByRnum APIs used to serve.
// Use these when you don't need ordinals/predicates/custom scopes -- those
// callers should drop down to the full Query API.
//
//   FindCharInRoom  -- scopes = {kRoom}
//   FindCharInWorld -- scopes = {kRoom, kWorld}, PC-priority world walk
//   FindObjAround   -- scopes = {kEquip, kInventory, kRoom}, walks containers
//   FindObjInWorld  -- scopes = {kEquip, kInventory, kRoom, kWorld} (legacy get_obj_vis)
//   FindObjByRnum   -- direct world-registry lookup by rnum
//
CharData *FindCharInRoom (CharData *finder, std::string_view name);
CharData *FindCharInWorld(CharData *finder, std::string_view name);
ObjData  *FindObjAround  (CharData *finder, std::string_view name);
ObjData  *FindObjInWorld (CharData *finder, std::string_view name);
ObjData  *FindObjByRnum  (ObjRnum rnum);

// FindPlayer: PC-only world walk. Bypasses CanSee so admin / system code
// (titles, name validation, do_set, do_wizutil) can reach wizinvis or
// otherwise hidden PCs. Replaces the legacy `get_player_pun`.
CharData *FindPlayer(CharData *finder, std::string_view name);

// FindCharInRoomOrSelf: room-scoped char lookup, BUT the names
// "self"/"me"/"я"/"меня"/"себя" resolve to the searcher itself without a
// room walk. Replaces the legacy `get_char_room_vis`, used by DG scripts
// to let mobs target themselves by alias.
CharData *FindCharInRoomOrSelf(CharData *finder, std::string_view name);

// FindPlayerVis: visible-only PC lookup over descriptor_list (currently
// playing + HERE-gated). Used by player-facing commands -- tell, ask,
// glory, send, etc. -- that want to reach actively-connected PCs only.
// Distinct from FindPlayer (which sweeps character_list including hidden
// PCs) and FindCharInWorld (which can match NPCs).
CharData *FindPlayerVis(CharData *finder, std::string_view name);

// ---- Named filter factories (issue #3375 stage 3) -------------------------
//
// Building blocks for Query.char_predicate / Query.obj_predicate. Each
// returns a `std::function<bool(...)>` ready to plug into a Query. Compose
// by wrapping in a lambda that AND/OR's the results -- e.g.
//
//   q.char_predicate = [a = MakeAllyFilter(ch), v = MakeVisibleFilter(ch)]
//                      (CharData *c) { return a(c) && v(c); };
//
// Most filters that need a "searcher" / "viewer" / "root" capture it at
// factory time, so the returned predicate is a plain `bool(...)`.

using CharPredicate = std::function<bool(CharData *)>;
using ObjPredicate  = std::function<bool(ObjData *)>;

// Relationship-side filters. "Ally" walks the charm-chain root the same way
// issue.ambiguous-spells does -- charmed pets resolve to their PC master, two
// mobs without PC roots count as conditionally allied, etc.
CharPredicate MakeAllyFilter(CharData *root);
CharPredicate MakeEnemyFilter(CharData *root);

// Basic kind / state filters.
CharPredicate MakeNpcFilter();
CharPredicate MakePcFilter();
CharPredicate MakeFightingFilter();
CharPredicate MakeSameRoomFilter(CharData *root);
CharPredicate MakeVisibleFilter(CharData *viewer);   // CanSee-gated
CharPredicate MakeAffectFilter(EAffect flag);        // AFF_FLAGGED(ch, flag)
CharPredicate MakeMobFlagFilter(EMobFlag flag);      // NPC carrying flag

// Vnum lookups. Mobs are matched by their proto vnum; objs by their proto vnum.
CharPredicate MakeMobVnumFilter(MobVnum vnum);
ObjPredicate  MakeObjVnumFilter(ObjVnum vnum);
ObjPredicate  MakeObjVisibleFilter(CharData *viewer); // CanSeeObj-gated


// Pick a random room in the same zone as `zone_room` that a teleport/recall may land in (skips
// secret/deathtrap/tunnel/no-teleport-in/gods rooms the actor can't enter; honours clan-house
// access). Returns kNowhere if the zone has no valid room. issue.spellhandlers: folds the old
// GetTeleportTargetRoom + its per-site GetZoneRooms call into one.
RoomRnum GetRandomTeleportTargetInZone(CharData *ch, RoomRnum zone_room);

// issue.handler-cleaning (Bucket 4): generic target search (moved from handler).
ObjData *get_obj_vis_for_locate(CharData *ch, const char *name);
inline ObjData *get_obj_vis_for_locate(CharData *ch, const std::string &name) {
	return get_obj_vis_for_locate(ch, name.c_str());
}
bool try_locate_obj(CharData *ch, ObjData *i);
int generic_find(char *arg, Bitvector bitvector, CharData *ch, CharData **tar_ch, ObjData **tar_obj);
int find_all_dots(char *arg);
RoomRnum FindRoomRnum(CharData *ch, char *rawroomstr, int trig);

}; // namespace target_resolver

// issue.handler-cleaning: global-scope finder API + Find modes (moved from handler.h).
// These names stay at global scope (and the using-bridges below re-export the
// namespaced finders unqualified) so existing call sites need no qualification.
using target_resolver::get_obj_vis_for_locate;
using target_resolver::try_locate_obj;
using target_resolver::generic_find;
using target_resolver::find_all_dots;
using target_resolver::FindRoomRnum;

const int kFindIndiv = 0;
const int kFindAll = 1;
const int kFindAlldot = 2;

enum EFind : Bitvector {
	kCharInRoom = 1 << 0,
	kCharInWorld = 1 << 1,
	kCharDisconnected = 1 << 6,
	kObjInventory = 1 << 2,
	kObjRoom = 1 << 3,
	kObjWorld = 1 << 4,
	kObjEquip = 1 << 5,
	kObjExtraDesc = 1 << 7
};

ObjData *get_obj_in_list(char *name, ObjData *list);
ObjData *get_obj_in_list(const char *name, const ObjData::obj_list_t &list);
ObjData *GetObjByVnumInContent(int vnum, const ObjData::obj_list_t &list);

// issue.handler-cleaning (split): entity finders moved from handler.cpp (global scope).
ObjData *GetObjByRnumInContent(int obj_rnum, ObjData *list);
ObjData *GetObjByRnumInContent(int obj_rnum, const ObjData::obj_list_t &list);
ObjData *GetObjByVnumInContent(int vnum, ObjData *list);
CharData *get_player_of_name(const char *name);
ObjData *get_obj_in_list_vis(CharData *ch, const char *name, const ObjData::obj_list_t &list, bool locate_item = false);
ObjData *get_obj_in_list_vis(CharData *ch, const char *name, ObjData *list, bool locate_item = false);
inline ObjData *get_obj_in_list_vis(CharData *ch, const std::string &name, const ObjData::obj_list_t &list) {
	return get_obj_in_list_vis(ch, name.c_str(), list);
}
inline ObjData *get_obj_in_list_vis(CharData *ch, const std::string &name, ObjData *list) {
	return get_obj_in_list_vis(ch, name.c_str(), list);
}
ObjData *get_object_in_equip_vis(CharData *ch, const char *arg, ObjData *equipment[], int *j);
inline ObjData *get_object_in_equip_vis(CharData *ch, const std::string &arg, ObjData *equipment[], int *j) {
	return get_object_in_equip_vis(ch, arg.c_str(), equipment, j);
}

#endif // _TARGET_RESOLVER_HPP_INCLUDED_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
