/* ************************************************************************
*   File: mail.cpp                                      Part of Bylins    *
*  Usage: Internal funcs and player spec-procs of mud-mail system         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

/******* MUD MAIL SYSTEM MAIN FILE ***************************************

Written by Jeremy Elson (jelson@circlemud.org)

*************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "mail.h"
#include "char.hpp"
#include "parcel.hpp"
#include "char_player.hpp"
#include "named_stuff.hpp"

void postmaster_send_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg);
void postmaster_check_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg);
void postmaster_receive_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg);
SPECIAL(postmaster);

extern int no_mail;
int find_name(const char *name);

mail_index_type *mail_index = NULL;	/* list of recs in the mail file  */
position_list_type *free_list = NULL;	/* list of free positions in file */
long file_end_pos = 0;		/* length of file */

/* local functions */
void push_free_list(long pos);
long pop_free_list(void);
mail_index_type *find_char_in_index(long searchee);
void write_to_file(void *buf, int size, long filepos);
void read_from_file(void *buf, int size, long filepos);
void index_mail(long id_to_index, long pos);

/* -------------------------------------------------------------------------- */

/*
 * void push_free_list(long #1)
 * #1 - What byte offset into the file the block resides.
 *
 * Net effect is to store a list of free blocks in the mail file in a linked
 * list.  This is called when people receive their messages and at startup
 * when the list is created.
 */
void push_free_list(long pos)
{
	position_list_type *new_pos;

	CREATE(new_pos, position_list_type, 1);
	new_pos->position = pos;
	new_pos->next = free_list;
	free_list = new_pos;
}


/*
 * long pop_free_list(none)
 * Returns the offset of a free block in the mail file.
 *
 * Typically used whenever a person mails a message.  The blocks are not
 * guaranteed to be sequential or in any order at all.
 */
long pop_free_list(void)
{
	position_list_type *old_pos;
	long return_value;

	/*
	 * If we don't have any free blocks, we append to the file.
	 */
	if ((old_pos = free_list) == NULL)
		return (file_end_pos);

	/* Save the offset of the free block. */
	return_value = free_list->position;
	/* Remove this block from the free list. */
	free_list = old_pos->next;
	/* Get rid of the memory the node took. */
	free(old_pos);
	/* Give back the free offset. */
	return (return_value);
}


/*
 * main_index_type *find_char_in_index(long #1)
 * #1 - The idnum of the person to look for.
 * Returns a pointer to the mail block found.
 *
 * Finds the first mail block for a specific person based on id number.
 */
mail_index_type *find_char_in_index(long searchee)
{
	mail_index_type *tmp;

	if (searchee < 0)
	{
		log("SYSERR: Mail system -- non fatal error #1 (searchee == %ld).", searchee);
		return (NULL);
	}
	for (tmp = mail_index; (tmp && tmp->recipient != searchee); tmp = tmp->next);

	return (tmp);
}


/*
 * void write_to_file(void * #1, int #2, long #3)
 * #1 - A pointer to the data to write, usually the 'block' record.
 * #2 - How much to write (because we'll write NUL terminated strings.)
 * #3 - What offset (block position) in the file to write to.
 *
 * Writes a mail block back into the database at the given location.
 */
void write_to_file(void *buf, int size, long filepos)
{
	FILE *mail_file;

	if (filepos % BLOCK_SIZE)
	{
		log("SYSERR: Mail system -- fatal error #2!!! (invalid file position %ld)", filepos);
		no_mail = TRUE;
		return;
	}
	if (!(mail_file = fopen(MAIL_FILE, "r+b")))
	{
		log("SYSERR: Unable to open mail file '%s'.", MAIL_FILE);
		no_mail = TRUE;
		return;
	}
	fseek(mail_file, filepos, SEEK_SET);
	fwrite(buf, size, 1, mail_file);

	/* find end of file */
	fseek(mail_file, 0L, SEEK_END);
	file_end_pos = ftell(mail_file);
	fclose(mail_file);
	return;
}


/*
 * void read_from_file(void * #1, int #2, long #3)
 * #1 - A pointer to where we should store the data read.
 * #2 - How large the block we're reading is.
 * #3 - What position in the file to read.
 *
 * This reads a block from the mail database file.
 */
