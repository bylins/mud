#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_H_

#include "engine/structs/structs.h"
#include "engine/structs/meta_enum.h"
#include "gameplay/skills/skills.h"   // ESkill
#include "utils/grammar/cases.h"   // grammar::ECase (per-stone object name)

#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

namespace parser_wrapper { class DataNode; }
class CharData;

// issue.runestones: the runestone mechanics. A runestone sits in a room; a character who reads
// (examines) one with enough skill memorises it and can later open a town-portal to that room. Data
// is in cfg/mechanics/rune_stones.xml (this is the registry) and the display text in
// cfg/messages/ru/rune_stone_msg.xml. The townportal skill + one-way portal stay in townportal.{h,cpp}.

// Display-text slots. kName is per-stone (sheaf keyed by stone vnum); the rest are shared (kDefault).
enum class ERuneStoneMsg {
	kUndefined = 0,
	kName,                              // the codeword players type / see
	kAliases,                          // the physical stone object's keyword aliases (default-fallback)
	kRoomNormal, kRoomDamaged,          // the stone's presence in the room (look)
	kInspectNormal, kInspectDamaged,    // examining the stone ({name})
	kLackSkillNormal, kLackSkillDamaged,// examining without enough skill
	kListEmpty, kListHeader, kListCount, kListLimit,   // the remembered-list ({count}/{limit})
	kMemorized, kForgotten,             // ({name})
	kMemoryFull, kCantMemorize, kCantForget,
};

[[nodiscard]] const std::string &RuneStoneMsg(ERuneStoneMsg slot);   // shared (kDefault) text
[[nodiscard]] const std::string &RuneStoneName(int stone_vnum);      // per-stone kName
// The physical stone object's declined display name (the <name> section), per-stone with a
// fallback to the kDefault sheaf.
[[nodiscard]] const std::string &RuneStoneObjName(int stone_vnum, grammar::ECase name_case);

class Runestone {
 public:
  enum class State { kEnabled, kDisabled, kForbidden };

  Runestone() = default;
  Runestone(std::string_view name,
			RoomVnum room_vnum,
			ESkill skill,
			int skill_level,
			std::string_view id = "",
			State state = State::kEnabled,
			int vnum = -1)
	  : name_(name),
		id_(id),
		vnum_(vnum),
		room_vnum_(room_vnum),
		skill_(skill),
		skill_level_(skill_level),
		state_(state) {}

  [[nodiscard]] std::string_view GetName() const { return name_; };
  [[nodiscard]] std::string_view GetId() const { return id_; };
  [[nodiscard]] int GetVnum() const { return vnum_; };   // the stone's identity (cfg/Vedun key), not the room
  [[nodiscard]] RoomVnum GetRoomVnum() const { return room_vnum_; };
  [[nodiscard]] ESkill GetSkill() const { return skill_; };
  [[nodiscard]] int GetSkillLevel() const { return skill_level_; };
  [[nodiscard]] bool IsEnabled() const { return (state_ == State::kEnabled); };
  [[nodiscard]] bool IsDisabled() const { return (state_ != State::kEnabled); };
  [[nodiscard]] bool IsAllowed() const { return (state_ != State::kForbidden); };
  [[nodiscard]] bool IsForbidden() const { return (state_ == State::kForbidden); };
  void SetEnabled(bool enabled);

 private:
  std::string name_;
  std::string id_;
  int vnum_{-1};
  RoomVnum room_vnum_{0};
  ESkill skill_{ESkill::kTownportal};
  int skill_level_{0};
  State state_{State::kEnabled};
};

class RunestoneRoster : private std::vector<Runestone> {
 public:
  RunestoneRoster();
  RunestoneRoster(const RunestoneRoster &) = delete;
  RunestoneRoster(RunestoneRoster &&) = delete;
  RunestoneRoster &operator=(const RunestoneRoster &) = delete;
  RunestoneRoster &operator=(RunestoneRoster &&) = delete;

  void Load(parser_wrapper::DataNode data);   // cfg/mechanics/rune_stones.xml
  [[nodiscard]] bool Validate(parser_wrapper::DataNode data) const;   // Vedun dry-run; no mutation
  void SpawnStones();                         // phase 3: place the physical stone object into each room
  void RefreshStoneObject(const Runestone &stone);   // re-sync a stone object's room desc to its state
  void SaveState();                           // persist each stone's damaged flag back to rune_stones.xml
  bool ViewRunestone(CharData *ch);           // inspect/memorise the stone in ch's room (obj spec proc)
  Runestone &FindRunestone(RoomVnum vnum);
  Runestone &FindRunestone(std::string_view name);
  std::vector<RoomVnum> GetVnumRoster();
  std::vector<std::string_view> GetNameRoster();
  [[nodiscard]] ObjVnum GetPrototypeVnum() const { return prototype_vnum_; };   // physical stone object (phase 3)

  // Read-only iteration for the Vedun editor's element list (re-exposes the private base's
  // iterators without hiding the non-const ones the member functions rely on).
  using std::vector<Runestone>::begin;
  using std::vector<Runestone>::end;

 private:
  Runestone incorrect_stone_;
  ObjVnum prototype_vnum_{0};
};

 class CharacterRunestoneRoster : private std::vector<RoomVnum> {
  public:
   void Clear() { clear(); };
   void Serialize(std::ostringstream &out);
   void DeleteIrrelevant(CharData *ch);
   void PageToChar(CharData *ch);
   bool AddRunestone(const Runestone &stone);
   bool RemoveRunestone(const Runestone &stone);
   void AddRunestone(CharData *ch, const Runestone &stone);
   void RemoveRunestone(CharData *ch, const Runestone &stone);
   bool Contains(const Runestone &stone);
   bool IsFull(CharData *ch);
   std::size_t Count() { return size(); };
   static std::size_t CalcLimit(CharData *ch);

  private:
   void ShrinkToLimit(CharData *ch);
   bool IsOverfilled(CharData *ch);
};

// Per-character runestone operations. Thin forwarders to ch's CharacterRunestoneRoster (in
// player_specials); they live here -- not as CharData methods -- so the runestone mechanic stays
// self-contained and CharData isn't bloated with mechanic-specific helpers.
void ClearRunestones(CharData *ch);
void AddRunestone(CharData *ch, const Runestone &stone);
void RemoveRunestone(CharData *ch, const Runestone &stone);
void DeleteIrrelevantRunestones(CharData *ch);
void PageRunestonesToChar(CharData *ch);
[[nodiscard]] bool IsRunestoneKnown(CharData *ch, const Runestone &stone);

template<>
const std::string &NAME_BY_ITEM<ERuneStoneMsg>(ERuneStoneMsg item);
template<>
ERuneStoneMsg ITEM_BY_NAME<ERuneStoneMsg>(const std::string &name);
template<>
const std::map<ERuneStoneMsg, std::string> &NAMES_OF<ERuneStoneMsg>();

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
