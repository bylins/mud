/************************************************************************
 *  OasisOLC - redit.c						v1.5	*
 *  Copyright 1996 Harvey Gilpin.					*
 *  Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "dg_olc.h"
#include "constants.h"
#include "im.h"
#include "description.h"
#include "deathtrap.hpp"

/* List each room saved, was used for debugging. */
#if 0
#define REDIT_LIST	1
#endif

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern CHAR_DATA *character_list;

extern vector < OBJ_DATA * >obj_proto;
extern CHAR_DATA *mob_proto;
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *exit_bits[];
extern struct zone_data *zone_table;
extern room_rnum r_frozen_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;
extern zone_rnum top_of_zone_table;
extern DESCRIPTOR_DATA *descriptor_list;

/*------------------------------------------------------------------------*/
void tascii(int *pointer, int num_planes, char *ascii);
int planebit(char *str, int *plane, int *bit);
/*
 * Function Prototypes
 */
void redit_setup(DESCRIPTOR_DATA * d, int real_num);

void room_copy(ROOM_DATA * dst, ROOM_DATA * src);
void room_free(ROOM_DATA * room);

void redit_save_internally(DESCRIPTOR_DATA * d);
void redit_save_to_disk(int zone);

void redit_parse(DESCRIPTOR_DATA * d, char *arg);
void redit_disp_extradesc_menu(DESCRIPTOR_DATA * d);
void redit_disp_exit_menu(DESCRIPTOR_DATA * d);
void redit_disp_exit_flag_menu(DESCRIPTOR_DATA * d);
void redit_disp_flag_menu(DESCRIPTOR_DATA * d);
void redit_disp_sector_menu(DESCRIPTOR_DATA * d);
void redit_disp_menu(DESCRIPTOR_DATA * d);

//***************************************************************************
#define  W_EXIT(room, num) (world[(room)]->dir_option[(num)])
//***************************************************************************

