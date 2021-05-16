/***************************************************************************
*  OasisOLC - olc.cpp 		                                           *
*    				                                           *
*  Copyright 1996 Harvey Gilpin.                                           *
* 				     					   *
*  $Author$                                                         *
*  $Date$                                            *
*  $Revision$                                                       *
***************************************************************************/

#include "olc.h"

#include "object.prototypes.hpp"
#include "obj.hpp"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "dg_script/dg_olc.h"
#include "screen.h"
#include "item.creation.hpp"
#include "im.h"
#include "privilege.hpp"
#include "chars/character.h"
#include "room.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "zone.table.hpp"

#include <vector>

// * External data structures.
extern CHAR_DATA *mob_proto;

extern DESCRIPTOR_DATA *descriptor_list;

// * External functions.
void zedit_setup(DESCRIPTOR_DATA *d, int room_num);
void zedit_save_to_disk(int zone);
int zedit_new_zone(CHAR_DATA *ch, int new_zone);
void medit_setup(DESCRIPTOR_DATA *d, int rmob_num);
void medit_save_to_disk(int zone);
void redit_setup(DESCRIPTOR_DATA *d, int rroom_num);
void redit_save_to_disk(int zone);
void oedit_setup(DESCRIPTOR_DATA *d, int robj_num);
void oedit_save_to_disk(int zone);
void sedit_setup_new(DESCRIPTOR_DATA *d);
void sedit_setup_existing(DESCRIPTOR_DATA *d, int robj_num);
void room_free(ROOM_DATA *room);
void medit_mobile_free(CHAR_DATA *mob);
void trigedit_setup_new(DESCRIPTOR_DATA *d);
void trigedit_setup_existing(DESCRIPTOR_DATA *d, int rtrg_num);
int real_trigger(int vnum);
void dg_olc_script_free(DESCRIPTOR_DATA *d);

// Internal function prototypes.
int real_zone(int number);
void olc_saveinfo(CHAR_DATA *ch);

// global data
const char *save_info_msg[5] = {"Rooms", "Objects", "Zone info", "Mobiles", "Shops"};
const char *nrm, *grn, *cyn, *yel, *iyel, *ired;
struct olc_save_info *olc_save_list = NULL;

struct olc_scmd_data {
  const char *text;
  int con_type;
};

struct olc_scmd_data olc_scmd_info[5] =
    {
        {"room", CON_REDIT},
        {"object", CON_OEDIT},
        {"room", CON_ZEDIT},
        {"mobile", CON_MEDIT},
        {"trigger", CON_TRIGEDIT}
    };

olc_data::olc_data()
    : mode(0),
      zone_num(0),
      number(0),
      value(0),
      total_mprogs(0),
      bitmask(0),
      mob(0),
      room(0),
      obj(0),
      zone(0),
      desc(0),
      mrec(0),
#if defined(OASIS_MPROG)
    mprog(0),
    mprogl(0),
#endif
      trig(0),
      script_mode(0),
      trigger_position(0),
      item_type(0),
      script(0),
      storage(0) {

}

//------------------------------------------------------------

/*
 * Exported ACMD do_olc function.
 *
 * This function is the OLC interface.  It deals with all the
 * generic OLC stuff, then passes control to the sub-olc sections.
 */

