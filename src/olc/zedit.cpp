/************************************************************************
 *  OasisOLC - zedit.c						v1.5	*
 *									*
 *  Copyright 1996 Harvey Gilpin.					*
 ************************************************************************/

#include "object.prototypes.hpp"
#include "obj.hpp"
#include "comm.h"
#include "db.h"
#include "olc.h"
#include "dg_script/dg_scripts.h"
#include "chars/character.h"
#include "room.hpp"
#include "help.hpp"
#include "zone.table.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <vector>

// * External data structures.
extern CHAR_DATA *mob_proto;
extern INDEX_DATA *mob_index;
extern char const *equipment_types[];
extern char const *dirs[];

//MZ.load
extern struct zone_type *zone_types;
//-MZ.load
//------------------------------------------------------------------------

// * Function prototypes.
void zedit_disp_menu(DESCRIPTOR_DATA *d);
void zedit_setup(DESCRIPTOR_DATA *d, int room_num);
//void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
//void remove_cmd_from_list(struct reset_com **list, int pos);
int delete_command(DESCRIPTOR_DATA *d, int pos);
int move_command(DESCRIPTOR_DATA *d, int src, int dst);
int new_command(DESCRIPTOR_DATA *d, int pos);
int start_change_command(DESCRIPTOR_DATA *d, int pos);
void zedit_disp_comtype(DESCRIPTOR_DATA *d);
void zedit_disp_arg1(DESCRIPTOR_DATA *d);
void zedit_disp_arg2(DESCRIPTOR_DATA *d);
void zedit_disp_arg3(DESCRIPTOR_DATA *d);
void zedit_disp_arg4(DESCRIPTOR_DATA *d);
void zedit_save_internally(DESCRIPTOR_DATA *d);
void zedit_save_to_disk(int zone_num);
void zedit_create_index(int znum, char *type);
void zedit_new_zone(CHAR_DATA *ch, int vzone_num);

void renum_single_table(int zone);
int is_number(const char *str);
//------------------------------------------------------------------------

// Количество команд для отображения без фильтра
#define        CMD_PAGE_SIZE        16

// * Nasty internal macros to clean up the code.
#define ZCMD        (zone_table[OLC_ZNUM(d)].cmd[subcmd])
#define MYCMD        (OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d)    (OLC_ZONE(d)->cmd[OLC_VAL(d)])

#define SEEK_CMD(d) zedit_seek_cmd( (pzcmd)OLC_ZONE(d)->cmd, OLC_VAL(d) )

#define TRANS_MOB(field)  item->cmd.field = mob_index[item->cmd.field].vnum
#define TRANS_OBJ(field)  item->cmd.field = obj_proto[item->cmd.field]->get_vnum()
#define TRANS_ROOM(field) item->cmd.field = world[item->cmd.field]->number

//------------------------------------------------------------------------

// * Utility functions.

//------------------------------------------------------------------------

int is_signednumber(const char *str) {
  if (*str == '+' || *str == '-')
    ++str;
  while (*str)
    if (!a_isdigit(*(str++)))
      return (0);

  return (1);
}

// Размер кольцевого буфера
int zedit_count_cmdlist(pzcmd head) {
  pzcmd item;
  int n;
  for (n = 1, item = head->next; item != head; item = item->next, ++n);
  return n;
}

// Построение циклического буфера с трансляцией в VNUM
pzcmd zedit_build_cmdlist(DESCRIPTOR_DATA *d) {
  pzcmd head, item;
  int subcmd;

  CREATE(head, 1);
  head->next = head->prev = head;
  head->cmd.command = 'S';

  for (subcmd = 0; ZCMD.command != 'S'; ++subcmd) {
    if (ZCMD.command == '*')
      continue;

    CREATE(item, 1);
    item->cmd = ZCMD;    // копирование команды

    switch (ZCMD.command) {
      case 'M': TRANS_MOB(arg1);
        TRANS_ROOM(arg3);
        break;

      case 'F': TRANS_ROOM(arg1);
        TRANS_MOB(arg2);
        TRANS_MOB(arg3);
        break;

      case 'Q': TRANS_MOB(arg1);
        break;

      case 'O': TRANS_OBJ(arg1);
        TRANS_ROOM(arg3);
        break;

      case 'P': TRANS_OBJ(arg1);
        TRANS_OBJ(arg3);
        break;

      case 'G': TRANS_OBJ(arg1);
        break;

      case 'E': TRANS_OBJ(arg1);
        break;

      case 'R': TRANS_ROOM(arg1);
        TRANS_OBJ(arg2);
        break;

      case 'D': TRANS_ROOM(arg1);
        break;

      case 'T':
        // arg2 не преобразовываю, хотя может надо :)
        if (item->cmd.arg1 == WLD_TRIGGER) {
          TRANS_ROOM(arg3);
        }
        break;

      case 'V':
        if (item->cmd.arg1 == WLD_TRIGGER) {
          TRANS_ROOM(arg2);
        }
        item->cmd.sarg1 = str_dup(item->cmd.sarg1);
        item->cmd.sarg2 = str_dup(item->cmd.sarg2);
        break;

      default: free(item);
        continue;
    }

    // Добавить item в конец буфера
    item->next = head;
    item->prev = head->prev;
    item->next->prev = item->prev->next = item;

  }

  return head;
}

void zedit_free_command(pzcmd item) {
  if (item->cmd.command == 'V') {
    free(item->cmd.sarg1);
    free(item->cmd.sarg2);
  }
  item->prev->next = item->next;
  item->next->prev = item->prev;
  free(item);
}

void zedit_delete_cmdlist(pzcmd head) {
  while (head->next != head)
    zedit_free_command(head->next);
  zedit_free_command(head);
}

pzcmd zedit_seek_cmd(pzcmd head, int pos) {
  pzcmd item;
  int n;
  for (n = 0, item = head->next; item != head && n != pos; ++n, item = item->next);
  return n == pos ? item : NULL;
}

// Удаляю команду номер pos
int delete_command(DESCRIPTOR_DATA *d, int pos) {
  pzcmd head, item;
  head = (pzcmd) OLC_ZONE(d)->cmd;
  item = zedit_seek_cmd(head, pos);
  if (!item || item == head)
    return 0;    // неправильный номер
  zedit_free_command(item);
  return 1;
}

// Перемещаю команду src на позицию dst
int move_command(DESCRIPTOR_DATA *d, int src, int dst) {
  pzcmd head, isrc, idst;
  if (src == dst)
    return 0;    // нечего делать
  head = (pzcmd) OLC_ZONE(d)->cmd;
  isrc = zedit_seek_cmd(head, src);
  idst = zedit_seek_cmd(head, dst);
  if (!isrc || !idst || isrc == head)
    return 0;    // неправильный номер
  // Вычленить isrc
  isrc->prev->next = isrc->next;
  isrc->next->prev = isrc->prev;
  // Добавить isrc перед idst
  isrc->prev = idst->prev;
  isrc->next = idst;
  isrc->prev->next = isrc;
  isrc->next->prev = isrc;
  return 1;
}

int start_change_command(DESCRIPTOR_DATA *d, int pos) {
  pzcmd head, item;
  head = (pzcmd) OLC_ZONE(d)->cmd;
  item = zedit_seek_cmd(head, pos);
  if (!item || item == head)
    return 0;    // неправильный номер
  OLC_VAL(d) = pos;
  return 1;
}

int new_command(DESCRIPTOR_DATA *d, int pos) {
  pzcmd head, item;
  head = (pzcmd) OLC_ZONE(d)->cmd;
  item = zedit_seek_cmd(head, pos);
  if (!item)
    return 0;    // очень большой номер
  // новый элемент добавляю перед item
  CREATE(head, 1);
  head->cmd.command = '*';
  head->cmd.if_flag = 0;
  head->cmd.arg1 = head->cmd.arg2 = head->cmd.arg3 = head->cmd.arg4 = -1;
  head->cmd.sarg1 = head->cmd.sarg2 = NULL;
  head->prev = item->prev;
  head->next = item;
  head->prev->next = head;
  head->next->prev = head;
  return 1;
}

// Процедура пролистывает список команд
// Правильность не отслеживается, корректируется при выводе на экран
void zedit_scroll_list(DESCRIPTOR_DATA *d, char *arg) {
  pzcmd head;
  int last;
  long pos;
  pos = d->olc->bitmask & ~OLC_BM_SHOWALLCMD;
  head = (pzcmd) OLC_ZONE(d)->cmd;
  last = zedit_count_cmdlist(head);
  while (1) {
    switch (*arg++) {
      case '1': pos = last;
        continue;
      case '2': ++pos;
        continue;
      case '3': pos += CMD_PAGE_SIZE - 1;
        continue;
      case '7': pos = 0;
        continue;
      case '8': --pos;
        continue;
      case '9': pos -= CMD_PAGE_SIZE - 1;
        continue;
      default: break;
    }
    break;
  }
  if (pos < 0)
    pos = 0;
  if (pos > last)
    pos = last;
  d->olc->bitmask &= OLC_BM_SHOWALLCMD;
  d->olc->bitmask |= pos;
}

