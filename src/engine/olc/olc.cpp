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

#include "engine/db/obj_prototypes.h"
#include "engine/entities/obj_data.h"
#include "engine/ui/interpreter.h"
#include "engine/core/comm.h"
#include "engine/db/db.h"
#include "engine/db/world_data_source_manager.h"
#include "engine/scripting/dg_olc.h"
#include "engine/ui/color.h"
#include "gameplay/crafting/item_creation.h"
#include "gameplay/crafting/im.h"
#include "administration/privilege.h"
#include "engine/entities/char_data.h"
#include "engine/entities/room_data.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/id_converter.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "engine/entities/zone.h"

#include <vector>

// * External data structures.
extern CharData *mob_proto;

extern DescriptorData *descriptor_list;

// * External functions.
void zedit_setup(DescriptorData *d, int room_num);
void zedit_save_to_disk(int zone);
void medit_setup(DescriptorData *d, int rmob_num);
void medit_save_to_disk(int zone);
void redit_setup(DescriptorData *d, int rroom_num);
void redit_save_to_disk(int zone);
void oedit_setup(DescriptorData *d, int robj_num);
void oedit_save_to_disk(int zone);
void sedit_setup_new(DescriptorData *d);
void sedit_setup_existing(DescriptorData *d, int robj_num);
void medit_mobile_free(CharData *mob);
void trigedit_setup_new(DescriptorData *d);
void trigedit_setup_existing(DescriptorData *d, int rtrg_num);
int GetTriggerRnum(int vnum);
void dg_olc_script_free(DescriptorData *d);

// Internal function prototypes.
void olc_saveinfo(CharData *ch);

// global data
const char *save_info_msg[5] = {"Rooms", "Objects", "Zone info", "Mobiles", "Shops"};
const char *nrm = kColorNrm;
const char *grn = kColorGrn;
const char *cyn = kColorCyn;
const char *yel = kColorYel;
const char *iyel = kColorBoldYel;
const char *ired = kColorBoldRed;
struct olc_save_info *olc_save_list = nullptr;

struct olc_scmd_data {
	const char *text;
	EConState con_type;
};

