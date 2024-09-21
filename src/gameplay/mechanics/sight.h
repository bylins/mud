/**
\file sight.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Mechanics of sight.
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_SIGHT_H_
#define BYLINS_SRC_GAME_MECHANICS_SIGHT_H_

#include "engine/entities/room_data.h"

const int EXIT_SHOW_WALL = (1 << 0);
const int EXIT_SHOW_LOOKING = (1 << 1);

class CharData;
void look_at_room(CharData *ch, int ignore_brief, bool msdp_mode = true);
void skip_hide_on_look(CharData *ch);
void list_char_to_char(const RoomData::people_t &list, CharData *ch);
bool look_at_target(CharData *ch, char *arg, int subcmd);
void show_glow_objs(CharData *ch);
void look_in_obj(CharData *ch, char *arg);
void look_in_direction(CharData *ch, int dir, int info_is);
void list_obj_to_char(ObjData *list, CharData *ch, int mode, int show);
void print_zone_info(CharData *ch);
char *find_exdesc(const char *word, const ExtraDescription::shared_ptr &list);
const char *show_obj_to_char(ObjData *object, CharData *ch, int mode, int show_state, int how);
void obj_info(CharData *ch, ObjData *obj, char buf[kMaxStringLength]);
void print_zone_info(CharData *ch);
const char *print_obj_state(int tm_pct);
void diag_char_to_char(CharData *i, CharData *ch);

#endif //BYLINS_SRC_GAME_MECHANICS_SIGHT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
