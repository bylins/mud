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

//******* MUD MAIL SYSTEM MAIN FILE ***************************************
// Written by Jeremy Elson (jelson@circlemud.org)
//*************************************************************************

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
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;
int find_name(const char *name);

mail_index_type *mail_index = NULL;	// list of recs in the mail file
position_list_type *free_list = NULL;	// list of free positions in file
long file_end_pos = 0;		// length of file

// local functions
void push_free_list(long pos);
long pop_free_list(void);
mail_index_type *find_char_in_index(long searchee);
void write_to_file(void *buf, int size, long filepos);
void read_from_file(void *buf, int size, long filepos);
void index_mail(long id_to_index, long pos);

// --------------------------------------------------------------------------

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

	// * If we don't have any free blocks, we append to the file.
	if ((old_pos = free_list) == NULL)
		return (file_end_pos);

	// Save the offset of the free block.
	return_value = free_list->position;
	// Remove this block from the free list.
	free_list = old_pos->next;
	// Get rid of the memory the node took.
	free(old_pos);
	// Give back the free offset.
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

	// find end of file
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
	if (!(new_index = find_char_in_index(id_to_index)))  	// name not already in index.. add it
	{
		CREATE(new_index, mail_index_type, 1);
		new_index->recipient = id_to_index;
		new_index->list_start = NULL;

		// add to front of list
		new_index->next = mail_index;
		mail_index = new_index;
	}
	// now, add this position to front of position list
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
}				// end of scan_file

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


//****************************************************************
//* Below is the spec_proc for a postmaster using the above      *
//* routines.  Written by Jeremy Elson (jelson@circlemud.org)    *
//****************************************************************

SPECIAL(postmaster)
{
	if (!ch->desc || IS_NPC(ch))
		return (0);	// so mobs don't get caught here

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

void postmaster_check_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg)
{
	bool empty = true;
	if (mail::has_mail(ch->get_uid()))
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
	{
		act("$n сказал$g вам : 'Похоже, сегодня вам ничего нет.'",
			FALSE, mailman, 0, ch, TO_VICT);
	}
	Parcel::print_sending_stuff(ch);
}


