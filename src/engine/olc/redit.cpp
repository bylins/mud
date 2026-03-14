/************************************************************************
 *  OasisOLC - redit.c						v1.5	*
 *  Copyright 1996 Harvey Gilpin.					*
 *  Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/db.h"
#include "engine/db/world_data_source_manager.h"
#include "olc.h"
#include "engine/scripting/dg_olc.h"
#include "gameplay/core/constants.h"
#include "gameplay/crafting/im.h"
#include "engine/db/description.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/deathtrap.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/entities/room_data.h"
#include "gameplay/clans/house.h"
#include "engine/db/world_characters.h"
#include "engine/entities/zone.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "gameplay/mechanics/dungeons.h"

#include <sys/stat.h>

#include <vector>

// * External data structures.
extern CharData *mob_proto;
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *exit_bits[];
extern RoomRnum r_frozen_start_room;
extern RoomRnum r_helled_start_room;
extern RoomRnum r_mortal_start_room;
extern RoomRnum r_immort_start_room;
extern RoomRnum r_named_start_room;
extern RoomRnum r_unreg_start_room;
extern DescriptorData *descriptor_list;

//------------------------------------------------------------------------
int planebit(const char *str, int *plane, int *bit);
// * Function Prototypes
void redit_setup(DescriptorData *d, int real_num);

void CopyRoom(RoomData *dst, RoomData *src);
void CleanupRoomData(RoomData *room);

void redit_save_internally(DescriptorData *d);
void redit_save_to_disk(int zone);

void redit_parse(DescriptorData *d, char *arg);
void redit_disp_extradesc_menu(DescriptorData *d);
void redit_disp_exit_menu(DescriptorData *d);
void redit_disp_exit_flag_menu(DescriptorData *d);
void redit_disp_flag_menu(DescriptorData *d);
void redit_disp_sector_menu(DescriptorData *d);
void redit_disp_menu(DescriptorData *d);
void redit_setup(DescriptorData *d, int real_num)
/*++
   Подготовка данных для редактирования комнаты.
      d        - OLC дескриптор
      real_num - RNUM исходной комнаты, новая -1
--*/
{
	RoomData *room = new RoomData;
	if (real_num == kNowhere) {
		room->name = str_dup("Недоделанная комната.");
		room->temp_description =
			str_dup("Вы оказались в комнате, наполненной обломками творческих мыслей билдера.\r\n");
	} else {
		CopyRoom(room, world[real_num]);
		// temp_description существует только на время редактирования комнаты в олц
		room->temp_description = str_dup(GlobalObjects::descriptions().get(world[real_num]->description_num).c_str());
	}

	OLC_ROOM(d) = room;
	OLC_ITEM_TYPE(d) = WLD_TRIGGER;
	redit_disp_menu(d);
	OLC_VAL(d) = 0;
}

//------------------------------------------------------------------------

