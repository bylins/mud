/* ************************************************************************
*   File: house.cpp                                     Part of Bylins    *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "house.h"
#include "constants.h"
#include "screen.h"
#include "pk.h"


extern INDEX_DATA *obj_index;
extern INDEX_DATA *mob_index;
extern CHAR_DATA *mob_proto;
extern mob_rnum top_of_mobt;
extern DESCRIPTOR_DATA *descriptor_list;
extern const char *Dirs[];

extern struct player_index_element *player_table;
extern int top_of_p_table;

OBJ_DATA *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(OBJ_DATA * obj, FILE * fl, int location);

struct house_control_rec house_control[MAX_HOUSES];
int num_of_houses = 0;

/* local functions */
long house_zone(room_rnum rnum);
int House_get_filename(int vnum, char *filename);
int House_load(room_vnum vnum);
int House_save(OBJ_DATA * obj, FILE * fp);
void House_restore_weight(OBJ_DATA * obj);
void House_delete_file(int house_num);
long House_create_unique(void);
void House_save_control(void);
void hcontrol_list_houses(CHAR_DATA * ch);
void hcontrol_build_house(CHAR_DATA * ch, char *arg);
void hcontrol_destroy_house(CHAR_DATA * ch, char *arg);
void hcontrol_pay_house(CHAR_DATA * ch, char *arg);
void hcontrol_set_guard(CHAR_DATA * ch, char *arg);
void hcontrol_set_prison(CHAR_DATA * ch, char *arg);
void hcontrol_change_leader(CHAR_DATA * ch, char *arg);
int find_politics_state(struct house_control_rec *house, room_vnum victim_vnum);
void set_politics(struct house_control_rec *house, room_vnum victim_vnum, int state);
ACMD(do_hcontrol);
ACMD(do_house);
ACMD(do_hchannel);
ACMD(do_whohouse);
ACMD(do_whoclan);
ACMD(do_newsclan);
ACMD(do_clanlist);
ACMD(do_show_politics);
ACMD(do_set_politics);
ACMD(do_prison);

void House_channel(CHAR_DATA * ch, char *msg);
void House_id_channel(CHAR_DATA * ch, int huid, char *msg);
void AHouse_channel(CHAR_DATA * ch, char *msg);
void AHouse_id_channel(CHAR_DATA * ch, int huid, char *msg);

/* extern */
extern int file_to_string(const char *name, char *buf);

#define GARBAGE_SIZE	128

// Структура работы с clanlist
struct clanlistitem {
	struct clanlistitem *next;	// следующий в списке
	long size;		// размер структуры
	unsigned long msg_id;	// идентификатор сообщения
	time_t msg_time;	// время создания записи
	long author_id;		// идентификатор автора
	long victim_id;		// идентификатор жертвы
	char msg[1];		// само сообщение
};

const char *house_rank[] = { "пришелец",
	"отрок",
	"вой",
	"муж",
	"гридень",
	"кметь",
	"храбр",
	"десятник",
	"боярин",
	"воевода",
	"изгнать",
	"\n"
};

const char *HCONTROL_FORMAT =
    "Формат: hcontrol build[возвести] <house vnum> <closest rent vnum> <exit direction> <player name> <shortname> [name]\r\n"
    "        hcontrol rooms[залы] <house vnum> <room vnum> ...\r\n"
    "        hcontrol destroy[разрушить] <house vnum>\r\n"
    "        hcontrol pay[налог] <house vnum>\r\n"
    "        hcontrol show[показать]\r\n"
    "        hcontrol guard[охранник] <house vnum> <mob vnum>\r\n"
    "        hcontrol leader[лидер] <новый лидер> <старый лидер>\r\n"
    "        hcontrol prison[темница] <house vnum> <prison vnum|0>\r\n";


/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
int House_get_filename(int vnum, char *filename)
{
	if (vnum == NOWHERE)
		return (0);

	sprintf(filename, LIB_HOUSE "%d.house", vnum);
	return (1);
}


/* Load all objects for a house */
int House_load(room_vnum vnum)
{
	FILE *fl;
	char fname[MAX_STRING_LENGTH];
	struct obj_file_elem object;
	room_rnum rnum;
	int i;

	return (0);
	if ((rnum = real_room(vnum)) == NOWHERE)
		return (0);
	if (!House_get_filename(vnum, fname))
		return (0);
	if (!(fl = fopen(fname, "r+b")))	/* no file found */
		return (0);
	while (!feof(fl)) {
		fread(&object, sizeof(struct obj_file_elem), 1, fl);
		if (ferror(fl)) {
			perror("SYSERR: Reading house file in House_load");
			fclose(fl);
			return (0);
		}
		if (!feof(fl))
			obj_to_room(Obj_from_store(object, &i), rnum);
	}

	fclose(fl);

	return (1);
}


/* Save all objects for a house (recursive; initial call must be followed
   by a call to House_restore_weight)  Assumes file is open already. */
int House_save(OBJ_DATA * obj, FILE * fp)
{
	OBJ_DATA *tmp;
	int result;

	if (obj) {
		House_save(obj->contains, fp);
		House_save(obj->next_content, fp);
		result = Obj_to_store(obj, fp, 0);
		if (!result)
			return (0);

		for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
			GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
	}
	return (1);
}


/* restore weight of containers after House_save has changed them for saving */
void House_restore_weight(OBJ_DATA * obj)
{
	if (obj) {
		House_restore_weight(obj->contains);
		House_restore_weight(obj->next_content);
		if (obj->in_obj)
			GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
	}
}


