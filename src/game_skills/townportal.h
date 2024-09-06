#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "structs/structs.h"

class CharData;

class Townportal {
 public:
  enum class State { kEnabled, kDisabled, kForbidden };

  Townportal() = default;
  Townportal(std::string_view name,
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

class TownportalRoster : private std::vector<Townportal> {
 public:
  TownportalRoster();
  TownportalRoster(const TownportalRoster &) = delete;
  TownportalRoster(TownportalRoster &&) = delete;
  TownportalRoster &operator=(const TownportalRoster &) = delete;
  TownportalRoster &operator=(TownportalRoster &&) = delete;

  void LoadTownportals();
  void ShowPortalRunestone(CharData *ch);
  bool ViewTownportal(CharData *ch, int where_bits);
  Townportal &FindTownportal(RoomVnum vnum);
  Townportal &FindTownportal(std::string_view name);

 private:
  Townportal incorrect_portal_;
};

 class CharacterTownportalRoster : private std::vector<RoomVnum> {
  public:
   void Clear() { clear(); };
   void Serialize(std::ostringstream &out);
   void AddTownportalToChar(const Townportal &portal);
   void CleanupSurplusPortals(CharData *ch);
   void ForgetTownportal(CharData *ch, const Townportal &portal);
   void ListKnownTownportalsToChar(CharData *ch);
   bool IsPortalKnown(const Townportal &portal);
   std::size_t Count() { return size(); };
   static std::size_t CalcMaxPortals(CharData *ch);

  private:
   void ShrinkToLimit(CharData *ch);
   bool IsTownportalsOverfilled(CharData *ch);
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
