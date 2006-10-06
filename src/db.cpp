/* ************************************************************************
*   File: db.cpp                                        Part of Bylins    *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
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

#define __DB_C__

#include "dirent.h"
#include "sys/stat.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mobmax.h"
#include "pk.h"
#include "olc.h"
#include "diskio.h"
#include "im.h"
#include "top.h"
#include "stuff.hpp"
#include "ban.hpp"
#include "privileges.hpp"
#include "item.creation.hpp"
#include "features.hpp"
#include "boards.h"
#include "description.h"
#include "deathtrap.hpp"

#define  TEST_OBJECT_TIMER   30

/**************************************************************************
*  declarations of global containers and objects                          *
**************************************************************************/
BanList *ban = 0;		// contains list of banned ip's and proxies + interface
PrivList *priv = 0;

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
long beginning_of_time = -1561789232;
#else
long beginning_of_time = 650336715;
#endif


//ROOM_DATA *world = NULL; /* array of rooms                */
vector < ROOM_DATA * >world;

room_rnum top_of_world = 0;	/* ref to top element of world   */

CHAR_DATA *character_list = NULL;	/* global linked list of
					 * chars         */

INDEX_DATA **trig_index;	/* index table for triggers      */
int top_of_trigt = 0;		/* top of trigger index table    */
long max_id = MOBOBJ_ID_BASE;	/* for unique mob/obj id's       */

INDEX_DATA *mob_index;		/* index table for mobile file   */
CHAR_DATA *mob_proto;		/* prototypes for mobs           */
mob_rnum top_of_mobt = 0;	/* top of mobile index table     */

int global_uid = 0;

OBJ_DATA *object_list = NULL;	/* global linked list of objs    */
INDEX_DATA *obj_index;		/* index table for object file   */
//OBJ_DATA *obj_proto;		/* prototypes for objs           */
vector < OBJ_DATA * >obj_proto;
obj_rnum top_of_objt = 0;	/* top of object index table     */

struct zone_data *zone_table;	/* zone table                    */
zone_rnum top_of_zone_table = 0;	/* top element of zone tab       */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages     */

struct player_index_element *player_table = NULL;	/* index to plr file     */
FILE *player_fl = NULL;		/* file desc of player file      */
int top_of_p_table = 0;		/* ref to top of table           */
int top_of_p_file = 0;		/* ref of size of p file         */
long top_idnum = 0;		/* highest idnum in use          */

int no_mail = 0;		/* mail disabled?                */
int mini_mud = 0;		/* mini-mud mode?                */
time_t boot_time = 0;		/* time of mud boot              */
int circle_restrict = 0;	/* level of game restriction     */
room_rnum r_mortal_start_room;	/* rnum of mortal start room     */
room_rnum r_immort_start_room;	/* rnum of immort start room     */
room_rnum r_frozen_start_room;	/* rnum of frozen start room     */
room_rnum r_helled_start_room;
room_rnum r_named_start_room;
room_rnum r_unreg_start_room;

char *credits = NULL;		/* game credits                  */
char *motd = NULL;		/* message of the day - mortals  */
char *rules = NULL;		/* rules for immorts             */
char *GREETINGS = NULL;		/* opening credits screen        */
char *help = NULL;		/* help screen                   */
char *info = NULL;		/* info page                     */
char *immlist = NULL;		/* list of peon gods             */
char *background = NULL;	/* background story              */
char *handbook = NULL;		/* handbook for new immortals    */
char *policies = NULL;		/* policies page                 */

struct help_index_element *help_table = 0;	/* the help table        */
int top_of_helpt = 0;		/* top of help index table       */

TIME_INFO_DATA time_info;	/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs     */
struct reset_q_type reset_q;	/* queue of zones to be reset    */
int supress_godsapply = FALSE;
const FLAG_DATA clear_flags = { {0, 0, 0, 0} };

struct cheat_list_type *cheaters_list;	/* Список разрешенных читеров */
struct portals_list_type *portals_list;	/* Список проталов для townportal */
int now_entrycount = FALSE;
extern int reboot_uptime;


//Polos.inserd_wanted_gem
class insert_wanted_gem iwg;
//-Polos.insert_wanted_gem

/* local functions */
void SaveGlobalUID(void);
void LoadGlobalUID(void);
int check_object_spell_number(OBJ_DATA * obj, int val);
int check_object_level(OBJ_DATA * obj, int val);
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode, char *filename);
int check_object(OBJ_DATA *);
void parse_trigger(FILE * fl, int virtual_nr);
void parse_room(FILE * fl, int virtual_nr, int virt);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE * fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void init_spec_procs(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE * fl);
int count_hash_records(FILE * fl);
int count_social_records(FILE * fl, int *messages, int *keywords);
void asciiflag_conv(char *flag, void *value);
void parse_simple_mob(FILE * mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE * mob_f, int i, int nr);
void get_one_line(FILE * fl, char *buf);
void save_etext(CHAR_DATA * ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
long get_ptable_by_name(char *name);
int mobs_in_room(int m_num, int r_num);
void new_build_player_index(void);
void renum_obj_zone(void);
void renum_mob_zone(void);
int get_zone_rooms(int, int *, int *);
void save_char(CHAR_DATA * ch, room_rnum load_room);
void init_guilds(void);
void init_basic_values(void);
int init_grouping(void);
void init_portals(void);
void init_im(void);

//MZ.load
void init_zone_types(void);
//-MZ.load


/* external functions */
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);	/* In objsave.cpp */
void sort_commands(void);
void sort_spells(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
int find_name(const char *name);
int hsort(const void *a, const void *b);
int csort(const void *a, const void *b);
void prune_crlf(char *txt);
void save_char_vars(CHAR_DATA * ch);
void Crash_read_timer(int index, int temp);
void Crash_clear_objects(int index);
void extract_mob(CHAR_DATA * ch);
//F@N|
int exchange_database_load(void);

void add_follower(CHAR_DATA * ch, CHAR_DATA * leader);
void load_socials(FILE * fl);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void name_from_drinkcon(OBJ_DATA * obj);
void name_to_drinkcon(OBJ_DATA * obj, int type);
void calc_easter(void);
extern void calc_god_celebrate();
void do_start(CHAR_DATA * ch, int newbie);
int calc_loadroom(CHAR_DATA * ch);
void die_follower(CHAR_DATA * ch);
extern void tascii(int *pointer, int num_planes, char *ascii);
extern void repop_decay(zone_rnum zone);	/* рассыпание обьектов ITEM_REPOP_DECAY */
int real_zone(int number);
int level_exp(CHAR_DATA * ch, int level);
extern void NewNameRemove(CHAR_DATA * ch);
extern void NewNameLoad();

/* external vars */
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum helled_start_room;
extern room_vnum named_start_room;
extern room_vnum unreg_start_room;
extern DESCRIPTOR_DATA *descriptor_list;
extern const char *unused_spellname;
extern int top_of_socialm;
extern int top_of_socialk;
extern struct month_temperature_type year_temp[];
extern const char *pc_class_types[];
extern char *house_rank[];
extern struct pclean_criteria_data pclean_criteria[];
extern void LoadProxyList();

#define READ_SIZE 256

/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
	char *tmp = argument, *ttag = tag, *wrt = argument;
	int i;

	for (i = 0; i < 4; i++)
		*(ttag++) = *(tmp++);
	*ttag = '\0';

	while (*tmp == ':' || *tmp == ' ')
		tmp++;

	while (*tmp)
		*(wrt++) = *(tmp++);
	*wrt = '\0';
}

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

void go_boot_socials(void)
{
	int i;

	if (soc_mess_list) {
		for (i = 0; i <= top_of_socialm; i++) {
			if (soc_mess_list[i].char_no_arg)
				free(soc_mess_list[i].char_no_arg);
			if (soc_mess_list[i].others_no_arg)
				free(soc_mess_list[i].others_no_arg);
			if (soc_mess_list[i].char_found)
				free(soc_mess_list[i].char_found);
			if (soc_mess_list[i].others_found)
				free(soc_mess_list[i].others_found);
			if (soc_mess_list[i].vict_found)
				free(soc_mess_list[i].vict_found);
			if (soc_mess_list[i].not_found)
				free(soc_mess_list[i].not_found);
		}
		free(soc_mess_list);
	}
	if (soc_keys_list) {
		for (i = 0; i <= top_of_socialk; i++)
			if (soc_keys_list[i].keyword)
				free(soc_keys_list[i].keyword);
		free(soc_keys_list);
	}
	top_of_socialm = -1;
	top_of_socialk = -1;
	index_boot(DB_BOOT_SOCIAL);


}

void go_boot_xhelp(void)
{
	int i;
	if (help_table) {
		for (i = 0; i <= top_of_helpt; i++) {
			if (help_table[i].keyword)
				free(help_table[i].keyword);
			if (help_table[i].entry && !help_table[i].duplicate)
				free(help_table[i].entry);
		}
		free(help_table);
	}
	top_of_helpt = 0;
	index_boot(DB_BOOT_HLP);
}

void load_sheduled_reboot()
{
	FILE *sch;
	int day = 0, hour = 0, minutes = 0, numofreaded = 0, timeOffset = 0;
	char str[10];
	int offsets[7];

	time_t currTime;

	sch = fopen(LIB_MISC "schedule", "r");
	if (!sch) {
		log("Error opening schedule");
		return;
	}
	time(&currTime);
	for (int i = 0; i < 7; i++) {
		if (fgets(str, 10, sch) == NULL) {
			break;
		}
		numofreaded = sscanf(str, "%i %i:%i", &day, &hour, &minutes);
		if (numofreaded < 1) {
			continue;
		}
		if (day != i) {
			break;
		}
		if (numofreaded == 1) {
			offsets[i] = 24 * 60;
			continue;
		}
		offsets[i] = hour * 60 + minutes;
	}
	fclose(sch);

	timeOffset = (int) difftime(currTime, boot_time);

	//set reboot_uptime equal to current uptime and align to the start of the day
	reboot_uptime = timeOffset / 60 - localtime(&currTime)->tm_min - 60 * (localtime(&currTime)->tm_hour);
	for (int i = localtime(&boot_time)->tm_wday; i < 7; i++) {
		//7 empty days was cycled - break with default uptime
		if (reboot_uptime - 1440 * 7 >= 0) {
			reboot_uptime = DEFAULT_REBOOT_UPTIME;	//2 days reboot bu default if no schedule
			break;
		}
		//if we get non-1-full-day offset, but server will reboot to early (36 hour is minimum)
		//we are going to find another scheduled day
		if (offsets[i] < 24 * 60 && (reboot_uptime + offsets[i]) < UPTIME_THRESHOLD * 60) {
			reboot_uptime += 24 * 60;
		}
		//we've found next point of reboot! :) break cycle
		if (offsets[i] < 24 * 60 && (reboot_uptime + offsets[i]) > UPTIME_THRESHOLD * 60) {
			reboot_uptime += offsets[i];
			break;
		}
		//empty day - add 24 hour and go next
		if (offsets[i] == 24 * 60) {
			reboot_uptime += offsets[i];
		}
		// jump to 1st day of the week
		if (i == 6)
			i = -1;
	}
	log("Setting up reboot_uptime: %i", reboot_uptime);
}

/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
	one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
		file_to_string_alloc(IMMLIST_FILE, &immlist);
		file_to_string_alloc(CREDITS_FILE, &credits);
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(RULES_FILE, &rules);
		file_to_string_alloc(HELP_PAGE_FILE, &help);
		file_to_string_alloc(INFO_FILE, &info);
		file_to_string_alloc(POLICIES_FILE, &policies);
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
		file_to_string_alloc(BACKGROUND_FILE, &background);
		go_boot_xhelp();
		go_boot_socials();
		init_im();
		init_zone_types();
		init_portals();
		priv->reload();
		load_sheduled_reboot();
		oload_table.init();
	} else if (!str_cmp(arg, "portals"))
		init_portals();
	else if (!str_cmp(arg, "privileges"))
		priv->reload();
	else if (!str_cmp(arg, "imagic"))
		init_im();
	else if (!str_cmp(arg, "ztypes"))
		init_zone_types();
	else if (!str_cmp(arg, "oloadtable"))
		oload_table.init();
	else if (!str_cmp(arg, "immlist"))
		file_to_string_alloc(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "credits"))
		file_to_string_alloc(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		file_to_string_alloc(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "rules"))
		file_to_string_alloc(RULES_FILE, &rules);
	else if (!str_cmp(arg, "help"))
		file_to_string_alloc(HELP_PAGE_FILE, &help);
	else if (!str_cmp(arg, "info"))
		file_to_string_alloc(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		file_to_string_alloc(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		file_to_string_alloc(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "greetings")) {
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
	} else if (!str_cmp(arg, "xhelp"))
		go_boot_xhelp();
	else if (!str_cmp(arg, "socials"))
		go_boot_socials();
	else if (!str_cmp(arg, "schedule"))
		load_sheduled_reboot();
	else if (!str_cmp(arg, "clan"))
		Clan::ClanLoad();
	else if (!str_cmp(arg, "godlist"))
		GodListLoad();
	else if (!str_cmp(arg, "proxy"))
		LoadProxyList();
	else if (!str_cmp(arg, "boards"))
		Board::BoardInit();
	else {
		send_to_char("Неверный параметр для перезагрузки файлов.\r\n", ch);
		return;
	}

	send_to_char(OK, ch);
}

void init_portals(void)
{
	FILE *portal_f;
	char nm[300], nm2[300], *wrd;
	int rnm = 0, i, level = 0;
	struct portals_list_type *curr, *prev;

	/* Сначала освобождаем все порталы */
	for (curr = portals_list; curr; curr = prev) {
		prev = curr->next_portal;
		free(curr->wrd);
		free(curr);
	}

	portals_list = NULL;
	prev = NULL;

	/* читаем файл */
	if (!(portal_f = fopen(LIB_MISC "portals.lst", "r"))) {
		log("Can not open portals.lst");
		return;
	}

	while (get_line(portal_f, nm)) {
		if (!nm[0] || nm[0] == ';')
			continue;
		sscanf(nm, "%d %d %s", &rnm, &level, nm2);
		if (real_room(rnm) == NOWHERE || nm2[0] == '\0') {
			log("Invalid portal entry detected");
			continue;
		}
		wrd = nm2;
		for (i = 0; !(i == 10 || wrd[i] == ' ' || wrd[i] == '\0'); i++);
		wrd[i] = '\0';
		/* добавляем портал в список - rnm - комната, wrd - слово */
		CREATE(curr, struct portals_list_type, 1);
		CREATE(curr->wrd, char, strlen(wrd) + 1);
		curr->vnum = rnm;
		curr->level = level;
		for (i = 0, curr->wrd[i] = '\0'; wrd[i]; i++)
			curr->wrd[i] = LOWER(wrd[i]);
		curr->wrd[i] = '\0';
		curr->next_portal = NULL;
		if (!portals_list)
			portals_list = curr;
		else
			prev->next_portal = curr;

		prev = curr;

	}
	fclose(portal_f);
}

void init_cheaters(void)
{
	FILE *ch_file;
	char name[300];
	struct cheat_list_type *curr_ch, *prev_ch;
	int i;

	cheaters_list = NULL;
	prev_ch = NULL;

	/* читаем файл */
	if (!(ch_file = fopen(LIB_MISC "cheaters.lst", "r"))) {
		log("Can not open cheaters.lst");
		return;
	}
	while (get_line(ch_file, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		/* добавляем имя в список */
		CREATE(curr_ch, struct cheat_list_type, 1);
		CREATE(curr_ch->name, char, strlen(name) + 1);
		for (i = 0, curr_ch->name[i] = '\0'; name[i]; i++)
			curr_ch->name[i] = LOWER(name[i]);
		curr_ch->name[i] = '\0';
		curr_ch->next_name = NULL;
		if (!cheaters_list)
			cheaters_list = curr_ch;
		else
			prev_ch->next_name = curr_ch;

		prev_ch = curr_ch;
	}
	fclose(ch_file);
}

void boot_world(void)
{
	log("Loading zone table.");
	index_boot(DB_BOOT_ZON);

	log("Loading triggers and generating index.");
	index_boot(DB_BOOT_TRG);

	log("Loading rooms.");
	index_boot(DB_BOOT_WLD);

	log("Renumbering rooms.");
	renum_world();

	log("Checking start rooms.");
	check_start_rooms();

	log("Loading mobs and generating index.");
	index_boot(DB_BOOT_MOB);

	log("Count mob qwantity by level");
	mob_lev_count();

	log("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);

	log("Renumbering zone table.");
	renum_zone_table();

	log("Renumbering Obj_zone.");
	renum_obj_zone();

	log("Renumbering Mob_zone.");
	renum_mob_zone();

	if (!no_specials) {
		log("Loading shops.");
		index_boot(DB_BOOT_SHP);
	}
}

//MZ.load
void init_zone_types(void)
{
	FILE *zt_file;
	char tmp[1024], dummy[128], name[128], itype_num[128];
	int names = 0;
	int i, j, k, n;

	if (zone_types != NULL)
	{
		for (i = 0; *zone_types[i].name != '\n'; i++)
		{
			if (zone_types[i].ingr_qty > 0)
				free(zone_types[i].ingr_types);

			free(zone_types[i].name);
		}
		free(zone_types[i].name);
		free(zone_types);
		zone_types = NULL;
	}

	zt_file = fopen(LIB_MISC "ztypes.lst", "r");
	if (!zt_file)
	{
		log("Can not open ztypes.lst");
		return;
	}

	while (get_line(zt_file, tmp))
	{
		if (!strn_cmp(tmp, "ИМЯ", 3))
		{
			if (sscanf(tmp, "%s %s", dummy, name) != 2)
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!get_line(zt_file, tmp))
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!strn_cmp(tmp, "ТИПЫ", 4))
			{
				if (tmp[4] != ' ' && tmp[4] != '\0')
				{
					log("Corrupted file : ztypes.lst");
					return;
				}
				for (i = 4; tmp[i] != '\0'; i++)
				{
					if (!isdigit(tmp[i]) && !isspace(tmp[i]))
					{
						log("Corrupted file : ztypes.lst");
						return;
					}
				}
			}
			else
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			names++;
		}
		else
		{
			log("Corrupted file : ztypes.lst");
			return;
		}
	}
	names++;

	CREATE(zone_types, struct zone_type, names);
	for (i = 0; i < names; i++)
	{
		zone_types[i].name = NULL;
		zone_types[i].ingr_qty = 0;
		zone_types[i].ingr_types = NULL;
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp))
	{
		sscanf(tmp, "%s %s", dummy, name);
		for (j = 0; name[j] != '\0'; j++)
			if (name[j] == '_')
				name[j] = ' ';
		zone_types[i].name = str_dup(name);
		if (get_line(zt_file, tmp));
		for (j = 4; tmp[j] != '\0'; j++)
		{
			if (isspace(tmp[j]))
				continue;
			zone_types[i].ingr_qty++;
			for (; tmp[j] != '\0' && isdigit(tmp[j]); j++);
			j--;
		}
		i++;
	}
	zone_types[i].name = str_dup("\n");

	for (i = 0; *zone_types[i].name != '\n'; i++)
		if (zone_types[i].ingr_qty > 0)
			CREATE(zone_types[i].ingr_types, int, zone_types[i].ingr_qty);

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp))
	{
		if (get_line(zt_file, tmp));
		for (j = 4, n = 0; tmp[j] != '\0'; j++)
		{
			if (isspace(tmp[j]))
				continue;
			for (k = 0; tmp[j] != '\0' && isdigit(tmp[j]); j++)
				itype_num[k++] = tmp[j];
			itype_num[k] = '\0';
			zone_types[i].ingr_types[n] = atoi(itype_num);
			n++;
			j--;
		}
		i++;
	}

	fclose(zt_file);
}
//-MZ.load


/* body of the booting system */
void boot_db(void)
{
	zone_rnum i;

	log("Boot db -- BEGIN.");

	log("Resetting the game time:");
	reset_time();

	log("Reading credits, help, bground, info & motds.");
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(RULES_FILE, &rules);
	file_to_string_alloc(HELP_PAGE_FILE, &help);
	file_to_string_alloc(INFO_FILE, &info);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
	file_to_string_alloc(POLICIES_FILE, &policies);
	file_to_string_alloc(HANDBOOK_FILE, &handbook);
	file_to_string_alloc(BACKGROUND_FILE, &background);
	if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
		prune_crlf(GREETINGS);

	log("Loading spell definitions.");
	mag_assign_spells();

	log("Loading feature definitions.");
	assign_feats();

	log("Booting IM");
	init_im();

//MZ.load
	log("Booting zone types and ingredient types for each zone type.");
	init_zone_types();
//-MZ.load

//Polos.insert_wanted_gem
	log("Booting inser_wanted.lst.");
	iwg.init();
//Polos.insert_wanted_gem

	boot_world();

	log("Booting stuff load table.");
	oload_table.init();

	log("Loading help entries.");
	index_boot(DB_BOOT_HLP);

	log("Loading social entries.");
	index_boot(DB_BOOT_SOCIAL);

	log("Generating player index.");
	build_player_index();

	log("Loading fight messages.");
	load_messages();

	log("Assigning function pointers:");

	if (!no_specials) {
		log("   Mobiles.");
		assign_mobiles();
		log("   Shopkeepers.");
		assign_the_shopkeepers();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}

	log("Assigning spell and skill levels.");
	init_spell_levels();

	log("Sorting command list and spells.");
	sort_commands();
	sort_spells();

	log("Booting mail system.");
	if (!scan_file()) {
		log("    Mail boot failed -- Mail system disabled");
		no_mail = 1;
	}
	log("Reading banned site, proxy, privileges and invalid-name list.");
	ban = new BanList();
	Read_Invalid_List();
	priv = new PrivList();

	log("Booting rented objects info");
	for (i = 0; i <= top_of_p_table; i++) {
		(player_table + i)->timer = NULL;
		Crash_read_timer(i, FALSE);
	}

	// последовательность лоада кланов/досок/иммов не менять
	log("Booting boards");
	Board::BoardInit();
	log("Booting clans");
	Clan::ClanLoad();
	log("Booting GodList");
	GodListLoad();

	for (i = 0; i <= top_of_zone_table; i++) {
		log("Resetting %s (rooms %d-%d).", zone_table[i].name,
		    (i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
		reset_zone(i);
	}

	reset_q.head = reset_q.tail = NULL;

	boot_time = time(0);
	//Конец изменений Ладником

	log("Booting basic values");
	init_basic_values();

	log("Booting grouping parameters");
	if (init_grouping())
		exit(1);

	log("Booting specials assigment");
	init_spec_procs();

	log("Booting guilds");
	init_guilds();

	log("Booting portals for 'town portal' spell");
	portals_list = NULL;
	init_portals();

	log("Booting allowed cheaters");
	init_cheaters();

	log("Booting maked items");
	init_make_items();

	log("Booting exchange");
	exchange_database_load();

	log("Load shedule reboot time");
	load_sheduled_reboot();

	log("Load proxy list");
	LoadProxyList();

	log("Load new_name list");
	NewNameLoad();

	log("Load global uid counter");
	LoadGlobalUID();

	log("Init DeathTrap list.");
	DeathTrap::load();

	log("Boot db -- DONE.");
}

/* free rooms structures and pointers*/
void free_rooms()
{
	vector < ROOM_DATA * >::iterator p = world.begin();

	while (p != world.end()) {
		room_free(*p);
		free(*p);
		p++;
	}
}

/* body of the close world system */
void free_db()
{
	log("Free db -- BEGIN.");

	// Free all room pointers.
	log("Free rooms.");
	free_rooms();


	log("Free db -- DONE.");
}

/* reset the time in the game from file */
void reset_time(void)
{
	time_info = *mud_time_passed(time(0), beginning_of_time);
	// Calculate moon day
	weather_info.moon_day =
	    ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % MOON_CYCLE;
	weather_info.week_day_mono =
	    ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % WEEK_CYCLE;
	weather_info.week_day_poly =
	    ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % POLY_WEEK_CYCLE;
	// Calculate Easter
	calc_easter();
	calc_god_celebrate();

	if (time_info.hours < sunrise[time_info.month][0])
		weather_info.sunlight = SUN_DARK;
	else if (time_info.hours == sunrise[time_info.month][0])
		weather_info.sunlight = SUN_RISE;
	else if (time_info.hours < sunrise[time_info.month][1])
		weather_info.sunlight = SUN_LIGHT;
	else if (time_info.hours == sunrise[time_info.month][1])
		weather_info.sunlight = SUN_SET;
	else
		weather_info.sunlight = SUN_DARK;

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);

	weather_info.temperature = (year_temp[time_info.month].med * 4 +
				    number(year_temp[time_info.month].min, year_temp[time_info.month].max)) / 5;
	weather_info.pressure = 960;
	if ((time_info.month >= MONTH_MAY) && (time_info.month <= MONTH_NOVEMBER))
		weather_info.pressure += dice(1, 50);
	else
		weather_info.pressure += dice(1, 80);

	weather_info.change = 0;
	weather_info.weather_type = 0;

	if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_MAY)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month == MONTH_MART && weather_info.temperature >= 3)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month >= MONTH_JUNE && time_info.month <= MONTH_AUGUST)
		weather_info.season = SEASON_SUMMER;
	else if (time_info.month >= MONTH_SEPTEMBER && time_info.month <= MONTH_OCTOBER)
		weather_info.season = SEASON_AUTUMN;
	else if (time_info.month == MONTH_NOVEMBER && weather_info.temperature >= 3)
		weather_info.season = SEASON_AUTUMN;
	else
		weather_info.season = SEASON_WINTER;

	if (weather_info.pressure <= 980)
		weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.pressure <= 1000) {
		weather_info.sky = SKY_RAINING;
		if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_OCTOBER)
			create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 40, 40, 20);
		else if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
			create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 50, 40, 10);
		else if (time_info.month == MONTH_NOVEMBER || time_info.month == MONTH_MART) {
			if (weather_info.temperature >= 3)
				create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 70, 30, 0);
			else
				create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 80, 20, 0);
		}
	} else if (weather_info.pressure <= 1020)
		weather_info.sky = SKY_CLOUDY;
	else
		weather_info.sky = SKY_CLOUDLESS;
}