/* Save all objects in a house */
void House_crashsave(room_vnum vnum)
{
	int rnum;
	char buf[MAX_STRING_LENGTH];
	FILE *fp;

	if ((rnum = real_room(vnum)) == NOWHERE)
		return;
	if (!House_get_filename(vnum, buf))
		return;
	if (!(fp = fopen(buf, "wb"))) {
		perror("SYSERR: Error saving house file");
		return;
	}
	if (!House_save(world[rnum]->contents, fp)) {
		fclose(fp);
		return;
	}
	fclose(fp);
	House_restore_weight(world[rnum]->contents);
	REMOVE_BIT(ROOM_FLAGS(rnum, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
}


/* Delete a house save file */
void House_delete_file(int vnum)
{
	char fname[MAX_INPUT_LENGTH];
	FILE *fl;

	if (!House_get_filename(vnum, fname))
		return;
	if (!(fl = fopen(fname, "rb"))) {
		if (errno != ENOENT)
			log("SYSERR: Error deleting house file #%d. (1): %s", vnum, strerror(errno));
		return;
	}
	fclose(fl);
	if (remove(fname) < 0)
		log("SYSERR: Error deleting house file #%d. (2): %s", vnum, strerror(errno));
}


/* List all objects in a house file */
void House_listrent(CHAR_DATA * ch, room_vnum vnum)
{
	FILE *fl;
	char fname[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct obj_file_elem object;
	OBJ_DATA *obj;
	int i;

	if (!House_get_filename(vnum, fname))
		return;
	if (!(fl = fopen(fname, "rb"))) {
		sprintf(buf, "No objects on file for house #%d.\r\n", vnum);
		send_to_char(buf, ch);
		return;
	}
	*buf = '\0';
	while (!feof(fl)) {
		fread(&object, sizeof(struct obj_file_elem), 1, fl);
		if (ferror(fl)) {
			fclose(fl);
			return;
		}
		if (!feof(fl) && (obj = Obj_from_store(object, &i)) != NULL) {
			sprintf(buf + strlen(buf), " [%5d] (%5dau) %s\r\n",
				GET_OBJ_VNUM(obj), GET_OBJ_RENT(obj), obj->short_description);
			extract_obj(obj);
		}
	}

	send_to_char(buf, ch);
	fclose(fl);
}


int House_closestrent(int to_room)
{
	int j;
	for (j = 0; j < num_of_houses; ++j) {
		if (get_name_by_id(house_control[j].owner) == NULL)
			continue;
		if (world[to_room]->zone == world[real_room(house_control[j].vnum)]->zone) {
			to_room = real_room(house_control[j].closest_rent);
			break;
		}
	}
	return to_room;
}

// Выясняет принадлежит ли комната to_room какому-то замку
// Просто сравнивает номер зоны
// rnum центра замка или NOWHERE
int House_vnum(int to_room)
{
	int j, r;
	for (j = 0; j < num_of_houses; ++j) {
		if (get_name_by_id(house_control[j].owner) == NULL)
			continue;
		r = real_room(house_control[j].vnum);
		if (world[to_room]->zone != world[r]->zone)
			continue;
		return r;
	}
	return NOWHERE;
}

int House_atrium(int to_room)
{
	int j;
	for (j = 0; j < num_of_houses; ++j) {
		if (get_name_by_id(house_control[j].owner) == NULL)
			continue;
		if (world[to_room]->zone == world[real_room(house_control[j].vnum)]->zone) {
			to_room = real_room(house_control[j].atrium);
			break;
		}
	}
	return to_room;
}


/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/
long house_zone(room_rnum rnum)
{
	int i, zn;

	zn = world[rnum]->zone;

	for (i = 0; i < num_of_houses; i++) {
		if (zn == world[real_room(house_control[i].vnum)]->zone) {
			return HOUSE_UNIQUE(i);
		}
	}
	return (0);
}

int find_house(room_vnum vnum)
{
	int i;

	for (i = 0; i < num_of_houses; i++)
		if (house_control[i].vnum == vnum)
			return (i);

	return (NOHOUSE);
}


long House_create_unique(void)
{
	int i;
	long j;

	for (; TRUE;) {
		j = (number(1, 255) << 24) + (number(1, 255) << 16) + (number(1, 255) << 8) + number(1, 255);
		for (i = 0; i < num_of_houses; i++)
			if (HOUSE_UNIQUE(i) == j)
				break;
		if (i >= num_of_houses)
			return (j);
	}
	return (0);
}

/* Save the house control information */
void House_save_control(void)
{
	FILE *fl;
	int i, j, k;
	char *nme = NULL;
	long int li;

	if (!(fl = fopen(HCONTROL_FILE, "w"))) {
		perror("SYSERR: Unable to open house control file.");
		return;
	}
	for (i = 0; i < num_of_houses; i++) {
		fprintf(fl, "Name: %s %s\n", house_control[i].sname, house_control[i].name);
		fprintf(fl, "Room: %d\n", house_control[i].vnum);
		fprintf(fl, "Atrm: %d\n", house_control[i].atrium);
		fprintf(fl, "Exit: %d\n", house_control[i].exit_num);
		li = house_control[i].built_on;
		fprintf(fl, "BlTm: %ld\n", li);
		fprintf(fl, "Mode: %d\n", house_control[i].mode);
		fprintf(fl, "MobK: %ld\n", house_control[i].keeper);
		fprintf(fl, "Uniq: %ld\n", house_control[i].unique);
		li = house_control[i].last_payment;
		fprintf(fl, "Paym: %ld\n", li);
		fprintf(fl, "CRnt: %d\n", house_control[i].closest_rent);
		fprintf(fl, "Pris: %d\n", house_control[i].prison);
		fprintf(fl, "Ownr: %ld (%s)\n", house_control[i].owner, get_name_by_id(house_control[i].owner));
		for (k = 0; k < MAX_HOUSES; k++) {
			if (!house_control[i].politics_state[k] ||
			    !house_control[i].politics_vnum[k] ||
			    house_control[i].politics_vnum[k] == house_control[i].vnum ||
			    find_house(house_control[i].vnum) == NOHOUSE)
				continue;
			fprintf(fl, "CPlt: %d:%d\n", house_control[i].politics_vnum[k],
				house_control[i].politics_state[k]);
		}
		for (j = 0; j < house_control[i].num_of_guests; j++) {
			nme = get_name_by_id(house_control[i].guests[j]);
			if (nme == NULL)
				continue;
			fprintf(fl, "Gues: %ld (%s)\n", house_control[i].guests[j], nme);
		}
		fprintf(fl, "~\n");
	}
	fclose(fl);
}

/* Загрузка данных house из файла */
int House_load_control(struct house_control_rec *house, FILE * fl)
{
	int i, j = 0;
	char line[MAX_INPUT_LENGTH + 1], tag[6];
	room_rnum real_rent;

	/* Установки по умолчанию */
	house->atrium = 0;
	house->prison = 0;
	house->exit_num = 0;
	house->built_on = 0;
	house->mode = 0;
	house->owner = 0;
	house->num_of_guests = 0;
	house->last_payment = 0;
	house->unique = 0;
	house->keeper = 0;
	*house->name = '\0';
	*house->sname = '\0';
	house->vnum = 0;
	if ((real_rent = real_room(49987)) != NOWHERE)
		house->closest_rent = 49987;
	else
		house->closest_rent = 4056;
	for (i = 0; i < MAX_HOUSES; i++) {
		house->politics_vnum[i] = 0;
		house->politics_state[i] = 0;
	}
	for (i = 0; i < MAX_GUESTS; i++)
		house->guests[i] = 0;
	/* Чтение файла */
	while (get_line(fl, line)) {
		if (line[0] == '~')
			return (1);
		tag_argument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++);
		line[i] = '\0';
		i++;

		if (!strcmp(tag, "Name")) {
			strcpy(house->name, line + i);
			strcpy(house->sname, line);
		} else if (!strcmp(tag, "Room"))
			house->vnum = atoi(line);
		else if (!strcmp(tag, "Atrm"))
			house->atrium = atoi(line);
		else if (!strcmp(tag, "Exit"))
			house->exit_num = atoi(line);
		else if (!strcmp(tag, "BlTm"))
			house->built_on = atol(line);
		else if (!strcmp(tag, "Mode"))
			house->mode = atoi(line);
		else if (!strcmp(tag, "MobK"))
			house->keeper = atol(line);
		else if (!strcmp(tag, "Uniq"))
			house->unique = atol(line);
		else if (!strcmp(tag, "Paym"))
			house->last_payment = atol(line);
		else if (!strcmp(tag, "CRnt"))
			house->closest_rent = atol(line);
		else if (!strcmp(tag, "Pris"))
			house->prison = atol(line);
		else if (!strcmp(tag, "Ownr"))
			house->owner = atol(line);
		else if (!strcmp(tag, "CPlt")) {
			sscanf(line, "%d:%d", &(house->politics_vnum[j]), &(house->politics_state[j]));
			j++;
		} else if (!strcmp(tag, "Gues")) {
			if (house->num_of_guests < MAX_GUESTS) {
				house->guests[house->num_of_guests] = atol(line);
				house->num_of_guests++;
			}
		}
	}
	return (0);
}

/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void)
{
	struct house_control_rec temp_house;
	room_rnum real_house, real_atrium;
	FILE *fl;
	int i;

	memset((char *) house_control, 0, sizeof(struct house_control_rec) * MAX_HOUSES);

	if (!(fl = fopen(HCONTROL_FILE, "r"))) {
		if (errno == ENOENT)
			log("   House control file '%s' does not exist.", HCONTROL_FILE);
		else
			perror("SYSERR: " HCONTROL_FILE);
		return;
	}
	while (House_load_control(&temp_house, fl) && num_of_houses < MAX_HOUSES) {

		if (get_name_by_id(temp_house.owner) == NULL)
			continue;	/* owner no longer exists -- skip */

		if ((real_house = real_room(temp_house.vnum)) == NOWHERE)
			continue;	/* this vnum doesn't exist -- skip */

		if (find_house(temp_house.vnum) != NOHOUSE)
			continue;	/* this vnum is already a house -- skip */

		if ((real_atrium = real_room(temp_house.atrium)) == NOWHERE)
			continue;	/* house doesn't have an atrium -- skip */

		if (temp_house.exit_num < 0 || temp_house.exit_num >= NUM_OF_DIRS)
			continue;	/* invalid exit num -- skip */

		if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
			continue;	/* exit num mismatch -- skip */

		house_control[num_of_houses] = temp_house;

//        SET_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE),   ROOM_HOUSE);
//        SET_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);

//        SET_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
		House_load(temp_house.vnum);

		for (i = 0; i <= top_of_mobt; ++i) {
			if (mob_index[i].zone != world[real_house]->zone)
				continue;
			SET_BIT(MOB_FLAGS((mob_proto + i), MOB_NOTRAIN), MOB_NOTRAIN);
		}

		num_of_houses++;
	}

	fclose(fl);
	House_save_control();
}

int House_for_uid(long uid)
{
	int i;
	for (i = 0; i < num_of_houses; i++)
		if (HOUSE_UNIQUE(i) == uid)
			return (i);
	return (NOHOUSE);
}

int House_check_exist(long uid)
{
	int j;
	for (j = 0; j < num_of_houses; j++)
		if (HOUSE_UNIQUE(j) == uid)
			return (TRUE);
	return (FALSE);
}

/*
int House_check_keeper(long uid)
{room_rnum i,j;
 CHAR_DATA *ch;

 if ((j=House_for_uid(uid)) == NOWHERE)
    return (FALSE);
 if (!house_control[j].keeper)
    return (FALSE);
 if ((i=real_room(house_control[j].atrium)) == NOWHERE)
    return (FALSE);
 for (ch = world[i]->people; ch; ch = ch->next_in_room)
     if (GET_MOB_VNUM(ch) == house_control[j].keeper)
        return (TRUE);
 return (FALSE);
}
*/

/* "House Control" functions */

DESCRIPTOR_DATA *House_find_desc(long id)
{
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && GET_ID(d->character) == id)
			break;
	return (d);
}


void hcontrol_destroy_house(CHAR_DATA * ch, char *arg)
{
	int i, j;
//  room_rnum real_atrium, real_house;
	room_rnum real_house;

	if (!*arg) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if ((i = find_house(atoi(arg))) == NOHOUSE) {
		send_to_char("Нет такого замка.\r\n", ch);
		return;
	}
/*
  if ((real_atrium = real_room(house_control[i].atrium)) == NOWHERE)
     log("SYSERR: House %d had invalid atrium %d!", atoi(arg),
	  house_control[i].atrium);
  else
     REMOVE_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
*/

	if ((real_house = real_room(house_control[i].vnum)) == NOWHERE)
		log("SYSERR: House %d had invalid vnum %d!", atoi(arg), house_control[i].vnum);
	else {
//            REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE), ROOM_HOUSE);
		REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE_CRASH), ROOM_HOUSE_CRASH);
//            REMOVE_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
	}

	House_delete_file(house_control[i].vnum);

	for (j = i; j < num_of_houses - 1; j++)
		house_control[j] = house_control[j + 1];

	num_of_houses--;

	send_to_char("Замок разрушен.\r\n", ch);
	House_save_control();

	/*
	 * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
	 * just in case the house we just deleted shared an atrium with another
	 * house.  --JE 9/19/94
	 */
/*
  for (i = 0; i < num_of_houses; i++)
      if ((real_atrium = real_room(house_control[i].atrium)) != NOWHERE)
         SET_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
*/
}


void hcontrol_pay_house(CHAR_DATA * ch, char *arg)
{
	int i;

	if (!*arg)
		send_to_char(HCONTROL_FORMAT, ch);
	else if ((i = find_house(atoi(arg))) == NOHOUSE)
		send_to_char("Нет такого замка.\r\n", ch);
	else {
		sprintf(buf, "Payment for house %s collected by %s.", arg, GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);

		house_control[i].last_payment = time(0);
		House_save_control();
		send_to_char("Вы внесли великокняжеский налог за свой замок.\r\n", ch);
	}
}


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	half_chop(argument, arg1, arg2);

	if (is_abbrev(arg1, "build") || is_abbrev(arg1, "возвести"))
		hcontrol_build_house(ch, arg2);
	else if (is_abbrev(arg1, "destroy") || is_abbrev(arg1, "разрушить"))
		hcontrol_destroy_house(ch, arg2);
	else if (is_abbrev(arg1, "pay") || is_abbrev(arg1, "налог"))
		hcontrol_pay_house(ch, arg2);
	else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "показать"))
		hcontrol_list_houses(ch);
	else if (is_abbrev(arg1, "guard") || is_abbrev(arg1, "охранник"))
		hcontrol_set_guard(ch, arg2);
	else if (is_abbrev(arg1, "leader") || is_abbrev(arg1, "лидер"))
		hcontrol_change_leader(ch, arg2);
	else if (is_abbrev(arg1, "prison") || is_abbrev(arg1, "темница"))
		hcontrol_set_prison(ch, arg2);
	else
		send_to_char(HCONTROL_FORMAT, ch);
}


