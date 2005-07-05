/* ************************************************************************
*   File: objsave.cpp                                     Part of Bylins    *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "im.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5
#define ITEM_DESTROYED  100


extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;
extern DESCRIPTOR_DATA *descriptor_list;
extern struct player_index_element *player_table;
extern OBJ_DATA *obj_proto;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_crashrent_period;
extern int free_rent;
//extern int max_obj_save;      /* change in config.cpp */
extern long last_rent_check;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

#define MAKESIZE(number) (sizeof(struct save_info) + sizeof(struct save_time_info) * number)
#define MAKEINFO(pointer, number) CREATE(pointer, char, MAKESIZE(number))
#define SAVEINFO(number) ((player_table+number)->timer)
#define RENTCODE(number) ((player_table+number)->timer->rent.rentcode)
#define SAVESIZE(number) (sizeof(struct save_info) +\
                          sizeof(struct save_time_info) * (player_table+number)->timer->rent.nitems)
#define GET_INDEX(ch) (get_ptable_by_name(GET_NAME(ch)))

/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int invalid_no_class(CHAR_DATA * ch, OBJ_DATA * obj);
int invalid_anti_class(CHAR_DATA * ch, OBJ_DATA * obj);
int invalid_unique(CHAR_DATA * ch, OBJ_DATA * obj);
int min_rent_cost(CHAR_DATA * ch);
void name_from_drinkcon(OBJ_DATA * obj);
void name_to_drinkcon(OBJ_DATA * obj, int type);
OBJ_DATA *create_obj(void);
void asciiflag_conv(char *flag, void *value);
void tascii(int *pointer, int num_planes, char *ascii);
int get_ptable_by_name(char *name);


/* local functions */
void Crash_extract_norent_eq(CHAR_DATA * ch);
int auto_equip(CHAR_DATA * ch, OBJ_DATA * obj, int location);
int Crash_offer_rent(CHAR_DATA * ch, CHAR_DATA * receptionist, int display, int factor, int *totalcost);
int Crash_report_unrentables(CHAR_DATA * ch, CHAR_DATA * recep, OBJ_DATA * obj);
void Crash_report_rent(CHAR_DATA * ch, CHAR_DATA * recep, OBJ_DATA * obj, int *cost, long *nitems, int display, int factor, int equip, int recursive);
void update_obj_file(void);
int Crash_write_rentcode(CHAR_DATA * ch, FILE * fl, struct save_rent_info *rent);
int gen_receptionist(CHAR_DATA * ch, CHAR_DATA * recep, int cmd, char *arg, int mode);
void Crash_save(int iplayer, OBJ_DATA * obj, int location);
void Crash_rent_deadline(CHAR_DATA * ch, CHAR_DATA * recep, long cost);
void Crash_restore_weight(OBJ_DATA * obj);
void Crash_extract_objs(OBJ_DATA * obj);
int Crash_is_unrentable(OBJ_DATA * obj);
void Crash_extract_norents(OBJ_DATA * obj);
void Crash_extract_expensive(OBJ_DATA * obj);
int Crash_calculate_rent(OBJ_DATA * obj);
int Crash_calculate_rent_eq(OBJ_DATA * obj);
int Crash_delete_files(int index);
int Crash_read_timer(int index, int temp);
void Crash_save_all_rent();
int Crash_calc_charmee_items (CHAR_DATA *ch);

#define DIV_CHAR  '#'
#define END_CHAR  '$'
#define END_LINE  '\n'
#define END_LINES '~'
#define COM_CHAR  '*'

int get_buf_line(char **source, char *target)
{
	char *otarget = target;
	int empty = TRUE;

	*target = '\0';
	for (; **source && **source != DIV_CHAR && **source != END_CHAR; (*source)++) {
		if (**source == END_LINE) {
			if (empty || *otarget == COM_CHAR) {
				target = otarget;
				*target = '\0';
				continue;
			}
			(*source)++;
			return (TRUE);
		}
		*target = **source;
		if (!isspace(*target++))
			empty = FALSE;
		*target = '\0';
	}
	return (FALSE);
}

int get_buf_lines(char **source, char *target)
{
	*target = '\0';

	for (; **source && **source != DIV_CHAR && **source != END_CHAR; (*source)++) {
		if (**source == END_LINES) {
			(*source)++;
			if (**source == END_LINE)
				(*source)++;
			return (TRUE);
		}
		*(target++) = **source;
		*target = '\0';
	}
	return (FALSE);
}

