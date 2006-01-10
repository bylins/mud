/* ************************************************************************
*   File: boards.cpp                                    Part of Bylins    *
*  Usage: handling of multiple bulletin boards                            *
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


/* FEATURES & INSTALLATION INSTRUCTIONS ***********************************

This board code has many improvements over the infamously buggy standard
Diku board code.  Features include:

- Arbitrary number of boards handled by one set of generalized routines.
  Adding a new board is as easy as adding another entry to an array.
- Safe removal of messages while other messages are being written.
- Does not allow messages to be removed by someone of a level less than
  the poster's level.


TO ADD A NEW BOARD, simply follow our easy 4-step program:

1 - Create a new board object in the object files

2 - Increase the NUM_OF_BOARDS constant in boards.h

3 - Add a new line to the board_info array below.  The fields, in order, are:

	Board's virtual number.
	Min level one must be to look at this board or read messages on it.
	Min level one must be to post a message to the board.
	Min level one must be to remove other people's messages from this
		board (but you can always remove your own message).
	Filename of this board, in quotes.
	Last field must always be 0.

4 - In spec_assign.c, find the section which assigns the special procedure
    gen_board to the other bulletin boards, and add your new one in a
    similar fashion.

*/


#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"

/* Board appearance order. */
#define	NEWEST_AT_TOP	TRUE

extern DESCRIPTOR_DATA *descriptor_list;

/*
format:	vnum, read lvl, write lvl, remove lvl, filename, 0, lastwrite at end
Be sure to also change NUM_OF_BOARDS in board.h
*/
struct board_info_type board_info[NUM_OF_BOARDS] = { {250, LVL_IMMORT, LVL_IMMORT, LVL_GOD, LIB_ETC "board.immort", 0},
{251, 0, 15, LVL_IMMORT, LIB_ETC "board.newbie", 0},
{252, LVL_GRGOD, LVL_GRGOD, LVL_GRGOD, LIB_ETC "board.milich", 0},
{253, LVL_GRGOD, 0, LVL_GRGOD, LIB_ETC "board.bug", 0},
{254, 0, 31, LVL_IMMORT, LIB_ETC "board.newbie", 0},
{255, 0, 0, LVL_IMMORT, LIB_ETC "board.newbie", 0},
{256, 0, 0, LVL_IMMORT, LIB_ETC "board.newbie", 0},
{257, 0, 12, LVL_IMMORT, LIB_ETC "board.mort", 0},
{258, 15, 15, LVL_IMMORT, LIB_ETC "board.mort.kill", 0},
{259, LVL_IMMORT, LVL_GRGOD, LVL_GRGOD, LIB_ETC "board.builder", 0},
{267, LVL_GRGOD, LVL_GRGOD, LVL_GRGOD, LIB_ETC "board.porenut", 0},
{12635, 0, 0, LVL_IMPL, LIB_ETC "board.nd", 0},
{268, 0, LVL_IMMORT, LVL_IMMORT, LIB_ETC "board.immortal_notice", 0}
};

/* local functions */
SPECIAL(gen_board);
int find_slot(void);
int find_board(CHAR_DATA * ch);
void init_boards(void);

char *msg_storage[INDEX_SIZE];
int msg_storage_taken[INDEX_SIZE];
int num_of_msgs[NUM_OF_BOARDS];
int ACMD_READ, ACMD_LOOK, ACMD_EXAMINE, ACMD_WRITE, ACMD_REMOVE,
    ACMD_RREAD, ACMD_RLOOK, ACMD_REXAMINE, ACMD_RWRITE, ACMD_RREMOVE;
struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];


int find_slot(void)
{
	int i;

	for (i = 0; i < INDEX_SIZE; i++)
		if (!msg_storage_taken[i]) {
			msg_storage_taken[i] = 1;
			return (i);
		}
	return (-1);
}


/* search the room ch is standing in to find which board he's looking at */
int find_board(CHAR_DATA * ch)
{
	OBJ_DATA *obj;
	int i;

	for (obj = world[IN_ROOM(ch)]->contents; obj; obj = obj->next_content)
		for (i = 0; i < NUM_OF_BOARDS; i++)
			if (BOARD_RNUM(i) == GET_OBJ_RNUM(obj))
				return (i);

	return (-1);
}


