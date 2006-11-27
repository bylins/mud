/* ************************************************************************
*  File: dg_db_scripts.cpp                                 Part of Bylins *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                   *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "dg_event.h"
#include "comm.h"
#include "spells.h"
#include "skills.h"
#include "im.h"
#include "features.hpp"

void trig_data_copy(TRIG_DATA * this_data, const TRIG_DATA * trg);
void trig_data_free(TRIG_DATA * this_data);

extern INDEX_DATA **trig_index;
extern int top_of_trigt;

extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;

extern void half_chop(char *string, char *arg1, char *arg2);
extern void asciiflag_conv(const char *flag, void *value);

int check_recipe_items(CHAR_DATA * ch, int spellnum, int spelltype, int extract);
int check_recipe_values(CHAR_DATA * ch, int spellnum, int spelltype, int showrecipe);

char * indent_trigger(char * cmd , int * level)
{
	// Удаляем впереди идущие пробелы.
	char * ptr, * tmp;
	if (level < 0) level = 0;

	int currlev,nextlev;
	currlev = nextlev = *level;

	if (!cmd) return cmd;
		
	ptr = cmd;
	skip_spaces(&ptr);

	// ptr содержит строку без первых пробелов. 
	if (!strn_cmp("case ", ptr , 5) || !strn_cmp("if ", ptr , 3) || 
	    !strn_cmp("while ", ptr , 6) || !strn_cmp("switch ", ptr, 7) || 
	    !strn_cmp("foreach ", ptr, 8) || !strn_cmp("default", ptr , 7) ) {
		// Увеличиваем уровень вложения
		nextlev++;
	} if (!strn_cmp("elseif ", ptr, 7) || !strn_cmp("else", ptr, 4))
	{
		currlev--;
	}
	else if (!strn_cmp("else", ptr, 4) || !strn_cmp("else", ptr, 4) || 
		!strn_cmp("break", ptr, 5) || !strn_cmp("done", ptr, 4) || 
		!strn_cmp("end", ptr, 3))
	{
		nextlev--;
		currlev--;
	}
	
	if (nextlev < 0 ) nextlev = 0;
	if (currlev < 0 ) currlev = 0;
	
	// Вставляем дополнительные пробелы 

	tmp = (char *) malloc(currlev*2+1);
	memset(tmp, 0x20, currlev*2);
	tmp[currlev*2] = '\0';
	
	tmp = str_add(tmp, ptr);

	cmd = (char *)realloc(cmd, strlen(tmp)+1);
	cmd = strcpy(cmd, tmp);

	free (tmp);

        *level = nextlev;
	return cmd;
}

void parse_trigger(FILE * trig_f, int nr)
{
	int t[2], k, attach_type, indlev;

	char line[256], *cmds, flags[256], *s;
	struct cmdlist_element *cle;
	index_data *index;
	TRIG_DATA *trig;

	CREATE(trig, trig_data, 1);
	CREATE(index, index_data, 1);

	index->vnum = nr;
	index->number = 0;
	index->func = NULL;
	index->proto = trig;

	sprintf(buf2, "trig vnum %d", nr);

	trig->nr = top_of_trigt;
	trig->name = fread_string(trig_f, buf2);

	get_line(trig_f, line);
	k = sscanf(line, "%d %s %d", &attach_type, flags, t);
	trig->attach_type = (byte) attach_type;
	asciiflag_conv(flags, &trig->trigger_type);
	trig->narg = (k == 3) ? t[0] : 0;

	trig->arglist = fread_string(trig_f, buf2);

	s = cmds = fread_string(trig_f, buf2);

	CREATE(trig->cmdlist, struct cmdlist_element, 1);

	trig->cmdlist->cmd = str_dup(strtok(s, "\n\r"));

	indlev = 0;	
	trig->cmdlist->cmd = indent_trigger(trig->cmdlist->cmd,&indlev);

	cle = trig->cmdlist;

	while ((s = strtok(NULL, "\n\r"))) {
		CREATE(cle->next, struct cmdlist_element, 1);
		cle = cle->next;
		cle->cmd = str_dup(s);
		cle->cmd = indent_trigger(cle->cmd,&indlev);
	}
	if (indlev > 0) log("Positive indent-level on trigger #%d end.",nr);

	free(cmds);

	trig_index[top_of_trigt++] = index;
}


/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
TRIG_DATA *read_trigger(int nr)
{
	index_data *index;
	TRIG_DATA *trig;

	if (nr >= top_of_trigt)
		return NULL;
	if ((index = trig_index[nr]) == NULL)
		return NULL;

	CREATE(trig, trig_data, 1);
	trig_data_copy(trig, index->proto);

	index->number++;

	return trig;
}


/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd)
{
	struct trig_var_data *i, *j;

	for (i = vd; i;) {
		j = i;
		i = i->next;
		if (j->name)
			free(j->name);
		if (j->value)
			free(j->value);
		free(j);
	}
}


/* release memory allocated for a script */
void free_script(SCRIPT_DATA * sc)
{
	if (sc == NULL)
		return;

	extract_script(sc);

	free_varlist(sc->global_vars);

	free(sc);
}

