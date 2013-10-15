/**************************************************************************
*  File: dg_olc.cpp                                      Part of Bylins   *
*  Usage: trig edit from olc functions                                    *
*                                                                         *
*  Usage: this source file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
* Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_   *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "olc.h"
#include "dg_olc.h"
#include "dg_event.h"
#include "char.hpp"
#include "room.hpp"

// declare externally defined globals
extern INDEX_DATA **trig_index;
extern const char *trig_types[], *otrig_types[], *wtrig_types[];
extern DESCRIPTOR_DATA *descriptor_list;
extern TRIG_DATA *trigger_list;
extern int top_of_trigt;
extern struct zone_data *zone_table;

void free_script(SCRIPT_DATA * sc);

// prototype externally defined functions
void trig_data_copy(TRIG_DATA * this_data, const TRIG_DATA * trg);
void free_varlist(struct trig_var_data *vd);

void trigedit_disp_menu(DESCRIPTOR_DATA * d);
void trigedit_save(DESCRIPTOR_DATA * d);
void trigedit_create_index(int znum, const char *type);
char * indent_trigger(char * cmd , int * level);

#if 0
// copy an entire script from one holder (mob/obj/room) to another
void script_copy(void *dst, void *src, int type)
/*++
   Копирование скрипта в составе:
      - прототип скрипта (копируются с созданием нового)
      - текущий скрипт (не копируются глобальные переменные и т.д.)
         в текущем скрипте копируются types, context = 0,
         и используется trig_data_copy (новые name и arglist)
   При создании новых структур данных старые удаляются
--*/
{
	SCRIPT_DATA *s_src;
	SCRIPT_DATA **s_dst;
	TRIG_DATA *t_src, **pt_dst;

	// find the scripts of the source and destination
	switch (type)
	{
	case MOB_TRIGGER:
		s_src = SCRIPT((CHAR_DATA *) src);
		s_dst = &SCRIPT((CHAR_DATA *) dst);
		proto_script_free(((CHAR_DATA *) dst)->proto_script);
		proto_script_copy(&((CHAR_DATA *) dst)->proto_script, ((CHAR_DATA *) src)->proto_script);
		break;
	case OBJ_TRIGGER:
		s_src = SCRIPT((OBJ_DATA *) src);
		s_dst = &SCRIPT((OBJ_DATA *) dst);
		proto_script_free(((OBJ_DATA *) dst)->proto_script);
		proto_script_copy(&((OBJ_DATA *) dst)->proto_script, ((OBJ_DATA *) src)->proto_script);
		break;
	case WLD_TRIGGER:
		s_src = SCRIPT((ROOM_DATA *) src);
		s_dst = &SCRIPT((ROOM_DATA *) dst);
		proto_script_free(((ROOM_DATA *) dst)->proto_script);
		proto_script_copy(&((ROOM_DATA *) dst)->proto_script, ((ROOM_DATA *) src)->proto_script);
		break;
	default:
		log("SYSERR: Unknown type code sent to script_copy()!");
		return;
	}

	// make sure the dst doesnt already have a script
	// if it does, delete it
	// free_script именно УДАЛЯЕТ со всеми потрохами
	free_script(s_dst[0]);
	CREATE(s_dst[0], SCRIPT_DATA, 1);

	// copy the scrip data
	s_dst[0]->types = s_src->types;
	s_dst[0]->context = 0;

	t_src = TRIGGERS(s_src);
	pt_dst = NULL;
	while (t_src)
	{
		pt_dst = pt_dst ? &(pt_dst[0]->next) : &TRIGGERS(s_dst[0]);
		CREATE(pt_dst[0], trig_data, 1);
		trig_data_copy(*pt_dst, t_src);
		t_src = t_src->next;
	}
}
#endif

// called when a mob or object is being saved to disk, so its script can
// be saved
void script_save_to_disk(FILE * fp, void *item, int type)
{
	struct trig_proto_list *t;

	if (type == MOB_TRIGGER)
		t = ((CHAR_DATA *) item)->proto_script;
	else if (type == OBJ_TRIGGER)
		t = ((OBJ_DATA *) item)->proto_script;
	else if (type == WLD_TRIGGER)
		t = ((ROOM_DATA *) item)->proto_script;
	else
	{
		log("SYSERR: Invalid type passed to script_save_mobobj_to_disk()");
		return;
	}

	while (t)
	{
		fprintf(fp, "T %d\n", t->vnum);
		t = t->next;
	}
}