struct olc_scmd_data olc_scmd_info[5] =
	{
		{"room", EConState::kRedit},
		{"object", EConState::kOedit},
		{"room", EConState::kZedit},
		{"mobile", EConState::kMedit},
		{"trigger", EConState::kTrigedit}
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

void do_olc(CharData *ch, char *argument, int cmd, int subcmd) {
	int number = -1, save = 0, real_num;
	bool lock = 0, unlock = 0;
	DescriptorData *d;

	// * No screwing around as a mobile.
	if (ch->IsNpc())
		return;

	if (subcmd == kScmdOlcSaveinfo) {
		olc_saveinfo(ch);
		return;
	}

	// * Parse any arguments.
	two_arguments(argument, buf1, buf2);
	if (!*buf1)        // No argument given.
	{
		switch (subcmd) {
			case kScmdOlcZedit:
			case kScmdOlcRedit: number = world[ch->in_room]->vnum;
				break;
			case kScmdOlcTrigedit:
			case kScmdOlcOedit:
			case kScmdOlcMedit: sprintf(buf, "Укажите %s VNUM для редактирования.\r\n", olc_scmd_info[subcmd].text);
				SendMsgToChar(buf, ch);
				return;
		}
	} else if (!a_isdigit(*buf1)) {
		if (strn_cmp("save", buf1, 4) == 0
			|| (lock = !strn_cmp("lock", buf1, 4)) == true
			|| (unlock = !strn_cmp("unlock", buf1, 6)) == true) {
			if (!*buf2) {
				if (GET_OLC_ZONE(ch)) {
					save = 1;
					number = (GET_OLC_ZONE(ch) * 100);
				} else {
					SendMsgToChar("И какую зону писать?\r\n", ch);
					return;
				}
			} else {
				save = 1;
				number = atoi(buf2) * 100;
			}
		} else if (subcmd == kScmdOlcZedit && (GetRealLevel(ch) >= kLvlBuilder || ch->IsFlagged(EPrf::kCoderinfo))) {
			SendMsgToChar("Создание новых зон отключено.\r\n", ch);
			return;
		} else {
			SendMsgToChar("Уточните, что вы хотите делать!\r\n", ch);
			return;
		}
	}
	// * If a numeric argument was given, get it.
	if (number == -1) {
		number = atoi(buf1);
	}

	// * Check that whatever it isn't already being edited.
	for (d = descriptor_list; d; d = d->next) {
		if (d->state == olc_scmd_info[subcmd].con_type) {
			if (d->olc && OLC_NUM(d) == number) {
				sprintf(buf, "%s в настоящий момент редактируется %s.\r\n",
						olc_scmd_info[subcmd].text, GET_PAD(d->character, 4));
				SendMsgToChar(buf, ch);
				return;
			}
		}
	}
	d = ch->desc;

	// лок/анлок редактирования зон только 34м и по привилегии
	if ((lock || unlock) && !IS_IMPL(ch) && !privilege::CheckFlag(d->character.get(), privilege::kFullzedit)) {
		SendMsgToChar("Вы не можете использовать эту команду.\r\n", ch);
		return;
	}

	// * Give descriptor an OLC struct.
	d->olc = new olc_data;

	// * Find the zone.
	if ((OLC_ZNUM(d) = get_zone_rnum_by_vnumum(number)) == -1) {
		SendMsgToChar("Звыняйтэ, такойи зоны нэмае.\r\n", ch);
		delete d->olc;
		return;
	}

	if (lock) {
		zone_table[OLC_ZNUM(d)].locked = true;
		SendMsgToChar("Защищаю зону от записи.\r\n", ch);
		sprintf(buf, "(GC) %s has locked zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].vnum);
		olc_log("%s locks zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].vnum);
		mudlog(buf, LGH, kLvlImplementator, SYSLOG, true);
		
		auto* data_source = world_loader::WorldDataSourceManager::Instance().GetDataSource();
			data_source->SaveZone(OLC_ZNUM(d));
		delete d->olc;
		return;
	}

	if (unlock) {
		zone_table[OLC_ZNUM(d)].locked = false;
		SendMsgToChar("Снимаю защиту от записи.\r\n", ch);
		sprintf(buf, "(GC) %s has unlocked zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].vnum);
		olc_log("%s unlocks zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].vnum);
		mudlog(buf, LGH, kLvlImplementator, SYSLOG, true);
		
		auto* data_source = world_loader::WorldDataSourceManager::Instance().GetDataSource();
			data_source->SaveZone(OLC_ZNUM(d));
		delete d->olc;
		return;
	}
	// Check if zone is protected from editing
	if ((zone_table[OLC_ZNUM(d)].locked) && (GetRealLevel(ch) != 34)) {
		SendMsgToChar("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
		delete d->olc;
		return;
	}

	// * Everyone but IMPLs can only edit zones they have been assigned.
	if (GetRealLevel(ch) < kLvlImplementator) {
		if (!privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), cmd, 0, false)) {
			if (!GET_OLC_ZONE(ch) || (zone_table[OLC_ZNUM(d)].vnum != GET_OLC_ZONE(ch))) {
				SendMsgToChar("Вам запрещен доступ к сией зоне.\r\n", ch);
				delete d->olc;
				return;
			}
		}
	}

	if (save) {
		const char *type = nullptr;
		switch (subcmd) {
			case kScmdOlcRedit: type = "room";
				break;
			case kScmdOlcZedit: type = "zone";
				break;
			case kScmdOlcMedit: type = "mobile";
				break;
			case kScmdOlcOedit: type = "object";
				break;
		}
		if (!type) {
			SendMsgToChar("Родной(ая,ое), объясни по людски - что записать.\r\n", ch);
			return;
		}
		sprintf(buf, "Saving all %ss in zone %d.\r\n", type, zone_table[OLC_ZNUM(d)].vnum);
		SendMsgToChar(buf, ch);
		sprintf(buf, "OLC: %s saves %s info for zone %d.", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].vnum);
		olc_log("%s save %s in Z%d", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].vnum);
		mudlog(buf, LGH, std::max(kLvlBuilder, GET_INVIS_LEV(ch)), SYSLOG, true);

		auto* data_source = world_loader::WorldDataSourceManager::Instance().GetDataSource();

		switch (subcmd) {
			case kScmdOlcRedit: data_source->SaveRooms(OLC_ZNUM(d));
				break;
			case kScmdOlcZedit: data_source->SaveZone(OLC_ZNUM(d));
				break;
			case kScmdOlcOedit: data_source->SaveObjects(OLC_ZNUM(d));
				break;
			case kScmdOlcMedit: data_source->SaveMobs(OLC_ZNUM(d));
				break;
		}
		delete d->olc;
		return;
	}
	OLC_NUM(d) = number;

	// * Steal player's descriptor start up subcommands.
	switch (subcmd) {
		case kScmdOlcTrigedit:
			if ((real_num = GetTriggerRnum(number)) >= 0)
				trigedit_setup_existing(d, real_num);
			else
				trigedit_setup_new(d);
			d->state = EConState::kTrigedit;
			break;
		case kScmdOlcRedit:
			if ((real_num = GetRoomRnum(number)) != kNowhere)
				redit_setup(d, real_num);
			else
				redit_setup(d, kNowhere);
			d->state = EConState::kRedit;
			break;
		case kScmdOlcZedit:
			if ((real_num = GetRoomRnum(number)) == kNowhere) {
				SendMsgToChar("Желательно создать комнату прежде, чем начинаете ее редактировать.\r\n", ch);
				delete d->olc;
				return;
			}
			zedit_setup(d, real_num);
			d->state = EConState::kZedit;
			break;
		case kScmdOlcMedit:
			if ((real_num = GetMobRnum(number)) >= 0)
				medit_setup(d, real_num);
			else
				medit_setup(d, -1);
			d->state = EConState::kMedit;
			break;
		case kScmdOlcOedit: real_num = GetObjRnum(number);
			if (real_num >= 0) {
				oedit_setup(d, real_num);
			} else {
				oedit_setup(d, -1);
			}
			d->state = EConState::kOedit;
			break;
	}
	act("$n по локоть запустил$g руки в глубины Мира и начал$g что-то со скрежетом там поворачивать.",
		true, d->character.get(), 0, 0, kToRoom);
	ch->SetFlag(EPlrFlag::kWriting);
}

// ------------------------------------------------------------
// Internal utilities
// ------------------------------------------------------------

void olc_saveinfo(CharData *ch) {
	struct olc_save_info *entry;

	if (olc_save_list) {
		SendMsgToChar("The following OLC components need saving:-\r\n", ch);
	} else {
		SendMsgToChar("The database is up to date.\r\n", ch);
	}

	for (entry = olc_save_list; entry; entry = entry->next) {
		sprintf(buf, " - %s for zone %d.\r\n", save_info_msg[(int) entry->type], entry->zone);
		SendMsgToChar(buf, ch);
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

void disp_planes_values(DescriptorData *d, const char *names[], short num_column) {
	int counter, column = 0, plane = 0;
	char c;
	if (d->character->GetLevel() < kLvlImplementator)
		SendMsgToChar(d->character.get(), "Ваш уровень меньше имплементатора, список выводится не полностью.\r\n");
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++) {
		if (*names[counter] == '\n') {
			plane++;
			c = 'a' - 1;
			continue;
		} else if (c == 'z') {
			c = 'A';
		} else
			c++;
		if (d->character->GetLevel() < kLvlImplementator && *names[counter] == '*')
			continue;
		sprintf(buf, "&g%c%d&n) %-30.30s %s", c, plane, names[counter], !(++column % num_column) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
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
void cleanup_olc(DescriptorData *d, byte cleanup_type) {
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
				case CLEANUP_ALL: CleanupRoomData(OLC_ROOM(d));    // удаляет все содержимое
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
			OLC_ZONE(d)->name.clear();
			zedit_delete_cmdlist((pzcmd) OLC_ZONE(d)->cmd);
			free(OLC_ZONE(d));
		}

		// Restore descriptor playing status.
		if (d->character) {
			d->character->UnsetFlag(EPlrFlag::kWriting);
			d->state = EConState::kPlaying;
			act("$n закончил$g работу и удовлетворенно посмотрел$g в развороченные недра Мироздания.",
				true, d->character.get(), 0, 0, kToRoom);
		}
		delete d->olc;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
