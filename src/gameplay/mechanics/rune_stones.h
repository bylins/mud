#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_H_

#include "engine/structs/structs.h"
#include "engine/structs/meta_enum.h"
#include "gameplay/skills/skills.h"   // ESkill

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
	kRoomNormal, kRoomDamaged,          // the stone's presence in the room (look)
	kInspectNormal, kInspectDamaged,    // examining the stone ({name})
	kLackSkillNormal, kLackSkillDamaged,// examining without enough skill
	kListEmpty, kListHeader, kListCount, kListLimit,   // the remembered-list ({count}/{limit})
	kMemorized, kForgotten,             // ({name})
	kMemoryFull, kCantMemorize, kCantForget,
};

[[nodiscard]] const std::string &RuneStoneMsg(ERuneStoneMsg slot);   // shared (kDefault) text
[[nodiscard]] const std::string &RuneStoneName(int stone_vnum);      // per-stone kName

class Runestone {
 public:
  enum class State { kEnabled, kDisabled, kForbidden };

  Runestone() = default;
  Runestone(std::string_view name,
			RoomVnum room_vnum,
			ESkill skill,
			int skill_level,
			std::string_view id = "",
			State state = State::kEnabled)
	  : name_(name),
		id_(id),
		room_vnum_(room_vnum),
		skill_(skill),
		skill_level_(skill_level),
		state_(state) {}

  [[nodiscard]] std::string_view GetName() const { return name_; };
  [[nodiscard]] std::string_view GetId() const { return id_; };
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
  void ShowRunestone(CharData *ch);
  bool ViewRunestone(CharData *ch, int where_bits);
  Runestone &FindRunestone(RoomVnum vnum);
  Runestone &FindRunestone(std::string_view name);
  std::vector<RoomVnum> GetVnumRoster();
  std::vector<std::string_view> GetNameRoster();
  [[nodiscard]] ObjVnum GetPrototypeVnum() const { return prototype_vnum_; };   // physical stone object (phase 3)

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

template<>
const std::string &NAME_BY_ITEM<ERuneStoneMsg>(ERuneStoneMsg item);
template<>
ERuneStoneMsg ITEM_BY_NAME<ERuneStoneMsg>(const std::string &name);
template<>
const std::map<ERuneStoneMsg, std::string> &NAMES_OF<ERuneStoneMsg>();

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_RUNE_STONES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