void init_boards(void)
{
	int i, j, fatal_error = 0;

	for (i = 0; i < INDEX_SIZE; i++) {
		msg_storage[i] = 0;
		msg_storage_taken[i] = 0;
	}

	for (i = 0; i < NUM_OF_BOARDS; i++) {
		if ((BOARD_RNUM(i) = real_object(BOARD_VNUM(i))) == -1) {
			log("SYSERR: Fatal board error: board vnum %d does not exist!", BOARD_VNUM(i));
			fatal_error = 1;
		}
		num_of_msgs[i] = 0;
		for (j = 0; j < MAX_BOARD_MESSAGES; j++) {
			memset((char *) &(msg_index[i][j]), 0, sizeof(struct board_msginfo));
			msg_index[i][j].slot_num = -1;
		}
		Board_load_board(i);
	}

	ACMD_READ = find_command("read");
	ACMD_WRITE = find_command("write");
	ACMD_REMOVE = find_command("remove");
	ACMD_LOOK = find_command("look");
	ACMD_EXAMINE = find_command("examine");
	ACMD_RREAD = find_command("читать");
	ACMD_RWRITE = find_command("писать");
	ACMD_RREMOVE = find_command("очистить");
	ACMD_RLOOK = find_command("смотреть");
	ACMD_REXAMINE = find_command("осмотреть");

	if (fatal_error)
		exit(1);
}


SPECIAL(gen_board)
{
	int board_type;
	static int loaded = 0;
	OBJ_DATA *board = (OBJ_DATA *) me;

	if (!loaded) {
		init_boards();
		loaded = 1;
	}
	if (!ch->desc)
		return (0);

	if (cmd != ACMD_WRITE &&
	    cmd != ACMD_LOOK &&
	    cmd != ACMD_EXAMINE &&
	    cmd != ACMD_READ &&
	    cmd != ACMD_REMOVE &&
	    cmd != ACMD_RWRITE && cmd != ACMD_RLOOK && cmd != ACMD_REXAMINE && cmd != ACMD_RREAD && cmd != ACMD_RREMOVE)
		return (0);

	if (AFF_FLAGGED(ch, AFF_BLIND)) {	/*Added by Niker */
		send_to_char("Вы ослеплены !\r\n", ch);
		return (1);
	}

	if ((board_type = find_board(ch)) == -1) {
		log("SYSERR:  degenerate board!  (what the hell...)");
		return (0);
	}
	if (cmd == ACMD_WRITE || cmd == ACMD_RWRITE)
		return (Board_write_message(board_type, ch, argument, board));
	else if (cmd == ACMD_LOOK || cmd == ACMD_EXAMINE || cmd == ACMD_RLOOK || cmd == ACMD_REXAMINE)
		return (Board_show_board(board_type, ch, argument, board));
	else if (cmd == ACMD_READ || cmd == ACMD_RREAD)
		return (Board_display_msg(board_type, ch, argument, board));
	else if (cmd == ACMD_REMOVE || cmd == ACMD_RREMOVE)
		return (Board_remove_msg(board_type, ch, argument, board));
	else
		return (0);
}