/* generate index table for the player file */
void build_player_index(void)
{
	new_build_player_index();
	return;
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *      -gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE * fl)
{
	char key[READ_SIZE], next_key[READ_SIZE];
	char line[READ_SIZE], *scan;
	int total_keywords = 0;

	/* get the first keyword line */
	get_one_line(fl, key);

	while (*key != '$') {	/* skip the text */
		do {
			get_one_line(fl, line);
			if (feof(fl))
				goto ackeof;
		}
		while (*line != '#');

		/* now count keywords */
		scan = key;
		do {
			scan = one_word(scan, next_key);
			if (*next_key)
				++total_keywords;
		}
		while (*next_key);

		/* get next keyword line (or $) */
		get_one_line(fl, key);

		if (feof(fl))
			goto ackeof;
	}

	return (total_keywords);

	/* No, they are not evil. -gg 6/24/98 */
      ackeof:
	log("SYSERR: Unexpected end of help file.");
	exit(1);		/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
	char buf[128];
	int count = 0;

	while (fgets(buf, 128, fl))
		if (*buf == '#')
			count++;

	return (count);
}

int count_social_records(FILE * fl, int *messages, int *keywords)
{
	char key[READ_SIZE], next_key[READ_SIZE];
	char line[READ_SIZE], *scan;

	/* get the first keyword line */
	get_one_line(fl, key);

	while (*key != '$') {	/* skip the text */
		do {
			get_one_line(fl, line);
			if (feof(fl))
				goto ackeof;
		}
		while (*line != '#');

		/* now count keywords */
		scan = key;
		++(*messages);
		do {
			scan = one_word(scan, next_key);
			if (*next_key)
				++(*keywords);
		}
		while (*next_key);

		/* get next keyword line (or $) */
		get_one_line(fl, key);

		if (feof(fl))
			goto ackeof;
	}

	return (TRUE);

	/* No, they are not evil. -gg 6/24/98 */
      ackeof:
	log("SYSERR: Unexpected end of help file.");
	exit(1);		/* Some day we hope to handle these things better... */
}




void index_boot(int mode)
{
	const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
	FILE *index, *db_file;

	int rec_count = 0, counter;
	int size[2];


	log("Index booting %d", mode);


	switch (mode) {
	case DB_BOOT_TRG:
		prefix = TRG_PREFIX;
		break;
	case DB_BOOT_WLD:
		prefix = WLD_PREFIX;
		break;
	case DB_BOOT_MOB:
		prefix = MOB_PREFIX;
		break;
	case DB_BOOT_OBJ:
		prefix = OBJ_PREFIX;
		break;
	case DB_BOOT_ZON:
		prefix = ZON_PREFIX;
		break;
	case DB_BOOT_SHP:
		prefix = SHP_PREFIX;
		break;
	case DB_BOOT_HLP:
		prefix = HLP_PREFIX;
		break;
	case DB_BOOT_SOCIAL:
		prefix = SOC_PREFIX;
		break;
	default:
		log("SYSERR: Unknown subcommand %d to index_boot!", mode);
		exit(1);
	}


	if (mini_mud)
		index_filename = MINDEX_FILE;
	else
		index_filename = INDEX_FILE;

	sprintf(buf2, "%s%s", prefix, index_filename);

	if (!(index = fopen(buf2, "r"))) {
		log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
		exit(1);
	}

	/* first, count the number of records in the file so we can malloc */
	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$') {
		sprintf(buf2, "%s%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r"))) {
			log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix, index_filename, strerror(errno));
			fscanf(index, "%s\n", buf1);
			continue;
		} else {
			if (mode == DB_BOOT_ZON)
				rec_count++;
			else if (mode == DB_BOOT_SOCIAL)
				rec_count += count_social_records(db_file, &top_of_socialm, &top_of_socialk);
			else if (mode == DB_BOOT_HLP)
				rec_count += count_alias_records(db_file);
			else if (mode == DB_BOOT_WLD) {
				counter = count_hash_records(db_file);
				if (counter >= 99) {
					log("SYSERR: File '%s' list more than 98 room", buf2);
					exit(1);
				}
				rec_count += (counter + 1);
			} else
				rec_count += count_hash_records(db_file);
		}
		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}

	/* Exit if 0 records, unless this is shops */
	if (!rec_count) {
		if (mode == DB_BOOT_SHP)
			return;
		log("SYSERR: boot error - 0 records counted in %s/%s.", prefix, index_filename);
		exit(1);
	}

	/* Any idea why you put this here Jeremy? */
	rec_count++;

	/*
	 * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	 */
	switch (mode) {
	case DB_BOOT_TRG:
		CREATE(trig_index, INDEX_DATA *, rec_count);
		break;
	case DB_BOOT_WLD:
		// Creating empty world with NOWHERE room.
		world.push_back(new(ROOM_DATA));
		memset(world[0], 0, sizeof(ROOM_DATA));
		top_of_world = FIRST_ROOM;
		size[0] = sizeof(ROOM_DATA) * rec_count;
		log("   %d rooms, %d bytes.", rec_count, size[0]);
		break;
	case DB_BOOT_MOB:
		CREATE(mob_proto, CHAR_DATA, rec_count);
		CREATE(mob_index, INDEX_DATA, rec_count);
		size[0] = sizeof(INDEX_DATA) * rec_count;
		size[1] = sizeof(CHAR_DATA) * rec_count;
		log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
		break;
	case DB_BOOT_OBJ:
		obj_proto.reserve(rec_count);
		CREATE(obj_index, INDEX_DATA, rec_count);
		size[0] = sizeof(INDEX_DATA) * rec_count;
		size[1] = sizeof(OBJ_DATA) * rec_count;
		log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
		break;
	case DB_BOOT_ZON:
		CREATE(zone_table, struct zone_data, rec_count);
		size[0] = sizeof(struct zone_data) * rec_count;
		log("   %d zones, %d bytes.", rec_count, size[0]);
		break;
	case DB_BOOT_HLP:
		CREATE(help_table, struct help_index_element, rec_count);
		size[0] = sizeof(struct help_index_element) * rec_count;
		log("   %d entries, %d bytes.", rec_count, size[0]);
		break;
	case DB_BOOT_SOCIAL:
		CREATE(soc_mess_list, struct social_messg, top_of_socialm + 1);
		CREATE(soc_keys_list, struct social_keyword, top_of_socialk + 1);
		size[0] = sizeof(struct social_messg) * (top_of_socialm + 1);
		size[1] = sizeof(struct social_keyword) * (top_of_socialk + 1);
		log("   %d entries(%d keywords), %d(%d) bytes.", top_of_socialm + 1,
		    top_of_socialk + 1, size[0], size[1]);
	}

	rewind(index);
	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$') {
		sprintf(buf2, "%s%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r"))) {
			log("SYSERR: %s: %s", buf2, strerror(errno));
			exit(1);
		}
		switch (mode) {
		case DB_BOOT_TRG:
		case DB_BOOT_WLD:
		case DB_BOOT_OBJ:
		case DB_BOOT_MOB:
			discrete_load(db_file, mode, buf2);
			break;
		case DB_BOOT_ZON:
			load_zones(db_file, buf2);
			break;
		case DB_BOOT_HLP:
			/*
			 * If you think about it, we have a race here.  Although, this is the
			 * "point-the-gun-at-your-own-foot" type of race.
			 */
			load_help(db_file);
			break;
		case DB_BOOT_SHP:
			boot_the_shops(db_file, buf2, rec_count);
			break;
		case DB_BOOT_SOCIAL:
			load_socials(db_file);
			break;
		}
		if (mode == DB_BOOT_WLD)
			parse_room(db_file, 0, TRUE);
		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}
	fclose(index);
	// Create virtual room for zone

	// sort the help index
	if (mode == DB_BOOT_HLP) {
		qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
		top_of_helpt--;
	}
	// sort the social index
	if (mode == DB_BOOT_SOCIAL) {
		qsort(soc_keys_list, top_of_socialk + 1, sizeof(struct social_keyword), csort);
	}
}


void discrete_load(FILE * fl, int mode, char *filename)
{
	int nr = -1, last;
	char line[256];

	const char *modes[] = { "world", "mob", "obj", "ZON", "SHP", "HLP", "trg" };
	/* modes positions correspond to DB_BOOT_xxx in db.h */


	for (;;) {		/*
				 * we have to do special processing with the obj files because they have
				 * no end-of-record marker :(
				 */
		if (mode != DB_BOOT_OBJ || nr < 0)
			if (!get_line(fl, line)) {
				if (nr == -1) {
					log("SYSERR: %s file %s is empty!", modes[mode], filename);
				} else {
					log("SYSERR: Format error in %s after %s #%d\n"
					    "...expecting a new %s, but file ended!\n"
					    "(maybe the file is not terminated with '$'?)", filename,
					    modes[mode], nr, modes[mode]);
				}
				exit(1);
			}

		if (*line == '$')
			return;
		/* This file create ADAMANT MUD ETITOR ? */
		if (strcmp(line, "#ADAMANT") == 0)
			continue;

		if (*line == '#') {
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1) {
				log("SYSERR: Format error after %s #%d", modes[mode], last);
				exit(1);
			}
			if (nr >= MAX_PROTO_NUMBER)
				return;
			else
				switch (mode) {
				case DB_BOOT_TRG:
					parse_trigger(fl, nr);
					break;
				case DB_BOOT_WLD:
					parse_room(fl, nr, FALSE);
					break;
				case DB_BOOT_MOB:
					parse_mobile(fl, nr);
					break;
				case DB_BOOT_OBJ:
					strcpy(line, parse_object(fl, nr));
					break;
				}
		} else {
			log("SYSERR: Format error in %s file %s near %s #%d", modes[mode], filename, modes[mode], nr);
			log("SYSERR: ... offending line: '%s'", line);
			exit(1);
		}
	}
}

void asciiflag_conv(char *flag, void *value)
{
	int *flags = (int *) value;
	int is_number = 1, block = 0, i;
	register char *p;

	for (p = flag; *p; p += i + 1) {
		i = 0;
		if (islower(*p)) {
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				block = (int) *(p + 1) - '0';
				i = 1;
			} else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (*p - 'a')));
		} else if (isupper(*p)) {
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				block = (int) *(p + 1) - '0';
				i = 1;
			} else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}
		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number) {
		is_number = atol(flag);
		block = is_number < INT_ONE ? 0 : is_number < INT_TWO ? 1 : is_number < INT_THREE ? 2 : 3;
		*(flags + block) = is_number & 0x3FFFFFFF;
	}
}

char fread_letter(FILE * fp)
{
	char c;
	do {
		c = getc(fp);
	}
	while (isspace(c));
	return c;
}

/* load the rooms */
void parse_room(FILE * fl, int virtual_nr, int virt)
{
	static int room_nr = FIRST_ROOM, zone = 0;
	int t[10], i;
	char line[256], flags[128];
	EXTRA_DESCR_DATA *new_descr;
	char letter;

	if (virt) {
		virtual_nr = zone_table[zone].top;
	}

	sprintf(buf2, "room #%d%s", virtual_nr, virt ? "(virt)" : "");

	if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
		log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
		exit(1);
	}
	while (virtual_nr > zone_table[zone].top)
		if (++zone > top_of_zone_table) {
			log("SYSERR: Room %d is outside of any zone.", virtual_nr);
			exit(1);
		}
	// Создаем новую комнату и вычищаем ее содержимое
	world.push_back(new(ROOM_DATA));
	memset(world[room_nr], 0, sizeof(ROOM_DATA));

	world[room_nr]->zone = zone;
	world[room_nr]->number = virtual_nr;
	if (virt) {
		world[room_nr]->name = str_dup("Виртуальная комната");
		world[room_nr]->description_num = RoomDescription::add_desc("Похоже, здесь Вам делать нечего.");
		world[room_nr]->room_flags.flags[0] = 0;
		world[room_nr]->room_flags.flags[1] = 0;
		world[room_nr]->room_flags.flags[2] = 0;
		world[room_nr]->room_flags.flags[3] = 0;
		world[room_nr]->sector_type = SECT_SECRET;
	} else {
		// не ставьте тут конструкцию ? : , т.к. gcc >=4.х вызывает в ней fread_string два раза
		world[room_nr]->name = fread_string(fl, buf2);
		if(!world[room_nr]->name)
			world[room_nr]->name = str_dup("");

		// тож временная галиматья
		char * temp_buf = fread_string(fl, buf2);
		if (!temp_buf)
			temp_buf = str_dup("");
		world[room_nr]->description_num = RoomDescription::add_desc(temp_buf);
		free(temp_buf);

		if (!get_line(fl, line)) {
			log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!", virtual_nr);
			exit(1);
		}

		if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
			log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
			exit(1);
		}
		/* t[0] is the zone number; ignored with the zone-file system */
		asciiflag_conv(flags, &world[room_nr]->room_flags);
		world[room_nr]->sector_type = t[2];
	}

	// Обнуляем флаги от аффектов и сами аффекты на комнате.
	world[room_nr]->affected = NULL;
	world[room_nr]->affected_by.flags[0] = 0;
	world[room_nr]->affected_by.flags[1] = 0;
	world[room_nr]->affected_by.flags[2] = 0;
	world[room_nr]->affected_by.flags[3] = 0;
	// Обнуляем базовые параметры (пока нет их загрузки)
	memset(&world[room_nr]->base_property,0,sizeof(room_property_data));

	// Обнуляем добавочные параметры комнаты
	memset(&world[room_nr]->add_property,0,sizeof(room_property_data));


	world[room_nr]->func = NULL;
	world[room_nr]->contents = NULL;
	world[room_nr]->people = NULL;
	world[room_nr]->track = NULL;
	world[room_nr]->light = 0;	// Zero light sources
	world[room_nr]->fires = 0;
	world[room_nr]->gdark = 0;
	world[room_nr]->glight = 0;
	world[room_nr]->forbidden = 0;
	world[room_nr]->ing_list = NULL;	// ингредиентов нет
	world[room_nr]->proto_script = NULL;

	for (i = 0; i < NUM_OF_DIRS; i++)
		world[room_nr]->dir_option[i] = NULL;

	world[room_nr]->ex_description = NULL;
	if (virt) {
		top_of_world = room_nr++;
		return;
	}

	sprintf(buf, "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

	for (;;) {
		if (!get_line(fl, line)) {
			log(buf);
			exit(1);
		}
		switch (*line) {
		case 'D':
			setup_dir(fl, room_nr, atoi(line + 1));
			break;
		case 'E':
			CREATE(new_descr, EXTRA_DESCR_DATA, 1);
			new_descr->keyword = NULL;
			new_descr->description = NULL;
			new_descr->keyword = fread_string(fl, buf2);
			new_descr->description = fread_string(fl, buf2);
			if (new_descr->keyword && new_descr->description) {
				new_descr->next = world[room_nr]->ex_description;
				world[room_nr]->ex_description = new_descr;
			} else {
				sprintf(buf, "SYSERR: Format error in room #%d (Corrupt extradesc)", virtual_nr);
				log(buf);
				free(new_descr);
			}
			break;
		case 'S':	/* end of room */
			/* DG triggers -- script is defined after the end of the room 'T' */
			/* Ингредиентная магия -- 'I' */
			do {
				letter = fread_letter(fl);
				ungetc(letter, fl);
				switch (letter) {
				case 'I':
					get_line(fl, line);
					im_parse(&(world[room_nr]->ing_list), line + 1);
					break;
				case 'T':
					dg_read_trigger(fl, world[room_nr], WLD_TRIGGER);
					break;
				default:
					letter = 0;
					break;
				}
			}
			while (letter != 0);
			top_of_world = room_nr++;
			return;
		default:
			log(buf);
			exit(1);
		}
	}
}



/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
	int t[5];
	char line[256];

	sprintf(buf2, "room #%d, direction D%d", GET_ROOM_VNUM(room), dir);

	CREATE(world[room]->dir_option[dir], EXIT_DATA, 1);
	world[room]->dir_option[dir]->general_description = fread_string(fl, buf2);

	// парс строки алиаса двери на имя;вининельный падеж, если он есть
	char *alias = fread_string(fl, buf2);
	if (alias && *alias) {
		std::string buffer(alias);
		std::string::size_type i = buffer.find('|');
		if (i != std::string::npos) {
			world[room]->dir_option[dir]->keyword = str_dup(buffer.substr(0,i).c_str());
			world[room]->dir_option[dir]->vkeyword = str_dup(buffer.substr(++i).c_str());
		} else {
			world[room]->dir_option[dir]->keyword = str_dup(buffer.c_str());
			world[room]->dir_option[dir]->vkeyword = str_dup(buffer.c_str());
		}
	}
	free(alias);

	if (!get_line(fl, line)) {
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}
	if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}

	if (t[0] & 1)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR;
	else if (t[0] & 2)
		world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
	else
		world[room]->dir_option[dir]->exit_info = 0;
	if (t[0] & 4)
		world[room]->dir_option[dir]->exit_info |= EX_HIDDEN;

	world[room]->dir_option[dir]->key = t[1];
	world[room]->dir_option[dir]->to_room = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
	if ((r_mortal_start_room = real_room(mortal_start_room)) == NOWHERE) {
		log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d", mortal_start_room);
		exit(1);
	}
	if ((r_immort_start_room = real_room(immort_start_room)) == NOWHERE) {
		if (!mini_mud)
			log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}
	if ((r_frozen_start_room = real_room(frozen_start_room)) == NOWHERE) {
		if (!mini_mud)
			log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}
	if ((r_helled_start_room = real_room(helled_start_room)) == NOWHERE) {
		if (!mini_mud)
			log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
		r_helled_start_room = r_mortal_start_room;
	}
	if ((r_named_start_room = real_room(named_start_room)) == NOWHERE) {
		if (!mini_mud)
			log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
		r_named_start_room = r_mortal_start_room;
	}
	if ((r_unreg_start_room = real_room(unreg_start_room)) == NOWHERE) {
		if (!mini_mud)
			log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
		r_unreg_start_room = r_mortal_start_room;
	}
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
	register int room, door;

	for (room = FIRST_ROOM; room <= top_of_world; room++)
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (world[room]->dir_option[door])
				if (world[room]->dir_option[door]->to_room != NOWHERE)
					world[room]->dir_option[door]->to_room =
					    real_room(world[room]->dir_option[door]->to_room);
}

// Установка принадлежности к зоне в прототипах
void renum_obj_zone(void)
{
	int i;
	for (i = 0; i <= top_of_objt; ++i) {
		obj_index[i].zone = real_zone(obj_index[i].vnum);
	}
}

// Установкапринадлежности к зоне в индексе
void renum_mob_zone(void)
{
	int i;
	for (i = 0; i <= top_of_mobt; ++i) {
		mob_index[i].zone = real_zone(mob_index[i].vnum);
	}
}



#define ZCMD zone_table[zone].cmd[cmd_no]
#define ZCMD_CMD(cmd_nom) zone_table[zone].cmd[cmd_nom]

void renum_single_table(int zone)
{
	int cmd_no, a, b, c, olda, oldb, oldc;
	char buf[128];

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
		a = b = c = 0;
		olda = ZCMD.arg1;
		oldb = ZCMD.arg2;
		oldc = ZCMD.arg3;
		switch (ZCMD.command) {
		case 'M':
			a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
			c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'F':
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			b = ZCMD.arg2 = real_mobile(ZCMD.arg2);
			c = ZCMD.arg3 = real_mobile(ZCMD.arg3);
			break;
		case 'O':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			if (ZCMD.arg3 != NOWHERE)
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'G':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			break;
		case 'E':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			break;
		case 'P':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			c = ZCMD.arg3 = real_object(ZCMD.arg3);
			break;
		case 'D':
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			break;
		case 'R':	/* rem obj from room */
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			b = ZCMD.arg2 = real_object(ZCMD.arg2);
			break;
		case 'Q':
			a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
			break;
		case 'T':	/* a trigger */
			/* designer's choice: convert this later */
			/* b = ZCMD.arg2 = real_trigger(ZCMD.arg2); */
			b = real_trigger(ZCMD.arg2);	/* leave this in for validation */
			if (ZCMD.arg1 == WLD_TRIGGER)
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'V':	/* trigger variable assignment */
			if (ZCMD.arg1 == WLD_TRIGGER)
				b = ZCMD.arg2 = real_room(ZCMD.arg2);
			break;
		}
		if (a < 0 || b < 0 || c < 0) {
			if (!mini_mud) {
				sprintf(buf, "Invalid vnum %d, cmd disabled", (a < 0) ? olda : ((b < 0) ? oldb : oldc));
				log_zone_error(zone, cmd_no, buf);
			}
			ZCMD.command = '*';
		}
	}
}

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
	zone_rnum zone;
	for (zone = 0; zone <= top_of_zone_table; zone++)
		renum_single_table(zone);
}


void parse_simple_mob(FILE * mob_f, int i, int nr)
{
	int j, t[10];
	char line[256];

	mob_proto[i].real_abils.str = 11;
	mob_proto[i].real_abils.intel = 11;
	mob_proto[i].real_abils.wis = 11;
	mob_proto[i].real_abils.dex = 11;
	mob_proto[i].real_abils.con = 11;
	mob_proto[i].real_abils.cha = 11;

	if (!get_line(mob_f, line)) {
		log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
		exit(1);
	}
	if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
		   t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
		log("SYSERR: Format error in mob #%d, 1th line\n" "...expecting line of form '# # # #d#+# #d#+#'", nr);
		exit(1);
	}

	GET_LEVEL(mob_proto + i) = t[0];
	mob_proto[i].real_abils.hitroll = 20 - t[1];
	mob_proto[i].real_abils.armor = 10 * t[2];

	/* max hit = 0 is a flag that H, M, V is xdy+z */
	mob_proto[i].points.max_hit = 0;
	GET_MEM_TOTAL(mob_proto + i) = t[3];
	GET_MEM_COMPLETED(mob_proto + i) = t[4];
	mob_proto[i].points.hit = t[5];

	mob_proto[i].points.move = 100;
	mob_proto[i].points.max_move = 100;

	mob_proto[i].mob_specials.damnodice = t[6];
	mob_proto[i].mob_specials.damsizedice = t[7];
	mob_proto[i].real_abils.damroll = t[8];

	if (!get_line(mob_f, line)) {
		log("SYSERR: Format error in mob #%d, 2th line\n"
		    "...expecting line of form '#d#+# #', but file ended!", nr);
		exit(1);
	}
	if (sscanf(line, " %dd%d+%d %d", t, t + 1, t + 2, t + 3) != 4) {
		log("SYSERR: Format error in mob #%d, 2th line\n" "...expecting line of form '#d#+# #'", nr);
		exit(1);
	}

	GET_GOLD(mob_proto + i) = t[2];
	GET_GOLD_NoDs(mob_proto + i) = t[0];
	GET_GOLD_SiDs(mob_proto + i) = t[1];
	GET_EXP(mob_proto + i) = t[3];

	if (!get_line(mob_f, line)) {
		log("SYSERR: Format error in 3th line of mob #%d\n"
		    "...expecting line of form '# # #', but file ended!", nr);
		exit(1);
	}

	switch (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) {
	case 3:
		mob_proto[i].mob_specials.speed = -1;
		break;
	case 4:
		mob_proto[i].mob_specials.speed = t[3];
		break;
	default:
		log("SYSERR: Format error in 3th line of mob #%d\n" "...expecting line of form '# # # #'", nr);
		exit(1);
	}

	mob_proto[i].char_specials.position = t[0];
	mob_proto[i].mob_specials.default_pos = t[1];
	mob_proto[i].player.sex = t[2];

	mob_proto[i].player.chclass = 0;
	mob_proto[i].player.weight = 200;
	mob_proto[i].player.height = 198;

	/*
	 * these are now save applies; base save numbers for MOBs are now from
	 * the warrior save table.
	 */
	for (j = 0; j < SAVING_COUNT; j++)
		GET_SAVE(mob_proto + i, j) = 0;
}



/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
	struct helper_data_type *helper;
	int k, num_arg, matched = 0, t[7];

	num_arg = atoi(value);