#define ZCMD (zone_table[zone].cmd[cmd_no])
extern void RestoreRoomExitData(RoomRnum rrn);
// * Сохранить новую комнату в памяти
void redit_save_internally(DescriptorData *d) {
	int j, rrn, cmd_no;
	ObjData *temp_obj;

	rrn = GetRoomRnum(OLC_ROOM(d)->vnum);
	// дальше temp_description уже нигде не участвует, описание берется как обычно через число
	OLC_ROOM(d)->description_num = GlobalObjects::descriptions().add(OLC_ROOM(d)->temp_description);
	// * Room exists: move contents over then free and replace it.
	if (rrn != kNowhere) {
		log("[REdit] Save room to mem %d", rrn);
		// Удаляю устаревшие данные
		CleanupRoomData(world[rrn]);
		// Текущее состояние комнаты не изменилось, обновляю редактированные данные
		CopyRoom(world[rrn], OLC_ROOM(d));
		// Теперь просто удалить OLC_ROOM(d) и все будет хорошо
		CleanupRoomData(OLC_ROOM(d));
		// Удаление "оболочки" произойдет в olc_cleanup
	} else {
		// если комнаты не было - добавляем новую
		auto it = world.cbegin();
		advance(it, kFirstRoom);
		int i = kFirstRoom;

		for (; it != world.cend(); ++it, ++i) {
			if ((*it)->vnum > OLC_NUM(d)) {
				break;
			}
		}

		RoomData *new_room = new RoomData;
		CopyRoom(new_room, OLC_ROOM(d));
		new_room->vnum = OLC_NUM(d);
		new_room->zone_rn = OLC_ZNUM(d);
		new_room->func = nullptr;
		rrn = i; // рнум новой комнаты

		if (it != world.cend()) {
			world.insert(it, new_room);
			// если комната потеснила рнумы, то их надо переписать у людей/шмота в этих комнатах
			for (i = rrn; i <= top_of_world; i++) {
				for (const auto temp_ch : world[i]->people) {
					if (temp_ch->in_room != kNowhere) {
						temp_ch->in_room = i;
					}
				}

				for (temp_obj = world[i]->contents; temp_obj; temp_obj = temp_obj->get_next_content()) {
					if (temp_obj->get_in_room() != kNowhere) {
						temp_obj->set_in_room(i);
					}
				}
			}
		} else {
			world.push_back(new_room);
		}
		zone_table[OLC_ZNUM(d)].RnumRoomsLocation.second++;
		fix_ingr_chest_rnum(rrn);//Фиксим позиции сундуков с инграми

		// Copy world table over to new one.
		top_of_world++;

// ПЕРЕИНДЕКСАЦИЯ

		// Update zone table.
		for (std::size_t zone = 0; zone < zone_table.size(); zone++) {
			if (!zone_table[zone].cmd)
				continue;
			for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
				switch (ZCMD.command) {
					case 'M':
					case 'O':
						if (ZCMD.arg3 >= rrn)
							ZCMD.arg3++;
						break;

					case 'F':
					case 'R':
					case 'D':
						if (ZCMD.arg1 >= rrn)
							ZCMD.arg1++;
						break;

					case 'T':
						if (ZCMD.arg1 == WLD_TRIGGER && ZCMD.arg3 >= rrn)
							ZCMD.arg3++;
						break;

					case 'V':
						if (ZCMD.arg2 >= rrn)
							ZCMD.arg2++;
						break;
				}
			}
		}

		// * Update load rooms, to fix creeping load room problem.
		if (rrn <= r_mortal_start_room)
			r_mortal_start_room++;
		if (rrn <= r_immort_start_room)
			r_immort_start_room++;
		if (rrn <= r_frozen_start_room)
			r_frozen_start_room++;
		if (rrn <= r_helled_start_room)
			r_helled_start_room++;
		if (rrn <= r_named_start_room)
			r_named_start_room++;
		if (rrn <= r_unreg_start_room)
			r_unreg_start_room++;


		// поля in_room для объектов и персонажей уже изменены
		for (const auto &temp_ch : character_list) {
			RoomRnum temp_room = temp_ch->get_was_in_room();
			if (temp_room >= rrn) {
				temp_ch->set_was_in_room(++temp_room);
			}
		}

		// Порталы, выходы
		for (i = kFirstRoom; i < top_of_world + 1; i++) {
/* переживем одну минуту с кривыми порталами, потом все равно закроются
			if (world[i]->portal_room >= rrn) { 
				world[i]->portal_room++;
			}
*/
			for (j = 0; j < EDirection::kMaxDirNum; j++) {
				if (world[i]->dir_option[j]) {
					const auto to_room = world[i]->dir_option[j]->to_room();

					if (to_room >= rrn) {
						world[i]->dir_option[j]->to_room(1 + to_room);
					}
				}
			}
		}

		// * Update any rooms being edited.
		for (auto dsc = descriptor_list; dsc; dsc = dsc->next) {
			if (dsc->state == EConState::kRedit) {
				for (j = 0; j < EDirection::kMaxDirNum; j++) {
					if (OLC_ROOM(dsc)->dir_option[j]) {
						const auto to_room = OLC_ROOM(dsc)->dir_option[j]->to_room();

						if (to_room >= rrn) {
							OLC_ROOM(dsc)->dir_option[j]->to_room(1 + to_room);
						}
					}
				}
			}
		}
	}

	CheckRoomForIncompatibleFlags(rrn);

	// пока мы не удаляем комнаты через олц - проблем нету
	// а вот в случае удаления надо будет обновлять указатели для списка слоу-дт и врат
	if (ROOM_FLAGGED(rrn, ERoomFlag::kSlowDeathTrap)
		|| ROOM_FLAGGED(rrn, ERoomFlag::kIceTrap)) {
		deathtrap::add(world[rrn]);
	} else {
		deathtrap::remove(world[rrn]);
	}

	// Настало время добавить триггеры
	SCRIPT(world[rrn])->cleanup();
	assign_triggers(world[rrn], WLD_TRIGGER);
