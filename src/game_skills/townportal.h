#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "structs/structs.h"

class CharData;

class Runestone {
 public:
  enum class State { kEnabled, kDisabled, kForbidden };

  Runestone() = default;
  Runestone(std::string_view name,
			RoomVnum room_vnum,
			int min_char_level,
			State state = State::kEnabled)
	  : name_(name),
		room_vnum_(room_vnum),
		min_char_level_(min_char_level),
		state_(state) {}

  [[nodiscard]] std::string_view GetName() const { return name_; };
  [[nodiscard]] RoomVnum GetRoomVnum() const { return room_vnum_; };
  [[nodiscard]] int GetMinCharLevel() const { return min_char_level_; };
  [[nodiscard]] bool IsEnabled() const { return (state_ == State::kEnabled); };
  [[nodiscard]] bool IsDisabled() const { return (state_ != State::kEnabled); };
  [[nodiscard]] bool IsAllowed() const { return (state_ != State::kForbidden); };
  [[nodiscard]] bool IsForbidden() const { return (state_ == State::kForbidden); };
  void SetEnabled(bool enabled);

 private:
  std::string name_;
  RoomVnum room_vnum_{0};
  int min_char_level_{0};
  State state_{State::kEnabled};
};

class RunestoneRoster : private std::vector<Runestone> {
 public:
  RunestoneRoster();
  RunestoneRoster(const RunestoneRoster &) = delete;
  RunestoneRoster(RunestoneRoster &&) = delete;
  RunestoneRoster &operator=(const RunestoneRoster &) = delete;
  RunestoneRoster &operator=(RunestoneRoster &&) = delete;

  void LoadRunestones();
  void ShowRunestone(CharData *ch);
  bool ViewRunestone(CharData *ch, int where_bits);
  Runestone &FindRunestone(RoomVnum vnum);
  Runestone &FindRunestone(std::string_view name);
  std::vector<RoomVnum> GetVnumRoster();
  std::vector<std::string_view> GetNameRoster();

 private:
  Runestone incorrect_stone_;
};

 class CharacterRunestoneRoster : private std::vector<RoomVnum> {
  public:
   void Clear() { clear(); };
   void Serialize(std::ostringstream &out);
   void DeleteIrrelevant(CharData *ch);
   void PageToChar(CharData *ch);
   bool AddRunestone(const Runestone &stone);
   bool RemoveRunestone(const Runestone &stone);
   bool Contains(const Runestone &stone);
   bool IsFull(CharData *ch);
   std::size_t Count() { return size(); };
   static std::size_t CalcLimit(CharData *ch);

  private:
   void ShrinkToLimit(CharData *ch);
   bool IsOverfilled(CharData *ch);
};

void DecayPortalMessage(RoomRnum room_num);

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/

namespace one_way_portal {
void ReplacePortalTimer(CharData *ch, RoomRnum from_room, RoomRnum to_room, int time);

} // namespace OneWayPortal


#endif //BYLINS_TOWNPORTAL_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