void redit_setup(DESCRIPTOR_DATA * d, int real_num)
/*++
   Подготовка данных для редактирования комнаты.
      d        - OLC дескриптор
      real_num - RNUM исходной комнаты, новая -1
--*/
{
	ROOM_DATA *room;
	CREATE(room, ROOM_DATA, 1);
	memset(room, 0, sizeof(ROOM_DATA));

	if (real_num == NOWHERE) {
		room->name = str_dup("Недоделанная комната");
		room->temp_description = str_dup("Вы оказались в комнате, наполненной обломками творческих мыслей билдера.\r\n");
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

/*------------------------------------------------------------------------*/

#define ZCMD (zone_table[zone].cmd[cmd_no])

void redit_save_internally(DESCRIPTOR_DATA * d)
/*++
   Сохранить новую комнату в памяти
--*/
{
	int i, j, room_num, found = 0, zone, cmd_no;
	CHAR_DATA *temp_ch;
	OBJ_DATA *temp_obj;
	DESCRIPTOR_DATA *dsc;
	vector < ROOM_DATA * >::iterator p;

	room_num = real_room(OLC_ROOM(d)->number);
	// дальше temp_description уже нигде не участвует, описание берется как обычно через число
	OLC_ROOM(d)->description_num = RoomDescription::add_desc(OLC_ROOM(d)->temp_description);
	/*
	 * Room exists: move contents over then free and replace it.
	 */
	if (room_num != NOWHERE) {
		log("[REdit] Save room to mem %d", room_num);
		// Удаляю устаревшие данные
		room_free(world[room_num]);
		// Текущее состояние комнаты не изменилось, обновляю редактированные данные
		room_copy(world[room_num], OLC_ROOM(d));
		// Теперь просто удалить OLC_ROOM(d) и все будет хорошо
		room_free(OLC_ROOM(d));
		// Удаление "оболочки" произойдет в olc_cleanup

	} else {
		/* Room doesn't exist, hafta add it. */
		// Count through world tables.
		p = world.begin();
		advance(p, FIRST_ROOM);
		for (i = FIRST_ROOM; i <= top_of_world; i++, p++) {
			if (!found) {
				// Is this the place?
				if (world[i]->number > OLC_NUM(d)) {
					found = TRUE;
					ROOM_DATA *new_room;
					new_room = new(ROOM_DATA);
					memset(new_room, 0, sizeof(ROOM_DATA));

					world.insert(p, new_room);
					room_copy(world[i], OLC_ROOM(d));

					world[i]->number = OLC_NUM(d);
					world[i]->func = NULL;

					room_num = i;
					continue;
				}
			} else {	/* Already been found. */
				// People in this room must have their in_rooms moved.
				for (temp_ch = world[i]->people; temp_ch; temp_ch = temp_ch->next_in_room)
					if (temp_ch->in_room != NOWHERE)
						temp_ch->in_room = i;
				// Move objects too.
				for (temp_obj = world[i]->contents; temp_obj; temp_obj = temp_obj->next_content)
					if (temp_obj->in_room != NOWHERE)
						temp_obj->in_room = i;

			}
		}
		if (!found) {	/* Still not found, insert at top of table. */
			world.push_back(new(ROOM_DATA));

			room_copy(world[i], OLC_ROOM(d));

			world[i]->number = OLC_NUM(d);
			world[i]->func = NULL;

			room_num = i;
		}
		// Copy world table over to new one.
		top_of_world++;

// ПЕРЕИНДЕКСАЦИЯ

		// Update zone table.
		for (zone = 0; zone <= top_of_zone_table; zone++)
			for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
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

		/*
		 * Update load rooms, to fix creeping load room problem.
		 */
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
		for (temp_ch = character_list; temp_ch; temp_ch = temp_ch->next) {
			if (temp_ch->was_in_room >= room_num)
				temp_ch->was_in_room++;
		}

		// Порталы, выходы
		for (i = FIRST_ROOM; i < top_of_world + 1; i++) {
			if (world[i]->portal_room >= room_num)
				world[i]->portal_room++;
			for (j = 0; j < NUM_OF_DIRS; j++)
				if (W_EXIT(i, j))
					if (W_EXIT(i, j)->to_room >= room_num)
						W_EXIT(i, j)->to_room++;
		}

		/*
		 * Update any rooms being edited.
		 */
		for (dsc = descriptor_list; dsc; dsc = dsc->next)
			if (dsc->connected == CON_REDIT)
				for (j = 0; j < NUM_OF_DIRS; j++)
					if (OLC_ROOM(dsc)->dir_option[j])
						if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
							OLC_ROOM(dsc)->dir_option[j]->to_room++;
	}

	// пока мы не удаляем комнаты через олц - проблем нету
	if (ROOM_FLAGGED(room_num, ROOM_SLOWDEATH) || ROOM_FLAGGED(room_num, ROOM_ICEDEATH))
		DeathTrap::add(world[room_num]);
	else
		DeathTrap::remove(world[room_num]);

	// Настало время добавить триггеры
	SCRIPT(world[room_num]) = NULL;
	assign_triggers(world[room_num], WLD_TRIGGER);
	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ROOM);
}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(int zone_num)
{
	int counter, counter2, realcounter;
	FILE *fp;
	ROOM_DATA *room;
	EXTRA_DESCR_DATA *ex_desc;

	if (zone_num < 0 || zone_num > top_of_zone_table) {
		log("SYSERR: redit_save_to_disk: Invalid real zone passed!");
		return;
	}

	sprintf(buf, "%s/%d.new", WLD_PREFIX, zone_table[zone_num].number);
	if (!(fp = fopen(buf, "w+"))) {
		mudlog("SYSERR: OLC: Cannot open room file!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		return;
	}
	for (counter = zone_table[zone_num].number * 100; counter < zone_table[zone_num].top; counter++) {
		if ((realcounter = real_room(counter)) != NOWHERE) {
			if (counter % 100 == 99)
				continue;
			room = world[realcounter];

#if defined(REDIT_LIST)
			sprintf(buf1, "OLC: Saving room %d.", room->number);
			log(buf1);
#endif

			/*
			 * Remove the '\r\n' sequences from description.
			 */
			strcpy(buf1, RoomDescription::show_desc(room->description_num).c_str());
			strip_string(buf1);

			/*
			 * Forget making a buffer, lets just write the thing now.
			 */
			*buf2 = '\0';
			tascii(&room->room_flags.flags[0], 4, buf2);
			fprintf(fp, "#%d\n%s~\n%s~\n%d %s %d\n", counter,
				room->name ? room->name : "неопределено", buf1,
				zone_table[room->zone].number, buf2, room->sector_type);

			/*
			 * Handle exits.
			 */
			for (counter2 = 0; counter2 < NUM_OF_DIRS; counter2++) {
				if (room->dir_option[counter2]) {
					int temp_door_flag;

					/*
					 * Again, strip out the garbage.
					 */
					if (room->dir_option[counter2]->general_description) {
						strcpy(buf1, room->dir_option[counter2]->general_description);
						strip_string(buf1);
					} else
						*buf1 = 0;

					/*
					 * Figure out door flag.
					 */
					if (IS_SET(room->dir_option[counter2]->exit_info, EX_ISDOOR)) {
						if (IS_SET(room->dir_option[counter2]->exit_info, EX_PICKPROOF))
							temp_door_flag = 2;
						else
							temp_door_flag = 1;
					} else
						temp_door_flag = 0;
					if (IS_SET(room->dir_option[counter2]->exit_info, EX_HIDDEN))
						temp_door_flag |= 4;

					/*
					 * Check for keywords.
					 */
					if (room->dir_option[counter2]->keyword)
						strcpy(buf2, room->dir_option[counter2]->keyword);
					// алиас в винительном падеже пишется сюда же через ;
					if (room->dir_option[counter2]->vkeyword) {
						strcpy(buf2 + strlen(buf2), "|");
						strcpy(buf2 + strlen(buf2), room->dir_option[counter2]->vkeyword);
					}
					else
						*buf2 = '\0';

					/*
					 * Ok, now wrote output to file.
					 */
					fprintf(fp, "D%d\n%s~\n%s~\n%d %d %d\n",
						counter2, buf1, buf2,
						temp_door_flag, room->dir_option[counter2]->key,
						room->dir_option[counter2]->to_room != NOWHERE ?
						world[room->dir_option[counter2]->to_room]->number : NOWHERE);
				}
			}
			/*
			 * Home straight, just deal with extra descriptions.
			 */
			if (room->ex_description) {
				for (ex_desc = room->ex_description; ex_desc; ex_desc = ex_desc->next) {
					strcpy(buf1, ex_desc->description);
					strip_string(buf1);
					fprintf(fp, "E\n%s~\n%s~\n", ex_desc->keyword, buf1);
				}
			}
			fprintf(fp, "S\n");
			script_save_to_disk(fp, room, WLD_TRIGGER);
			im_inglist_save_to_disk(fp, room->ing_list);
		}
	}
	/*
	 * Write final line and close.
	 */
	fprintf(fp, "$\n$\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.wld", WLD_PREFIX, zone_table[zone_num].number);
	/*
	 * We're fubar'd if we crash between the two lines below.
	 */
	remove(buf2);
	rename(buf, buf2);

	olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_ROOM);
}


/**************************************************************************
 Menu functions
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(DESCRIPTOR_DATA * d)
{
	EXTRA_DESCR_DATA *extra_desc = OLC_DESC(d);

	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
		"%s1%s) Ключ: %s%s\r\n"
		"%s2%s) Описание:\r\n%s%s\r\n"
		"%s3%s) Следующее описание: ",
		grn, nrm, yel,
		extra_desc->keyword ? extra_desc->keyword : "<NONE>", grn, nrm,
		yel, extra_desc->description ? extra_desc->description : "<NONE>", grn, nrm);

	strcat(buf, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
	strcat(buf, "Enter choice (0 to quit) : ");
	send_to_char(buf, d->character);
	OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(DESCRIPTOR_DATA * d)
{
	/*
	 * if exit doesn't exist, alloc/create it
	 */
	if (!OLC_EXIT(d)) {
		CREATE(OLC_EXIT(d), EXIT_DATA, 1);
		OLC_EXIT(d)->to_room = NOWHERE;
	}

	/*
	 * Weird door handling!
	 */
	if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR)) {
		if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
			strcpy(buf2, "Невзламываемый");
		else
			strcpy(buf2, "Дверь");
	} else
		strcpy(buf2, "Нет двери");
	if (IS_SET(OLC_EXIT(d)->exit_info, EX_HIDDEN))
		strcat(buf2, ", выход скрыт");

	get_char_cols(d->character);
	sprintf(buf,
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
		OLC_EXIT(d)->to_room !=
		NOWHERE ? world[OLC_EXIT(d)->to_room]->number : NOWHERE, grn, nrm,
		yel,
		OLC_EXIT(d)->general_description ? OLC_EXIT(d)->
		general_description : "<NONE>", grn, nrm, yel,
		OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
		OLC_EXIT(d)->vkeyword ? OLC_EXIT(d)->vkeyword : "<NONE>", grn, nrm,
		cyn, OLC_EXIT(d)->key, grn, nrm, cyn, buf2, grn, nrm);

	send_to_char(buf, d->character);
	OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(DESCRIPTOR_DATA * d)
{
	get_char_cols(d->character);
	sprintf(buf,
		"%s1%s) [%c]Нет двери\r\n"
		"%s2%s) [%c]Закрываемая\r\n"
		"%s3%s) [%c]Невзламываемая\r\n"
		"%s4%s) [%c]Скрытая\r\n"
		"Ваш выбор (0 - выход): ",
		grn, nrm, !IS_SET(OLC_EXIT(d)->exit_info,
				  EX_ISDOOR | EX_PICKPROOF) ? 'x' : ' ', grn, nrm,
		IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR) ? 'x' : ' ', grn, nrm,
		IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF) ? 'x' : ' ', grn,
		nrm, IS_SET(OLC_EXIT(d)->exit_info, EX_HIDDEN) ? 'x' : ' ');
	send_to_char(buf, d->character);
}