void trig_data_init(TRIG_DATA * this_data)
{
	this_data->nr = NOTHING;
	this_data->data_type = 0;
	this_data->name = NULL;
	this_data->trigger_type = 0;
	this_data->cmdlist = NULL;
	this_data->curr_state = NULL;
	this_data->narg = 0;
	this_data->arglist = NULL;
	this_data->depth = 0;
	this_data->wait_event = NULL;
	this_data->purged = FALSE;
	this_data->var_list = NULL;

	this_data->next = NULL;
}


void trig_data_copy(TRIG_DATA * this_data, const TRIG_DATA * trg)
{
	trig_data_init(this_data);

	this_data->nr = trg->nr;
	this_data->attach_type = trg->attach_type;
	this_data->data_type = trg->data_type;
	this_data->name = str_dup(trg->name);
	this_data->trigger_type = trg->trigger_type;
	this_data->cmdlist = trg->cmdlist;
	this_data->narg = trg->narg;
	if (trg->arglist)
		this_data->arglist = str_dup(trg->arglist);
}


void trig_data_free(TRIG_DATA * this_data)
{
/*    struct cmdlist_element *i, *j;*/

	free(this_data->name);

	/*
	 * The command list is a memory leak right now!
	 *
	 if (cmdlist != trigg->cmdlist || this_data->proto)
	 for (i = cmdlist; i;) 
	 {j = i;
	 i = i->next;
	 free(j->cmd);
	 free(j);
	 }
	 */

	if (this_data->arglist)	// Не люблю вызовов free( NULL )
		free(this_data->arglist);

	free_varlist(this_data->var_list);

	if (this_data->wait_event) {
		// Не нужно забывать эмулировать callback вызов trig_wait_event
		// а именно выполнить освобождение структуры info
		free(this_data->wait_event->info);
		remove_event(this_data->wait_event);
	}

	free(this_data);
}

/* for mobs and rooms: */
void dg_read_trigger(FILE * fp, void *proto, int type)
{
	char line[256];
	char junk[8];
	int vnum, rnum, count;
	CHAR_DATA *mob;
	room_data *room;
	struct trig_proto_list *trg_proto, *new_trg;

	get_line(fp, line);
	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2) {	/* should do a better job of making this message */
		log("SYSERR: Error assigning trigger!");
		return;
	}

	rnum = real_trigger(vnum);
	if (rnum < 0) {
		sprintf(line, "SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
		log(line);
		return;
	}

	switch (type) {
	case MOB_TRIGGER:
		CREATE(new_trg, struct trig_proto_list, 1);
		new_trg->vnum = vnum;
		new_trg->next = NULL;

		mob = (CHAR_DATA *) proto;
		trg_proto = mob->proto_script;
		if (!trg_proto) {
			mob->proto_script = trg_proto = new_trg;
		} else {
			while (trg_proto->next)
				trg_proto = trg_proto->next;
			trg_proto->next = new_trg;
		}
		break;
	case WLD_TRIGGER:
		CREATE(new_trg, struct trig_proto_list, 1);
		new_trg->vnum = vnum;
		new_trg->next = NULL;
		room = (room_data *) proto;
		trg_proto = room->proto_script;
		if (!trg_proto) {
			room->proto_script = trg_proto = new_trg;
		} else {
			while (trg_proto->next)
				trg_proto = trg_proto->next;
			trg_proto->next = new_trg;
		}

		if (rnum >= 0) {
			if (!(room->script))
				CREATE(room->script, SCRIPT_DATA, 1);
			add_trigger(SCRIPT(room), read_trigger(rnum), -1);
		} else {
			sprintf(line, "SYSERR: non-existant trigger #%d assigned to room #%d", vnum, room->number);
			log(line);
		}
		break;
	default:
		sprintf(line, "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
		log(line);
	}
}

void dg_obj_trigger(char *line, OBJ_DATA * obj)
{
	char junk[8];
	int vnum, rnum, count;
	struct trig_proto_list *trg_proto, *new_trg;

	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2) {	/* should do a better job of making this message */
		log("SYSERR: Error assigning trigger!");
		return;
	}

	rnum = real_trigger(vnum);
	if (rnum < 0) {
		sprintf(line, "SYSERR: Trigger vnum #%d asked for but non-existant!", vnum);
		log(line);
		return;
	}

	CREATE(new_trg, struct trig_proto_list, 1);
	new_trg->vnum = vnum;
	new_trg->next = NULL;

	trg_proto = obj->proto_script;
	if (!trg_proto) {
		obj->proto_script = trg_proto = new_trg;
	} else {
		while (trg_proto->next)
			trg_proto = trg_proto->next;
		trg_proto->next = new_trg;
	}
}