/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
	DESCRIPTOR_DATA *d;
	int i, j, id = -1, rank = RANK_GUEST;

	argument = one_argument(argument, arg);

	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
		send_to_char("Вам стоит войти в свой замок.\r\n", ch);
	else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOHOUSE)
		send_to_char("Хмм.. Что-то не очень похоже на замок.\r\n", ch);
	else if (GET_IDNUM(ch) != house_control[i].owner)
		send_to_char("Вы же не хозяин этого замка !\r\n", ch);
	else if (!*arg)
		House_list_guests(ch, i, FALSE);
	else if ((id = get_id_by_name(arg)) < 0)
		send_to_char("Неизвестный игрок.\r\n", ch);
	else if (id == GET_IDNUM(ch))
		send_to_char("Это же ВАШ замок!\r\n", ch);
	else {
		if (!(d = House_find_desc(id))) {
			send_to_char("Этого игрока нет в игре !\r\n", ch);
			/* Если чара нет в игре разрешается только выгнать его */
			one_argument(argument, arg);
			if (is_abbrev(arg, "изгнать")) {
				for (j = 0; j < house_control[i].num_of_guests; j++)
					if (house_control[i].guests[j] == id) {
						for (; j < house_control[i].num_of_guests; j++)
							house_control[i].guests[j] = house_control[i].guests[j + 1];
						house_control[i].num_of_guests--;
						House_save_control();
						send_to_char("но больше он не Ваш соратник.\r\n", ch);
						return;
					}
				send_to_char("...и он к тому же не Ваш соратник.\r\n", ch);
			}
			return;
		}

		one_argument(argument, arg);
		if (!*arg)
			rank = RANK_GUEST;
		else if ((rank = search_block(arg, house_rank, FALSE)) < 0) {
			send_to_char("Укажите положение этого игрока или ИЗГНАТЬ.\r\n", ch);
			send_to_char(" Возможные положения:\r\n", ch);
			for (j = RANK_GUEST; j < RANK_KNIEZE; j++) {
				send_to_char(house_rank[j], ch);
				send_to_char("\r\n", ch);
			}
			return;
		}

		if (GET_HOUSE_UID(d->character)
		    && GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(i)) {
			send_to_char("Этот игрок не является Вашим соратником.\r\n", ch);
			return;
		}

		if (!GET_HOUSE_UID(d->character) && rank > RANK_KNIEZE) {
			send_to_char("Этот игрок не является Вашим соратником.\r\n", ch);
			return;
		}

		if (RENTABLE(d->character)) {
			send_to_char("Статус игрока нельзя изменять во время боевых действий.\r\n", ch);
			return;
		}

		if (rank > RANK_KNIEZE) {
			for (j = 0; j < house_control[i].num_of_guests; j++)
				if (house_control[i].guests[j] == id) {
					for (; j < house_control[i].num_of_guests; j++)
						house_control[i].guests[j] = house_control[i].guests[j + 1];
					house_control[i].num_of_guests--;
					House_save_control();
					act("Игрок $N изгнан$G из числа Ваших соратников.", FALSE,
					    ch, 0, d->character, TO_CHAR);

					GET_HOUSE_UID(d->character) = GET_HOUSE_RANK(d->character) = 0;
					save_char(d->character, NOWHERE);
					sprintf(buf, "Вас исключили из дружины '%s'.", House_name(ch));
					act(buf, FALSE, d->character, 0, ch, TO_CHAR);
					return;
				}
			return;
		}

		if (rank == RANK_KNIEZE) {
			send_to_char("В замке может быть лишь один КНЯЗЬ !\r\n", ch);
			return;
		}

		if (!GET_HOUSE_UID(d->character)) {
			if (house_control[i].num_of_guests == MAX_GUESTS) {
				send_to_char("Слишком много соратников в замке.\r\n", ch);
				return;
			}

			j = house_control[i].num_of_guests++;
			house_control[i].guests[j] = id;
			House_save_control();
		}
		GET_HOUSE_UID(d->character) = HOUSE_UNIQUE(i);
		GET_HOUSE_RANK(d->character) = rank;
		save_char(d->character, NOWHERE);
		sprintf(buf, "$N приписан$G к Вашей дружине, статус - %s.", house_rank[rank]);
		act(buf, FALSE, ch, 0, d->character, TO_CHAR);
/*      sprintf(buf,"Вас приписали к дружине $N1, статус - %s.",house_rank[rank]);*/
		sprintf(buf, "Вас приписали к дружине '%s', статус - %s.", House_name(ch), house_rank[rank]);
		act(buf, FALSE, d->character, 0, ch, TO_CHAR);
	}
}

ACMD(do_hchannel)
{
	int i;
	char arg2[MAX_INPUT_LENGTH];


	skip_spaces(&argument);

	if (!IS_GRGOD(ch) && !GET_HOUSE_UID(ch)) {
		send_to_char("Вы не принадлежите ни к одной дружине.\r\n", ch);
		return;
	}
	if ((GET_HOUSE_RANK(ch) == RANK_GUEST) && (!IS_IMMORTAL(ch))) {
		send_to_char("Вы гость дружины, и не можете пользоваться ее привилегиями.\r\n", ch);
		return;
	}

/* if (!IS_GOD(ch) && !GET_HOUSE_RANK(ch))
    {send_to_char("Вы не в состоянии ничего сюда сообщить.\r\n",ch);
     return;
    } */
	if (!*argument) {
		send_to_char("Что Вы хотите сообщить ?\r\n", ch);
		return;
	}
	if (IS_GRGOD(ch)) {
//     sprintf(arg2,argument);
		strcpy(arg2, argument);
		argument = one_argument(argument, arg);
		for (i = 0; i < num_of_houses; i++)
			if (!str_cmp(house_control[i].sname, arg)) {
				skip_spaces(&argument);
				switch (subcmd) {
				case SCMD_CHANNEL:
					House_id_channel(ch, house_control[i].unique, argument);
					break;
				case SCMD_ACHANNEL:
					AHouse_id_channel(ch, house_control[i].unique, argument);
					break;
				}
				return;
			}
		if (GET_HOUSE_UID(ch)) {
			switch (subcmd) {
			case SCMD_CHANNEL:
				House_channel(ch, arg2);
				break;
			case SCMD_ACHANNEL:
				AHouse_channel(ch, arg2);
				break;
			}
		} else
			send_to_char("Такой дружины нет.", ch);
	} else
		switch (subcmd) {
		case SCMD_CHANNEL:
			House_channel(ch, argument);
			break;
		case SCMD_ACHANNEL:
			AHouse_channel(ch, argument);
			break;
		}
}


/* Misc. administrative functions */


/* crash-save all the houses */
void House_save_all(void)
{
	int i;
	room_rnum real_house;

	for (i = 0; i < num_of_houses; i++)
		if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
			if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
				House_crashsave(house_control[i].vnum);
}


void House_list_guests(CHAR_DATA * ch, int i, int quiet)
{
	int j;
	char *temp;
	char buf[MAX_STRING_LENGTH], buf2[MAX_NAME_LENGTH + 2];

	if (house_control[i].num_of_guests == 0) {
		if (!quiet)
			send_to_char("К замку никто не приписан.\r\n", ch);
		return;
	}

	strcpy(buf, "К замку приписаны : ");

	/* Avoid <UNDEF>. -gg 6/21/98 */
	for (j = 0; j < house_control[i].num_of_guests; j++) {
		if ((temp = get_name_by_id(house_control[i].guests[j])) == NULL)
			continue;
		sprintf(buf2, "%s ", temp);
		strcat(buf, CAP(buf2));
	}

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}

int House_major(CHAR_DATA * ch)
{
	int i;
	DESCRIPTOR_DATA *d;

	if (!GET_HOUSE_UID(ch))
		return (FALSE);
	if (GET_HOUSE_RANK(ch) == RANK_KNIEZE)
		return (TRUE);
	if (GET_HOUSE_RANK(ch) < RANK_CENTURION)
		return (FALSE);
	if ((i = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE)
		return (FALSE);
	for (d = descriptor_list; d; d = d->next) {
		if (!d->character ||
		    GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(i) ||
		    STATE(d) != CON_PLAYING || GET_HOUSE_RANK(d->character) <= GET_HOUSE_RANK(ch))
			continue;
		break;
	}
	return (d == NULL);
}

/*
void House_set_keeper(CHAR_DATA *ch)
{int    house, guard_room;
 CHAR_DATA* guard;

 if (!GET_HOUSE_UID(ch))
    {send_to_char("Вы не приписаны ни к одному замку !\r\n",ch);
     return;
    }

 if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOWHERE)
    {send_to_char("Ваш замок, похоже, разрушен.\r\n",ch);
     GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
     return;
    }

 if (!house_control[house].keeper)
    {send_to_char("Охрана Вашего замка не предусмотрена.\r\n",ch);
     return;
    };

 if (!House_major(ch))
    {send_to_char("Вы не самый старший соратник в игре !\r\n",ch);
     return;
    }
 if (House_check_keeper(GET_HOUSE_UID(ch)))
    {send_to_char("Охрана уже выставлена !\r\n",ch);
     return;
    }
 house = House_for_uid(GET_HOUSE_UID(ch));
 if ((guard_room = real_room(house_control[house].atrium)) == NOWHERE)
    {send_to_char("Нет комнаты для охраны.\r\n",ch);
     return;
    }
 if (!(guard = read_mobile(house_control[house].keeper, VIRTUAL)))
    {send_to_char("Нет прототипа охранника.\r\n",ch);
     return;
    }
 char_to_room(guard, guard_room);
 act("Вы установили $N3 на стражу замка.",FALSE,ch,0,guard,TO_CHAR);
}
*/

char *House_rank(CHAR_DATA * ch)
{
	int house = 0;
	if (!GET_HOUSE_UID(ch))
		return (NULL);

	if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE) {
		GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
		return (NULL);
	}
	return ((char *) house_rank[GET_HOUSE_RANK(ch)]);
}

char *House_name(CHAR_DATA * ch)
{
	int house = 0;
	if (!GET_HOUSE_UID(ch))
		return (NULL);

	if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE) {
		GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
		return (NULL);
	}
	return (house_control[house].name);
}


char *House_sname(CHAR_DATA * ch)
{
	int house = 0;
	if (!GET_HOUSE_UID(ch))
		return (NULL);

	if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE) {
		GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
		return (NULL);
	}
	return (house_control[house].sname);
}