/*
 * For room flags.
 */
void redit_disp_flag_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++) {
		if (*room_bits[counter] == '\n') {
			plane++;
			c = 'a' - 1;
			continue;
		} else if (c == 'z')
			c = 'A';
		else
			c++;

		sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
			room_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbits(OLC_ROOM(d)->room_flags, room_bits, buf1, ",");
	sprintf(buf, "\r\nФлаги комнаты: %s%s%s\r\n" "Введите флаг комнаты (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = REDIT_FLAGS;
}

/*
 * For sector type.
 */
void redit_disp_sector_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
			sector_types[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nТип поверхности в комнате : ", d->character);
	OLC_MODE(d) = REDIT_SECTOR;
}

/*
 * The main menu.
 */
void redit_disp_menu(DESCRIPTOR_DATA * d)
{
	ROOM_DATA *room;

	get_char_cols(d->character);
	room = OLC_ROOM(d);

	sprintbits(room->room_flags, room_bits, buf1, ",");
	sprinttype(room->sector_type, sector_types, buf2);
	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
		"-- Комната : [%s%d%s]  	Зона: [%s%d%s]\r\n"
		"%s1%s) Название    : %s%s\r\n"
		"%s2%s) Описание    :\r\n%s%s"
		"%s3%s) Флаги       : %s%s\r\n"
		"%s4%s) Поверхность : %s%s\r\n"
		"%s5%s) На севере   : %s%d\r\n"
		"%s6%s) На востоке  : %s%d\r\n"
		"%s7%s) На юге      : %s%d\r\n"
		"%s8%s) На западе   : %s%d\r\n"
		"%s9%s) Вверху      : %s%d\r\n"
		"%sA%s) Внизу       : %s%d\r\n"
		"%sB%s) Меню экстраописаний\r\n"
		"%sН%s) Ингредиенты : %s%s\r\n"
		"%sS%s) Скрипты     : %s%s\r\n"
		"%sQ%s) Quit\r\n"
		"Ваш выбор : ",
		cyn, OLC_NUM(d), nrm,
		cyn, zone_table[OLC_ZNUM(d)].number, nrm,
		grn, nrm, yel, room->name,
		grn, nrm, yel, room->temp_description,
		grn, nrm, cyn, buf1, grn, nrm, cyn, buf2, grn, nrm, cyn, room->dir_option[NORTH]
		&& room->dir_option[NORTH]->to_room !=
		NOWHERE ? world[room->dir_option[NORTH]->to_room]->
		number : NOWHERE, grn, nrm, cyn, room->dir_option[EAST]
		&& room->dir_option[EAST]->to_room !=
		NOWHERE ? world[room->dir_option[EAST]->to_room]->
		number : NOWHERE, grn, nrm, cyn, room->dir_option[SOUTH]
		&& room->dir_option[SOUTH]->to_room !=
		NOWHERE ? world[room->dir_option[SOUTH]->to_room]->
		number : NOWHERE, grn, nrm, cyn, room->dir_option[WEST]
		&& room->dir_option[WEST]->to_room !=
		NOWHERE ? world[room->dir_option[WEST]->to_room]->number : NOWHERE, grn, nrm, cyn, room->dir_option[UP]
		&& room->dir_option[UP]->to_room !=
		NOWHERE ? world[room->dir_option[UP]->to_room]->number : NOWHERE, grn, nrm, cyn, room->dir_option[DOWN]
		&& room->dir_option[DOWN]->to_room !=
		NOWHERE ? world[room->dir_option[DOWN]->to_room]->
		number : NOWHERE, grn, nrm, grn, nrm, cyn,
		room->ing_list ? "Есть" : "Нет", grn, nrm, cyn, room->proto_script ? "Set." : "Not Set.", grn, nrm);
	send_to_char(buf, d->character);

	OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	int number, plane, bit;

	switch (OLC_MODE(d)) {
	case REDIT_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
		case 'д':
		case 'Д':
			redit_save_internally(d);
			sprintf(buf, "OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
			olc_log("%s edit room %d", GET_NAME(d->character), OLC_NUM(d));
			mudlog(buf, NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			/*
			 * Do NOT free strings! Just the room structure.
			 */
			cleanup_olc(d, CLEANUP_STRUCTS);
			send_to_char("Room saved to memory.\r\n", d->character);
			break;
		case 'n':
		case 'N':
		case 'н':
		case 'Н':
			/*
			 * Free everything up, including strings, etc.
			 */
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			send_to_char("Неверный выбор!\r\nВы желаете сохранить комнату в памяти ? : ", d->character);
			break;
		}
		return;

	case REDIT_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {	/* Something has been modified. */
				send_to_char("Вы желаете сохранить комнату в памяти ? : ", d->character);
				OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '1':
			send_to_char("Введите название комнаты:-\r\n] ", d->character);
			OLC_MODE(d) = REDIT_NAME;
			break;
		case '2':
			OLC_MODE(d) = REDIT_DESC;
#if defined(CLEAR_SCREEN)
			SEND_TO_Q("\x1B[H\x1B[J", d);
#endif
			SEND_TO_Q("Введите описание комнаты: (/s записать /h помощь)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_ROOM(d)->temp_description) {
				SEND_TO_Q(OLC_ROOM(d)->temp_description, d);
				d->backstr = str_dup(OLC_ROOM(d)->temp_description);
			}
			d->str = &OLC_ROOM(d)->temp_description;
			d->max_str = MAX_ROOM_DESC;
			d->mail_to = 0;
			OLC_VAL(d) = 1;
			break;
		case '3':
			redit_disp_flag_menu(d);
			break;
		case '4':
			redit_disp_sector_menu(d);
			break;
		case '5':
			OLC_VAL(d) = NORTH;
			redit_disp_exit_menu(d);
			break;
		case '6':
			OLC_VAL(d) = EAST;
			redit_disp_exit_menu(d);
			break;
		case '7':
			OLC_VAL(d) = SOUTH;
			redit_disp_exit_menu(d);
			break;
		case '8':
			OLC_VAL(d) = WEST;
			redit_disp_exit_menu(d);
			break;
		case '9':
			OLC_VAL(d) = UP;
			redit_disp_exit_menu(d);
			break;
		case 'a':
		case 'A':
			OLC_VAL(d) = DOWN;
			redit_disp_exit_menu(d);
			break;
		case 'b':
		case 'B':
			/*
			 * If the extra description doesn't exist.
			 */
			if (!OLC_ROOM(d)->ex_description) {
				CREATE(OLC_ROOM(d)->ex_description, EXTRA_DESCR_DATA, 1);
				OLC_ROOM(d)->ex_description->next = NULL;
			}
			OLC_DESC(d) = OLC_ROOM(d)->ex_description;
			redit_disp_extradesc_menu(d);
			break;
		case 'h':
		case 'H':
		case 'н':
		case 'Н':
			OLC_MODE(d) = REDIT_ING;
			xedit_disp_ing(d, OLC_ROOM(d)->ing_list);
			return;
		case 's':
		case 'S':
			dg_olc_script_copy(d);
			OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
			dg_script_menu(d);
			return;
		default:
			send_to_char("Неверный выбор!", d->character);
			redit_disp_menu(d);
			break;
		}
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
		/*
		 * We will NEVER get here, we hope.
		 */
		mudlog("SYSERR: Reached REDIT_DESC case in parse_redit", BRF, LVL_BUILDER, SYSLOG, TRUE);
		break;

	case REDIT_FLAGS:
		number = planebit(arg, &plane, &bit);
		if (number < 0) {
			send_to_char("Неверный выбор !\r\n", d->character);
			redit_disp_flag_menu(d);
		} else if (number == 0)
			break;
		else {		/*
				 * Toggle the bit.
				 */
			TOGGLE_BIT(OLC_ROOM(d)->room_flags.flags[plane], (1 << bit));
			redit_disp_flag_menu(d);
		}
		return;

	case REDIT_SECTOR:
		number = atoi(arg);
		if (number < 0 || number >= NUM_ROOM_SECTORS) {
			send_to_char("Неверный выбор!", d->character);
			redit_disp_sector_menu(d);
			return;
		} else
			OLC_ROOM(d)->sector_type = number;
		break;

	case REDIT_EXIT_MENU:
		switch (*arg) {
		case '0':
			break;
		case '1':
			OLC_MODE(d) = REDIT_EXIT_NUMBER;
			send_to_char("Попасть в комнату N (vnum) : ", d->character);
			return;
		case '2':
			OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
			send_to_char("Введите описание выхода : ", d->character);
			return;
/*			SEND_TO_Q("Введите описание выхода: (/s сохранить /h помощь)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_EXIT(d)->general_description) {
				SEND_TO_Q(OLC_EXIT(d)->general_description, d);
				d->backstr = str_dup(OLC_EXIT(d)->general_description);
			}
			d->str = &OLC_EXIT(d)->general_description;
			d->max_str = MAX_EXIT_DESC;
			d->mail_to = 0;
*/
		case '3':
			OLC_MODE(d) = REDIT_EXIT_KEYWORD;
			send_to_char("Введите ключевое слово : ", d->character);
			return;
		case '4':
			OLC_MODE(d) = REDIT_EXIT_KEY;
			send_to_char("Введите номер ключа : ", d->character);
			return;
		case '5':
			redit_disp_exit_flag_menu(d);
			OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
			return;
		case '6':
			/*
			 * Delete an exit.
			 */
			if (OLC_EXIT(d)->keyword)
				free(OLC_EXIT(d)->keyword);
			if (OLC_EXIT(d)->vkeyword)
				free(OLC_EXIT(d)->vkeyword);
			if (OLC_EXIT(d)->general_description)
				free(OLC_EXIT(d)->general_description);
			if (OLC_EXIT(d))
				free(OLC_EXIT(d));
			OLC_EXIT(d) = NULL;
			break;
		default:
			send_to_char("Неверный выбор!\r\nВаш выбор : ", d->character);
			return;
		}
		break;

	case REDIT_EXIT_NUMBER:
		if ((number = atoi(arg)) != NOWHERE)
			if ((number = real_room(number)) == NOWHERE) {
				send_to_char("Нет такой комнаты - повторите ввод : ", d->character);
				return;
			}
		OLC_EXIT(d)->to_room = number;
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_DESCRIPTION:
		if (OLC_EXIT(d)->general_description)
			free(OLC_EXIT(d)->general_description);
		OLC_EXIT(d)->general_description = ((arg && *arg) ? str_dup(arg) : NULL);

		redit_disp_exit_menu(d);
		return;

		/*
		 * We should NEVER get here, hopefully.
		 */
/*
		mudlog("SYSERR: Reached REDIT_EXIT_DESC case in parse_redit", BRF, LVL_BUILDER, SYSLOG, TRUE);
		break;
*/
	case REDIT_EXIT_KEYWORD:
		if (OLC_EXIT(d)->keyword)
			free(OLC_EXIT(d)->keyword);
		if (OLC_EXIT(d)->vkeyword)
			free(OLC_EXIT(d)->vkeyword);

		if (arg && *arg) {
			std::string buffer(arg);
			std::string::size_type i = buffer.find('|');
			if (i != std::string::npos) {
				OLC_EXIT(d)->keyword = str_dup(buffer.substr(0,i).c_str());
				OLC_EXIT(d)->vkeyword = str_dup(buffer.substr(++i).c_str());
			} else {
				OLC_EXIT(d)->keyword = str_dup(buffer.c_str());
				OLC_EXIT(d)->vkeyword = str_dup(buffer.c_str());
			}
		} else {
			OLC_EXIT(d)->keyword = NULL;
			OLC_EXIT(d)->vkeyword = NULL;
		}
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_KEY:
		OLC_EXIT(d)->key = atoi(arg);
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_DOORFLAGS:
		number = atoi(arg);
		if ((number < 0) || (number > 4)) {
			send_to_char("Неверный выбор!\r\n", d->character);
			redit_disp_exit_flag_menu(d);
		} else if (number == 0)
			redit_disp_exit_menu(d);
		else {		/*
				 * Doors are a bit idiotic, don't you think? :) I agree.
				 */
			if (number == 1)
				REMOVE_BIT(OLC_EXIT(d)->exit_info, 3);
			else if (number == 2) {
				REMOVE_BIT(OLC_EXIT(d)->exit_info, EX_PICKPROOF);
				TOGGLE_BIT(OLC_EXIT(d)->exit_info, EX_ISDOOR);
			} else if (number == 3) {
				TOGGLE_BIT(OLC_EXIT(d)->exit_info, EX_PICKPROOF);
				if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
					SET_BIT(OLC_EXIT(d)->exit_info, EX_ISDOOR);
				else
					REMOVE_BIT(OLC_EXIT(d)->exit_info, EX_ISDOOR);
			} else
				TOGGLE_BIT(OLC_EXIT(d)->exit_info, EX_HIDDEN);
			redit_disp_exit_flag_menu(d);
		}
		return;

	case REDIT_EXTRADESC_KEY:
		OLC_DESC(d)->keyword = ((arg && *arg) ? str_dup(arg) : NULL);
		redit_disp_extradesc_menu(d);
		return;

	case REDIT_EXTRADESC_MENU:
		switch ((number = atoi(arg))) {
		case 0:
			{	/*
				 * If something got left out, delete the extra description
				 * when backing out to the menu.
				 */
				if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
					EXTRA_DESCR_DATA **tmp_desc;

					if (OLC_DESC(d)->keyword)
						free(OLC_DESC(d)->keyword);
					if (OLC_DESC(d)->description)
						free(OLC_DESC(d)->description);

					/*
					 * Clean up pointers.
					 */
					for (tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc;
					     tmp_desc = &((*tmp_desc)->next))
						if (*tmp_desc == OLC_DESC(d)) {
							*tmp_desc = NULL;
							break;
						}
					free(OLC_DESC(d));
				}
			}
			break;
		case 1:
			OLC_MODE(d) = REDIT_EXTRADESC_KEY;
			send_to_char("Введите ключевые слова, разделенные пробелами : ", d->character);
			return;
		case 2:
			OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
			SEND_TO_Q("Введите экстраописание: (/s сохранить /h помощь)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_DESC(d)->description) {
				SEND_TO_Q(OLC_DESC(d)->description, d);
				d->backstr = str_dup(OLC_DESC(d)->description);
			}
			d->str = &OLC_DESC(d)->description;
			d->max_str = 4096;
			d->mail_to = 0;
			return;

		case 3:
			if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
				send_to_char
				    ("Вы не можете редактировать следующее экстраописание, не завершив текущее.\r\n",
				     d->character);
				redit_disp_extradesc_menu(d);
			} else {
				EXTRA_DESCR_DATA *new_extra;

				if (OLC_DESC(d)->next)
					OLC_DESC(d) = OLC_DESC(d)->next;
				else {	/*
					 * Make new extra description and attach at end.
					 */
					CREATE(new_extra, EXTRA_DESCR_DATA, 1);
					OLC_DESC(d)->next = new_extra;
					OLC_DESC(d) = new_extra;
				}
				redit_disp_extradesc_menu(d);
			}
			return;
		}
		break;
	case REDIT_ING:
		if (!xparse_ing(d, &OLC_ROOM(d)->ing_list, arg)) {
			redit_disp_menu(d);
			return;
		}
		OLC_VAL(d) = 1;
		xedit_disp_ing(d, OLC_ROOM(d)->ing_list);
		return;

	default:
		/*
		 * We should never get here.
		 */
		mudlog("SYSERR: Reached default case in parse_redit", BRF, LVL_BUILDER, SYSLOG, TRUE);
		break;
	}
	/*
	 * If we get this far, something has been changed.
	 */
	OLC_VAL(d) = 1;
	redit_disp_menu(d);
}