//Added by Adept
	CASE("Resistances") {
		if (sscanf(value, "%d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6) != 7) {
			log("SYSERROR : Excepted format <# # # # # # #> for RESISTANCES in MOB #%d", i);
			return;
		}
		for (k = 0; k < MAX_NUMBER_RESISTANCE; k++)
				GET_RESIST(mob_proto + i, k) = MIN(300, MAX(-1000, t[k]));
	}

	CASE("Saves") {
		if (sscanf(value, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
			log("SYSERROR : Excepted format <# # # #> for SAVES in MOB #%d", i);
			return;
		}
		for (k = 0; k < SAVING_COUNT; k++)
			GET_SAVE(mob_proto + i, k) = MIN(200, MAX(-200, t[k]));
	}

	CASE("HPReg") {
		RANGE(-200, 200);
		mob_proto[i].add_abils.hitreg = num_arg;
	}

	CASE("Armour") {
		RANGE(0, 100);
		mob_proto[i].add_abils.armour = num_arg;
	}

	CASE("PlusMem") {
		RANGE(-200, 200);
		mob_proto[i].add_abils.manareg = num_arg;
	}

	CASE("CastSuccess") {
		RANGE(-200, 200);
		mob_proto[i].add_abils.cast_success = num_arg;
	}

	CASE("Success") {
		RANGE(-200, 200);
		mob_proto[i].add_abils.morale_add = num_arg;
	}

	CASE("Initiative") {
		RANGE(-200, 200);
		mob_proto[i].add_abils.initiative_add = num_arg;
	}

	CASE("Absorbe") {
		RANGE(-200, 200);
		mob_proto[i].add_abils.absorb = num_arg;
	}
	CASE("AResist") {
		RANGE(0, 100);
		mob_proto[i].add_abils.aresist = num_arg;
	}
	CASE("MResist") {
		RANGE(0, 100);
		mob_proto[i].add_abils.mresist = num_arg;
	}
//End of changed
	CASE("BareHandAttack") {
		RANGE(0, 99);
		mob_proto[i].mob_specials.attack_type = num_arg;
	}

	CASE("Destination") {
		if (mob_proto[i].mob_specials.dest_count < MAX_DEST) {
			mob_proto[i].mob_specials.dest[mob_proto[i].mob_specials.dest_count] = num_arg;
			mob_proto[i].mob_specials.dest_count++;
		}
	}

	CASE("Str") {
		RANGE(3, 50);
		mob_proto[i].real_abils.str = num_arg;
	}

	CASE("StrAdd") {
		RANGE(0, 100);
		mob_proto[i].add_abils.str_add = num_arg;
	}

	CASE("Int") {
		RANGE(3, 50);
		mob_proto[i].real_abils.intel = num_arg;
	}

	CASE("Wis") {
		RANGE(3, 50);
		mob_proto[i].real_abils.wis = num_arg;
	}

	CASE("Dex") {
		RANGE(3, 50);
		mob_proto[i].real_abils.dex = num_arg;
	}

	CASE("Con") {
		RANGE(3, 50);
		mob_proto[i].real_abils.con = num_arg;
	}

	CASE("Cha") {
		RANGE(3, 50);
		mob_proto[i].real_abils.cha = num_arg;
	}

	CASE("Size") {
		RANGE(0, 100);
		mob_proto[i].real_abils.size = num_arg;
	}
  /**** Extended for Adamant */
	CASE("LikeWork") {
		RANGE(0, 200);
		mob_proto[i].mob_specials.LikeWork = num_arg;
	}

	CASE("MaxFactor") {
		RANGE(0, 255);
		mob_proto[i].mob_specials.MaxFactor = num_arg;
	}

	CASE("ExtraAttack") {
		RANGE(0, 255);
		mob_proto[i].mob_specials.ExtraAttack = num_arg;
	}

	CASE("Class") {
		RANGE(100, 116);
		mob_proto[i].player.chclass = num_arg;
	}


	CASE("Height") {
		RANGE(0, 200);
		mob_proto[i].player.height = num_arg;
	}

	CASE("Weight") {
		RANGE(0, 200);
		mob_proto[i].player.weight = num_arg;
	}

	CASE("Race") {
		RANGE(0, 20);
		mob_proto[i].player.Race = num_arg;
	}

	CASE("Special_Bitvector") {
		asciiflag_conv((char *) value, &mob_proto[i].mob_specials.npc_flags);
  /**** Empty now */
	}

/* Gorrah */
	CASE("Feat") {
		if (sscanf(value, "%d", t) != 1) {
			log("SYSERROR : Excepted format <#> for FEAT in MOB #%d", i);
			return;
		}
		if (t[0] >= MAX_FEATS || t[0] <= 0) {
			log("SYSERROR : Unknown feat No %d for MOB #%d", t[0], i);
			return;
		}
		SET_FEAT(mob_proto + i, t[0]);
	}
/* End of changes */

	CASE("Skill") {
		if (sscanf(value, "%d %d", t, t + 1) != 2) {
			log("SYSERROR : Excepted format <# #> for SKILL in MOB #%d", i);
			return;
		}
		if (t[0] > MAX_SKILLS || t[0] < 1) {
			log("SYSERROR : Unknown skill No %d for MOB #%d", t[0], i);
			return;
		}
		t[1] = MIN(200, MAX(0, t[1]));
		SET_SKILL(mob_proto + i, t[0], t[1]);
	}

	CASE("Spell") {
		if (sscanf(value, "%d", t + 0) != 1) {
			log("SYSERROR : Excepted format <#> for SPELL in MOB #%d", i);
			return;
		}
		if (t[0] > MAX_SPELLS || t[0] < 1) {
			log("SYSERROR : Unknown spell No %d for MOB #%d", t[0], i);
			return;
		}
		GET_SPELL_MEM(mob_proto + i, t[0]) += 1;
		GET_CASTER(mob_proto + i) += (IS_SET(spell_info[t[0]].routines, NPC_CALCULATE) ? 1 : 0);
		log("Set spell %d to %d(%s)", t[0], GET_SPELL_MEM(mob_proto + i, t[0]), GET_NAME(mob_proto + i));
	}

	CASE("Helper") {
		CREATE(helper, struct helper_data_type, 1);
		helper->mob_vnum = num_arg;
		helper->next_helper = GET_HELPER(mob_proto + i);
		GET_HELPER(mob_proto + i) = helper;
	}

	if (!matched) {
		log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d", keyword, nr);
	}
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
	char *ptr;

	if ((ptr = strchr(buf, ':')) != NULL) {
		*(ptr++) = '\0';
		while (a_isspace(*ptr))
			ptr++;
#if 0				/* Need to evaluate interpret_espec()'s NULL handling. */
	}
#else
	} else
		ptr = "";
#endif
	interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE * mob_f, int i, int nr)
{
	char line[256];

	parse_simple_mob(mob_f, i, nr);

	while (get_line(mob_f, line)) {
		if (!strcmp(line, "E"))	/* end of the enhanced section */
			return;
		else if (*line == '#') {	/* we've hit the next mob, maybe? */
			log("SYSERR: Unterminated E section in mob #%d", nr);
			exit(1);
		} else
			parse_espec(line, i, nr);
	}

	log("SYSERR: Unexpected end of file reached after mob #%d", nr);
	exit(1);
}

/* Make own name by process aliases*/
int trans_obj_name(OBJ_DATA * obj, CHAR_DATA * ch)
{
	// ищем метку @p , @p1 ... и заменяем на падежи.
	string obj_pad;
	char *ptr;
	int i, j, k;
	for (i = 0; i < NUM_PADS; i++) {
		obj_pad = string(GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i));
		j = obj_pad.find("@p");
		if (j > 0) {
			// Родитель найден прописываем его.
			ptr = GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i);
			if (GET_OBJ_PNAME(obj, i) != ptr)
				free(GET_OBJ_PNAME(obj, i));

			k = atoi(obj_pad.substr(j + 2, j + 3).c_str());
			obj_pad.replace(j, 3, GET_PAD(ch, k));

			GET_OBJ_PNAME(obj, i) = str_dup(obj_pad.c_str());
			// Если имя в именительном то дублируем запись
			if (i == 0) {
				obj->short_description = str_dup(obj_pad.c_str());
				ptr = obj_proto[GET_OBJ_RNUM(obj)]->short_description;
				if (obj->short_description != ptr)
					free(obj->short_description);
				obj->short_description = str_dup(obj_pad.c_str());
			}
		}
	};
	return (TRUE);
}

void dl_list_copy(load_list * *pdst, load_list * src)
{
	load_list::iterator p;
	if (src == NULL) {
		*pdst = NULL;
		return;
	} else {
		*pdst = new load_list;
		p = src->begin();
		while (p != src->end()) {
			(*pdst)->push_back(*p);
			p++;
		}
	}
}


/* Dead load list object load */
int dl_load_obj(OBJ_DATA * corpse, CHAR_DATA * ch, CHAR_DATA * chr, int DL_LOAD_TYPE)
{
	bool last_load = true;
	bool load = false;
	bool miw;
	load_list::iterator p;
	OBJ_DATA *tobj;

	if (mob_proto[GET_MOB_RNUM(ch)].dl_list == NULL)
		return FALSE;

	p = mob_proto[GET_MOB_RNUM(ch)].dl_list->begin();

	while (p != mob_proto[GET_MOB_RNUM(ch)].dl_list->end()) {
		switch ((*p)->load_type) {
		case DL_LOAD_ANYWAY:
			last_load = load;
		case DL_LOAD_ANYWAY_NC:
			break;
		case DL_LOAD_IFLAST:
			last_load = load && last_load;
		case DL_LOAD_IFLAST_NC:
			load = load && last_load;
			break;
		}
		// Блокируем лоад в зависимости от значения смецпараметра
		if ((*p)->spec_param != DL_LOAD_TYPE)
			load = false;
		else
			load = true;
		if (load) {
			tobj = read_object((*p)->obj_vnum, VIRTUAL);
			if (tobj == NULL) {
				sprintf(buf,
					"Попытка загрузки в труп (VNUM:%d) несуществующего объекта (VNUM:%d).",
					GET_MOB_VNUM(ch), (*p)->obj_vnum);
				mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			} else {
				// Проверяем мах_ин_ворлд и вероятность загрузки, если это необходимо для такого DL_LOAD_TYPE
				if (GET_OBJ_MIW(tobj) >=
				    obj_index[GET_OBJ_RNUM(tobj)].stored + obj_index[GET_OBJ_RNUM(tobj)].number)
					miw = true;
				else
					miw = false;
				switch (DL_LOAD_TYPE) {
				case DL_ORDINARY:	/*Обычная загрузка - без выкрутасов */
					if (miw && (number(1, 100) <= (*p)->load_prob))
						load = true;
					else
						load = false;
					break;
				case DL_PROGRESSION:	/*Загрузка с убывающей до 0.01 вероятностью */
					if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
						load = true;
					else
						load = false;
					break;
				case DL_SKIN:	/*Загрузка по применению "освежевать" */
					if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
						load = true;
					else
						load = false;
					if (chr == NULL)
						load = false;
					break;
				}
				if (load) {
					GET_OBJ_PARENT(tobj) = GET_MOB_VNUM(ch);
					trans_obj_name(tobj, ch);
					// Добавлена проверка на отсутствие трупа
					if (MOB_FLAGGED(ch, MOB_CORPSE)) {
						act("На земле остал$U лежать $o.", FALSE, ch, tobj, 0, TO_ROOM);
						obj_to_room(tobj, IN_ROOM(ch));
					} else {
						if ((DL_LOAD_TYPE == DL_SKIN) && (corpse->carried_by == chr))
							can_carry_obj(chr, tobj);
						else
							obj_to_obj(tobj, corpse);
					}
				} else {
					extract_obj(tobj);
					load = false;
				}

			}
		}
		p++;
	}
	return (TRUE);
}

/* Dead load list object parse */
int dl_parse(load_list ** dl_list, char *line)
{
	// Формат парсинга D {номер прототипа} {вероятность загрузки} {спец поле - тип загрузки}
	int vnum, prob, type, spec;
	struct load_data *new_item;

	if (sscanf(line, "%d %d %d %d", &vnum, &prob, &type, &spec) != 4) {
		// Ошибка чтения.
		log("SYSERR: Parse death load list (bad param count).");
		return FALSE;
	};
	// проверяем существование прототипа в мире (предметы уже должны быть загружены)
	if (*dl_list == NULL) {
		// Создаем новый список.
		*dl_list = new load_list;
	}

	CREATE(new_item, struct load_data, 1);
	new_item->obj_vnum = vnum;
	new_item->load_prob = prob;
	new_item->load_type = type;
	new_item->spec_param = spec;

	(*dl_list)->push_back(new_item);
	return TRUE;
}

