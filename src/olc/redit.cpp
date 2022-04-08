/************************************************************************
 *  OasisOLC - redit.c						v1.5	*
 *  Copyright 1996 Harvey Gilpin.					*
 *  Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "entities/obj_data.h"
#include "comm.h"
#include "db.h"
#include "olc.h"
#include "dg_script/dg_olc.h"
#include "constants.h"
#include "crafts/im.h"
#include "description.h"
#include "game_mechanics/deathtrap.h"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "entities/room_data.h"
#include "house.h"
#include "entities/world_characters.h"
#include "entities/zone.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "conf.h"

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

void room_copy(RoomData *dst, RoomData *src);
void room_free(RoomData *room);

void redit_save_internally(DescriptorData *d);
void redit_save_to_disk(int zone);

void redit_parse(DescriptorData *d, char *arg);
void redit_disp_extradesc_menu(DescriptorData *d);
void redit_disp_exit_menu(DescriptorData *d);
void redit_disp_exit_flag_menu(DescriptorData *d);
void redit_disp_flag_menu(DescriptorData *d);
void redit_disp_sector_menu(DescriptorData *d);
void redit_disp_menu(DescriptorData *d);

//***************************************************************************
#define  W_EXIT(room, num) (world[(room)]->dir_option[(num)])
//***************************************************************************

void redit_setup(DescriptorData *d, int real_num)
/*++
   Подготовка данных для редактирования комнаты.
      d        - OLC дескриптор
      real_num - RNUM исходной комнаты, новая -1
--*/
{
	RoomData *room = new RoomData;
	if (real_num == kNowhere) {
		room->name = str_dup("Недоделанная комната.\r\n");
		room->temp_description =
			str_dup("Вы оказались в комнате, наполненной обломками творческих мыслей билдера.\r\n");
	} else {
		room_copy(room, world[real_num]);
		// temp_description существует только на время редактирования комнаты в олц
		room->temp_description = str_dup(RoomDescription::show_desc(world[real_num]->description_num).c_str());
	}

	OLC_ROOM(d) = room;
	OLC_ITEM_TYPE(d) = WLD_TRIGGER;
	redit_disp_menu(d);
	OLC_VAL(d) = 0;
}

//------------------------------------------------------------------------

#define ZCMD (zone_table[zone].cmd[cmd_no])