void zedit_setup(DESCRIPTOR_DATA *d, int/* room_num*/) {
  ZoneData *zone;
  int i;

  // Allocate one scratch zone structure. //
  CREATE(zone, 1);
  if (zone_table[OLC_ZNUM(d)].typeA_count) {
    CREATE(zone->typeA_list, zone_table[OLC_ZNUM(d)].typeA_count);
  }
  if (zone_table[OLC_ZNUM(d)].typeB_count) {
    CREATE(zone->typeB_list, zone_table[OLC_ZNUM(d)].typeB_count);
  }

  // Copy all the zone header information over. //
  zone->name = str_dup(zone_table[OLC_ZNUM(d)].name);
  zone->comment = str_dup(zone_table[OLC_ZNUM(d)].comment);
  zone->location = zone_table[OLC_ZNUM(d)].location;
  zone->author = zone_table[OLC_ZNUM(d)].author;
  zone->description = zone_table[OLC_ZNUM(d)].description;
//MZ.load
  zone->level = zone_table[OLC_ZNUM(d)].level;
  zone->type = zone_table[OLC_ZNUM(d)].type;
//-MZ.load
  zone->lifespan = zone_table[OLC_ZNUM(d)].lifespan;
  zone->top = zone_table[OLC_ZNUM(d)].top;
  zone->reset_mode = zone_table[OLC_ZNUM(d)].reset_mode;
  zone->reset_idle = zone_table[OLC_ZNUM(d)].reset_idle;
  zone->typeA_count = zone_table[OLC_ZNUM(d)].typeA_count;
  for (i = 0; i < zone->typeA_count; i++)
    zone->typeA_list[i] = zone_table[OLC_ZNUM(d)].typeA_list[i];
  zone->typeB_count = zone_table[OLC_ZNUM(d)].typeB_count;
  for (i = 0; i < zone->typeB_count; i++)
    zone->typeB_list[i] = zone_table[OLC_ZNUM(d)].typeB_list[i];
  zone->under_construction = zone_table[OLC_ZNUM(d)].under_construction;
  zone->group = zone_table[OLC_ZNUM(d)].group;

  // The remaining fields are used as a 'has been modified' flag //
  zone->number = 0;    // Header information has changed.      //
  zone->age = 0;        // The commands have changed.           //

  // Переменная используется не по назначению
  zone->cmd = (struct reset_com *) zedit_build_cmdlist(d);

  OLC_ZONE(d) = zone;
  d->olc->bitmask = 0;

  zedit_disp_menu(d);
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------

// * Save all the information in the player's temporary buffer back into the current zone table.
void zedit_save_internally(DESCRIPTOR_DATA *d) {
  int subcmd;
  int count, i;
  pzcmd head, item;

  // Удалить старую таблицу команд
  for (subcmd = 0; ZCMD.command != 'S'; ++subcmd) {
    if (ZCMD.command == 'V') {
      free(ZCMD.sarg1);
      free(ZCMD.sarg2);
    }
  }
  free(zone_table[OLC_ZNUM(d)].cmd);

  head = (pzcmd) OLC_ZONE(d)->cmd;
  count = zedit_count_cmdlist(head);
  CREATE(zone_table[OLC_ZNUM(d)].cmd, count);
  // Перенос команд обратно с трансляцией в RNUM

  for (subcmd = 0, item = head->next; item != head; item = item->next, ++subcmd) {
    ZCMD = item->cmd;    // копирование команды
  }
  ZCMD.command = 'S';
  renum_single_table(OLC_ZNUM(d));

  // Finally, if zone headers have been changed, copy over
  if (OLC_ZONE(d)->number) {
    free(zone_table[OLC_ZNUM(d)].name);
    zone_table[OLC_ZNUM(d)].name = str_dup(OLC_ZONE(d)->name);

    if (zone_table[OLC_ZNUM(d)].comment) {
      free(zone_table[OLC_ZNUM(d)].comment);
    }
    zone_table[OLC_ZNUM(d)].comment = str_dup(OLC_ZONE(d)->comment);
    zone_table[OLC_ZNUM(d)].location = OLC_ZONE(d)->location;
    zone_table[OLC_ZNUM(d)].author = OLC_ZONE(d)->author;
    zone_table[OLC_ZNUM(d)].description = OLC_ZONE(d)->description;

//MZ.load
    zone_table[OLC_ZNUM(d)].level = OLC_ZONE(d)->level;
    zone_table[OLC_ZNUM(d)].type = OLC_ZONE(d)->type;
//-MZ.load
    zone_table[OLC_ZNUM(d)].top = OLC_ZONE(d)->top;
    zone_table[OLC_ZNUM(d)].reset_mode = OLC_ZONE(d)->reset_mode;
    zone_table[OLC_ZNUM(d)].lifespan = OLC_ZONE(d)->lifespan;
    zone_table[OLC_ZNUM(d)].reset_idle = OLC_ZONE(d)->reset_idle;
    zone_table[OLC_ZNUM(d)].typeA_count = OLC_ZONE(d)->typeA_count;
    zone_table[OLC_ZNUM(d)].typeB_count = OLC_ZONE(d)->typeB_count;
    free(zone_table[OLC_ZNUM(d)].typeA_list);
    if (zone_table[OLC_ZNUM(d)].typeA_count) {
      CREATE(zone_table[OLC_ZNUM(d)].typeA_list, zone_table[OLC_ZNUM(d)].typeA_count);
    }
    for (i = 0; i < zone_table[OLC_ZNUM(d)].typeA_count; i++)
      zone_table[OLC_ZNUM(d)].typeA_list[i] = OLC_ZONE(d)->typeA_list[i];
    free(zone_table[OLC_ZNUM(d)].typeB_list);
    if (zone_table[OLC_ZNUM(d)].typeB_count) {
      CREATE(zone_table[OLC_ZNUM(d)].typeB_list, zone_table[OLC_ZNUM(d)].typeB_count);
    }
    for (i = 0; i < zone_table[OLC_ZNUM(d)].typeB_count; i++)
      zone_table[OLC_ZNUM(d)].typeB_list[i] = OLC_ZONE(d)->typeB_list[i];
    free(zone_table[OLC_ZNUM(d)].typeB_flag);
    if (zone_table[OLC_ZNUM(d)].typeB_count) {
      CREATE(zone_table[OLC_ZNUM(d)].typeB_flag, zone_table[OLC_ZNUM(d)].typeB_count);
    }
    zone_table[OLC_ZNUM(d)].under_construction = OLC_ZONE(d)->under_construction;
    zone_table[OLC_ZNUM(d)].locked = OLC_ZONE(d)->locked;
    if (zone_table[OLC_ZNUM(d)].group != OLC_ZONE(d)->group) {
      zone_table[OLC_ZNUM(d)].group = OLC_ZONE(d)->group;
      HelpSystem::reload(HelpSystem::STATIC);
    }
  }

  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ZONE);
}

//------------------------------------------------------------------------

/*
 * Save all the zone_table for this zone to disk.  This function now
 * writes simple comments in the form of (<name>) to each record.  A
 * header for each field is also there.
 */