void read_from_file(void *buf, int size, long filepos)
{
	FILE *mail_file;

	if (filepos % BLOCK_SIZE)
	{
		log("SYSERR: Mail system -- fatal error #3!!! (invalid filepos read %ld)", filepos);
		no_mail = TRUE;
		return;
	}
	if (!(mail_file = fopen(MAIL_FILE, "r+b")))
	{
		log("SYSERR: Unable to open mail file '%s'.", MAIL_FILE);
		no_mail = TRUE;
		return;
	}

	fseek(mail_file, filepos, SEEK_SET);
	fread(buf, size, 1, mail_file);
	fclose(mail_file);
	return;
}


void index_mail(long id_to_index, long pos)
{
	mail_index_type *new_index;
	position_list_type *new_position;

	if (id_to_index < 0)
	{
		log("SYSERR: Mail system -- non-fatal error #4. (id_to_index == %ld)", id_to_index);
		return;
	}
	if (!(new_index = find_char_in_index(id_to_index)))  	/* name not already in index.. add it */
	{
		CREATE(new_index, mail_index_type, 1);
		new_index->recipient = id_to_index;
		new_index->list_start = NULL;

		/* add to front of list */
		new_index->next = mail_index;
		mail_index = new_index;
	}
	/* now, add this position to front of position list */
	CREATE(new_position, position_list_type, 1);
	new_position->position = pos;
	new_position->next = new_index->list_start;
	new_index->list_start = new_position;
}


/*
 * int scan_file(none)
 * Returns false if mail file is corrupted or true if everything correct.
 *
 * This is called once during boot-up.  It scans through the mail file
 * and indexes all entries currently in the mail file.
 */
int scan_file(void)
{
	FILE *mail_file;
	header_block_type next_block;
	int total_messages = 0, block_num = 0;

	if (!(mail_file = fopen(MAIL_FILE, "r")))
	{
		log("   Mail file non-existant... creating new file.");
		touch(MAIL_FILE);
		return (1);
	}
	while (fread(&next_block, sizeof(header_block_type), 1, mail_file))
	{
		if (next_block.block_type == HEADER_BLOCK)
		{
			index_mail(next_block.header_data.to, block_num * BLOCK_SIZE);
			total_messages++;
		}
		else if (next_block.block_type == DELETED_BLOCK)
			push_free_list(block_num * BLOCK_SIZE);
		block_num++;
	}

	file_end_pos = ftell(mail_file);
	fclose(mail_file);
	log("   %ld bytes read.", file_end_pos);
	if (file_end_pos % BLOCK_SIZE)
	{
		log("SYSERR: Error booting mail system -- Mail file corrupt!");
		log("SYSERR: Mail disabled!");
		return (0);
	}
	log("   Mail file read -- %d messages.", total_messages);
	return (1);
}				/* end of scan_file */


/*
 * int has_mail(long #1)
 * #1 - id number of the person to check for mail.
 * Returns true or false.
 *
 * A simple little function which tells you if the guy has mail or not.
 */
int has_mail(long recipient)
{
	return (find_char_in_index(recipient) != NULL);
}


/*
 * void store_mail(long #1, long #2, char * #3)
 * #1 - id number of the person to mail to.
 * #2 - id number of the person the mail is from.
 * #3 - The actual message to send.
 *
 * call store_mail to store mail.  (hard, huh? :-) )  Pass 3 arguments:
 * who the mail is to (long), who it's from (long), and a pointer to the
 * actual message text (char *).
 */