extern vector < OBJ_DATA * >obj_proto;
extern CHAR_DATA *mob_proto;

void assign_triggers(void *i, int type)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	ROOM_DATA *room;
	int rnum;
	char buf[256];
	struct trig_proto_list *trg_proto;

	switch (type) {
	case MOB_TRIGGER:
		mob = (CHAR_DATA *) i;
		trg_proto = mob_proto[GET_MOB_RNUM(mob)].proto_script;
		while (trg_proto) {
			rnum = real_trigger(trg_proto->vnum);
			if (rnum == -1) {
				sprintf(buf, "SYSERR: trigger #%d non-existant, for mob #%d",
					trg_proto->vnum, mob_index[mob->nr].vnum);
				log(buf);
			} else {
				if (!SCRIPT(mob))
					CREATE(SCRIPT(mob), SCRIPT_DATA, 1);
				add_trigger(SCRIPT(mob), read_trigger(rnum), -1);
			}
			trg_proto = trg_proto->next;
		}
		break;
	case OBJ_TRIGGER:
		obj = (OBJ_DATA *) i;
		trg_proto = obj_proto[GET_OBJ_RNUM(obj)]->proto_script;
		while (trg_proto) {
			rnum = real_trigger(trg_proto->vnum);
			if (rnum == -1) {
				sprintf(buf, "SYSERR: trigger #%d non-existant, for obj #%d",
					trg_proto->vnum, obj_index[obj->item_number].vnum);
				log(buf);
			} else {
				if (!SCRIPT(obj))
					CREATE(SCRIPT(obj), SCRIPT_DATA, 1);
				add_trigger(SCRIPT(obj), read_trigger(rnum), -1);
			}
			trg_proto = trg_proto->next;
		}
		break;
	case WLD_TRIGGER:
		room = (ROOM_DATA *) i;
		trg_proto = room->proto_script;
		while (trg_proto) {
			rnum = real_trigger(trg_proto->vnum);
			if (rnum == -1) {
				sprintf(buf, "SYSERR: trigger #%d non-existant, for room #%d",
					trg_proto->vnum, room->number);
				log(buf);
			} else {
				if (!SCRIPT(room))
					CREATE(SCRIPT(room), SCRIPT_DATA, 1);
				add_trigger(SCRIPT(room), read_trigger(rnum), -1);
			}
			trg_proto = trg_proto->next;
		}
		break;
	default:
		log("SYSERR: unknown type for assign_triggers()");
		break;
	}
}

void trg_featturn(CHAR_DATA * ch, int featnum, int featdiff)
{
	if (HAVE_FEAT(ch, featnum)) {
		if (featdiff)
			return;
		else {
			sprintf(buf, "Вы утратили способность '%s'.\r\n", feat_info[featnum].name);
			send_to_char(buf, ch);
			UNSET_FEAT(ch, featnum);
		}
	} else {
		if (featdiff) {
			sprintf(buf, "Вы обрели способность '%s'.\r\n", feat_info[featnum].name);
			send_to_char(buf, ch);
		if (feat_info[featnum].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
			SET_FEAT(ch, featnum);
		};
	}
}

void trg_skillturn(CHAR_DATA * ch, int skillnum, int skilldiff)
{
	if (GET_SKILL(ch, skillnum)) {
		if (skilldiff)
			return;
		else {
			sprintf(buf, "Вас лишили умения '%s'.\r\n", skill_name(skillnum));
			send_to_char(buf, ch);
			GET_SKILL(ch, skillnum) = 0;
		}
	} else {
		if (skilldiff) {
			sprintf(buf, "Вы изучили умение '%s'.\r\n", skill_name(skillnum));
			send_to_char(buf, ch);
		if (skill_info[skillnum].classknow[(int) GET_KIN(ch)][(int) GET_CLASS(ch)] == KNOW_SKILL)
			if ( GET_LEVEL(ch) >= skill_info[skillnum].min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
			|| GET_REMORT(ch) >= skill_info[skillnum].min_remort[(int) GET_CLASS(ch) ][(int) GET_KIN(ch)]){
				GET_SKILL (ch, skillnum) = 5;
			}
		};
	}
}

void trg_skilladd(CHAR_DATA * ch, int skillnum, int skilldiff)
{
	int skill = GET_SKILL(ch, skillnum);
	GET_SKILL(ch, skillnum) = MAX(1, MIN(GET_SKILL(ch, skillnum) + skilldiff, 200));
	if (skill > GET_SKILL(ch, skillnum)) {
		sprintf(buf, "Ваше умение '%s' понизилось.\r\n", skill_name(skillnum));
		send_to_char(buf, ch);
	} else if (skill < GET_SKILL(ch, skillnum)) {
		sprintf(buf, "Вы повысили свое умение '%s'.\r\n", skill_name(skillnum));
		send_to_char(buf, ch);
	} else {
		sprintf(buf, "Ваше умение осталось неизменным '%s '.\r\n", skill_name(skillnum));
		send_to_char(buf, ch);
	}
}

void trg_spellturn(CHAR_DATA * ch, int spellnum, int spelldiff)
{
	int spell = GET_SPELL_TYPE(ch, spellnum);

	if (spell & SPELL_KNOW) {
		if (spelldiff)
			return;
		else {
			sprintf(buf, "Вы начисто забыли заклинание '%s'.\r\n", spell_name(spellnum));
			send_to_char(buf, ch);
			REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW);
			if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP))
				GET_SPELL_MEM(ch, spellnum) = 0;
		}
	} else {
		if (spelldiff) {
			sprintf(buf, "Вы постигли заклинание '%s'.\r\n", spell_name(spellnum));
			send_to_char(buf, ch);
			SET_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW);
		};
	}
}

