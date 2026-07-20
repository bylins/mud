/**
\file spec_procs.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_AI_SPEC_PROCS_H_
#define BYLINS_SRC_GAMEPLAY_AI_SPEC_PROCS_H_

#include "engine/structs/structs.h"

#include <set>
#include <unordered_map>

class CharData;
int exchange(CharData *ch, void *me, int cmd, char *argument);
int horse_keeper(CharData *ch, void *me, int cmd, char *argument);
int mercenary(CharData *ch, void * /*me*/, int cmd, char *argument);
int shop_ext(CharData *ch, void *me, int cmd, char *argument);
bool is_post(RoomRnum room);
bool is_rent(RoomRnum room);

int RentReceptionist(CharData *ch, void *me, int cmd, char *argument);
int postmaster(CharData *, void *, int, char *);
int bank(CharData *, void *, int, char *);
int shop_ext(CharData *, void *, int, char *);
int mercenary(CharData *, void *, int, char *);
void npc_groupbattle(CharData *ch);

namespace specials {
// issue.specials: data-driven registry of "what special is this", mirroring the prototype func
// pointer; the single source of truth for special-entity IDENTITY (replaces GET_MOB_SPEC == fn).
enum class ESpecial {
	kNone, kRent, kMail, kBank, kHorse, kExchange, kMercenary, kOutfit, kTorc, kPuff,
	kShop, kGuild, kBoard, kDump,
};
void RegisterMob(int vnum, ESpecial s);
void UnregisterMob(int vnum, ESpecial s);  // remove ONE special (vs RegisterMob(kNone) = erase all)
void RegisterObj(int vnum, ESpecial s);
void RegisterRoom(int vnum, ESpecial s);
[[nodiscard]] const std::set<ESpecial> &MobSpecials(int vnum);  // a mob may have several specials
// Весь реестр специальных мобов (vnum -> набор специальностей), как он загружен из cfg/specials.xml.
// Нужен тем, кому интересны не отдельные мобы, а картина целиком -- например, в каких зонах стоят
// рента, почта и банкир. Перебрать десяток записей реестра дешевле, чем обходить мир.
[[nodiscard]] const std::unordered_map<int, std::set<ESpecial>> &AllMobSpecials();
[[nodiscard]] ESpecial ObjSpecial(int vnum);
[[nodiscard]] bool IsMobSpecial(int vnum);
[[nodiscard]] bool IsMobSpecial(int vnum, ESpecial s);
[[nodiscard]] bool IsMobSpecialInRoom(RoomRnum room, ESpecial s);
[[nodiscard]] bool SharesMobSpecial(int v1, int v2);  // do two mobs have any special in common
// issue.utils-cleaning: service-keeper identity, registry-backed. Replaces the IS_*KEEPER macros
// that compared mob_index[].func -- which is no longer set for mobs (all mob specials are
// data-driven via cfg/specials.xml -> do_specproc), so those macros were dead/always-false.
[[nodiscard]] bool IsShopkeeper(const CharData *ch);
[[nodiscard]] bool IsPostkeeper(const CharData *ch);
[[nodiscard]] bool IsBankkeeper(const CharData *ch);
[[nodiscard]] bool IsRentkeeper(const CharData *ch);
// Find the room carrier registered with `s` (honouring fnum) and run `line` through its handler;
// if no carrier handles it, send the standard "wrong place" reply.
void RunSpecCommand(CharData *ch, ESpecial s, char *line);
} // namespace specials

#endif //BYLINS_SRC_GAMEPLAY_AI_SPEC_PROCS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