/**************************************************************************
 *  Редактирование ТРИГГЕРОВ
 *  trigedit
 **************************************************************************/

void trigedit_setup_new(DESCRIPTOR_DATA * d)
{
	TRIG_DATA *trig;

	// Allocate a scratch trigger structure
	CREATE(trig, TRIG_DATA, 1);

	trig->nr = -1;

	// Set up some defaults
	trig->name = str_dup("new trigger");
	trig->trigger_type = MTRIG_GREET;

	// cmdlist will be a large char string until the trigger is saved
	CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
	strcpy(OLC_STORAGE(d), "say My trigger commandlist is not complete!\r\n");
	trig->narg = 100;

	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;		// Has changed flag. (It hasn't so far, we just made it.)

	trigedit_disp_menu(d);
}

void trigedit_setup_existing(DESCRIPTOR_DATA * d, int rtrg_num)
{
	TRIG_DATA *trig;
	struct cmdlist_element *c;

	// Allocate a scratch trigger structure
	CREATE(trig, TRIG_DATA, 1);

	trig_data_copy(trig, trig_index[rtrg_num]->proto);

	// convert cmdlist to a char string
	c = trig->cmdlist;
	CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
	strcpy(OLC_STORAGE(d), "");

	while (c)
	{
		strcat(OLC_STORAGE(d), c->cmd);
		strcat(OLC_STORAGE(d), "\r\n");
		c = c->next;
	}
	// now trig->cmdlist is something to pass to the text editor
	// it will be converted back to a real cmdlist_element list later

	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;		// Has changed flag. (It hasn't so far, we just made it.)

	trigedit_disp_menu(d);
}


void trigedit_disp_menu(DESCRIPTOR_DATA * d)
{
	TRIG_DATA *trig = OLC_TRIG(d);
	const char *attach_type;
	char trgtypes[256];

	get_char_cols(d->character);

	if (trig->attach_type == MOB_TRIGGER)
	{
		attach_type = "Mobiles";
		sprintbit(GET_TRIG_TYPE(trig), trig_types, trgtypes);
	}
	else if (trig->attach_type == OBJ_TRIGGER)
	{
		attach_type = "Objects";
		sprintbit(GET_TRIG_TYPE(trig), otrig_types, trgtypes);
	}
	else if (trig->attach_type == WLD_TRIGGER)
	{
		attach_type = "Rooms";
		sprintbit(GET_TRIG_TYPE(trig), wtrig_types, trgtypes);
	}
	else
	{
		attach_type = "undefined";
		trgtypes[0] = '\0';
	}

	sprintf(buf,
#if defined(CLEAR_SCREEN)
			"[H[J"
#endif
			"Trigger Editor [%s%d%s]\r\n\r\n"
			"%s1)%s Name         : %s%s\r\n"
			"%s2)%s Intended for : %s%s\r\n"
			"%s3)%s Trigger types: %s%s\r\n"
			"%s4)%s Numberic Arg : %s%d\r\n"
			"%s5)%s Arguments    : %s%s\r\n"
			"%s6)%s Commands:\r\n%s&S%s&s\r\n"
			"%sQ)%s Quit\r\n" "Enter Choice :",
			grn, OLC_NUM(d), nrm,	// vnum on the title line
			grn, nrm, yel, GET_TRIG_NAME(trig),	// name
			grn, nrm, yel, attach_type,	// attach type
			grn, nrm, yel, trgtypes,	// greet/drop/etc
			grn, nrm, yel, trig->narg,	// numeric arg
			grn, nrm, yel, trig->arglist ? trig->arglist : "",	// strict arg
			grn, nrm, cyn, OLC_STORAGE(d),	// the command list
			grn, nrm);	// quit colors

	send_to_char(buf, d->character);
	OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}