void parse_mobile(FILE * mob_f, int nr)
{
	static int i = 0;
	int j, t[10];
	char line[256], *tmpptr, letter;
	char f1[128], f2[128];

	mob_index[i].vnum = nr;
	mob_index[i].number = 0;
	mob_index[i].func = NULL;

	clear_char(mob_proto + i);
	/*
	 * Mobiles should NEVER use anything in the 'player_specials' structure.
	 * The only reason we have every mob in the game share this copy of the
	 * structure is to save newbie coders from themselves. -gg 2/25/98
	 */
	mob_proto[i].player_specials = &dummy_mob;
	sprintf(buf2, "mob vnum %d", nr);
  /***** String data *****/
	mob_proto[i].player.name = fread_string(mob_f, buf2);	/* aliases */
	tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
	/* real name */
	CREATE(GET_PAD(mob_proto + i, 0), char, strlen(mob_proto[i].player.short_descr) + 1);
	strcpy(GET_PAD(mob_proto + i, 0), mob_proto[i].player.short_descr);
	for (j = 1; j < NUM_PADS; j++)
		GET_PAD(mob_proto + i, j) = fread_string(mob_f, buf2);
	if (tmpptr && *tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") || !str_cmp(fname(tmpptr), "the"))
			*tmpptr = LOWER(*tmpptr);

	mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
	mob_proto[i].player.description = fread_string(mob_f, buf2);
	mob_proto[i].mob_specials.Questor = NULL;
	mob_proto[i].player.title = NULL;

	// mob_proto[i].mob_specials.Questor = fread_string(mob_f, buf2);

	/* *** Numeric data *** */
	if (!get_line(mob_f, line)) {
		log("SYSERR: Format error after string section of mob #%d\n"
		    "...expecting line of form '# # # {S | E}', but file ended!\n%s", nr, line);
		exit(1);
	}
#ifdef CIRCLE_ACORN		/* Ugh. */
	if (sscanf(line, "%s %s %d %s", f1, f2, t + 2, &letter) != 4) {
#else
	if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4) {
#endif
		log("SYSERR: Format error after string section of mob #%d\n"
		    "...expecting line of form '# # # {S | E}'\n%s", nr, line);
		exit(1);
	}
	asciiflag_conv(f1, &MOB_FLAGS(mob_proto + i, 0));
	SET_BIT(MOB_FLAGS(mob_proto + i, MOB_ISNPC), MOB_ISNPC);
	asciiflag_conv(f2, &AFF_FLAGS(mob_proto + i, 0));
	GET_ALIGNMENT(mob_proto + i) = t[2];
	switch (UPPER(letter)) {
	case 'S':		/* Simple monsters */
		parse_simple_mob(mob_f, i, nr);
		break;
	case 'E':		/* Circle3 Enhanced monsters */
		parse_enhanced_mob(mob_f, i, nr);
		break;
		/* add new mob types here.. */
	default:
		log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
		exit(1);
	}

	/* DG triggers -- script info follows mob S/E section */
	/* DG triggers -- script is defined after the end of the room 'T' */
	/* Ингредиентная магия -- 'I' */
	/* Объекты загружаемые по-смертно -- 'D' */

	do {
		letter = fread_letter(mob_f);
		ungetc(letter, mob_f);
		switch (letter) {
		case 'I':
			get_line(mob_f, line);
			im_parse(&mob_proto[i].ing_list, line + 1);
			break;
		case 'L':
			get_line(mob_f, line);
			dl_parse(&mob_proto[i].dl_list, line + 1);
			break;
		case 'T':
			dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
			break;
		default:
			letter = 0;
			break;
		}
	}
	while (letter != 0);

	for (j = 0; j < NUM_WEARS; j++)
		mob_proto[i].equipment[j] = NULL;

	mob_proto[i].nr = i;
	mob_proto[i].desc = NULL;

  /**** See reading mob
  log("%s/%s/%s/%s/%s/%s",GET_PAD(mob_proto+i,0),GET_PAD(mob_proto+i,1),
                          GET_PAD(mob_proto+i,2),GET_PAD(mob_proto+i,3),
                          GET_PAD(mob_proto+i,4),GET_PAD(mob_proto+i,5));

  log("Str: %d, Dex: %d, Con: %d, Int: %d, Wis: %d, Cha: %d, Size: %d",
      GET_STR(mob_proto+i),GET_DEX(mob_proto+i),GET_CON(mob_proto+i),
      GET_INT(mob_proto+i),GET_WIS(mob_proto+i),GET_CHA(mob_proto+i),
      GET_SIZE(mob_proto+i));

  log("Weight: %d, Height: %d, Damage: %dD%d",
       GET_WEIGHT(mob_proto+i),
       GET_HEIGHT(mob_proto+i),
       mob_proto[i].mob_specials.damnodice,
       mob_proto[i].mob_specials.damsizedice);

  log("Level: %d, HR: %d, DR: %d, AC: %d",
      GET_LEVEL(mob_proto + i),
      mob_proto[i].real_abils.damroll,
      mob_proto[i].real_abils.hitroll,
      mob_proto[i].real_abils.armor);

  log("MaxHit: %d, Hit: %dD%d+%d, Mv: %d",
      mob_proto[i].points.max_hit,
      mob_proto[i].ManaMemNeeded,
      mob_proto[i].ManaMemStored,
      mob_proto[i].points.hit,
      mob_proto[i].points.move);
  for(j=1, count=0; j<MAX_SPELLS; j++)
     {if (GET_SPELL_MEM(mob_proto+i,j))
         count += sprintf(buf+count," %d ",j);
     }
  if (count) log("SPELLS : %s", buf);
  for(j=1, count=0; j<MAX_SKILLS; j++)
     {if (GET_SKILL(mob_proto+i,j))
         count += sprintf(buf+count," %d ",j);
     }
  if (count) log("SKILLS : %s", buf);
  */

	top_of_mobt = i++;
}

// #define SEVEN_DAYS 60*24*30

/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
	static int i = 0;
	static char line[256];
	int t[10], j = 0, retval;
	char *tmpptr;
	char f0[256], f1[256], f2[256];
	EXTRA_DESCR_DATA *new_descr;
	OBJ_DATA *tobj;

	NEWCREATE(tobj, OBJ_DATA);

	obj_index[i].vnum = nr;
	obj_index[i].number = 0;
	obj_index[i].stored = 0;
	obj_index[i].func = NULL;

	tobj->item_number = i;

  /**** Add some initialization fields */
	tobj->obj_flags.Obj_max = 100;
	tobj->obj_flags.Obj_cur = 100;
	tobj->obj_flags.Obj_sex = 1;
	tobj->obj_flags.Obj_timer = SEVEN_DAYS;
	tobj->obj_flags.Obj_level = 1;
	tobj->obj_flags.Obj_destroyer = 60;

	sprintf(buf2, "object #%d", nr);

	/* *** string data *** */
	if ((tobj->name = fread_string(obj_f, buf2)) == NULL) {
		log("SYSERR: Null obj name or format error at or near %s", buf2);
		exit(1);
	}
	tmpptr = tobj->short_description = fread_string(obj_f, buf2);
	*tobj->short_description = LOWER(*tobj->short_description);
	CREATE(tobj->PNames[0], char, strlen(tobj->short_description) + 1);
	strcpy(tobj->PNames[0], tobj->short_description);

	for (j = 1; j < NUM_PADS; j++) {
		tobj->PNames[j] = fread_string(obj_f, buf2);
		*tobj->PNames[j] = LOWER(*tobj->PNames[j]);
	}

	if (tmpptr && *tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") || !str_cmp(fname(tmpptr), "the"))
			*tmpptr = LOWER(*tmpptr);

	tmpptr = tobj->description = fread_string(obj_f, buf2);
	if (tmpptr && *tmpptr)
		CAP(tmpptr);
	tobj->action_description = fread_string(obj_f, buf2);

	if (!get_line(obj_f, line)) {
		log("SYSERR: Expecting *1th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %s %d %d %d", f0, t + 1, t + 2, t + 3)) != 4) {
		log("SYSERR: Format error in *1th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	asciiflag_conv(f0, &tobj->obj_flags.Obj_skill);
	tobj->obj_flags.Obj_max = t[1];
	tobj->obj_flags.Obj_cur = t[2];
	tobj->obj_flags.Obj_mater = t[3];

	if (!get_line(obj_f, line)) {
		log("SYSERR: Expecting *2th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
		log("SYSERR: Format error in *2th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->obj_flags.Obj_sex = t[0];
	tobj->obj_flags.Obj_timer = t[1] > 0 ? t[1] : SEVEN_DAYS;
	tobj->obj_flags.Obj_spell = t[2];
	tobj->obj_flags.Obj_level = t[3];

	if (!get_line(obj_f, line)) {
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %s %s %s", f0, f1, f2)) != 3) {
		log("SYSERR: Format error in *3th* numeric line (expecting 3 args, got %d), %s", retval, buf2);
		exit(1);
	}
	asciiflag_conv(f0, &tobj->obj_flags.affects);
							 /*** Affects */
	asciiflag_conv(f1, &tobj->obj_flags.anti_flag);
							 /*** Miss for ...    */
	asciiflag_conv(f2, &tobj->obj_flags.no_flag);
							 /*** Deny for ...    */

	if (!get_line(obj_f, line)) {
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %d %s %s", t, f1, f2)) != 3) {
		log("SYSERR: Format error in *3th* misc line (expecting 3 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->obj_flags.type_flag = t[0];	    /*** What's a object */
	asciiflag_conv(f1, &tobj->obj_flags.extra_flags);
							    /*** Its effects     */
	asciiflag_conv(f2, &tobj->obj_flags.wear_flags);
							   /*** Wear on ...     */

	if (!get_line(obj_f, line)) {
		log("SYSERR: Expecting *5th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, "%s %d %d %d", f0, t + 1, t + 2, t + 3)) != 4) {
		log("SYSERR: Format error in *5th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	asciiflag_conv(f0, &tobj->obj_flags.value);
						    /****/
	tobj->obj_flags.value[1] = t[1];
					  /****/
	tobj->obj_flags.value[2] = t[2];
					  /****/
	tobj->obj_flags.value[3] = t[3];
					  /****/

	if (!get_line(obj_f, line)) {
		log("SYSERR: Expecting *6th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
		log("SYSERR: Format error in *6th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->obj_flags.weight = t[0];
	tobj->obj_flags.cost = t[1];
	tobj->obj_flags.cost_per_day_off = t[2];
	tobj->obj_flags.cost_per_day_on = t[3];

	/* check to make sure that weight of containers exceeds curr. quantity */
	if (tobj->obj_flags.type_flag == ITEM_DRINKCON || tobj->obj_flags.type_flag == ITEM_FOUNTAIN) {
		if (tobj->obj_flags.weight < tobj->obj_flags.value[1])
			tobj->obj_flags.weight = tobj->obj_flags.value[1] + 5;
	}

	/* *** extra descriptions and affect fields *** */
	strcat(buf2, ", after numeric constants\n" "...expecting 'E', 'A', '$', or next object number");
	j = 0;

	for (;;) {
		if (!get_line(obj_f, line)) {
			log("SYSERR: Format error in %s", buf2);
			exit(1);
		}
		switch (*line) {
		case 'E':
			CREATE(new_descr, EXTRA_DESCR_DATA, 1);
			new_descr->keyword = NULL;
			new_descr->description = NULL;
			new_descr->keyword = fread_string(obj_f, buf2);
			new_descr->description = fread_string(obj_f, buf2);
			if (new_descr->keyword && new_descr->description) {
				new_descr->next = tobj->ex_description;
				tobj->ex_description = new_descr;
			} else {
				sprintf(buf, "SYSERR: Format error in %s (Corrupt extradesc)", buf2);
				log(buf);
				free(new_descr);
			}
			break;
		case 'A':
			if (j >= MAX_OBJ_AFFECT) {
				log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
				exit(1);
			}
			if (!get_line(obj_f, line)) {
				log("SYSERR: Format error in 'A' field, %s\n"
				    "...expecting 2 numeric constants but file ended!", buf2);
				exit(1);
			}
			if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
				log("SYSERR: Format error in 'A' field, %s\n"
				    "...expecting 2 numeric arguments, got %d\n"
				    "...offending line: '%s'", buf2, retval, line);
				exit(1);
			}
			tobj->affected[j].location = t[0];
			tobj->affected[j].modifier = t[1];
			j++;
			break;
		case 'T':	/* DG triggers */
			dg_obj_trigger(line, tobj);
			break;
		case 'M':
			GET_OBJ_MIW(tobj) = atoi(line + 1);
			break;

		case '$':
		case '#':
			check_object(tobj);
			obj_proto.push_back(tobj);
			top_of_objt = i++;
			return (line);
		default:
			log("SYSERR: Format error in %s", buf2);
			exit(1);
		}
	}
}


#define Z       zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
	static zone_rnum zone = 0;
	int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, a_number = 0, b_number = 0;
	char *ptr, buf[256], zname[256];
	char t1[80], t2[80];
//MZ.load
	Z.level = 1;
	Z.type = 0;
//-MZ.load
	Z.typeA_count = 0;
	Z.typeB_count = 0;
	Z.locked = FALSE;
	Z.reset_idle = FALSE;
	Z.used = FALSE;
	Z.activity = 0;
	strcpy(zname, zonename);

	while (get_line(fl, buf)) {
		ptr = buf;
		skip_spaces(&ptr);
		if (*ptr == 'A')
			Z.typeA_count++;
		if (*ptr == 'B')
			Z.typeB_count++;
		num_of_cmds++;	/* this should be correct within 3 or so */
	}
	rewind(fl);
	if (Z.typeA_count)
		CREATE(Z.typeA_list, int, Z.typeA_count);
	if (Z.typeB_count) {
		CREATE(Z.typeB_list, int, Z.typeB_count);
		CREATE(Z.typeB_flag, bool, Z.typeB_count);
		// сбрасываем все флаги
		for (b_number = Z.typeB_count; b_number > 0; b_number--)
			Z.typeB_flag[b_number - 1] = FALSE;
	}

	if (num_of_cmds == 0) {
		log("SYSERR: %s is empty!", zname);
		exit(1);
	} else
		CREATE(Z.cmd, struct reset_com, num_of_cmds);

	line_num += get_line(fl, buf);

	if (sscanf(buf, "#%d", &Z.number) != 1) {
		log("SYSERR: Format error in %s, line %d", zname, line_num);
		exit(1);
	}
	sprintf(buf2, "beginning of zone #%d", Z.number);

	line_num += get_line(fl, buf);
	if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
		*ptr = '\0';
	Z.name = str_dup(buf);

//MZ.load
	line_num += get_line(fl, buf);
	if (*buf == '#')
	{
		sscanf(buf, "#%d %d", &Z.level, &Z.type);
		line_num += get_line(fl, buf);
	}
//-MZ.load
	*t1 = 0;
	*t2 = 0;
	if (sscanf(buf, " %d %d %d %d %s %s", &Z.top, &Z.lifespan, &Z.reset_mode, (int *)&Z.reset_idle, t1, t2) < 4) {
		// если нет четырех констант, то, возможно, это старый формат -- попробуем прочитать три
		if (sscanf(buf, " %d %d %d %s %s", &Z.top, &Z.lifespan, &Z.reset_mode, t1, t2) < 3) {
			log("SYSERR: Format error in 3-constant line of %s", zname);
			exit(1);
		}
	}
	Z.under_construction = !str_cmp(t1, "test");
	Z.locked = !str_cmp(t2, "locked");
	cmd_no = 0;

	for (;;) {
		if ((tmp = get_line(fl, buf)) == 0) {
			log("SYSERR: Format error in %s - premature end of file", zname);
			exit(1);
		}
		line_num += tmp;
		ptr = buf;
		skip_spaces(&ptr);

		if ((ZCMD.command = *ptr) == '*')
			continue;
		ptr++;
		// Новые параметры формата файла:
		// A номер_зоны -- зона типа A из списка
		// B номер_зоны -- зона типа B из списка
		if (ZCMD.command == 'A') {
			sscanf(ptr, " %d", &Z.typeA_list[a_number]);
			a_number++;
			continue;
		}
		if (ZCMD.command == 'B') {
			sscanf(ptr, " %d", &Z.typeB_list[b_number]);
			b_number++;
			continue;
		}

		if (ZCMD.command == 'S' || ZCMD.command == '$') {
			ZCMD.command = 'S';
			break;
		}
		error = 0;
		ZCMD.arg4 = -1;
		if (strchr("MOEGPDTVQF", ZCMD.command) == NULL) {	/* a 3-arg command */
			if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
				error = 1;
		} else if (ZCMD.command == 'V') {	/* a string-arg command */
			if (sscanf(ptr, " %d %d %d %d %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, t1, t2) != 6)
				error = 1;
			else {
				ZCMD.sarg1 = str_dup(t1);
				ZCMD.sarg2 = str_dup(t2);
			}
		} else if (ZCMD.command == 'Q') {	/* a number command */
			if (sscanf(ptr, " %d %d", &tmp, &ZCMD.arg1) != 2)
				error = 1;
			else
				tmp = 0;
		} else {
			if (sscanf(ptr, " %d %d %d %d %d", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4) < 4)
				error = 1;
		}

		ZCMD.if_flag = tmp;

		if (error) {
			log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
			exit(1);
		}
		ZCMD.line = line_num;
		cmd_no++;
	}
	top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE * fl, char *buf)
{
	if (fgets(buf, READ_SIZE, fl) == NULL) {
		log("SYSERR: error reading help file: not terminated with $?");
		exit(1);
	}
	buf[strlen(buf) - 1] = '\0';	/* take off the trailing \n */
}


void load_help(FILE * fl)
{
#if defined(CIRCLE_MACINTOSH)
	static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];	/* ? */
#else
	char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
	char line[READ_SIZE + 1], *scan;
	struct help_index_element el;

	/* get the first keyword line */
	get_one_line(fl, key);
	while (*key != '$') {	/* read in the corresponding help entry */
		strcpy(entry, strcat(key, "\r\n"));
		get_one_line(fl, line);
		while (*line != '#') {
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(fl, line);
		}
		/* Assign read level */
		el.min_level = 0;
		if ((*line == '#') && (*(line + 1) != 0))
			el.min_level = atoi((line + 1));

		el.min_level = MAX(0, MIN(el.min_level, LVL_IMPL));

		/* now, add the entry to the index with each keyword on the keyword line */
		el.duplicate = 0;
		el.entry = str_dup(entry);
		scan = one_word(key, next_key);
		while (*next_key) {
			el.keyword = str_dup(next_key);
			help_table[top_of_helpt++] = el;
			el.duplicate++;
			scan = one_word(scan, next_key);
		}

		/* get next keyword line (or $) */
		get_one_line(fl, key);
	}
}


int hsort(const void *a, const void *b)
{
	const struct help_index_element *a1, *b1;

	a1 = (const struct help_index_element *) a;
	b1 = (const struct help_index_element *) b;

	return (str_cmp(a1->keyword, b1->keyword));
}


int csort(const void *a, const void *b)
{
	const struct social_keyword *a1, *b1;

	a1 = (const struct social_keyword *) a;
	b1 = (const struct social_keyword *) b;

	return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/



int vnum_mobile(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_mobt; nr++) {
		if (isname(searchname, mob_proto[nr].player.name)) {
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
				mob_index[nr].vnum, mob_proto[nr].player.short_descr);
			send_to_char(buf, ch);
		}
	}
	return (found);
}



int vnum_object(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_objt; nr++) {
		if (isname(searchname, obj_proto[nr]->name)) {
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, obj_proto[nr]->short_description);
			send_to_char(buf, ch);
		}
	}
	return (found);
}


int vnum_flag(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0, plane = 0, counter = 0, plane_offset = 0;
	bool f = FALSE;
	char *text;
// type:
// 0 -- неизвестный тип
// 1 -- объекты
// 2 -- мобы
// 4 -- комнаты
// Ищем для объектов в списках: extra_bits[], apply_types[], weapon_affects[]
//  Ищем для мобов в списках  action_bits[], function_bits[],  affected_bits[], preference_bits[]
// Ищем для комнат в списках room_bits[]

	text = (char *)calloc(1, 1);

	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*extra_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		};
		if (is_abbrev(searchname, extra_bits[counter])) {
			f = TRUE;
			break;
		}
		plane_offset++;
	}
	if (f)
		for (nr = 0; nr <= top_of_objt; nr++) {
			if (obj_proto[nr]->obj_flags.extra_flags.flags[plane] & (1 << (plane_offset))) {
				sprintf(buf, "%3d. [%5d] %s :   %s\r\n", ++found,
					obj_index[nr].vnum, obj_proto[nr]->short_description, extra_bits[counter]);
				text = (char *)realloc(text, strlen(text) + strlen(buf) + 1);
				text = strcat(text, buf);
//				send_to_char(buf, ch);
			}
		};
	f = FALSE;
// ---------------------
	for (counter = 0; *apply_types[counter] != '\n'; counter++) {
		if (is_abbrev(searchname, apply_types[counter])) {
			f = TRUE;
			break;
		}
	}
	if (f)
		for (nr = 0; nr <= top_of_objt; nr++) {
			for (plane = 0; plane < MAX_OBJ_AFFECT; plane++)
				if (obj_proto[nr]->affected[plane].location == counter) {
					sprintf(buf, "%3d. [%5d] %s :   %s\r\n", ++found,
						obj_index[nr].vnum, obj_proto[nr]->short_description,
						apply_types[counter]);
					text = (char *)realloc(text, strlen(text) + strlen(buf) + 1);
					text = strcat(text, buf);
//					send_to_char(buf, ch);
					continue;
				}
		};
	f = FALSE;
// ---------------------
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*weapon_affects[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		};
		if (is_abbrev(searchname, weapon_affects[counter])) {
			f = TRUE;
			break;
		}
		plane_offset++;
	}
	if (f)
		for (nr = 0; nr <= top_of_objt; nr++) {
			if (obj_proto[nr]->obj_flags.affects.flags[plane] & (1 << (plane_offset))) {
				sprintf(buf, "%3d. [%5d] %s :   %s\r\n", ++found,
					obj_index[nr].vnum, obj_proto[nr]->short_description, weapon_affects[counter]);
				text = (char *)realloc(text, strlen(text) + strlen(buf) + 1);
				text = strcat(text, buf);
//				send_to_char(buf, ch);
			}
		};

	if (strlen(text))
		page_string(ch->desc, text, TRUE);
	free(text);

	f = FALSE;

	return (found);
}


/* create a character, and add it to the char list */
CHAR_DATA *create_char(void)
{
	CHAR_DATA *ch;

	CREATE(ch, CHAR_DATA, 1);
	clear_char(ch);
	ch->next = character_list;
	character_list = ch;
	GET_ID(ch) = max_id++;

	return (ch);
}


/* create a new mobile from a prototype */
CHAR_DATA *read_mobile(mob_vnum nr, int type)
{				/* and mob_rnum */
	int is_corpse = 0;
	mob_rnum i;
	CHAR_DATA *mob;

	if (nr < 0) {
		is_corpse = 1;
		nr = -nr;
	}

	if (type == VIRTUAL) {
		if ((i = real_mobile(nr)) < 0) {
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (NULL);
		}
	} else
		i = nr;

	CREATE(mob, CHAR_DATA, 1);
	clear_char(mob);
	*mob = mob_proto[i];
	mob->proto_script = NULL;
	mob->next = character_list;
	character_list = mob;

	if (!mob->points.max_hit) {
		mob->points.max_hit = dice(GET_MEM_TOTAL(mob), GET_MEM_COMPLETED(mob)) + mob->points.hit;
	} else
		mob->points.max_hit = number(mob->points.hit, GET_MEM_TOTAL(mob));

	mob->points.hit = mob->points.max_hit;
	GET_MEM_TOTAL(mob) = GET_MEM_COMPLETED(mob) = 0;
	GET_HORSESTATE(mob) = 200;
	GET_LASTROOM(mob) = NOWHERE;
	GET_PFILEPOS(mob) = -1;
	if (mob->mob_specials.speed <= -1)
		GET_ACTIVITY(mob) = number(0, PULSE_MOBILE - 1);
	else
		GET_ACTIVITY(mob) = number(0, mob->mob_specials.speed);
	EXTRACT_TIMER(mob) = 0;
	mob->points.move = mob->points.max_move;
	GET_GOLD(mob) += dice(GET_GOLD_NoDs(mob), GET_GOLD_SiDs(mob));

	mob->player.time.birth = time(0);
	mob->player.time.played = 0;
	mob->player.time.logon = time(0);

	mob_index[i].number++;
	GET_ID(mob) = max_id++;
	if (!is_corpse)
		assign_triggers(mob, MOB_TRIGGER);

	i = mob_index[i].zone;
	if (i != -1 && zone_table[i].under_construction) {
		// mobile принадлежит тестовой зоне
		SET_BIT(MOB_FLAGS(mob, MOB_NOSUMMON), MOB_NOSUMMON);
	}

	return (mob);
}


/* create an object, and add it to the object list */
OBJ_DATA *create_obj(void)
{
	OBJ_DATA *obj;

	NEWCREATE(obj, OBJ_DATA);
	obj->next = object_list;
	object_list = obj;
	GET_ID(obj) = max_id++;

	return (obj);
}

// никакая это не копия, строковые и остальные поля с выделением памяти остаются общими
// мы просто отдаем константный указатель на прототип
const OBJ_DATA* read_object_mirror(obj_vnum nr)
{
	int i;
	if ((i = real_object(nr)) < 0) {
		log("Object (V) %d does not exist in database.", nr);
		return (NULL);
	};

	return obj_proto[i];
	// Мы не регистрируем объект в листе и не даем никаких ID-ов
}

/* create a new object from a prototype */
OBJ_DATA *read_object(obj_vnum nr, int type)
{				/* and obj_rnum */
	OBJ_DATA *obj;
	obj_rnum i;

	if (nr < 0) {
		log("SYSERR: Trying to create obj with negative (%d) num!", nr);
		return (NULL);
	}
	if (type == VIRTUAL) {
		if ((i = real_object(nr)) < 0) {
			log("Object (V) %d does not exist in database.", nr);
			return (NULL);
		}
	} else
		i = nr;

	NEWCREATE(obj, OBJ_DATA(*obj_proto[i]));
	obj_index[i].number++;
	i = obj_index[i].zone;
	if (i != -1 && zone_table[i].under_construction) {
		// модификация объектов тестовой зоны
		GET_OBJ_TIMER(obj) = TEST_OBJECT_TIMER;
		SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NOLOCATE), ITEM_NOLOCATE);
	}
	obj->proto_script = NULL;
	obj->next = object_list;
	object_list = obj;
	GET_ID(obj) = max_id++;
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
		name_from_drinkcon(obj);
		if (GET_OBJ_VAL(obj, 1) && GET_OBJ_VAL(obj, 2))
			name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2));
	}
	assign_triggers(obj, OBJ_TRIGGER);

	return (obj);
}



#define ZO_DEAD  9999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
	int i, j, k = 0;
	struct reset_q_element *update_u, *temp;
	static int timer = 0;
	char buf[1024];

	/* jelson 10/22/92 */
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {	/* one minute has passed */
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		/* since one minute has passed, increment zone ages */
		for (i = 0; i <= top_of_zone_table; i++) {
			if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode &&
			    (zone_table[i].reset_idle || zone_table[i].used))
				(zone_table[i].age)++;

			if (zone_table[i].age >= zone_table[i].lifespan &&
			    zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode &&
			    (zone_table[i].reset_idle || zone_table[i].used)) {	/* enqueue zone */

				CREATE(update_u, struct reset_q_element, 1);
				update_u->zone_to_reset = i;
				update_u->next = 0;

				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else {
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}

				zone_table[i].age = ZO_DEAD;
			}
		}
	}



	/* end - one minute has passed */
	/* dequeue zones (if possible) and reset                    */
	/* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if (zone_table[update_u->zone_to_reset].reset_mode == 2 || (is_empty(update_u->zone_to_reset)
									    && zone_table[update_u->zone_to_reset].
									    reset_mode != 3)
		    || can_be_reset(update_u->zone_to_reset)) {
			reset_zone(update_u->zone_to_reset);
			sprintf(buf, "Auto zone reset: %s", zone_table[update_u->zone_to_reset].name);
			if (zone_table[update_u->zone_to_reset].reset_mode == 3) {
				for (i = 0; i < zone_table[update_u->zone_to_reset].typeA_count; i++) {
					//Ищем real_zone по vnum
					for (j = 0; j <= top_of_zone_table; j++) {
						if (zone_table[j].number ==
						    zone_table[update_u->zone_to_reset].typeA_list[i]) {
							reset_zone(j);
							sprintf(buf, "%s]\r\n[Multireset: also resetting %s",
								buf, zone_table[j].name);
							break;
						}
					}
				}
			}
			mudlog(buf, LGH, LVL_GOD, SYSLOG, FALSE);
			/* dequeue */
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else {
				for (temp = reset_q.head; temp->next != update_u; temp = temp->next);

				if (!update_u->next)
					reset_q.tail = temp;

				temp->next = update_u->next;
			}

			free(update_u);
			k++;
			if (k >= ZONES_RESET)
				break;
		}
}

bool can_be_reset(zone_rnum zone)
{
	int i = 0, j = 0;
	if (zone_table[zone].reset_mode != 3)
		return FALSE;
// проверяем себя
	if (!is_empty(zone))
		return FALSE;
// проверяем список B
	for (i = 0; i < zone_table[zone].typeB_count; i++) {
		//Ищем real_zone по vnum
		for (j = 0; j <= top_of_zone_table; j++) {
			if (zone_table[j].number == zone_table[zone].typeB_list[i]) {
				if (!zone_table[zone].typeB_flag[i] || !is_empty(j))
					return FALSE;
				break;
			}
		}
	}
// проверяем список A
	for (i = 0; i < zone_table[zone].typeA_count; i++) {
		//Ищем real_zone по vnum
		for (j = 0; j <= top_of_zone_table; j++) {
			if (zone_table[j].number == zone_table[zone].typeA_list[i]) {
				if (!is_empty(j))
					return FALSE;
				break;
			}
		}
	}
	return TRUE;
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
	char buf[256];

	sprintf(buf, "SYSERR: zone file: %s", message);
	mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);

	sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
		ZCMD.command, zone_table[zone].number, ZCMD.line);
	mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
}

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); }

// Выполить команду, только если предыдущая успешна
#define		CHECK_SUCCESS		1
// Команда не должна изменить флаг
#define		FLAG_PERSIST		2

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
	int cmd_no;
	int cmd_tmp, obj_in_room_max, obj_in_room = 0;
	CHAR_DATA *mob = NULL, *leader = NULL, *ch;
	OBJ_DATA *obj, *obj_to, *obj_room;
	int rnum_start, rnum_stop;
	CHAR_DATA *tmob = NULL;	/* for trigger assignment */
	OBJ_DATA *tobj = NULL;	/* for trigger assignment */

	int last_state, curr_state;	// статус завершения последней и текущей команды

	log("[Reset] Start zone %s", zone_table[zone].name);
	repop_decay(zone);	/* рассыпание обьектов ITEM_REPOP_DECAY */

	// Очищаем все порталы, чтоб не читили-------------------------------
	for (rnum_start = FIRST_ROOM; rnum_start <= top_of_world; rnum_start++) {
		if (world[world[rnum_start]->portal_room]->zone == zone)
			world[rnum_start]->portal_time = 0;
		if (world[rnum_start]->zone == zone && world[rnum_start]->portal_time)
			world[rnum_start]->portal_time = 0;
	}

	//----------------------------------------------------------------------------
	last_state = 1;		// для первой команды считаем, что все ок

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
		if (ZCMD.command == '*') {
			// комментарий - ни на что не влияет
			continue;
		}

		curr_state = 0;	// по умолчанию - неудачно
		if (!(ZCMD.if_flag & CHECK_SUCCESS) || last_state) {
			// Выполняем команду, если предыдущая успешна или не нужно проверять
			switch (ZCMD.command) {
			case 'M':
				/* read a mobile */
				// 'M' <flag> <mob_vnum> <max_in_world> <room_vnum> <max_in_room|-1>
				mob = NULL;	//Добавлено Ладником
				if (mob_index[ZCMD.arg1].number < ZCMD.arg2 &&
				    (ZCMD.arg4 < 0 || mobs_in_room(ZCMD.arg1, ZCMD.arg3) < ZCMD.arg4)) {
					mob = read_mobile(ZCMD.arg1, REAL);
					char_to_room(mob, ZCMD.arg3);
					load_mtrigger(mob);
					tmob = mob;
					curr_state = 1;
				}
				tobj = NULL;
				break;

			case 'F':
				/* Follow mobiles */
				// 'F' <flag> <room_vnum> <leader_vnum> <mob_vnum>
				leader = NULL;
				if (ZCMD.arg1 >= FIRST_ROOM && ZCMD.arg1 <= top_of_world) {
					for (ch = world[ZCMD.arg1]->people; ch && !leader; ch = ch->next_in_room)
						if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg2)
							leader = ch;
					for (ch = world[ZCMD.arg1]->people; ch && leader; ch = ch->next_in_room)
						if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg3
						    && leader != ch && ch->master != leader) {
							if (ch->master)
								stop_follower(ch, SF_EMPTY);
							add_follower(ch, leader);
							curr_state = 1;
						}
				}
				break;

			case 'Q':
				/* delete all mobiles */
				// 'Q' <flag> <mob_vnum>
				for (ch = character_list; ch; ch = leader) {
					leader = ch->next;
					// Карачун. Поднятые мобы не должны уничтожаться.
					if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg1 && !MOB_FLAGGED(ch, MOB_RESURRECTED)) {
						// Карачун. Мобы должны оставлять стафф.
						extract_char(ch, FALSE);
						//extract_mob(ch);
						curr_state = 1;
					}
				}
				tobj = NULL;
				tmob = NULL;
				break;

			case 'O':
				/* read an object */
				// 'O' <flag> <obj_vnum> <max_in_world> <room_vnum|-1> <load%|-1>
				// Примечание: room_vnum = -1 НЕ ЗАДАВАТЬ, БУДЕТ КРЭШ
				/* Проверка  - сколько всего таких же обьектов надо на эту клетку */
				for (cmd_tmp = 0, obj_in_room_max = 0; ZCMD_CMD(cmd_tmp).command != 'S'; cmd_tmp++)
					if ((ZCMD_CMD(cmd_tmp).command == 'O')
					    && (ZCMD.arg1 == ZCMD_CMD(cmd_tmp).arg1)
					    && (ZCMD.arg3 == ZCMD_CMD(cmd_tmp).arg3))
						obj_in_room_max++;
				/* Теперь считаем склько их на текущей клетке */
				if (ZCMD.arg3 >= 0)
					for (obj_room = world[ZCMD.arg3]->contents, obj_in_room = 0;
					     obj_room; obj_room = obj_room->next_content)
						if (ZCMD.arg1 == GET_OBJ_RNUM(obj_room))
							obj_in_room++;
				/* Теперь грузим обьект если надо */
				if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored <
				    ZCMD.arg2 && (ZCMD.arg4 <= 0 || number(1, 100) <= ZCMD.arg4)
				    && (obj_in_room < obj_in_room_max)) {
					obj = read_object(ZCMD.arg1, REAL);
					if (ZCMD.arg3 >= 0) {
						GET_OBJ_ZONE(obj) = world[ZCMD.arg3]->zone;
						obj_to_room(obj, ZCMD.arg3);
						load_otrigger(obj);
					} else {
						IN_ROOM(obj) = NOWHERE;
					}
					tobj = obj;
					curr_state = 1;
					if (!OBJ_FLAGGED(obj, ITEM_NODECAY)) {
						sprintf(buf,
							"&YВНИМАНИЕ&G На землю загружен объект без флага NODECAY : %s (VNUM=%d)",
							GET_OBJ_PNAME(obj, 0), GET_OBJ_VNUM(obj));
						mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
					}
				}
				tmob = NULL;
				break;

			case 'P':
				/* object to object */
				// 'P' <flag> <obj_vnum> <max_in_world> <target_vnum> <load%|-1>
				if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored <
				    ZCMD.arg2 && (ZCMD.arg4 <= 0 || number(1, 100) <= ZCMD.arg4)) {
					if (!(obj_to = get_obj_num(ZCMD.arg3))) {
						ZONE_ERROR("target obj not found, command omited");
//                 ZCMD.command = '*';
						break;
					}
					if (GET_OBJ_TYPE(obj_to) != ITEM_CONTAINER) {
						ZONE_ERROR("attempt put obj to non container, omited");
						ZCMD.command = '*';
						break;
					}
					obj = read_object(ZCMD.arg1, REAL);
					if (obj_to->in_room != NOWHERE)
						GET_OBJ_ZONE(obj) = world[obj_to->in_room]->zone;
					else if (obj_to->worn_by)
						GET_OBJ_ZONE(obj) = world[IN_ROOM(obj_to->worn_by)]->zone;
					else if (obj_to->carried_by)
						GET_OBJ_ZONE(obj) = world[IN_ROOM(obj_to->carried_by)]->zone;
					obj_to_obj(obj, obj_to);
					load_otrigger(obj);
					tobj = obj;
					curr_state = 1;
				}
				tmob = NULL;
				break;

			case 'G':
				/* obj_to_char */
				// 'G' <flag> <obj_vnum> <max_in_world> <-> <load%|-1>
				if (!mob)
					//Изменено Ладником
				{
					// ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
					// ZCMD.command = '*';
					break;
				}
				if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored <
				    ZCMD.arg2 && (ZCMD.arg4 <= 0 || number(1, 100) <= ZCMD.arg4)) {
					obj = read_object(ZCMD.arg1, REAL);
					obj_to_char(obj, mob);
					GET_OBJ_ZONE(obj) = world[IN_ROOM(mob)]->zone;
					tobj = obj;
					load_otrigger(obj);
					curr_state = 1;
				}
				tmob = NULL;
				break;

			case 'E':
				/* object to equipment list */
				// 'E' <flag> <obj_vnum> <max_in_world> <wear_pos> <load%|-1>
				//Изменено Ладником
				if (!mob) {
					//ZONE_ERROR("trying to equip non-existant mob, command disabled");
					// ZCMD.command = '*';
					break;
				}
				if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored <
				    ZCMD.arg2 && (ZCMD.arg4 <= 0 || number(1, 100) <= ZCMD.arg4)) {
					if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
						ZONE_ERROR("invalid equipment pos number");
					} else {
						obj = read_object(ZCMD.arg1, REAL);
						GET_OBJ_ZONE(obj) = world[IN_ROOM(mob)]->zone;
						IN_ROOM(obj) = IN_ROOM(mob);
						load_otrigger(obj);
						if (wear_otrigger(obj, mob, ZCMD.arg3)) {
							IN_ROOM(obj) = NOWHERE;
							equip_char(mob, obj, ZCMD.arg3);
						} else
							obj_to_char(obj, mob);
						if (!(obj->carried_by == mob) && !(obj->worn_by == mob)) {
							extract_obj(obj);
							tobj = NULL;
						} else {
							tobj = obj;
							curr_state = 1;
						}
					}
				}
				tmob = NULL;
				break;

			case 'R':
				/* rem obj from room */
				// 'R' <flag> <room_vnum> <obj_vnum>
				if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1]->contents)) != NULL) {
					obj_from_room(obj);
					extract_obj(obj);
					curr_state = 1;
				}
				tmob = NULL;
				tobj = NULL;
				break;

			case 'D':
				/* set state of door */
				// 'D' <flag> <room_vnum> <door_pos> <door_state>
				if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
				    (world[ZCMD.arg1]->dir_option[ZCMD.arg2] == NULL)) {
					ZONE_ERROR("door does not exist, command disabled");
					ZCMD.command = '*';
				} else {
					switch (ZCMD.arg3) {
					case 0:
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
							   exit_info, EX_LOCKED);
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
							   exit_info, EX_CLOSED);
						break;
					case 1:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
							   exit_info, EX_LOCKED);
						break;
					case 2:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
						break;
					case 3:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
						break;
					case 4:
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
							   exit_info, EX_HIDDEN);
						break;
					}
					curr_state = 1;
				}
				tmob = NULL;
				tobj = NULL;
				break;

			case 'T':
				/* trigger command; details to be filled in later */
				// 'T' <flag> <trigger_type> <trigger_vnum> <room_vnum, для WLD_TRIGGER>
				if (ZCMD.arg1 == MOB_TRIGGER && tmob) {
					if (!SCRIPT(tmob))
						CREATE(SCRIPT(tmob), SCRIPT_DATA, 1);
					add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
					curr_state = 1;
				} else if (ZCMD.arg1 == OBJ_TRIGGER && tobj) {
					if (!SCRIPT(tobj))
						CREATE(SCRIPT(tobj), SCRIPT_DATA, 1);
					add_trigger(SCRIPT(tobj), read_trigger(real_trigger(ZCMD.arg2)), -1);
					curr_state = 1;
				} else if (ZCMD.arg1 == WLD_TRIGGER) {
					if (ZCMD.arg3 != NOWHERE) {
						if (!(world[ZCMD.arg3]->script))
							CREATE(world[ZCMD.arg3]->script, SCRIPT_DATA, 1);
						add_trigger(world[ZCMD.arg3]->script,
							    read_trigger(real_trigger(ZCMD.arg2)), -1);
						curr_state = 1;
					}
				}
				break;

			case 'V':
				// 'V' <flag> <trigger_type> <room_vnum> <context> <var_name> <var_value>
				if (ZCMD.arg1 == MOB_TRIGGER && tmob) {
					if (!SCRIPT(tmob)) {
						ZONE_ERROR("Attempt to give variable to scriptless mobile");
					} else {
						add_var_cntx(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1,
							     ZCMD.sarg2, ZCMD.arg3);
						curr_state = 1;
					}
				} else if (ZCMD.arg1 == OBJ_TRIGGER && tobj) {
					if (!SCRIPT(tobj)) {
						ZONE_ERROR("Attempt to give variable to scriptless object");
					} else {
						add_var_cntx(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1,
							     ZCMD.sarg2, ZCMD.arg3);
						curr_state = 1;
					}
				} else if (ZCMD.arg1 == WLD_TRIGGER) {
					if (ZCMD.arg2 < FIRST_ROOM || ZCMD.arg2 > top_of_world) {
						ZONE_ERROR("Invalid room number in variable assignment");
					} else {
						if (!(world[ZCMD.arg2]->script)) {
							ZONE_ERROR("Attempt to give variable to scriptless object");
						} else {
							add_var_cntx(&
								     (world[ZCMD.arg2]->script->
								      global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
							curr_state = 1;
						}
					}
				}
				break;

			default:
				ZONE_ERROR("unknown cmd in reset table; cmd disabled");
				ZCMD.command = '*';
				break;
			}
		}
		/* if ( (ZCMD.if_flag&CHECK_SUCCESS) && !last_state ) */
		if (!(ZCMD.if_flag & FLAG_PERSIST)) {
			// команда изменяет флаг
			last_state = curr_state;
		}

	}

	zone_table[zone].age = 0;
	zone_table[zone].used = FALSE;

	if (get_zone_rooms(zone, &rnum_start, &rnum_stop)) {
		int rnum;
		/* handle reset_wtrigger's */
		log("[Reset] Triggers");
		for (rnum = rnum_start; rnum <= rnum_stop; ++rnum) {
			reset_wtrigger(world[rnum]);
		}

		log("[Reset] Ingredients");
		for (rnum = rnum_start; rnum <= rnum_stop; ++rnum) {
//MZ.load
im_reset_room(world[rnum], zone_table[zone].level, zone_table[zone].type);
//-MZ.load

		}
	}

	log("[Reset] Paste mobiles");
	paste_mobiles(zone);
	log("[Reset] Parsing multireset");
	for (rnum_start = 0; rnum_start <= top_of_zone_table; rnum_start++) {
		// проверяем, не содержится ли текущая зона в чьем-либо typeB_list
		for (curr_state = zone_table[rnum_start].typeB_count; curr_state > 0; curr_state--) {
			if (zone_table[rnum_start].typeB_list[curr_state - 1] == zone_table[zone].number) {
				zone_table[rnum_start].typeB_flag[curr_state - 1] = TRUE;
				log("[Reset] Adding TRUE for zone %d in the array contained by zone %d",
				    zone_table[zone].number, zone_table[rnum_start].number);
				break;
			}
		};
	}