//	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].vnum, OLC_SAVE_ROOM);
	RestoreRoomExitData(rrn);
	// Save only this specific room (vnum = OLC_NUM(d))
	auto* data_source = world_loader::WorldDataSourceManager::Instance().GetDataSource();
	data_source->SaveRooms(OLC_ZNUM(d), OLC_NUM(d));
}

//------------------------------------------------------------------------

void redit_save_to_disk(ZoneRnum zone_num) {
	int counter, counter2, realcounter;
	FILE *fp;
	RoomData *room;

	if (zone_table[zone_num].vnum >= dungeons::kZoneStartDungeons) {
			sprintf(buf, "Отказ сохранения зоны %d на диск.", zone_table[zone_num].vnum);
			mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			return;
	}
	if (zone_num < 0 || zone_num >= static_cast<int>(zone_table.size())) {
		log("SYSERR: redit_save_to_disk: Invalid real zone passed!");
		return;
	}

	sprintf(buf, "%s/%d.new", WLD_PREFIX, zone_table[zone_num].vnum);
	if (!(fp = fopen(buf, "w+"))) {
		mudlog("SYSERR: OLC: Cannot open room file!", BRF, kLvlBuilder, SYSLOG, true);
		return;
	}
	for (counter = zone_table[zone_num].vnum * 100; counter < zone_table[zone_num].top; counter++) {
		if ((realcounter = GetRoomRnum(counter)) != kNowhere) {
			if (counter % 100 == 99)
				continue;
			room = world[realcounter];

#if defined(REDIT_LIST)
			sprintf(buf1, "OLC: Saving room %d.", room->number);
			log(buf1);
#endif

			// * Remove the '\r\n' sequences from description.
			strcpy(buf1, GlobalObjects::descriptions().get(room->description_num).c_str());
			strip_string(buf1);

			// * Forget making a buffer, lets just write the thing now.
			*buf2 = '\0';
			room->flags_tascii(FlagData::kPlanesNumber, buf2);
			fprintf(fp, "#%d\n%s~\n%s~\n%d %s %d\n", counter,
					room->name ? room->name : "неопределено", buf1,
					zone_table[room->zone_rn].vnum, buf2, room->sector_type);

			// * Handle exits.
			for (counter2 = 0; counter2 < EDirection::kMaxDirNum; counter2++) {
				if (room->dir_option_proto[counter2]) {
					// * Again, strip out the garbage.
					if (!room->dir_option_proto[counter2]->general_description.empty()) {
						const std::string &description = room->dir_option_proto[counter2]->general_description;
						strcpy(buf1, description.c_str());
						strip_string(buf1);
					} else {
						*buf1 = 0;
					}

					// * Check for keywords.
					if (room->dir_option_proto[counter2]->keyword) {
						strcpy(buf2, room->dir_option_proto[counter2]->keyword);
					}

					// алиас в винительном падеже пишется сюда же через ;
					if (room->dir_option_proto[counter2]->vkeyword) {
						strcpy(buf2 + strlen(buf2), "|");
						strcpy(buf2 + strlen(buf2), room->dir_option_proto[counter2]->vkeyword);
					} else
						*buf2 = '\0';
					REMOVE_BIT(room->dir_option_proto[counter2]->exit_info, EExitFlag::kBrokenLock);
					fprintf(fp, "D%d\n%s~\n%s~\n%d %d %d %d\n",
							counter2, buf1, buf2,
							room->dir_option_proto[counter2]->exit_info, room->dir_option_proto[counter2]->key,
							room->dir_option_proto[counter2]->to_room() != kNowhere ?
							world[room->dir_option_proto[counter2]->to_room()]->vnum : kNowhere,
							room->dir_option_proto[counter2]->lock_complexity);
				}
			}
			// * Home straight, just deal with extra descriptions.
			if (room->ex_description) {
				for (auto ex_desc = room->ex_description; ex_desc; ex_desc = ex_desc->next) {
					if (ex_desc->keyword && ex_desc->description) {
						strcpy(buf1, ex_desc->description);
						strip_string(buf1);
						fprintf(fp, "E\n%s~\n%s~\n", ex_desc->keyword, buf1);
					}
				}
			}
			fprintf(fp, "S\n");
			script_save_to_disk(fp, room, WLD_TRIGGER);
		}
	}
	// * Write final line and close.
	fprintf(fp, "$\n$\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.wld", WLD_PREFIX, zone_table[zone_num].vnum);
	// * We're fubar'd if we crash between the two lines below.
	remove(buf2);
	rename(buf, buf2);
#ifndef _WIN32
	if (chmod(buf2, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0) {
		std::stringstream ss;
		ss << "Error chmod file: " << buf2 << " (" << __FILE__ << " "<< __func__ << "  "<< __LINE__ << ")";
		mudlog(ss.str(), BRF, kLvlGod, SYSLOG, true);
	}
#endif

	olc_remove_from_save_list(zone_table[zone_num].vnum, OLC_SAVE_ROOM);
}


// *************************************************************************
// * Menu functions                                                        *
// *************************************************************************

// * For extra descriptions.
void redit_disp_extradesc_menu(DescriptorData *d) {
	auto extra_desc = OLC_DESC(d);

	sprintf(buf,
			"&g1&n) Ключ: &y%s\r\n"
			"&g2&n) Описание:\r\n&y%s\r\n"
			"&g3&n) Следующее описание: ",
			extra_desc->keyword ? extra_desc->keyword : "<NONE>",
			extra_desc->description ? extra_desc->description : "<NONE>");

	strcat(buf, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
	strcat(buf, "Enter choice (0 to quit) : ");
	SendMsgToChar(buf, d->character.get());
	OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

// * For exits.
void redit_disp_exit_menu(DescriptorData *d) {
	// * if exit doesn't exist, alloc/create it
	if (!OLC_EXIT(d)) {
		OLC_EXIT(d).reset(new ExitData());
		OLC_EXIT(d)->to_room(kNowhere);
	}

	// * Weird door handling!
	if (IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHasDoor)) {
		strcpy(buf2, "Дверь ");
		if (IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kPickroof)) {
			strcat(buf2, "Невзламываемая ");
		}
		sprintf(buf2 + strlen(buf2), " (Сложность замка [%d])", OLC_EXIT(d)->lock_complexity);
	} else {
		strcpy(buf2, "Нет двери");
	}

	if (IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHidden)) {
		strcat(buf2, " (Выход скрыт)");
	}

	snprintf(buf, kMaxStringLength,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			 "%s1%s) Ведет в        : %s%d\r\n"
			 "%s2%s) Описание       :-\r\n%s%s\r\n"
			 "%s3%s) Синонимы двери : %s%s (%s)\r\n"
			 "%s4%s) Номер ключа    : %s%d\r\n"
			 "%s5%s) Флаги двери    : %s%s\r\n"
			 "%s6%s) Очистить выход.\r\n"
			 "Ваш выбор (0 - конец) : ",
			 grn, nrm, cyn,
			 OLC_EXIT(d)->to_room() != kNowhere ? world[OLC_EXIT(d)->to_room()]->vnum : kNowhere,
			 grn, nrm,
			 yel,
			 !OLC_EXIT(d)->general_description.empty() ? OLC_EXIT(d)->general_description.c_str() : "<NONE>",
			 grn, nrm, yel,
			 OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
			 OLC_EXIT(d)->vkeyword ? OLC_EXIT(d)->vkeyword : "<NONE>", grn, nrm,
			 cyn, OLC_EXIT(d)->key, grn, nrm, cyn, buf2, grn, nrm);

	SendMsgToChar(buf, d->character.get());
	OLC_MODE(d) = REDIT_EXIT_MENU;
}