#undef  ZCMD
#define ZCMD    (zone_table[zone_num].cmd[subcmd])
void zedit_save_to_disk(int zone_num) {
  int subcmd, arg1 = -1, arg2 = -1, arg3 = -1, arg4 = -1, i;
  char fname[64];
  const char *comment = NULL;
  FILE *zfile;

  sprintf(fname, "%s/%d.new", ZON_PREFIX, zone_table[zone_num].number);
  if (!(zfile = fopen(fname, "w"))) {
    sprintf(buf, "SYSERR: OLC: zedit_save_to_disk:  Can't write zone %d.", zone_table[zone_num].number);
    mudlog(buf, BRF, LVL_BUILDER, SYSLOG, TRUE);
    return;
  }

  // * Print zone header to file
  fprintf(zfile, "#%d\n" "%s~\n",
          zone_table[zone_num].number, (zone_table[zone_num].name && *zone_table[zone_num].name)
                                       ? zone_table[zone_num].name : "неопределено");
  if (zone_table[zone_num].comment && *zone_table[zone_num].comment) {
    fprintf(zfile, "^%s~\n", zone_table[zone_num].comment);
  }
  if (zone_table[zone_num].location && *zone_table[zone_num].location) {
    fprintf(zfile, "&%s~\n", zone_table[zone_num].location);
  }
  if (zone_table[zone_num].author && *zone_table[zone_num].author) {
    fprintf(zfile, "!%s~\n", zone_table[zone_num].author);
  }
  if (zone_table[zone_num].description && *zone_table[zone_num].description) {
    fprintf(zfile, "$%s~\n", zone_table[zone_num].description);
  }

  fprintf(zfile, "#%d %d %d\n" "%d %d %d %d %s %s\n",
          zone_table[zone_num].level,
          zone_table[zone_num].type,
          zone_table[zone_num].group,
          zone_table[zone_num].top,
          zone_table[zone_num].lifespan,
          zone_table[zone_num].reset_mode,
          zone_table[zone_num].reset_idle,
          zone_table[zone_num].under_construction ? "test" : "*",
          zone_table[zone_num].locked ? "locked" : "");

#if defined(ZEDIT_HELP_IN_FILE)
  fprintf(zfile, "* Field #1    Field #3   Field #4  Field #5   Field #6\n"
          "* A (zone type A) Zone-Vnum\n"
          "* B (zone type B) Zone-Vnum\n"
          "* M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum  Room-Max\n"
          "* F (Follow)  Room-Vnum  LMob-Vnum FMob->Vnum\n"
          "* O (Object)  Obj-Vnum   Wld-Max   Room-Vnum Load-Perc\n"
          "* G (Give)    Obj-Vnum   Wld-Max   Unused Load-Perc\n"
          "* E (Equip)   Obj-Vnum   Wld-Max   EQ-Position Load-Perc\n"
          "* P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum Load-Perc\n"
          "* D (Door)    Room-Vnum  Door-Dir  Door-State\n"
          "* R (Remove)  Room-Vnum  Obj-Vnum  Unused\n"
          "* Q (Purge)   Mob-Vnum   Unused    Unused\n"
          "* T (Trigger) Trig-Type  Trig-Vnum Room-Vnum(if need)\n"
          "* V (Variable)Trig-Type  Room-Vnum Context  Varname Value\n");
#endif

  for (i = 0; i < zone_table[zone_num].typeA_count; i++)
    fprintf(zfile, "A %d\n", zone_table[zone_num].typeA_list[i]);
  for (i = 0; i < zone_table[zone_num].typeB_count; i++)
    fprintf(zfile, "B %d\n", zone_table[zone_num].typeB_list[i]);

  for (subcmd = 0; ZCMD.command != 'S'; subcmd++) {
    arg1 = arg2 = arg3 = arg4 = -1;
    switch (ZCMD.command) {
      case 'M': arg1 = mob_index[ZCMD.arg1].vnum;
        arg2 = ZCMD.arg2;
        arg3 = world[ZCMD.arg3]->number;
        arg4 = ZCMD.arg4;
        comment = mob_proto[ZCMD.arg1].get_npc_name().c_str();
        break;

      case 'F': arg1 = world[ZCMD.arg1]->number;
        arg2 = mob_index[ZCMD.arg2].vnum;
        arg3 = mob_index[ZCMD.arg3].vnum;
        comment = mob_proto[ZCMD.arg2].get_npc_name().c_str();
        break;

      case 'O': arg1 = obj_proto[ZCMD.arg1]->get_vnum();
        arg2 = ZCMD.arg2;
        arg3 = world[ZCMD.arg3]->number;
        arg4 = ZCMD.arg4;
        comment = obj_proto[ZCMD.arg1]->get_short_description().c_str();
        break;

      case 'G': arg1 = obj_proto[ZCMD.arg1]->get_vnum();
        arg2 = ZCMD.arg2;
        arg3 = -1;
        arg4 = ZCMD.arg4;
        comment = obj_proto[ZCMD.arg1]->get_short_description().c_str();
        break;

      case 'E': arg1 = obj_proto[ZCMD.arg1]->get_vnum();
        arg2 = ZCMD.arg2;
        arg3 = ZCMD.arg3;
        arg4 = ZCMD.arg4;
        comment = obj_proto[ZCMD.arg1]->get_short_description().c_str();
        break;

      case 'Q': arg1 = mob_index[ZCMD.arg1].vnum;
        arg2 = -1;
        arg3 = -1;
        comment = mob_proto[ZCMD.arg1].get_npc_name().c_str();
        break;

      case 'P': arg1 = obj_proto[ZCMD.arg1]->get_vnum();
        arg2 = ZCMD.arg2;
        arg3 = obj_proto[ZCMD.arg3]->get_vnum();
        arg4 = ZCMD.arg4;
        comment = obj_proto[ZCMD.arg1]->get_short_description().c_str();
        break;

      case 'D': arg1 = world[ZCMD.arg1]->number;
        arg2 = ZCMD.arg2;
        arg3 = ZCMD.arg3;
        comment = world[ZCMD.arg1]->name;
        break;

      case 'R': arg1 = world[ZCMD.arg1]->number;
        arg2 = obj_proto[ZCMD.arg2]->get_vnum();
        comment = obj_proto[ZCMD.arg2]->get_short_description().c_str();
        arg3 = -1;
        break;

      case 'T': arg1 = ZCMD.arg1;    // trigger type
        arg2 = ZCMD.arg2;
        if (arg1 == WLD_TRIGGER) {
          arg3 = world[ZCMD.arg3]->number;
          comment = world[ZCMD.arg3]->name;
        }
        break;

      case 'V': arg1 = ZCMD.arg1;    // trigger type
        arg2 = ZCMD.arg2;    // context
        break;

      case '*':
        // * Invalid commands are replaced with '*' - Ignore them.
        continue;

      default: sprintf(buf, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '%c' - NOT saving", ZCMD.command);
        mudlog(buf, BRF, LVL_BUILDER, SYSLOG, TRUE);
        continue;
    }

    if (ZCMD.command != 'V') {
      fprintf(zfile, "%c %d %d %d %d %d\t(%s)\n",
              ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, arg4, comment);
    } else {
      fprintf(zfile, "%c %d %d %d %d %s %s\n",
              ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, ZCMD.sarg1, ZCMD.sarg2);
    }
  }
  fprintf(zfile, "S\n$\n");
  fclose(zfile);
  sprintf(buf2, "%s/%d.zon", ZON_PREFIX, zone_table[zone_num].number);
  // * We're fubar'd if we crash between the two lines below.
  remove(buf2);
  rename(fname, buf2);

  olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_ZONE);
}

// *************************************************************************
// * Menu functions                                                        *
// *************************************************************************

namespace {

enum { MOB_NAME, OBJ_NAME, ROOM_NAME, TRIG_NAME };

/**
* Замена макросов:
* #define MOB_NAME(vnum) (rnum=real_mobile(vnum))>=0?mob_proto[rnum].player_data.short_descr:"???"
* #define OBJ_NAME(vnum) (rnum=real_object(vnum))>=0?obj_proto[rnum]->short_description:"???"
* #define ROOM_NAME(vnum) (rnum=real_room(vnum))>=0?world[rnum]->name:"???"
* #define TRIG_NAME(vnum) (rnum=real_trigger(vnum))>=0?trig_index[rnum]->proto->name:"???"
* т.к. gcc 4.x на такие конструкции косо смотрит и правильно делает
* \param vnum - номер (внум) моба/объекта/комнаты/триггера
* \param type - тип номера, чье имя возвращаем
* \return имя или "???" если такого не существует
*/
const char *name_by_vnum(int vnum, int type) {
  int rnum;

  switch (type) {
    case MOB_NAME: rnum = real_mobile(vnum);
      if (rnum >= 0) {
        return mob_proto[rnum].get_npc_name().c_str();
      }
      break;

    case OBJ_NAME: rnum = real_object(vnum);
      if (rnum >= 0) {
        return obj_proto[rnum]->get_short_description().c_str();
      }
      break;

    case ROOM_NAME: rnum = real_room(vnum);
      if (rnum >= 0) {
        return world[rnum]->name;
      }
      break;

    case TRIG_NAME: rnum = real_trigger(vnum);
      if (rnum >= 0) {
        return trig_index[rnum]->proto->get_name().c_str();
      }
      break;

    default: log("SYSERROR : bad type '%d' (%s %s %d)", type, __FILE__, __func__, __LINE__);
  }

  return "???";
}

} // no-name namespace

const char *if_flag_msg[] =
    {
        "",
        "в случае успеха ",
        "не изменяя флаг, ",
        "в случае успеха, не изменяя флаг, "
    };

const char *if_flag_text(int if_flag) {
  return if_flag_msg[if_flag & 3];
}