void postmaster_receive_mail(CHAR_DATA* ch, CHAR_DATA* mailman, int cmd, char* arg)
{
	if (!Parcel::has_parcel(ch) && !mail::has_mail(ch->get_uid()))
	{
		act("$n удивленно сказал$g вам : 'Но для вас нет писем!?'",
			FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	Parcel::receive(ch, mailman);
	mail::receive(ch, mailman);
}

extern struct player_index_element *player_table;
extern int top_of_p_table;

#include "conf.h"
#include <string>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "pugixml.hpp"
#include "db.h"
#include "parse.hpp"
#include "screen.h"

namespace coder
{

static const std::string base64_chars =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"0123456789+/";

static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const char* in_str, size_t in_len)
{
	const unsigned char* bytes_to_encode =
		reinterpret_cast<const unsigned char*>(in_str);
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';

	}

	return ret;

}

std::string base64_decode(std::string const& encoded_string)
{
	size_t in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i ==4)
		{
			for (i = 0; i <4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

std::string to_iso8601(time_t time)
{
	char buf_[MAX_INPUT_LENGTH];
	strftime(buf_, sizeof(buf_), "%FT%T", localtime(&time));
	return buf_;
}

time_t from_iso8601(const std::string& str)
{
	std::tm tm_time;
	int parsed = sscanf(str.c_str(), "%d-%d-%dT%d:%d:%d",
		&tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
		&tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec);

	time_t time = 0;
	if (parsed == 6)
	{
		tm_time.tm_mon -= 1;
		tm_time.tm_year -= 1900;
		tm_time.tm_isdst = -1;
		time = std::mktime(&tm_time);
	}
	return time;
}

} // namespace coder

namespace mail
{

const char* MAIL_XML_FILE = LIB_ETC"plrmail.xml";
// макс сообщений в доставке от безмортовых чаров до NAME_LEVEL уровня
const int LOW_LVL_MAX_POST = 10;
// тоже самое для всех остальных
const int HIGH_LVL_MAX_POST = 100;

struct mail_node
{
	mail_node() : from(-1), date(0) {};

	// уид автора (может быть невалидным)
	int from;
	// дата написания письма
	time_t date;
	// текст письма в перемешанном base64
	std::string text;
	// (дата в iso8601 + автор + получатель) в перемешанном base64
	std::string header;
};

// флаг для избегания пустых автосохранений
bool need_save = false;
// уид получателя -> письмо
std::unordered_multimap<int, mail_node> mail_list;
// уид -> кол-во отправленных сообщений в бд (спам-контроль)
std::unordered_map<int, int> poster_list;
// уид - условный список чаров (с дальнейшей проверкой), которым нужно
// единоразово вывести уведомление про новую почту (спам-контроль)
std::unordered_set<int> notice_list;

void add_notice(int uid)
{
	notice_list.insert(uid);
}

void print_notices()
{
	for(auto i = notice_list.begin(); i != notice_list.end(); ++i)
	{
		if (!has_mail(*i))
		{
			continue;
		}
		DESCRIPTOR_DATA* d = DescByUID(*i);
		if (d)
		{
			send_to_char(d->character,
				"%sВам пришло письмо, зайдите на почту и распишитесь!%s\r\n",
				CCWHT(d->character, C_NRM), CCNRM(d->character, C_NRM));
		}
	}
	notice_list.clear();
}

void add_poster(int uid)
{
	if (uid < 0) return;

	auto i = poster_list.find(uid);
	if (i != poster_list.end())
	{
		i->second += 1;
	}
	else
	{
		poster_list[uid] = 1;
	}
}

void sub_poster(int uid)
{
	if (uid < 0) return;

	auto i = poster_list.find(uid);
	if (i != poster_list.end())
	{
		i->second -= 1;
		if (i->second <= 0)
		{
			poster_list.erase(i);
		}
	}
}

bool check_poster_cnt(CHAR_DATA* ch)
{
	auto i = poster_list.find(ch->get_uid());
	if (i != poster_list.end())
	{
		if (GET_REMORT(ch) <= 0
			&& GET_LEVEL(ch) <= NAME_LEVEL
			&& i->second >= LOW_LVL_MAX_POST)
		{
				return false;
		}
		if (i->second >= HIGH_LVL_MAX_POST)
		{
			return false;
		}
	}
	return true;
}

void add(int to_uid, int from_uid, char* message)
{
	if (to_uid < 0 || !message || !*message)
	{
		return;
	}

	mail_node node;
	node.from = from_uid;
	node.date = time(0);
	node.text = coder::base64_encode(message, strlen(message));

	char buf_[MAX_STRING_LENGTH];
	snprintf(buf_, sizeof(buf_), "%s %d %d",
		coder::to_iso8601(node.date).c_str(), from_uid, to_uid);
	node.header = coder::base64_encode(buf_, strlen(buf_));

	mail_list.insert(std::make_pair(to_uid, node));
	add_poster(from_uid);
	need_save = true;
}

void add_by_id(int to_id, int from_id, char* message)
{
	const int to_uid = get_uid_by_id(to_id);
	const int from_uid = from_id >= 0 ? get_uid_by_id(from_id) : from_id;

	add(to_uid, from_uid, message);
}

bool has_mail(int uid)
{
	return (mail_list.find(uid) != mail_list.end());
}

std::string get_author_name(int uid)
{
	std::string out;
	if (uid == -1)
	{
		out = "Почтовая служба";
	}
	else if (uid == -2)
	{
		out = "<персонаж был удален>";
	}
	else if (uid < 0)
	{
		out = "Неизвестно";
	}
	else
	{
		const char* name = get_name_by_unique(uid);
		if (name)
		{
			out = name;
			name_convert(out);
		}
		else
		{
			out = "<персонаж был удален>";
		}
	}
	return out;
}

void receive(CHAR_DATA* ch, CHAR_DATA* mailman)
{
	auto rng = mail_list.equal_range(ch->get_uid());
	for(auto i = rng.first; i != rng.second; ++i)
	{
		OBJ_DATA *obj = create_obj();
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
		GET_OBJ_MATER(obj) = MAT_PAPER;
		obj->set_cost(0);
		obj->set_rent(10);
		obj->set_rent_eq(10);
		obj->set_timer(24 * 60);

		char buf_date[MAX_INPUT_LENGTH];
		strftime(buf_date, sizeof(buf_date),
			"%H:%M %d-%m-%Y", localtime(&i->second.date));

		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_),
			" * * * * Княжеская почта * * * *\r\n"
			"Дата: %s\r\n"
			"Кому: %s\r\n"
			"  От: %s\r\n\r\n",
			buf_date, ch->get_name(), get_author_name(i->second.from).c_str());

		std::string text = coder::base64_decode(i->second.text);
		boost::trim_if(text, ::isspace);
		obj->action_description = str_dup((buf_ + text + "\r\n\r\n").c_str());

		obj_to_char(obj, ch);
		act("$n дал$g вам письмо.", FALSE, mailman, 0, ch, TO_VICT);
		act("$N дал$G $n2 письмо.", FALSE, ch, 0, mailman, TO_ROOM);

		sub_poster(i->second.from);
	}
	mail_list.erase(rng.first, rng.second);
	need_save = true;
}

void save()
{
	print_notices();

	if (!need_save) return;

	pugi::xml_document doc;
	doc.append_child().set_name("mail");
	pugi::xml_node mail_n = doc.child("mail");

	for (auto i = mail_list.cbegin(), iend = mail_list.cend(); i != iend; ++i)
	{
		pugi::xml_node msg_n = mail_n.append_child();
		msg_n.set_name("m");
		msg_n.append_attribute("h") = i->second.header.c_str();
		msg_n.append_attribute("t") = i->second.text.c_str();
	}

	doc.save_file(MAIL_XML_FILE);
	need_save = false;
}

void load()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MAIL_XML_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node mail_n = doc.child("mail");
    if (!mail_n)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<mail> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }

	for (pugi::xml_node msg_n = mail_n.child("m");
		msg_n; msg_n = msg_n.next_sibling("m"))
	{
		mail_node message;
		message.header = Parse::attr_str(msg_n, "h");
		message.text = Parse::attr_str(msg_n, "t");
		// раскодируем строку хедера
		std::string header = coder::base64_decode(message.header);
		int to_uid = -1;
		// вытаскиваем дату, отправителя и получателя
		std::vector<std::string> param_list;
		boost::split(param_list, header, boost::is_any_of(" "));
		if (param_list.size() == 3)
		{
			message.date = coder::from_iso8601(param_list[0]);
			try
			{
				message.from = boost::lexical_cast<int>(param_list[1]);
				to_uid = boost::lexical_cast<int>(param_list[2]);
			}
			catch (boost::bad_lexical_cast&)
			{
				log("SYSERROR: ошибка чтения mail header #1 (%s)",
					header.c_str());
				continue;
			}
		}
		else
		{
			log("SYSERROR: ошибка чтения mail header #2 (%s)",
				header.c_str());
			continue;
		}
		// проверяем, чего распарсили в хедере
		if (!get_name_by_unique(to_uid))
		{
			// адресата больше нет
			continue;
		}
		if (message.from == -1 && time(0) - message.date > 60 * 60 * 24 * 365)
		{
			// технические сообщения старше года
			continue;
		}
		if (message.from > 0 && !get_name_by_unique(message.from))
		{
			// убираем левые уиды, чтобы потом с кем-нить другим не совпало
			message.from = -2;
		}
		mail_list.insert(std::make_pair(to_uid, message));
		add_poster(message.from);
	}
	need_save = true;
}