void House_channel(CHAR_DATA * ch, char *msg)
{
	int house;
	char message[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;

	if (!GET_HOUSE_UID(ch))
		return;

	/* added by Pereplut */
	if (PLR_FLAGGED(ch, PLR_DUMB)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE) {
		GET_HOUSE_UID(ch) = 0;
		return;
	}
	for (d = descriptor_list; d; d = d->next) {
		if (!d->character || d->character == ch ||
		    STATE(d) != CON_PLAYING ||
		    AFF_FLAGGED(d->character, AFF_DEAFNESS) ||
		    GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(house) || ignores(d->character, ch, IGNORE_CLAN))
			continue;
		sprintf(message, "%s дружине: %s'%s'.%s\r\n",
			GET_NAME(ch), CCIRED(d->character, C_NRM), msg, CCNRM(d->character, C_NRM));
		SEND_TO_Q(message, d);
	}

	sprintf(message, "Вы дружине: %s'%s'.%s\r\n", CCIRED(ch, C_NRM), msg, CCNRM(ch, C_NRM));
	send_to_char(message, ch);

}

void AHouse_channel(CHAR_DATA * ch, char *msg)
{
	int house, i;
	char message[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;

	if (!GET_HOUSE_UID(ch))
		return;

	/* added by Pereplut */
	if (PLR_FLAGGED(ch, PLR_DUMB)) {
		send_to_char("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE) {
		GET_HOUSE_UID(ch) = 0;
		return;
	}
	for (d = descriptor_list; d; d = d->next) {
		if (!d->character || d->character == ch ||
		    STATE(d) != CON_PLAYING || AFF_FLAGGED(d->character, AFF_DEAFNESS))
			continue;
		if ((i = House_for_uid(GET_HOUSE_UID(d->character))) == NOHOUSE)
			continue;
		if (ignores(d->character, ch, IGNORE_ALLIANCE))
			continue;
		if (find_politics_state(&house_control[house], house_control[i].vnum)
		    == POLITICS_ALLIANCE || GET_HOUSE_UID(d->character) == HOUSE_UNIQUE(house)) {
			sprintf(message, "%s союзникам: %s'%s'.%s\r\n",
				GET_NAME(ch), CCIGRN(d->character, C_NRM), msg, CCNRM(d->character, C_NRM));
			SEND_TO_Q(message, d);
		}
	}

	sprintf(message, "Вы союзникам: %s'%s'.%s\r\n", CCIGRN(ch, C_NRM), msg, CCNRM(ch, C_NRM));
	send_to_char(message, ch);

}

void House_id_channel(CHAR_DATA * ch, int huid, char *msg)
{
	char message[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;
	int i;

	for (d = descriptor_list; d; d = d->next) {
		if (!d->character ||
		    STATE(d) != CON_PLAYING ||
		    AFF_FLAGGED(d->character, AFF_DEAFNESS) ||
		    huid != GET_HOUSE_UID(d->character) || ch == d->character)
			continue;
		sprintf(message, "%s ВАШЕЙ ДРУЖИНЕ: %s'%s'%s\r\n",
			GET_NAME(ch), CCIRED(d->character, C_NRM), msg, CCNRM(d->character, C_NRM));
		SEND_TO_Q(message, d);
	}

	for (i = 0; i < num_of_houses; i++)
		if (house_control[i].unique == huid) {
			sprintf(message, "Вы дружине %s: %s'%s'.%s\r\n",
				house_control[i].sname, CCIRED(ch, C_NRM), msg, CCNRM(ch, C_NRM));
			send_to_char(message, ch);

			return;
		}
	send_to_char("Такой дружины нет.", ch);
}

void AHouse_id_channel(CHAR_DATA * ch, int huid, char *msg)
{
	char message[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;
	int i, house;

	if ((house = House_for_uid(huid)) == NOHOUSE)
		return;

	for (d = descriptor_list; d; d = d->next) {
		if (!d->character ||
		    STATE(d) != CON_PLAYING || AFF_FLAGGED(d->character, AFF_DEAFNESS) || ch == d->character)
			continue;

		if ((i = House_for_uid(GET_HOUSE_UID(d->character))) == NOHOUSE)
			continue;
		if (find_politics_state(&house_control[house], house_control[i].vnum)
		    == POLITICS_ALLIANCE || GET_HOUSE_UID(d->character) == huid) {
			sprintf(message, "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n",
				GET_NAME(ch), CCIGRN(d->character, C_NRM), msg, CCNRM(d->character, C_NRM));
			SEND_TO_Q(message, d);
		}
	}

	for (i = 0; i < num_of_houses; i++)
		if (house_control[i].unique == huid) {
			sprintf(message, "Вы союзникам %s: %s'%s'.%s\r\n",
				house_control[i].sname, CCIGRN(ch, C_NRM), msg, CCNRM(ch, C_NRM));
			send_to_char(message, ch);

			return;
		}
	send_to_char("Такой дружины нет.", ch);
}

ACMD(do_whohouse)
{
	House_list(ch);
}

void House_list(CHAR_DATA * ch)
{
	int house, num;
	DESCRIPTOR_DATA *d;

	if (!GET_HOUSE_UID(ch)) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}
	if ((house = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE) {
		GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
		return;
	}
	sprintf(buf,
		" Ваша дружина: %s%s%s.\r\n %sСейчас в игре Ваши соратники :%s\r\n\r\n",
		CCIRED(ch, C_NRM), House_name(ch), CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
	for (d = descriptor_list, num = 0; d; d = d->next) {
		if (!d->character || STATE(d) != CON_PLAYING || GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(house))
			continue;
		sprintf(buf2, "    %s\r\n", race_or_title(d->character));
		strcat(buf, buf2);
		num++;
	}
	sprintf(buf2, "\r\n Всего %d.\r\n", num);
	strcat(buf, buf2);
	send_to_char(buf, ch);
}

ACMD(do_whoclan)
{
	if (GET_HOUSE_RANK(ch) != RANK_KNIEZE) {
		send_to_char("Только воевода имеет доступ к полному списку!\r\n", ch);
		return;
	}
	House_list_all(ch);
}

void House_list_all(CHAR_DATA * ch)
{
	sh_int house_num;


	if (!GET_HOUSE_UID(ch)) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}

	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)) {
		send_to_char("Вам стоит войти в свой замок.\r\n", ch);
		return;
	}

	house_num = find_house(GET_ROOM_VNUM(IN_ROOM(ch)));
	if (house_num == NOHOUSE) {
		sprintf(buf, "Хмм.. Что-то не очень похоже на замок.\r\n");
		send_to_char(buf, ch);
		return;
	}

	if (house_control[house_num].owner != GET_ID(ch)) {
		send_to_char("Вы же не хозяин замка !\r\n", ch);
		return;
	}

	sprintf(buf,
		" Ваша дружина: %s%s%s.\r\n %sПолный список Ваших соратников :%s\r\n\r\n",
		CCIRED(ch, C_NRM), House_name(ch), CCNRM(ch, C_NRM), CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	House_list_guests(ch, house_num, TRUE);


}

struct lcs_tag {
	CHAR_DATA *ch;
	struct lcs_tag *link;
};
typedef struct lcs_tag lcs, *plcs;

/*
Распечатка списка членов кланов online
1. "дружины" - вывод списка зарегестрированных дружин
2. "дружины <имя>|'все'" - вывод членов конкретной/всех дружины
*/
ACMD(do_listclan)
{
	int clan_id, j, cnt;
	plcs head = NULL, item;
	char *nm;

	skip_spaces(&argument);
	if (*argument) {
		DESCRIPTOR_DATA *d;
		plcs ins_after, ins_before;

		for (clan_id = 0; clan_id < num_of_houses; ++clan_id)
			if (is_abbrev(argument, house_control[clan_id].sname))
				break;
		if (clan_id == num_of_houses && (is_abbrev(argument, "все") || is_abbrev(argument, "all")))
			clan_id = -1;

		if (clan_id == num_of_houses) {
			send_to_char("Такая дружина не зарегистрирована\r\n", ch);
			return;
		}
// Построение списка
		for (d = descriptor_list; d; d = d->next) {
			if (!d->character || STATE(d) != CON_PLAYING || GET_HOUSE_UID(d->character) == 0)
				continue;

			if (clan_id != -1 && GET_HOUSE_UID(d->character) != house_control[clan_id].unique)
				continue;

			if (!CAN_SEE_CHAR(ch, d->character) || IS_IMMORTAL(d->character))
				continue;

			// добавляю в список
			CREATE(item, lcs, 1);
			item->ch = d->character;
			item->link = NULL;

			ins_after = NULL;
			ins_before = head;
			while (ins_before) {
				if (!(GET_HOUSE_UID(item->ch) > GET_HOUSE_UID(ins_before->ch)
				      || GET_HOUSE_RANK(item->ch) <= GET_HOUSE_RANK(ins_before->ch)))
					break;
				ins_after = ins_before;
				ins_before = ins_before->link;
			}
			if (ins_after == NULL)
				head = item;
			else
				ins_after->link = item;
			item->link = ins_before;
		}
	} else {
		clan_id = -1;
	}

// clan_id - идентификатор дружины для вывода
// head - списо членов дружин

// Распечатка списка
	send_to_char("В игре зарегистрированы следующие дружины:\r\n", ch);
	send_to_char("     #                 Глава Название\r\n\r\n", ch);
	cnt = 0;
	for (j = 0; j < num_of_houses; ++j) {
		if (clan_id != -1 && clan_id != j)
			continue;
		if ((nm = get_name_by_id(house_control[j].owner)) == NULL)
			continue;
		++cnt;
		strcpy(buf1, nm);
		CAP(buf1);
		// название клана
		sprintf(buf, " %5d %5s %15s %s\r\n", j + 1, house_control[j].sname, buf1, house_control[j].name);
		send_to_char(buf, ch);
		// поиск членов клана в списке
		for (item = head; item; item = item->link) {
			if (GET_HOUSE_UID(item->ch) != house_control[j].unique)
				continue;
			buf1[0] = 0;
			if (PLR_FLAGGED(item->ch, PLR_KILLER))
				sprintf(buf1, " %s(ДУШЕГУБ)%s", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
			sprintf(buf, " %-10s%s%s%s%s\r\n",
				GET_HOUSE_RANK(item->ch) ? House_rank(item->ch) : "",
				CCPK(ch, C_NRM, item->ch), noclan_title(item->ch), CCNRM(ch, C_NRM), buf1);
			send_to_char(buf, ch);
		}
	}

// Удаление списка
	for (j = 0, item = head; item; item = head, ++j) {
		head = head->link;
		free(item);
	}

	sprintf(buf, "\r\n Всего игроков - %d" "\r\n Всего дружин  - %d\r\n", j, cnt);
	send_to_char(buf, ch);
}

// mode :
// HCE_ATRIUM - шаг из ATRIUM комнаты
// HCE_PORTAL - телепортация
int House_can_enter(CHAR_DATA * ch, room_rnum house, int mode)
{
	CHAR_DATA *mobs;
	int i, j;
	int ismember = 0;
	room_rnum hroom;

	// выяснить принадлежит ли комната внутренности замка
	hroom = House_vnum(house);

	// проверка не нужна
	if (hroom == NOWHERE)
		return 1;

	// hroom - rnum центра замка

	// Если это большой БОГ
	if (GET_LEVEL(ch) >= LVL_GRGOD)
		return 1;

	if (!ROOM_FLAGGED(house, ROOM_HOUSE))
		return 1;

	for (i = 0; i < num_of_houses; ++i) {
		if (get_name_by_id(house_control[i].owner) == NULL)
			continue;
		if (GET_ROOM_VNUM(hroom) == house_control[i].vnum)
			break;
	}
	// Бесхозный замок, входить можно
	if (i == num_of_houses)
		return 1;

	// публичный доступ
	if (house_control[i].mode != HOUSE_PRIVATE)
		return 1;

	if ((j = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE)
		return 0;

	// Проверяю входящего на принадлежность замку
	if (GET_IDNUM(ch) == house_control[i].owner ||
	    // Проверка, принадлежит ли персонаж к альянсу
	    find_politics_state(&house_control[i], house_control[j].vnum) == POLITICS_ALLIANCE)
		ismember = 1;
	for (j = 0; !ismember && j < house_control[i].num_of_guests; j++)
		if (GET_IDNUM(ch) == house_control[i].guests[j])
			ismember = 1;

	switch (mode) {
	case HCE_ATRIUM:	// попытка войти через дверь, 
		// контроль осуществляет охранник
		for (mobs = world[IN_ROOM(ch)]->people; mobs; mobs = mobs->next_in_room)
			if (house_control[i].keeper == GET_MOB_VNUM(mobs))
				break;
		if (!mobs)
			return 1;	// охранника нет, свободный доступ
		// break; -- не нужен, обычная проверка дальше с блокировкой входа

	case HCE_PORTAL:	// телепортация,
		// контроль осуществляют стены

		if (!ismember)
			return 0;	// не членов замка не пускать
		if (RENTABLE(ch))	// с флагом не пускать
		{
			if (mode == HCE_ATRIUM)
				send_to_char
				    ("Пускай сначала кровь с тебя стечет, а потом входи сколько угодно.\r\n", ch);
			return 0;
		}
		return 1;
	}

	return 0;
}

void hcontrol_set_guard(CHAR_DATA * ch, char *arg)
{
	int i, guard_vnum;
	char arg1[MAX_INPUT_LENGTH];

	arg = one_argument(arg, arg1);

	if (!*arg) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}

	if ((i = find_house(atoi(arg1))) == NOHOUSE) {
		send_to_char("Нет такого замка.\r\n", ch);
		return;
	}

	guard_vnum = atoi(arg);

	house_control[i].keeper = guard_vnum;
	sprintf(arg1, "Вы установили моба %ld на стражу замка %s", house_control[i].keeper, house_control[i].sname);
	act(arg1, FALSE, ch, 0, 0, TO_CHAR);
	House_save_control();
}

void hcontrol_set_prison(CHAR_DATA * ch, char *arg)
{
	int i, prison_vnum, prison_rnum = 0;
	char arg1[MAX_INPUT_LENGTH];

	arg = one_argument(arg, arg1);

	if (!*arg) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}

	if ((i = find_house(atoi(arg1))) == NOHOUSE) {
		send_to_char("Нет такого замка.\r\n", ch);
		return;
	}

	prison_vnum = atoi(arg);

	if (prison_vnum == 0) {
		house_control[i].prison = 0;
		sprintf(arg1, "Дружинная-темница убрана у замка %s", house_control[i].sname);
		act(arg1, FALSE, ch, 0, 0, TO_CHAR);
		House_save_control();
		return;
	}
	// Проверка на соответствие комнаты-темницы определенным стандартам.
	if ((prison_rnum = real_room(prison_vnum)) == NOWHERE) {
		send_to_char("Нет комнаты с заданным vnum.\r\n", ch);
		return;
	}
	if (prison_vnum == house_control[i].prison) {
		send_to_char("Эта комната уже является дружинной-темницей.\r\n", ch);
		return;
	}
	if (!is_rent(prison_rnum)) {
		send_to_char("В комнате отсутствует моб-рентер.\r\n", ch);
		return;
	}

	if (real_sector(prison_rnum) == SECT_FLYING || real_sector(prison_rnum) == SECT_UNDERWATER) {
		send_to_char("Неправильный тип сектора у комнаты.\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(prison_rnum, ROOM_DEATH) ||
	    ROOM_FLAGGED(prison_rnum, ROOM_SLOWDEATH) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_HOUSE) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_ARENA) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_NOSUMMON) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_NOTELEPORTOUT) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_NOTELEPORTIN) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_NOMAGIC) ||
	    !ROOM_FLAGGED(prison_rnum, ROOM_PEACEFUL) || !ROOM_FLAGGED(prison_rnum, ROOM_SOUNDPROOF)) {
		send_to_char("Проверьте наличие на комнате флагов:\r\n"
			     "!комната является частью замка!, !арена!, !незля призвать!, !нет магии!\r\n"
			     "!нельзя телепортироваться в эту комнату!, !глухая!, !мирная! !нельзя телепортироваться из этой комнаты!\r\n"
			     "Так-же в комнате должны отсутствовать флаги: !дт! !медленное дт!\r\n", ch);
		return;
	}

	house_control[i].prison = prison_vnum;
	sprintf(arg1, "Теперь комната %d - дружинная-темница замка %s", prison_vnum, house_control[i].sname);
	act(arg1, FALSE, ch, 0, 0, TO_CHAR);
	House_save_control();
}