void zedit_disp_commands(DESCRIPTOR_DATA *d, char *buf) {
  pzcmd head, item;
  int room, counter = 0;
  int hl = 0;        // требуется ли подсветка
  int rnum = 0;
  int show_all = d->olc->bitmask & OLC_BM_SHOWALLCMD;
  int start = d->olc->bitmask & ~OLC_BM_SHOWALLCMD, stop;

  room = OLC_NUM(d);
  head = (pzcmd) OLC_ZONE(d)->cmd;

  // Проверка допустимости индекса start
  stop = zedit_count_cmdlist(head);    // количество элементов
  sprintf(buf1, "[Command list (0:%d)]\r\n", stop - 1);
  strcat(buf, buf1);
  if (show_all) {
    if (start > stop - CMD_PAGE_SIZE)
      start = stop - CMD_PAGE_SIZE;
    if (start < 0)
      start = 0;
    stop = start + CMD_PAGE_SIZE;
    // Выводить команды { start <= n < stop }
    d->olc->bitmask = OLC_BM_SHOWALLCMD | start;
  }

  // Вывод всех команд зоны
  // Подсветка команд, относящихся к данной комнате
  for (item = head->next; item != head; item = item->next, ++counter) {
    // Разбор аргументов, выяснение подсветки
    // if_flag и подсветка с номером - ниже
    switch (item->cmd.command) {
      case 'M':
        sprintf(buf2,
                "загрузить моба %d [%s] в комнату %d [%s], Max(игра/комната) : %d/%d",
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, MOB_NAME),
                item->cmd.arg3, name_by_vnum(item->cmd.arg3, ROOM_NAME), item->cmd.arg2, item->cmd.arg4);
        hl = (item->cmd.arg3 == room);
        break;

      case 'F':
        sprintf(buf2,
                "%d [%s] следует за %d [%s] в комнате %d [%s]",
                item->cmd.arg3, name_by_vnum(item->cmd.arg3, MOB_NAME),
                item->cmd.arg2, name_by_vnum(item->cmd.arg2, MOB_NAME),
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, ROOM_NAME));
        hl = (item->cmd.arg1 == room);
        break;

      case 'Q': sprintf(buf2, "убрать всех мобов %d [%s]", item->cmd.arg1, name_by_vnum(item->cmd.arg1, MOB_NAME));
        hl = 0;
        break;

      case 'O':
        sprintf(buf2,
                "загрузить объект %d [%s] в комнату %d [%s], Load%% %d",
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, OBJ_NAME),
                item->cmd.arg3, name_by_vnum(item->cmd.arg3, ROOM_NAME), item->cmd.arg4);
        hl = (item->cmd.arg3 == room);
        break;

      case 'P':
        sprintf(buf2,
                "поместить %d [%s] в %d [%s], Load%% %d",
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, OBJ_NAME),
                item->cmd.arg3, name_by_vnum(item->cmd.arg3, OBJ_NAME), item->cmd.arg4);
        // hl - не изменяется
        break;

      case 'G':
        sprintf(buf2,
                "дать %d [%s], Load%% %d",
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, OBJ_NAME), item->cmd.arg4);
        // hl - не изменяется
        break;

      case 'E': {
        for (rnum = 0; *equipment_types[rnum] != '\n' && rnum != item->cmd.arg3; ++rnum);
        const char *str = equipment_types[rnum];
        if (*str == '\n')
          str = "???";
        sprintf(buf2,
                "экипировать %d [%s], %s, Load%% %d",
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, OBJ_NAME), str, item->cmd.arg4);
        // hl - не изменяется
        break;
      }
      case 'R':
        sprintf(buf2,
                "удалить %d [%s] из комнаты %d [%s]",
                item->cmd.arg2, name_by_vnum(item->cmd.arg2, OBJ_NAME),
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, ROOM_NAME));
        hl = (item->cmd.arg1 == room);
        break;

      case 'D':
        sprintf(buf2,
                "для комнаты %d [%s] установить выход %s как %s",
                item->cmd.arg1, name_by_vnum(item->cmd.arg1, ROOM_NAME),
                dirs[item->cmd.arg2],
                item->cmd.arg3 == 0 ? "открытый" :
                item->cmd.arg3 == 1 ? "закрытый" :
                item->cmd.arg3 == 2 ? "запертый" :
                item->cmd.arg3 == 3 ? "скрытый" : item->cmd.arg3 == 4 ? "явный" : "неизвестно что");
        hl = (item->cmd.arg1 == room);
        break;

      case 'T': sprintf(buf2, "привязать тригер %d [%s] к ", item->cmd.arg2, name_by_vnum(item->cmd.arg2, TRIG_NAME));
        switch (item->cmd.arg1) {
          case MOB_TRIGGER: strcat(buf2, "мобу");
            break;
          case OBJ_TRIGGER: strcat(buf2, "предмету");
            break;
          case WLD_TRIGGER:
            sprintf(buf2 + strlen(buf2),
                    "комнате %d [%s]", item->cmd.arg3, name_by_vnum(item->cmd.arg3, ROOM_NAME));
            hl = (item->cmd.arg3 == room);
            break;
          default: strcat(buf2, "???");
            break;
        }
        break;

      case 'V':
        switch (item->cmd.arg1) {
          case MOB_TRIGGER: strcpy(buf2, "для моба ");
            break;
          case OBJ_TRIGGER: strcpy(buf2, "для предмета ");
            break;
          case WLD_TRIGGER:
            sprintf(buf2,
                    "для комнаты %d [%s] ",
                    item->cmd.arg2,
                    name_by_vnum(item->cmd.arg2, ROOM_NAME));
            hl = (item->cmd.arg2 == room);
            break;
          default: strcpy(buf2, "для??? ");
            break;
        }
        sprintf(buf2 + strlen(buf2),
                "установить глобальную переменную %s:%d = %s",
                item->cmd.sarg1, item->cmd.arg3, item->cmd.sarg2);
        break;

      default: strcpy(buf2, "<Неизвестная команда>");
        break;

    }

    // Build the display buffer for this command
    if ((show_all && start <= counter && stop > counter) || (!show_all && hl)) {
      snprintf(buf1, MAX_STRING_LENGTH, "%s%d - %s%s%s\r\n", nrm, counter, hl ? iyel : yel,
               if_flag_text(item->cmd.if_flag), buf2);
      strcat(buf, buf1);
    }
  }

  // Последняя команда
  if (!show_all || (start <= counter && stop > counter)) {
    sprintf(buf1, "%s%d - <END>\r\n", nrm, counter);
    strcat(buf, buf1);
  }

  return;
}