// * Сохранить новую комнату в памяти
void redit_save_internally(DescriptorData *d) {
	int j, room_num, cmd_no;
	ObjData *temp_obj;

	room_num = real_room(OLC_ROOM(d)->room_vn);
	// дальше temp_description уже нигде не участвует, описание берется как обычно через число
	OLC_ROOM(d)->description_num = RoomDescription::add_desc(OLC_ROOM(d)->temp_description);
	// * Room exists: move contents over then free and replace it.
	if (room_num != kNowhere) {
		log("[REdit] Save room to mem %d", room_num);
		// Удаляю устаревшие данные
		room_free(world[room_num]);
		// Текущее состояние комнаты не изменилось, обновляю редактированные данные
		room_copy(world[room_num], OLC_ROOM(d));
		// Теперь просто удалить OLC_ROOM(d) и все будет хорошо
		room_free(OLC_ROOM(d));
		// Удаление "оболочки" произойдет в olc_cleanup
	} else {
		// если комнаты не было - добавляем новую
		auto it = world.cbegin();
		advance(it, kFirstRoom);
		int i = kFirstRoom;

		for (; it != world.cend(); ++it, ++i) {
			if ((*it)->room_vn > OLC_NUM(d)) {
				break;
			}
		}

		RoomData *new_room = new RoomData;
		room_copy(new_room, OLC_ROOM(d));
		new_room->room_vn = OLC_NUM(d);
		new_room->zone_rn = OLC_ZNUM(d);
		new_room->func = nullptr;
		room_num = i; // рнум новой комнаты

		if (it != world.cend()) {
			world.insert(it, new_room);
			// если комната потеснила рнумы, то их надо переписать у людей/шмота в этих комнатах
			for (i = room_num; i <= top_of_world; i++) {
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

		fix_ingr_chest_rnum(room_num);//Фиксим позиции сундуков с инграми

		// Copy world table over to new one.
		top_of_world++;

// ПЕРЕИНДЕКСАЦИЯ

		// Update zone table.
		for (std::size_t zone = 0; zone < zone_table.size(); zone++) {
			for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
				switch (ZCMD.command) {
					case 'M':
					case 'O':
						if (ZCMD.arg3 >= room_num)
							ZCMD.arg3++;
						break;

					case 'F':
					case 'R':
					case 'D':
						if (ZCMD.arg1 >= room_num)
							ZCMD.arg1++;
						break;

					case 'T':
						if (ZCMD.arg1 == WLD_TRIGGER && ZCMD.arg3 >= room_num)
							ZCMD.arg3++;
						break;

					case 'V':
						if (ZCMD.arg2 >= room_num)
							ZCMD.arg2++;
						break;
				}
			}
		}

		// * Update load rooms, to fix creeping load room problem.
		if (room_num <= r_mortal_start_room)
			r_mortal_start_room++;
		if (room_num <= r_immort_start_room)
			r_immort_start_room++;
		if (room_num <= r_frozen_start_room)
			r_frozen_start_room++;
		if (room_num <= r_helled_start_room)
			r_helled_start_room++;
		if (room_num <= r_named_start_room)
			r_named_start_room++;
		if (room_num <= r_unreg_start_room)
			r_unreg_start_room++;


		// поля in_room для объектов и персонажей уже изменены
		for (const auto &temp_ch : character_list) {
			RoomRnum temp_room = temp_ch->get_was_in_room();
			if (temp_room >= room_num) {
				temp_ch->set_was_in_room(++temp_room);
			}
		}

		// Порталы, выходы
		for (i = kFirstRoom; i < top_of_world + 1; i++) {
			if (world[i]->portal_room >= room_num) {
				world[i]->portal_room++;
			}

			for (j = 0; j < EDirection::kMaxDirNum; j++) {
				if (W_EXIT(i, j)) {
					const auto to_room = W_EXIT(i, j)->to_room();

					if (to_room >= room_num) {
						W_EXIT(i, j)->to_room(1 + to_room);
					}
				}
			}
		}

		// * Update any rooms being edited.
		for (auto dsc = descriptor_list; dsc; dsc = dsc->next) {
			if (dsc->connected == CON_REDIT) {
				for (j = 0; j < EDirection::kMaxDirNum; j++) {
					if (OLC_ROOM(dsc)->dir_option[j]) {
						const auto to_room = OLC_ROOM(dsc)->dir_option[j]->to_room();

						if (to_room >= room_num) {
							OLC_ROOM(dsc)->dir_option[j]->to_room(1 + to_room);
						}
					}
				}
			}
		}
	}

	check_room_flags(room_num);

	// пока мы не удаляем комнаты через олц - проблем нету
	// а вот в случае удаления надо будет обновлять указатели для списка слоу-дт и врат
	if (ROOM_FLAGGED(room_num, ERoomFlag::kSlowDeathTrap)
		|| ROOM_FLAGGED(room_num, ERoomFlag::kIceTrap)) {
		deathtrap::add(world[room_num]);
	} else {
		deathtrap::remove(world[room_num]);
	}

	// Настало время добавить триггеры
	SCRIPT(world[room_num])->cleanup();
	assign_triggers(world[room_num], WLD_TRIGGER);
	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].vnum, OLC_SAVE_ROOM);
}

//------------------------------------------------------------------------