//Если это ведущая зона, то при ее сбросе обнуляем typeB_flag
	for (rnum_start = zone_table[zone].typeB_count; rnum_start > 0; rnum_start--)
		zone_table[zone].typeB_flag[rnum_start - 1] = FALSE;
	log("[Reset] Stop zone %s", zone_table[zone].name);
}


void paste_mobiles(int zone)
{
	CHAR_DATA *ch, *ch_next;
	OBJ_DATA *obj, *obj_next;
	int time_ok, month_ok, need_move, no_month, no_time, room = -1;

	for (ch = character_list; ch; ch = ch_next) {
		ch_next = ch->next;
		if (!IS_NPC(ch))
			continue;
		if (FIGHTING(ch))
			continue;
		if (GET_POS(ch) < POS_STUNNED)
			continue;
		if (AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HORSE) || AFF_FLAGGED(ch, AFF_HOLD))
			continue;
		if (MOB_FLAGGED(ch, MOB_CORPSE))
			continue;
		if ((room = IN_ROOM(ch)) == NOWHERE)
			continue;
		if (zone >= 0 && world[room]->zone != zone)
			continue;

		time_ok = FALSE;
		month_ok = FALSE;
		need_move = FALSE;
		no_month = TRUE;
		no_time = TRUE;

		if (MOB_FLAGGED(ch, MOB_LIKE_DAY)) {
			if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
				time_ok = TRUE;
			need_move = TRUE;
			no_time = FALSE;
		}
		if (MOB_FLAGGED(ch, MOB_LIKE_NIGHT)) {
			if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
				time_ok = TRUE;
			need_move = TRUE;
			no_time = FALSE;
		}
		if (MOB_FLAGGED(ch, MOB_LIKE_FULLMOON)) {
			if ((weather_info.sunlight == SUN_SET ||
			     weather_info.sunlight == SUN_DARK) &&
			    (weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
				time_ok = TRUE;
			need_move = TRUE;
			no_time = FALSE;
		}
		if (MOB_FLAGGED(ch, MOB_LIKE_WINTER)) {
			if (weather_info.season == SEASON_WINTER)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (MOB_FLAGGED(ch, MOB_LIKE_SPRING)) {
			if (weather_info.season == SEASON_SPRING)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (MOB_FLAGGED(ch, MOB_LIKE_SUMMER)) {
			if (weather_info.season == SEASON_SUMMER)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (MOB_FLAGGED(ch, MOB_LIKE_AUTUMN)) {
			if (weather_info.season == SEASON_AUTUMN)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (need_move) {
			month_ok |= no_month;
			time_ok |= no_time;
			if (month_ok && time_ok) {
				if (world[room]->number != zone_table[world[room]->zone].top)
					continue;
				if (GET_LASTROOM(ch) == NOWHERE) {
					extract_mob(ch);
					continue;
				}
				char_from_room(ch);
				char_to_room(ch, real_room(GET_LASTROOM(ch)));
				//log("Put %s at room %d",GET_NAME(ch),world[IN_ROOM(ch)]->number);
			} else {
				if (world[room]->number == zone_table[world[room]->zone].top)
					continue;
				GET_LASTROOM(ch) = GET_ROOM_VNUM(room);
				char_from_room(ch);
				room = real_room(zone_table[world[room]->zone].top);
				if (room == NOWHERE)
					room = real_room(GET_LASTROOM(ch));
				char_to_room(ch, room);
				//log("Remove %s at room %d",GET_NAME(ch),world[IN_ROOM(ch)]->number);
			}
		}
	}

	for (obj = object_list; obj; obj = obj_next) {
		obj_next = obj->next;
		if (obj->carried_by || obj->worn_by || (room = obj->in_room) == NOWHERE)
			continue;
		if (zone >= 0 && world[room]->zone != zone)
			continue;
		time_ok = FALSE;
		month_ok = FALSE;
		need_move = FALSE;
		no_time = TRUE;
		no_month = TRUE;
		if (OBJ_FLAGGED(obj, ITEM_DAY)) {
			if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
				time_ok = TRUE;
			need_move = TRUE;
			no_time = FALSE;
		}
		if (OBJ_FLAGGED(obj, ITEM_NIGHT)) {
			if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
				time_ok = TRUE;
			need_move = TRUE;
			no_time = FALSE;
		}
		if (OBJ_FLAGGED(obj, ITEM_FULLMOON)) {
			if ((weather_info.sunlight == SUN_SET ||
			     weather_info.sunlight == SUN_DARK) &&
			    (weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
				time_ok = TRUE;
			need_move = TRUE;
			no_time = FALSE;
		}
		if (OBJ_FLAGGED(obj, ITEM_WINTER)) {
			if (weather_info.season == SEASON_WINTER)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (OBJ_FLAGGED(obj, ITEM_SPRING)) {
			if (weather_info.season == SEASON_SPRING)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (OBJ_FLAGGED(obj, ITEM_SUMMER)) {
			if (weather_info.season == SEASON_SUMMER)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (OBJ_FLAGGED(obj, ITEM_AUTUMN)) {
			if (weather_info.season == SEASON_AUTUMN)
				month_ok = TRUE;
			need_move = TRUE;
			no_month = FALSE;
		}
		if (need_move) {
			month_ok |= no_month;
			time_ok |= no_time;
			if (month_ok && time_ok) {
				if (world[room]->number != zone_table[world[room]->zone].top)
					continue;
				if (OBJ_GET_LASTROOM(obj) == NOWHERE) {
					extract_obj(obj);
					continue;
				}
				obj_from_room(obj);
				obj_to_room(obj, real_room(OBJ_GET_LASTROOM(obj)));
				//log("Put %s at room %d",obj->PNames[0],world[OBJ_GET_LASTROOM(obj)]->number);
			} else {
				if (world[room]->number == zone_table[world[room]->zone].top)
					continue;
				OBJ_GET_LASTROOM(obj) = GET_ROOM_VNUM(room);
				obj_from_room(obj);
				room = real_room(zone_table[world[room]->zone].top);
				if (room == NOWHERE)
					room = real_room(OBJ_GET_LASTROOM(obj));
				obj_to_room(obj, room);
				//log("Remove %s at room %d",GET_NAME(ch),world[room]->number);
			}
		}
	}
}

// Ищет RNUM первой и последней комнаты зоны
// Еси возвращает 0 - комнат в зоне нету
int get_zone_rooms(int zone_nr, int *start, int *stop)
{
	int first_room_vnum, rnum;
	first_room_vnum = zone_table[zone_nr].top;
	rnum = real_room(first_room_vnum);
	if (rnum == NOWHERE)
		return 0;
	*stop = rnum;
	rnum = NOWHERE;
	while (zone_nr) {
		first_room_vnum = zone_table[--zone_nr].top;
		rnum = real_room(first_room_vnum);
		if (rnum != NOWHERE) {
			++rnum;
			break;
		}
	}
	if (rnum == NOWHERE)
		rnum = 1;	// самая первая зона начинается с 1
	*start = rnum;
	return 1;
}


/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
	DESCRIPTOR_DATA *i;
	int rnum_start, rnum_stop;
	CHAR_DATA *c;

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == NOWHERE)
			continue;
		if (GET_LEVEL(i->character) >= LVL_IMMORT)
			continue;
		if (world[i->character->in_room]->zone != zone_nr)
			continue;

		return (0);
	}

	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!get_zone_rooms(zone_nr, &rnum_start, &rnum_stop))
		return 1;	// в зоне нет комнат :)

	for (; rnum_start <= rnum_stop; rnum_start++) {
// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		for (c = world[rnum_start]->people; c; c = c->next_in_room)
			if (!IS_NPC(c) && (GET_LEVEL(c) < LVL_IMMORT))
				return 0;
	}

// теперь проверю всех товарищей в void комнате STRANGE_ROOM
	for (c = world[STRANGE_ROOM]->people; c; c = c->next_in_room) {
		int was = GET_WAS_IN(c);
		if (was == NOWHERE)
			continue;
		if (GET_LEVEL(c) >= LVL_IMMORT)
			continue;
		if (world[was]->zone != zone_nr)
			continue;
		return 0;
	}

	return (1);
}

int mobs_in_room(int m_num, int r_num)
{
	CHAR_DATA *ch;
	int count = 0;

	for (ch = world[r_num]->people; ch; ch = ch->next_in_room)
		if (m_num == GET_MOB_RNUM(ch))
			count++;

	return count;
}


/*************************************************************************
*  stuff related to the save/load player system                          *
*************************************************************************/

long cmp_ptable_by_name(char *name, int len)
{
	int i;

	len = MIN(len, strlen(name));
	one_argument(name, arg);
	for (i = 0; i <= top_of_p_table; i++)
		if (!strn_cmp(player_table[i].name, arg, MIN(len, strlen(player_table[i].name))))
			return (i);
	return (-1);
}



long get_ptable_by_name(char *name)
{
	int i;

	one_argument(name, arg);
	for (i = 0; i <= top_of_p_table; i++)
		if (!str_cmp(player_table[i].name, arg))
			return (i);
	sprintf(buf, "Char %s(%s) not found !!!", name, arg);
	mudlog(buf, LGH, LVL_IMMORT, SYSLOG, FALSE);
	return (-1);
}


long get_id_by_name(char *name)
{
	int i;

	one_argument(name, arg);
	for (i = 0; i <= top_of_p_table; i++)
		if (!str_cmp(player_table[i].name, arg))
			return (player_table[i].id);

	return (-1);
}


char *get_name_by_id(long id)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		if (player_table[i].id == id)
			return (player_table[i].name);

	return (NULL);
}

char *get_name_by_unique(long unique)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		if (player_table[i].unique == unique)
			return (player_table[i].name);

	return (NULL);
}


void delete_unique(CHAR_DATA * ch)
{
	int i;

	if ((i = get_id_by_name(GET_NAME(ch))) >= 0)
		player_table[i].unique = GET_UNIQUE(ch) = -1;

}

int correct_unique(int unique)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		if (player_table[i].unique == unique)
			return (TRUE);

	return (FALSE);
}

int create_unique(void)
{
	int unique;

	do {
		unique = (number(0, 64) << 24) + (number(0, 255) << 16) + (number(0, 255) << 8) + (number(0, 255));
	}
	while (correct_unique(unique));
	return (unique);
}

// shapirus
struct ignore_data *parse_ignore(char *buf)
{
	struct ignore_data *result;

	CREATE(result, struct ignore_data, 1);

//log("parsing ignore item '%s'", buf);

	if (sscanf(buf, "[%ld]%ld", &result->mode, &result->id) < 2) {
		free(result);
		return NULL;
	} else {
		result->next = NULL;
		return result;
	}
}

// возвращает истину, если чар с заданным id
// существует, ложь в противном случае
bool ign_plr_exists(long id)
{
	int i;

	if (id == -1)
		return TRUE;

	for (i = 0; i <= top_of_p_table; i++)
		if (id == player_table[i].id)
			return TRUE;
	return FALSE;
}

// можно вызывать много раз по разу, а можно один раз
// вызывать и скормить в одной строке всех
void load_ignores(CHAR_DATA * ch, char *line)
{
	struct ignore_data *cur_ign, *ignore_list;
	char *buf;
	unsigned int i, k, done = 0;

// найдем последний элемент списка на случай, если функцию
// хотят вызывать многократно
	for (ignore_list = IGNORE_LIST(ch); ignore_list && ignore_list->next; ignore_list = ignore_list->next);
	buf = strdup(line);
	for (i = k = 0;;) {
		if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == 0) {
			if (buf[i] == 0)
				done = 1;
			buf[i] = 0;
			cur_ign = parse_ignore(&(buf[k]));
			if (cur_ign) {
				if (!ignore_list) {
					IGNORE_LIST(ch) = cur_ign;
				} else {
					ignore_list->next = cur_ign;
				}
				ignore_list = cur_ign;
				// удаленных игроков из листа нафиг
				if (!ign_plr_exists(ignore_list->id))
					ignore_list->id = 0;
			} else {
				log("WARNING: could not parse ignore list " "of %s: invalid format", GET_NAME(ch));
				return;
			}
			// skip whitespace
			for (k = i + 1; buf[k] == ' ' || buf[k] == '\t'; k++);
			i = k;
			if (done || buf[k] == 0)
				break;
		} else {
			i++;
		}
	}
	free(buf);
}

#define NUM_OF_SAVE_THROWS	5

/* new load_char reads ascii pfiles */
/* Load a char, TRUE if loaded, FALSE if not */

// на счет reboot: используется только при старте мада в вызовах из entrycount
// при включенном флаге файл читается только до поля Rebt, все остальные поля пропускаются
// поэтому при каких-то изменениях в entrycount, must_be_deleted и TopPlayer::Refresh следует
// убедиться, что изменный код работает с действительно проинициализированными полями персонажа
// на данный момент это: PLR_FLAGS, GET_CLASS, GET_EXP, GET_IDNUM, LAST_LOGON, GET_LEVEL, GET_NAME, GET_REMORT, GET_UNIQUE
int load_char_ascii(const char *name, CHAR_DATA * ch, bool reboot = 0)
{
	int id, num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, i;
	long int lnum = 0, lnum3 = 0;
	FBFILE *fl;
	char filename[40];
	char buf[128], line[MAX_INPUT_LENGTH + 1], tag[6];
	char line1[MAX_INPUT_LENGTH + 1];
	AFFECT_DATA af;
	struct timed_type timed;


	*filename = '\0';
	log("Load ascii char %s", name);
	if (now_entrycount) {
		id = 1;
	} else {
		id = find_name(name);
	}
	if (!(id >= 0 && get_filename(name, filename, PLAYERS_FILE) && (fl = fbopen(filename, FB_READ)))) {
		log("Cann't load ascii %d %s", id, filename);
		return (-1);
	}

///////////////////////////////////////////////////////////////////////////////

	// первыми иним и парсим поля для ребута до поля "Rebt", если reboot на входе = 1, то на этом парс и кончается
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, struct player_special_data, 1);
	GET_LEVEL(ch) = 1;
	GET_CLASS(ch) = 1;
	GET_UNIQUE(ch) = 0;
	LAST_LOGON(ch) = time(0);
	GET_IDNUM(ch) = 0;
	GET_EXP(ch) = 0;
	GET_REMORT(ch) = 0;
	asciiflag_conv("", &PLR_FLAGS(ch, 0));

	bool skip_file = 0;

	do {
		if (!fbgetline(fl, line)) {
			log("SYSERROR: Wrong file ascii %d %s", id, filename);
			return (-1);
		}

		tag_argument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++) {
			line1[i] = line[i];
		}
		line1[i] = '\0';
		num = atoi(line1);
		lnum = atol(line1);

		switch (*tag) {
		case 'A':
			if (!strcmp(tag, "Act "))
				asciiflag_conv(line, &PLR_FLAGS(ch, 0));
			break;
		case 'C':
			if (!strcmp(tag, "Clas"))
				GET_CLASS(ch) = num;
			break;
		case 'E':
			if (!strcmp(tag, "Exp "))
				GET_EXP(ch) = lnum;
			break;
		case 'I':
			if (!strcmp(tag, "Id  "))
				GET_IDNUM(ch) = lnum;
		case 'L':
			if (!strcmp(tag, "LstL"))
				LAST_LOGON(ch) = lnum;
			else if (!strcmp(tag, "Levl"))
				GET_LEVEL(ch) = num;
		case 'N':
			if (!strcmp(tag, "Name"))
				GET_NAME(ch) = str_dup(line);
		case 'R':
			if (!strcmp(tag, "Rebt"))
				skip_file = 1;
			else if (!strcmp(tag, "Rmrt"))
				GET_REMORT(ch) = num;
			break;
		case 'U':
			if (!strcmp(tag, "UIN "))
				GET_UNIQUE(ch) = num;
			break;
		default:
			sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	} while (!skip_file);

	// если с загруженными выше полями что-то хочется делать после лоада - делайте это здесь

	//Indexing experience - if his exp is lover than required for his level - set it to required
	if (GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch)))
		GET_EXP(ch) = level_exp(ch, GET_LEVEL(ch));

	if (reboot) {
		fbclose(fl);
		return id;
	}

	// если происходит обычный лоад плеера, то читаем файл дальше и иним все остальные поля

///////////////////////////////////////////////////////////////////////////////


	/* character init */
	/* initializations necessary to keep some things straight */

	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;

	ch->real_abils.Feats.reset();
	for (i = 1; i <= MAX_SKILLS; i++)
		GET_SKILL(ch, i) = 0;
	for (i = 1; i <= MAX_SPELLS; i++)
		GET_SPELL_TYPE(ch, num) = 0;
	for (i = 1; i <= MAX_SPELLS; i++)
		GET_SPELL_MEM(ch, num) = 0;
	ch->char_specials.saved.affected_by = clear_flags;
	POOFIN(ch) = NULL;
	POOFOUT(ch) = NULL;
	GET_LAST_TELL(ch) = NOBODY;
	GET_RSKILL(ch) = NULL;	// рецептов не знает
	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;
	ch->real_abils.armor = 100;
	GET_MEM_TOTAL(ch) = 0;
	GET_MEM_COMPLETED(ch) = 0;
	MemQ_init(ch);

	GET_AC(ch) = 10;
	GET_ALIGNMENT(ch) = 0;
	GET_BAD_PWS(ch) = 0;
	GET_BANK_GOLD(ch) = 0;
	ch->player.time.birth = time(0);
	GET_CHA(ch) = 10;
	GET_KIN(ch) = 0;
	GET_CON(ch) = 10;
	GET_COMMSTATE(ch) = 0;
	GET_DEX(ch) = 10;
	GET_COND(ch, DRUNK) = 0;
	GET_DRUNK_STATE(ch) = 0;

// Punish Init
	DUMB_DURATION(ch) = 0;
	DUMB_REASON(ch) = 0;
	GET_DUMB_LEV(ch) = 0;
	DUMB_GODID(ch) = 0;

	MUTE_DURATION(ch) = 0;
	MUTE_REASON(ch) = 0;
	GET_MUTE_LEV(ch) = 0;
	MUTE_GODID(ch) = 0;

	HELL_DURATION(ch) = 0;
	HELL_REASON(ch) = 0;
	GET_HELL_LEV(ch) = 0;
	HELL_GODID(ch) = 0;

	FREEZE_DURATION(ch) = 0;
	FREEZE_REASON(ch) = 0;
	GET_FREEZE_LEV(ch) = 0;
	FREEZE_GODID(ch) = 0;

	GCURSE_DURATION(ch) = 0;
	GCURSE_REASON(ch) = 0;
	GET_GCURSE_LEV(ch) = 0;
	GCURSE_GODID(ch) = 0;

	NAME_DURATION(ch) = 0;
	NAME_REASON(ch) = 0;
	GET_NAME_LEV(ch) = 0;
	NAME_GODID(ch) = 0;

	UNREG_DURATION(ch) = 0;
	UNREG_REASON(ch) = 0;
	GET_UNREG_LEV(ch) = 0;
	UNREG_GODID(ch) = 0;