void do_olc(CHAR_DATA *ch, char *argument, int cmd, int subcmd) {
  int number = -1, save = 0, real_num;
  bool lock = 0, unlock = 0;
  DESCRIPTOR_DATA *d;

  // * No screwing around as a mobile.
  if (IS_NPC(ch))
    return;

  if (subcmd == SCMD_OLC_SAVEINFO) {
    olc_saveinfo(ch);
    return;
  }

  // * Parse any arguments.
  two_arguments(argument, buf1, buf2);
  if (!*buf1)        // No argument given.
  {
    switch (subcmd) {
      case SCMD_OLC_ZEDIT:
      case SCMD_OLC_REDIT: number = world[ch->in_room]->number;
        break;
      case SCMD_OLC_TRIGEDIT:
      case SCMD_OLC_OEDIT:
      case SCMD_OLC_MEDIT: sprintf(buf, "Укажите %s VNUM для редактирования.\r\n", olc_scmd_info[subcmd].text);
        send_to_char(buf, ch);
        return;
    }
  } else if (!a_isdigit(*buf1)) {
    if (strn_cmp("save", buf1, 4) == 0
        || (lock = !strn_cmp("lock", buf1, 4)) == TRUE
        || (unlock = !strn_cmp("unlock", buf1, 6)) == TRUE) {
      if (!*buf2) {
        if (GET_OLC_ZONE(ch)) {
          save = 1;
          number = (GET_OLC_ZONE(ch) * 100);
        } else {
          send_to_char("И какую зону писать?\r\n", ch);
          return;
        }
      } else {
        save = 1;
        number = atoi(buf2) * 100;
      }
    } else if (subcmd == SCMD_OLC_ZEDIT && (GET_LEVEL(ch) >= LVL_BUILDER || PRF_FLAGGED(ch, PRF_CODERINFO))) {
      send_to_char("Создание новых зон отключено.\r\n", ch);
      return;
      /*
                if ((strn_cmp("new", buf1, 3) == 0) && *buf2)
                    zedit_new_zone(ch, atoi(buf2));
                else
                    send_to_char("Укажите номер новой зоны.\r\n", ch);
                return;
      */
    } else {
      send_to_char("Уточните, что вы хотите делать!\r\n", ch);
      return;
    }
  }
  // * If a numeric argument was given, get it.
  if (number == -1) {
    number = atoi(buf1);
  }

  // * Check that whatever it is isn't already being edited.
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected == olc_scmd_info[subcmd].con_type) {
      if (d->olc && OLC_NUM(d) == number) {
        sprintf(buf, "%s в настоящий момент редактируется %s.\r\n",
                olc_scmd_info[subcmd].text, GET_PAD(d->character, 4));
        send_to_char(buf, ch);
        return;
      }
    }
  }
  d = ch->desc;

  // лок/анлок редактирования зон только 34м и по привилегии
  if ((lock || unlock) && !IS_IMPL(ch) && !Privilege::check_flag(d->character.get(), Privilege::FULLZEDIT)) {
    send_to_char("Вы не можете использовать эту команду.\r\n", ch);
    return;
  }

  // * Give descriptor an OLC struct.
  d->olc = new olc_data;

  // * Find the zone.
  if ((OLC_ZNUM(d) = real_zone(number)) == -1) {
    send_to_char("Звыняйтэ, такойи зоны нэмае.\r\n", ch);
    delete d->olc;
    return;
  }

  if (lock) {
    zone_table[OLC_ZNUM(d)].locked = TRUE;
    send_to_char("Защищаю зону от записи.\r\n", ch);
    sprintf(buf, "(GC) %s has locked zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
    olc_log("%s locks zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
    mudlog(buf, LGH, LVL_IMPL, SYSLOG, TRUE);
    zedit_save_to_disk(OLC_ZNUM(d));
    delete d->olc;
    return;
  }

  if (unlock) {
    zone_table[OLC_ZNUM(d)].locked = FALSE;
    send_to_char("Снимаю защиту от записи.\r\n", ch);
    sprintf(buf, "(GC) %s has unlocked zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
    olc_log("%s unlocks zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
    mudlog(buf, LGH, LVL_IMPL, SYSLOG, TRUE);
    zedit_save_to_disk(OLC_ZNUM(d));
    delete d->olc;
    return;
  }
  // Check if zone is protected from editing
  if ((zone_table[OLC_ZNUM(d)].locked) && (GET_LEVEL(ch) != 34)) {
    send_to_char("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
    delete d->olc;
    return;
  }

  // * Everyone but IMPLs can only edit zones they have been assigned.
  if (GET_LEVEL(ch) < LVL_IMPL) {
    if (!Privilege::can_do_priv(ch, std::string(cmd_info[cmd].command), cmd, 0, false)) {
      if (!GET_OLC_ZONE(ch) || (zone_table[OLC_ZNUM(d)].number != GET_OLC_ZONE(ch))) {
        send_to_char("Вам запрещен доступ к сией зоне.\r\n", ch);
        delete d->olc;
        return;
      }
    }
  }

  if (save) {
    const char *type = NULL;
    switch (subcmd) {
      case SCMD_OLC_REDIT: type = "room";
        break;
      case SCMD_OLC_ZEDIT: type = "zone";
        break;
      case SCMD_OLC_MEDIT: type = "mobile";
        break;
      case SCMD_OLC_OEDIT: type = "object";
        break;
    }
    if (!type) {
      send_to_char("Родной(ая,ое), объясни по людски - что записать.\r\n", ch);
      return;
    }
    sprintf(buf, "Saving all %ss in zone %d.\r\n", type, zone_table[OLC_ZNUM(d)].number);
    send_to_char(buf, ch);
    sprintf(buf, "OLC: %s saves %s info for zone %d.", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].number);
    olc_log("%s save %s in Z%d", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].number);
    mudlog(buf, LGH, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), SYSLOG, TRUE);

    switch (subcmd) {
      case SCMD_OLC_REDIT: redit_save_to_disk(OLC_ZNUM(d));
        break;
      case SCMD_OLC_ZEDIT: zedit_save_to_disk(OLC_ZNUM(d));
        break;
      case SCMD_OLC_OEDIT: oedit_save_to_disk(OLC_ZNUM(d));
        break;
      case SCMD_OLC_MEDIT: medit_save_to_disk(OLC_ZNUM(d));
        break;
    }
    delete d->olc;
    return;
  }
  OLC_NUM(d) = number;

  // * Steal player's descriptor start up subcommands.
  switch (subcmd) {
    case SCMD_OLC_TRIGEDIT:
      if ((real_num = real_trigger(number)) >= 0)
        trigedit_setup_existing(d, real_num);
      else
        trigedit_setup_new(d);
      STATE(d) = CON_TRIGEDIT;
      break;
    case SCMD_OLC_REDIT:
      if ((real_num = real_room(number)) != NOWHERE)
        redit_setup(d, real_num);
      else
        redit_setup(d, NOWHERE);
      STATE(d) = CON_REDIT;
      break;
    case SCMD_OLC_ZEDIT:
      if ((real_num = real_room(number)) == NOWHERE) {
        send_to_char("Желательно создать комнату прежде, чем начинаете ее редактировать.\r\n", ch);
        delete d->olc;
        return;
      }
      zedit_setup(d, real_num);
      STATE(d) = CON_ZEDIT;
      break;
    case SCMD_OLC_MEDIT:
      if ((real_num = real_mobile(number)) >= 0)
        medit_setup(d, real_num);
      else
        medit_setup(d, -1);
      STATE(d) = CON_MEDIT;
      break;
    case SCMD_OLC_OEDIT: real_num = real_object(number);
      if (real_num >= 0) {
        oedit_setup(d, real_num);
      } else {
        oedit_setup(d, -1);
      }
      STATE(d) = CON_OEDIT;
      break;
  }
  act("$n по локоть запустил$g руки в глубины Мира и начал$g что-то со скрежетом там поворачивать.",
      TRUE, d->character.get(), 0, 0, TO_ROOM);
  PLR_FLAGS(ch).set(PLR_WRITING);
}

// ------------------------------------------------------------
// Internal utilities
// ------------------------------------------------------------

void olc_saveinfo(CHAR_DATA *ch) {
  struct olc_save_info *entry;

  if (olc_save_list) {
    send_to_char("The following OLC components need saving:-\r\n", ch);
  } else {
    send_to_char("The database is up to date.\r\n", ch);
  }

  for (entry = olc_save_list; entry; entry = entry->next) {
    sprintf(buf, " - %s for zone %d.\r\n", save_info_msg[(int) entry->type], entry->zone);
    send_to_char(buf, ch);
  }
}

// ------------------------------------------------------------
// Exported utilities
// ------------------------------------------------------------

// * Add an entry to the 'to be saved' list.
void olc_add_to_save_list(int zone, byte type) {
  struct olc_save_info *lnew;

  // * Return if it's already in the list.
  for (lnew = olc_save_list; lnew; lnew = lnew->next)
    if ((lnew->zone == zone) && (lnew->type == type))
      return;

  CREATE(lnew, 1);
  lnew->zone = zone;
  lnew->type = type;
  lnew->next = olc_save_list;
  olc_save_list = lnew;
}

// * Remove an entry from the 'to be saved' list.
void olc_remove_from_save_list(int zone, byte type) {
  struct olc_save_info **entry;
  struct olc_save_info *temp;

  for (entry = &olc_save_list; *entry; entry = &(*entry)->next)
    if (((*entry)->zone == zone) && ((*entry)->type == type)) {
      temp = *entry;
      *entry = temp->next;
      free(temp);
      return;
    }
}

/*
* Set the colour string pointers for that which this char will
* see at color level NRM.  Changing the entries here will change
* the colour scheme throughout the OLC. */
void get_char_cols(CHAR_DATA *ch) {
  nrm = CCNRM(ch, C_NRM);
  grn = CCGRN(ch, C_NRM);
  cyn = CCCYN(ch, C_NRM);
  yel = CCYEL(ch, C_NRM);
  iyel = CCIYEL(ch, C_NRM);
  ired = CCIRED(ch, C_NRM);
}

/*
 * This procedure removes the '\r\n' from a string so that it may be
 * saved to a file.  Use it only on buffers, not on the original
 * strings.
 */
void strip_string(char *buffer) {
  char *ptr, *str;

  ptr = buffer;
  str = ptr;

  while ((*str = *ptr)) {
    str++;
    ptr++;
    if (*ptr == '\r')
      ptr++;
  }
}

/*
 * This procdure frees up the strings and/or the structures
 * attatched to a descriptor, sets all flags back to how they
 * should be.
 */
void cleanup_olc(DESCRIPTOR_DATA *d, byte cleanup_type) {
  if (d->olc) {

    // Освободить редактируемый триггер
    if (OLC_TRIG(d)) {
      delete OLC_TRIG(d);
    }

    // Освободить массив данных (похоже, только для триггеров)
    if (OLC_STORAGE(d)) {
      free(OLC_STORAGE(d));
    }

    // Освободить прототип
    if (!OLC_SCRIPT(d).empty()) {
      dg_olc_script_free(d);
    }

    // Освободить комнату
    if (OLC_ROOM(d)) {
      switch (cleanup_type) {
        case CLEANUP_ALL: room_free(OLC_ROOM(d));    // удаляет все содержимое
          // break; - не нужен

          // fall through
        case CLEANUP_STRUCTS: delete OLC_ROOM(d);    // удаляет только оболочку
          break;

        default:    // The caller has screwed up.
          break;
      }
    }
    // Освободить mob
    if (OLC_MOB(d)) {
      switch (cleanup_type) {
        case CLEANUP_ALL: medit_mobile_free(OLC_MOB(d));    // удаляет все содержимое
          delete OLC_MOB(d);    // удаляет только оболочку
          break;
        default:    // The caller has screwed up.
          break;
      }
    }
    // Освободить объект
    if (OLC_OBJ(d)) {
      switch (cleanup_type) {
        case CLEANUP_ALL: delete OLC_OBJ(d);    // удаляет только оболочку
          break;

        default:    // The caller has screwed up.
          break;
      }
    }

    // Освободить зону
    if (OLC_ZONE(d)) {
      free(OLC_ZONE(d)->name);
      zedit_delete_cmdlist((pzcmd) OLC_ZONE(d)->cmd);
      free(OLC_ZONE(d));
    }

    // Restore descriptor playing status.
    if (d->character) {
      PLR_FLAGS(d->character).unset(PLR_WRITING);
      STATE(d) = CON_PLAYING;
      act("$n закончил$g работу и удовлетворенно посмотрел$g в развороченные недра Мироздания.",
          TRUE, d->character.get(), 0, 0, TO_ROOM);
    }
    delete d->olc;
  }
}

void xedit_disp_ing(DESCRIPTOR_DATA *d, int *ping) {
  char str[128];
  int i = 0;

  send_to_char("Ингредиенты:\r\n", d->character.get());
  for (; im_ing_dump(ping, str + 5); ping += 2) {
    sprintf(str, "% 4d", i++);
    str[4] = ' ';
    send_to_char(str, d->character.get());
    send_to_char("\r\n", d->character.get());
  }
  send_to_char("у <номер> - [у]далить ингредиент\r\n"
               "у *       - [у]далить все ингредиенты\r\n"
               "д <ингр>  - [д]обавить ингредиенты\r\n" "в         - [в]ыход\r\n" "Команда> ", d->character.get());
}

int xparse_ing(DESCRIPTOR_DATA * /*d*/, int **pping, char *arg) {
  switch (*arg) {
    case 'у':
    case 'У': ++arg;
      skip_spaces(&arg);
      if (arg[0] == '*') {
        if (*pping)
          free(*pping);
        *pping = NULL;
      } else if (a_isdigit(arg[0])) {
        im_extract_ing(pping, atoi(arg));
      }
      break;
    case 'д':
    case 'Д': im_parse(pping, arg + 1);
      break;
    case 'в':
    case 'В': return 0;
  }
  return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