// Данная процедура выбирает предмет из буфера.
// с поддержкой нового формата вещей игроков [от 10.12.04].
OBJ_DATA *read_one_object_new(char **data, int *error)
{
	char buffer[MAX_STRING_LENGTH];
	char read_line[MAX_STRING_LENGTH];
	int t[1];
	int vnum;
	OBJ_DATA *object = NULL;
	EXTRA_DESCR_DATA *new_descr;

	*error = 1;
	// Станем на начало предмета
	for (; **data != DIV_CHAR; (*data)++)
		if (!**data || **data == END_CHAR)
			return (object);
	*error = 2;
	// Пропустим #
	(*data)++;
	// Считаем vnum предмета
	if (!get_buf_line(data, buffer))
		return (object);
	*error = 3;
	if (!(vnum = atoi(buffer)))
		return (object);

	// Если без прототипа, создаем новый. Иначе читаем прототип и возвращаем NULL,
	// если прототип не существует.
	if (vnum < 0) {
		object = create_obj();
		GET_OBJ_RNUM(object) = NOTHING;
		object->ex_description = NULL;
	} else {
		object = read_object(vnum, VIRTUAL);
		if (!object) {
			*error = 4;
			return NULL;
		}
	}
	// Далее найденные параметры присваиваем к прототипу
	while (get_buf_lines(data, buffer)) {
		tag_argument(buffer, read_line);

		if (read_line != NULL) {
			// Чтобы не портился прототип вещи некоторые параметры изменяются лишь у вещей с vnum < 0
			if (!strcmp(read_line, "Alia")) {
				*error = 6;
				GET_OBJ_ALIAS(object) = str_dup(buffer);
				object->name = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad0")) {
				*error = 7;
				GET_OBJ_PNAME(object, 0) = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad1")) {
				*error = 8;
				GET_OBJ_PNAME(object, 1) = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad2")) {
				*error = 9;
				GET_OBJ_PNAME(object, 2) = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad3")) {
				*error = 10;
				GET_OBJ_PNAME(object, 3) = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad4")) {
				*error = 11;
				GET_OBJ_PNAME(object, 4) = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad5")) {
				*error = 12;
				GET_OBJ_PNAME(object, 5) = str_dup(buffer);
			} else if (!strcmp(read_line, "Desc")) {
				*error = 13;
				GET_OBJ_DESC(object) = str_dup(buffer);
			} else if (!strcmp(read_line, "ADsc")) {
				*error = 14;
				GET_OBJ_ACT(object) = str_dup(buffer);
			} else if (!strcmp(read_line, "Lctn")) {
				*error = 5;
				object->worn_on = atoi(buffer);
			} else if (!strcmp(read_line, "Skil")) {
				*error = 15;
				GET_OBJ_SKILL(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Maxx")) {
				*error = 16;
				GET_OBJ_MAX(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Curr")) {
				*error = 17;
				GET_OBJ_CUR(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Mter")) {
				*error = 18;
				GET_OBJ_MATER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Sexx")) {
				*error = 19;
				GET_OBJ_SEX(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Tmer")) {
				*error = 20;
				GET_OBJ_TIMER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Spll")) {
				*error = 21;
				GET_OBJ_SPELL(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Levl")) {
				*error = 22;
				GET_OBJ_LEVEL(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Affs")) {
				*error = 23;
				asciiflag_conv(buffer, &GET_OBJ_AFFECTS(object));
			} else if (!strcmp(read_line, "Anti")) {
				*error = 24;
				asciiflag_conv(buffer, &GET_OBJ_ANTI(object));
			} else if (!strcmp(read_line, "Nofl")) {
				*error = 25;
				asciiflag_conv(buffer, &GET_OBJ_NO(object));
			} else if (!strcmp(read_line, "Extr")) {
				*error = 26;
				/* обнуляем старое extra_flags - т.к. asciiflag_conv только добавляет флаги */
				object->obj_flags.extra_flags.flags[0] = 0;
				object->obj_flags.extra_flags.flags[1] = 0;
				object->obj_flags.extra_flags.flags[2] = 0;
				object->obj_flags.extra_flags.flags[3] = 0;
				asciiflag_conv(buffer, &GET_OBJ_EXTRA(object, 0));
			} else if (!strcmp(read_line, "Wear")) {
				*error = 27;
				/* обнуляем старое wear_flags - т.к. asciiflag_conv только добавляет флаги */
				GET_OBJ_WEAR(object) = 0;
				asciiflag_conv(buffer, &GET_OBJ_WEAR(object));
			} else if (!strcmp(read_line, "Type")) {
				*error = 28;
				GET_OBJ_TYPE(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Val0")) {
				*error = 29;
				GET_OBJ_VAL(object, 0) = atoi(buffer);
			} else if (!strcmp(read_line, "Val1")) {
				*error = 30;
				GET_OBJ_VAL(object, 1) = atoi(buffer);
			} else if (!strcmp(read_line, "Val2")) {
				*error = 31;
				GET_OBJ_VAL(object, 2) = atoi(buffer);
			} else if (!strcmp(read_line, "Val3")) {
				*error = 32;
				GET_OBJ_VAL(object, 3) = atoi(buffer);
			} else if (!strcmp(read_line, "Weig")) {
				*error = 33;
				GET_OBJ_WEIGHT(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Cost")) {
				*error = 34;
				GET_OBJ_COST(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Rent")) {
				*error = 35;
				GET_OBJ_RENT(object) = atoi(buffer);
			} else if (!strcmp(read_line, "RntQ")) {
				*error = 36;
				GET_OBJ_RENTEQ(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Ownr")) {
				*error = 37;
				GET_OBJ_OWNER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Mker")) {
				*error = 38;
				GET_OBJ_MAKER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Prnt")) {
				*error = 39;
				GET_OBJ_PARENT(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Afc0")) {
				*error = 40;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[0].location = t[0];
				object->affected[0].modifier = t[1];
			} else if (!strcmp(read_line, "Afc1")) {
				*error = 41;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[1].location = t[0];
				object->affected[1].modifier = t[1];
			} else if (!strcmp(read_line, "Afc2")) {
				*error = 42;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[2].location = t[0];
				object->affected[2].modifier = t[1];
			} else if (!strcmp(read_line, "Afc3")) {
				*error = 43;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[3].location = t[0];
				object->affected[3].modifier = t[1];
			} else if (!strcmp(read_line, "Afc4")) {
				*error = 44;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[4].location = t[0];
				object->affected[4].modifier = t[1];
			} else if (!strcmp(read_line, "Afc5")) {
				*error = 45;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[5].location = t[0];
				object->affected[5].modifier = t[1];
			} else if (!strcmp(read_line, "Edes")) {
				*error = 46;
				CREATE(new_descr, EXTRA_DESCR_DATA, 1);
				new_descr->keyword = str_dup(buffer);
				if (!get_buf_lines(data, buffer)) {
					free(new_descr->keyword);
					free(new_descr);
					*error = 47;
					return (object);
				}
				new_descr->description = str_dup(buffer);
				new_descr->next = object->ex_description;
				object->ex_description = new_descr;
			} else {
				sprintf(buf, "WARNING: \"%s\" is not valid key for character items! [value=\"%s\"]",
					read_line, buffer);
				mudlog(buf, NRM, LVL_GRGOD, ERRLOG, TRUE);
			}
		}
	}
	*error = 0;

	// Проверить вес фляг и т.п.
	if (GET_OBJ_TYPE(object) == ITEM_DRINKCON || GET_OBJ_TYPE(object) == ITEM_FOUNTAIN) {
		if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object, 1))
			GET_OBJ_WEIGHT(object) = GET_OBJ_VAL(object, 1) + 5;
	}
	// Проверка на ингры
	if (GET_OBJ_TYPE(object) == ITEM_MING) {
		int err = im_assign_power(object);
		if (err)
			*error = 100 + err;
	}
	return (object);
}

// Данная процедура выбирает предмет из буфера
// ВНИМАНИЕ!!! эта функция используется только для чтения вещей персонажа,
// сохраненных в старом формате, для чтения нового формата применяется ф-ия read_one_object_new
OBJ_DATA *read_one_object(char **data, int *error)
{
	char buffer[MAX_STRING_LENGTH], f0[MAX_STRING_LENGTH], f1[MAX_STRING_LENGTH], f2[MAX_STRING_LENGTH];
	int vnum, i, j, t[5];
	OBJ_DATA *object = NULL;
	EXTRA_DESCR_DATA *new_descr;

	*error = 1;
	// Станем на начало предмета
	for (; **data != DIV_CHAR; (*data)++)
		if (!**data || **data == END_CHAR)
			return (object);
	*error = 2;
	// Пропустим #
	(*data)++;
	// Считаем vnum предмета
	if (!get_buf_line(data, buffer))
		return (object);
	*error = 3;
	if (!(vnum = atoi(buffer)))
		return (object);

	if (vnum < 0) {		// Предмет не имеет прототипа
		object = create_obj();
		GET_OBJ_RNUM(object) = NOTHING;
		*error = 4;
		if (!get_buf_lines(data, buffer))
			return (object);
		// Алиасы
		GET_OBJ_ALIAS(object) = str_dup(buffer);
		object->name = str_dup(buffer);
		// Падежи
		*error = 5;
		for (i = 0; i < NUM_PADS; i++) {
			if (!get_buf_lines(data, buffer))
				return (object);
			GET_OBJ_PNAME(object, i) = str_dup(buffer);
		}
		// Описание когда на земле
		*error = 6;
		if (!get_buf_lines(data, buffer))
			return (object);
		GET_OBJ_DESC(object) = str_dup(buffer);
		// Описание при действии
		*error = 7;
		if (!get_buf_lines(data, buffer))
			return (object);
		GET_OBJ_ACT(object) = str_dup(buffer);
	} else if (!(object = read_object(vnum, VIRTUAL))) {
		*error = 8;
		return (object);
	}

	*error = 9;
	if (!get_buf_line(data, buffer) || sscanf(buffer, " %s %d %d %d", f0, t + 1, t + 2, t + 3) != 4)
		return (object);
	asciiflag_conv(f0, &GET_OBJ_SKILL(object));
	GET_OBJ_MAX(object) = t[1];
	GET_OBJ_CUR(object) = t[2];
	GET_OBJ_MATER(object) = t[3];

	*error = 10;
	if (!get_buf_line(data, buffer) || sscanf(buffer, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4)
		return (object);
	GET_OBJ_SEX(object) = t[0];
	GET_OBJ_TIMER(object) = t[1];
	GET_OBJ_SPELL(object) = t[2];
	GET_OBJ_LEVEL(object) = t[3];

	*error = 11;
	if (!get_buf_line(data, buffer) || sscanf(buffer, " %s %s %s", f0, f1, f2) != 3)
		return (object);
	asciiflag_conv(f0, &GET_OBJ_AFFECTS(object));
	asciiflag_conv(f1, &GET_OBJ_ANTI(object));
	asciiflag_conv(f2, &GET_OBJ_NO(object));

	*error = 12;
	if (!get_buf_line(data, buffer) || sscanf(buffer, " %d %s %s", t, f1, f2) != 3)
		return (object);
	GET_OBJ_TYPE(object) = t[0];
	asciiflag_conv(f1, &GET_OBJ_EXTRA(object, 0));
	asciiflag_conv(f2, &GET_OBJ_WEAR(object));

	*error = 13;
	if (!get_buf_line(data, buffer) || sscanf(buffer, "%s %d %d %d", f0, t + 1, t + 2, t + 3) != 4)
		return (object);
	asciiflag_conv(f0, &GET_OBJ_VAL(object, 0));
	GET_OBJ_VAL(object, 1) = t[1];
	GET_OBJ_VAL(object, 2) = t[2];
	GET_OBJ_VAL(object, 3) = t[3];

	*error = 14;
	if (!get_buf_line(data, buffer) || sscanf(buffer, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4)
		return (object);
	GET_OBJ_WEIGHT(object) = t[0];
	GET_OBJ_COST(object) = t[1];
	GET_OBJ_RENT(object) = t[2];
	GET_OBJ_RENTEQ(object) = t[3];

	*error = 15;
	if (!get_buf_line(data, buffer) || sscanf(buffer, "%d %d", t, t + 1) != 2)
		return (object);
	object->worn_on = t[0];
	GET_OBJ_OWNER(object) = t[1];

	// Проверить вес фляг и т.п.
	if (GET_OBJ_TYPE(object) == ITEM_DRINKCON || GET_OBJ_TYPE(object) == ITEM_FOUNTAIN) {
		if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object, 1))
			GET_OBJ_WEIGHT(object) = GET_OBJ_VAL(object, 1) + 5;
	}

	object->ex_description = NULL;	// Exlude doubling ex_description !!!
	j = 0;

	for (;;) {
		if (!get_buf_line(data, buffer)) {
			*error = 0;
			if (GET_OBJ_TYPE(object) == ITEM_MING) {
				int err = im_assign_power(object);
				if (err)
					*error = 100 + err;
			}
			return (object);
		}
		switch (*buffer) {
		case 'E':
			CREATE(new_descr, EXTRA_DESCR_DATA, 1);
			if (!get_buf_lines(data, buffer)) {
				free(new_descr);
				*error = 16;
				return (object);
			}
			new_descr->keyword = str_dup(buffer);
			if (!get_buf_lines(data, buffer)) {
				free(new_descr->keyword);
				free(new_descr);
				*error = 17;
				return (object);
			}
			new_descr->description = str_dup(buffer);
			new_descr->next = object->ex_description;
			object->ex_description = new_descr;
			break;
		case 'A':
			if (j >= MAX_OBJ_AFFECT) {
				*error = 18;
				return (object);
			}
			if (!get_buf_line(data, buffer)) {
				*error = 19;
				return (object);
			}
			if (sscanf(buffer, " %d %d ", t, t + 1) == 2) {
				object->affected[j].location = t[0];
				object->affected[j].modifier = t[1];
				j++;
			}
			break;
		case 'M':
			// Вставляем сюда уникальный номер создателя
			if (!get_buf_line(data, buffer)) {
				*error = 20;
				return (object);
			}
			if (sscanf(buffer, " %d ", t) == 1) {
				GET_OBJ_MAKER(object) = t[0];
			}
			break;
		case 'P':
			// Вставляем сюда уникальный номер создателя
			if (!get_buf_line(data, buffer)) {
				*error = 21;
				return (object);
			}
			if (sscanf(buffer, " %d ", t) == 1) {
				int rnum;
				GET_OBJ_PARENT(object) = t[0];
				rnum = real_mobile(GET_OBJ_PARENT(object));
				if (rnum > -1) {
					trans_obj_name(object, &mob_proto[rnum]);
				}
			}
			break;

		default:
			break;
		}
	}
	*error = 22;
	return (object);
}

// shapirus: функция проверки наличия доп. описания в прототипе
inline bool proto_has_descr(EXTRA_DESCR_DATA * odesc, EXTRA_DESCR_DATA * pdesc)
{
	EXTRA_DESCR_DATA *desc;

	for (desc = pdesc; desc; desc = desc->next)
		if (!str_cmp(odesc->keyword, desc->keyword) && !str_cmp(odesc->description, desc->description))
			return TRUE;

	return FALSE;
}

// Данная процедура помещает предмет в буфер
// [ ИСПОЛЬЗУЕТСЯ В НОВОМ ФОРМАТЕ ВЕЩЕЙ ПЕРСОНАЖА ОТ 10.12.04 ]
void write_one_object(char **data, OBJ_DATA * object, int location)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	EXTRA_DESCR_DATA *descr;
//  EXTRA_DESCR_DATA *descr2;
	int count = 0, i, j;
	OBJ_DATA *proto;	// add by Pereplut

	// vnum
	count += sprintf(*data + count, "#%d\n", GET_OBJ_VNUM(object));
	// Положение в экипировке (сохраняем только если > 0)
	if (location)
		count += sprintf(*data + count, "Lctn: %d~\n", location);

	// Если у шмотки есть прототип то будем сохранять по обрезанной схеме, иначе
	// придется сохранять все статсы шмотки.
	if (GET_OBJ_VNUM(object) >= 0 && (proto = read_object(GET_OBJ_VNUM(object), VIRTUAL))) {
		// Алиасы
		if (str_cmp(GET_OBJ_ALIAS(object), GET_OBJ_ALIAS(proto)))
			count += sprintf(*data + count, "Alia: %s~\n", GET_OBJ_ALIAS(object));
		// Падежи
		for (i = 0; i < NUM_PADS; i++)
			if (str_cmp(GET_OBJ_PNAME(object, i), GET_OBJ_PNAME(proto, i)))
				count += sprintf(*data + count, "Pad%d: %s~\n", i, GET_OBJ_PNAME(object, i));
		// Описание когда на земле
		if (str_cmp(GET_OBJ_DESC(object), GET_OBJ_DESC(proto)))
			count +=
			    sprintf(*data + count, "Desc: %s~\n", GET_OBJ_DESC(object) ? GET_OBJ_DESC(object) : "");
		// Описание при действии
		if (str_cmp(GET_OBJ_ACT(object), GET_OBJ_ACT(proto)))
			count += sprintf(*data + count, "ADsc: %s~\n", GET_OBJ_ACT(object) ? GET_OBJ_ACT(object) : "");
		// Тренируемый скилл
		if (GET_OBJ_SKILL(object) != GET_OBJ_SKILL(proto))
			count += sprintf(*data + count, "Skil: %d~\n", GET_OBJ_SKILL(object));
		// Макс. прочность
		if (GET_OBJ_MAX(object) != GET_OBJ_MAX(proto))
			count += sprintf(*data + count, "Maxx: %d~\n", GET_OBJ_MAX(object));
		// Текущая прочность
		if (GET_OBJ_CUR(object) != GET_OBJ_CUR(proto))
			count += sprintf(*data + count, "Curr: %d~\n", GET_OBJ_CUR(object));
		// Материал
		if (GET_OBJ_MATER(object) != GET_OBJ_MATER(proto))
			count += sprintf(*data + count, "Mter: %d~\n", GET_OBJ_MATER(object));
		// Пол
		if (GET_OBJ_SEX(object) != GET_OBJ_SEX(proto))
			count += sprintf(*data + count, "Sexx: %d~\n", GET_OBJ_SEX(object));
		// Таймер
		if (GET_OBJ_TIMER(object) != GET_OBJ_TIMER(proto))
			count += sprintf(*data + count, "Tmer: %d~\n", GET_OBJ_TIMER(object));
		// Вызываемое заклинание
		if (GET_OBJ_SPELL(object) != GET_OBJ_SPELL(proto))
			count += sprintf(*data + count, "Spll: %d~\n", GET_OBJ_SPELL(object));
		// Уровень заклинания
		if (GET_OBJ_LEVEL(object) != GET_OBJ_LEVEL(proto))
			count += sprintf(*data + count, "Levl: %d~\n", GET_OBJ_LEVEL(object));
		// Наводимые аффекты
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_AFFECTS(object), 4, buf);
		tascii((int *) &GET_OBJ_AFFECTS(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Affs: %s~\n", buf);
		// Анти флаги
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_ANTI(object), 4, buf);
		tascii((int *) &GET_OBJ_ANTI(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Anti: %s~\n", buf);
		// Запрещающие флаги
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_NO(object), 4, buf);
		tascii((int *) &GET_OBJ_NO(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Nofl: %s~\n", buf);
		// Экстра флаги
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_EXTRA(object, 0), 4, buf);
		tascii((int *) &GET_OBJ_EXTRA(proto, 0), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Extr: %s~\n", buf);
		// Флаги слотов экипировки
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_WEAR(object), 4, buf);
		tascii((int *) &GET_OBJ_WEAR(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Wear: %s~\n", buf);
		// Тип предмета
		if (GET_OBJ_TYPE(object) != GET_OBJ_TYPE(proto))
			count += sprintf(*data + count, "Type: %d~\n", GET_OBJ_TYPE(object));
		// Значение 0, Значение 1, Значение 2, Значение 3.
		for (i = 0; i < 4; i++)
			if (GET_OBJ_VAL(object, i) != GET_OBJ_VAL(proto, i))
				count += sprintf(*data + count, "Val%d: %d~\n", i, GET_OBJ_VAL(object, i));
		// Вес
		if (GET_OBJ_WEIGHT(object) != GET_OBJ_WEIGHT(proto))
			count += sprintf(*data + count, "Weig: %d~\n", GET_OBJ_WEIGHT(object));
		// Цена
		if (GET_OBJ_COST(object) != GET_OBJ_COST(proto))
			count += sprintf(*data + count, "Cost: %d~\n", GET_OBJ_COST(object));
		// Рента (снято)
		if (GET_OBJ_RENT(object) != GET_OBJ_RENT(proto))
			count += sprintf(*data + count, "Rent: %d~\n", GET_OBJ_RENT(object));
		// Рента (одето)
		if (GET_OBJ_RENTEQ(object) != GET_OBJ_RENTEQ(proto))
			count += sprintf(*data + count, "RntQ: %d~\n", GET_OBJ_RENTEQ(object));
		// Владелец
		if (GET_OBJ_OWNER(object) && GET_OBJ_OWNER(object) != GET_OBJ_OWNER(proto))
			count += sprintf(*data + count, "Ownr: %d~\n", GET_OBJ_OWNER(object));
		// Создатель
		if (GET_OBJ_MAKER(object) && GET_OBJ_MAKER(object) != GET_OBJ_MAKER(proto))
			count += sprintf(*data + count, "Mker: %d~\n", GET_OBJ_MAKER(object));
		// Родитель
		if (GET_OBJ_PARENT(object) && GET_OBJ_PARENT(object) != GET_OBJ_PARENT(proto))
			count += sprintf(*data + count, "Prnt: %d~\n", GET_OBJ_PARENT(object));

		// Аффекты
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			if (object->affected[j].location
			    && object->affected[j].location != proto->affected[j].location
			    && object->affected[j].modifier != proto->affected[j].modifier)
				count += sprintf(*data + count, "Afc%d: %d %d~\n", j,
						 object->affected[j].location, object->affected[j].modifier);

		// Доп описания
		// shapirus: исправлена ошибка с несохранением, например, меток
		// на крафтеных луках
		for (descr = object->ex_description; descr; descr = descr->next) {
			if (proto_has_descr(descr, proto->ex_description))
				continue;
			count += sprintf(*data + count, "Edes: %s~\n%s~\n",
					 descr->keyword ? descr->keyword : "",
					 descr->description ? descr->description : "");
		}
		extract_obj(proto);
	} else			// Если у шмотки нет прототипа - придется сохранять ее целиком.
	{
		// Алиасы
		if (GET_OBJ_ALIAS(object))
			count += sprintf(*data + count, "Alia: %s~\n", GET_OBJ_ALIAS(object));
		// Падежи
		for (i = 0; i < NUM_PADS; i++)
			if (GET_OBJ_PNAME(object, i))
				count += sprintf(*data + count, "Pad%d: %s~\n", i, GET_OBJ_PNAME(object, i));
		// Описание когда на земле
		if (GET_OBJ_DESC(object))
			count +=
			    sprintf(*data + count, "Desc: %s~\n", GET_OBJ_DESC(object) ? GET_OBJ_DESC(object) : "");
		// Описание при действии
		if (GET_OBJ_ACT(object))
			count += sprintf(*data + count, "ADsc: %s~\n", GET_OBJ_ACT(object) ? GET_OBJ_ACT(object) : "");
		// Тренируемый скилл
		if (GET_OBJ_SKILL(object))
			count += sprintf(*data + count, "Skil: %d~\n", GET_OBJ_SKILL(object));
		// Макс. прочность
		if (GET_OBJ_MAX(object))
			count += sprintf(*data + count, "Maxx: %d~\n", GET_OBJ_MAX(object));
		// Текущая прочность
		if (GET_OBJ_CUR(object))
			count += sprintf(*data + count, "Curr: %d~\n", GET_OBJ_CUR(object));
		// Материал
		if (GET_OBJ_MATER(object))
			count += sprintf(*data + count, "Mter: %d~\n", GET_OBJ_MATER(object));
		// Пол
		if (GET_OBJ_SEX(object))
			count += sprintf(*data + count, "Sexx: %d~\n", GET_OBJ_SEX(object));
		// Таймер
		if (GET_OBJ_TIMER(object))
			count += sprintf(*data + count, "Tmer: %d~\n", GET_OBJ_TIMER(object));
		// Вызываемое заклинание
		if (GET_OBJ_SPELL(object))
			count += sprintf(*data + count, "Spll: %d~\n", GET_OBJ_SPELL(object));
		// Уровень заклинания
		if (GET_OBJ_LEVEL(object))
			count += sprintf(*data + count, "Levl: %d~\n", GET_OBJ_LEVEL(object));
		// Наводимые аффекты
		*buf = '\0';
		tascii((int *) &GET_OBJ_AFFECTS(object), 4, buf);
		count += sprintf(*data + count, "Affs: %s~\n", buf);
		// Анти флаги
		*buf = '\0';
		tascii((int *) &GET_OBJ_ANTI(object), 4, buf);
		count += sprintf(*data + count, "Anti: %s~\n", buf);
		// Запрещающие флаги
		*buf = '\0';
		tascii((int *) &GET_OBJ_NO(object), 4, buf);
		count += sprintf(*data + count, "Nofl: %s~\n", buf);
		// Экстра флаги
		*buf = '\0';
		tascii((int *) &GET_OBJ_EXTRA(object, 0), 4, buf);
		count += sprintf(*data + count, "Extr: %s~\n", buf);
		// Флаги слотов экипировки
		*buf = '\0';
		tascii((int *) &GET_OBJ_WEAR(object), 4, buf);
		count += sprintf(*data + count, "Wear: %s~\n", buf);
		// Тип предмета
		count += sprintf(*data + count, "Type: %d~\n", GET_OBJ_TYPE(object));
		// Значение 0, Значение 1, Значение 2, Значение 3.
		for (i = 0; i < 4; i++)
			if (GET_OBJ_VAL(object, i))
				count += sprintf(*data + count, "Val%d: %d~\n", i, GET_OBJ_VAL(object, i));
		// Вес
		if (GET_OBJ_WEIGHT(object))
			count += sprintf(*data + count, "Weig: %d~\n", GET_OBJ_WEIGHT(object));
		// Цена
		if (GET_OBJ_COST(object))
			count += sprintf(*data + count, "Cost: %d~\n", GET_OBJ_COST(object));
		// Рента (снято)
		if (GET_OBJ_RENT(object))
			count += sprintf(*data + count, "Rent: %d~\n", GET_OBJ_RENT(object));
		// Рента (одето)
		if (GET_OBJ_RENTEQ(object))
			count += sprintf(*data + count, "RntQ: %d~\n", GET_OBJ_RENTEQ(object));
		// Владелец
		if (GET_OBJ_OWNER(object))
			count += sprintf(*data + count, "Ownr: %d~\n", GET_OBJ_OWNER(object));
		// Создатель
		if (GET_OBJ_MAKER(object))
			count += sprintf(*data + count, "Mker: %d~\n", GET_OBJ_MAKER(object));
		// Родитель
		if (GET_OBJ_PARENT(object))
			count += sprintf(*data + count, "Prnt: %d~\n", GET_OBJ_PARENT(object));

		// Аффекты
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			if (object->affected[j].location)
				count += sprintf(*data + count, "Afc%d: %d %d~\n", j,
						 object->affected[j].location, object->affected[j].modifier);

		// Доп описания
		for (descr = object->ex_description; descr; descr = descr->next)
			count += sprintf(*data + count, "Edes: %s~\n%s~\n",
					 descr->keyword ? descr->keyword : "",
					 descr->description ? descr->description : "");
	}
	*data += count;
	**data = '\0';
}

int auto_equip(CHAR_DATA * ch, OBJ_DATA * obj, int location)
{
	int j;

	// Lots of checks...
	if (location > 0) {	// Was wearing it.
		switch (j = (location - 1)) {
		case WEAR_LIGHT:
			break;
		case WEAR_FINGER_R:
		case WEAR_FINGER_L:
			if (!CAN_WEAR(obj, ITEM_WEAR_FINGER))	// not fitting
				location = LOC_INVENTORY;
			break;
		case WEAR_NECK_1:
		case WEAR_NECK_2:
			if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
				location = LOC_INVENTORY;
			break;
		case WEAR_BODY:
			if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
				location = LOC_INVENTORY;
			break;
		case WEAR_HEAD:
			if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
				location = LOC_INVENTORY;
			break;
		case WEAR_LEGS:
			if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
				location = LOC_INVENTORY;
			break;
		case WEAR_FEET:
			if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
				location = LOC_INVENTORY;
			break;
		case WEAR_HANDS:
			if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
				location = LOC_INVENTORY;
			break;
		case WEAR_ARMS:
			if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
				location = LOC_INVENTORY;
			break;
		case WEAR_SHIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_ABOUT:
			if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
				location = LOC_INVENTORY;
			break;
		case WEAR_WAIST:
			if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
				location = LOC_INVENTORY;
			break;
		case WEAR_WRIST_R:
		case WEAR_WRIST_L:
			if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
				location = LOC_INVENTORY;
			break;
		case WEAR_WIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_HOLD:
			if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
				break;
			location = LOC_INVENTORY;
			break;
		case WEAR_BOTHS:
			if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
				break;
		default:
			location = LOC_INVENTORY;
		}

		if (location > 0) {	/* Wearable. */
			if (!GET_EQ(ch, j)) {	/*
						 * Check the characters's alignment to prevent them from being
						 * zapped through the auto-equipping.
						 */
				if (invalid_align(ch, obj) || invalid_anti_class(ch, obj) || invalid_no_class(ch, obj))
					location = LOC_INVENTORY;
				else {
					equip_char(ch, obj, j | 0x80 | 0x40);
					log("Equipped with %s %d", (obj)->short_description, j);
				}
			} else {	/* Oops, saved a player with double equipment? */
				char aeq[128];
				sprintf(aeq,
					"SYSERR: autoeq: '%s' already equipped in position %d.",
					GET_NAME(ch), location);
				mudlog(aeq, BRF, LVL_IMMORT, SYSLOG, TRUE);
				location = LOC_INVENTORY;
			}
		}
	}
	if (location <= 0)	/* Inventory */
		obj_to_char(obj, ch);
	return (location);
}

int Crash_delete_crashfile(CHAR_DATA * ch)
{
	int index;

	index = GET_INDEX(ch);
	if (index < 0)
		return FALSE;
	if (!SAVEINFO(index))
		Crash_delete_files(index);
	return TRUE;
}

int Crash_delete_files(int index)
{
	char filename[MAX_STRING_LENGTH + 1], name[MAX_NAME_LENGTH + 1];
	FILE *fl;
	int retcode = FALSE;

	if (index < 0)
		return retcode;

	strcpy(name, player_table[index].name);

	/*удаляем файл описания объектов */
	if (!get_filename(name, filename, TEXT_CRASH_FILE)) {
		log("SYSERR: Error deleting objects file for %s - unable to resolve file name.", name);
		retcode = FALSE;
	} else {
		if (!(fl = fopen(filename, "rb"))) {
			if (errno != ENOENT)	/* if it fails but NOT because of no file */
				log("SYSERR: Error deleting objects file %s (1): %s", filename, strerror(errno));
			retcode = FALSE;
		} else {
			fclose(fl);
			/* if it fails, NOT because of no file */
			if (remove(filename) < 0 && errno != ENOENT) {
				log("SYSERR: Error deleting objects file %s (2): %s", filename, strerror(errno));
				retcode = FALSE;
			}
		}
	}

	/*удаляем файл таймеров */
	if (!get_filename(name, filename, TIME_CRASH_FILE)) {
		log("SYSERR: Error deleting timer file for %s - unable to resolve file name.", name);
		retcode = FALSE;
	} else {
		if (!(fl = fopen(filename, "rb"))) {
			if (errno != ENOENT)	/* if it fails but NOT because of no file */
				log("SYSERR: deleting timer file %s (1): %s", filename, strerror(errno));
			retcode = FALSE;
		} else {
			fclose(fl);
			/* if it fails, NOT because of no file */
			if (remove(filename) < 0 && errno != ENOENT) {
				log("SYSERR: deleting timer file %s (2): %s", filename, strerror(errno));
				retcode = FALSE;
			}
		}
	}
	return (retcode);
}

/********* Timer utils: create, read, write, list, timer_objects *********/

void Crash_clear_objects(int index)
{
	int i = 0, rnum;
	Crash_delete_files(index);
	if (SAVEINFO(index)) {
		for (; i < SAVEINFO(index)->rent.nitems; i++)
			if (SAVEINFO(index)->time[i].timer >= 0 &&
			    (rnum = real_object(SAVEINFO(index)->time[i].vnum)) >= 0)
				obj_index[rnum].stored--;
		free(SAVEINFO(index));
		SAVEINFO(index) = NULL;
	}
}

void Crash_reload_timer(int index)
{
	int i = 0, rnum;
	if (SAVEINFO(index)) {
		for (; i < SAVEINFO(index)->rent.nitems; i++)
			if (SAVEINFO(index)->time[i].timer >= 0 &&
			    (rnum = real_object(SAVEINFO(index)->time[i].vnum)) >= 0)
				obj_index[rnum].stored--;
		free(SAVEINFO(index));
		SAVEINFO(index) = NULL;
	}

	if (!Crash_read_timer(index, FALSE)) {
		sprintf(buf, "SYSERR: Unable to read timer file for %s.", player_table[index].name);
		mudlog(buf, BRF, MAX(LVL_IMMORT, LVL_GOD), SYSLOG, TRUE);
	}

}

void Crash_create_timer(int index, int num)
{
	if (SAVEINFO(index))
		free(SAVEINFO(index));

	if (((sizeof(struct save_info) + sizeof(struct save_time_info) * num)) * sizeof(char) <= 0)
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);
	if (!((((player_table+index)->timer)) = (save_info *) calloc (((sizeof(struct save_info)
	+ sizeof(struct save_time_info) * num)), sizeof(char)))) {
		perror("SYSERR: malloc failure");
		abort();
	}

//	MAKEINFO((char *) SAVEINFO(index), num);
	memset(SAVEINFO(index), 0, MAKESIZE(num));
}

int Crash_write_timer(int index)
{
	FILE *fl;
	char fname[MAX_STRING_LENGTH];
	char name[MAX_NAME_LENGTH + 1];

	strcpy(name, player_table[index].name);
	if (!SAVEINFO(index)) {
		log("SYSERR: Error writing %s timer file - no data.", name);
		return FALSE;
	}
	if (!get_filename(name, fname, TIME_CRASH_FILE)) {
		log("SYSERR: Error writing %s timer file - unable to resolve file name.", name);
		return FALSE;
	}
	if (!(fl = fopen(fname, "wb"))) {
		log("[WriteTimer] Error writing %s timer file - unable to open file %s.", name, fname);
		return FALSE;
	}
	fwrite(SAVEINFO(index), SAVESIZE(index), 1, fl);
	fclose(fl);
	return TRUE;
}

int Crash_read_timer(int index, int temp)
{
	FILE *fl;
	char fname[MAX_INPUT_LENGTH];
	char name[MAX_NAME_LENGTH + 1];
	int size = 0, count = 0, rnum, num = 0;
	struct save_rent_info rent;
	struct save_time_info info;

	strcpy(name, player_table[index].name);
	if (!get_filename(name, fname, TIME_CRASH_FILE)) {
		log("[ReadTimer] Error reading %s timer file - unable to resolve file name.", name);
		return FALSE;
	}
	if (!(fl = fopen(fname, "rb"))) {
		if (errno != ENOENT) {
			log("SYSERR: fatal error opening timer file %s", fname);
			return FALSE;
		} else
			return TRUE;
	}

	fseek(fl, 0L, SEEK_END);
	size = ftell(fl);
	rewind(fl);
	if ((size = size - sizeof(struct save_rent_info)) < 0 || size % sizeof(struct save_time_info)) {
		log("WARNING:  Timer file %s is corrupt!", fname);
		return FALSE;
	}

	sprintf(buf, "[ReadTimer] Reading timer file %s for %s :", fname, name);
	fread(&rent, sizeof(struct save_rent_info), 1, fl);
	switch (rent.rentcode) {
	case RENT_RENTED:
		strcat(buf, " Rent ");
		break;
	case RENT_CRASH:
//           if (rent.time<1001651000L) //креш-сейв до Sep 28 00:26:20 2001
		rent.net_cost_per_diem = 0;	//бесплатно!
		strcat(buf, " Crash ");
		break;
	case RENT_CRYO:
		strcat(buf, " Cryo ");
		break;
	case RENT_TIMEDOUT:
		strcat(buf, " TimedOut ");
		break;
	case RENT_FORCED:
		strcat(buf, " Forced ");
		break;
	default:
		log("[ReadTimer] Error reading %s timer file - undefined rent code.", name);
		return FALSE;
		//strcat(buf, " Undef ");
		//rent.rentcode = RENT_CRASH;
		break;
	}
	strcat(buf, "rent code.");
	log(buf);
	Crash_create_timer(index, rent.nitems);
	player_table[index].timer->rent = rent;
	for (; count < rent.nitems && !feof(fl); count++) {
		fread(&info, sizeof(struct save_time_info), 1, fl);
		if (ferror(fl)) {
			log("SYSERR: I/O Error reading %s timer file.", name);
			fclose(fl);
			free(SAVEINFO(index));
			SAVEINFO(index) = NULL;
			return FALSE;
		}
		if (info.vnum && info.timer >= -1)
			player_table[index].timer->time[num++] = info;
		else
			log("[ReadTimer] Warning: incorrect vnum (%d) or timer (%d) while reading %s timer file.",
			    info.vnum, info.timer, name);

		if (info.timer >= 0 && (rnum = real_object(info.vnum)) >= 0 && !temp)
			obj_index[rnum].stored++;
	}
	fclose(fl);
	if (rent.nitems != num) {
		log("[ReadTimer] Error reading %s timer file - file is corrupt.", fname);
		free(SAVEINFO(index));
		SAVEINFO(index) = NULL;
		return FALSE;
	} else
		return TRUE;
}

void Crash_timer_obj(int index, long time)
{
	char name[MAX_NAME_LENGTH + 1];
	int nitems = 0, idelete = 0, ideleted = 0, rnum, timer, i;
	int rentcode, timer_dec;


#ifndef USE_AUTOEQ
	return;
#endif

	strcpy(name, player_table[index].name);

	if (!player_table[index].timer) {
		log("[TO] %s has no save data.", name);
		return;
	}
	rentcode = SAVEINFO(index)->rent.rentcode;
	timer_dec = time - SAVEINFO(index)->rent.time;

	/*удаляем просроченные файлы ренты */
	if (rentcode == RENT_RENTED && timer_dec > rent_file_timeout * SECS_PER_REAL_DAY) {
		Crash_clear_objects(index);
		log("[TO] Deleting %s's rent info - time outed.", name);
		return;
	} else if (rentcode != RENT_CRYO && timer_dec > crash_file_timeout * SECS_PER_REAL_DAY) {
		Crash_clear_objects(index);
		strcpy(buf, "");
		switch (rentcode) {
		case RENT_CRASH:
			log("[TO] Deleting crash rent info for %s  - time outed.", name);
			break;
		case RENT_FORCED:
			log("[TO] Deleting forced rent info for %s  - time outed.", name);
			break;
		case RENT_TIMEDOUT:
			log("[TO] Deleting autorent info for %s  - time outed.", name);
			break;
		default:
			log("[TO] Deleting UNKNOWN rent info for %s  - time outed.", name);
			break;
		}
		return;
	}

	timer_dec = (timer_dec / SECS_PER_MUD_HOUR) + (timer_dec % SECS_PER_MUD_HOUR ? 1 : 0);

	/*уменьшаем таймеры */
	nitems = player_table[index].timer->rent.nitems;
//  log("[TO] Checking items for %s (%d items, rented time %dmin):",
//      name, nitems, timer_dec);
	//sprintf (buf,"[TO] Checking items for %s (%d items) :", name, nitems);
	//mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
	for (i = 0; i < nitems; i++)
		if (player_table[index].timer->time[i].timer >= 0) {
			rnum = real_object(player_table[index].timer->time[i].vnum);
			timer = player_table[index].timer->time[i].timer;
			if (timer < timer_dec) {
				player_table[index].timer->time[i].timer = -1;
				idelete++;
				if (rnum >= 0) {
					obj_index[rnum].stored--;
					log("[TO] Player %s : item %s deleted - time outted", name,
					    (obj_proto + rnum)->PNames[0]);
				}
			}
		} else
			ideleted++;

//  log("Objects (%d), Deleted (%d)+(%d).", nitems, ideleted, idelete);

	/*если появились новые просроченные объекты, обновляем файл таймеров */
	if (idelete)
		if (!Crash_write_timer(index)) {
			sprintf(buf, "SYSERR: [TO] Error writing timer file for %s.", name);
			mudlog(buf, CMP, MAX(LVL_IMMORT, LVL_GOD), SYSLOG, TRUE);
		}
}

void Crash_list_objects(CHAR_DATA * ch, int index)
{
	int i = 0, rnum;
	struct save_time_info data;
	long timer_dec;
	float num_of_days;

	if (!SAVEINFO(index))
		return;

	timer_dec = time(0) - SAVEINFO(index)->rent.time;
	num_of_days = (float) timer_dec / SECS_PER_REAL_DAY;
	timer_dec = (timer_dec / SECS_PER_MUD_HOUR) + (timer_dec % SECS_PER_MUD_HOUR ? 1 : 0);

	strcpy(buf, "Код ренты - ");
	switch (SAVEINFO(index)->rent.rentcode) {
	case RENT_RENTED:
		strcat(buf, "Rented.\r\n");
		break;
	case RENT_CRASH:
		strcat(buf, "Crash.\r\n");
		break;
	case RENT_CRYO:
		strcat(buf, "Cryo.\r\n");
		break;
	case RENT_TIMEDOUT:
		strcat(buf, "TimedOut.\r\n");
		break;
	case RENT_FORCED:
		strcat(buf, "Forced.\r\n");
		break;
	default:
		strcat(buf, "UNDEF!\r\n");
		break;
	}
	for (; i < SAVEINFO(index)->rent.nitems; i++) {
		data = SAVEINFO(index)->time[i];
		if ((rnum = real_object(data.vnum)) > -1) {
			sprintf(buf + strlen(buf), " [%5d] (%5dau) <%6d> %-20s\r\n",
				data.vnum, GET_OBJ_RENT(obj_proto + rnum),
				MAX(-1, data.timer - timer_dec), (obj_proto + rnum)->short_description);
		} else {
			sprintf(buf + strlen(buf), " [%5d] (?????au) <%2d> %-20s\r\n",
				data.vnum, MAX(-1, data.timer - timer_dec), "БЕЗ ПРОТОТИПА");
		}
		if (strlen(buf) > MAX_STRING_LENGTH - 80) {
			strcat(buf, "** Excessive rent listing. **\r\n");
			break;
		}
	}
	send_to_char(buf, ch);
	sprintf(buf, "Время в ренте: %ld тиков.\r\n", timer_dec);
	send_to_char(buf, ch);
	sprintf(buf,
		"Предметов: %d. Стоимость: (%d в день) * (%1.2f дней) = %d.\r\n",
		SAVEINFO(index)->rent.nitems,
		SAVEINFO(index)->rent.net_cost_per_diem, num_of_days,
		(int) (num_of_days * SAVEINFO(index)->rent.net_cost_per_diem));
	send_to_char(buf, ch);
}

void Crash_listrent(CHAR_DATA * ch, char *name)
{
	int index;

	index = get_ptable_by_name(name);

	if (index < 0) {
		send_to_char("Нет такого игрока.\r\n", ch);
		return;
	}

	if (!SAVEINFO(index))
		if (!Crash_read_timer(index, TRUE)) {
			sprintf(buf, "Ubable to read %s timer file.\r\n", name);
			send_to_char(buf, ch);
		} else if (!SAVEINFO(index)) {
			sprintf(buf, "%s не имеет файла ренты.\r\n", CAP(name));
			send_to_char(buf, ch);
		} else {
			sprintf(buf, "%s находится в игре. Содержимое файла ренты:\r\n", CAP(name));
			send_to_char(buf, ch);
			Crash_list_objects(ch, index);
			free(SAVEINFO(index));
			SAVEINFO(index) = NULL;
	} else {
		sprintf(buf, "%s находится в ренте. Содержимое файла ренты:\r\n", CAP(name));
		send_to_char(buf, ch);
		Crash_list_objects(ch, index);
	}
}

struct container_list_type {
	OBJ_DATA *tank;
	struct container_list_type *next;
	int location;
};

// разобраться с возвращаемым значением
/*******************  load_char_objects ********************/
int Crash_load(CHAR_DATA * ch)
{
	FILE *fl;
	char fname[MAX_STRING_LENGTH], *data, *readdata;
	int cost, num_objs = 0, reccount, fsize, error, index;
	float num_of_days;
	OBJ_DATA *obj, *obj2, *obj_list = NULL;
	int location, rnum;
	struct container_list_type *tank_list = NULL, *tank, *tank_to;
	long timer_dec;
	bool need_convert_character_objects = 0;	// add by Pereplut

	if ((index = GET_INDEX(ch)) < 0)
		return (1);

	Crash_reload_timer(index);

	if (!SAVEINFO(index)) {
		sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		return (1);
	}

	switch (RENTCODE(index)) {
	case RENT_RENTED:
		sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
		break;
	case RENT_CRASH:
		sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
		break;
	case RENT_CRYO:
		sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
		break;
	case RENT_FORCED:
		sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
		break;
	case RENT_TIMEDOUT:
		sprintf(buf, "%s retrieving auto-saved items and entering game.", GET_NAME(ch));
		break;
	default:
		sprintf(buf, "SYSERR: %s entering game with undefined rent code %d.", GET_NAME(ch), RENTCODE(index));
		mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		send_to_char("\r\n** Неизвестный код ренты **\r\n"
			     "Проблемы с восстановлением Ваших вещей из файла.\r\n"
			     "Обращайтесь за помощью к Богам.\r\n", ch);
		Crash_clear_objects(index);
		return (1);
		break;
	}
	mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);

	/*Деньги за постой */
	num_of_days = (float) (time(0) - SAVEINFO(index)->rent.time) / SECS_PER_REAL_DAY;
	sprintf(buf, "%s was %1.2f days in rent.", GET_NAME(ch), num_of_days);
	mudlog(buf, LGH, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
	cost = (int) (SAVEINFO(index)->rent.net_cost_per_diem * num_of_days);
	cost = MAX(0, cost);
	if ((RENTCODE(index) == RENT_CRASH || RENTCODE(index) == RENT_FORCED) && SAVEINFO(index)->rent.time + free_crashrent_period * SECS_PER_REAL_HOUR > time(0)) {	/*Бесплатная рента, если выйти в течение 2 часов после ребута или креша */
		sprintf(buf, "%s** На сей раз постой был бесплатным **%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		sprintf(buf, "%s entering game, free crashrent.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
	} else if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
		sprintf(buf, "%sВы находились на постое %1.2f дней.\n\r"
			"%s"
			"Вам предъявили счет на %d %s за постой (%d %s в день).\r\n"
			"Но все, что у Вас было - %ld %s... Увы. Все Ваши вещи переданы мобам.%s\n\r",
			CCWHT(ch, C_NRM),
			num_of_days,
			RENTCODE(index) ==
			RENT_TIMEDOUT ?
			"Вас пришлось тащить до кровати, за это постой был дороже.\r\n"
			: "", cost, desc_count(cost, WHAT_MONEYu),
			SAVEINFO(index)->rent.net_cost_per_diem,
			desc_count(SAVEINFO(index)->rent.net_cost_per_diem,
				   WHAT_MONEYa), GET_GOLD(ch) + GET_BANK_GOLD(ch),
			desc_count(GET_GOLD(ch) + GET_BANK_GOLD(ch), WHAT_MONEYa), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		sprintf(buf, "%s: rented equipment lost (no $).", GET_NAME(ch));
		mudlog(buf, LGH, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		GET_GOLD(ch) = GET_BANK_GOLD(ch) = 0;
		Crash_clear_objects(index);
		return (2);
	} else {
		if (cost) {
			sprintf(buf, "%sВы находились на постое %1.2f дней.\n\r"
				"%s"
				"С Вас содрали %d %s за постой (%d %s в день).%s\r\n",
				CCWHT(ch, C_NRM),
				num_of_days,
				RENTCODE(index) ==
				RENT_TIMEDOUT ?
				"Вас пришлось тащить до кровати, за это постой был дороже.\r\n"
				: "", cost, desc_count(cost, WHAT_MONEYu),
				SAVEINFO(index)->rent.net_cost_per_diem,
				desc_count(SAVEINFO(index)->rent.net_cost_per_diem, WHAT_MONEYa), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		GET_GOLD(ch) -= MAX(cost - GET_BANK_GOLD(ch), 0);
		GET_BANK_GOLD(ch) = MAX(GET_BANK_GOLD(ch) - cost, 0);
		//???
		//save_char(ch, NOWHERE); 
	}

	/*Чтение описаний объектов в буфер */
	if (!get_filename(GET_NAME(ch), fname, TEXT_CRASH_FILE) || !(fl = fopen(fname, "r+b"))) {
		send_to_char("\r\n** Нет файла описания вещей **\r\n"
			     "Проблемы с восстановлением Ваших вещей из файла.\r\n"
			     "Обращайтесь за помощью к Богам.\r\n", ch);
		Crash_clear_objects(index);
		return (1);
	}
	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);
	if (!fsize) {
		fclose(fl);
		send_to_char("\r\n** Файл описания вещей пустой **\r\n"
			     "Проблемы с восстановлением Ваших вещей из файла.\r\n"
			     "Обращайтесь за помощью к Богам.\r\n", ch);
		Crash_clear_objects(index);
		return (1);
	}

	CREATE(readdata, char, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	if (!fread(readdata, fsize, 1, fl) || ferror(fl) || !readdata) {
		fclose(fl);
		send_to_char("\r\n** Ошибка чтения файла описания вещей **\r\n"
			     "Проблемы с восстановлением Ваших вещей из файла.\r\n"
			     "Обращайтесь за помощью к Богам.\r\n", ch);
		log("Memory error or cann't read %s(%d)...", fname, fsize);
		free(readdata);
		Crash_clear_objects(index);
		return (1);
	};
	fclose(fl);

	data = readdata;
	*(data + fsize) = '\0';

	// Проверка в каком формате записана информация о персонаже.
	if (!strn_cmp(readdata, "@", 1))
		need_convert_character_objects = 1;

	/*Создание объектов */
	timer_dec = time(0) - SAVEINFO(index)->rent.time;
	timer_dec = (timer_dec / SECS_PER_MUD_HOUR) + (timer_dec % SECS_PER_MUD_HOUR ? 1 : 0);

	for (fsize = 0, reccount = SAVEINFO(index)->rent.nitems;
	     reccount > 0 && *data && *data != END_CHAR; reccount--, fsize++) {
		if (need_convert_character_objects) {
			// Формат новый => используем новую функцию
			if ((obj = read_one_object_new(&data, &error)) == NULL) {
				//send_to_char ("Ошибка при чтении - чтение предметов прервано.\r\n", ch);
				send_to_char("Ошибка при чтении файла объектов.\r\n", ch);
				sprintf(buf, "SYSERR: Objects reading fail for %s error %d, stop reading.",
					GET_NAME(ch), error);
				mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
				//break; 
				continue;	//Ann
			}
		} else {
			// Формат старый => используем старую функцию
			if ((obj = read_one_object(&data, &error)) == NULL) {
				//send_to_char ("Ошибка при чтении - чтение предметов прервано.\r\n", ch);
				send_to_char("Ошибка при чтении файла объектов.\r\n", ch);
				sprintf(buf, "SYSERR: Objects reading fail for %s error %d, stop reading.",
					GET_NAME(ch), error);
				mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
				//break; 
				continue;	//Ann
			}
		}
		if (error) {
			sprintf(buf, "WARNING: Error #%d reading item #%d from %s.", error, num_objs, fname);
			mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
		}

		if (GET_OBJ_VNUM(obj) != SAVEINFO(index)->time[fsize].vnum) {
			send_to_char("Нет соответствия заголовков - чтение предметов прервано.\r\n", ch);
			sprintf(buf, "SYSERR: Objects reading fail for %s (2), stop reading.", GET_NAME(ch));
			mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
			extract_obj(obj);
			break;
		}

		/*Check timers */
		if (SAVEINFO(index)->time[fsize].timer > 0 &&
		    (rnum = real_object(SAVEINFO(index)->time[fsize].vnum)) >= 0)
			obj_index[rnum].stored--;
		GET_OBJ_TIMER(obj) = MAX(-1, SAVEINFO(index)->time[fsize].timer - timer_dec);

		// Предмет разваливается от старости
		if (GET_OBJ_TIMER(obj) <= 0) {
			sprintf(buf, "%s%s рассыпал%s от длительного использования.\r\n",
				CCWHT(ch, C_NRM), CAP(obj->PNames[0]), GET_OBJ_SUF_2(obj));
			send_to_char(buf, ch);
			extract_obj(obj);
			continue;
		}
		//очищаем ZoneDecay объедки
		if (OBJ_FLAGGED(obj, ITEM_ZONEDECAY)) {
			sprintf(buf, "%s рассыпал%s в прах.\r\n", CAP(obj->PNames[0]), GET_OBJ_SUF_2(obj));
			send_to_char(buf, ch);
			extract_obj(obj);
			continue;
		}
		// Check valid class
		if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj)) {
			sprintf(buf, "%s рассыпал%s, как запрещенн%s для Вас.\r\n",
				CAP(obj->PNames[0]), GET_OBJ_SUF_2(obj), GET_OBJ_SUF_3(obj));
			send_to_char(buf, ch);
			extract_obj(obj);
			continue;
		}
		obj->next_content = obj_list;
		obj_list = obj;
	}

	free(readdata);

	for (obj = obj_list; obj; obj = obj2) {
		obj2 = obj->next_content;
		obj->next_content = NULL;
		if (obj->worn_on >= 0) {	// Equipped or in inventory
			if (obj2 && obj2->worn_on < 0 && GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {	// This is container and it is not free
				CREATE(tank, struct container_list_type, 1);
				tank->next = tank_list;
				tank->tank = obj;
				tank->location = 0;
				tank_list = tank;
			} else {
				while (tank_list) {	// Clear tanks list
					tank = tank_list;
					tank_list = tank->next;
					free(tank);
				}
			}
			location = obj->worn_on;
			obj->worn_on = 0;

			auto_equip(ch, obj, location);
		} else {
			if (obj2 && obj2->worn_on < obj->worn_on && GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {	// This is container and it is not free
				tank_to = tank_list;
				CREATE(tank, struct container_list_type, 1);
				tank->next = tank_list;
				tank->tank = obj;
				tank->location = obj->worn_on;
				tank_list = tank;
			} else {
				while ((tank_to = tank_list))
					// Clear all tank than less or equal this object location
					if (tank_list->location > obj->worn_on)
						break;
					else {
						tank = tank_list;
						tank_list = tank->next;
						free(tank);
					}
			}
			obj->worn_on = 0;
			if (tank_to)
				obj_to_obj(obj, tank_to->tank);
			else
				obj_to_char(obj, ch);
		}
	}
	while (tank_list) {	//Clear tanks list
		tank = tank_list;
		tank_list = tank->next;
		free(tank);
	}
	affect_total(ch);
	free(SAVEINFO(index));
	SAVEINFO(index) = NULL;
	//???
	//Crash_crashsave();
	return (0);
/*  if (RENTCODE(index) == RENT_RENTED ||
      RENTCODE(index) == RENT_CRYO
     )
     return (0);
  else
     return (1);
*/
}

/********** Some util functions for objects save... **********/

void Crash_restore_weight(OBJ_DATA * obj)
{
	for (; obj; obj = obj->next_content) {
		Crash_restore_weight(obj->contains);
		if (obj->in_obj)
			GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
	}
}

void Crash_extract_objs(OBJ_DATA * obj)
{
	int rnum;
	OBJ_DATA *next;
	for (; obj; obj = next) {
		next = obj->next_content;
		Crash_extract_objs(obj->contains);
		if ((rnum = real_object(GET_OBJ_VNUM(obj))) >= 0 && GET_OBJ_TIMER(obj) >= 0)
			obj_index[rnum].stored++;
		extract_obj(obj);
	}
}


int Crash_is_unrentable(OBJ_DATA * obj)
{
	if (!obj)
		return FALSE;

	if (IS_OBJ_STAT(obj, ITEM_NORENT) ||
	    GET_OBJ_RENT(obj) < 0 || GET_OBJ_RNUM(obj) <= NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY)
		return TRUE;

	return FALSE;
}


void Crash_extract_norents(OBJ_DATA * obj)
{
	OBJ_DATA *next;
	for (; obj; obj = next) {
		next = obj->next_content;
		Crash_extract_norents(obj->contains);
		if (Crash_is_unrentable(obj))
			extract_obj(obj);
	}
}

void Crash_extract_norent_eq(CHAR_DATA * ch)
{
	int j;

	for (j = 0; j < NUM_WEARS; j++) {
		if (GET_EQ(ch, j) == NULL)
			continue;

		if (Crash_is_unrentable(GET_EQ(ch, j)))
			obj_to_char(unequip_char(ch, j), ch);
		else
			Crash_extract_norents(GET_EQ(ch, j));
	}
}

/*
void Crash_extract_expensive(OBJ_DATA * obj)
{
  OBJ_DATA *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
      if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
         max = tobj;
  extract_obj(max);
}
*/

int Crash_calculate_rent(OBJ_DATA * obj)
{
	int cost = 0;
	for (; obj; obj = obj->next_content) {
		cost += Crash_calculate_rent(obj->contains);
		cost += MAX(0, GET_OBJ_RENT(obj));
	}
	return (cost);
}

int Crash_calculate_rent_eq(OBJ_DATA * obj)
{
	int cost = 0;
	for (; obj; obj = obj->next_content) {
		cost += Crash_calculate_rent(obj->contains);
		cost += MAX(0, GET_OBJ_RENTEQ(obj));
	}
	return (cost);
}

int Crash_calcitems(OBJ_DATA * obj)
{
	int i = 0;
	for (; obj; obj = obj->next_content, i++)
		i += Crash_calcitems(obj->contains);
	return (i);
}

int Crash_calc_charmee_items (CHAR_DATA *ch)
{
  CHAR_DATA * charmee = NULL;
  struct follow_type *k, *next;
  int num = 0;
  int j = 0;

  if (!ch->followers) 
    return 0;
  for (k = ch->followers; k && k->follower->master; k = next) 
  {
    next = k->next;
    charmee = ch->followers->follower;
    if (!IS_CHARMICE(charmee))
      continue;
    for (j = 0; j < NUM_WEARS; j++)
      num += Crash_calcitems(GET_EQ(charmee, j));
    num += Crash_calcitems(charmee->carrying);
  }
  return num;
}

#define CRASH_LENGTH   0x40000
#define CRASH_DEPTH    0x1000

int Crashitems;
char *Crashbufferdata = NULL;
char *Crashbufferpos;

void Crash_save(int iplayer, OBJ_DATA * obj, int location)
{
	for (; obj; obj = obj->next_content) {
		if (obj->in_obj)
			GET_OBJ_WEIGHT(obj->in_obj) -= GET_OBJ_WEIGHT(obj);
		Crash_save(iplayer, obj->contains, MIN(0, location) - 1);
		if (iplayer >= 0 &&
//           Crashitems < MAX_SAVED_ITEMS &&  /* Removed to avoid objects loss */
		    Crashbufferpos - Crashbufferdata + CRASH_DEPTH < CRASH_LENGTH) {
			write_one_object(&Crashbufferpos, obj, location);
			SAVEINFO(iplayer)->time[Crashitems].vnum = GET_OBJ_VNUM(obj);
			SAVEINFO(iplayer)->time[Crashitems].timer = GET_OBJ_TIMER(obj);
			Crashitems++;
		}
	}
}

/********************* save_char_objects ********************************/
int save_char_objects(CHAR_DATA * ch, int savetype, int rentcost)
{
	FILE *fl;
	char fname[MAX_STRING_LENGTH];
	struct save_rent_info rent;
	int j, num = 0, iplayer = -1, cost;
  CHAR_DATA * charmee = NULL;
  struct follow_type *k, *next;

	if (IS_NPC(ch))
		return FALSE;

	if ((iplayer = GET_INDEX(ch)) < 0) {
		sprintf(buf, "[SYSERR] Store file '%s' - INVALID ID %d", GET_NAME(ch), iplayer);
		mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
		return FALSE;
	}

	if (savetype != RENT_CRASH) {	/*не crash и не ld */
		Crash_extract_norent_eq(ch);
		Crash_extract_norents(ch->carrying);
	}

	/*количество предметов */
	for (j = 0; j < NUM_WEARS; j++)
		num += Crash_calcitems(GET_EQ(ch, j));
	num += Crash_calcitems(ch->carrying);

	if (savetype == RENT_CRASH)
		num += Crash_calc_charmee_items (ch);

	if (!num) {
		Crash_delete_files(iplayer);
		return FALSE;
	}

	/*цена ренты */
	cost = Crash_calculate_rent(ch->carrying);
	for (j = 0; j < NUM_WEARS; j++)
		cost += Crash_calculate_rent_eq(GET_EQ(ch, j));

	/*чаевые */
	if (min_rent_cost(ch))
		cost += MAX(0, min_rent_cost(ch));

	if (GET_LEVEL(ch) <= 15)
		cost /= 2;

	if (savetype == RENT_TIMEDOUT)
		cost *= 2;

	rent.rentcode = savetype;
	rent.net_cost_per_diem = cost;
	rent.time = time(0);
	rent.nitems = num;
	//CRYO-rent надо дорабатывать либо выкидывать нафиг
	if (savetype == RENT_CRYO) {
		rent.net_cost_per_diem = 0;
		GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);
	}
	if (savetype == RENT_RENTED)
		rent.net_cost_per_diem = rentcost;
	rent.gold = GET_GOLD(ch);
	rent.account = GET_BANK_GOLD(ch);

	Crash_create_timer(iplayer, num);
	SAVEINFO(iplayer)->rent = rent;

	if (Crashbufferdata)
		free(Crashbufferdata);	//?
	CREATE(Crashbufferdata, char, CRASH_LENGTH);
	Crashitems = 0;
	Crashbufferpos = Crashbufferdata;
	*Crashbufferpos = '\0';

  /*
This shit is saving all stuff for given ch and its iplayer.
*/

  for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j)) {
			Crash_save(iplayer, GET_EQ(ch, j), j + 1);
			Crash_restore_weight(GET_EQ(ch, j));
		}
	Crash_save(iplayer, ch->carrying, 0);
	Crash_restore_weight(ch->carrying);

	if (ch->followers && savetype == RENT_CRASH) 
		for (k = ch->followers; k && k->follower->master; k = next) {
			next = k->next;
			charmee = ch->followers->follower;
			if (!IS_CHARMICE(charmee))
				continue;
			for (j = 0; j < NUM_WEARS; j++)
				if (GET_EQ(charmee, j)) {
					Crash_save(iplayer, GET_EQ(charmee, j), 0);
					Crash_restore_weight(GET_EQ(charmee, j));
				}
			Crash_save(iplayer, charmee->carrying, 0);
			Crash_restore_weight(charmee->carrying);
	}

	if (savetype != RENT_CRASH) {
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(ch, j))
				Crash_extract_objs(GET_EQ(ch, j));
		Crash_extract_objs(ch->carrying);
	}

	if (get_filename(GET_NAME(ch), fname, TEXT_CRASH_FILE)) {
		if (!(fl = fopen(fname, "w"))) {
			sprintf(buf, "[SYSERR] Store objects file '%s'- MAY BE LOCKED.", fname);
			mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
			free(Crashbufferdata);
			Crashbufferdata = NULL;
			Crash_delete_files(iplayer);
			return FALSE;
		}
		fprintf(fl, "@ Items file\n%s\n$\n$\n", Crashbufferdata);
		fclose(fl);
	} else {
		free(Crashbufferdata);
		Crashbufferdata = NULL;
		Crash_delete_files(iplayer);
		return FALSE;
	}

	free(Crashbufferdata);
	Crashbufferdata = NULL;
	if (!Crash_write_timer(iplayer)) {
		Crash_delete_files(iplayer);
		return FALSE;
	}

	if (savetype == RENT_CRASH) {
		free(SAVEINFO(iplayer));
		SAVEINFO(iplayer) = NULL;
	}

	return TRUE;
}

/*some dummy functions*/

void Crash_crashsave(CHAR_DATA * ch)
{
	save_char_objects(ch, RENT_CRASH, 0);
}

void Crash_ldsave(CHAR_DATA * ch)
{
	save_char_objects(ch, RENT_CRASH, 0);
}

void Crash_idlesave(CHAR_DATA * ch)
{
	save_char_objects(ch, RENT_TIMEDOUT, 0);
}

int Crash_rentsave(CHAR_DATA * ch, int cost)
{
	return save_char_objects(ch, RENT_RENTED, cost);
}

int Crash_cryosave(CHAR_DATA * ch, int cost)
{
	return save_char_objects(ch, RENT_TIMEDOUT, cost);
}

/**************************************************************************
 * Routines used for the receptionist                                     *
 **************************************************************************/

void Crash_rent_deadline(CHAR_DATA * ch, CHAR_DATA * recep, long cost)
{
	long rent_deadline;

	if (!cost) {
		send_to_char("Ты сможешь жить у меня до второго пришествия.\r\n", ch);
		return;
	}

	rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
	sprintf(buf,
		"$n сказал$g Вам :\r\n"
		"\"Постой обойдется тебе в %ld %s.\"\r\n"
		"\"Твоих денег хватит на %ld %s.\"\r\n",
		cost, desc_count(cost, WHAT_MONEYu), rent_deadline, desc_count(rent_deadline, WHAT_DAY));
	act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(CHAR_DATA * ch, CHAR_DATA * recep, OBJ_DATA * obj)
{
	char buf[128];
	int has_norents = 0;

	if (obj) {
		if (Crash_is_unrentable(obj)) {
			has_norents = 1;
			sprintf(buf, "$n сказал$g Вам : \"Я не приму на постой %s.\"", OBJN(obj, ch, 3));
			act(buf, FALSE, recep, 0, ch, TO_VICT);
		}
		has_norents += Crash_report_unrentables(ch, recep, obj->contains);
		has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
	}
	return (has_norents);
}



void
Crash_report_rent(CHAR_DATA * ch, CHAR_DATA * recep,
		  OBJ_DATA * obj, int *cost, long *nitems, int display, int factor, int equip, int recursive)
{
	static char buf[256];
	char bf[80];

	if (obj) {
		if (!Crash_is_unrentable(obj)) {
			(*nitems)++;
			*cost += MAX(0, ((equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) * factor));
			if (display) {
				if (*nitems == 1) {
					if (!recursive)
						act("$n сказал$g Вам : \"Одет$W спать будешь? Хм.. Ну смотри, тогда дешевле возьму\"", FALSE, recep, 0, ch, TO_VICT);
					else
						act("$n сказал$g Вам : \"Это я в чулане запру:\"", FALSE,
						    recep, 0, ch, TO_VICT);
				}
				if (CAN_WEAR_ANY(obj)) {
					if (equip)
						sprintf(bf, " (%d если снять)", GET_OBJ_RENT(obj) * factor);
					else
						sprintf(bf, " (%d если надеть)", GET_OBJ_RENTEQ(obj) * factor);
				} else
					*bf = '\0';
				sprintf(buf, "%s - %d %s%s за %s %s",
					recursive ? "" : CCWHT(ch, C_SPR),
					(equip ? GET_OBJ_RENTEQ(obj) : GET_OBJ_RENT(obj)) *
					factor,
					desc_count((equip ? GET_OBJ_RENTEQ(obj) :
						    GET_OBJ_RENT(obj)) * factor,
						   WHAT_MONEYa), bf, OBJN(obj, ch, 3),
					recursive ? "" : CCNRM(ch, C_SPR));
				act(buf, FALSE, recep, 0, ch, TO_VICT);
			}
		}
		if (recursive) {
			Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor, FALSE, TRUE);
			Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor, FALSE, TRUE);
		}
	}
}



int Crash_offer_rent(CHAR_DATA * ch, CHAR_DATA * receptionist, int display, int factor, int *totalcost)
{
	char buf[MAX_EXTEND_LENGTH];
	int i, divide = 1;
	long numitems = 0, norent;
// added by Dikiy (Лель)
	long numitems_weared = 0;
// end by Dikiy

	*totalcost = 0;
	norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
	for (i = 0; i < NUM_WEARS; i++)
		norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

	if (norent)
		return (FALSE);

	*totalcost = min_rent_cost(ch) * factor;

	for (i = 0; i < NUM_WEARS; i++)
		Crash_report_rent(ch, receptionist, GET_EQ(ch, i), totalcost, &numitems, display, factor, TRUE, FALSE);

	numitems_weared = numitems;
	numitems = 0;

	Crash_report_rent(ch, receptionist, ch->carrying, totalcost, &numitems, display, factor, FALSE, TRUE);

	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i)) {
			Crash_report_rent(ch, receptionist, (GET_EQ(ch, i))->contains,
					  totalcost, &numitems, display, factor, FALSE, TRUE);
			//Crash_report_rent(ch, receptionist, (GET_EQ(ch, i))->next_content, totalcost, &numitems, display, factor, TRUE, TRUE);
		}

	numitems += numitems_weared;

	if (!numitems) {
		act("$n сказал$g Вам : \"Но у тебя ведь ничего нет ! Просто набери \"конец\" !\"",
		    FALSE, receptionist, 0, ch, TO_VICT);
		return (FALSE);
	}
	// Зачем оно нужно? - чтобы вещи не пропадали (или систему хранения переделать нужно)
	if (numitems > MAX_SAVED_ITEMS) {
		sprintf(buf,
			"$n сказал$g Вам : \"Извините, но я не могу хранить больше %d предметов.\"", MAX_SAVED_ITEMS);
		act(buf, FALSE, receptionist, 0, ch, TO_VICT);
		return (FALSE);
	}

	divide = 1;

	if (display) {
		if (min_rent_cost(ch)) {
			sprintf(buf,
				"$n сказал$g Вам : \"И еще %d %s мне на сбитень с медовухой :)\"",
				min_rent_cost(ch) * factor, desc_count(min_rent_cost(ch) * factor, WHAT_MONEYu));
			act(buf, FALSE, receptionist, 0, ch, TO_VICT);
		}
		sprintf(buf, "$n сказал$g Вам : \"В сумме это составит %d %s %s.\"",
			*totalcost, desc_count(*totalcost, WHAT_MONEYu), (factor == RENT_FACTOR ? "в день " : ""));
		act(buf, FALSE, receptionist, 0, ch, TO_VICT);
		if (MAX(0, *totalcost / divide) > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
			act("\"...которых у тебя отродясь не было.\"", FALSE, receptionist, 0, ch, TO_VICT);
			return (FALSE);
		};

		*totalcost = MAX(0, *totalcost / divide);
		if (divide > 1)
			act("\"Так уж и быть, я скощу тебе половину.\"", FALSE, receptionist, 0, ch, TO_VICT);
		if (factor == RENT_FACTOR)
			Crash_rent_deadline(ch, receptionist, *totalcost);
	} else
		*totalcost = MAX(0, *totalcost / divide);
	return (TRUE);
}



int gen_receptionist(CHAR_DATA * ch, CHAR_DATA * recep, int cmd, char *arg, int mode)
{
	room_rnum save_room;
	int cost, rentshow = TRUE;

	if (!ch->desc || IS_NPC(ch))
		return (FALSE);

	if (!cmd && !number(0, 5))
		return (FALSE);

	if (!CMD_IS("offer") && !CMD_IS("rent") &&
	    !CMD_IS("постой") && !CMD_IS("предложение") && !CMD_IS("конец") && !CMD_IS("quit"))
		return (FALSE);

	save_room = IN_ROOM(ch);

	if (CMD_IS("конец") || CMD_IS("quit")) {
		if (save_room != r_helled_start_room &&
		    save_room != r_named_start_room && save_room != r_unreg_start_room)
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
		return (FALSE);
	}

	if (!AWAKE(recep)) {
		sprintf(buf, "%s не в состоянии говорить с Вами...\r\n", HSSH(recep));
		send_to_char(buf, ch);
		return (TRUE);
	}
	if (!CAN_SEE(recep, ch)) {
		act("$n сказал$g : \"Не люблю говорить с теми, кого я не вижу !\"", FALSE, recep, 0, 0, TO_ROOM);
		return (TRUE);
	}
	if (Clan::InEnemyZone(ch)) {
		act("$n сказал$g : \"Чужакам здесь не место !\"", FALSE, recep, 0, 0, TO_ROOM);
		return (TRUE);
	}
	if (RENTABLE(ch)) {
		send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
		return (TRUE);
	}
	if (FIGHTING(ch)) {
		return (FALSE);
	}
	if (free_rent) {
		act("$n сказал$g Вам : \"Сегодня спим нахаляву.  Наберите просто \"конец\".\"",
		    FALSE, recep, 0, ch, TO_VICT);
		return (1);
	}
	if (CMD_IS("rent") || CMD_IS("постой")) {
		if (!Crash_offer_rent(ch, recep, rentshow, mode, &cost))
			return (TRUE);

		if (!rentshow) {
			if (mode == RENT_FACTOR)
				sprintf(buf,
					"$n сказал$g Вам : \"Дневной постой обойдется тебе в %d %s.\"",
					cost, desc_count(cost, WHAT_MONEYu));
			else if (mode == CRYO_FACTOR)
				sprintf(buf,
					"$n сказал$g Вам : \"Дневной постой обойдется тебе в %d %s (за пользование холодильником :)\"",
					cost, desc_count(cost, WHAT_MONEYu));
			act(buf, FALSE, recep, 0, ch, TO_VICT);

			if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
				act("$n сказал$g Вам : '..но такой голытьбе, как ты, это не по карману.'",
				    FALSE, recep, 0, ch, TO_VICT);
				return (TRUE);
			}
			if (cost && (mode == RENT_FACTOR))
				Crash_rent_deadline(ch, recep, cost);
		}

		if (mode == RENT_FACTOR) {
			act("$n запер$q Ваши вещи в сундук и повел$g в тесную каморку.", FALSE, recep, 0, ch, TO_VICT);
			Crash_rentsave(ch, cost);
			sprintf(buf, "%s has rented (%d/day, %ld tot.)",
				GET_NAME(ch), cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
		} else {	/* cryo */
			act("$n запер$q Ваши вещи в сундук и повел$g в тесную каморку.\r\n"
			    "Белый призрак появился в комнате, обдав Вас холодом...\r\n"
			    "Вы потеряли связь с окружающими Вас...", FALSE, recep, 0, ch, TO_VICT);
			Crash_cryosave(ch, cost);
			sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
			SET_BIT(PLR_FLAGS(ch, PLR_CRYO), PLR_CRYO);
		}

		mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);

		if (save_room == r_named_start_room)
			act("$n проводил$g $N3 мощным пинком на свободную лавку.", FALSE, recep, 0, ch, TO_ROOM);
		else if (save_room == r_helled_start_room || save_room == r_unreg_start_room)
			act("$n проводил$g $N3 мощным пинком на свободные нары.", FALSE, recep, 0, ch, TO_ROOM);
		else
			act("$n помог$q $N2 отойти ко сну.", FALSE, recep, 0, ch, TO_NOTVICT);
		if (save_room != r_helled_start_room &&
		    save_room != r_named_start_room && save_room != r_unreg_start_room)
			GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
		extract_char(ch, FALSE);
//		save_char(ch, save_room);
	} else {
		Crash_offer_rent(ch, recep, TRUE, mode, &cost);
		act("$N предложил$G $n2 остановиться у н$S.", FALSE, ch, 0, recep, TO_ROOM);
	}
	return (TRUE);
}


SPECIAL(receptionist)
{
	return (gen_receptionist(ch, (CHAR_DATA *) me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
	return (gen_receptionist(ch, (CHAR_DATA *) me, cmd, argument, CRYO_FACTOR));
}


void Crash_frac_save_all(int frac_part)
{
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character) && GET_ACTIVITY(d->character) == frac_part) {
			Crash_crashsave(d->character);
			save_char(d->character, NOWHERE);
			REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
		}
	}
}

void Crash_save_all(void)
{
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
			if (PLR_FLAGGED(d->character, PLR_CRASH)) {
				Crash_crashsave(d->character);
				save_char(d->character, NOWHERE);
				REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
			}
		}
	}
}

void Crash_save_all_rent(void)
{
	int cost;

// shapirus: проходим не по списку дескрипторов,
// а по списку чаров, чтобы сохранить заодно и тех,
// кто перед ребутом ушел в ЛД с целью сохранить
// свои грязные денежки.
	CHAR_DATA *ch;
	for (ch = character_list; ch; ch = ch->next)
		if (!IS_NPC(ch)) {
			Crash_offer_rent(ch, ch, FALSE, RENT_FACTOR, &cost);
			Crash_rentsave(ch, cost);
			log("Saving char: %s with rent %i \n", GET_NAME(ch), cost);
			REMOVE_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
			REMOVE_BIT(PLR_FLAGS(ch, AFF_GROUP), AFF_GROUP);
			REMOVE_BIT(PLR_FLAGS(ch, AFF_HORSE), AFF_HORSE);
			extract_char(ch, FALSE);
		}
}


void Crash_frac_rent_time(int frac_part)
{
	int c;
	for (c = 0; c <= top_of_p_table; c++)
		if (player_table[c].activity == frac_part && player_table[c].unique != -1 && SAVEINFO(c))
			Crash_timer_obj(c, time(0));
}

void Crash_rent_time(int dectime)
{
	int c;
	for (c = 0; c <= top_of_p_table; c++)
		if (player_table[c].unique != -1)
			Crash_timer_obj(c, time(0));
}