void convert_msg(int recipient)
{
	header_block_type header;
	data_block_type data;
	mail_index_type *mail_pointer, *prev_mail;
	position_list_type *position_pointer;
	long mail_address, following_block;
	char *message;
	size_t string_size;

	if (recipient < 0)
	{
		log("SYSERR: Mail system -- non-fatal error #6. (recipient: %d)", recipient);
		return;
	}
	if (!(mail_pointer = find_char_in_index(recipient)))
	{
		log("SYSERR: Mail system -- post office spec_proc error?  Error #7. (invalid character in index)");
		return;
	}
	if (!(position_pointer = mail_pointer->list_start))
	{
		log("SYSERR: Mail system -- non-fatal error #8. (invalid position pointer %p)", position_pointer);
		return;
	}
	if (!(position_pointer->next))  	// just 1 entry in list.
	{
		mail_address = position_pointer->position;
		free(position_pointer);

		// now free up the actual name entry
		if (mail_index == mail_pointer)  	// name is 1st in list
		{
			mail_index = mail_pointer->next;
			free(mail_pointer);
		}
		else  	// find entry before the one we're going to del
		{
			for (prev_mail = mail_index; prev_mail->next != mail_pointer; prev_mail = prev_mail->next);
			prev_mail->next = mail_pointer->next;
			free(mail_pointer);
		}
	}
	else  		// move to next-to-last record
	{
		while (position_pointer->next->next)
			position_pointer = position_pointer->next;
		mail_address = position_pointer->next->position;
		free(position_pointer->next);
		position_pointer->next = NULL;
	}

	// ok, now lets do some readin'! //
	read_from_file(&header, BLOCK_SIZE, mail_address);

	if (header.block_type != HEADER_BLOCK)
	{
		log("SYSERR: Oh dear. (Header block %ld != %d)", header.block_type, HEADER_BLOCK);
		no_mail = TRUE;
		log("SYSERR: Mail system disabled!  -- Error #9. (Invalid header block.)");
		return;
	}

	const int from_id = header.header_data.from;
	const int to_id = header.header_data.to;
	const time_t date = header.header_data.mail_time;

	string_size = (sizeof(char) * (strlen(header.txt) + 1));
	CREATE(message, char, string_size);
	strcpy(message, header.txt);
	message[string_size - 1] = '\0';
	following_block = header.header_data.next_block;

	// mark the block as deleted //
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

	mail_node node;
	if (from_id < 0)
	{
		node.from = -1;
	}
	else
	{
		node.from = get_uid_by_id(from_id);
		if (node.from < 0)
		{
			node.from = -2;
		}
	}
	node.date = date;
	node.text = coder::base64_encode(message, strlen(message));

	char buf_[MAX_STRING_LENGTH];
	snprintf(buf_, sizeof(buf_), "%s %d %d",
		coder::to_iso8601(node.date).c_str(), node.from, get_uid_by_id(to_id));
	node.header = coder::base64_encode(buf_, strlen(buf_));

	mail_list.insert(std::make_pair(get_uid_by_id(to_id), node));
}