// * For exit flags.
void redit_disp_exit_flag_menu(DescriptorData *d) {
	sprintf(buf,
			"%s1%s) [%c]Дверь\r\n"
			"%s2%s) [%c]Невзламываемая\r\n"
			"%s3%s) [%c]Скрытый выход\r\n"
			"%s4%s) [%c]Закрыто\r\n"
			"%s5%s) [%c]Заперто\r\n"
			"%s6%s) [%d]Сложность замка\r\n"
			"Ваш выбор (0 - выход): ",
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHasDoor) ? 'x' : ' ',
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kPickroof) ? 'x' : ' ',
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHidden) ? 'x' : ' ',
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kClosed) ? 'x' : ' ',
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kLocked) ? 'x' : ' ',
			grn, nrm, OLC_EXIT(d)->lock_complexity);
	SendMsgToChar(buf, d->character.get());
}

// * For room flags.
void redit_disp_flag_menu(DescriptorData *d) {
	disp_planes_values(d, room_bits, 2);
	OLC_ROOM(d)->flags_sprint(buf1, ",", true);
	snprintf(buf,
			 kMaxStringLength,
			 "\r\nФлаги комнаты: %s%s%s\r\n" "Введите флаг комнаты (0 - выход) : ",
			 cyn,
			 buf1,
			 nrm);
	SendMsgToChar(buf, d->character.get());
	OLC_MODE(d) = REDIT_FLAGS;
}