void trigedit_disp_types(DESCRIPTOR_DATA * d)
{
	int i, columns = 0;
	const char **types;

	switch (OLC_TRIG(d)->attach_type)
	{
	case WLD_TRIGGER:
		types = wtrig_types;
		break;
	case OBJ_TRIGGER:
		types = otrig_types;
		break;
	case MOB_TRIGGER:
	default:
		types = trig_types;
		break;
	}

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (i = 0; i < NUM_TRIG_TYPE_FLAGS; i++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, types[i], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), types, buf1, 2);
	sprintf(buf, "\r\nCurrent types : %s%s%s\r\nEnter type (0 to quit) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);

}

void trigedit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	int i = 0;

	switch (OLC_MODE(d))
	{
	case TRIGEDIT_MAIN_MENU:
		switch (tolower(*arg))
		{
		case 'q':
			if (OLC_VAL(d))  	// Anything been changed?
			{
				if (!GET_TRIG_TYPE(OLC_TRIG(d)))
				{
					send_to_char("Invalid Trigger Type! Answer a to abort quit!\r\n", d->character);
				}
				send_to_char("Do you wish to save the changes to the trigger? (y/n): ", d->character);
				OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
			}
			else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '1':
			OLC_MODE(d) = TRIGEDIT_NAME;
			send_to_char("Name: ", d->character);
			break;
		case '2':
			OLC_MODE(d) = TRIGEDIT_INTENDED;
			send_to_char("0: Mobiles, 1: Objects, 2: Rooms: ", d->character);
			break;
		case '3':
			OLC_MODE(d) = TRIGEDIT_TYPES;
			trigedit_disp_types(d);
			break;
		case '4':
			OLC_MODE(d) = TRIGEDIT_NARG;
			send_to_char("Numeric argument: ", d->character);
			break;
		case '5':
			OLC_MODE(d) = TRIGEDIT_ARGUMENT;
			send_to_char("Argument: ", d->character);
			break;
		case '6':
			OLC_MODE(d) = TRIGEDIT_COMMANDS;
			send_to_char("Enter trigger commands: (/s saves /h for help)\r\n\r\n", d->character);
			d->backstr = NULL;
			if (OLC_STORAGE(d))
			{
				send_to_char(d->character, "&S%s&s", OLC_STORAGE(d));
				d->backstr = str_dup(OLC_STORAGE(d));
			}
			d->str = &OLC_STORAGE(d);
			d->max_str = MAX_CMD_LENGTH;
			d->mail_to = 0;
			OLC_VAL(d) = 1;

			break;
		default:
			trigedit_disp_menu(d);
			return;
		}
		return;

	case TRIGEDIT_CONFIRM_SAVESTRING:
		switch (tolower(*arg))
		{
		case 'y':
			trigedit_save(d);
			sprintf(buf, "OLC: %s edits trigger %d", GET_NAME(d->character), OLC_NUM(d));
			olc_log("%s end trig %d", GET_NAME(d->character), OLC_NUM(d));
			mudlog(buf, NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			// fall through
		case 'n':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		case 'a':	// abort quitting
			break;
		default:
			send_to_char("Invalid choice!\r\n", d->character);
			send_to_char("Do you wish to save the trigger? : ", d->character);
			return;
		}
		break;

	case TRIGEDIT_NAME:
		if (OLC_TRIG(d)->name)
			free(OLC_TRIG(d)->name);
		OLC_TRIG(d)->name = str_dup((arg && *arg) ? arg : "undefined");
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_INTENDED:
		if ((atoi(arg) >= MOB_TRIGGER) || (atoi(arg) <= WLD_TRIGGER))
			OLC_TRIG(d)->attach_type = atoi(arg);
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_NARG:
		OLC_TRIG(d)->narg = atoi(arg);
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_ARGUMENT:
		if (OLC_TRIG(d)->arglist)
			free(OLC_TRIG(d)->arglist);
		OLC_TRIG(d)->arglist = (*arg ? str_dup(arg) : NULL);
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_TYPES:
		if ((i = atoi(arg)) == 0)
			break;
		else if (!((i < 0) || (i > NUM_TRIG_TYPE_FLAGS)))
			TOGGLE_BIT((GET_TRIG_TYPE(OLC_TRIG(d))), 1 << (i - 1));
		OLC_VAL(d)++;
		trigedit_disp_types(d);
		return;

	case TRIGEDIT_COMMANDS:
		break;

	}

	OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
	trigedit_disp_menu(d);
}