void convert()
{
	for (int i = 0; i <= top_of_p_table; i++)
	{
		while(::has_mail(player_table[i].id))
		{
			convert_msg(player_table[i].id);
		}
	}
	need_save = true;
	save();
	// чистка
	mail_list.clear();
	poster_list.clear();
	load();
	need_save = true;
	save();
}

size_t get_msg_count()
{
	return mail_list.size();
}

} // namespace mail

void postmaster_send_mail(CHAR_DATA * ch, CHAR_DATA * mailman, int cmd, char *arg)
{
	int recipient;
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
	if (!mail::check_poster_cnt(ch))
	{
		act("$n сказал$g вам, 'Извините, вы уже отправили максимальное кол-во сообщений!'",
			FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	arg = one_argument(arg, buf);

	if (!*buf)  		// you'll get no argument from me!
	{
		act("$n сказал$g вам, 'Вы не указали адресата!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}
	if ((recipient = GetUniqueByName(buf)) <= 0)
	{
		act("$n сказал$g вам, 'Извините, но такого игрока нет в игре!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	skip_spaces(&arg);
	if (*arg)
	{
		if ((IN_ROOM(ch) == r_helled_start_room) ||
			(IN_ROOM(ch) == r_named_start_room) ||
			(IN_ROOM(ch) == r_unreg_start_room))
		{
			act("$n сказал$g вам : 'Посылку? Не положено!'", FALSE, mailman, 0, ch, TO_VICT);
			return;
		}
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
	SET_BIT(PLR_FLAGS(ch, PLR_MAILING), PLR_MAILING);	// string_write() sets writing.

	// Start writing!
	CREATE(write, char *, 1);
	string_write(ch->desc, write, MAX_MAIL_SIZE, recipient, NULL);
}