// * For sector type.
void redit_disp_sector_menu(DescriptorData *d) {
	int counter, columns = 0;

#if defined(CLEAR_SCREEN)
	SendMsgToChar("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				sector_types[counter], !(++columns % 2) ? "\r\n" : "");
		SendMsgToChar(buf, d->character.get());
	}
	SendMsgToChar("\r\nТип поверхности в комнате : ", d->character.get());
	OLC_MODE(d) = REDIT_SECTOR;
}

// * The main menu.
void redit_disp_menu(DescriptorData *d) {
	RoomData *room;
	room = OLC_ROOM(d);
	room->flags_sprint(buf1, ",");
	sprinttype(room->sector_type, sector_types, buf2);
	snprintf(buf, kMaxStringLength,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
			 "-- Комната : [%s%d%s]  	Зона: [%s%d%s]\r\n"
			 "%s1%s) Название    : &C&q%s&e&Q\r\n"
			 "%s2&n) Описание    :\r\n%s&e"
			 "%s3%s) Флаги       : %s%s\r\n"
			 "%s4%s) Поверхность : %s%s\r\n"
			 "%s5%s) На севере   : %s%d\r\n"
			 "%s6%s) На востоке  : %s%d\r\n"
			 "%s7%s) На юге      : %s%d\r\n"
			 "%s8%s) На западе   : %s%d\r\n"
			 "%s9%s) Вверху      : %s%d\r\n"
			 "%sA%s) Внизу       : %s%d\r\n"
			 "%sB%s) Меню экстраописаний\r\n"
			 //		"%sН%s) Ингредиенты : %s%s\r\n"
			 "%sS%s) Скрипты     : %s%s\r\n"
			 "%sQ%s) Quit\r\n"
			 "Ваш выбор : ",
			 cyn, OLC_NUM(d), nrm,
			 cyn, zone_table[OLC_ZNUM(d)].vnum, nrm,
			 grn, nrm, room->name,
			 grn, room->temp_description,
			 grn, nrm, cyn, buf1, grn, nrm, cyn, buf2, grn, nrm, cyn,
			 room->dir_option_proto[EDirection::kNorth] && room->dir_option_proto[EDirection::kNorth]->to_room() != kNowhere
			 ? world[room->dir_option_proto[EDirection::kNorth]->to_room()]->vnum : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option_proto[EDirection::kEast] && room->dir_option_proto[EDirection::kEast]->to_room() != kNowhere
			 ? world[room->dir_option_proto[EDirection::kEast]->to_room()]->vnum : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option_proto[EDirection::kSouth] && room->dir_option_proto[EDirection::kSouth]->to_room() != kNowhere
			 ? world[room->dir_option_proto[EDirection::kSouth]->to_room()]->vnum : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option_proto[EDirection::kWest] && room->dir_option_proto[EDirection::kWest]->to_room() != kNowhere
			 ? world[room->dir_option_proto[EDirection::kWest]->to_room()]->vnum : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option_proto[EDirection::kUp] && room->dir_option_proto[EDirection::kUp]->to_room() != kNowhere
			 ? world[room->dir_option_proto[EDirection::kUp]->to_room()]->vnum : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option_proto[EDirection::kDown] && room->dir_option_proto[EDirection::kDown]->to_room() != kNowhere
			 ? world[room->dir_option_proto[EDirection::kDown]->to_room()]->vnum : kNowhere,
			 grn, nrm, grn, nrm, cyn,
			 !room->proto_script->empty() ? "Set." : "Not Set.",
			 grn, nrm);
	SendMsgToChar(buf, d->character.get());

	OLC_MODE(d) = REDIT_MAIN_MENU;
}