void store_mail(long to, long from, char *message_pointer)
{
	header_block_type header;
	data_block_type data;
	long last_address, target_address;
	char *msg_txt = message_pointer;
	int bytes_written, total_length = strlen(message_pointer);

	if ((sizeof(header_block_type) != sizeof(data_block_type)) || (sizeof(header_block_type) != BLOCK_SIZE))
	{
		log("SYSERR: store_mail...");
		//core_dump();
		return;
	}

	if (to < 0 || !*message_pointer)
	{
		log("SYSERR: Mail system -- non-fatal error #5. (from == %ld, to == %ld)", from, to);
		return;
	}
	memset((char *) &header, 0, sizeof(header));	/* clear the record */
	header.block_type = HEADER_BLOCK;
	header.header_data.next_block = LAST_BLOCK;
	header.header_data.from = from;
	header.header_data.to = to;
	header.header_data.mail_time = time(0);
	strncpy(header.txt, msg_txt, HEADER_BLOCK_DATASIZE);
	header.txt[HEADER_BLOCK_DATASIZE] = '\0';

	target_address = pop_free_list();	/* find next free block */
	index_mail(to, target_address);	/* add it to mail index in memory */
	write_to_file(&header, BLOCK_SIZE, target_address);

	if (strlen(msg_txt) <= HEADER_BLOCK_DATASIZE)
		return;		/* that was the whole message */

	bytes_written = HEADER_BLOCK_DATASIZE;
	msg_txt += HEADER_BLOCK_DATASIZE;	/* move pointer to next bit of text */

	/*
	 * find the next block address, then rewrite the header to reflect where
	 * the next block is.
	 */
	last_address = target_address;
	target_address = pop_free_list();
	header.header_data.next_block = target_address;
	write_to_file(&header, BLOCK_SIZE, last_address);

	/* now write the current data block */
	memset((char *) &data, 0, sizeof(data));	/* clear the record */
	data.block_type = LAST_BLOCK;
	strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE);
	data.txt[DATA_BLOCK_DATASIZE] = '\0';
	write_to_file(&data, BLOCK_SIZE, target_address);
	bytes_written += strlen(data.txt);
	msg_txt += strlen(data.txt);

	/*
	 * if, after 1 header block and 1 data block there is STILL part of the
	 * message left to write to the file, keep writing the new data blocks and
	 * rewriting the old data blocks to reflect where the next block is.  Yes,
	 * this is kind of a hack, but if the block size is big enough it won't
	 * matter anyway.  Hopefully, MUD players won't pour their life stories out
	 * into the Mud Mail System anyway.
	 *
	 * Note that the block_type data field in data blocks is either a number >=0,
	 * meaning a link to the next block, or LAST_BLOCK flag (-2) meaning the
	 * last block in the current message.  This works much like DOS' FAT.
	 */
	while (bytes_written < total_length)
	{
		last_address = target_address;
		target_address = pop_free_list();

		/* rewrite the previous block to link it to the next */
		data.block_type = target_address;
		write_to_file(&data, BLOCK_SIZE, last_address);

		/* now write the next block, assuming it's the last.  */
		data.block_type = LAST_BLOCK;
		strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE);
		data.txt[DATA_BLOCK_DATASIZE] = '\0';
		write_to_file(&data, BLOCK_SIZE, target_address);

		bytes_written += strlen(data.txt);
		msg_txt += strlen(data.txt);
	}
}				/* store mail */


/*
 * char *read_delete(long #1)
 * #1 - The id number of the person we're checking mail for.
 * Returns the message text of the mail received.
 *
 * Retrieves one messsage for a player_data. The mail is then discarded from
 * the file and the mail index.
 */