// the main menu
void zedit_disp_menu(DESCRIPTOR_DATA *d) {
//	char *buf = (char *) malloc(32 * 1024);
  char *type1_zones = (char *) malloc(1024);
  char *type2_zones = (char *) malloc(1024);
  int i;
  type1_zones[0] = '\0';
  type2_zones[0] = '\0';
  for (i = 0; i < OLC_ZONE(d)->typeA_count; i++)
    sprintf(type1_zones, "%s %d", type1_zones, OLC_ZONE(d)->typeA_list[i]);
  for (i = 0; i < OLC_ZONE(d)->typeB_count; i++)
    sprintf(type2_zones, "%s %d", type2_zones, OLC_ZONE(d)->typeB_list[i]);

  get_char_cols(d->character.get());

  // Menu header
  sprintf(buf,
#if defined(CLEAR_SCREEN)
      "[H[J"
#endif
          "Room number: %s%d%s		Room zone: %s%d\r\n"
          "%sZ%s) Имя зоны         : %s%s\r\n"
          "%sC%s) Комментарий      : %s%s\r\n"
          "%sW%s) Местоположение   : %s%s\r\n"
          "%sO%s) Описание         : %s%s\r\n"
          "%sU%s) Автор зоны       : %s%s\r\n"
          "%sS%s) Уровень зоны     : %s%d  (не учитывается, ставится средний по уровню мобов)\r\n"
          "%sY%s) Тип зоны         : %s%s\r\n"
          "%sL%s) Время жизни      : %s%d minutes\r\n"
          "%sP%s) Макс.комната     : %s%d\r\n"
          "%sR%s) Тип очистки      : %s%s\r\n"
          "%sI%s) Оч. никто не был : %s%s%s\r\n",
          cyn,
          OLC_NUM(d),
          nrm,
          cyn,
          zone_table[OLC_ZNUM(d)].number,
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>",
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->comment ? OLC_ZONE(d)->comment : "<NONE!>",
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->location ? OLC_ZONE(d)->location : "<NONE!>",
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->description ? OLC_ZONE(d)->description : "<NONE!>",
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->author ? OLC_ZONE(d)->author : "<NONE!>",
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->level,
          grn,
          nrm,
          yel,
          zone_types[OLC_ZONE(d)->type].name,
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->lifespan,
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->top,
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->reset_mode ? ((OLC_ZONE(d)->reset_mode == 1) ? "Если в зоне нет игроков."
                                                                    : ((OLC_ZONE(d)->reset_mode == 3)
                                                                       ? "Общая очистка(комплекс зон)."
                                                                       : "Обычная очистка(даже если есть игроки)."))
                                  : "Никогда не очищается",
          grn,
          nrm,
          yel,
          OLC_ZONE(d)->reset_idle ? "Да" : "Нет",
          nrm);
  send_to_char(buf, d->character.get());
  if (OLC_ZONE(d)->reset_mode == 3) {
    snprintf(buf, MAX_STRING_LENGTH, "%sA%s) Зоны первого типа       : %s%s%s\r\n"
                                     "%sB%s) Зоны второго типа       : %s%s%s\r\n",
             grn, nrm, ired, type1_zones, nrm, grn, nrm, grn, type2_zones, nrm);
    send_to_char(buf, d->character.get());
  }
  snprintf(buf, MAX_STRING_LENGTH, "%sT%s) Режим            : %s%s%s\r\n",
           grn, nrm, yel, OLC_ZONE(d)->under_construction ? "ТЕСТИРУЕТСЯ" : "подключена", nrm);
  send_to_char(buf, d->character.get());
  snprintf(buf, MAX_STRING_LENGTH, "%sG%s) Оптимальное число игроков  : %s%d%s\r\n",
           grn, nrm, yel, OLC_ZONE(d)->group, nrm);
  send_to_char(buf, d->character.get());
  // Print the commands into display buffer.
  zedit_disp_commands(d, buf);
  send_to_char(buf, d->character.get());
  // Finish off menu
  if (d->olc->bitmask & OLC_BM_SHOWALLCMD) {
    // Режим отображения всех команд
    sprintf(buf1,
            "%sF%s) Фильтр - ВСЕ КОМАНДЫ   %s8%s) Вверх\r\n"
            "%sN%s) Добавить команду       %s2%s) Вниз\r\n"
            "%sE%s) Редактировать команду  %s9%s) Страница вверх\r\n"
            "%sM%s) Перенести команду      %s3%s) Страница вниз\r\n"
            "%sD%s) Удалить команду        %s7%s) В начало списка\r\n"
            "%sX%s) Выход                  %s1%s) В конец списка\r\n"
            "Ваш выбор : ",
            grn, nrm, grn, nrm,
            grn, nrm, grn, nrm,
            grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  } else {
    // Режим отображения команд комнаты
    sprintf(buf1,
            "%sF%s) Фильтр - КОМАНДЫ КОМНАТЫ\r\n"
            "%sN%s) Добавить команду\r\n"
            "%sE%s) Редактировать команду\r\n"
            "%sM%s) Перенести команду\r\n"
            "%sD%s) Удалить команду\r\n"
            "%sX%s) Выход\r\n" "Ваш выбор : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  }

  send_to_char(buf1, d->character.get());
  free(type1_zones);
  free(type2_zones);

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

//MZ.load
void zedit_disp_type_menu(DESCRIPTOR_DATA *d) {
  int counter, columns = 0;

  get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
  send_to_char("[H[J", d->character);
#endif
  for (counter = 0; *zone_types[counter].name != '\n'; counter++) {
    sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
            zone_types[counter].name, !(++columns % 2) ? "\r\n" : "");
    send_to_char(buf, d->character.get());
  }
  send_to_char("\r\nВыберите тип зоны : ", d->character.get());
}
//-MZ.load

//------------------------------------------------------------------------

// * Print the command type menu and setup response catch.
void zedit_disp_comtype(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);
  get_char_cols(d->character.get());
  send_to_char("\r\n", d->character.get());
  sprintf(buf,
#if defined(CLEAR_SCREEN)
      "[H[J"
#endif
          "%sM%s) Загрузить моба в комнату        %sO%s) Загрузить предмет в комнату\r\n"
          "%sE%s) Экипировать моба                %sG%s) Дать предмет мобу\r\n"
          "%sP%s) Поместить предмет в контейнер   %sD%s) Установить выход\r\n"
          "%sR%s) Удалить предмет из комнаты      %sQ%s) Удалить всех мобов данного типа\r\n"
          "%sF%s) Создать цепочку последователей\r\n"
          "%sT%s) Назначить тригер                %sV%s) Установить глобальную переменную\r\n"
          "Редактируемая команда : %c\r\n"
          "Укажите тип команды   : ",
          grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm,
          grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, item->cmd.command);
  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

//------------------------------------------------------------------------
/*
 * Print the appropriate message for the command type for arg1 and set
 * up the input catch clause
 */
void zedit_disp_arg1(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);

  send_to_char("\r\n", d->character.get());

  switch (item->cmd.command) {
    case 'M':
    case 'Q':
      sprintf(buf,
              "Выбор моба\r\n"
              "Текущий моб  : %d [%s]\r\n" "Введите номер: ", item->cmd.arg1, name_by_vnum(item->cmd.arg1, MOB_NAME));
      break;

    case 'O':
    case 'E':
    case 'P':
    case 'G':
      sprintf(buf,
              "Выбор предмета\r\n"
              "Текущий предмет: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg1,
              name_by_vnum(item->cmd.arg1, OBJ_NAME));
      break;

    case 'D':
    case 'R':
    case 'F':
      if (item->cmd.arg1 == -1)
        item->cmd.arg1 = OLC_NUM(d);
      sprintf(buf,
              "Выбор комнаты\r\n"
              "Текущая комната: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg1,
              name_by_vnum(item->cmd.arg1, ROOM_NAME));
      break;

    case 'T':
    case 'V':
      sprintf(buf,
              "Выбор типа триггера\r\n"
              "  0 - моб\r\n"
              "  1 - предмет\r\n" "  2 - комната\r\n" "Текущий тип: %d\r\n" "Введите тип: ", item->cmd.arg1);
      break;

    default:
      // * We should never get here.
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_disp_arg1(): Help!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Oops...\r\n", d->character.get());
      return;
  }
  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_ARG1;
}

//------------------------------------------------------------------------
/*
 * Print the appropriate message for the command type for arg2 and set
 * up the input catch clause.
 */
void zedit_disp_arg2(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);

  int i = 0;

  send_to_char("\r\n", d->character.get());

  switch (item->cmd.command) {
    case 'M':
    case 'O':
    case 'E':
    case 'P':
    case 'G':
      sprintf(buf,
              "Максимальное количество в мире\r\n"
              "Текущее значение: %d\r\n" "Введите значение: ", item->cmd.arg2);
      break;

    case 'D': sprintf(buf, "Выбор направления выхода\r\n");
      for (i = 0; *dirs[i] != '\n'; ++i)
        sprintf(buf + strlen(buf), "   %d - %s\r\n", i, dirs[i]);
      sprintf(buf + strlen(buf),
              "Текущее направление выхода: %d\r\n" "Введите направление выхода: ", item->cmd.arg2);
      break;

    case 'R':
      sprintf(buf,
              "Выбор предмета\r\n"
              "Текущий предмет: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg2,
              name_by_vnum(item->cmd.arg2, OBJ_NAME));
      break;

    case 'F':
      sprintf(buf,
              "Выбор моба-лидера\r\n"
              "Текущий моб  : %d [%s]\r\n" "Введите номер: ", item->cmd.arg2, name_by_vnum(item->cmd.arg2, MOB_NAME));
      break;

    case 'T':
      sprintf(buf,
              "Выбор триггера\r\n"
              "Текущий триггер: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg2,
              name_by_vnum(item->cmd.arg2, TRIG_NAME));
      break;

    case 'V':
      if (item->cmd.arg2 == -1)
        item->cmd.arg2 = OLC_NUM(d);
      sprintf(buf,
              "Выбор комнаты\r\n"
              "Текущая комната: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg2,
              name_by_vnum(item->cmd.arg2, ROOM_NAME));
      break;

    case 'Q':
    default:
      // * We should never get here, but just in case...
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Опаньки...\r\n", d->character.get());
      return;
  }

  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_ARG2;
}

//------------------------------------------------------------------------

/*
 * Print the appropriate message for the command type for arg3 and set
 * up the input catch clause.
 */
void zedit_disp_arg3(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);

  int i = 0;

  send_to_char("\r\n", d->character.get());

  switch (item->cmd.command) {
    case 'E': sprintf(buf, "Выбор позиции\r\n");
      for (i = 0; *equipment_types[i] != '\n'; ++i)
        sprintf(buf + strlen(buf), "   %2d - %s\r\n", i, equipment_types[i]);
      sprintf(buf + strlen(buf), "Текущая позиция: %d\r\n" "Введите позицию: ", item->cmd.arg3);
      break;

    case 'P':
      sprintf(buf,
              "Выбор контейнера\r\n"
              "Текущий предмет: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg3,
              name_by_vnum(item->cmd.arg3, OBJ_NAME));
      break;

    case 'D':
      sprintf(buf,
              "Выбор состояния двери\r\n"
              "  0 - Дверь открыта\r\n"
              "  1 - Дверь закрыта\r\n"
              "  2 - Дверь заперта\r\n"
              "  3 - Выход скрыт\r\n"
              "  4 - Выход явный\r\n" "Текущее состояние: %d\r\n" "Введите состояние: ", item->cmd.arg3);
      break;

    case 'V':
      sprintf(buf,
              "Выбор контекста переменной\r\n"
              "Текущий контекст: %d\r\n" "Введите контекст: ", item->cmd.arg3);
      break;

    case 'F':
      sprintf(buf,
              "Выбор моба-последователя\r\n"
              "Текущий моб  : %d [%s]\r\n" "Введите номер: ", item->cmd.arg3, name_by_vnum(item->cmd.arg3, MOB_NAME));
      break;

    case 'O':
    case 'M':
    case 'T':
      if (item->cmd.arg3 == -1)
        item->cmd.arg3 = OLC_NUM(d);
      sprintf(buf,
              "Выбор комнаты\r\n"
              "Текущая комната: %d [%s]\r\n" "Введите номер  : ",
              item->cmd.arg3,
              name_by_vnum(item->cmd.arg3, ROOM_NAME));
      break;

    case 'Q':
    case 'R':
    case 'G':
    default:
      // * We should never get here, just in case.
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_disp_arg3(): Help!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Опаньки...\r\n", d->character.get());
      return;
  }

  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_ARG3;
}