int Board_write_message(int board_type, CHAR_DATA * ch, char *arg, OBJ_DATA * board)
{
	char *tmstr;
	time_t ct;
	char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

	if (!str_cmp(GET_NAME(ch), LAST_WRITE(board_type))) {
		send_to_char("Вы ведь только что писали, успокойтесь...\r\n", ch);
		return (1);
	}
	if (GET_LEVEL(ch) < WRITE_LVL(board_type) && !GET_COMMSTATE(ch)) {
		send_to_char("Рановато будет...\r\n", ch);
		return (1);
	}
	if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES) {
		Board_autoremove_msg(board_type, board);
	}
	if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1) {
		send_to_char("Сломана :(.\r\n", ch);
		log("SYSERR: Board: failed to find empty slot on write.");
		return (1);
	}
	/* skip blanks */
	skip_spaces(&arg);
	delete_doubledollar(arg);

	/* JE 27 Oct 95 - Truncate headline at 80 chars if it's longer than that */
	arg[80] = '\0';

	if (!*arg) {
		send_to_char("Сообщение должно начинаться с заголовка!\r\n", ch);
		return (1);
	}
	ct = time(0);
	tmstr = (char *) asctime(localtime(&ct));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	sprintf(buf2, "(%s)", GET_NAME(ch));
	sprintf(buf, "%6.10s %-12s :: %s", tmstr, buf2, arg);
	NEW_MSG_INDEX(board_type).heading = str_dup(buf);
	NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);

	send_to_char("Можете писать сообщение.  (/s записать /h помощь)\r\n\r\n", ch);
	act("$n высунул$g язык и начал$g карябать сообщение.", TRUE, ch, 0, 0, TO_ROOM);

	string_write(ch->desc, &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]),
		     MAX_MESSAGE_LENGTH, board_type + BOARD_MAGIC, NULL);

	num_of_msgs[board_type]++;
	if (!IS_IMMORTAL(ch))
		sprintf(LAST_WRITE(board_type), "%s", GET_NAME(ch));
	return (1);
}