// End punish init

	GET_DR(ch) = 0;
	GET_GOLD(ch) = 0;

	GET_GLORY(ch) = 0;
	ch->player_specials->saved.GodsLike = 0;
	GET_HIT(ch) = 21;
	GET_MAX_HIT(ch) = 21;
	GET_HEIGHT(ch) = 50;
	GET_HOME(ch) = 0;
	GET_HR(ch) = 0;
	GET_COND(ch, FULL) = 0;
	GET_HOUSE_UID(ch) = 0;
	GET_INT(ch) = 10;
	GET_INVIS_LEV(ch) = 0;
	ch->player.time.logon = time(0);
	GET_MOVE(ch) = 44;
	GET_MAX_MOVE(ch) = 44;
	KARMA(ch) = 0;
	LOGON_LIST(ch) = 0;
	NAME_GOD(ch) = 0;
	STRING_LENGTH(ch) = 80;
	STRING_WIDTH(ch) = 25;
	NAME_ID_GOD(ch) = 0;
	GET_OLC_ZONE(ch) = 0;
	ch->player.time.played = 0;
	IS_KILLER(ch) = 0;
	GET_LOADROOM(ch) = NOWHERE;
	GET_RELIGION(ch) = 1;
	GET_RACE(ch) = 1;
	GET_HOUSE_RANK(ch) = 0;
	GET_SEX(ch) = 0;
	GET_STR(ch) = 10;
	GET_COND(ch, THIRST) = 0;
	GET_WEIGHT(ch) = 50;
	GET_WIMP_LEV(ch) = 0;
	GET_WIS(ch) = 10;
	asciiflag_conv("", &PRF_FLAGS(ch, 0));
	asciiflag_conv("", &AFF_FLAGS(ch, 0));
	ch->Questing.quests = NULL;
	ch->Questing.count = 0;
	GET_PORTALS(ch) = NULL;
	GET_LASTIP(ch)[0] = 0;
	EXCHANGE_FILTER(ch) = NULL;
	IGNORE_LIST(ch) = NULL;
	CREATE(GET_LOGS(ch), int, NLOG);

	// здесь можно указать дату, с которой пойдет отсчет новых сообщений,
	// например, чтобы не пугать игрока 200+ новостями при первом запуске системы
	GENERAL_BOARD_DATE(ch) = 1143706650;
	NEWS_BOARD_DATE(ch) = 1143706650;
	IDEA_BOARD_DATE(ch) = 1143706650;
	ERROR_BOARD_DATE(ch) = 1143706650;
	GODNEWS_BOARD_DATE(ch) = 1143706650;
	GODGENERAL_BOARD_DATE(ch) = 1143706650;
	GODBUILD_BOARD_DATE(ch) = 1143706650;
	GODCODE_BOARD_DATE(ch) = 1143706650;
	GODPUNISH_BOARD_DATE(ch) = 1143706650;
	PERS_BOARD_DATE(ch) = 1143706650;
	CLAN_BOARD_DATE(ch) = 1143706650;
	CLANNEWS_BOARD_DATE(ch) = 1143706650;

	while (fbgetline(fl, line)) {
		tag_argument(line, tag);
		for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++) {
			line1[i] = line[i];
		}
		line1[i] = '\0';
		num = atoi(line1);
		lnum = atol(line1);

		switch (*tag) {
		case 'A':
			if (!strcmp(tag, "Ac  "))
				GET_AC(ch) = num;
			else if (!strcmp(tag, "Aff "))
				asciiflag_conv(line, &AFF_FLAGS(ch, 0));
			else if (!strcmp(tag, "Affs")) {
				i = 0;
				do {
					fbgetline(fl, line);
					sscanf(line, "%d %d %d %d %d", &num, &num2, &num3, &num4, &num5);
					if (num > 0) {
						af.type = num;
						af.duration = num2;
						af.modifier = num3;
						af.location = num4;
						af.bitvector = num5;
						affect_to_char(ch, &af);
						i++;
					}
				}
				while (num != 0);
			} else if (!strcmp(tag, "Alin"))
				GET_ALIGNMENT(ch) = num;
			break;

		case 'B':
			if (!strcmp(tag, "Badp"))
				GET_BAD_PWS(ch) = num;
			else if (!strcmp(tag, "Bank"))
				GET_BANK_GOLD(ch) = lnum;

			else if (!strcmp(tag, "Br01"))
				GENERAL_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br02"))
				NEWS_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br03"))
				IDEA_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br04"))
				ERROR_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br05"))
				GODNEWS_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br06"))
				GODGENERAL_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br07"))
				GODBUILD_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br08"))
				GODCODE_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br09"))
				GODPUNISH_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br10"))
				PERS_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br11"))
				CLAN_BOARD_DATE(ch) = lnum;
			else if (!strcmp(tag, "Br12"))
				CLANNEWS_BOARD_DATE(ch) = lnum;

			else if (!strcmp(tag, "Brth"))
				ch->player.time.birth = lnum;
			break;

		case 'C':
			if (!strcmp(tag, "Cha "))
				GET_CHA(ch) = num;
			else if (!strcmp(tag, "Con "))
				GET_CON(ch) = num;
			else if (!strcmp(tag, "ComS"))
				GET_COMMSTATE(ch) = num;
			break;

		case 'D':
			if (!strcmp(tag, "Desc")) {
				ch->player.description = fbgetstring(fl);
			} else if (!strcmp(tag, "Dex "))
				GET_DEX(ch) = num;
			else if (!strcmp(tag, "Drnk"))
				GET_COND(ch, DRUNK) = num;
			else if (!strcmp(tag, "DrSt"))
				GET_DRUNK_STATE(ch) = num;
			// Оставлено для совместимости со старым форматом наказаний
			else if (!strcmp(tag, "DmbD")) {
				DUMB_DURATION(ch) = lnum;
				while (line[i] && a_isspace(line[i]))
					++i;
				if (line[i])
					DUMB_REASON(ch) = strcpy((char *) malloc(strlen(line + i) + 1), line + i);
			} else if (!strcmp(tag, "Drol"))
				GET_DR(ch) = num;
			break;

		case 'E':
			if (!strcmp(tag, "EMal"))
				strcpy(GET_EMAIL(ch), line);
//F@N++
			else if (!strcmp(tag, "ExFl"))
				EXCHANGE_FILTER(ch) = str_dup(line);
//F@N--
			break;

		case 'F':
			// Оставлено для совместимости со старым форматом наказаний
			if (!strcmp(tag, "Frez"))
				GET_FREEZE_LEV(ch) = num;
			else if (!strcmp(tag, "FrzD")) {
				FREEZE_DURATION(ch) = lnum;
				while (line[i] && a_isspace(line[i]))
					++i;
				if (line[i])
					FREEZE_REASON(ch) = strcpy((char *) malloc(strlen(line + i) + 1), line + i);
			} else if (!strcmp(tag, "Feat")) {
				do {
					fbgetline(fl, line);
					sscanf(line, "%d", &num);
					if (num > 0 && num < MAX_FEATS)
						if(feat_info[num].classknow[(int) GET_CLASS(ch)][(int) GET_KIN (ch)])
							SET_FEAT(ch, num);
				}
				while (num != 0);
			} else if (!strcmp(tag, "FtTm")) {
				do {
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0) {
						timed.skill = num;
						timed.time = num2;
						timed_feat_to_char(ch, &timed);
					}
				}
				while (num != 0);
			}
			break;

		case 'G':
			if (!strcmp(tag, "Gold"))
				GET_GOLD(ch) = num;
			else if (!strcmp(tag, "GodD"))
				GCURSE_DURATION(ch) = lnum;
			else if (!strcmp(tag, "Glor"))
				GET_GLORY(ch) = num;
			else if (!strcmp(tag, "GdFl"))
				ch->player_specials->saved.GodsLike = lnum;
			break;

		case 'H':
			if (!strcmp(tag, "Hit ")) {
				sscanf(line, "%d/%d", &num, &num2);
				GET_HIT(ch) = num;
				GET_MAX_HIT(ch) = num2;
			} else if (!strcmp(tag, "Hite"))
				GET_HEIGHT(ch) = num;
			else if (!strcmp(tag, "Home"))
				GET_HOME(ch) = num;
			else if (!strcmp(tag, "Hrol"))
				GET_HR(ch) = num;
			else if (!strcmp(tag, "Hung"))
				GET_COND(ch, FULL) = num;
			// Оставлено для совместимости со старым форматом наказаний
			else if (!strcmp(tag, "HelD")) {
				HELL_DURATION(ch) = lnum;
				while (line[i] && a_isspace(line[i]))
					++i;
				if (line[i])
					HELL_REASON(ch) = strcpy((char *) malloc(strlen(line + i) + 1), line + i);
			} else if (!strcmp(tag, "HsID"))
				GET_HOUSE_UID(ch) = lnum;
			else if (!strcmp(tag, "Host"))
				strcpy(GET_LASTIP(ch), line);
			break;

		case 'I':
			if (!strcmp(tag, "Int "))
				GET_INT(ch) = num;
			else if (!strcmp(tag, "Invs"))
				GET_INVIS_LEV(ch) = num;
// shapirus
			else if (!strcmp(tag, "Ignr"))
				load_ignores(ch, line);
			break;

		case 'K':
			if (!strcmp (tag, "Kin "))
			    GET_KIN(ch) = num;
   			else if (!strcmp(tag, "Karm"))
			    KARMA(ch) = fbgetstring(fl);
			break;
		case 'L':
			if (!strcmp(tag, "LogL")) {
				i = 0;
				struct logon_data * cur_log = 0;
				long  lnum,lnum2;
				do {
					fbgetline(fl, line);
					sscanf(line, "%s %ld %ld", &buf[0], &lnum, &lnum2);
					if ( buf[0] != '~' ) {
						if (i == 0)
							cur_log = LOGON_LIST(ch) = new (struct logon_data);
						else
						{
							cur_log->next = new (struct logon_data);
							cur_log = cur_log->next;
						}
						// Добавляем в список.
						cur_log->ip = strdup(buf);
						cur_log->count = lnum;
						cur_log->lasttime = lnum2;
						cur_log->next = 0;   // Терминатор списка
						i++;
					}
					else break;
				} while (true);
			}
// Gunner
			else if (!strcmp(tag, "Logs")) {
				sscanf(line, "%d %d", &num, &num2);
				if (num >= 0 && num < NLOG)
					GET_LOGS(ch)[num] = num2;
			}
			break;

		case 'M':
			if (!strcmp(tag, "Mana")) {
				sscanf(line, "%d/%d", &num, &num2);
				GET_MEM_COMPLETED(ch) = num;
				GET_MEM_TOTAL(ch) = num2;
			} else if (!strcmp(tag, "Move")) {
				sscanf(line, "%d/%d", &num, &num2);
				GET_MOVE(ch) = num;
				GET_MAX_MOVE(ch) = num2;
			} else if (!strcmp(tag, "MutD")) {
				MUTE_DURATION(ch) = lnum;
				while (line[i] && a_isspace(line[i]))
					++i;
				if (line[i])
					MUTE_REASON(ch) = strcpy((char *) malloc(strlen(line + i) + 1), line + i);
			} else if (!strcmp(tag, "Mobs")) {
				do {
					if (!fbgetline(fl, line))
						break;
					if (*line == '~')
						break;
					sscanf(line, "%d %d", &num, &num2);
					inc_kill_vnum(ch, num, num2);
				} while (true);
			}
			break;

		case 'N':
			if (!strcmp(tag, "NmI "))
				GET_PAD(ch, 0) = str_dup(line);
			else if (!strcmp(tag, "NmR "))
				GET_PAD(ch, 1) = str_dup(line);
			else if (!strcmp(tag, "NmD "))
				GET_PAD(ch, 2) = str_dup(line);
			else if (!strcmp(tag, "NmV "))
				GET_PAD(ch, 3) = str_dup(line);
			else if (!strcmp(tag, "NmT "))
				GET_PAD(ch, 4) = str_dup(line);
			else if (!strcmp(tag, "NmP "))
				GET_PAD(ch, 5) = str_dup(line);
			else if (!strcmp(tag, "NamD"))
				NAME_DURATION(ch) = lnum;
			else if (!strcmp(tag, "NamG"))
				NAME_GOD(ch) = num;
			else if (!strcmp(tag, "NaID"))
				NAME_ID_GOD(ch) = lnum;
			break;

		case 'O':
			if (!strcmp(tag, "Olc "))
				GET_OLC_ZONE(ch) = num;
			break;


		case 'P':
			if (!strcmp(tag, "Pass"))
				strcpy(GET_PASSWD(ch), line);
			else if (!strcmp(tag, "Plyd"))
				ch->player.time.played = num;
			else if (!strcmp(tag, "PfIn"))
				POOFIN(ch) = str_dup(line);
			else if (!strcmp(tag, "PfOt"))
				POOFOUT(ch) = str_dup(line);
			else if (!strcmp(tag, "Pref"))
				asciiflag_conv(line, &PRF_FLAGS(ch, 0));
			else if (!strcmp(tag, "Pkil")) {
				do {
					if (!fbgetline(fl, line))
						break;
					if (*line == '~')
						break;
					sscanf(line, "%ld %d", &lnum, &num);

					if (lnum < 0 || !correct_unique(lnum))
						continue;
					struct PK_Memory_type * pk_one = NULL;
					for (pk_one = ch->pk_list; pk_one; pk_one = pk_one->next)
						if (pk_one->unique == lnum)
							break;
					if (pk_one) {
						log("SYSERROR: duplicate entry pkillers data for %d %s", id, filename);
						continue;
					}

					CREATE(pk_one, struct PK_Memory_type, 1);
					pk_one->unique = lnum;
					pk_one->kill_num = num;
					pk_one->next = ch->pk_list;
					ch->pk_list = pk_one;
				} while (true);
			} else if (!strcmp(tag, "PK  "))
				IS_KILLER(ch) = lnum;
			else if (!strcmp(tag, "Prtl"))
				add_portal_to_char(ch, num);
			// Loads Here new punishment strings
			else if (!strcmp(tag, "PMut"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				MUTE_DURATION(ch)=lnum;
				GET_MUTE_LEV(ch)= num2;
				MUTE_GODID(ch)=lnum3;
				MUTE_REASON(ch)=str_dup(buf);
			}
			else if (!strcmp(tag, "PHel"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				HELL_DURATION(ch)=lnum;
				GET_HELL_LEV(ch)= num2;
				HELL_GODID(ch)=lnum3;
				HELL_REASON(ch)=str_dup(buf);
			}
			else if (!strcmp(tag, "PDum"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				DUMB_DURATION(ch)=lnum;
				GET_DUMB_LEV(ch)= num2;
				DUMB_GODID(ch)=lnum3;
				DUMB_REASON(ch)=str_dup(buf);
			}
			else if (!strcmp(tag, "PNam"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				NAME_DURATION(ch)=lnum;
				GET_NAME_LEV(ch)= num2;
				NAME_GODID(ch)=lnum3;
				NAME_REASON(ch)=str_dup(buf);
			}
			else if (!strcmp(tag, "PFrz"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				FREEZE_DURATION(ch)=lnum;
				GET_FREEZE_LEV(ch)= num2;
				FREEZE_GODID(ch)=lnum3;
				FREEZE_REASON(ch)=str_dup(buf);
			}
			else if (!strcmp(tag, "PGcs"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				GCURSE_DURATION(ch)=lnum;
				GET_GCURSE_LEV(ch)= num2;
				GCURSE_GODID(ch)=lnum3;
				GCURSE_REASON(ch)=str_dup(buf);
			}
			else if (!strcmp(tag, "PUnr"))
			{
				sscanf(line, "%ld %d %ld %[^~]", &lnum, &num2, &lnum3, &buf[0]);
				UNREG_DURATION(ch)=lnum;
				GET_UNREG_LEV(ch)= num2;
				UNREG_GODID(ch)=lnum3;
				UNREG_REASON(ch)=str_dup(buf);
			}

			break;

		case 'Q':
			if (!strcmp(tag, "Qst "))
				set_quested(ch, num);
			break;

		case 'R':
			if (!strcmp(tag, "Room"))
				GET_LOADROOM(ch) = num;
			else if (!strcmp(tag, "Reli"))
				GET_RELIGION(ch) = num;
			else if (!strcmp(tag, "Race"))
				GET_RACE(ch) = num;
			else if (!strcmp(tag, "Rank"))
				GET_HOUSE_RANK(ch) = num;
			else if (!strcmp(tag, "Rcps")) {
				im_rskill *last = NULL;
				for (;;) {
					im_rskill *rs;
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num < 0)
						break;
					num = im_get_recipe(num);
// +newbook.patch (Alisher)
					if (num < 0 || imrecipes[num].classknow[(int) GET_CLASS(ch)] != KNOW_RECIPE)
// -newbook.patch (Alisher)
						continue;
					CREATE(rs, im_rskill, 1);
					rs->rid = num;
					rs->perc = num2;
					rs->link = NULL;
					if (last)
						last->link = rs;
					else
						GET_RSKILL(ch) = rs;
					last = rs;
				}
			}
			break;

		case 'S':
			if (!strcmp(tag, "Size"))
				GET_SIZE(ch) = num;
			else if (!strcmp(tag, "Sex "))
				GET_SEX(ch) = num;
			else if (!strcmp(tag, "Skil")) {
				do {
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0)
						if(skill_info[num].classknow[(int) GET_KIN (ch) ][(int) GET_CLASS(ch)] == KNOW_SKILL)
							GET_SKILL(ch, num) = num2;
				}
				while (num != 0);
			} else if (!strcmp(tag, "SkTm")) {
				do {
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0) {
						timed.skill = num;
						timed.time = num2;
						timed_to_char(ch, &timed);
					}
				}
				while (num != 0);
			} else if (!strcmp(tag, "Spel")) {
				do {
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0 && spell_info[num].name)
						GET_SPELL_TYPE(ch, num) = num2;
				}
				while (num != 0);
			} else if (!strcmp(tag, "SpMe")) {
				do {
					fbgetline(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if (num != 0)
						GET_SPELL_MEM(ch, num) = num2;
				}
				while (num != 0);
			} else if (!strcmp(tag, "Str "))
				GET_STR(ch) = num;
			else if (!strcmp(tag, "StrL"))
				STRING_LENGTH(ch) = num;
			else if (!strcmp(tag, "StrW"))
				STRING_WIDTH(ch) = num;
			break;

		case 'T':
			if (!strcmp(tag, "Thir"))
				GET_COND(ch, THIRST) = num;
			else if (!strcmp(tag, "Titl"))
				GET_TITLE(ch) = str_dup(line);
			break;

		case 'W':
			if (!strcmp(tag, "Wate"))
				GET_WEIGHT(ch) = num;
			else if (!strcmp(tag, "Wimp"))
				GET_WIMP_LEV(ch) = num;
			else if (!strcmp(tag, "Wis "))
				GET_WIS(ch) = num;
			break;

		default:
			sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	}

	/* affect_total(ch);  */
	/* initialization for imms */
	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		for (i = 1; i <= MAX_SKILLS; i++)
			GET_SKILL(ch, i) = 100;
		GET_COND(ch, FULL) = -1;
		GET_COND(ch, THIRST) = -1;
		GET_COND(ch, DRUNK) = -1;
		GET_LOADROOM(ch) = NOWHERE;
	}

	/* Set natural features - added by Gorrah */
	for (i = 1; i < MAX_FEATS; i++)
		if (can_get_feat(ch, i) && feat_info[i].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
			SET_FEAT(ch, i);

	if (IS_GRGOD(ch)) {
		for (i = 0; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(ch, i) = GET_SPELL_TYPE(ch, i) |
			    SPELL_ITEMS | SPELL_KNOW | SPELL_RUNES | SPELL_SCROLL | SPELL_POTION | SPELL_WAND;
	} else if (!IS_IMMORTAL(ch)) {
		for (i = 0; i <= MAX_SPELLS; i++) {
			if (spell_info[i].slot_forc[(int) GET_CLASS (ch)][(int) GET_KIN (ch)] == MAX_SLOT)
				REMOVE_BIT(GET_SPELL_TYPE(ch, i), SPELL_KNOW);
// shapirus: изученное не убираем на всякий случай, но из мема выкидываем,
// если мортов мало
			if (GET_REMORT(ch) < MIN_CAST_REM(spell_info[i],ch))
				GET_SPELL_MEM(ch, i) = 0;
		}
	}

	/*
	 * If you're not poisioned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	if (!AFF_FLAGGED(ch, AFF_POISON) && (((long) (time(0) - LAST_LOGON(ch))) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(ch) = GET_REAL_MAX_HIT(ch);
		GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);
	} else
		GET_HIT(ch) = MIN(GET_HIT(ch), GET_REAL_MAX_HIT(ch));
	fbclose(fl);

	return (id);
}


// по умолчанию reboot = 0 (пользуется только при ребуте)
int load_char(const char *name, CHAR_DATA * char_element, bool reboot)
{
	int player_i;

	player_i = load_char_ascii(name, char_element, reboot);
	if (player_i > -1) {
		GET_PFILEPOS(char_element) = player_i;
	}
	return (player_i);
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
	int i, pos, found = TRUE;

	if (top_of_p_table == -1) {	/* no table */
		CREATE(player_table, struct player_index_element, 1);
		pos = top_of_p_table = 0;
		found = FALSE;
	} else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
		i = ++top_of_p_table + 1;
		RECREATE(player_table, struct player_index_element, i);
		pos = top_of_p_table;
		found = FALSE;
	}

	CREATE(player_table[pos].name, char, strlen(name) + 1);

	/* copy lowercase equivalent of name to table field */
	for (i = 0, player_table[pos].name[i] = '\0'; (player_table[pos].name[i] = LOWER(name[i])); i++);
	/* create new save activity */
	player_table[pos].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
	player_table[pos].timer = NULL;

	return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature                     *
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
	char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
	register char *point;
	int done = 0, length = 0, templength;

	*buf = '\0';

	do {
		if (!fgets(tmp, 512, fl)) {
			log("SYSERR: fread_string: format error at or near %s", error);
			exit(1);
		}
		/* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
		if ((point = strchr(tmp, '~')) != NULL) {
			/* Два символа '~' подряд интерпретируются как '~' и
			   строка продолжается.
			   Можно не использовать.
			   Позволяет писать в триггерах что-то типа
			   wat 26000 wechoaround %actor% ~%actor.name% появил%actor.u% тут в клубах дыма.
			   Чтобы такая строва правильно загрузилась, в trg файле следует указать два символа '~'
			   wat 26000 wechoaround %actor% ~~%actor.name% появил%actor.u% тут в клубах дыма.
			 */
			if (point[1] != '~') {
				*point = '\0';
				done = 1;
			} else {
				memmove(point, point + 1, strlen(point));
			}
		} else {
			point = tmp + strlen(tmp) - 1;
			*(point++) = '\r';
			*(point++) = '\n';
			*point = '\0';
		}

		templength = strlen(tmp);

		if (length + templength >= MAX_STRING_LENGTH) {
			log("SYSERR: fread_string: string too large (db.c)");
			log(error);
			exit(1);
		} else {
			strcat(buf + length, tmp);
			length += templength;
		}
	}
	while (!done);

	/* allocate space for the new string and copy it */
	if (strlen(buf) > 0) {
		CREATE(rslt, char, length + 1);
		strcpy(rslt, buf);
	} else
		rslt = NULL;
	return (rslt);
}