void zedit_disp_arg4(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);

  send_to_char("\r\n", d->character.get());

  switch (item->cmd.command) {
    case 'M':
      sprintf(buf,
              "Максимальное количество в комнате\r\n"
              "Текущее значение: %d\r\n" "Введите значение: ", item->cmd.arg4);
      break;

    case 'O':
    case 'E':
    case 'P':
    case 'G':
      sprintf(buf,
              "Вероятность загрузки (-1 = 100%%)\r\n"
              "Текущее значение: %d\r\n" "Введите значение: ", item->cmd.arg4);
      break;

    case 'Q':
    case 'F':
    case 'R':
    case 'D':
    case 'T':
    case 'V':
    default:
      // * We should never get here, but just in case...
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Опаньки...\r\n", d->character.get());
      return;
  }

  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_ARG4;
}

void zedit_disp_sarg1(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);

  send_to_char("\r\n", d->character.get());

  switch (item->cmd.command) {
    case 'V':
      sprintf(buf,
              "Выбор имени глобальной переменной\r\n"
              "Текущее имя: %s\r\n" "Введите имя: ", item->cmd.sarg1 ? item->cmd.sarg1 : "<NULL>");
      break;

    case 'M':
    case 'O':
    case 'E':
    case 'P':
    case 'G':
    case 'Q':
    case 'F':
    case 'R':
    case 'D':
    case 'T':
    default:
      // * We should never get here, but just in case...
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_disp_sarg1(): Help!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Опаньки...\r\n", d->character.get());
      return;
  }

  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_SARG1;
}

void zedit_disp_sarg2(DESCRIPTOR_DATA *d) {
  pzcmd item = SEEK_CMD(d);

  send_to_char("\r\n", d->character.get());

  switch (item->cmd.command) {
    case 'V':
      sprintf(buf,
              "Выбор значения глобальной переменной\r\n"
              "Текущее значение: %s\r\n" "Введите значение: ", item->cmd.sarg2 ? item->cmd.sarg2 : "<NULL>");
      break;

    case 'M':
    case 'O':
    case 'E':
    case 'P':
    case 'G':
    case 'Q':
    case 'F':
    case 'R':
    case 'D':
    case 'T':
    default:
      // * We should never get here, but just in case...
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_disp_sarg2(): Help!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Опаньки...\r\n", d->character.get());
      return;
  }

  send_to_char(buf, d->character.get());
  OLC_MODE(d) = ZEDIT_SARG2;
}

// *************************************************************************
// * The GARGANTAUN event handler                                          *
// *************************************************************************

#define CHECK_MOB(d, n)  if(real_mobile(n)<0)   {send_to_char("Неверный номер моба, повторите : ",d->character.get());return;}
#define CHECK_OBJ(d, n)  if(real_object(n)<0)   {send_to_char("Неверный номер объекта, повторите : ",d->character.get());return;}
#define CHECK_ROOM(d, n) if(real_room(n)<=NOWHERE)     {send_to_char("Неверный номер комнаты, повторите : ",d->character.get());return;}
#define CHECK_TRIG(d, n) if(real_trigger(n)<0)  {send_to_char("Неверный номер триггера, повторите : ",d->character.get());return;}
#define CHECK_NUM(d, n)  if(!is_signednumber(n)){send_to_char("Ожидается число, повторите : ",d->character.get());return;}