// * print out the letter codes pertaining to the bits set in 'data'
void sprintbyts(int data, char *dest)
{
	int i;
	char *p = dest;

	for (i = 0; i < 32; i++)
	{
		if (data & (1 << i))
		{
			*p = ((i <= 25) ? ('a' + i) : ('A' + i));
			p++;
		}
	}
	*p = '\0';
}


// save the zone's triggers to internal memory and to disk
void trigedit_save(DESCRIPTOR_DATA * d)
{
	int trig_rnum, i;
	int found = 0;
	char *s;
	TRIG_DATA *proto;
	TRIG_DATA *trig = OLC_TRIG(d);
	TRIG_DATA *live_trig;
	struct cmdlist_element *cmd, *next_cmd;
	INDEX_DATA **new_index;
	DESCRIPTOR_DATA *dsc;
	FILE *trig_file;
	int zone, top;
	char buf[MAX_CMD_LENGTH];
	char bitBuf[MAX_INPUT_LENGTH];
	char fname[MAX_INPUT_LENGTH];
	char logbuf[MAX_INPUT_LENGTH];


	// Recompile the command list from the new script
	s = OLC_STORAGE(d);
	olc_log("%s start trig %d:\n%s", GET_NAME(d->character), OLC_NUM(d), OLC_STORAGE(d));

	CREATE(trig->cmdlist, struct cmdlist_element, 1);
	trig->cmdlist->cmd = str_dup(strtok(s, "\n\r"));
	cmd = trig->cmdlist;

	while ((s = strtok(NULL, "\n\r")))
	{
		CREATE(cmd->next, struct cmdlist_element, 1);
		cmd = cmd->next;
		cmd->cmd = str_dup(s);
	}
	cmd->next = NULL;

	if ((trig_rnum = real_trigger(OLC_NUM(d))) != -1)
	{

		// Этот триггер уже есть.

		// Очистка прототипа
		proto = trig_index[trig_rnum]->proto;
		for (cmd = proto->cmdlist; cmd; cmd = next_cmd)
		{
			next_cmd = cmd->next;
			if (cmd->cmd)
				free(cmd->cmd);
			free(cmd);
		}

		if (proto->arglist)
			free(proto->arglist);	// Опять же, не люблю вызовов free(NULL);
		free(proto->name);

		// make the prorotype look like what we have
		trig_data_copy(proto, trig);

		// go through the mud and replace existing triggers

		live_trig = trigger_list;
		while (live_trig)
		{
			if (GET_TRIG_RNUM(live_trig) == trig_rnum)
			{
				TRIG_DATA *t;
				if (live_trig->arglist)
				{
					free(live_trig->arglist);
					live_trig->arglist = NULL;
				}
				if (live_trig->name)
				{
					free(live_trig->name);
					live_trig->name = NULL;
				}
				// Предотвратить возможный вызов триггера по wait
				if (GET_TRIG_WAIT(live_trig))
				{
					free(GET_TRIG_WAIT(live_trig)->info);	// Причина уже обсуждалась
					remove_event(GET_TRIG_WAIT(live_trig));
				}
				free_varlist(live_trig->var_list);

				// сохранить ссылку на следующий
				t = live_trig->next;
				trig_data_copy(live_trig, proto);
				live_trig->next = t;
			}

			live_trig = live_trig->next_in_world;
		}

	}
	else
	{
		// this is a new trigger
		CREATE(new_index, INDEX_DATA *, top_of_trigt + 2);

		for (i = 0; i < top_of_trigt; i++)
		{
			if (!found)
			{
				if (trig_index[i]->vnum > OLC_NUM(d))
				{
					found = TRUE;
					trig_rnum = i;

					CREATE(new_index[trig_rnum], INDEX_DATA, 1);
					GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
					new_index[trig_rnum]->vnum = OLC_NUM(d);
					new_index[trig_rnum]->number = 0;
					new_index[trig_rnum]->func = NULL;
					CREATE(proto, TRIG_DATA, 1);
					new_index[trig_rnum]->proto = proto;

					trig_data_copy(proto, trig);

					--i;
					continue;	// повторить копирование еще раз, но уже по-другому
				}
				else
				{
					new_index[i] = trig_index[i];
				}
			}
			else
			{
				// докопировать
				new_index[i + 1] = trig_index[i];
				proto = trig_index[i]->proto;
				proto->nr = i + 1;
			}
		}

		if (!found)
		{
			trig_rnum = i;
			CREATE(new_index[trig_rnum], INDEX_DATA, 1);
			GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
			new_index[trig_rnum]->vnum = OLC_NUM(d);
			new_index[trig_rnum]->number = 0;
			new_index[trig_rnum]->func = NULL;

			CREATE(proto, TRIG_DATA, 1);
			new_index[trig_rnum]->proto = proto;
			trig_data_copy(proto, trig);
		}

		free(trig_index);
		trig_index = new_index;
		top_of_trigt++;

		// HERE IT HAS TO GO THROUGH AND FIX ALL SCRIPTS/TRIGS OF HIGHER RNUM
		for (live_trig = trigger_list; live_trig; live_trig = live_trig->next_in_world)
			if (GET_TRIG_RNUM(live_trig) > trig_rnum)
				GET_TRIG_RNUM(live_trig)++;

		// * Update other trigs being edited.
		for (dsc = descriptor_list; dsc; dsc = dsc->next)
			if (dsc->connected == CON_TRIGEDIT)
				if (GET_TRIG_RNUM(OLC_TRIG(dsc)) >= trig_rnum)
					GET_TRIG_RNUM(OLC_TRIG(dsc))++;

	}

	// now write the trigger out to disk, along with the rest of the  //
	// triggers for this zone, of course                              //
	// note: we write this to disk NOW instead of letting the builder //
	// have control because if we lose this after having assigned a   //
	// new trigger to an item, we will get SYSERR's upton reboot that //
	// could make things hard to debug.                               //

	zone = zone_table[OLC_ZNUM(d)].number;
	top = zone_table[OLC_ZNUM(d)].top;

#ifdef CIRCLE_MAC
	sprintf(fname, "%s:%i.new", TRG_PREFIX, zone);
#else
	sprintf(fname, "%s/%i.new", TRG_PREFIX, zone);
#endif

	if (!(trig_file = fopen(fname, "w")))
	{
		sprintf(logbuf, "SYSERR: OLC: Can't open trig file \"%s\"", fname);
		mudlog(logbuf, BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
		return;
	}

	for (i = zone * 100; i <= top; i++)
	{
		if ((trig_rnum = real_trigger(i)) != -1)
		{
			trig = trig_index[trig_rnum]->proto;

			if (fprintf(trig_file, "#%d\n", i) < 0)
			{
				sprintf(logbuf, "SYSERR: OLC: Can't write trig file!");
				mudlog(logbuf, BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
				fclose(trig_file);
				return;
			}
			sprintbyts(GET_TRIG_TYPE(trig), bitBuf);
			fprintf(trig_file, "%s~\n"
					"%d %s %d\n"
					"%s~\n",
					(GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) :
					"unknown trigger", trig->attach_type, bitBuf,
					GET_TRIG_NARG(trig), GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "");

			// Build the text for the script
			int lev = 0;
			strcpy(buf, "");
			for (cmd = trig->cmdlist; cmd; cmd = cmd->next)
			{
				// Indenting
				cmd->cmd = indent_trigger(cmd->cmd, &lev);
				strcat(buf, cmd->cmd);
				strcat(buf, "\n");
			}
			if (lev > 0)
			{
				send_to_char(d->character,
					"WARNING: Positive indent-level on trigger #%d end.\r\n", i);
			}

			if (!buf[0])
				strcpy(buf, "* Empty script");
			else
			{
				char *p;
				// замена одиночного '~' на '~~'
				p = strtok(buf, "~");
				fprintf(trig_file, "%s", p);
				while ((p = strtok(NULL, "~")) != NULL)
				{
					fprintf(trig_file, "~~%s", p);
				}
				fprintf(trig_file, "~\n");
			}

//      fprintf(trig_file, "%s~\n", buf );

			*buf = '\0';
		}
	}

	fprintf(trig_file, "$~\n");
	fclose(trig_file);

#ifdef CIRCLE_MAC
	sprintf(buf, "%s:%d.trg", TRG_PREFIX, zone);
#else
	sprintf(buf, "%s/%d.trg", TRG_PREFIX, zone);
#endif

	remove(buf);
	rename(fname, buf);

	send_to_char("Saving Index file\r\n", d->character);
	trigedit_create_index(zone, "trg");
}


void trigedit_create_index(int znum, const char *type)
{
	FILE *newfile, *oldfile;
	char new_name[32], old_name[32];
	int num, found = FALSE;

	const char *prefix = TRG_PREFIX;

	sprintf(old_name, "%s/index", prefix);
	sprintf(new_name, "%s/newindex", prefix);

	if (!(oldfile = fopen(old_name, "r")))
	{
		sprintf(buf1, "SYSERR: DG_OLC: Failed to open %s", buf);
		mudlog(buf1, BRF, LVL_IMPL, SYSLOG, TRUE);
		return;
	}
	else if (!(newfile = fopen(new_name, "w")))
	{
		sprintf(buf1, "SYSERR: DG_OLC: Failed to open %s", buf);
		mudlog(buf1, BRF, LVL_IMPL, SYSLOG, TRUE);
		return;
	}

	// Index contents must be in order: search through the old file for the
	// right place, insert the new file, then copy the rest over.
	sprintf(buf1, "%d.%s", znum, type);
	while (get_line(oldfile, buf))
	{
		if (*buf == '$')
		{
			fprintf(newfile, "%s\n$\n", (!found ? buf1 : "$"));
			break;
		}
		else if (!found)
		{
			sscanf(buf, "%d", &num);
			if (num == znum)
				found = TRUE;
			else if (num > znum)
			{
				found = TRUE;
				fprintf(newfile, "%s\n", buf1);
			}
		}
		fprintf(newfile, "%s\n", buf);
	}

	fclose(newfile);
	fclose(oldfile);

	// * Out with the old, in with the new.
	remove(old_name);
	rename(new_name, old_name);
}


//****************************************************************


/**************************************************************************
 *  Редактирование ПРОТОТИПОВ СКРИПТОВ
 *  trigedit
 **************************************************************************/


void dg_olc_script_free(DESCRIPTOR_DATA * d)
//   Удаление прототипа в OLC_SCRIPT
{
	proto_script_free(OLC_SCRIPT(d));
	OLC_SCRIPT(d) = NULL;
}


void dg_olc_script_copy(DESCRIPTOR_DATA * d)
//   Создание копии прототипа скрипта для текщего редактируемого моба/объекта/комнаты
{
	struct trig_proto_list *origscript;

	if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
		origscript = OLC_MOB(d)->proto_script;
	else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
		origscript = OLC_OBJ(d)->proto_script;
	else
		origscript = OLC_ROOM(d)->proto_script;

	// OLC_SCRIPT должен быть NULL (наверное так и есть)
	proto_script_copy(&OLC_SCRIPT(d), origscript);
}


void dg_script_menu(DESCRIPTOR_DATA * d)
{
	struct trig_proto_list *editscript;
	int i = 0;

	// make sure our input parser gets used
	OLC_MODE(d) = OLC_SCRIPT_EDIT;
	OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;


#if defined(CLEAR_SCREEN)	// done wierd to compile with the vararg send()
#define FMT    "[H[J     Script Editor\r\n\r\n     Trigger List:\r\n"
#else
#define FMT    "     Script Editor\r\n\r\n     Trigger List:\r\n"
#endif
	send_to_char(FMT, d->character);
#undef FMT

	editscript = OLC_SCRIPT(d);
	while (editscript)
	{
		sprintf(buf, "     %2d) [%s%d%s] %s%s%s", ++i, cyn,
				editscript->vnum, nrm, cyn, trig_index[real_trigger(editscript->vnum)]->proto->name, nrm);
		send_to_char(buf, d->character);
		if (trig_index[real_trigger(editscript->vnum)]->proto->attach_type != OLC_ITEM_TYPE(d))
			sprintf(buf, "   %s** Mis-matched Trigger Type **%s\r\n", grn, nrm);
		else
			sprintf(buf, "\r\n");
		send_to_char(buf, d->character);

		editscript = editscript->next;
	}
	if (i == 0)
		send_to_char("     <none>\r\n", d->character);

	sprintf(buf, "\r\n"
			" %sN%s)  New trigger for this script\r\n"
			" %sD%s)  Delete a trigger in this script\r\n"
			" %sX%s)  Exit Script Editor\r\n"
			" %sQ%s)  Quit Script Editor (no save) \r\n\r\n"
			"     Enter choice :", grn, nrm, grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf, d->character);
}

int dg_script_edit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	struct trig_proto_list *trig, *currtrig;
	int count, pos, vnum;

	switch (OLC_SCRIPT_EDIT_MODE(d))
	{
	case SCRIPT_MAIN_MENU:
		switch (tolower(*arg))
		{
		case 'x':
			if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
			{
				trig = OLC_MOB(d)->proto_script;
				OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
			}
			else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
			{
				trig = OLC_OBJ(d)->proto_script;
				OLC_OBJ(d)->proto_script = OLC_SCRIPT(d);
			}
			else
			{
				trig = OLC_ROOM(d)->proto_script;
				OLC_ROOM(d)->proto_script = OLC_SCRIPT(d);
			}
			OLC_SCRIPT(d) = trig;
			// тут break не нужен
		case 'q':
			dg_olc_script_free(d);
			return 0;
		case 'n':
			send_to_char("\r\nPlease enter position, vnum   (ex: 1, 200):", d->character);
			OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
			break;
		case 'd':
			send_to_char("     Which entry should be deleted?  0 to abort :", d->character);
			OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
			break;
		default:
			dg_script_menu(d);
			break;
		}
		return 1;

	case SCRIPT_NEW_TRIGGER:
		vnum = -1;
		count = sscanf(arg, "%d, %d", &pos, &vnum);
		if (count == 1)
		{
			vnum = pos;
			pos = 999;
		}

		if (pos <= 0)
			break;	// this aborts a new trigger entry

		if (vnum == 0)
			break;	// this aborts a new trigger entry

		if (real_trigger(vnum) < 0)
		{
			send_to_char("Invalid Trigger VNUM!\r\n"
						 "Please enter position, vnum   (ex: 1, 200):", d->character);
			return 1;
		}

		// add the new info in position
		currtrig = OLC_SCRIPT(d);
		CREATE(trig, struct trig_proto_list, 1);
		trig->vnum = vnum;

		if (pos == 1 || !currtrig)
		{
			trig->next = OLC_SCRIPT(d);
			OLC_SCRIPT(d) = trig;
		}
		else
		{
			while (currtrig->next && --pos)
			{
				currtrig = currtrig->next;
			}
			trig->next = currtrig->next;
			currtrig->next = trig;
		}
		OLC_VAL(d)++;
		break;

	case SCRIPT_DEL_TRIGGER:
		pos = atoi(arg);
		if (pos <= 0)
			break;

		if (pos == 1 && OLC_SCRIPT(d))
		{
			OLC_VAL(d)++;
			currtrig = OLC_SCRIPT(d);
			OLC_SCRIPT(d) = currtrig->next;
			free(currtrig);
			break;
		}

		pos--;
		currtrig = OLC_SCRIPT(d);
		while (--pos && currtrig)
			currtrig = currtrig->next;
		// now curtrig points one before the target
		if (currtrig && currtrig->next)
		{
			OLC_VAL(d)++;
			trig = currtrig->next;
			currtrig->next = trig->next;
			free(trig);
		}
		break;
	}

	dg_script_menu(d);
	return 1;
}