char *read_delete(long recipient)
{
	header_block_type header;
	data_block_type data;
	mail_index_type *mail_pointer, *prev_mail;
	position_list_type *position_pointer;
	long mail_address, following_block;
	char *message, *tmstr, buf[200];
	size_t string_size;

	if (recipient < 0)
	{
		log("SYSERR: Mail system -- non-fatal error #6. (recipient: %ld)", recipient);
		return (NULL);
	}
	if (!(mail_pointer = find_char_in_index(recipient)))
	{
		log("SYSERR: Mail system -- post office spec_proc error?  Error #7. (invalid character in index)");
		return (NULL);
	}
	if (!(position_pointer = mail_pointer->list_start))
	{
		log("SYSERR: Mail system -- non-fatal error #8. (invalid position pointer %p)", position_pointer);
		return (NULL);
	}
	if (!(position_pointer->next))  	/* just 1 entry in list. */
	{
		mail_address = position_pointer->position;
		free(position_pointer);

		/* now free up the actual name entry */
		if (mail_index == mail_pointer)  	/* name is 1st in list */
		{
			mail_index = mail_pointer->next;
			free(mail_pointer);
		}
		else  	/* find entry before the one we're going to del */
		{
			for (prev_mail = mail_index; prev_mail->next != mail_pointer; prev_mail = prev_mail->next);
			prev_mail->next = mail_pointer->next;
			free(mail_pointer);
		}
	}
	else  		/* move to next-to-last record */
	{
		while (position_pointer->next->next)
			position_pointer = position_pointer->next;
		mail_address = position_pointer->next->position;
		free(position_pointer->next);
		position_pointer->next = NULL;
	}

	/* ok, now lets do some readin'! */
	read_from_file(&header, BLOCK_SIZE, mail_address);

	if (header.block_type != HEADER_BLOCK)
	{
		log("SYSERR: Oh dear. (Header block %ld != %d)", header.block_type, HEADER_BLOCK);
		no_mail = TRUE;
		log("SYSERR: Mail system disabled!  -- Error #9. (Invalid header block.)");
		return (NULL);
	}
	tmstr = asctime(localtime(&header.header_data.mail_time));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	const char *from = header.header_data.from >= 0 ? get_name_by_id(header.header_data.from) : "Почтовая служба";
	const char *to = get_name_by_id(recipient);

	sprintf(buf, " * * * * Княжеская почта * * * *\r\n"
			"Дата: %s\r\n"
			"Кому: %s\r\n" "  От: %s\r\n\r\n", tmstr, to ? to : "Получатель", from ? from : "Неизвестно");

	string_size = (sizeof(char) * (strlen(buf) + strlen(header.txt) + 1));
	CREATE(message, char, string_size);
	strcpy(message, buf);
	strcat(message, header.txt);
	message[string_size - 1] = '\0';
	following_block = header.header_data.next_block;

	/* mark the block as deleted */
	header.block_type = DELETED_BLOCK;
	write_to_file(&header, BLOCK_SIZE, mail_address);
	push_free_list(mail_address);

	while (following_block != LAST_BLOCK)
	{
		read_from_file(&data, BLOCK_SIZE, following_block);

		string_size = (sizeof(char) * (strlen(message) + strlen(data.txt) + 1));
		RECREATE(message, char, string_size);
		strcat(message, data.txt);
		message[string_size - 1] = '\0';
		mail_address = following_block;
		following_block = data.block_type;
		data.block_type = DELETED_BLOCK;
		write_to_file(&data, BLOCK_SIZE, mail_address);
		push_free_list(mail_address);
	}

	return (message);
}


/****************************************************************
* Below is the spec_proc for a postmaster using the above       *
* routines.  Written by Jeremy Elson (jelson@circlemud.org) *
****************************************************************/

SPECIAL(postmaster)
{
	if (!ch->desc || IS_NPC(ch))
		return (0);	/* so mobs don't get caught here */

	if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")
			|| CMD_IS("почта") || CMD_IS("получить") || CMD_IS("отправить")
			|| CMD_IS("return") || CMD_IS("вернуть")))
		return (0);

	if (no_mail)
	{
		send_to_char("Извините, почтальон в запое.\r\n", ch);
		return (0);
	}

	if (CMD_IS("mail") || CMD_IS("отправить"))
	{
		postmaster_send_mail(ch, (CHAR_DATA *) me, cmd, argument);
		return (1);
	}
	else if (CMD_IS("check") || CMD_IS("почта"))
	{
		postmaster_check_mail(ch, (CHAR_DATA *) me, cmd, argument);
		return (1);
	}
	else if (CMD_IS("receive") || CMD_IS("получить"))
	{
		one_argument(argument, arg);
		if(is_abbrev(arg, "вещи")) {
			NamedStuff::receive_items(ch, (CHAR_DATA *) me);
		} else {
			postmaster_receive_mail(ch, (CHAR_DATA *) me, cmd, argument);
		}
		return (1);
	}
	else if (CMD_IS("return") || CMD_IS("вернуть"))
	{
		Parcel::bring_back(ch, (CHAR_DATA *) me);
		return (1);
	}
	else
		return (0);
}