int Board_show_board(int board_type, CHAR_DATA * ch, char *arg, OBJ_DATA * board)
{
	int i;
	char tmp[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

	if (!ch->desc)
		return (0);

	one_argument(arg, tmp);

	if (!*tmp || !isname(tmp, board->name))
		return (0);

	if (GET_LEVEL(ch) < READ_LVL(board_type) && !GET_COMMSTATE(ch)) {
		send_to_char("Вы не смогли разобрать ни слова.\r\n", ch);
		return (1);
	}
	act("$n начал$g изучать доску сообщений.", TRUE, ch, 0, 0, TO_ROOM);

	strcpy(buf,
	       "Это доска, на которой всякие разные личности оставили свои IMHO.\r\n"
	       "Формат: ЧИТАТЬ/ОЧИСТИТЬ <номер сообщения>, ПИСАТЬ <тема сообщения>.\r\n");
	if (!num_of_msgs[board_type])
		strcat(buf, "Никто ниче не накарябал, слава Богам.\r\n");
	else {
		sprintf(buf + strlen(buf), "Всего сообщений :  %d .\r\n", num_of_msgs[board_type]);
#if NEWEST_AT_TOP
		for (i = num_of_msgs[board_type] - 1; i >= 0; i--)
#else
		for (i = 0; i < num_of_msgs[board_type]; i++)
#endif
		{
			if (MSG_HEADING(board_type, i))
#if NEWEST_AT_TOP
				sprintf(buf + strlen(buf), "%-2d : %s\r\n",
					num_of_msgs[board_type] - i, MSG_HEADING(board_type, i));
#else
				sprintf(buf + strlen(buf), "%-2d : %s\r\n", i + 1, MSG_HEADING(board_type, i));
#endif
			else {
				log("SYSERR: The board is fubar'd.");
				send_to_char("Что-то не работает. А жаль :(.\r\n", ch);
				return (1);
			}
		}
	}
	page_string(ch->desc, buf, 1);

	return (1);
}


int Board_display_msg(int board_type, CHAR_DATA * ch, char *arg, OBJ_DATA * board)
{
	char number[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
	int msg, ind;

	one_argument(arg, number);
	if (!*number)
		return (0);
	if (isname(number, board->name))	/* so "read board" works */
		return (Board_show_board(board_type, ch, arg, board));
	if (strchr(number, '.'))	/* read 2.mail, look 2.sword */
		return (0);
	if (!isdigit(*number) || (!(msg = atoi(number))))
		return (0);

	if (GET_LEVEL(ch) < READ_LVL(board_type) && !GET_COMMSTATE(ch)) {
		send_to_char("Вы слабы в понимании сиих письмен.\r\n", ch);
		return (1);
	}
	if (!num_of_msgs[board_type]) {
		send_to_char("Доска абсолютно чиста !\r\n", ch);
		return (1);
	}
	if (msg < 1 || msg > num_of_msgs[board_type]) {
		send_to_char("Это сообщение может Вам только присниться.\r\n", ch);
		return (1);
	}
#if NEWEST_AT_TOP
	ind = num_of_msgs[board_type] - msg;
#else
	ind = msg - 1;
#endif
	if (MSG_SLOTNUM(board_type, ind) < 0 || MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE) {
		send_to_char("Сломалось. Доигрались :(\r\n", ch);
		log("SYSERR: Board is screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
		return (1);
	}
	if (!(MSG_HEADING(board_type, ind))) {
		send_to_char("Кто-то приложил немало усилий, чтобы испортить это сообщений.\r\n", ch);
		return (1);
	}
	if (!(msg_storage[MSG_SLOTNUM(board_type, ind)])) {
		send_to_char("Кто-то не оставил от сообщения ни слова.\r\n", ch);
		return (1);
	}
	sprintf(buffer, "Сообщение %d : %s\r\n\r\n%s\r\n", msg,
		MSG_HEADING(board_type, ind), msg_storage[MSG_SLOTNUM(board_type, ind)]);

	page_string(ch->desc, buffer, 1);

	return (1);
}


int Board_remove_msg(int board_type, CHAR_DATA * ch, char *arg, OBJ_DATA * board)
{
	int ind, msg, slot_num;
	char number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;

	one_argument(arg, number);

	if (!*number || !isdigit(*number))
		return (0);
	if (!(msg = atoi(number)))
		return (0);

	if (!num_of_msgs[board_type]) {
		send_to_char("Стерильно !\r\n", ch);
		return (1);
	}
	if (msg < 1 || msg > num_of_msgs[board_type]) {
		act("From : Боги\r\nTo : $n\r\nSubj: Неверный номер сообщения\r\n"
		    "\r\n Челом бьем, досточтим$w $n.\r\n %SUBJ%\r\n         БОГИ", FALSE, ch, 0, 0, TO_CHAR);
		return (1);
	}
#if NEWEST_AT_TOP
	ind = num_of_msgs[board_type] - msg;
#else
	ind = msg - 1;
#endif

	if (!MSG_HEADING(board_type, ind)) {
		send_to_char("Непонятная белиберда накарябана тут.\r\n", ch);
		return (1);
	}
	sprintf(buf, "(%s)", GET_NAME(ch));
	if (GET_LEVEL(ch) < REMOVE_LVL(board_type) && !GET_COMMSTATE(ch) &&
	    !(strstr(MSG_HEADING(board_type, ind), buf))) {
		send_to_char("Эти буквы явно не из русского алфавита.\r\n", ch);
		return (1);
	}
	if (GET_LEVEL(ch) < MSG_LEVEL(board_type, ind) && !GET_COMMSTATE(ch)) {
		send_to_char("Сии строки защищены Высшей волшбой.\r\n", ch);
		return (1);
	}
	slot_num = MSG_SLOTNUM(board_type, ind);
	if (slot_num < 0 || slot_num >= INDEX_SIZE) {
		send_to_char("Сообщение утеряно для потомков.\r\n", ch);
		log("SYSERR: The board is seriously screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
		return (1);
	}
	for (d = descriptor_list; d; d = d->next)
		if (STATE(d) == CON_PLAYING && d->str == &(msg_storage[slot_num])) {
			send_to_char("Дайте автору дописать !\r\n", ch);
			return (1);
		}

	if (msg_storage[slot_num])
		free(msg_storage[slot_num]);
	msg_storage[slot_num] = 0;
	msg_storage_taken[slot_num] = 0;
	if (MSG_HEADING(board_type, ind))
		free(MSG_HEADING(board_type, ind));

	for (; ind < num_of_msgs[board_type] - 1; ind++) {
		MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
		MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
		MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
	}
	num_of_msgs[board_type]--;
	send_to_char("Сообщение удалено.\r\n", ch);
	sprintf(buf, "$n удалил$g %d сообщение.", msg);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	Board_save_board(board_type);

	if (!str_cmp(GET_NAME(ch), LAST_WRITE(board_type)) && msg == 1)
		LAST_WRITE(board_type)[0] = 0;

	return (1);
}


void Board_save_board(int board_type)
{
	FILE *fl;
	int i;
	char *tmp1, *tmp2 = NULL;

	if (!num_of_msgs[board_type]) {
		remove(FILENAME(board_type));
		return;
	}
	if (!(fl = fopen(FILENAME(board_type), "wb"))) {
		perror("SYSERR: Error writing board");
		return;
	}
	fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

	for (i = 0; i < num_of_msgs[board_type]; i++) {
		if ((tmp1 = MSG_HEADING(board_type, i)) != NULL)
			msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
		else
			msg_index[board_type][i].heading_len = 0;

		if (MSG_SLOTNUM(board_type, i) < 0 ||
		    MSG_SLOTNUM(board_type, i) >= INDEX_SIZE || (!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
			msg_index[board_type][i].message_len = 0;
		else
			msg_index[board_type][i].message_len = strlen(tmp2) + 1;

		fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
		if (tmp1)
			fwrite(tmp1, sizeof(char), msg_index[board_type][i].heading_len, fl);
		if (tmp2)
			fwrite(tmp2, sizeof(char), msg_index[board_type][i].message_len, fl);
	}

	fclose(fl);
}


void Board_load_board(int board_type)
{
	FILE *fl;
	int i, len1, len2;
	char *tmp1, *tmp2;

	if (!(fl = fopen(FILENAME(board_type), "rb"))) {
		if (errno != ENOENT)
			perror("SYSERR: Error reading board");
		return;
	}
	fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl);
	if (num_of_msgs[board_type] < 1 || num_of_msgs[board_type] > MAX_BOARD_MESSAGES) {
		log("SYSERR: Board file %d corrupt.  Resetting.", board_type);
		Board_reset_board(board_type);
		return;
	}
	for (i = 0; i < num_of_msgs[board_type]; i++) {
		fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
		if ((len1 = msg_index[board_type][i].heading_len) <= 0) {
			log("SYSERR: Board file %d corrupt!  Resetting.", board_type);
			Board_reset_board(board_type);
			return;
		}
		CREATE(tmp1, char, len1);
		fread(tmp1, sizeof(char), len1, fl);
		MSG_HEADING(board_type, i) = tmp1;

		if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1) {
			log("SYSERR: Out of slots booting board %d!  Resetting...", board_type);
			Board_reset_board(board_type);
			return;
		}
		if ((len2 = msg_index[board_type][i].message_len) > 0) {
			CREATE(tmp2, char, len2);
			fread(tmp2, sizeof(char), len2, fl);
			msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
		} else
			msg_storage[MSG_SLOTNUM(board_type, i)] = NULL;
	}

	fclose(fl);
}


void Board_reset_board(int board_type)
{
	int i;

	for (i = 0; i < MAX_BOARD_MESSAGES; i++) {
		if (MSG_HEADING(board_type, i))
			free(MSG_HEADING(board_type, i));
		if (msg_storage[MSG_SLOTNUM(board_type, i)])
			free(msg_storage[MSG_SLOTNUM(board_type, i)]);
		msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
		memset((char *) &(msg_index[board_type][i]), 0, sizeof(struct board_msginfo));
		msg_index[board_type][i].slot_num = -1;
	}
	num_of_msgs[board_type] = 0;
	remove(FILENAME(board_type));
}

int Board_autoremove_msg(int board_type, OBJ_DATA * board)
{
	int ind, msg = MAX_BOARD_MESSAGES, slot_num;

#if NEWEST_AT_TOP
	ind = num_of_msgs[board_type] - msg;
#else
	ind = msg - 1;
#endif

	slot_num = MSG_SLOTNUM(board_type, ind);
	if (slot_num < 0 || slot_num >= INDEX_SIZE) {
		log("SYSERR: The board is seriously screwed up.");
		return (1);
	}

	if (msg_storage[slot_num])
		free(msg_storage[slot_num]);
	msg_storage[slot_num] = 0;
	msg_storage_taken[slot_num] = 0;
	if (MSG_HEADING(board_type, ind))
		free(MSG_HEADING(board_type, ind));

	for (; ind < num_of_msgs[board_type] - 1; ind++) {
		MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
		MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
		MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
	}
	num_of_msgs[board_type]--;
	Board_save_board(board_type);

	return (1);
}