/*const char *HCONTROL_FORMAT =
"Формат: hcontrol build[возвести] <house vnum> <exit direction> <player name> <shortname> [name]\r\n"
"        hcontrol rooms[залы] <house vnum> <room vnum> ...\r\n"
"        hcontrol destroy[разрушить] <house vnum>\r\n"
"        hcontrol pay[налог] <house vnum>\r\n"
"        hcontrol show[показать]\r\n"
"        hcontrol guard[охранник] <house vnum> <mob vnum>"; */

void hcontrol_list_houses(CHAR_DATA * ch)
{
	int i;
	char *timestr, *temp;
	char built_on[128], last_pay[128], own_name[128];

	if (!num_of_houses) {
		send_to_char("Родовые замки не определены.\r\n", ch);
		return;
	}
	strcpy(buf, "Комната  Вход    Возведен    Игроки  Князь        Налог внесен\r\n");
	strcat(buf, "-------  ------  ----------  ------  ------------ ------------\r\n");
	send_to_char(buf, ch);

	for (i = 0; i < num_of_houses; i++) {	/* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
		if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
			continue;

		if (house_control[i].built_on) {
			timestr = asctime(localtime(&(house_control[i].built_on)));
			*(timestr + 10) = '\0';
			strcpy(built_on, timestr);
		} else
			strcpy(built_on, " Вечен ");

		if (house_control[i].last_payment) {
			timestr = asctime(localtime(&(house_control[i].last_payment)));
			*(timestr + 10) = '\0';
			strcpy(last_pay, timestr);
		} else
			strcpy(last_pay, "Нет");

		/* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
		strcpy(own_name, temp);

		sprintf(buf, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
			house_control[i].vnum, house_control[i].atrium, built_on,
			house_control[i].num_of_guests, CAP(own_name), last_pay);

		send_to_char(buf, ch);
		House_list_guests(ch, i, TRUE);
		sprintf(buf, "Дружинная-темница: %d\r\n", house_control[i].prison);
		send_to_char(buf, ch);
		sprintf(buf, "Моб охранник: %ld\r\n", house_control[i].keeper);
		send_to_char(buf, ch);
	}
}

void hcontrol_build_house(CHAR_DATA * ch, char *arg)
{
	DESCRIPTOR_DATA *d;
	char arg1[MAX_INPUT_LENGTH];
	char sn[HOUSE_SNAME_LEN + 1];
	struct house_control_rec temp_house;
	room_vnum virt_house, virt_atrium, virt_rent;
	room_rnum real_house, real_atrium, real_rent;
	sh_int exit_num;
	long owner;
	int i;

	if (num_of_houses >= MAX_HOUSES) {
		send_to_char("Слишком много родовых замков.\r\n", ch);
		return;
	}

	/* first arg: house's vnum */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	virt_house = atoi(arg1);
	if ((real_house = real_room(virt_house)) == NOWHERE) {
		send_to_char("Нет такой комнаты.\r\n", ch);
		return;
	}
	if ((find_house(virt_house)) != NOHOUSE) {
		send_to_char("Эта комната уже занята под замок.\r\n", ch);
		return;
	}

	/* first arg b: house's vnum */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	virt_rent = atoi(arg1);
	if ((real_rent = real_room(virt_rent)) == NOWHERE) {
		send_to_char("Нет такой комнаты.\r\n", ch);
		return;
	}

	/* second arg: direction of house's exit */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if ((exit_num = search_block(arg1, dirs, FALSE)) < 0 && (exit_num = search_block(arg1, Dirs, FALSE)) < 0) {
		sprintf(buf, "'%s' не является верным направлением.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}
	if (TOROOM(real_house, exit_num) == NOWHERE) {
		sprintf(buf, "Не существует выхода на %s из комнаты %d.\r\n", Dirs[exit_num], virt_house);
		send_to_char(buf, ch);
		return;
	}

	real_atrium = TOROOM(real_house, exit_num);
	virt_atrium = GET_ROOM_VNUM(real_atrium);

	if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house) {
		send_to_char("Выход из замка не ведет обратно в замок.\r\n", ch);
		return;
	}

	/* third arg: player's name */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if ((owner = get_id_by_name(arg1)) < 0) {
		sprintf(buf, "Не существует игрока '%s'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}

	if (!(d = House_find_desc(owner))) {
		send_to_char("Создание замка допустимо только для играющего персонажа.\r\n", ch);
		return;
	}

	if (GET_HOUSE_UID(d->character)) {
		sprintf(buf, "Игрок %s приписан к уже существующему замку !", GET_NAME(d->character));
		send_to_char(buf, ch);
		return;
	}

	/* 4th arg: short clan name */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if (strlen(arg1) > HOUSE_SNAME_LEN) {
		send_to_char("Краткое название клана не должно быть такое большое.\r\n", ch);
		return;
	}
	strcpy(sn, arg1);

	skip_spaces(&arg);

	if (strlen(arg) > HOUSE_NAME_LEN - 1) {
		send_to_char("Полное название клана не должно быть такое большое.\r\n", ch);
		return;
	}

	memset(&temp_house, sizeof(struct house_control_rec), 0);
	temp_house.mode = HOUSE_PRIVATE;
	temp_house.vnum = virt_house;
	sprintf(temp_house.name, "%s", arg);
	for (i = 0; sn[i] != 0; i++)
		sn[i] = UPPER(sn[i]);
	sprintf(temp_house.sname, "%s", sn);
	temp_house.atrium = virt_atrium;
	temp_house.exit_num = exit_num;
	temp_house.prison = 0;
	temp_house.built_on = time(0);
	temp_house.last_payment = 0;
	temp_house.owner = owner;
	temp_house.num_of_guests = 0;
	temp_house.keeper = 0;
	temp_house.unique = House_create_unique();
	temp_house.closest_rent = virt_rent;
	for (i = 0; i < MAX_HOUSES; i++) {
		temp_house.politics_vnum[i] = 0;
		temp_house.politics_state[i] = 0;
	}

	house_control[num_of_houses++] = temp_house;

//  SET_BIT(ROOM_FLAGS(real_house, ROOM_HOUSE), ROOM_HOUSE);
//  SET_BIT(ROOM_FLAGS(real_house, ROOM_PRIVATE), ROOM_PRIVATE);
//  SET_BIT(ROOM_FLAGS(real_atrium, ROOM_ATRIUM), ROOM_ATRIUM);
	House_crashsave(virt_house);

	send_to_char("Замок возведен !\r\n", ch);
	House_save_control();

	GET_HOUSE_UID(d->character) = temp_house.unique;
	GET_HOUSE_RANK(d->character) = RANK_KNIEZE;
	SEND_TO_Q("Вы стали хозяином нового замка. Добро пожаловать !\r\n", d);
	save_char(d->character, NOWHERE);
}

/* Работа с новостями */
void House_fname(CHAR_DATA * ch, char *name)
{
	char id[MAX_INPUT_LENGTH];

	/* Вычисляем имя файла */
	strcpy(name, LIB_HOUSE);
	sprintf(id, "%ld", GET_HOUSE_UID(ch));
	strcat(name, id);
	strcat(name, ".hnews");
	/*  */
}

int House_news(DESCRIPTOR_DATA * d)
{
	char news[MAX_EXTEND_LENGTH];
	char fname[MAX_INPUT_LENGTH];

	House_fname(d->character, fname);
	if (!file_to_string(fname, news)) {
		SEND_TO_Q(news, d);
		return (1);
	}
	return (0);
}

void delete_house_news(CHAR_DATA * ch)
{
	char fname[MAX_INPUT_LENGTH];

	House_fname(ch, fname);
	remove(fname);
	send_to_char("Новости удалены!\r\n", ch);
}

void add_house_news(CHAR_DATA * ch, char *nw)
{
	char fname[MAX_INPUT_LENGTH];
	FILE *fl;

	House_fname(ch, fname);

	if (!(fl = fopen(fname, "a")))
		return;

	if (GET_HOUSE_RANK(ch) == RANK_KNIEZE)
		fprintf(fl, "%s\n", nw);
	else
		fprintf(fl, "[%s] %s\n", GET_NAME(ch), nw);

	fclose(fl);
	send_to_char("Ваша новость добавлена!\r\n", ch);

}


ACMD(do_newsclan)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	if (!GET_HOUSE_UID(ch)) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}

	half_chop(argument, arg1, arg2);

	if (!*buf || GET_HOUSE_RANK(ch) < RANK_CENTURION) {
		House_news(ch->desc);
		return;
	}

	if (is_abbrev(arg1, "стереть") && GET_HOUSE_RANK(ch) == RANK_KNIEZE)
		delete_house_news(ch);
	else if (is_abbrev(arg1, "добавить") && GET_HOUSE_RANK(ch) >= RANK_CENTURION)
		add_house_news(ch, arg2);
	else
		House_news(ch->desc);

}

/* Функция проверяет есть ли id чара в списке клана и если нет - убирает его */
void sync_char_with_clan(CHAR_DATA * ch)
{
	int j, i, id;

	if (GET_HOUSE_UID(ch) && !House_check_exist(GET_HOUSE_UID(ch)))
		GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;

	i = House_for_uid(GET_HOUSE_UID(ch));
	if (i == NOHOUSE)
		return;
	id = GET_IDNUM(ch);

	if (id == house_control[i].owner)
		return;

	for (j = 0; j < house_control[i].num_of_guests; j++)
		if (house_control[i].guests[j] == id)
			return;

	/* чара в списке нет - поэтому выкидываем его */
	GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;
}

void char_from_houses(CHAR_DATA * ch)
{
	int i, j;

	GET_HOUSE_UID(ch) = GET_HOUSE_RANK(ch) = 0;

	for (i = 0; i < num_of_houses; i++) {
		for (j = 0; j < house_control[i].num_of_guests; j++) {
			if (house_control[i].guests[j] == GET_IDNUM(ch)) {
				for (; j < house_control[i].num_of_guests; j++)
					house_control[i].guests[j] = house_control[i].guests[j + 1];
				house_control[i].num_of_guests--;
			}
		}
	}
}

void hcontrol_change_leader(CHAR_DATA * ch, char *arg)
{
	char arg1[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *new_lid, *old_lid, *d;
	long id;
	int i, j;
	char message[MAX_STRING_LENGTH];

	/* 1st arg: new leader name */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}

	if ((id = get_id_by_name(arg1)) < 0) {
		sprintf(buf, "Не существует игрока '%s'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}

	if (!(new_lid = House_find_desc(id))) {
		send_to_char("Нового лидера сейчас нет в игре.\r\n", ch);
		return;
	}

	/* 2nd arg: old leader name */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}

	if ((id = get_id_by_name(arg1)) < 0) {
		sprintf(buf, "Не существует игрока '%s'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	}

	if (!(old_lid = House_find_desc(id))) {
		send_to_char("Старого лидера сейчас нет в игре.\r\n", ch);
		return;
	}

	/* Проверяем - старый лидер действительно лидер клана? */
	j = 1;
	for (i = 0; i < num_of_houses; i++) {
		if (GET_IDNUM(old_lid->character) == house_control[i].owner)
			j = 0;
	}
	if (j) {
		send_to_char("Выбранный игрок не лидер клана!\r\n", ch);
		return;
	}

	/* Проверяем не является ли новый лидер уже лидером */
	for (i = 0; i < num_of_houses; i++) {
		if (GET_IDNUM(new_lid->character) == house_control[i].owner) {
			send_to_char("Назначаемый игрок уже лидер другого клана!\r\n", ch);
			return;
		}
	}

	/* Исключаем нового лидера из клана */
	char_from_houses(new_lid->character);

	/* Меняем лидера */
	for (i = 0; i < num_of_houses; i++) {
		if (GET_IDNUM(old_lid->character) == house_control[i].owner) {
			house_control[i].owner = GET_IDNUM(new_lid->character);
			break;
		}
	}
	GET_HOUSE_UID(new_lid->character) = GET_HOUSE_UID(old_lid->character);
	GET_HOUSE_RANK(new_lid->character) = RANK_KNIEZE;

	/* Исключаем старого лидера из клана */
	char_from_houses(old_lid->character);

	House_save_control();
	save_char(new_lid->character, NOWHERE);
	save_char(old_lid->character, NOWHERE);

	/* Выдаем клану сообщение */
	sprintf(buf, "Прощай %s. Долгие года новому князю %s!\r\n",
		GET_NAME(old_lid->character), GET_PAD(new_lid->character, 2));
	send_to_char(buf, ch);

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING || GET_HOUSE_UID(d->character) != GET_HOUSE_UID(new_lid->character))
			continue;
		sprintf(message, "%s: %s'%s'.%s\r\n",
			GET_NAME(ch), CCIRED(d->character, C_NRM), buf, CCNRM(d->character, C_NRM));
		SEND_TO_Q(message, d);
	}
}

/* Функция проверяет находится ли чар в зоне чужого клана */
int in_enemy_clanzone(CHAR_DATA * ch)
{
	int i, zn;

	zn = world[IN_ROOM(ch)]->zone;
	for (i = 0; i < num_of_houses; i++) {
		if (zn == world[real_room(house_control[i].vnum)]->zone) {
			if (GET_HOUSE_UID(ch) == HOUSE_UNIQUE(i) ||
			    find_politics_state(&house_control[i],
						house_control[House_for_uid
							      (GET_HOUSE_UID(ch))].vnum) == POLITICS_ALLIANCE) {
				return (FALSE);
			} else {
				return (TRUE);
			}
		}
	}
	return (FALSE);
}


//**************************************************************************
//****** Работа со списками кланов *****************************************
//**************************************************************************

void clanlist_getfilename(char *filename, int clanid, char *listname)
/*++
   Построение имени файла списка клана.
      filename - буфер для размещения имени файла
      clanid   - идентификатор клана
      listname - имя списка
--*/
{
	sprintf(filename, "%s%d.%s", LIB_HOUSE, clanid, listname);
}

int clanlist_searchid(long *id, char *name)
/*++
   Поиск id по имени
      name - имя персонажа
      id   - адрес, куда положить id (-1 - очень много людей)
   Возвращает
      1  - нашел
      -1 - не нашел
      0 - нашел имма
--*/
{
	int i, ret = -1;

	*id = -1;
	for (i = 0; i <= top_of_p_table; ++i) {
		if (is_abbrev(name, player_table[i].name)) {
			if (player_table[i].level >= LVL_IMMORT)
				return 0;
			ret = 1;
			if (*id != -1) {
				*id = -1;
				break;
			}
			*id = player_table[i].id;
		}
	}
	return ret;
}

char *clanlist_searchname(long id)
/*++
   Поиск имени по id
      id - идентификатор
   Возвращает имя или NULL
--*/
{
	int i;

	for (i = 0; i <= top_of_p_table; ++i)
		if (id == player_table[i].id)
			return player_table[i].name;
	return NULL;
}

char *clanlist_buildbuffer(struct clanlistitem *list)
/*++
   Создание буфера для отображения списка клана.
   Список уничтожается.
--*/
{
	int blen = 0;
	char *listbuf, *s1, *s2;
	char *ctm;
	struct clanlistitem *li = list;

// 1 - расчет размера буфера
	while (li) {
		blen += 88 + 2 * MAX_NAME_LENGTH;	// заголовок и накладные
		blen += strlen(li->msg);
		li = li->next;
	}

// 2 - создание буфера
	listbuf = (char *) malloc(blen);
	listbuf[0] = 0;

// 3 - заполнение буфера
	for (; list; list = li) {
		ctm = (char *) asctime(localtime(&list->msg_time));
		ctm[strlen(ctm) - 1] = 0;
		s1 = clanlist_searchname(list->author_id);
		s2 = clanlist_searchname(list->victim_id);
		sprintf(listbuf + strlen(listbuf),
			"#%ld: %s (%s) :: %s\r\n%s\r\n\r\n",
			list->msg_id, ctm, s1 ? s1 : "кто-то", s2 ? s2 : "кто-то", list->msg);

		li = list->next;
		free(list);
	}

	return listbuf;
}

struct clanlistitem *clanlist_additem(struct clanlistitem *list, struct clanlistitem *item)
/*++
   Добавляет элемент в список клана, сортировка по времени
   Возвращает новое начало списка   
--*/
{
	struct clanlistitem *head = list;
	if (!list || (list->msg_time > item->msg_time)) {
		item->next = list;
		return item;
	}

	while (list->next && item->msg_time >= list->next->msg_time) {
		list = list->next;
	}

	item->next = list->next;
	list->next = item;
	return head;
}

long clanlist_getcellsize(FILE * f, unsigned long *id)
/*++
   Получение размера и типа ячейки в файле
      -1 - нет больше ячеек
--*/
{
	unsigned long m[2];
	size_t s;

	s = fread(m, 4, 2, f);
	if (s < 2) {
		return -1;
	}
	*id = m[1];
	return m[0];
}

void clanlist_skipcell(FILE * f, long s)
/*++
   Пропуск ячейки (размер s)    
--*/
{
	fseek(f, s - 8, SEEK_CUR);
}

void clanlist_readcell(FILE * f, struct clanlistitem *li)
/*
   Считывание и заполнение элемента списка (size и id уже считаны)
   В элементе установлен параметр size
*/
{
	li->msg_id = ftell(f) - 8;
	fread(&li->msg_time, 1, li->size - 8, f);
}

int clanlist_deletecell(FILE * f, unsigned long trgt, long numid)
/*++
   Удаление записи
      1 - удалена
      0 - не найдена
     -1 - нет прав
   trgt - идентификатор записи
   numid - кто хочет удалить, -1 - самый главный
--*/
{
	long s, S;
	unsigned long id;

	fseek(f, 0, SEEK_SET);

	while ((unsigned long) ftell(f) != trgt) {
		s = clanlist_getcellsize(f, &id);
		if (s == -1)
			return 0;
		clanlist_skipcell(f, s);
	}
	// Нашел элемент
	S = clanlist_getcellsize(f, &id);
	if (S == -1 || id == 0xDEADBEEF)
		return 0;
	// Есть ли права?
	if (numid != -1) {
		struct clanlistitem li;
		li.size = 4 * 4;
		clanlist_readcell(f, &li);
		fseek(f, -8, SEEK_CUR);
		if (li.author_id != numid)
			return -1;
	}
	// Можно удалять
	// Считаю новый размер ячейки
	s = S;
	while (1) {
		clanlist_skipcell(f, s);
		s = clanlist_getcellsize(f, &id);
		if (s == -1 || id != 0xDEADBEEF)
			break;
		S += s;
	}

	fseek(f, trgt, SEEK_SET);
	fwrite(&S, 4, 1, f);
	S = 0xDEADBEEF;
	fwrite(&S, 4, 1, f);
	return 1;
}

int clanlist_addcell(FILE * f, struct clanlistitem *li)
/*++
   Добавление записи в файл
      TRUE - добавлена
      FALSE - нет
   В li должны быть заполнены все элементы. msg_id = 0
--*/
{
	unsigned long id;
	long s;

	fseek(f, 0, SEEK_SET);

	while (1) {
		s = clanlist_getcellsize(f, &id);
		if (s == -1) {
			return (fwrite(&li->size, 1, li->size, f) == (unsigned long) li->size);
		}
		if (id == 0xDEADBEEF && s > li->size) {
			fseek(f, -8, SEEK_CUR);
			s -= li->size;
			if (s < GARBAGE_SIZE) {
				li->size += s;
				return (fwrite(&li->size, 1, li->size, f) == (unsigned long) li->size);
			}
			fwrite(&li->size, 1, li->size, f);
			fwrite(&s, 4, 1, f);
			s = 0xDEADBEEF;
			fwrite(&s, 4, 1, f);
			return TRUE;
		}
		clanlist_skipcell(f, s);
	}

	return FALSE;
}

struct clanlistitem *clanlist_read(FILE * f, long numid)
/*++
   Чтение клановского файла
      f     - файл
      numid - фильтр жертвы, -1 все подряд
--*/
{
	struct clanlistitem *head = NULL, *li;
	unsigned long id;
	long s;

	fseek(f, 0, SEEK_SET);

	while (1) {
		s = clanlist_getcellsize(f, &id);
		if (s == -1)
			break;
		if (id == 0xDEADBEEF) {
			clanlist_skipcell(f, s);
		} else {
			li = (struct clanlistitem *) malloc(s + 4);
			li->size = s;
			clanlist_readcell(f, li);
			if (numid == -1 || numid == li->victim_id) {
				head = clanlist_additem(head, li);
			} else {
				free(li);
			}
		}
	}

	return head;
}

int clanlist_isonline(long id)
/*++
   Находится ли персонаж с идентификаторм id в игре?
--*/
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *tch;

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING)
			continue;
		tch = d->original ? d->original : d->character;
		if (!tch)
			continue;
		if (id == GET_IDNUM(tch) && !IS_IMMORTAL(tch))
			return TRUE;
	}
	return FALSE;
}