void trg_spelladd(CHAR_DATA * ch, int spellnum, int spelldiff)
{
	int spell = GET_SPELL_MEM(ch, spellnum);
	GET_SPELL_MEM(ch, spellnum) = MAX(0, MIN(spell + spelldiff, 50));

	if (spell > GET_SPELL_MEM(ch, spellnum)) {
		if (GET_SPELL_MEM(ch, spellnum)) {
			sprintf(buf, "Вы забыли часть заклинаний '%s'.\r\n", spell_name(spellnum));
		} else {
			sprintf(buf, "Вы забыли все заклинания '%s'.\r\n", spell_name(spellnum));
			REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP);
		}
		send_to_char(buf, ch);
	} else if (spell < GET_SPELL_MEM(ch, spellnum)) {
		if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW))
			SET_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP);
		sprintf(buf, "Вы выучили несколько заклинаний '%s'.\r\n", spell_name(spellnum));
		send_to_char(buf, ch);
	}
}

void trg_spellitem(CHAR_DATA * ch, int spellnum, int spelldiff, int spell)
{
	char type[MAX_STRING_LENGTH];

	if ((spelldiff && IS_SET(GET_SPELL_TYPE(ch, spellnum), spell)) ||
	    (!spelldiff && !IS_SET(GET_SPELL_TYPE(ch, spellnum), spell)))
		return;
	if (!spelldiff) {
		REMOVE_BIT(GET_SPELL_TYPE(ch, spellnum), spell);
		switch (spell) {
		case SPELL_SCROLL:
			strcpy(type, "создания свитка");
			break;
		case SPELL_POTION:
			strcpy(type, "приготовления напитка");
			break;
		case SPELL_WAND:
			strcpy(type, "изготовления посоха");
			break;
		case SPELL_ITEMS:
			strcpy(type, "предметной магии");
			break;
		case SPELL_RUNES:
			strcpy(type, "использования рун");
			break;
		};
		sprintf(buf, "Вы утратили умение %s '%s'", type, spell_name(spellnum));
	} else {
		SET_BIT(GET_SPELL_TYPE(ch, spellnum), spell);
		switch (spell) {
		case SPELL_SCROLL:
			if (!GET_SKILL(ch, SKILL_CREATE_SCROLL))
				SET_SKILL(ch, SKILL_CREATE_SCROLL, 5);
			strcpy(type, "создания свитка");
			break;
		case SPELL_POTION:
			if (!GET_SKILL(ch, SKILL_CREATE_POTION))
				SET_SKILL(ch, SKILL_CREATE_POTION, 5);
			strcpy(type, "приготовления напитка");
			break;
		case SPELL_WAND:
			if (!GET_SKILL(ch, SKILL_CREATE_WAND))
				SET_SKILL(ch, SKILL_CREATE_WAND, 5);
			strcpy(type, "изготовления посоха");
			break;
		case SPELL_ITEMS:
			strcpy(type, "предметной магии");
			break;
		case SPELL_RUNES:
			strcpy(type, "использования рун");
			break;
		};
		sprintf(buf, "Вы приобрели умение %s '%s'", type, spell_name(spellnum));
		send_to_char(buf, ch);
		check_recipe_items(ch, spellnum, spell, TRUE);
	}
}
