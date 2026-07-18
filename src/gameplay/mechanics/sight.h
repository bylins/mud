/**
\file sight.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Mechanics of sight.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_SIGHT_H_
#define BYLINS_SRC_GAME_MECHANICS_SIGHT_H_

#include "engine/entities/room_data.h"

#include <memory>

class CharData;
class ObjData;
class CObjectPrototype;

namespace sight {

const int EXIT_SHOW_WALL = (1 << 0);
const int EXIT_SHOW_LOOKING = (1 << 1);
void look_at_room(CharData *ch, int ignore_brief, bool msdp_mode = true);
void skip_hide_on_look(CharData *ch);
void list_char_to_char(const RoomData::people_t &list, CharData *ch);
bool look_at_target(CharData *ch, char *arg, int subcmd);
void show_glow_objs(CharData *ch);
void look_in_obj(CharData *ch, char *arg);
void look_in_direction(CharData *ch, int dir, int info_is);
void list_obj_to_char(ObjData *list, CharData *ch, int mode, int show);
void print_zone_info(CharData *ch);
const char *find_exdesc(const char *word, const std::vector<ExtraDescription> &list);
const char *show_obj_to_char(ObjData *object, CharData *ch, int mode, int show_state, int how);
void obj_info(CharData *ch, ObjData *obj, char buf[kMaxStringLength]);
void print_zone_info(CharData *ch);
const char *print_obj_state(int tm_pct);
void diag_char_to_char(CharData *i, CharData *ch);
void Appear(CharData *ch);


// issue.chardata-cleaning: "who can see whom" predicates (moved off CharData).
// issue.utils-cleaning: consider_light=false skips the room-darkness check; CanSeeIgnoringLight
// folds the old MORT_CAN_SEE_CHAR/IMM_CAN_SEE_CHAR/CAN_SEE_CHAR "see without light" macros.
// issue.utils-cleaning: moved here from utils.h / char_data.h (both are vision predicates).
bool CanSeeInDark(const CharData *ch);   // infravision or holylight
bool InvisOk(const CharData *sub, const CharData *obj);  // invis/hide/disguise don't block sub seeing obj
inline bool InvisOk(const CharData *sub, const std::shared_ptr<CharData> &obj) { return InvisOk(sub, obj.get()); }
bool MortCanSee(const CharData *sub, const CharData *obj, bool consider_light = true);
bool MaySee(const CharData *ch, const CharData *sub, const CharData *obj);
bool ImmCanSee(const CharData *sub, const CharData *obj, bool consider_light = true);
bool CanSee(const CharData *sub, const CharData *obj);
bool CanSeeIgnoringLight(const CharData *sub, const CharData *obj);
inline bool CanSee(const CharData *sub, const std::shared_ptr<CharData> &obj) { return CanSee(sub, obj.get()); }
inline bool CanSee(const std::shared_ptr<CharData> &sub, const CharData *obj) { return CanSee(sub.get(), obj); }
inline bool CanSee(const std::shared_ptr<CharData> &sub, const std::shared_ptr<CharData> &obj) {
	return CanSee(sub.get(), obj.get());
}
inline bool CanSeeIgnoringLight(const CharData *sub, const std::shared_ptr<CharData> &obj) { return CanSeeIgnoringLight(sub, obj.get()); }
inline bool CanSeeIgnoringLight(const std::shared_ptr<CharData> &sub, const CharData *obj) { return CanSeeIgnoringLight(sub.get(), obj); }
inline bool CanSeeIgnoringLight(const std::shared_ptr<CharData> &sub, const std::shared_ptr<CharData> &obj) {
	return CanSeeIgnoringLight(sub.get(), obj.get());
}

// issue.utils-cleaning: PERS as a function. A viewer's rendering of character `ch`'s name in
// grammatical case `pad`: the real declined name if the viewer can see them, else the indefinite
// "кто-то" fallback. (Replaces the PERS macro that lived in utils.h.)
const char *PersonName(const CharData *ch, const CharData *viewer, int pad);

// issue.utils-cleaning: object visibility predicates, mirroring CanSee/MortCanSee (moved here
// from engine/core/utils_char_obj.inl for cohesion -- all "who/what can see what" in one place).
bool MortCanSeeObj(const CharData *sub, const ObjData *obj);
bool CanSeeObj(const CharData *sub, const ObjData *obj);

// issue.chardata-cleaning: object-description helpers (were ad-hoc extern-declared in economics/etc.)
char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);
char *diag_timer_to_char(const ObjData *obj);
const char *diag_obj_timer(const ObjData *obj);

}  // namespace sight

#endif //BYLINS_SRC_GAME_MECHANICS_SIGHT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