struct clanlistitem *clanlist_onlinefilter(struct clanlistitem *list)
/*++
   Фильтрация списка. В нем остаются только online персонажи
--*/
{
	struct clanlistitem *head = list, *prev = NULL, *next;

	for (; list; list = next) {
		next = list->next;

		if (clanlist_isonline(list->victim_id)) {
			// Персонаж в игре, перенос в новый список
			prev = list;
		} else {
			// Персонажа в игре нету, удалить из списка
			if (head == list)
				head = next;
			free(list);
		}
		if (prev)
			prev->next = next;
	}

	return head;
}

char *clanlistnames[] = { "pklist", "nopklist" };

ACMD(do_clanlist)
{
	FILE *f;
	int clanid = GET_HOUSE_UID(ch);
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char *arg3;
	char fn[FILENAME_MAX];
	int show = 0;		// Просто показать список
	int online = 1;		// Показывать только online персонажей

	clanid = GET_HOUSE_UID(ch);

	if (!clanid) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}

	if (subcmd > 1) {
		send_to_char("Списка не существует\r\n", ch);
		return;
	}
	clanlist_getfilename(fn, clanid, clanlistnames[subcmd]);

	f = fopen(fn, "r+b");
	if (f == NULL) {
		if (GET_HOUSE_RANK(ch) < RANK_CENTURION) {
			send_to_char("Ваша дружина не имеет такого списка.\r\n", ch);
			return;
		}
		f = fopen(fn, "w+b");
		if (f == NULL) {
			send_to_char("Невозможно создать список. Сообщите богам.\r\n", ch);
			return;
		}
		send_to_char("Вы создали новый список.\r\n", ch);
	}
	// Попытка выделить аргументы
	argument = one_argument(argument, arg1);
	arg3 = one_argument(argument, arg2);

	while (*arg1 && GET_HOUSE_RANK(ch) >= RANK_CENTURION) {
		if (is_abbrev(arg1, "add")) {
			long aid, vid;
			int m;
			show = 1;
			aid = GET_IDNUM(ch);
			m = clanlist_searchid(&vid, arg2);
			if (m == 0)
				send_to_char("Не дело это, Богов добавлять куда не надо....\r\n", ch);
			else if (!m)
				send_to_char("Интересующий Вас персонаж не зарегистрирован.\r\n", ch);
			else {
				if (vid == -1)
					send_to_char("Уточните имя персонажа.\r\n", ch);
			}
			if (vid != -1) {
				if (!arg3 || !*arg3) {
					send_to_char("За что вы его так?\r\n", ch);
				} else {
					int s = sizeof(struct clanlistitem) + strlen(arg3);
					struct clanlistitem *li;
					li = (struct clanlistitem *) malloc(s);
					li->size = s - 4;
					li->msg_id = 0;
					li->msg_time = time(0);
					li->author_id = aid;
					li->victim_id = vid;
					strcpy(li->msg, arg3);
					if (clanlist_addcell(f, li)) {
						send_to_char("Вы добавили запись в список.\r\n", ch);
					} else {
						send_to_char("Вам не удалось добавить запись в список!\r\n", ch);
					}
					free(li);
				}
			}
			break;
		}
		if (is_abbrev(arg1, "delete")) {
			unsigned long rec;
			long aid;
			int r;
			show = 1;

			if (!is_number(arg2)) {
				send_to_char("Необходимо указать номер удаляемой записи.\r\n", ch);
				break;
			}

			rec = atoi(arg2);
			aid = GET_IDNUM(ch);
			if (GET_HOUSE_RANK(ch) >= RANK_KNIEZE)
				aid = -1;
			r = clanlist_deletecell(f, rec, aid);
			if (r == 1) {
				send_to_char("Запись удалена.\r\n", ch);
			}
			if (r == 0) {
				send_to_char("Такой записи нету.\r\n", ch);
			}
			if (r == -1) {
				send_to_char("Не Вы добавляли, не Вам и удалять.\r\n", ch);
			}
			break;
		}
		if (is_abbrev(arg1, "clean")) {
			int clean_all = 0;
			struct clanlistitem *list, *li;

			show = 1;

			if (GET_HOUSE_RANK(ch) < RANK_KNIEZE) {
				send_to_char("Только воевода может использовать эту команду.\r\n", ch);
				break;
			}
			if (*arg2 && (is_abbrev(arg2, "все") || is_abbrev(arg2, "all"))) {
				clean_all = 1;
			}

			list = clanlist_read(f, -1);	// построить весь список

			// теперь удаляю все элементы списка
			for (; list; list = li) {
				if (clean_all || clanlist_searchname(list->victim_id) == 0) {
					clanlist_deletecell(f, list->msg_id, -1);
				}
				li = list->next;
				free(list);
			}

			if (clean_all)
				send_to_char("Вы очистили список.\r\n", ch);
			else
				send_to_char("Вы удалили устаревшие записи.\r\n", ch);
		}
		break;
	}
	while (!show) {
		// Показать список
		char header[128];
		struct clanlistitem *list;
		char *text;
		long numid = -1;
		if (*arg1) {

			// указан аргумент, может быть "all"
			if (is_abbrev(arg1, "все") || is_abbrev(arg1, "all")) {
				online = 0;	// Показать все
			} else {
				if (is_abbrev(arg2, "все") || is_abbrev(arg2, "all")) {
					online = 0;	// Показать все
				}

				if (!clanlist_searchid(&numid, arg1)) {
					send_to_char("Интересующий Вас персонаж не зарегистрирован.\r\n", ch);
				} else {
					if (numid == -1) {
						send_to_char("Уточните имя персонажа.\r\n", ch);
						break;
					}
				}

			}
		}

		list = clanlist_read(f, numid);

		if (online == 1) {
			list = clanlist_onlinefilter(list);
			sprintf(header,
				"%sОтображаются только находящиеся в игре персонажи%s\r\n\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		} else {
			sprintf(header,
				"%sСписок отображается полностью [all/все]%s\r\n\r\n",
				CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		send_to_char(header, ch);

		if (list) {
			text = clanlist_buildbuffer(list);
			page_string(ch->desc, text, TRUE);
			free(text);
		} else {
			send_to_char("Записи в выбранном списке отсутствуют.\r\n", ch);
		}
		break;
	}

	fclose(f);

	return;
}

////////////////////////////////////////////////////////////////////////
// клановая политика
////////////////////////////////////////////////////////////////////////
int find_politics_state(struct house_control_rec *house, room_vnum victim_vnum)
{
	int i;
	for (i = 0; i < MAX_HOUSES; i++)
		if (house->politics_vnum[i] == victim_vnum)
			return house->politics_state[i];
	return POLITICS_NEUTRAL;
}

void set_politics(struct house_control_rec *house, room_vnum victim_vnum, int state)
{
	int i, j = 0;
	for (i = MAX_HOUSES - 1; i >= 0; i--) {
		if (!house->politics_vnum[i])
			j = i;
		if (house->politics_vnum[i] == victim_vnum) {
			house->politics_state[i] = state;
			return;
		}
	}
	house->politics_vnum[j] = victim_vnum;
	house->politics_state[j] = state;
	return;
}

// "политика"

char *politicsnames[] = { "Нейтралитет", "Война", "Альянс" };


ACMD(do_show_politics)
{

	int clan_id, i, j, p1, p2;

	if (!GET_HOUSE_UID(ch)) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}
// print politics list
	sprintf(buf, "Отношения Вашей дружины с другими дружинами:\r\n");
	sprintf(buf, "%sНазвание     Отношение Вашей дружины     Отношение к Вашей дружине\r\n", buf);
	for (clan_id = 0; clan_id < num_of_houses; clan_id++) {
		if (house_control[clan_id].unique == GET_HOUSE_UID(ch))
			continue;
		if ((i = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE)
			return;
		if ((j = find_house(house_control[clan_id].vnum)) == NOHOUSE)
			continue;
		p1 = find_politics_state(&house_control[i], house_control[clan_id].vnum);
		p2 = find_politics_state(&house_control[j], house_control[i].vnum);
		sprintf(buf,
			"%s  %-3s             %s%-11s%s                 %s%-11s%s\r\n",
			buf, house_control[clan_id].sname,
			(p1 == POLITICS_WAR) ? CCIRED(ch,
						      C_NRM) : ((p1 ==
								 POLITICS_ALLIANCE) ?
								CCGRN(ch,
								      C_NRM) :
								CCNRM(ch, C_NRM)),
			politicsnames[p1], CCNRM(ch, C_NRM),
			(p2 == POLITICS_WAR) ? CCIRED(ch,
						      C_NRM) : ((p2 ==
								 POLITICS_ALLIANCE) ?
								CCGRN(ch,
								      C_NRM) :
								CCNRM(ch, C_NRM)), politicsnames[p2], CCNRM(ch, C_NRM));
	}
	send_to_char(buf, ch);

}

ACMD(do_set_politics)
{

	int clan_id, i;
	char message[MAX_STRING_LENGTH], arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;


	if (!GET_HOUSE_UID(ch)) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}

	if (GET_HOUSE_RANK(ch) < RANK_KNIEZE) {
		send_to_char("Только воевода может использовать эту команду.\r\n", ch);
		return;
	}
	if ((i = House_for_uid(GET_HOUSE_UID(ch))) == NOHOUSE)
		return;
	skip_spaces(&argument);
	two_arguments(argument, arg1, arg2);
	for (clan_id = 0; clan_id < num_of_houses; clan_id++)
		if (is_abbrev(arg1, house_control[clan_id].sname))
			break;
	if (clan_id == i) {
		send_to_char("Оригинально. Но глупо.\r\n", ch);
		return;
	}			//Поменять потом!!!

	if (!*arg1 || clan_id == num_of_houses) {
		switch (subcmd) {
		case SCMD_NEUTRAL:
			send_to_char("С кем Вы хотите заключить нейтралитет?\r\n", ch);
			break;
		case SCMD_WAR:
			send_to_char("С кем Вы хотите начать войну?\r\n", ch);
			break;
		case SCMD_ALLIANCE:
			send_to_char("С кем Вы хотите заключить альянс?\r\n", ch);
			break;
		}
		return;
	}
	if (!*arg2 || (strcmp(arg2, "заключить") && strcmp(arg2, "начать"))) {
		send_to_char("Формат команды: <действие> <имя дружины> <начать | заключить>\r\n", ch);
		return;
	}

	switch (subcmd) {
	case SCMD_NEUTRAL:
		if (find_politics_state(&house_control[i], house_control[clan_id].vnum)
		    == POLITICS_NEUTRAL) {
			send_to_char
			    ("Ваша дружина и так уже находится в состоянии нейтралитета с этой дружиной.\r\n", ch);
			return;
		}
		set_politics(&house_control[i], house_control[clan_id].vnum, POLITICS_NEUTRAL);
		sprintf(buf, "Вы заключили нейтралитет с дружиной %s.\r\n", house_control[clan_id].sname);
		send_to_char(buf, ch);
		break;
	case SCMD_WAR:
		if (find_politics_state(&house_control[i], house_control[clan_id].vnum)
		    == POLITICS_WAR) {
			send_to_char("Ваша дружина и так уже воюет с этой дружиной.\r\n", ch);
			return;
		}
		set_politics(&house_control[i], house_control[clan_id].vnum, POLITICS_WAR);
		for (d = descriptor_list; d; d = d->next) {
			if (!d->character || d->character == ch ||
			    STATE(d) != CON_PLAYING || GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(clan_id))
				continue;
			sprintf(message,
				"%sДружина %s объявила вашей дружине войну!!!%s\r\n",
				CCIRED(d->character, C_NRM), house_control[i].sname, CCNRM(d->character, C_NRM));
			SEND_TO_Q(message, d);
		}
		sprintf(buf, "Вы объявили войну дружине %s.\r\n", house_control[clan_id].sname);
		send_to_char(buf, ch);
		break;
	case SCMD_ALLIANCE:
		if (find_politics_state(&house_control[i], house_control[clan_id].vnum)
		    == POLITICS_ALLIANCE) {
			send_to_char("Ваша дружина и так уже в альянсе с этой дружиной.\r\n", ch);
			return;
		}
		set_politics(&house_control[i], house_control[clan_id].vnum, POLITICS_ALLIANCE);
		for (d = descriptor_list; d; d = d->next) {
			if (!d->character || d->character == ch ||
			    STATE(d) != CON_PLAYING || GET_HOUSE_UID(d->character) != HOUSE_UNIQUE(clan_id))
				continue;
			sprintf(message,
				"%sДружина %s заключила с вашей дружиной альянс!!!%s\r\n",
				CCGRN(d->character, C_NRM), house_control[i].sname, CCNRM(d->character, C_NRM));
			SEND_TO_Q(message, d);
		}
		sprintf(buf, "Вы заключили альянс с дружиной %s.\r\n", house_control[clan_id].sname);
		send_to_char(buf, ch);
		break;
	}
	House_save_control();
	return;

}

ACMD(do_prison)
{
	CHAR_DATA *vict = NULL;
	room_rnum prison_rnum = 0;
	int i;

	one_argument(argument, arg);

	if (GET_HOUSE_UID(ch) == 0) {
		send_to_char("Вы не приписаны ни к одному замку !\r\n", ch);
		return;
	}
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)) {
		send_to_char("Вам стоит войти в свой замок.\r\n", ch);
		return;
	}
	if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOHOUSE) {
		send_to_char("Хмм.. Что-то не очень похоже на замок.\r\n", ch);
		return;
	}
	if (GET_IDNUM(ch) != house_control[i].owner) {
		send_to_char("Вы же не хозяин этого замка !\r\n", ch);
		return;
	}
	if ((prison_rnum = real_room(house_control[i].prison)) == NOWHERE) {
		send_to_char("Похоже в Вашем замке дружинной-темницы не предусмотрено.\r\n", ch);
		return;
	}
	if (!*arg) {
		send_to_char("Кого Вы желаете поместить в темницу?\r\n", ch);
		return;
	}
	if (!(vict = get_player_vis(ch, arg, FIND_CHAR_WORLD))) {
		send_to_char("Вы не видите этого игрока.\r\n", ch);
		return;
	}
	if (!str_cmp(GET_NAME(vict), GET_NAME(ch))) {
		send_to_char("Не занимайтесь ерундой.\r\n", ch);
		return;
	}
	if (GET_HOUSE_UID(ch) != GET_HOUSE_UID(vict)) {
		sprintf(buf, "Но %s не является ратником Вашей дружины!\r\n", GET_NAME(vict));
		send_to_char(buf, ch);
		return;
	}
	if (RENTABLE(ch) || RENTABLE(vict)) {
		send_to_char("В крови в темницу посадить нельзя!\r\n", ch);
		return;
	}
	// Если чар еще не посажен куда-нибудь
	if (!(PLR_FLAGGED(vict, PLR_FROZEN) && FREEZE_DURATION(vict))) {
		if (!(PLR_FLAGGED(vict, PLR_NAMED) && NAME_DURATION(vict))) {
			if (!(PLR_FLAGGED(vict, PLR_HELLED) && HELL_DURATION(vict))) {
				if (world[ch->in_room]->zone == world[vict->in_room]->zone
				    && ROOM_FLAGGED(vict->in_room, ROOM_HOUSE)) {
					if (IN_ROOM(vict) != prison_rnum) {
						// садим чара в клан-темницу
						sprintf(buf, "%s$N поместил$G Вас в дружинную-темницу.%s",
							CCIRED(vict, C_NRM), CCNRM(vict, C_NRM));
						act(buf, FALSE, vict, 0, ch, TO_CHAR);
						act("$n водворен$a в дружинную-темницу !", FALSE, vict, 0, 0, TO_ROOM);
						char_from_room(vict);
						char_to_room(vict, prison_rnum);
						look_at_room(vict, prison_rnum);
						GET_LOADROOM(vict) = house_control[i].prison;
						act("$n водворен$a в дружинную-темницу !", FALSE, vict, 0, 0, TO_ROOM);
						sprintf(buf, "%s помещен%s в дружинную-темницу воеводой %s.",
							GET_NAME(vict), GET_CH_SUF_1(vict), GET_PAD(ch, 4));
						mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
						imm_log(buf);
					} else {
						// выпускаем чараз из клан-темницы
						send_to_char("Вас выпустили из темницы.\r\n", vict);
						act("$n выпущен$a из темницы !", FALSE, vict, 0, 0, TO_ROOM);
						char_from_room(vict);
						char_to_room(vict, IN_ROOM(ch));
						look_at_room(vict, IN_ROOM(ch));
						GET_LOADROOM(vict) = GET_ROOM_VNUM(IN_ROOM(ch));
						act("$n выпущен$a из темницы !", FALSE, vict, 0, 0, TO_ROOM);
						sprintf(buf, "%s выпущен%s из дружинной-темницы воеводой %s.",
							GET_NAME(vict), GET_CH_SUF_1(vict), GET_PAD(ch, 4));
						mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
						imm_log(buf);
					}
				} else {
					send_to_char
					    ("Для заточения в дружинной-темнице, реципиент должен находиться в клан замке.\r\n",
					     ch);
				}
			}
		}
	}
}