void redit_save_to_disk(int zone_num) {
	int counter, counter2, realcounter;
	FILE *fp;
	RoomData *room;

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
		if ((realcounter = real_room(counter)) != kNowhere) {
			if (counter % 100 == 99)
				continue;
			room = world[realcounter];

#if defined(REDIT_LIST)
			sprintf(buf1, "OLC: Saving room %d.", room->number);
			log(buf1);
#endif

			// * Remove the '\r\n' sequences from description.
			strcpy(buf1, RoomDescription::show_desc(room->description_num).c_str());
			strip_string(buf1);

			// * Forget making a buffer, lets just write the thing now.
			*buf2 = '\0';
			room->flags_tascii(4, buf2);
			fprintf(fp, "#%d\n%s~\n%s~\n%d %s %d\n", counter,
					room->name ? room->name : "неопределено", buf1,
					zone_table[room->zone_rn].vnum, buf2, room->sector_type);

			// * Handle exits.
			for (counter2 = 0; counter2 < EDirection::kMaxDirNum; counter2++) {
				if (room->dir_option[counter2]) {
					// * Again, strip out the garbage.
					if (!room->dir_option[counter2]->general_description.empty()) {
						const std::string &description = room->dir_option[counter2]->general_description;
						strcpy(buf1, description.c_str());
						strip_string(buf1);
					} else {
						*buf1 = 0;
					}

					// * Check for keywords.
					if (room->dir_option[counter2]->keyword) {
						strcpy(buf2, room->dir_option[counter2]->keyword);
					}

					// алиас в винительном падеже пишется сюда же через ;
					if (room->dir_option[counter2]->vkeyword) {
						strcpy(buf2 + strlen(buf2), "|");
						strcpy(buf2 + strlen(buf2), room->dir_option[counter2]->vkeyword);
					} else
						*buf2 = '\0';

					//Флаги направления перед записью по какой-то причине ресетятся
					//Сохраним их, чтобы не поломать загруженную зону
					byte old_exit_info = room->dir_option[counter2]->exit_info;

					REMOVE_BIT(room->dir_option[counter2]->exit_info, EExitFlag::kClosed);
					REMOVE_BIT(room->dir_option[counter2]->exit_info, EExitFlag::kLocked);
					REMOVE_BIT(room->dir_option[counter2]->exit_info, EExitFlag::kBrokenLock);
					// * Ok, now wrote output to file.
					fprintf(fp, "D%d\n%s~\n%s~\n%d %d %d %d\n",
							counter2, buf1, buf2,
							room->dir_option[counter2]->exit_info, room->dir_option[counter2]->key,
							room->dir_option[counter2]->to_room() != kNowhere ?
							world[room->dir_option[counter2]->to_room()]->room_vn : kNowhere,
							room->dir_option[counter2]->lock_complexity);

					//Восстановим флаги направления в памяти
					room->dir_option[counter2]->exit_info = old_exit_info;
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

	get_char_cols(d->character.get());
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
			 OLC_EXIT(d)->to_room() != kNowhere ? world[OLC_EXIT(d)->to_room()]->room_vn : kNowhere,
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
	get_char_cols(d->character.get());
	sprintf(buf,
			"ВНИМАНИЕ! Созданная здесь дверь будет всегда отперта и открыта.\r\n"
			"Изменить состояние двери по умолчанию можно только командами зоны (zedit).\r\n\r\n"
			"%s1%s) [%c]Дверь\r\n"
			"%s2%s) [%c]Невзламываемая\r\n"
			"%s3%s) [%c]Скрытый выход\r\n"
			"%s4%s) [%d]Сложность замка\r\n"
			"Ваш выбор (0 - выход): ",
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHasDoor) ? 'x' : ' ',
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kPickroof) ? 'x' : ' ',
			grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EExitFlag::kHidden) ? 'x' : ' ',
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

	get_char_cols(d->character.get());
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
			 room->dir_option[EDirection::kNorth] && room->dir_option[EDirection::kNorth]->to_room() != kNowhere
			 ? world[room->dir_option[EDirection::kNorth]->to_room()]->room_vn : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option[EDirection::kEast] && room->dir_option[EDirection::kEast]->to_room() != kNowhere
			 ? world[room->dir_option[EDirection::kEast]->to_room()]->room_vn : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option[EDirection::kSouth] && room->dir_option[EDirection::kSouth]->to_room() != kNowhere
			 ? world[room->dir_option[EDirection::kSouth]->to_room()]->room_vn : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option[EDirection::kWest] && room->dir_option[EDirection::kWest]->to_room() != kNowhere
			 ? world[room->dir_option[EDirection::kWest]->to_room()]->room_vn : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option[EDirection::kUp] && room->dir_option[EDirection::kUp]->to_room() != kNowhere
			 ? world[room->dir_option[EDirection::kUp]->to_room()]->room_vn : kNowhere,
			 grn, nrm, cyn,
			 room->dir_option[EDirection::kDown] && room->dir_option[EDirection::kDown]->to_room() != kNowhere
			 ? world[room->dir_option[EDirection::kDown]->to_room()]->room_vn : kNowhere,
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
					mudlog(buf, NRM, MAX(kLvlBuilder, GET_INVIS_LEV(d->character)), SYSLOG, true);
					// * Do NOT free strings! Just the room structure.
					cleanup_olc(d, CLEANUP_STRUCTS);
					SendMsgToChar("Room saved to memory.\r\n", d->character.get());
					break;

				case 'n':
				case 'N':
				case 'н':
				case 'Н':
					// * Free everything up, including strings, etc.
					cleanup_olc(d, CLEANUP_ALL);
					break;
				default:
					SendMsgToChar("Неверный выбор!\r\nВы желаете сохранить комнату в памяти? : ",
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
						SendMsgToChar("Вы желаете сохранить комнату в памяти? : ", d->character.get());
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
					SEND_TO_Q("\x1B[H\x1B[J", d);
#endif
					SEND_TO_Q("Введите описание комнаты: (/s записать /h помощь)\r\n\r\n", d);
					d->backstr = nullptr;
					if (OLC_ROOM(d)->temp_description) {
						SEND_TO_Q(OLC_ROOM(d)->temp_description, d);
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
				number = real_room(number);
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
					SEND_TO_Q("Введите экстраописание: (/s сохранить /h помощь)\r\n\r\n", d);
					d->backstr = nullptr;
					if (OLC_DESC(d)->description) {
						SEND_TO_Q(OLC_DESC(d)->description, d);
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