// *************************************************************************
// *  The main loop                                                        *
// *************************************************************************
void redit_parse(DescriptorData *d, char *arg) {
	int number, plane, bit;

	switch (OLC_MODE(d)) {
		case REDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
				case 'д':
				case 'Д': redit_save_internally(d);
					sprintf(buf, "OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
					olc_log("%s edit room %d", GET_NAME(d->character), OLC_NUM(d));
					mudlog(buf, NRM, std::max(kLvlBuilder, GET_INVIS_LEV(d->character)), SYSLOG, true);
					// * Do NOT free strings! Just the room structure.
					cleanup_olc(d, CLEANUP_STRUCTS);
					SendMsgToChar("Комната сохранена.\r\n", d->character.get());
					break;

				case 'n':
				case 'N':
				case 'н':
				case 'Н':
					// * Free everything up, including strings, etc.
					cleanup_olc(d, CLEANUP_ALL);
					break;
				default:
					SendMsgToChar("Неверный выбор!\r\nВы желаете сохранить комнату? : ",
								 d->character.get());
					break;
			}
			return;

		case REDIT_MAIN_MENU:
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d))    // Something has been modified.
					{
						SendMsgToChar("Вы желаете сохранить комнату? : ", d->character.get());
						OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
					} else {
						cleanup_olc(d, CLEANUP_ALL);
					}
					return;

				case '1': SendMsgToChar("Введите название комнаты:-\r\n] ", d->character.get());
					OLC_MODE(d) = REDIT_NAME;
					break;

				case '2': OLC_MODE(d) = REDIT_DESC;
#if defined(CLEAR_SCREEN)
				iosystem::write_to_output("\x1B[H\x1B[J", d);