void zedit_parse(DESCRIPTOR_DATA *d, char *arg) {
  pzcmd item;
  int pos, i, j;
  int *temp = NULL;

  switch (OLC_MODE(d)) {
    case ZEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
        case 'д':
        case 'Д':
          // * Save the zone in memory, hiding invisible people.
          send_to_char("Сохраняю зону в памяти.\r\n", d->character.get());
          zedit_save_internally(d);
          sprintf(buf, "OLC: %s edits zone info for room %d.", GET_NAME(d->character), OLC_NUM(d));
          olc_log("%s edit zone %d", GET_NAME(d->character), OLC_NUM(d));
          mudlog(buf, NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
          // FALL THROUGH
        case 'n':
        case 'N':
        case 'н':
        case 'Н': cleanup_olc(d, CLEANUP_ALL);
          break;
        default: send_to_char("Неверный выбор!\r\n", d->character.get());
          send_to_char("Вы желаете сохранить зону в памяти? : ", d->character.get());
          break;
      }
      break;
      // End of ZEDIT_CONFIRM_SAVESTRING

    case ZEDIT_MAIN_MENU:
      switch (*arg) {
        case 'x':
        case 'X':
          if (OLC_ZONE(d)->age || OLC_ZONE(d)->number) {
            send_to_char("Вы желаете сохранить изменения зоны в памяти? (y/n) : ", d->character.get());
            OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
          } else {
            send_to_char("Не было изменений.\r\n", d->character.get());
            cleanup_olc(d, CLEANUP_ALL);
          }
          break;

        case '1':
        case '2':
        case '3':
        case '7':
        case '8':
        case '9':
          if (d->olc->bitmask & OLC_BM_SHOWALLCMD)
            zedit_scroll_list(d, arg);
          zedit_disp_menu(d);
          break;

        case 'f':
        case 'F':    // Фильтр
          d->olc->bitmask ^= OLC_BM_SHOWALLCMD;
          zedit_disp_menu(d);
          break;

        case 'n':
        case 'N':    // New entry
          send_to_char("Выберите номер новой команды? : ", d->character.get());
          OLC_MODE(d) = ZEDIT_NEW_ENTRY;
          break;

        case 'e':
        case 'E':    // Change an entry
          send_to_char("Какую команду Вы хотите изменить? : ", d->character.get());
          OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
          break;

        case 'm':
        case 'M':    // Move an entry
          send_to_char("Какую команду и куда Вы хотите переместить? : ", d->character.get());
          OLC_MODE(d) = ZEDIT_MOVE_ENTRY;
          break;

        case 'd':
        case 'D':    // Delete an entry
          send_to_char("Какую команду Вы хотите удалить? : ", d->character.get());
          OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
          break;

        case 'z':
        case 'Z':
          // * Edit zone name.
          send_to_char("Введите новое имя зоны : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_NAME;
          break;
        case 'c':
        case 'C': send_to_char("Введите новый комментарий к зоне : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_COMMENT;
          break;
        case 'w':
        case 'W': send_to_char("Введите местоположение зоны : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_LOCATION;
          break;
        case 'u':
        case 'U': send_to_char("Введите автора зоны : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_AUTOR;
          break;
        case 'o':
        case 'O': send_to_char("Введите описание к зоне : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_DESCRIPTION;
          break;
//MZ.load
        case 's':
        case 'S':
          // * Edit zone level.
          send_to_char("Введите уровень зоны : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_LEVEL;
          break;
        case 'y':
        case 'Y':
          // * Edit surface type.
          zedit_disp_type_menu(d);
          OLC_MODE(d) = ZEDIT_ZONE_TYPE;
          break;
//-MZ.load

        case 't':
        case 'T': OLC_ZONE(d)->under_construction = !(OLC_ZONE(d)->under_construction);
          OLC_ZONE(d)->number = 1;
          zedit_disp_menu(d);
          break;
        case 'g':
        case 'G': send_to_char(d->character.get(), "Оптимальное число игроков (1 - 20): ");
          OLC_MODE(d) = ZEDIT_ZONE_GROUP;
          break;
        case 'p':
        case 'P':
          // * Edit top of zone.
          if (GET_LEVEL(d->character) < LVL_IMPL)
            zedit_disp_menu(d);
          else {
            send_to_char("Введите новую старшую комнату зоны.\r\n"
                         "Помните, она всегда должны быть равна НомерЗоны*100+99 : ", d->character.get());
            OLC_MODE(d) = ZEDIT_ZONE_TOP;
          }
          break;
        case 'l':
        case 'L':
          // * Edit zone lifespan.
          send_to_char("Введите новое время жизни зоны : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_LIFE;
          break;
        case 'i':
        case 'I':
          // * Edit zone reset_idle flag.
          send_to_char("Выберите, очищается ли неиспользованная зона (y/n) : ", d->character.get());
          OLC_MODE(d) = ZEDIT_RESET_IDLE;
          break;
        case 'r':
        case 'R':
          // * Edit zone reset mode.
          send_to_char("\r\n"
                       "0) Никогда не очищать\r\n"
                       "1) Очищать, если в зоне нет игроков\r\n"
                       "2) Обычная очистка(даже если есть игроки)\r\n"
                       "3) Общая очистка(комплекс зон)\r\n" "Выберите тип очистки зоны : ", d->character.get());
          OLC_MODE(d) = ZEDIT_ZONE_RESET;
          break;
        case 'a':
        case 'A':
          // * Edit type A list.
          send_to_char("Введите новую зону для списка одновременно перезагружаемых зон: ", d->character.get());
          OLC_MODE(d) = ZEDIT_TYPE_A_LIST;
          break;
        case 'b':
        case 'B':
          // * Edit type B list.
          send_to_char("Введите новую зону для списка зон, проверяемых при перезагрузке: ", d->character.get());
          OLC_MODE(d) = ZEDIT_TYPE_B_LIST;
          break;
        default: zedit_disp_menu(d);
          break;
      }
      break;
      // End of ZEDIT_MAIN_MENU

    case ZEDIT_MOVE_ENTRY: {
      // Получить два параметра: исходная строка и целевая строка
      int src, dst;
      if (sscanf(arg, "%d %d", &src, &dst) == 2)
        if (move_command(d, src, dst))
          OLC_ZONE(d)->age = 1;
    }
      zedit_disp_menu(d);
      break;

    case ZEDIT_DELETE_ENTRY:
      // Get the line number and delete the line
      if (is_number(arg))
        if (delete_command(d, atoi(arg)))
          OLC_ZONE(d)->age = 1;
      zedit_disp_menu(d);
      break;

    case ZEDIT_NEW_ENTRY:
      // Get the line number and insert the new line. //
      if (!is_number(arg) || !new_command(d, atoi(arg))) {
        zedit_disp_menu(d);
        break;
      }
      // тут break не нужен, иду редактировать

      // fall through
    case ZEDIT_CHANGE_ENTRY:
      // Parse the input for which line to edit, and goto next quiz. //
      if (is_number(arg) && start_change_command(d, atoi(arg))) {
        zedit_disp_comtype(d);
        OLC_ZONE(d)->age = 1;
      } else
        zedit_disp_menu(d);
      break;

    case ZEDIT_COMMAND_TYPE:
      // Parse the input for which type of command this is, and goto next quiz. //
      item = SEEK_CMD(d);
      item->cmd.command = toupper(*arg);
      if (!item->cmd.command || (strchr("MFQOPEDGRTV", item->cmd.command) == NULL))
        send_to_char("Неверный выбор, повторите : ", d->character.get());
      else {
        sprintf(buf,
                "Режимы исполнения команды:\r\n"
                "  0 - выполняется всегда\r\n"
                "  1 - выполняется только в случае успешного выполнения предыдущей\r\n"
                "  2 - выполняется всегда, не изменяет признак успешного выполнения\r\n"
                "  3 - выполняется только в случае успешного выполнения предыдущей, не изменяет признак успешного выполнения\r\n"
                "Текущий режим  : %d\r\n" "Выберите режим : ", item->cmd.if_flag);
        send_to_char(buf, d->character.get());
        OLC_MODE(d) = ZEDIT_IF_FLAG;
      }
      break;

    case ZEDIT_IF_FLAG:
      // Parse the input for the if flag, and goto next quiz. //
      item = SEEK_CMD(d);
      switch (*arg) {
        case '0':
        case '1':
        case '2':
        case '3': item->cmd.if_flag = arg[0] - '0';
          break;
        default: send_to_char("Повторите ввод : ", d->character.get());
          return;
      }
      zedit_disp_arg1(d);
      break;

    case ZEDIT_ARG1:
      // Parse the input for arg1, and goto next quiz. //
      CHECK_NUM(d, arg)
      item = SEEK_CMD(d);
      pos = atoi(arg);
      item->cmd.arg1 = pos;
      switch (item->cmd.command) {
        case 'M':
        case 'Q': CHECK_MOB(d, pos)
          if (item->cmd.command == 'M')
            zedit_disp_arg2(d);
          else
            zedit_disp_menu(d);
          break;

        case 'O':
        case 'P':
        case 'E':
        case 'G':
//			CHECK_OBJ(d, pos) zedit_disp_arg2(d);
//Gorrah: Поскольку у нас теперь max in world хранится в объекте, то редактировать его тут не нужно.
          CHECK_OBJ(d, pos);
          if (item->cmd.command == 'G')
            zedit_disp_arg4(d);
          else
            zedit_disp_arg3(d);
          break;

        case 'F':
        case 'D':
        case 'R': CHECK_ROOM(d, pos)
          zedit_disp_arg2(d);
          break;

        case 'T':
        case 'V':
          if (pos != 0 && pos != 1 && pos != 2) {
            send_to_char("Неверный тип триггера, повторите : ", d->character.get());
            return;
          }
          if (item->cmd.command == 'V' && pos != WLD_TRIGGER)
            zedit_disp_arg3(d);
          else
            zedit_disp_arg2(d);
          break;

        default:
          // * We should never get here.
          cleanup_olc(d, CLEANUP_ALL);
          mudlog("SYSERR: OLC: zedit_parse(): case ARG1: Ack!", BRF, LVL_BUILDER, SYSLOG, TRUE);
          send_to_char("Опаньки...\r\n", d->character.get());
          break;
      }
      break;

    case ZEDIT_ARG2:
      // Parse the input for arg2, and goto next quiz.
      CHECK_NUM(d, arg)
      item = SEEK_CMD(d);
      pos = atoi(arg);
      item->cmd.arg2 = pos;
      switch (item->cmd.command) {
        case 'M':
        case 'O':
        case 'P':
        case 'E':
        case 'G':
          if (pos < 0) {
            send_to_char("Параметр должен быть неотрицательным, повторите : ", d->character.get());
            return;
          }
          if (item->cmd.command == 'G')
            zedit_disp_arg4(d);
          else
            zedit_disp_arg3(d);
          break;

        case 'F': CHECK_MOB(d, pos)
          zedit_disp_arg3(d);
          break;

        case 'V': CHECK_ROOM(d, pos)
          zedit_disp_arg3(d);
          break;

        case 'R': CHECK_OBJ(d, pos)
          zedit_disp_menu(d);
          break;

        case 'D': for (i = 0; *dirs[i] != '\n' && i != pos; ++i);
          if (*dirs[i] == '\n') {
            send_to_char("Неверное направление, повторите : ", d->character.get());
            return;
          }
          zedit_disp_arg3(d);
          break;

        case 'T': CHECK_TRIG(d, pos)
          if (item->cmd.arg1 == WLD_TRIGGER)
            zedit_disp_arg3(d);
          else
            zedit_disp_menu(d);
          break;

        case 'Q':
        default:
          // * We should never get here, but just in case...
          cleanup_olc(d, CLEANUP_ALL);
          mudlog("SYSERR: OLC: zedit_parse(): case ARG2: Ack!", BRF, LVL_BUILDER, SYSLOG, TRUE);
          send_to_char("Опаньки...\r\n", d->character.get());
          break;
      }
      break;

    case ZEDIT_ARG3:
      // Parse the input for arg3, and go back to main menu.
      CHECK_NUM(d, arg)
      item = SEEK_CMD(d);
      pos = atoi(arg);
      item->cmd.arg3 = pos;
      switch (item->cmd.command) {
        case 'M':
        case 'O':
        case 'T': CHECK_ROOM(d, pos)
          if (item->cmd.command == 'T')
            zedit_disp_menu(d);
          else
            zedit_disp_arg4(d);
          break;

        case 'F': CHECK_MOB(d, pos)
          zedit_disp_menu(d);
          break;

        case 'P': CHECK_OBJ(d, pos)
          zedit_disp_arg4(d);
          break;

        case 'E': for (i = 0; *equipment_types[i] != '\n' && i != pos; ++i);
          if (*equipment_types[i] == '\n') {
            send_to_char("Неверная позиция, повторите : ", d->character.get());
            return;
          }
          zedit_disp_arg4(d);
          break;

        case 'V': zedit_disp_sarg1(d);
          break;

        case 'D':
          if (pos != 0 && pos != 1 && pos != 2 && pos != 3 && pos != 4) {
            send_to_char("Неверное состояние выхода, повторите : ", d->character.get());
            return;
          }
          zedit_disp_menu(d);
          break;

        case 'Q':
        case 'R':
        case 'G':
        default:
          // * We should never get here, but just in case...
          cleanup_olc(d, CLEANUP_ALL);
          mudlog("SYSERR: OLC: zedit_parse(): case ARG3: Ack!", BRF, LVL_BUILDER, SYSLOG, TRUE);
          send_to_char("Опаньки...\r\n", d->character.get());
          break;
      }
      break;

    case ZEDIT_ARG4: CHECK_NUM(d, arg)
      item = SEEK_CMD(d);
      pos = atoi(arg);
      item->cmd.arg4 = pos;
      switch (item->cmd.command) {
        case 'M':
          if (pos < 0)
            item->cmd.arg4 = -1;
          zedit_disp_menu(d);
          break;

        case 'G':
        case 'O':
        case 'E':
        case 'P':
          if (pos < 0)
            item->cmd.arg4 = -1;
          if (pos > 100)
            item->cmd.arg4 = 100;
          zedit_disp_menu(d);
          break;

        case 'F':
        case 'Q':
        case 'R':
        case 'D':
        case 'T':
        case 'V':
        default:
          // * We should never get here, but just in case...
          cleanup_olc(d, CLEANUP_ALL);
          mudlog("SYSERR: OLC: zedit_parse(): case ARG4: Ack!", BRF, LVL_BUILDER, SYSLOG, TRUE);
          send_to_char("Опаньки...\r\n", d->character.get());
          break;
      }
      break;

    case ZEDIT_SARG1: item = SEEK_CMD(d);
      if (item->cmd.sarg1)
        free(item->cmd.sarg1);
      item->cmd.sarg1 = NULL;
      if (strlen(arg))
        item->cmd.sarg1 = str_dup(arg);
      zedit_disp_sarg2(d);
      break;

    case ZEDIT_SARG2: item = SEEK_CMD(d);
      if (item->cmd.sarg2)
        free(item->cmd.sarg2);
      item->cmd.sarg2 = NULL;
      if (strlen(arg))
        item->cmd.sarg2 = str_dup(arg);
      zedit_disp_menu(d);
      break;

    case ZEDIT_ZONE_NAME:
      // * Add new name and return to main menu.
      if (OLC_ZONE(d)->name)
        free(OLC_ZONE(d)->name);
      else
        log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
      OLC_ZONE(d)->name = str_dup(arg);
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
      break;
//MZ.load

    case ZEDIT_ZONE_LEVEL:
      // * Parse and add new level and return to main menu.
      pos = atoi(arg);
      if (!is_number(arg) || (pos < MIN_ZONE_LEVEL) || (pos > MAX_ZONE_LEVEL))
        send_to_char(d->character.get(), "Повторите ввод (%d-%d) : ", MIN_ZONE_LEVEL, MAX_ZONE_LEVEL);
      else {
        OLC_ZONE(d)->level = pos;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;

    case ZEDIT_ZONE_TYPE:
      // * Parse and add new zone type and return to main menu.
      pos = atoi(arg);
      if (!is_number(arg) || (pos < 0))
        zedit_disp_type_menu(d);
      else {
        for (i = 0; *zone_types[i].name != '\n'; i++)
          if (i == pos)
            break;
        if (*zone_types[i].name == '\n')
          zedit_disp_type_menu(d);
        else {
          OLC_ZONE(d)->type = pos;
          OLC_ZONE(d)->number = 1;
          zedit_disp_menu(d);
        }
      }
      break;
//-MZ.load

    case ZEDIT_ZONE_RESET:
      // * Parse and add new reset_mode and return to main menu.
      pos = atoi(arg);
      if (!is_number(arg) || (pos < 0) || (pos > 3)) {
        send_to_char("Повторите ввод (0-3) : ", d->character.get());
      } else {
        OLC_ZONE(d)->reset_mode = pos;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;

    case ZEDIT_ZONE_LIFE:
      // * Parse and add new lifespan and return to main menu.
      pos = atoi(arg);
      if (!is_number(arg) || (pos < 0) || (pos > 240)) {
        send_to_char("Повторите ввод (0-240) : ", d->character.get());
      } else {
        OLC_ZONE(d)->lifespan = pos;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;

    case ZEDIT_RESET_IDLE:
      // * Parse and add new reset_idle and return to main menu.
      if (!arg[0] || !strchr("YyNnДдНн", arg[0])) {
        send_to_char("Повторите ввод (y или n) : ", d->character.get());
      } else {
        if (strchr("YyДд", arg[0]))
          OLC_ZONE(d)->reset_idle = 1;
        else
          OLC_ZONE(d)->reset_idle = 0;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;

    case ZEDIT_TYPE_A_LIST:
      // * Add or delete new zone in the type A zones list.
      pos = atoi(arg);
      if (!is_number(arg) || (pos < 1) || (pos > MAX_PROTO_NUMBER)) {
        send_to_char(d->character.get(), "Повторите ввод (1-%d) : ", MAX_PROTO_NUMBER);
      } else {
        for (i = 0; i < OLC_ZONE(d)->typeA_count; i++) {
          if (OLC_ZONE(d)->typeA_list[i] == pos)    // нашли совпадающий -- убираем элемент
          {
            if (OLC_ZONE(d)->typeA_count > 1)
              CREATE(temp, (OLC_ZONE(d)->typeA_count - 1));
// копируем в temp с пропуском значения из pos
            for (j = 0; j < i; j++)
              temp[j] = OLC_ZONE(d)->typeA_list[j];
            for (j = i; j < (OLC_ZONE(d)->typeA_count - 1); j++)
              temp[j] = OLC_ZONE(d)->typeA_list[j + 1];
            break;
          }
        }

        if (i == OLC_ZONE(d)->typeA_count)    // не нашли совпадающих -- добавляем новый
        {
          CREATE(temp, (OLC_ZONE(d)->typeA_count + 1));
          for (j = 0; j < OLC_ZONE(d)->typeA_count; j++) {
            temp[j] = OLC_ZONE(d)->typeA_list[j];
          }
          temp[j] = pos;
          OLC_ZONE(d)->typeA_count++;
        } else {
          --OLC_ZONE(d)->typeA_count;
        }

        free(OLC_ZONE(d)->typeA_list);
        CREATE(OLC_ZONE(d)->typeA_list, OLC_ZONE(d)->typeA_count);
        for (i = 0; i < OLC_ZONE(d)->typeA_count; i++) {
          OLC_ZONE(d)->typeA_list[i] = temp[i];
        }
        free(temp);
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;

    case ZEDIT_TYPE_B_LIST:
      // * Add or delete new zone in the type A zones list.
      pos = atoi(arg);
      if (!is_number(arg) || (pos < 1) || (pos > MAX_PROTO_NUMBER)) {
        send_to_char(d->character.get(), "Повторите ввод (1-%d) : ", MAX_PROTO_NUMBER);
      } else {
        for (i = 0; i < OLC_ZONE(d)->typeB_count; i++) {
          if (OLC_ZONE(d)->typeB_list[i] == pos)    // нашли совпадающий -- убираем элемент
          {
            if (OLC_ZONE(d)->typeB_count > 1) {
              CREATE(temp, (OLC_ZONE(d)->typeB_count - 1));
            }
// копируем в temp с пропуском значения из pos
            for (j = 0; j < i; j++)
              temp[j] = OLC_ZONE(d)->typeB_list[j];
            for (j = i; j < (OLC_ZONE(d)->typeB_count - 1); j++)
              temp[j] = OLC_ZONE(d)->typeB_list[j + 1];
            break;
          }
        }
        if (i == OLC_ZONE(d)->typeB_count)    // не нашли совпадающих -- добавляем новый
        {
          CREATE(temp, (OLC_ZONE(d)->typeB_count + 1));
          for (j = 0; j < OLC_ZONE(d)->typeB_count; j++) {
            temp[j] = OLC_ZONE(d)->typeB_list[j];
          }
          temp[j] = pos;
          OLC_ZONE(d)->typeB_count++;
        } else {
          --OLC_ZONE(d)->typeB_count;
        }

        free(OLC_ZONE(d)->typeB_list);
        CREATE(OLC_ZONE(d)->typeB_list, OLC_ZONE(d)->typeB_count);
        for (i = 0; i < OLC_ZONE(d)->typeB_count; i++) {
          OLC_ZONE(d)->typeB_list[i] = temp[i];
        }
        free(temp);
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;

    case ZEDIT_ZONE_TOP:
      // * Parse and add new top room in zone and return to main menu.
      if (OLC_ZNUM(d) == static_cast<zone_rnum>(zone_table.size()) - 1) {
        OLC_ZONE(d)->top = MAX(OLC_ZNUM(d) * 100, MIN(99900, atoi(arg)));
      } else {
        OLC_ZONE(d)->top = MAX(OLC_ZNUM(d) * 100, MIN(zone_table[OLC_ZNUM(d) + 1].number * 100, atoi(arg)));
      }

      OLC_ZONE(d)->top = (OLC_ZONE(d)->top / 100) * 100 + 99;
      zedit_disp_menu(d);
      break;

    case ZEDIT_ZONE_COMMENT:
      if (OLC_ZONE(d)->comment) {
        free(OLC_ZONE(d)->comment);
      }
      OLC_ZONE(d)->comment = str_dup(arg);
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
      break;

    case ZEDIT_ZONE_LOCATION:
      if (OLC_ZONE(d)->location) {
        free(OLC_ZONE(d)->location);
        OLC_ZONE(d)->location = NULL;
      }
      if (arg && *arg) {
        OLC_ZONE(d)->location = str_dup(arg);
      }
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
      break;

    case ZEDIT_ZONE_AUTOR:
      if (OLC_ZONE(d)->author) {
        free(OLC_ZONE(d)->author);
        OLC_ZONE(d)->author = NULL;
      }
      if (arg && *arg) {
        OLC_ZONE(d)->author = str_dup(arg);
      }
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
      break;

    case ZEDIT_ZONE_DESCRIPTION:
      if (OLC_ZONE(d)->description) {
        free(OLC_ZONE(d)->description);
      }
      OLC_ZONE(d)->description = str_dup(arg);
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
      break;

    case ZEDIT_ZONE_GROUP: {
      int num = atoi(arg);
      if (num < 1 || num > 20) {
        send_to_char("Повторите ввод (от 1 до 20) :", d->character.get());
      } else {
        OLC_ZONE(d)->group = num;
        OLC_ZONE(d)->number = 1;
        zedit_disp_menu(d);
      }
      break;
    }

    default:
      // * We should never get here, but just in case...
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): Reached default case!", BRF, LVL_BUILDER, SYSLOG, TRUE);
      send_to_char("Опаньки...\r\n", d->character.get());
      break;
  }
}

// * End of parse_zedit()

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