/* release memory allocated for a char struct */
void free_char(CHAR_DATA * ch)
{
	int i, j, id = -1;
	struct alias_data *a;
	struct helper_data_type *temp;

	log("[FREE CHAR] (%s)", GET_NAME(ch));

	if (!IS_NPC(ch)) {
		id = get_ptable_by_name(GET_NAME(ch));
		if (id >= 0) {
			player_table[id].level = (GET_REMORT(ch) ? 30 : GET_LEVEL(ch));
			player_table[id].last_logon = time(0);
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {	/* if this is a player, or a non-prototyped non-player, free all */
		if (GET_NAME(ch))
			free(GET_NAME(ch));

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(ch, j))
				free(GET_PAD(ch, j));

		if (ch->player.title)
			free(ch->player.title);

		if (ch->player.short_descr)
			free(ch->player.short_descr);

		if (ch->player.long_descr)
			free(ch->player.long_descr);

		if (ch->player.description)
			free(ch->player.description);

		if (IS_NPC(ch) && ch->mob_specials.Questor)
			free(ch->mob_specials.Questor);

		if (ch->Questing.quests)
			free(ch->Questing.quests);

		free_mkill(ch);

		pk_free_list(ch);

		while (ch->helpers)
			REMOVE_FROM_LIST(ch->helpers, ch->helpers, next_helper);
	} else if ((i = GET_MOB_RNUM(ch)) >= 0) {	/* otherwise, free strings only if the string is not pointing at proto */
		if (ch->player.name && ch->player.name != mob_proto[i].player.name)
			free(ch->player.name);

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(ch, j)
			    && (ch->player.PNames[j] != mob_proto[i].player.PNames[j]))
				free(ch->player.PNames[j]);

		if (ch->player.title && ch->player.title != mob_proto[i].player.title)
			free(ch->player.title);

		if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
			free(ch->player.short_descr);

		if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
			free(ch->player.long_descr);

		if (ch->player.description && ch->player.description != mob_proto[i].player.description)
			free(ch->player.description);

		if (ch->mob_specials.Questor && ch->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(ch->mob_specials.Questor);
	}

	supress_godsapply = TRUE;
	while (ch->affected)
		affect_remove(ch, ch->affected);
	supress_godsapply = FALSE;

	while (ch->timed)
		timed_from_char(ch, ch->timed);

	if (ch->desc)
		ch->desc->character = NULL;

	if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
		while ((a = GET_ALIASES(ch)) != NULL) {
			GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
			free_alias(a);
		}
		if (ch->player_specials->poofin)
			free(ch->player_specials->poofin);
		if (ch->player_specials->poofout)
			free(ch->player_specials->poofout);
		/* рецепты */
		while (GET_RSKILL(ch) != NULL) {
			im_rskill *r;
			r = GET_RSKILL(ch)->link;
			free(GET_RSKILL(ch));
			GET_RSKILL(ch) = r;
		}
		/* порталы */
		while (GET_PORTALS(ch) != NULL) {
			struct char_portal_type *prt_next;
			prt_next = GET_PORTALS(ch)->next;
			free(GET_PORTALS(ch));
			GET_PORTALS(ch) = prt_next;
		}
// Cleanup punish reasons
		if (MUTE_REASON(ch))
			free(MUTE_REASON(ch));
		if (DUMB_REASON(ch))
			free(DUMB_REASON(ch));
		if (HELL_REASON(ch))
			free(HELL_REASON(ch));
		if (FREEZE_REASON(ch))
			free(FREEZE_REASON(ch));
		if (NAME_REASON(ch))
			free(NAME_REASON(ch));
// End reasons cleanup

		if (KARMA(ch))
			free(KARMA(ch));

		if (GET_LAST_ALL_TELL(ch))
			free(GET_LAST_ALL_TELL(ch));
		free(GET_LOGS(ch));
// shapirus: подчистим за криворукуми кодерами memory leak,
// вызванный неосвобождением фильтра базара...
		if (EXCHANGE_FILTER(ch))
			free(EXCHANGE_FILTER(ch));
		EXCHANGE_FILTER(ch) = NULL;	// на всякий случай
// ...а заодно и игнор лист *смущ :)
		while (IGNORE_LIST(ch)) {
			struct ignore_data *ign_next;
			ign_next = IGNORE_LIST(ch)->next;
			free(IGNORE_LIST(ch));
			IGNORE_LIST(ch) = ign_next;
		}
		IGNORE_LIST(ch) = NULL;

		// пока без деструктора чистим сами
		CLAN(ch).reset();
		CLAN_MEMBER(ch).reset();
		if (GET_CLAN_STATUS(ch))
			free(GET_CLAN_STATUS(ch));

		// Чистим лист логонов
		while (LOGON_LIST(ch)) {
			struct logon_data *log_next;
			log_next = LOGON_LIST(ch)->next;
//			free(LOGON_LIST(ch));
                        free(ch->player_specials->logons->ip);
                        delete LOGON_LIST(ch);
			LOGON_LIST(ch) = log_next;
		}
		LOGON_LIST(ch) = NULL;

		free(ch->player_specials);
		ch->player_specials = NULL;	// чтобы словить ACCESS VIOLATION !!!
		if (IS_NPC(ch))
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
	};

	free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(OBJ_DATA * obj)
{
	int nr, i;
	EXTRA_DESCR_DATA *thisd, *next_one, *tmp, *tmp_next;

	if ((nr = GET_OBJ_RNUM(obj)) == -1) {
		if (obj->name)
			free(obj->name);

		for (i = 0; i < NUM_PADS; i++)
			if (obj->PNames[i])
				free(obj->PNames[i]);

		if (obj->description)
			free(obj->description);

		if (obj->short_description)
			free(obj->short_description);

		if (obj->action_description)
			free(obj->action_description);

		if (obj->ex_description)
			for (thisd = obj->ex_description; thisd; thisd = next_one) {
				next_one = thisd->next;
				if (thisd->keyword)
					free(thisd->keyword);
				if (thisd->description)
					free(thisd->description);
				free(thisd);
			}
	} else {
		if (obj->name && obj->name != obj_proto[nr]->name)
			free(obj->name);

		for (i = 0; i < NUM_PADS; i++)
			if (obj->PNames[i] && obj->PNames[i] != obj_proto[nr]->PNames[i])
				free(obj->PNames[i]);

		if (obj->description && obj->description != obj_proto[nr]->description)
			free(obj->description);

		if (obj->short_description && obj->short_description != obj_proto[nr]->short_description)
			free(obj->short_description);

		if (obj->action_description && obj->action_description != obj_proto[nr]->action_description)
			free(obj->action_description);

		if (obj->ex_description && obj->ex_description != obj_proto[nr]->ex_description)
			for (thisd = obj->ex_description; thisd; thisd = next_one) {
				next_one = thisd->next;
				i = 0;
				for (tmp = obj_proto[nr]->ex_description; tmp; tmp = tmp_next) {
					tmp_next = tmp->next;
					if (tmp == thisd) {
						i = 1;
						break;
					}
				}
				if (i)
					continue;
				if (thisd->keyword)
					free(thisd->keyword);
				if (thisd->description)
					free(thisd->description);
				free(thisd);
			}
	}
	delete obj;
}


/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int file_to_string_alloc(const char *name, char **buf)
{
	char temp[MAX_EXTEND_LENGTH];
	DESCRIPTOR_DATA *in_use;

	for (in_use = descriptor_list; in_use; in_use = in_use->next)
		if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
			return (-1);

	/* Lets not free() what used to be there unless we succeeded. */
	if (file_to_string(name, temp) < 0)
		return (-1);

	if (*buf)
		free(*buf);

	*buf = str_dup(temp);
	return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
	FILE *fl;
	char tmp[READ_SIZE + 3];

	*buf = '\0';

	if (!(fl = fopen(name, "r"))) {
		log("SYSERR: reading %s: %s", name, strerror(errno));
		return (-1);
	}
	do {
		fgets(tmp, READ_SIZE, fl);
		tmp[strlen(tmp) - 1] = '\0';	/* take off the trailing \n */
		strcat(tmp, "\r\n");

		if (!feof(fl)) {
			if (strlen(buf) + strlen(tmp) + 1 > MAX_EXTEND_LENGTH) {
				log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
				*buf = '\0';
				return (-1);
			}
			strcat(buf, tmp);
		}
	}
	while (!feof(fl));

	fclose(fl);

	return (0);
}



/* clear some of the the working variables of a char */
void reset_char(CHAR_DATA * ch)
{
	int i;

	for (i = 0; i < NUM_WEARS; i++)
		GET_EQ(ch, i) = NULL;
	memset((void *) &ch->add_abils, 0, sizeof(struct char_played_ability_data));

	ch->followers = NULL;
	ch->master = NULL;
	ch->in_room = NOWHERE;
	ch->carrying = NULL;
	ch->next = NULL;
	ch->next_fighting = NULL;
	ch->next_in_room = NULL;
	ch->Protecting = NULL;
	ch->Touching = NULL;
	ch->BattleAffects = clear_flags;
	ch->Poisoner = 0;
	FIGHTING(ch) = NULL;
	ch->char_specials.position = POS_STANDING;
	ch->mob_specials.default_pos = POS_STANDING;
	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;

	if (GET_HIT(ch) <= 0)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) <= 0)
		GET_MOVE(ch) = 1;
	if (!IS_MANA_CASTER(ch)) {
		MemQ_init(ch);
	}

	GET_CASTER(ch) = 0;
	GET_DAMAGE(ch) = 0;
	GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(CHAR_DATA * ch)
{
	memset((char *) ch, 0, sizeof(CHAR_DATA));
	ch->in_room = NOWHERE;
	GET_PFILEPOS(ch) = -1;
	GET_MOB_RNUM(ch) = NOBODY;
	GET_WAS_IN(ch) = NOWHERE;
	GET_POS(ch) = POS_STANDING;
	GET_CASTER(ch) = 0;
	GET_DAMAGE(ch) = 0;
	MemQ_flush(ch);
	ch->Poisoner = 0;
	ch->mob_specials.default_pos = POS_STANDING;
}


void clear_char_skills(CHAR_DATA * ch)
{
	int i;
	ch->real_abils.Feats.reset();
	for (i = 0; i < MAX_SKILLS + 1; i++)
		ch->real_abils.Skills[i] = 0;
	for (i = 0; i < MAX_SPELLS + 1; i++)
		ch->real_abils.SplKnw[i] = 0;
	for (i = 0; i < MAX_SPELLS + 1; i++)
		ch->real_abils.SplMem[i] = 0;
}

/* initialize a new character only if class is set */
void init_char(CHAR_DATA * ch)
{
	int i, start_room = NOWHERE;

	/* create a player_special structure */
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, struct player_special_data, 1);
#ifdef TEST_BUILD
	if (top_of_p_table == 0) GET_LEVEL(ch) = LVL_IMPL; // При собирании через make test первый чар в маде становится иммом 34
#endif
	start_room = calc_loadroom(ch);
	set_title(ch, NULL);
	GET_PORTALS(ch) = NULL;
	CREATE(GET_LOGS(ch), int, NLOG);
	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;
	ch->player.description = NULL;
	ch->player.hometown = 1;
	ch->player.time.birth = time(0);
	ch->player.time.played = 0;
	ch->player.time.logon = time(0);

	for (i = 0; i < MAX_TONGUE; i++)
		GET_TALK(ch, i) = 0;

	/* make favors for sex */
	if (ch->player.sex == SEX_MALE) {
		ch->player.weight = number(120, 180);
		ch->player.height = number(160, 200);
	} else {
		ch->player.weight = number(100, 160);
		ch->player.height = number(150, 180);
	}

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 82;
	ch->points.move = GET_MAX_MOVE(ch);
	ch->real_abils.armor = 100;

	if ((i = get_ptable_by_name(GET_NAME(ch))) != -1) {
		player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
		player_table[i].unique = GET_UNIQUE(ch) = create_unique();
		player_table[i].level = 0;
		player_table[i].last_logon = -1;
	} else {
		log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));
	}

	for (i = 1; i <= MAX_SKILLS; i++) {
		if (GET_LEVEL(ch) < LVL_GRGOD)
			SET_SKILL(ch, i, 0);
		else
			SET_SKILL(ch, i, 100);
	}
	for (i = 1; i <= MAX_SPELLS; i++) {
		if (GET_LEVEL(ch) < LVL_GRGOD)
			GET_SPELL_TYPE(ch, i) = 0;
		else
			GET_SPELL_TYPE(ch, i) = SPELL_KNOW;
	}

	ch->char_specials.saved.affected_by = clear_flags;
	for (i = 0; i < SAVING_COUNT; i++)
		GET_SAVE(ch, i) = 0;
	for (i = 0; i < MAX_NUMBER_RESISTANCE; i++)
		GET_RESIST(ch, i) = 0;

	if (GET_LEVEL(ch) == LVL_IMPL) {
		ch->real_abils.intel = 25;
		ch->real_abils.wis = 25;
		ch->real_abils.dex = 25;
		ch->real_abils.str = 25;
		ch->real_abils.con = 25;
		ch->real_abils.cha = 25;
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++) {
		GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : i == DRUNK ? 0 : 24);
	}
	GET_LASTIP(ch)[0] = 0;
	GET_LOADROOM(ch) = start_room;
	SET_BIT(PLR_FLAGS(ch, PLR_LOADROOM), PLR_LOADROOM);
	SET_BIT(PRF_FLAGS(ch, PRF_DISPHP), PRF_DISPHP);
	SET_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
	SET_BIT(PRF_FLAGS(ch, PRF_DISPEXITS), PRF_DISPEXITS);
	SET_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
	SET_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
	SET_BIT(PRF_FLAGS(ch, PRF_DISPFIGHT), PRF_DISPFIGHT);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_SUMMONABLE), PRF_SUMMONABLE);
	STRING_LENGTH(ch) = 80;
	STRING_WIDTH(ch) = 25;
	// новому игроку вываливать все новости/мессаги на доске как непроченные не имеет смысла
	GENERAL_BOARD_DATE(ch) = time(0);
	NEWS_BOARD_DATE(ch) = time(0);
	IDEA_BOARD_DATE(ch) = time(0);
	ERROR_BOARD_DATE(ch) = time(0);
	GODNEWS_BOARD_DATE(ch) = time(0);
	GODGENERAL_BOARD_DATE(ch) = time(0);
	GODBUILD_BOARD_DATE(ch) = time(0);
	GODCODE_BOARD_DATE(ch) = time(0);
	GODPUNISH_BOARD_DATE(ch) = time(0);
	PERS_BOARD_DATE(ch) = time(0);
	CLAN_BOARD_DATE(ch) = time(0);
	CLANNEWS_BOARD_DATE(ch) = time(0);

	save_char(ch, NOWHERE);
}

const char *remort_msg =
    "  Если Вы так настойчивы в желании начать все заново -\r\n" "наберите <перевоплотиться> полностью.\r\n";

ACMD(do_remort)
{
	char filename[MAX_INPUT_LENGTH];
	int i, load_room = NOWHERE;
	struct helper_data_type *temp;
	const char *remort_msg2 = "$n вспыхнул$g ослепительным пламенем и пропал$g!\r\n";


	if (IS_NPC(ch) || IS_IMMORTAL(ch)) {
		send_to_char("Вам это, похоже, совсем ни к чему.\r\n", ch);
		return;
	}
//  if (!GET_GOD_FLAG(ch, GF_REMORT))
	if (GET_EXP(ch) < level_exp(ch, LVL_IMMORT) - 1) {
		send_to_char("ЧАВО ???\r\n", ch);
		return;
	}
	if (RENTABLE(ch)) {
		send_to_char("Вы не можете перевоплотиться в связи с боевыми действиями.\r\n", ch);
		return;
	}
	if (!subcmd) {
		send_to_char(remort_msg, ch);
		return;
	}

	log("Remort %s", GET_NAME(ch));
	get_filename(GET_NAME(ch), filename, PQUESTS_FILE);
	remove(filename);
	get_filename(GET_NAME(ch), filename, PMKILL_FILE);
	remove(filename);

	act(remort_msg2, FALSE, ch, 0, 0, TO_ROOM);

	GET_REMORT(ch)++;
	CLR_GOD_FLAG(ch, GF_REMORT);
	GET_STR(ch) += 1;
	GET_CON(ch) += 1;
	GET_DEX(ch) += 1;
	GET_INT(ch) += 1;
	GET_WIS(ch) += 1;
	GET_CHA(ch) += 1;

	if (FIGHTING(ch))
		stop_fighting(ch, TRUE);

	die_follower(ch);

	ch->Questing.count = 0;
	free_mkill(ch);
	delete_mkill_file(GET_NAME(ch));

	if (ch->Questing.quests) {
		free(ch->Questing.quests);
		ch->Questing.quests = 0;
	}

	while (ch->helpers)
		REMOVE_FROM_LIST(ch->helpers, ch->helpers, next_helper);

	supress_godsapply = TRUE;
	while (ch->affected)
		affect_remove(ch, ch->affected);
	supress_godsapply = FALSE;

/*  for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch,i))
         extract_obj(unequip_char(ch,i));
  for (obj = ch->carrying; obj; obj = nobj)
      {nobj = obj->next_content;
       extract_obj(obj);
      }*/
// Снимаем весь стафф
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
			obj_to_char(unequip_char(ch, i), ch);

	while (ch->timed)
		timed_from_char(ch, ch->timed);
	for (i = 1; i <= MAX_SKILLS; i++)
		SET_SKILL(ch, i, 0);
	for (i = 1; i <= MAX_SPELLS; i++) {
		GET_SPELL_TYPE(ch, i) = (GET_CLASS(ch) == CLASS_DRUID ? SPELL_RUNES : 0);
		GET_SPELL_MEM(ch, i) = 0;
	}
	while (ch->timed_feat)
		timed_feat_from_char(ch, ch->timed_feat);
	for (i = 1; i < MAX_FEATS; i++)
		UNSET_FEAT(ch, i);

	GET_HIT(ch) = GET_MAX_HIT(ch) = 10;
	GET_MOVE(ch) = GET_MAX_MOVE(ch) = 82;
	GET_MEM_TOTAL(ch) = GET_MEM_COMPLETED(ch) = 0;
	GET_LEVEL(ch) = 0;
	GET_WIMP_LEV(ch) = 0;
	//GET_GOLD(ch)      = GET_BANK_GOLD(ch) = 0;
	GET_AC(ch) = 100;
	GET_LOADROOM(ch) = calc_loadroom(ch);
	SET_BIT(PLR_FLAGS(ch, PLR_LOADROOM), PLR_LOADROOM);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_SUMMONABLE), PRF_SUMMONABLE);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_AWAKE), PRF_AWAKE);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_PUNCTUAL), PRF_PUNCTUAL);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_POWERATTACK), PRF_POWERATTACK);
	REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATPOWERATTACK), PRF_GREATPOWERATTACK);
	// Убираем все заученные порталы
	check_portals(ch);

	do_start(ch, FALSE);
	save_char(ch, NOWHERE);
	if (PLR_FLAGGED(ch, PLR_HELLED))
		load_room = r_helled_start_room;
	else if (PLR_FLAGGED(ch, PLR_NAMED))
		load_room = r_named_start_room;
	else if (PLR_FLAGGED(ch, PLR_FROZEN))
		load_room = r_frozen_start_room;
	else {
		if ((load_room = GET_LOADROOM(ch)) == NOWHERE)
			load_room = calc_loadroom(ch);
		load_room = real_room(load_room);
	}
	if (load_room == NOWHERE) {
		if (GET_LEVEL(ch) >= LVL_IMMORT)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}
	char_from_room(ch);
	char_to_room(ch, load_room);
	look_at_room(ch, 0);
	SET_BIT(PLR_FLAGS(ch, PLR_NODELETE), PLR_NODELETE);
	act("$n вступил$g в игру.", TRUE, ch, 0, 0, TO_ROOM);
	act("Вы перевоплотились ! Желаем удачи !", FALSE, ch, 0, 0, TO_CHAR);
}