#endif
				iosystem::write_to_output("Введите описание комнаты: (/s записать /h помощь)\r\n\r\n", d);
					d->backstr = nullptr;
					if (OLC_ROOM(d)->temp_description) {
					iosystem::write_to_output(OLC_ROOM(d)->temp_description, d);
						d->backstr = str_dup(OLC_ROOM(d)->temp_description);
					}
					d->writer.reset(new utils::DelegatedStringWriter(OLC_ROOM(d)->temp_description));
					d->max_str = MAX_ROOM_DESC;
					d->mail_to = 0;
					OLC_VAL(d) = 1;
					break;

				case '3': redit_disp_flag_menu(d);
					break;

				case '4': redit_disp_sector_menu(d);
					break;

				case '5': OLC_VAL(d) = EDirection::kNorth;
					redit_disp_exit_menu(d);
					break;

				case '6': OLC_VAL(d) = EDirection::kEast;
					redit_disp_exit_menu(d);
					break;

				case '7': OLC_VAL(d) = EDirection::kSouth;
					redit_disp_exit_menu(d);
					break;

				case '8': OLC_VAL(d) = EDirection::kWest;
					redit_disp_exit_menu(d);
					break;

				case '9': OLC_VAL(d) = EDirection::kUp;
					redit_disp_exit_menu(d);
					break;

				case 'a':
				case 'A': OLC_VAL(d) = EDirection::kDown;
					redit_disp_exit_menu(d);
					break;

				case 'b':
				case 'B':
					// * If the extra description doesn't exist.
					if (!OLC_ROOM(d)->ex_description) {
						OLC_ROOM(d)->ex_description.reset(new ExtraDescription());
					}
					OLC_DESC(d) = OLC_ROOM(d)->ex_description;
					redit_disp_extradesc_menu(d);
					break;

				case 's':
				case 'S': dg_olc_script_copy(d);
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
					dg_script_menu(d);
					return;
				default: SendMsgToChar("Неверный выбор!", d->character.get());
					redit_disp_menu(d);
					break;
			}
			olc_log("%s command %c", GET_NAME(d->character), *arg);
			return;

		case OLC_SCRIPT_EDIT:
			if (dg_script_edit_parse(d, arg))
				return;
			break;

		case REDIT_NAME:
			if (OLC_ROOM(d)->name)
				free(OLC_ROOM(d)->name);
			if (strlen(arg) > MAX_ROOM_NAME)
				arg[MAX_ROOM_NAME - 1] = '\0';
			OLC_ROOM(d)->name = str_dup((arg && *arg) ? arg : "неопределено");
			break;

		case REDIT_DESC:
			// * We will NEVER get here, we hope.
			mudlog("SYSERR: Reached REDIT_DESC case in parse_redit", BRF, kLvlBuilder, SYSLOG, true);
			break;

		case REDIT_FLAGS: number = planebit(arg, &plane, &bit);
			if (number < 0) {
				SendMsgToChar("Неверный выбор!\r\n", d->character.get());
				redit_disp_flag_menu(d);
			} else if (number == 0)
				break;
			else {
				// * Toggle the bit.
				OLC_ROOM(d)->toggle_flag(plane, 1 << bit);
				redit_disp_flag_menu(d);
			}
			return;

		case REDIT_SECTOR: number = atoi(arg);
			if (number < 0 || number >= NUM_ROOM_SECTORS) {
				SendMsgToChar("Неверный выбор!", d->character.get());
				redit_disp_sector_menu(d);
				return;
			} else
				OLC_ROOM(d)->sector_type = number;
			break;

		case REDIT_EXIT_MENU:
			switch (*arg) {
				case '0': break;
				case '1': OLC_MODE(d) = REDIT_EXIT_NUMBER;
					SendMsgToChar("Попасть в комнату N (vnum) : ", d->character.get());
					return;
				case '2': OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
					SendMsgToChar("Введите описание выхода : ", d->character.get());
					return;

				case '3': OLC_MODE(d) = REDIT_EXIT_KEYWORD;
					SendMsgToChar("Введите ключевые слова (им.падеж|вин.падеж): ", d->character.get());
					return;
				case '4': OLC_MODE(d) = REDIT_EXIT_KEY;
					SendMsgToChar("Введите номер ключа : ", d->character.get());
					return;
				case '5': redit_disp_exit_flag_menu(d);
					OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
					return;
				case '6':
					// * Delete an exit.
					OLC_EXIT(d).reset();
					break;
				default: SendMsgToChar("Неверный выбор!\r\nВаш выбор : ", d->character.get());
					return;
			}
			break;

		case REDIT_EXIT_NUMBER: number = atoi(arg);
			if (number != kNowhere) {
				number = GetRoomRnum(number);
				if (number == kNowhere) {
					SendMsgToChar("Нет такой комнаты - повторите ввод : ", d->character.get());

					return;
				}
			}
			OLC_EXIT(d)->to_room(number);
			redit_disp_exit_menu(d);
			return;

		case REDIT_EXIT_DESCRIPTION: OLC_EXIT(d)->general_description = arg ? arg : "";
			redit_disp_exit_menu(d);
			return;

		case REDIT_EXIT_KEYWORD: OLC_EXIT(d)->set_keywords(arg);
			redit_disp_exit_menu(d);
			return;

		case REDIT_EXIT_KEY: OLC_EXIT(d)->key = atoi(arg);
			redit_disp_exit_menu(d);
			return;

		case REDIT_EXIT_DOORFLAGS: number = atoi(arg);
			if ((number < 0) || (number > 6)) {
				SendMsgToChar("Неверный выбор!\r\n", d->character.get());
				redit_disp_exit_flag_menu(d);
			} else if (number == 0)
				redit_disp_exit_menu(d);
			else {
				if (number == 1) {
					if (IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHasDoor)) {
						OLC_EXIT(d)->exit_info = 0;
						OLC_EXIT(d)->lock_complexity = 0;
					} else
						SET_BIT(OLC_EXIT(d)->exit_info, EExitFlag::kHasDoor);
				} else if (number == 2) {
					TOGGLE_BIT(OLC_EXIT(d)->exit_info, EExitFlag::kPickroof);
				} else if (number == 3) {
					TOGGLE_BIT(OLC_EXIT(d)->exit_info, EExitFlag::kHidden);
				} else if (number == 4) {
					TOGGLE_BIT(OLC_EXIT(d)->exit_info, EExitFlag::kClosed);
				} else if (number == 5) {
					TOGGLE_BIT(OLC_EXIT(d)->exit_info, EExitFlag::kLocked);
				} else if (number == 6) {
					OLC_MODE(d) = REDIT_LOCK_COMPLEXITY;
					SendMsgToChar("Введите сложность замка, (0-255): ", d->character.get());
					return;
				}
				redit_disp_exit_flag_menu(d);
			}
			return;
		case REDIT_LOCK_COMPLEXITY: OLC_EXIT(d)->lock_complexity = ((arg && *arg) ? atoi(arg) : 0);
			OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
			redit_disp_exit_flag_menu(d);
			return;
		case REDIT_EXTRADESC_KEY: OLC_DESC(d)->keyword = ((arg && *arg) ? str_dup(arg) : nullptr);
			redit_disp_extradesc_menu(d);
			return;

		case REDIT_EXTRADESC_MENU:
			switch ((number = atoi(arg))) {
				case 0:
					// * If something got left out, delete the extra description
					// * when backing out to the menu.
					if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
						auto &desc = OLC_DESC(d);
						desc.reset();
					}
					break;

				case 1: OLC_MODE(d) = REDIT_EXTRADESC_KEY;
					SendMsgToChar("Введите ключевые слова, разделенные пробелами : ", d->character.get());
					return;

				case 2: OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
				iosystem::write_to_output("Введите экстраописание: (/s сохранить /h помощь)\r\n\r\n", d);
					d->backstr = nullptr;
					if (OLC_DESC(d)->description) {
					iosystem::write_to_output(OLC_DESC(d)->description, d);
						d->backstr = str_dup(OLC_DESC(d)->description);
					}
					d->writer.reset(new utils::DelegatedStringWriter(OLC_DESC(d)->description));
					d->max_str = 4096;
					d->mail_to = 0;
					return;

				case 3:
					if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
						SendMsgToChar("Вы не можете редактировать следующее экстраописание, не завершив текущее.\r\n",
									 d->character.get());
						redit_disp_extradesc_menu(d);
					} else {
						if (OLC_DESC(d)->next) {
							OLC_DESC(d) = OLC_DESC(d)->next;
						} else {
							// * Make new extra description and attach at end.
							ExtraDescription::shared_ptr new_extra(new ExtraDescription());
							OLC_DESC(d)->next = new_extra;
							OLC_DESC(d) = new_extra;
						}
						redit_disp_extradesc_menu(d);
					}
					return;
			}
			break;

		default:
			// * We should never get here.
			mudlog("SYSERR: Reached default case in parse_redit", BRF, kLvlBuilder, SYSLOG, true);
			break;
	}
	// * If we get this far, something has been changed.
	OLC_VAL(d) = 1;
	redit_disp_menu(d);
}

