/* ************************************************************************
*   File: top.cpp                                       Part of Bylins    *
*  Usage: tops handling funcs                                             *
*                                                                         *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "comm.h"

#include "top.h"

extern struct player_index_element *player_table;
extern int top_of_p_table;

struct max_remort_top_element max_remort_top[NUM_CLASSES][MAX_REMORT_TOP_SIZE];

ACMD(do_best);

// return index by player name if max_remort_top array contains this player name
// or MAX_REMORT_TOP_SIZE if it doesn't
int get_ibpn_max_remort_top(CHAR_DATA * ch)
{
	int i;

	for (i = 0; i < MAX_REMORT_TOP_SIZE; i++)
		if (*(max_remort_top[(int) GET_CLASS(ch)][i].name) == '\0' || !strcmp(GET_NAME(ch), max_remort_top[(int) GET_CLASS(ch)][i].name))
			break;

	if (i < MAX_REMORT_TOP_SIZE && *(max_remort_top[(int) GET_CLASS(ch)][i].name) == '\0')
		i = MAX_REMORT_TOP_SIZE;

	return i;
}

// delete player from max_remort_top array
void del_p_max_remort_top(CHAR_DATA * ch)
{
	int i = get_ibpn_max_remort_top(ch);

	if (i == MAX_REMORT_TOP_SIZE)
		return;

	for (; i + 1 < MAX_REMORT_TOP_SIZE; i++) {
		strcpy(max_remort_top[(int) GET_CLASS(ch)][i].name, max_remort_top[(int) GET_CLASS(ch)][i+1].name);
		max_remort_top[(int) GET_CLASS(ch)][i].remort = max_remort_top[(int) GET_CLASS(ch)][i+1].remort;
		max_remort_top[(int) GET_CLASS(ch)][i].exp = max_remort_top[(int) GET_CLASS(ch)][i+1].exp;
	}
	*(max_remort_top[(int) GET_CLASS(ch)][i].name) = '\0';
	max_remort_top[(int) GET_CLASS(ch)][i].remort = 0;
	max_remort_top[(int) GET_CLASS(ch)][i].exp = 0;
}

// find correct index for the player in max_remort_top array
int find_ind_max_remort_top(CHAR_DATA * ch)
{
	int i;

	for (i = MAX_REMORT_TOP_SIZE - 1; i >= 0; i--)
		if (GET_REMORT(ch) < max_remort_top[(int) GET_CLASS(ch)][i].remort
		||
		GET_REMORT(ch) == max_remort_top[(int) GET_CLASS(ch)][i].remort && GET_EXP(ch) < max_remort_top[(int) GET_CLASS(ch)][i].exp)
			break;

	return i + 1;
}

// add player to max_remort_top array
void add_p_max_remort_top(CHAR_DATA * ch)
{
	int i = find_ind_max_remort_top(ch);

	if (i == MAX_REMORT_TOP_SIZE)
		return;

	for (int j = MAX_REMORT_TOP_SIZE - 1; j > i; j--) {
		strcpy(max_remort_top[(int) GET_CLASS(ch)][j].name, max_remort_top[(int) GET_CLASS(ch)][j-1].name);
		max_remort_top[(int) GET_CLASS(ch)][j].remort = max_remort_top[(int) GET_CLASS(ch)][j-1].remort;
		max_remort_top[(int) GET_CLASS(ch)][j].exp = max_remort_top[(int) GET_CLASS(ch)][j-1].exp;
	}
	strcpy(max_remort_top[(int) GET_CLASS(ch)][i].name, GET_NAME(ch));
	max_remort_top[(int) GET_CLASS(ch)][i].remort = GET_REMORT(ch);
	max_remort_top[(int) GET_CLASS(ch)][i].exp = GET_EXP(ch);
}

// update player in max_remort_top array
// only for existing mortals
// if player has become immortal or has been deleted he won't be shown in top only after reload or reboot
void upd_p_max_remort_top(CHAR_DATA * ch)
{
	if (ch != NULL && !IS_NPC(ch) && !IS_SET(PLR_FLAGS(ch, PLR_DELETED), PLR_DELETED) && !IS_IMMORTAL(ch)) {
		del_p_max_remort_top(ch);
		add_p_max_remort_top(ch);
	}
}

// max_remort_top load with initialization
void load_max_remort_top(void)
{
	int i, j, c;
	CHAR_DATA *dummy;

	// clear max_remort_top
	for (i = 0; i < NUM_CLASSES; i++)
		for (j = 0; j < MAX_REMORT_TOP_SIZE; j++) {
			*(max_remort_top[i][j].name) = '\0';
			max_remort_top[i][j].remort = 0;
			max_remort_top[i][j].exp = 0;
		}

	// load max_remort_top array to show the best players at the moment
	for (c = 0; c <= top_of_p_table; c++) {
		CREATE(dummy, CHAR_DATA, 1);
		clear_char(dummy);
		if (load_char(player_table[c].name, dummy) > -1) {
			upd_p_max_remort_top(dummy);
			free_char(dummy);
		} else
			free(dummy);
	}
}

struct top_show_struct top_show_fields[] =
{
// class tops
	{"лекари", CLASS_CLERIC},
	{"колдуны", CLASS_BATTLEMAGE},
	{"тати", CLASS_THIEF},
	{"богатыри", CLASS_WARRIOR},
	{"наемники", CLASS_ASSASINE},
	{"дружинники", CLASS_GUARD},
	{"кудесники", CLASS_CHARMMAGE},
	{"волшебники", CLASS_DEFENDERMAGE},
	{"чернокнижники", CLASS_NECROMANCER},
	{"витязи", CLASS_PALADINE},
	{"охотники", CLASS_RANGER},
	{"кузнецы", CLASS_SMITH},
	{"купцы", CLASS_MERCHANT},
	{"волхвы", CLASS_DRUID},
	{"\n", CLASS_UNDEFINED},
// other tops
	{"игроки", TOP_ALL},
	{"дружины", TOP_CLANS},
	{"\n", CLASS_UNDEFINED}
};

ACMD(do_best)
{
	int i, j, rem;
	char *last_word;

	if (!ch->desc)
		return;

	one_argument(argument, arg);

	if (!*arg) {
		strcpy(buf, "Лучшими могут быть:\r\n");
		for (j = 0, i = 0; *(top_show_fields[i].cmd) != '\n'; i++)
			sprintf(buf + strlen(buf), "%-15s%s", top_show_fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
		for (i++; *(top_show_fields[i].cmd) != '\n'; i++)
			sprintf(buf + strlen(buf), "%-15s%s", top_show_fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
		if (j % 5)
			strcat(buf, "\r\n");
		send_to_char(buf, ch);
		return;
	}

	for (i = 0; *(top_show_fields[i].cmd) != '\n'; i++)
		if (!strncmp(arg, top_show_fields[i].cmd, strlen(arg)))
			break;

	if (*(top_show_fields[i].cmd) == '\n')
		for (i++; *(top_show_fields[i].cmd) != '\n'; i++)
			if (!strncmp(arg, top_show_fields[i].cmd, strlen(arg)))
				break;

	switch (top_show_fields[i].mode) {
		case CLASS_CLERIC:
		case CLASS_BATTLEMAGE:
		case CLASS_THIEF:
		case CLASS_WARRIOR:
		case CLASS_ASSASINE:
		case CLASS_GUARD:
		case CLASS_CHARMMAGE:
		case CLASS_DEFENDERMAGE:
		case CLASS_NECROMANCER:
		case CLASS_PALADINE:
		case CLASS_RANGER:
		case CLASS_SMITH:
		case CLASS_MERCHANT:
		case CLASS_DRUID:
			CREATE(last_word, char, strlen("перевоплощение") + 1);
			sprintf(buf, "&cЛучшие %s:&n\r\n", top_show_fields[i].cmd);
			for (j = 0; j < MAX_REMORT_TOP_SIZE && *(max_remort_top[(int) top_show_fields[i].mode][j].name) != '\0'; j++) {
				rem = max_remort_top[(int) top_show_fields[i].mode][j].remort;
				if (!(rem % 10) || rem % 10 >= 5 && rem % 10 <= 9 || !((rem - 11) % 100) || !((rem - 12) % 100) || !((rem - 13) % 100) || !((rem - 14) % 100))
					strcpy(last_word, "перевоплощений");
				else if (rem % 10 == 1)
					strcpy(last_word, "перевоплощение");
				else
					strcpy(last_word, "перевоплощения");
				sprintf(buf + strlen(buf),
					"\t%-20s %-2d %s\r\n",
					max_remort_top[(int) top_show_fields[i].mode][j].name, max_remort_top[(int) top_show_fields[i].mode][j].remort, last_word);
			}
			send_to_char(buf, ch);
			free(last_word);
			break;
		case TOP_ALL:
			CREATE(last_word, char, strlen("перевоплощение") + 1);
			buf[0] = '\0';
			for (i = 0; *(top_show_fields[i].cmd) != '\n'; i++) {
				sprintf(buf + strlen(buf), "&cЛучшие %s:&n\r\n", top_show_fields[i].cmd);
				for (j = 0; j < MAX_REMORT_TOP_SIZE && *(max_remort_top[(int) top_show_fields[i].mode][j].name) != '\0'; j++) {
					rem = max_remort_top[(int) top_show_fields[i].mode][j].remort;
					if (!(rem % 10) || rem % 10 >= 5 && rem % 10 <= 9 || !((rem - 11) % 100) || !((rem - 12) % 100) || !((rem - 13) % 100) || !((rem - 14) % 100))
						strcpy(last_word, "перевоплощений");
					else if (rem % 10 == 1)
						strcpy(last_word, "перевоплощение");
					else
						strcpy(last_word, "перевоплощения");
					sprintf(buf + strlen(buf),
						"\t%-20s %-2d %s\r\n",
						max_remort_top[(int) top_show_fields[i].mode][j].name, max_remort_top[(int) top_show_fields[i].mode][j].remort, last_word);
				}
				if (*(top_show_fields[i+1].cmd) != '\n')
					sprintf(buf + strlen(buf), "\r\n");
			}
			page_string(ch->desc, buf, 1);
			free(last_word);
			break;
		case TOP_CLANS:
			strcpy(buf, "Раздел пока недоступен.\r\n");
			send_to_char(buf, ch);
			break;
		default:
			strcpy(buf, "Вы можете остаться при своем мнении!\r\n");
			send_to_char(buf, ch);
			break;
	}
}