void postmaster_send_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg)
{
	long recipient;
	int cost;
	char buf[256], **write;

	IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO) ? cost = 0 : cost = STAMP_PRICE;

	if (GET_LEVEL(ch) < MIN_MAIL_LEVEL)
	{
		sprintf(buf,
				"$n сказал$g вам, 'Извините, вы должны достигнуть %d уровня, чтобы отправить письмо!'",
				MIN_MAIL_LEVEL);
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	arg = one_argument(arg, buf);

	if (!*buf)  		/* you'll get no argument from me! */
	{
		act("$n сказал$g вам, 'Вы не указали адресата!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if ((recipient = get_id_by_name(buf)) < 0)
	{
		act("$n сказал$g вам, 'Извините, но такого игрока нет в игре!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	skip_spaces(&arg);
	if (*arg)
	{
		long vict_uid = GetUniqueByName(buf);
		if (vict_uid > 0)
		{
			Parcel::send(ch, mailman, vict_uid, arg);
		}
		else
		{
			act("$n сказал$g вам : 'Ошибочка вышла, сообщите Богам!'", FALSE, mailman, 0, ch, TO_VICT);
		}
		return;
	}

	if (ch->get_gold() < cost)
	{
		sprintf(buf, "$n сказал$g вам, 'Письмо стоит %d %s.'\r\n"
				"$n сказал$g вам, '...которых у вас просто-напросто нет.'",
				STAMP_PRICE, desc_count(STAMP_PRICE, WHAT_MONEYu));
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	act("$n начал$g писать письмо.", TRUE, ch, 0, 0, TO_ROOM);
	if (cost == 0)
		sprintf(buf, "$n сказал$g вам, 'Со своих - почтовый сбор не берем.'\r\n"
				"$n сказал$g вам, 'Можете писать, (/s saves /h for help)'");
	else
		sprintf(buf,
				"$n сказал$g вам, 'Отлично, с вас %d %s почтового сбора.'\r\n"
				"$n сказал$g вам, 'Можете писать, (/s saves /h for help)'",
				STAMP_PRICE, desc_count(STAMP_PRICE, WHAT_MONEYa));

	act(buf, FALSE, mailman, 0, ch, TO_VICT);
	ch->remove_gold(cost);
	SET_BIT(PLR_FLAGS(ch, PLR_MAILING), PLR_MAILING);	/* string_write() sets writing. */

	/* Start writing! */
	CREATE(write, char *, 1);
	string_write(ch->desc, write, MAX_MAIL_SIZE, recipient, NULL);
}


void postmaster_check_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg)
{
	bool empty = true;
	if (has_mail(GET_IDNUM(ch)))
	{
		empty = false;
		act("$n сказал$g вам : 'Вас ожидает почта.'", FALSE, mailman, 0, ch, TO_VICT);
	}
	if (Parcel::has_parcel(ch))
	{
		empty = false;
		act("$n сказал$g вам : 'Вас ожидает посылка.'", FALSE, mailman, 0, ch, TO_VICT);
	}

	if (empty)
		act("$n сказал$g вам : 'Похоже, сегодня вам ничего нет.'", FALSE, mailman, 0, ch, TO_VICT);

	Parcel::print_sending_stuff(ch);
}


void postmaster_receive_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg)
{
	char buf[256];
	OBJ_DATA *obj;

	if (!has_mail(GET_IDNUM(ch)) && !Parcel::has_parcel(ch))
	{
		sprintf(buf, "$n удивленно сказал$g вам : 'Но для вас нет писем !?'");
		act(buf, FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	while (has_mail(GET_IDNUM(ch)))
	{
		obj = create_obj();
		obj->aliases = str_dup("mail paper letter письмо почта бумага");
		obj->short_description = str_dup("письмо");
		obj->description = str_dup("Кто-то забыл здесь свое письмо.");
		obj->PNames[0] = str_dup("письмо");
		obj->PNames[1] = str_dup("письма");
		obj->PNames[2] = str_dup("письма");
		obj->PNames[3] = str_dup("письмо");
		obj->PNames[4] = str_dup("письмом");
		obj->PNames[5] = str_dup("письме");

		GET_OBJ_TYPE(obj) = ITEM_NOTE;
		GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE | ITEM_WEAR_HOLD;
		GET_OBJ_WEIGHT(obj) = 1;
		GET_OBJ_COST(obj) = 30;
		GET_OBJ_RENT(obj) = 10;
		obj->set_timer(24 * 60);
		obj->action_description = read_delete(GET_IDNUM(ch));

		if (obj->action_description == NULL)
			obj->action_description =
				str_dup("Почтовая система сломана - сообщите Богам.  Ошибка #11.\r\n");

		obj_to_char(obj, ch);

		act("$n дал$g вам письмо.", FALSE, mailman, 0, ch, TO_VICT);
		act("$N дал$G $n2 письмо.", FALSE, ch, 0, mailman, TO_ROOM);
	}
	Parcel::receive(ch, mailman);
}