/*++
   Функция делает создает копию комнаты.
   После вызова этой функции создается полностью независимая копия комнты src
   за исключением полей track, contents, people, affected.
   Все поля имеют те же значения, но занимают свои области памяти.
      dst - "чистый" указатель на структуру room_data.
      src - исходная комната
   Примечание: Неочищенный указатель dst приведет к утечке памяти.
               Используйте redit_room_free() для очистки содержимого комнаты
--*/
void CopyRoom(RoomData *dst, RoomData *src) {
	int i;

	{
		// Сохраняю track, contents, people, аффекты
		struct TrackData *track = dst->track;
		ObjData *contents = dst->contents;
		const auto people_backup = dst->people;
		auto affected = dst->affected;

		// Копирую все поверх
		*dst = *src;

		// Восстанавливаю track, contents, people, аффекты
		dst->track = track;
		dst->contents = contents;
		dst->people = people_backup;
		dst->affected = affected;
	}

	// Теперь нужно выделить собственные области памяти

	// Название и описание
	dst->name = str_dup(src->name ? src->name : "неопределено");
	dst->temp_description = nullptr; // так надо

	// Выходы и входы
	for (i = 0; i < EDirection::kMaxDirNum; ++i) {
		const auto &rdd = src->dir_option_proto[i];
		if (rdd) {
			dst->dir_option[i] = std::make_shared<ExitData>();
			// Копируем числа
			*dst->dir_option_proto[i] = *rdd;
			// Выделяем память
			dst->dir_option_proto[i]->general_description = rdd->general_description;
			dst->dir_option_proto[i]->keyword = (rdd->keyword ? str_dup(rdd->keyword) : nullptr);
			dst->dir_option_proto[i]->vkeyword = (rdd->vkeyword ? str_dup(rdd->vkeyword) : nullptr);
		}
	}

	// Дополнительные описания, если есть
	ExtraDescription::shared_ptr *pddd = &dst->ex_description;
	ExtraDescription::shared_ptr sdd = src->ex_description;
	*pddd = nullptr;

	while (sdd) {
		*pddd = std::make_shared<ExtraDescription>();
		(*pddd)->keyword = sdd->keyword ? str_dup(sdd->keyword) : nullptr;
		(*pddd)->description = sdd->description ? str_dup(sdd->description) : nullptr;
		pddd = &((*pddd)->next);
		sdd = sdd->next;
	}

	// Копирую скрипт и прототипы
	SCRIPT(dst).reset(new Script());

	dst->proto_script = std::make_shared<ObjData::triggers_list_t>();
	*dst->proto_script = *src->proto_script;
}

/*++
   Функция полностью освобождает память, занимаемую данными комнаты.
   ВНИМАНИЕ. Память самой структуры room_data не освобождается.
             Необходимо дополнительно использовать delete()
--*/
void CleanupRoomData(RoomData *room)
{
	// Название и описание
	if (room->name) {
		free(room->name);
		room->name = nullptr;
	}

	if (room->temp_description) {
		free(room->temp_description);
		room->temp_description = nullptr;
	}

	// Выходы и входы
	for (int i = 0; i < EDirection::kMaxDirNum; i++) {
		if (room->dir_option[i]) {
			room->dir_option[i].reset();
		}
	}

	// Скрипт
	room->cleanup_script();
	room->affected.clear();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