/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
	room_rnum bot, top, mid;

	bot = 1;		// 0 - room is NOWHERE
	top = top_of_world;
	/* perform binary search on world-table */
	for (;;) {
		mid = (bot + top) / 2;

		if (world[mid]->number == vnum)
			return (mid);
		if (bot >= top)
			return (NOWHERE);
		if (world[mid]->number > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
	mob_rnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	/* perform binary search on mob-table */
	for (;;) {
		mid = (bot + top) / 2;

		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}



/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
	obj_rnum bot, top, mid;

	bot = 0;
	top = top_of_objt;

	/* perform binary search on obj-table */
	for (;;) {
		mid = (bot + top) / 2;

		if ((obj_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((obj_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(OBJ_DATA * obj)
{
	int error = FALSE;

	if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
		log("SYSERR: Object #%d (%s) has negative weight (%d).",
		    GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

	if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
		log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
		    GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
	if (strstr(buf, "UNDEFINED") && (error = TRUE))
		log("SYSERR: Object #%d (%s) has unknown wear flags.", GET_OBJ_VNUM(obj), obj->short_description);

	sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ",");
	if (strstr(buf, "UNDEFINED") && (error = TRUE))
		log("SYSERR: Object #%d (%s) has unknown extra flags.", GET_OBJ_VNUM(obj), obj->short_description);

	sprintbits(obj->obj_flags.affects, affected_bits, buf, ",");

	if (strstr(buf, "UNDEFINED") && (error = TRUE))
		log("SYSERR: Object #%d (%s) has unknown affection flags.", GET_OBJ_VNUM(obj), obj->short_description);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_DRINKCON:
//  { char onealias[MAX_INPUT_LENGTH], *space = strchr(obj->name, ' ');
//
//    int offset = space ? space - obj->name : strlen(obj->name);
//
//    strncpy(onealias, obj->name, offset);
//    onealias[offset] = '\0';
//
//    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
//       log("SYSERR: Object #%d (%s) doesn't have drink type as first alias. (%s)",
//                   GET_OBJ_VNUM(obj), obj->short_description, obj->name);
//  }
		/* Fall through. */
	case ITEM_FOUNTAIN:
		if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
			log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
			    GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		error |= check_object_level(obj, 0);
		error |= check_object_spell_number(obj, 1);
		error |= check_object_spell_number(obj, 2);
		error |= check_object_spell_number(obj, 3);
		break;
	case ITEM_BOOK:
		error |= check_object_spell_number(obj, 1);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		error |= check_object_level(obj, 0);
		error |= check_object_spell_number(obj, 3);
		if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
			log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
			    GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
		break;
	}

	return (error);
}

int check_object_spell_number(OBJ_DATA * obj, int val)
{
	int error = FALSE;
	const char *spellname;

	if (GET_OBJ_VAL(obj, val) == -1)	/* i.e.: no spell */
		return (error);

	/*
	 * Check for negative spells, spells beyond the top define, and any
	 * spell which is actually a skill.
	 */
	if (GET_OBJ_VAL(obj, val) < 0)
		error = TRUE;
	if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
		error = TRUE;
	if (error)
		log("SYSERR: Object #%d (%s) has out of range spell #%d.",
		    GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

	/*
	 * This bug has been fixed, but if you don't like the special behavior...
	 */
#if 0
	if (GET_OBJ_TYPE(obj) == ITEM_STAFF && HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
		log("... '%s' (#%d) uses %s spell '%s'.",
		    obj->short_description, GET_OBJ_VNUM(obj),
		    HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val),
				      MAG_AREAS) ? "area" : "mass", spell_name(GET_OBJ_VAL(obj, val)));
#endif

	if (scheck)		/* Spell names don't exist in syntax check mode. */
		return (error);

	/* Now check for unnamed spells. */
	spellname = spell_name(GET_OBJ_VAL(obj, val));

	if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname))
	    && (error = TRUE))
		log("SYSERR: Object #%d (%s) uses '%s' spell #%d.", GET_OBJ_VNUM(obj),
		    obj->short_description, spellname, GET_OBJ_VAL(obj, val));
	return (error);
}

int check_object_level(OBJ_DATA * obj, int val)
{
	int error = FALSE;

	if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL)
	    && (error = TRUE))
		log("SYSERR: Object #%d (%s) has out of range level #%d.",
		    GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));
	return (error);
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
int must_be_deleted(CHAR_DATA * short_ch)
{
	int ci, timeout;

	if (IS_SET(PLR_FLAGS(short_ch, PLR_NODELETE), PLR_NODELETE))
		return (0);

	if (GET_REMORT(short_ch))
		return (0);

	if (IS_SET(PLR_FLAGS(short_ch, PLR_DELETED), PLR_DELETED))
		return (1);

	timeout = -1;
	for (ci = 0; ci == 0 || pclean_criteria[ci].level > pclean_criteria[ci - 1].level; ci++) {
		if (GET_LEVEL(short_ch) <= pclean_criteria[ci].level) {
			timeout = pclean_criteria[ci].days;
			break;
		}
	}
	if (timeout >= 0) {
		timeout *= SECS_PER_REAL_DAY;
		if ((time(0) - LAST_LOGON(short_ch)) > timeout) {
			return (1);
		}
	}

	return (0);
}


// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void entrycount(char *name)
{
	int i, deleted;
	CHAR_DATA * short_ch;
	char filename[MAX_STRING_LENGTH];

	if (get_filename(name, filename, PLAYERS_FILE)) {

		CREATE(short_ch, CHAR_DATA, 1);
		clear_char(short_ch);
		deleted = 1;
		// персонаж загружается неполностью
		if (load_char(name, short_ch, 1) > -1) {
			/* если чар удален или им долго не входили, то не создаем для него запись */
			if (!must_be_deleted(short_ch)) {
				deleted = 0;
				/* new record */
				if (player_table)
					RECREATE(player_table, struct player_index_element, top_of_p_table + 2);
				else
					CREATE(player_table, struct player_index_element, 1);
				top_of_p_file++;
				top_of_p_table++;

				CREATE(player_table[top_of_p_table].name, char, strlen(GET_NAME(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].name[i] = '\0';
				     (player_table[top_of_p_table].name[i] = LOWER(GET_NAME(short_ch)[i])); i++);
				player_table[top_of_p_table].id = GET_IDNUM(short_ch);
				player_table[top_of_p_table].unique = GET_UNIQUE(short_ch);
				player_table[top_of_p_table].level = (GET_REMORT(short_ch) ? 30 : GET_LEVEL(short_ch));
				player_table[top_of_p_table].timer = NULL;
				if (IS_SET(PLR_FLAGS(short_ch, PLR_DELETED), PLR_DELETED)) {
					player_table[top_of_p_table].last_logon = -1;
					player_table[top_of_p_table].activity = -1;
				} else {
					player_table[top_of_p_table].last_logon = LAST_LOGON(short_ch);
					player_table[top_of_p_table].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
				}
				top_idnum = MAX(top_idnum, GET_IDNUM(short_ch));
				TopPlayer::Refresh(short_ch, 1);
				log("Add new player %s", player_table[top_of_p_table].name);
			}
			free_char(short_ch);
		} else {
			free(short_ch);
		}
		/* если чар уже удален, то стираем с диска его файл */
		if (deleted) {
			log("Player %s already deleted - kill player file", name);
			remove(filename);
			// 2) Remove all other files

			get_filename(name, filename, ETEXT_FILE);
			remove(filename);

			get_filename(name, filename, ALIAS_FILE);
			remove(filename);

			get_filename(name, filename, SCRIPT_VARS_FILE);
			remove(filename);

			get_filename(name, filename, PQUESTS_FILE);
			remove(filename);

			delete_mkill_file(name);
		}
	}
	return;
}

void new_build_player_index(void)
{
	FILE *players;
	char name[MAX_INPUT_LENGTH], playername[MAX_INPUT_LENGTH];
	int c;

	player_table = NULL;
	top_of_p_file = top_of_p_table = -1;
	if (!(players = fopen(LIB_PLRS "players.lst", "r"))) {
		log("Players list empty...");
		return;
	}

	now_entrycount = TRUE;
	while (get_line(players, name)) {
		if (!*name || *name == ';')
			continue;
		if (sscanf(name, "%s ", playername) == 0)
			continue;
		for (c = 0; c <= top_of_p_table; c++)
			if (!str_cmp(playername, player_table[c].name))
				break;
		if (c <= top_of_p_table)
			continue;
		entrycount(playername);
	}
	fclose(players);
	now_entrycount = FALSE;
}

void flush_player_index(void)
{
	FILE *players;
	char name[MAX_STRING_LENGTH];
	int i;

	if (!(players = fopen(LIB_PLRS "players.lst", "w+"))) {
		log("Cann't save players list...");
		return;
	}
	for (i = 0; i <= top_of_p_table; i++) {
		if (!player_table[i].name || !*player_table[i].name)
			continue;

		// check double
		// for (c = 0; c < i; c++)
		//     if (!str_cmp(player_table[c].name, player_table[i].name))
		//         break;
		// if (c < i)
		//    continue;

		sprintf(name, "%s %ld %ld %d %d\n",
			player_table[i].name,
			player_table[i].id, player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Сохранено индексов %d (считано при загрузке %d)", i, top_of_p_file + 1);
}

void dupe_player_index(void)
{
	FILE *players;
	char name[MAX_STRING_LENGTH];
	int i, c;

	sprintf(name, LIB_PLRS "players.dup");

	if (!(players = fopen(name, "w+"))) {
		log("Cann't save players list...");
		return;
	}
	for (i = 0; i <= top_of_p_table; i++) {
		if (!player_table[i].name || !*player_table[i].name)
			continue;

		// check double
		for (c = 0; c < i; c++)
			if (!str_cmp(player_table[c].name, player_table[i].name))
				break;
		if (c < i)
			continue;

		sprintf(name, "%s %ld %ld %d %d\n",
			player_table[i].name,
			player_table[i].id, player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Продублировано индексов %d (считано при загрузке %d)", i, top_of_p_file + 1);
}


/* remove ^M's from file output */
void kill_ems(char *str)
{
	char *ptr1, *ptr2, *tmp;

	tmp = str;
	ptr1 = str;
	ptr2 = str;

	while (*ptr1) {
		if ((*(ptr2++) = *(ptr1++)) == '\r')
			if (*ptr1 == '\r')
				ptr1++;
	}
	*ptr2 = '\0';
}

void save_char(CHAR_DATA * ch, room_rnum load_room)
{
	FILE *saved;
	char filename[MAX_STRING_LENGTH];
	room_rnum location;
	int i;
	time_t li;
	AFFECT_DATA *aff, tmp_aff[MAX_AFFECT];
	OBJ_DATA *char_eq[NUM_WEARS];
	struct timed_type *skj;
	struct char_portal_type *prt;
	int tmp = time(0) - ch->player.time.logon;
	if (!now_entrycount)
		if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
			return;

	if (!PLR_FLAGGED(ch, PLR_LOADROOM)) {
		if (load_room > NOWHERE) {
			GET_LOADROOM(ch) = GET_ROOM_VNUM(load_room);
			log("Player %s save at room %d", GET_NAME(ch), GET_ROOM_VNUM(load_room));
		}
	}

	log("Save char %s", GET_NAME(ch));
	save_char_vars(ch);

/* Запись чара в новом формате */
	get_filename(GET_NAME(ch), filename, PLAYERS_FILE);
	if (!(saved = fopen(filename, "w"))) {
		perror("Unable open charfile");
		return;
	}
/* подготовка */
	/* снимаем все возможные аффекты  */
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)) {
			char_eq[i] = unequip_char(ch, i | 0x80 | 0x40);
#ifndef NO_EXTRANEOUS_TRIGGERS
			remove_otrigger(char_eq[i], ch);
#endif
		} else
			char_eq[i] = NULL;
	}

	for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
		if (aff) {
			if (aff->type == SPELL_ARMAGEDDON || aff->type < 1 || aff->type > LAST_USED_SPELL)
				i--;
			else {
				tmp_aff[i] = *aff;
				tmp_aff[i].next = 0;
			}
			aff = aff->next;
		} else {
			tmp_aff[i].type = 0;	/* Zero signifies not used */
			tmp_aff[i].duration = 0;
			tmp_aff[i].modifier = 0;
			tmp_aff[i].location = 0;
			tmp_aff[i].bitvector = 0;
			tmp_aff[i].next = 0;
		}
	}

	/*
	 * remove the affections so that the raw values are stored; otherwise the
	 * effects are doubled when the char logs back in.
	 */
	supress_godsapply = TRUE;
	while (ch->affected)
		affect_remove(ch, ch->affected);
	supress_godsapply = FALSE;

	if ((i >= MAX_AFFECT) && aff && aff->next)
		log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

	// первыми идут поля, необходимые при ребуте мада, тут без необходимости трогать ничего не надо
	if (GET_NAME(ch))
		fprintf(saved, "Name: %s\n", GET_NAME(ch));
	fprintf(saved, "Levl: %d\n", GET_LEVEL(ch));
	fprintf(saved, "Clas: %d\n", GET_CLASS(ch));
	fprintf(saved, "UIN : %d\n", GET_UNIQUE(ch));
	fprintf(saved, "LstL: %ld\n", LAST_LOGON(ch));
	fprintf(saved, "Id  : %ld\n", GET_IDNUM(ch));
	fprintf(saved, "Exp : %ld\n", GET_EXP(ch));
	if (GET_REMORT(ch) > 0)
		fprintf(saved, "Rmrt: %d\n", GET_REMORT(ch));
	// флаги
	*buf = '\0';
	tascii(&PLR_FLAGS(ch, 0), 4, buf);
	fprintf(saved, "Act : %s\n", buf);
	// это пишем обязательно посленим, потому что после него ничего не прочитается
	fprintf(saved, "Rebt: следующие далее поля при перезагрузке не парсятся\n\n");
	// дальше пишем как хотим и что хотим

	if (GET_PAD(ch, 0))
		fprintf(saved, "NmI : %s\n", GET_PAD(ch, 0));
	if (GET_PAD(ch, 0))
		fprintf(saved, "NmR : %s\n", GET_PAD(ch, 1));
	if (GET_PAD(ch, 0))
		fprintf(saved, "NmD : %s\n", GET_PAD(ch, 2));
	if (GET_PAD(ch, 0))
		fprintf(saved, "NmV : %s\n", GET_PAD(ch, 3));
	if (GET_PAD(ch, 0))
		fprintf(saved, "NmT : %s\n", GET_PAD(ch, 4));
	if (GET_PAD(ch, 0))
		fprintf(saved, "NmP : %s\n", GET_PAD(ch, 5));
	if (GET_PASSWD(ch))
		fprintf(saved, "Pass: %s\n", GET_PASSWD(ch));
	if (GET_EMAIL(ch))
		fprintf(saved, "EMal: %s\n", GET_EMAIL(ch));
	if (GET_TITLE(ch))
		fprintf(saved, "Titl: %s\n", GET_TITLE(ch));
	if (ch->player.description && *ch->player.description) {
		strcpy(buf, ch->player.description);
		kill_ems(buf);
		fprintf(saved, "Desc:\n%s~\n", buf);
	}
	if (POOFIN(ch))
		fprintf(saved, "PfIn: %s\n", POOFIN(ch));
	if (POOFOUT(ch))
		fprintf(saved, "PfOt: %s\n", POOFOUT(ch));
	fprintf(saved, "Sex : %d %s\n", GET_SEX(ch), genders[(int) GET_SEX(ch)]);
	fprintf (saved, "Kin : %d %s\n", GET_KIN (ch),kin_name[GET_KIN (ch)][(int) GET_SEX (ch)]);
	if ((location = real_room(GET_HOME(ch))) != NOWHERE)
		fprintf(saved, "Home: %d %s\n", GET_HOME(ch), world[(location)]->name);
	li = ch->player.time.birth;
	fprintf(saved, "Brth: %ld %s\n", li, ctime(&li));
	// Gunner
	// time.logon ЦАБ=---Lг- -= -Е-г ┐ёЮ-L-- - А┐АБг-Ц = time(0) -= БгLЦИ┐L ---г-Б
	//-Юг-О - АгLЦ-г=Е А -=Г=L= ┐ёЮК
	tmp += ch->player.time.played;
	fprintf(saved, "Plyd: %d\n", tmp);
	// Gunner end
	li = ch->player.time.logon;
	fprintf(saved, "Last: %ld %s\n", li, ctime(&li));
	if (ch->desc)
		strcpy(buf, ch->desc->host);
	else
		strcpy(buf, "Unknown");
	fprintf(saved, "Host: %s\n", buf);
	fprintf(saved, "Hite: %d\n", GET_HEIGHT(ch));
	fprintf(saved, "Wate: %d\n", GET_WEIGHT(ch));
	fprintf(saved, "Size: %d\n", GET_SIZE(ch));
	/* структуры */
	fprintf(saved, "Alin: %d\n", GET_ALIGNMENT(ch));
	*buf = '\0';
	tascii(&AFF_FLAGS(ch, 0), 4, buf);
	fprintf(saved, "Aff : %s\n", buf);

	/* дальше не по порядку */
	/* статсы */
	fprintf(saved, "Str : %d\n", GET_STR(ch));
	fprintf(saved, "Int : %d\n", GET_INT(ch));
	fprintf(saved, "Wis : %d\n", GET_WIS(ch));
	fprintf(saved, "Dex : %d\n", GET_DEX(ch));
	fprintf(saved, "Con : %d\n", GET_CON(ch));
	fprintf(saved, "Cha : %d\n", GET_CHA(ch));

	/* способности - added by Gorrah */
	if (GET_LEVEL(ch) < LVL_IMMORT) {
		fprintf(saved, "Feat:\n");
		for (i = 1; i < MAX_FEATS; i++) {
			if (HAVE_FEAT(ch, i))
				fprintf(saved, "%d %s\n", i, feat_info[i].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* Задержки на cпособности */
	if (GET_LEVEL(ch) < LVL_IMMORT) {
		fprintf(saved, "FtTm:\n");
		for (skj = ch->timed_feat; skj; skj = skj->next) {
			fprintf(saved, "%d %d %s\n", skj->skill, skj->time, feat_info[skj->skill].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* скилы */
	if (GET_LEVEL(ch) < LVL_IMMORT) {
		fprintf(saved, "Skil:\n");
		for (i = 1; i <= MAX_SKILLS; i++) {
			if (GET_SKILL(ch, i))
				fprintf(saved, "%d %d %s\n", i, GET_SKILL(ch, i), skill_info[i].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* Задержки на скилы */
	if (GET_LEVEL(ch) < LVL_IMMORT) {
		fprintf(saved, "SkTm:\n");
		for (skj = ch->timed; skj; skj = skj->next) {
			fprintf(saved, "%d %d %s\n", skj->skill, skj->time, skill_info[skj->skill].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* спелы */
	if (GET_LEVEL(ch) < LVL_IMPL) {
		fprintf(saved, "Spel:\n");
		for (i = 1; i <= MAX_SPELLS; i++) {
			if (GET_SPELL_TYPE(ch, i))
				fprintf(saved, "%d %d %s\n", i, GET_SPELL_TYPE(ch, i), spell_info[i].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* Замемленые спелы */
	if (GET_LEVEL(ch) < LVL_IMMORT) {
		fprintf(saved, "SpMe:\n");
		for (i = 1; i <= MAX_SPELLS; i++) {
			if (GET_SPELL_MEM(ch, i))
				fprintf(saved, "%d %d %s\n", i, GET_SPELL_MEM(ch, i), spell_info[i].name);
		}
		fprintf(saved, "0 0\n");
	}

	/* Рецепты */
//    if (GET_LEVEL(ch) < LVL_IMMORT)
	{
		im_rskill *rs;
		im_recipe *r;
		fprintf(saved, "Rcps:\n");
		for (rs = GET_RSKILL(ch); rs; rs = rs->link) {
			if (rs->perc <= 0)
				continue;
			r = &imrecipes[rs->rid];
			fprintf(saved, "%d %d %s\n", r->id, rs->perc, r->name);
		}
		fprintf(saved, "-1 -1\n");
	}

	fprintf(saved, "Hrol: %d\n", GET_HR(ch));
	fprintf(saved, "Drol: %d\n", GET_DR(ch));
	fprintf(saved, "Ac  : %d\n", GET_AC(ch));

	fprintf(saved, "Hit : %d/%d\n", GET_HIT(ch), GET_MAX_HIT(ch));
	fprintf(saved, "Mana: %d/%d\n", GET_MEM_COMPLETED(ch), GET_MEM_TOTAL(ch));
	fprintf(saved, "Move: %d/%d\n", GET_MOVE(ch), GET_MAX_MOVE(ch));
	fprintf(saved, "Gold: %d\n", GET_GOLD(ch));
	fprintf(saved, "Bank: %ld\n", GET_BANK_GOLD(ch));
	fprintf(saved, "PK  : %ld\n", IS_KILLER(ch));

	fprintf(saved, "Wimp: %d\n", GET_WIMP_LEV(ch));
	fprintf(saved, "Frez: %d\n", GET_FREEZE_LEV(ch));
	fprintf(saved, "Invs: %d\n", GET_INVIS_LEV(ch));
	fprintf(saved, "Room: %d\n", GET_LOADROOM(ch));

	fprintf(saved, "Badp: %d\n", GET_BAD_PWS(ch));

	fprintf(saved, "Br01: %ld\n", GENERAL_BOARD_DATE(ch));
	fprintf(saved, "Br02: %ld\n", NEWS_BOARD_DATE(ch));
	fprintf(saved, "Br03: %ld\n", IDEA_BOARD_DATE(ch));
	fprintf(saved, "Br04: %ld\n", ERROR_BOARD_DATE(ch));
	fprintf(saved, "Br05: %ld\n", GODNEWS_BOARD_DATE(ch));
	fprintf(saved, "Br06: %ld\n", GODGENERAL_BOARD_DATE(ch));
	fprintf(saved, "Br07: %ld\n", GODBUILD_BOARD_DATE(ch));
	fprintf(saved, "Br08: %ld\n", GODCODE_BOARD_DATE(ch));
	fprintf(saved, "Br09: %ld\n", GODPUNISH_BOARD_DATE(ch));
	fprintf(saved, "Br10: %ld\n", PERS_BOARD_DATE(ch));
	fprintf(saved, "Br11: %ld\n", CLAN_BOARD_DATE(ch));
	fprintf(saved, "Br12: %ld\n", CLANNEWS_BOARD_DATE(ch));

	if (GET_LEVEL(ch) < LVL_IMMORT)
		fprintf(saved, "Hung: %d\n", GET_COND(ch, FULL));
	if (GET_LEVEL(ch) < LVL_IMMORT)
		fprintf(saved, "Thir: %d\n", GET_COND(ch, THIRST));
	if (GET_LEVEL(ch) < LVL_IMMORT)
		fprintf(saved, "Drnk: %d\n", GET_COND(ch, DRUNK));

	fprintf(saved, "Reli: %d %s\n", GET_RELIGION(ch), religion_name[GET_RELIGION(ch)][(int) GET_SEX(ch)]);
	fprintf(saved, "Race: %d %s\n", GET_RACE(ch), race_name[GET_RACE(ch)][(int) GET_SEX(ch)]);
	fprintf(saved, "DrSt: %d\n", GET_DRUNK_STATE(ch));
	fprintf(saved, "ComS: %d\n", GET_COMMSTATE(ch));
	fprintf(saved, "Glor: %d\n", GET_GLORY(ch));
	fprintf(saved, "Olc : %d\n", GET_OLC_ZONE(ch));
	*buf = '\0';
	tascii(&PRF_FLAGS(ch, 0), 4, buf);
	fprintf(saved, "Pref: %s\n", buf);
	if (GET_HOUSE_UID(ch) != 0)
		fprintf(saved, "HsID: %ld\n", GET_HOUSE_UID(ch));
	if (GET_HOUSE_RANK(ch) != 0)
		fprintf(saved, "Rank: %d\n", GET_HOUSE_RANK(ch));

	if (MUTE_DURATION(ch) > 0)
		fprintf(saved, "PMut: %ld %d %ld %s~\n", MUTE_DURATION(ch), GET_MUTE_LEV(ch), MUTE_GODID(ch), MUTE_REASON(ch));
	if (NAME_DURATION(ch) > 0)
		fprintf(saved, "PNam: %ld %d %ld %s~\n", NAME_DURATION(ch), GET_NAME_LEV(ch), NAME_GODID(ch), NAME_REASON(ch));
	if (DUMB_DURATION(ch) > 0)
		fprintf(saved, "PDum: %ld %d %ld %s~\n", DUMB_DURATION(ch), GET_DUMB_LEV(ch), DUMB_GODID(ch), DUMB_REASON(ch));
	if (HELL_DURATION(ch) > 0)
		fprintf(saved, "PHel: %ld %d %ld %s~\n", HELL_DURATION(ch), GET_HELL_LEV(ch), HELL_GODID(ch), HELL_REASON(ch));
	if (GCURSE_DURATION(ch) > 0)
		fprintf(saved, "PGcs: %ld %d %ld %s~\n", GCURSE_DURATION(ch), GET_GCURSE_LEV(ch), GCURSE_GODID(ch), GCURSE_REASON(ch));
	if (FREEZE_DURATION(ch) > 0)
		fprintf(saved, "PFrz: %ld %d %ld %s~\n", FREEZE_DURATION(ch), GET_FREEZE_LEV(ch), FREEZE_GODID(ch), FREEZE_REASON(ch));
	if (UNREG_DURATION(ch) > 0)
		fprintf(saved, "PUnr: %ld %d %ld %s~\n", UNREG_DURATION(ch), GET_UNREG_LEV(ch), UNREG_GODID(ch), UNREG_REASON(ch));


	if (KARMA(ch) > 0) {
		strcpy(buf, KARMA(ch));
		kill_ems(buf);
		fprintf(saved, "Karm:\n%s~\n", buf);
	}
	if (LOGON_LIST(ch) > 0) {
		log("Saving logon list.");
		struct logon_data * next_log = LOGON_LIST(ch);
		buf[0] = 0;
		while (next_log)
		{
			buf1[0] = 0;
			sprintf(buf1,"%s %ld %ld\n",next_log->ip , next_log->count, next_log->lasttime);
			sprintf(buf,"%s%s",buf,buf1);
			next_log = next_log->next;
		}
		fprintf(saved, "LogL:\n%s~\n",buf);
	}
	fprintf(saved, "GdFl: %ld\n", ch->player_specials->saved.GodsLike);
	fprintf(saved, "NamG: %d\n", NAME_GOD(ch));
	fprintf(saved, "NaID: %ld\n", NAME_ID_GOD(ch));
	fprintf(saved, "StrL: %d\n", STRING_LENGTH(ch));
	fprintf(saved, "StrW: %d\n", STRING_WIDTH(ch));
	if (EXCHANGE_FILTER(ch))
		fprintf(saved, "ExFl: %s\n", EXCHANGE_FILTER(ch));

	// shapirus: игнор лист
	{
		struct ignore_data *cur = IGNORE_LIST(ch);
		if (cur) {
			for (; cur; cur = cur->next) {
				if (!cur->id)
					continue;
				fprintf(saved, "Ignr: [%ld]%ld\n", cur->mode, cur->id);
			}
		}
	}

	/* affected_type */
	if (tmp_aff[0].type > 0) {
		fprintf(saved, "Affs:\n");
		for (i = 0; i < MAX_AFFECT; i++) {
			aff = &tmp_aff[i];
			if (aff->type)
				fprintf(saved, "%d %d %d %d %d %s\n", aff->type, aff->duration,
					aff->modifier, aff->location, (int) aff->bitvector, spell_name(aff->type));
		}
		fprintf(saved, "0 0 0 0 0\n");
	}

	/* порталы */
	for (prt = GET_PORTALS(ch); prt; prt = prt->next) {
		fprintf(saved, "Prtl: %d\n", prt->vnum);
	}
	for (i = 0; i < NLOG; ++i) {
		if (!GET_LOGS(ch)) {
			log("SYSERR: Saving NULL logs for char %s", GET_NAME(ch));
			break;
		}
		fprintf(saved, "Logs: %d %d\n", i, GET_LOGS(ch)[i]);
	}
	/* Квесты */
	if (ch->Questing.quests) {
		for (i = 0; i < ch->Questing.count; i++) {
			fprintf(saved, "Qst : %d\n", *(ch->Questing.quests + i));
		}
	}

	save_mkill(ch, saved);
	save_pkills(ch, saved);

	fclose(saved);

	/* восстанавливаем аффекты */
	/* add spell and eq affections back in now */
	for (i = 0; i < MAX_AFFECT; i++) {
		if (tmp_aff[i].type)
			affect_to_char(ch, &tmp_aff[i]);
	}

	for (i = 0; i < NUM_WEARS; i++) {
		if (char_eq[i]) {
#ifndef NO_EXTRANEOUS_TRIGGERS
			if (wear_otrigger(char_eq[i], ch, i))
#endif
				equip_char(ch, char_eq[i], i | 0x80 | 0x40);
#ifndef NO_EXTRANEOUS_TRIGGERS
			else
				obj_to_char(char_eq[i], ch);
#endif
		}
	}
	affect_total(ch);

	if ((i = get_ptable_by_name(GET_NAME(ch))) >= 0) {
		log("[CHAR TO STORE] Change logon time");
		player_table[i].last_logon = -1;
		player_table[i].level = GET_LEVEL(ch);
	}

}

void rename_char(CHAR_DATA * ch, char *oname)
{
	char filename[MAX_INPUT_LENGTH], ofilename[MAX_INPUT_LENGTH];

	// 1) Rename(if need) char and pkill file - directly
	log("Rename char %s->%s", GET_NAME(ch), oname);
	get_filename(oname, ofilename, PLAYERS_FILE);
	get_filename(GET_NAME(ch), filename, PLAYERS_FILE);
	rename(ofilename, filename);

	save_char(ch, GET_LOADROOM(ch));

	// 2) Rename all other files
	get_filename(oname, ofilename, CRASH_FILE);
	get_filename(GET_NAME(ch), filename, CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, TEXT_CRASH_FILE);
	get_filename(GET_NAME(ch), filename, TEXT_CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, TIME_CRASH_FILE);
	get_filename(GET_NAME(ch), filename, TIME_CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, ETEXT_FILE);
	get_filename(GET_NAME(ch), filename, ETEXT_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, ALIAS_FILE);
	get_filename(GET_NAME(ch), filename, ALIAS_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, SCRIPT_VARS_FILE);
	get_filename(GET_NAME(ch), filename, SCRIPT_VARS_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, PQUESTS_FILE);
	get_filename(GET_NAME(ch), filename, PQUESTS_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, PMKILL_FILE);
	get_filename(GET_NAME(ch), filename, PMKILL_FILE);
	rename(ofilename, filename);
}

void delete_char(char *name)
{
	CHAR_DATA *st;
	int id;

	CREATE(st, CHAR_DATA, 1);
	clear_char(st);
	if ((id = load_char(name, st)) >= 0) {
		// 1) Mark char as deleted
		SET_BIT(PLR_FLAGS(st, PLR_DELETED), PLR_DELETED);

		// выносим из листа неодобренных имен, если есть
		NewNameRemove(st);

		save_char(st, GET_LOADROOM(st));
		extract_char(st, FALSE);

		Crash_clear_objects(id);
		player_table[id].unique = -1;
		player_table[id].level = -1;
		player_table[id].last_logon = -1;
		player_table[id].activity = -1;
	} else
		free(st);
}

void room_copy(ROOM_DATA * dst, ROOM_DATA * src)
/*++
   Функция делает создает копию комнаты.
   После вызова этой функции создается полностью независимая копия комнты src
   за исключением полей track, contents, people.
   Все поля имеют те же значения, но занимают свои области памяти.
      dst - "чистый" указатель на структуру room_data.
      src - исходная комната
   Примечание: Неочищенный указатель dst приведет к утечке памяти.
               Используйте redit_room_free() для очистки содержимого комнаты
--*/
{
	int i;
	EXTRA_DESCR_DATA **pddd, *sdd;

	{
		// Сохраняю track, contents, people, аффекты
		struct track_data *track = dst->track;
		OBJ_DATA *contents = dst->contents;
		CHAR_DATA *people = dst->people;

		// Копирую все поверх
		*dst = *src;

		// Восстанавливаю track, contents, people, аффекты
		dst->track = track;
		dst->contents = contents;
		dst->people = people;
	}

	// Теперь нужно выделить собственные области памяти

	// Название и описание
	dst->name = str_dup(src->name ? src->name : "неопределено");
	dst->temp_description = 0; // так надо

	// Выходы и входы
	for (i = 0; i < NUM_OF_DIRS; ++i) {
		EXIT_DATA *rdd;
		if ((rdd = src->dir_option[i]) != NULL) {
			CREATE(dst->dir_option[i], EXIT_DATA, 1);
			// Копируем числа
			*dst->dir_option[i] = *rdd;
			// Выделяем память
			dst->dir_option[i]->general_description =
			    (rdd->general_description ? str_dup(rdd->general_description) : NULL);
			dst->dir_option[i]->keyword = (rdd->keyword ? str_dup(rdd->keyword) : NULL);
			dst->dir_option[i]->vkeyword = (rdd->vkeyword ? str_dup(rdd->vkeyword) : NULL);
		}
	}

	// Дополнительные описания, если есть
	pddd = &dst->ex_description;
	sdd = src->ex_description;

	while (sdd) {
		CREATE(pddd[0], EXTRA_DESCR_DATA, 1);
		pddd[0]->keyword = sdd->keyword ? strdup(sdd->keyword) : NULL;
		pddd[0]->description = sdd->description ? strdup(sdd->description) : NULL;
		pddd = &(pddd[0]->next);
		sdd = sdd->next;
	}

	// Копирую скрипт и прототипы
	SCRIPT(dst) = NULL;
	dst->proto_script = NULL;
	proto_script_copy(&dst->proto_script, src->proto_script);

	im_inglist_copy(&dst->ing_list, src->ing_list);
}


void room_free(ROOM_DATA * room)
/*++
   Функция полностью освобождает память, занимаемую данными комнаты.
   ВНИМАНИЕ. Память самой структуры room_data не освобождается.
             Необходимо дополнительно использовать free()
--*/
{
	int i;
	EXTRA_DESCR_DATA *lthis, *next;

	// Название и описание
	if (room->name)
		free(room->name);
	if (room->temp_description) {
		free(room->temp_description);
		room->temp_description = 0;
	}

	// Выходы и входы
	for (i = 0; i < NUM_OF_DIRS; i++)
		if (room->dir_option[i]) {
			if (room->dir_option[i]->general_description)
				free(room->dir_option[i]->general_description);
			if (room->dir_option[i]->keyword)
				free(room->dir_option[i]->keyword);
			if (room->dir_option[i]->vkeyword)
				free(room->dir_option[i]->vkeyword);
			free(room->dir_option[i]);
		}
	// Дополнительные описания
	for (lthis = room->ex_description; lthis; lthis = next) {
		next = lthis->next;
		if (lthis->keyword)
			free(lthis->keyword);
		if (lthis->description)
			free(lthis->description);
		free(lthis);
	}

	// Прототип
	proto_script_free(room->proto_script);
	// Скрипт
	free_script(SCRIPT(room));

	if (room->ing_list) {
		free(room->ing_list);
		room->ing_list = NULL;
	}

	AFFECT_DATA *af,*next_af;
	for (af = room->affected; af; af = next_af)
	{
		next_af = af->next;
		free(af);
	}
	room->affected = NULL;
}

void LoadGlobalUID(void)
{
	FILE *guid;
	char buffer[256];

	global_uid = 0;

	if (!(guid = fopen(LIB_MISC "globaluid", "r"))) {
		log("Can't open global uid file...");
		return;
	}
	get_line(guid, buffer);
	global_uid = atoi(buffer);
	fclose(guid);
	return;
}

void SaveGlobalUID(void)
{
	FILE *guid;

	if (!(guid = fopen(LIB_MISC "globaluid", "w"))) {
		log("Can't write global uid file...");
		return;
	}

	fprintf(guid, "%d\n", global_uid);
	fclose(guid);
	return;
}

