/* ************************************************************************
*   File: modify.cpp                                      Part of Bylins    *
*  Usage: Run-time modification of game variables                         *
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

#include "modify.h"

#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spell_parser.hpp"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "screen.h"
#include "olc.h"
#include "features.hpp"
#include "house.h"
#include "privilege.hpp"
#include "char.hpp"
#include "skills.h"
#include "genchar.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

void show_string(DESCRIPTOR_DATA * d, char *input);

#define PARSE_FORMAT      0
#define PARSE_REPLACE     1
#define PARSE_HELP        2
#define PARSE_DELETE      3
#define PARSE_INSERT      4
#define PARSE_LIST_NORM   5
#define PARSE_LIST_NUM    6
#define PARSE_EDIT        7

extern const char *MENU;
extern const char *unused_spellname;	// spell_parser.cpp

// local functions
void smash_tilde(char *str);
void do_skillset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
char *next_page(char *str, CHAR_DATA * ch);
int count_pages(char *str, CHAR_DATA * ch);
void paginate_string(char *str, DESCRIPTOR_DATA * d);

const char *string_fields[] =
{
	"name",
	"short",
	"long",
	"description",
	"title",
	"delete-description",
	"\n"
};


// maximum length for text field x+1
int length[] =
{
	15,
	60,
	256,
	240,
	60
};


// ************************************************************************
// *  modification of malloc'ed strings                                   *
// ************************************************************************

/*
 * Put '#if 1' here to erase ~, or roll your own method.  A common idea
 * is smash/show tilde to convert the tilde to another innocuous character
 * to save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP.
 *   -gg 9/9/98
 */
void smash_tilde(char *str)
{
	/*
	 * Erase any ~'s inserted by people in the editor.  This prevents anyone
	 * using online creation from causing parse errors in the world files.
	 * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
	 * his tildes thank you very much.), -gg 2/20/98
	 */
	while ((str = strchr(str, '~')) != NULL)
		* str = ' ';
}

/*
 * Basic API function to start writing somewhere.
 *
 * 'data' isn't used in stock CircleMUD but you can use it to pass whatever
 * else you may want through it.  The improved editor patch when updated
 * could use it to pass the old text buffer, for instance.
 */
void string_write(DESCRIPTOR_DATA * d, const AbstractStringWriter::shared_ptr& writer, size_t len, int mailto, void *data)
{
	if (d->character && !IS_NPC(d->character))
	{
		PLR_FLAGS(d->character).set(PLR_WRITING);
	}

	if (data)
	{
		mudlog("SYSERR: string_write: I don't understand special data.", BRF, LVL_IMMORT, SYSLOG, TRUE);
	}

	d->writer = writer;
	d->max_str = len;
	d->mail_to = mailto;
}


// * Handle some editor commands.
void parse_action(int command, char *string, DESCRIPTOR_DATA * d)
{
	int indent = 0, rep_all = 0, flags = 0, replaced;
	int j = 0;
	int i, line_low, line_high;
	char *s, *t;
	//log("[PA] Start %d(%s)", command, string);
	switch (command)
	{
	case PARSE_HELP:
		sprintf(buf,
				"Формат команд редактора: /<letter>\r\n\r\n"
				"/a         -  прекратить редактирование\r\n"
				"/c         -  очистить буфер\r\n"
				"/d#,       -  удалить строку #\r\n"
				"/e# <text> -  заменить строку # на текст <text>\r\n"
				"/f         -  форматировать текст\r\n"
				"/fi        -  indented formatting of text\r\n"
				"/h         -  отобразить список команд (помощь)\r\n"
				"/i# <text> -  вставить <text> после строки #\r\n"
				"/l         -  пролистать буфер\r\n"
				"/n         -  пролистать буфер с номерами строк\r\n"
				"/r 'a' 'b' -  заменить первое вхождение текста <a> в буфере на текст <b>\r\n"
				"/ra 'a' 'b'-  заменить все вхождения текста <a> в буфере на текст <b>\r\n"
				"              Формат: /r[a] 'шаблон' 'на_что_меняем'\r\n" "/s         -  сохранить текст\r\n");
		SEND_TO_Q(buf, d);
		break;

	case PARSE_FORMAT:
		while (isalpha(string[j]) && j < 2)
		{
			switch (string[j])
			{
			case 'i':
				if (!indent)
				{
					indent = TRUE;
					flags += FORMAT_INDENT;
				}
				break;
			default:
				break;
			}
			j++;
		}

		format_text(d->writer, flags, d, d->max_str);

		sprintf(buf, "Текст отформатирован %s\r\n", (indent ? "WITH INDENT." : "."));
		SEND_TO_Q(buf, d);
		break;

	case PARSE_REPLACE:
		while (isalpha(string[j]) && j < 2)
		{
			switch (string[j])
			{
			case 'a':
				if (!indent)
					rep_all = 1;
				break;
			default:
				break;
			}
			j++;
		}
		if ((s = strtok(string, "'")) == NULL)
		{
			SEND_TO_Q("Неверный формат.\r\n", d);
			return;
		}
		else if ((s = strtok(NULL, "'")) == NULL)
		{
			SEND_TO_Q("Шаблон должен быть заключен в апострофы.\r\n", d);
			return;
		}
		else if ((t = strtok(NULL, "'")) == NULL)
		{
			SEND_TO_Q("No replacement string.\r\n", d);
			return;
		}
		else if ((t = strtok(NULL, "'")) == NULL)
		{
			SEND_TO_Q("Замещающая строка должна быть заключена в апострофы.\r\n", d);
			return;
		}
		else
		{
			size_t total_len = strlen(t) + strlen(d->writer->get_string()) - strlen(s);
			if (total_len <= d->max_str)
			{
				replaced = replace_str(d->writer, s, t, rep_all, static_cast<int>(d->max_str));
				if (replaced > 0)
				{
					sprintf(buf, "Заменено вхождений '%s' на '%s' - %d.\r\n", s, t, replaced);
					SEND_TO_Q(buf, d);
				}
				else if (replaced == 0)
				{
					sprintf(buf, "Шаблон '%s' не найден.\r\n", s);
					SEND_TO_Q(buf, d);
				}
				else
				{
					SEND_TO_Q("ОШИБКА: При попытке замены буфер переполнен - прервано.\r\n", d);
				}
			}
			else
			{
				SEND_TO_Q("Нет свободного места для завершения команды.\r\n", d);
			}
		}
		break;

	case PARSE_DELETE:
		switch (sscanf(string, " %d - %d ", &line_low, &line_high))
		{
		case 0:
			SEND_TO_Q("Вы должны указать номер строки или диапазон для удаления.\r\n", d);
			return;
		case 1:
			line_high = line_low;
			break;
		case 2:
			if (line_high < line_low)
			{
				SEND_TO_Q("Неверный диапазон.\r\n", d);
				return;
			}
			break;
		}

		i = 1;
		{
			char* buffer = str_dup(d->writer->get_string());
			std::shared_ptr<char> guard(buffer, free);
			s = buffer;
			if (s == nullptr)
			{
				SEND_TO_Q("Буфер пуст.\r\n", d);
				return;
			}
			else if (line_low > 0)
			{
				unsigned int total_len = 1;
				while (s && (i < line_low))
				{
					if ((s = strchr(s, '\n')) != NULL)
					{
						i++;
						s++;
					}
				}
				if ((i < line_low) || (s == NULL))
				{
					SEND_TO_Q("Строка(и) вне диапазона - проигнорировано.\r\n", d);
					return;
				}
				t = s;
				while (s && (i < line_high))
				{
					if ((s = strchr(s, '\n')) != NULL)
					{
						i++;
						total_len++;
						s++;
					}
				}
				if ((s) && ((s = strchr(s, '\n')) != NULL))
				{
					s++;
					while (*s != '\0')
					{
						*(t++) = *(s++);
					}
				}
				else
				{
					total_len--;
				}
				*t = '\0';
				d->writer->set_string(buffer);

				sprintf(buf, "%u line%sdeleted.\r\n", total_len, ((total_len != 1) ? "s " : " "));
				SEND_TO_Q(buf, d);
			}
			else
			{
				SEND_TO_Q("Отрицательный или нулевой номер строки для удаления.\r\n", d);
				return;
			}
		}
		break;

	case PARSE_LIST_NORM:
		// * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
		// * they are probly ok for what to do here.
		*buf = '\0';
		if (*string != '\0')
			switch (sscanf(string, " %d - %d ", &line_low, &line_high))
			{
			case 0:
				line_low = 1;
				line_high = 999999;
				break;
			case 1:
				line_high = line_low;
				break;
			}
		else
		{
			line_low = 1;
			line_high = 999999;
		}

		if (line_low < 1)
		{
			SEND_TO_Q("Начальный номер отрицательный или 0.\r\n", d);
			return;
		}
		else if (line_high < line_low)
		{
			SEND_TO_Q("Неверный диапазон.\r\n", d);
			return;
		}
		*buf = '\0';
		if ((line_high < 999999) || (line_low > 1))
			sprintf(buf, "Текущий диапазон [%d - %d]:\r\n", line_low, line_high);
		i = 1;
		{
			const char* pos = d->writer->get_string();

			unsigned int total_len = 0;
			while (pos && (i < line_low))
			{
				if ((pos = strchr(pos, '\n')) != NULL)
				{
					i++;
					pos++;
				}
			}

			if ((i < line_low) || (pos == NULL))
			{
				SEND_TO_Q("Строка(и) вне диапазона - проигнорировано.\r\n", d);
				return;
			}

			const char* beginning = pos;
			while (pos && (i <= line_high))
			{
				if ((pos = strchr(pos, '\n')) != NULL)
				{
					i++;
					total_len++;
					pos++;
				}
			}

			if (pos)
			{
				strncat(buf, beginning, pos - beginning);
			}
			else
			{
				strcat(buf, beginning);
			}

			page_string(d, buf, TRUE);
		}

		break;

	case PARSE_LIST_NUM:
		// * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
		// * they are probly ok for what to do here.
		*buf = '\0';
		if (*string != '\0')
		{
			switch (sscanf(string, " %d - %d ", &line_low, &line_high))
			{
			case 0:
				line_low = 1;
				line_high = 999999;
				break;
			case 1:
				line_high = line_low;
				break;
			}
		}
		else
		{
			line_low = 1;
			line_high = 999999;
		}

		if (line_low < 1)
		{
			SEND_TO_Q("Номер строки должен быть больше 0.\r\n", d);
			return;
		}
		if (line_high < line_low)
		{
			SEND_TO_Q("Неверный диапазон.\r\n", d);
			return;
		}
		*buf = '\0';
		i = 1;
		{
			const char* pos = d->writer->get_string();
			unsigned int total_len = 0;

			while (pos && (i < line_low))
			{
				if ((pos = strchr(pos, '\n')) != NULL)
				{
					i++;
					pos++;
				}
			}

			if ((i < line_low) || (pos == NULL))
			{
				SEND_TO_Q("Строка(и) вне диапазона - проигнорировано.\r\n", d);
				return;
			}

			const char* beginning = pos;
			while (pos && (i <= line_high))
			{
				if ((pos = strchr(pos, '\n')) != NULL)
				{
					i++;
					total_len++;
					pos++;
					sprintf(buf, "%s%4d:\r\n", buf, (i - 1));
					strncat(buf, beginning, pos - beginning);
					beginning = pos;
				}
			}

			if (pos && beginning)
			{
				strncat(buf, beginning, pos - beginning);
			}
			else if (beginning)
			{
				strcat(buf, beginning);
			}
		}

		page_string(d, buf, TRUE);
		break;

	case PARSE_INSERT:
		half_chop(string, buf, buf2);
		if (*buf == '\0')
		{
			SEND_TO_Q("Вы должны указать номер строки, после которой вставить текст.\r\n", d);
			return;
		}
		line_low = atoi(buf);
		strcat(buf2, "\r\n");

		i = 1;
		*buf = '\0';
		{
			const char* pos = d->writer->get_string();
			const char* beginning = pos;
			if (pos == NULL)
			{
				SEND_TO_Q("Буфер пуст - ничего не вставлено.\r\n", d);
				return;
			}
			if (line_low > 0)
			{
				while (pos && (i < line_low))
				{
					if ((pos = strchr(pos, '\n')) != NULL)
					{
						i++;
						pos++;
					}
				}
				if ((i < line_low) || (pos == NULL))
				{
					SEND_TO_Q("Номер строки вне диапазона - прервано.\r\n", d);
					return;
				}
				if ((pos - beginning + strlen(buf2) + strlen(pos) + 3) > d->max_str)
				{
					SEND_TO_Q("Превышение размеров буфера - прервано.\r\n", d);
					return;
				}
				if (beginning && (*beginning != '\0'))
				{
					strncat(buf, beginning, pos - beginning);
				}
				strcat(buf, buf2);
				if (*pos != '\0')
				{
					strcat(buf, pos);
				}
				d->writer->set_string(buf);
				SEND_TO_Q("Строка вставлена.\r\n", d);
			}
			else
			{
				SEND_TO_Q("Номер строки должен быть больше 0.\r\n", d);
				return;
			}
		}
		break;

	case PARSE_EDIT:
		half_chop(string, buf, buf2);
		if (*buf == '\0')
		{
			SEND_TO_Q("Вы должны указать номер строки изменяемого текста.\r\n", d);
			return;
		}
		line_low = atoi(buf);
		strcat(buf2, "\r\n");

		i = 1;
		*buf = '\0';
		{
			const char* s = d->writer->get_string();
			const char* beginning = s;
			if (s == nullptr)
			{
				SEND_TO_Q("Буфер пуст - изменения не проведены.\r\n", d);
				return;
			}

			if (line_low > 0)
			{	// Loop through the text counting \\n characters until we get to the line/
				while (s && (i < line_low))
				{
					if ((s = strchr(s, '\n')) != NULL)
					{
						i++;
						s++;
					}
				}

				// * Make sure that there was a THAT line in the text.
				if ((i < line_low) || (s == NULL))
				{
					SEND_TO_Q("Строка вне диапазона - прервано.\r\n", d);
					return;
				}

				// If s is the same as *d->str that means I'm at the beginning of the
				// message text and I don't need to put that into the changed buffer.
				if (s != beginning)
				{	// First things first .. we get this part into the buffer.
					// Put the first 'good' half of the text into storage.
					strncat(buf, beginning, s - beginning);
				}
				// Put the new 'good' line into place.
				strcat(buf, buf2);
				if ((s = strchr(s, '\n')) != NULL)
				{
					/*
					* This means that we are at the END of the line, we want out of
					* there, but we want s to point to the beginning of the line
					* AFTER the line we want edited
					*/
					s++;
					// * Now put the last 'good' half of buffer into storage.
					strcat(buf, s);
				}

				// * Check for buffer overflow.
				if (strlen(buf) > d->max_str)
				{
					SEND_TO_Q("Превышение максимального размера буфера - прервано.\r\n", d);
					return;
				}

				// * Change the size of the REAL buffer to fit the new text.
				d->writer->set_string(buf);
				SEND_TO_Q("Строка изменена.\r\n", d);
			}
			else
			{
				SEND_TO_Q("Номер строки должен быть больше 0.\r\n", d);
				return;
			}
		}
		break;

	default:
		SEND_TO_Q("Неверная опция.\r\n", d);
		mudlog("SYSERR: invalid command passed to parse_action", BRF, LVL_IMPL, SYSLOG, TRUE);
		return;
	}
	//log("[PA] Stop");
}




// Add user input to the 'current' string (as defined by d->str) //
void string_add(DESCRIPTOR_DATA * d, char *str)
{
	int terminator = 0, action = 0;
	int i = 2, j = 0;
	char actions[MAX_INPUT_LENGTH];

	// determine if this is the terminal string, and truncate if so //
	// changed to only accept '@' at the beginning of line - J. Elson 1/17/94 //
	// Changed to accept '/<letter>' style editing commands - instead //
	//   of solitary '@' to end. - M. Scott 10/15/96 //

	delete_doubledollar(str);

#if 0
	// Removed old handling of '@' character, put #if 1 to re-enable it. //
	if ((terminator = (*str == '@')))
		* str = '\0';
#endif

	// почту логировать как-то не оно
	if (d->character && !PLR_FLAGGED(d->character, PLR_MAILING))
		log("[SA] <%s> adds string '%s'", GET_NAME(d->character), str);

	smash_tilde(str);
	if (!d->writer)
	{
		return;
	}

	if ((action = (*str == '/')))
	{
		while (str[i] != '\0')
		{
			actions[j] = str[i];
			i++;
			j++;
		}
		actions[j] = '\0';
		*str = '\0';
		switch (str[1])
		{
		case 'a':
			terminator = 2;	// Working on an abort message, //
			break;
		case 'c':
			if (d->writer->get_string())
			{
				d->writer->clear();
				SEND_TO_Q("Буфер очищен.\r\n", d);
			}
			else
				SEND_TO_Q("Буфер пуст.\r\n", d);
			break;
		case 'd':
			parse_action(PARSE_DELETE, actions, d);
			break;
		case 'e':
			parse_action(PARSE_EDIT, actions, d);
			break;
		case 'f':
			if (d->writer->get_string())
			{
				parse_action(PARSE_FORMAT, actions, d);
			}
			else
			{
				SEND_TO_Q("Буфер пуст.\r\n", d);
			}
			break;
		case 'i':
			if (d->writer->get_string())
			{
				parse_action(PARSE_INSERT, actions, d);
			}
			else
			{
				SEND_TO_Q("Буфер пуст.\r\n", d);
			}
			break;
		case 'h':
			parse_action(PARSE_HELP, actions, d);
			break;
		case 'l':
			if (d->writer->get_string())
			{
				parse_action(PARSE_LIST_NORM, actions, d);
			}
			else
			{
				SEND_TO_Q("Буфер пуст.\r\n", d);
			}
			break;
		case 'n':
			if (d->writer->get_string())
			{
				parse_action(PARSE_LIST_NUM, actions, d);
			}
			else
			{
				SEND_TO_Q("Буфер пуст.\r\n", d);
			}
			break;
		case 'r':
			parse_action(PARSE_REPLACE, actions, d);
			break;
		case 's':
			terminator = 1;
			*str = '\0';
			break;
		default:
			SEND_TO_Q("Интересный выбор. Я о нем подумаю...\r\n", d);
			break;
		}
	}

	if (!d->writer->get_string())
	{
		if (strlen(str) + 3 > d->max_str)
		{
			send_to_char("Слишком длинная строка - усечена.\r\n", d->character.get());
			strcpy(&str[d->max_str - 3], "\r\n");
			d->writer->set_string(str);
		}
		else if (CON_WRITE_MOD == STATE(d) && strlen(str) + 3 > 80)
		{
			send_to_char("Слишком длинная строка - усечена.\r\n", d->character.get());
			str[80 - 3] = '\0';
			d->writer->set_string(str);
		}
		else
		{
			d->writer->set_string(str);
		}
	}
	else
	{
		if (CON_WRITE_MOD == STATE(d) && strlen(str) + 3 > 80)
		{
			send_to_char("Слишком длинная строка - усечена.\r\n", d->character.get());
			str[80 - 3] = '\0';
		}

		if (strlen(str) + d->writer->length() + 3 > d->max_str)  	// \r\n\0 //
		{
			send_to_char(d->character.get(),
				"Слишком длинное послание > %lu симв. Последняя строка проигнорирована.\r\n",
				d->max_str - 3);
			action = TRUE;
		}
		else
		{
			d->writer->append_string(str);
		}
	}

	if (terminator)
	{	// OLC Edits
		extern void oedit_disp_menu(DESCRIPTOR_DATA * d);
		extern void oedit_disp_extradesc_menu(DESCRIPTOR_DATA * d);
		extern void redit_disp_menu(DESCRIPTOR_DATA * d);
		extern void redit_disp_extradesc_menu(DESCRIPTOR_DATA * d);
		extern void redit_disp_exit_menu(DESCRIPTOR_DATA * d);
		extern void medit_disp_menu(DESCRIPTOR_DATA * d);
		extern void trigedit_disp_menu(DESCRIPTOR_DATA * d);

#if defined(OASIS_MPROG)
		extern void medit_change_mprog(DESCRIPTOR_DATA * d);

		if (STATE(d) == CON_MEDIT)
		{
			switch (OLC_MODE(d))
			{
			case MEDIT_D_DESC:
				medit_disp_menu(d);
				break;
			case MEDIT_MPROG_COMLIST:
				medit_change_mprog(d);
				break;
			}
		}
#endif

		// * Here we check for the abort option and reset the pointers.
		if ((terminator == 2) && ((STATE(d) == CON_REDIT) || (STATE(d) == CON_MEDIT) || (STATE(d) == CON_OEDIT) || (STATE(d) == CON_TRIGEDIT) || (STATE(d) == CON_EXDESC)))  	//log("[SA] 2s");
		{
			if (d->backstr)
			{
				d->writer->set_string(d->backstr);
				free(d->backstr);
			}
			else
			{
				d->writer->clear();
			}
			d->backstr = nullptr;
			d->writer.reset();
		}
		// * This fix causes the editor to NULL out empty messages -- M. Scott
		// * Fixed to fix the fix for empty fixed messages. -- gg
		else if (d->writer
			&& d->writer->get_string()
			&& '\0' == *d->writer->get_string())
		{
			d->writer->set_string("\r\n");
		}

		if (STATE(d) == CON_MEDIT)
			medit_disp_menu(d);

		if (STATE(d) == CON_TRIGEDIT)
			trigedit_disp_menu(d);

		if (STATE(d) == CON_OEDIT)
		{
			switch (OLC_MODE(d))
			{
			case OEDIT_ACTDESC:
				oedit_disp_menu(d);
				break;
			case OEDIT_EXTRADESC_DESCRIPTION:
				oedit_disp_extradesc_menu(d);
				break;
			}
		}
		else if (STATE(d) == CON_REDIT)
		{
			switch (OLC_MODE(d))
			{
			case REDIT_DESC:
				redit_disp_menu(d);
				break;
			case REDIT_EXIT_DESCRIPTION:
				redit_disp_exit_menu(d);
				break;
			case REDIT_EXTRADESC_DESCRIPTION:
				redit_disp_extradesc_menu(d);
				break;
			}
		}
		else if (STATE(d) == CON_WRITEBOARD)
		{
			// добавление сообщения на доску
			if (terminator == 1
				&& d->writer->get_string()
				&& !d->board.expired())
			{
				auto board = d->board.lock();
				d->message->date = time(0);
				d->message->text = d->writer->get_string();
				// для новостных отступов ну и вообще мож все так сейвить, посмотрим
				if (board->get_type() == Boards::NEWS_BOARD
					|| board->get_type() == Boards::GODNEWS_BOARD)
				{
					format_news_message(d->message->text);
				}
				board->write_message(d->message);
				Boards::Static::new_message_notify(board);
				SEND_TO_Q("Спасибо за ваши излияния души, послание сохранено.\r\n", d);
			}
			else
			{
				SEND_TO_Q("Редактирование прервано.\r\n", d);
			}
			d->message.reset();
			d->board.reset();
			if (d->writer)
			{
				d->writer->clear();
				d->writer.reset();
			}
			d->connected = CON_PLAYING;
		}
		else if (STATE(d) == CON_WRITE_MOD)
		{
			// писали клановое сообщение дня
			if (terminator == 1
				&& d->writer->get_string())
			{
				std::string body = d->writer->get_string();
				boost::trim(body);
				if (body.empty())
				{
					CLAN(d->character)->write_mod(body);
					send_to_char("Сообщение удалено.\r\n", d->character.get());
				}
				else
				{
					// за время редактирования могли лишиться привилегии/клана
					if (d->character
						&& CLAN(d->character)
						&& CLAN(d->character)->CheckPrivilege(
							CLAN_MEMBER(d->character)->rank_num,
							ClanSystem::MAY_CLAN_MOD))
					{
						std::string head = boost::str(boost::format
							("Сообщение дружины (автор %1%):\r\n")
							% GET_NAME(d->character));
						// отступ (копи-паст из CON_WRITEBOARD выше)
						format_news_message(body);
						head += body;

						CLAN(d->character)->write_mod(head);
						SEND_TO_Q("Сообщение добавлено.\r\n", d);
					}
					else
					{
						SEND_TO_Q("Ошибочка вышла...\r\n", d);
					}
				}
			}
			else
			{
				SEND_TO_Q("Сообщение прервано.\r\n", d);
			}

			if (d->writer)
			{
				d->writer->clear();
				d->writer.reset();
			}
			d->connected = CON_PLAYING;
		}
		else if (!d->connected && (PLR_FLAGGED(d->character, PLR_MAILING)))
		{
			if ((terminator == 1) && d->writer->get_string())
			{
				mail::add(d->mail_to, d->character->get_uid(), d->writer->get_string());
				SEND_TO_Q("Ближайшей оказией я отправлю ваше письмо адресату!\r\n", d);
				if (DescByUID(d->mail_to))
				{
					mail::add_notice(d->mail_to);
				}
			}
			else
				SEND_TO_Q("Письмо удалено.\r\n", d);
			//log("[SA] 5s");
			d->mail_to = 0;
			if (d->writer)
			{
				d->writer->clear();
				d->writer.reset();
			}
		}
		else if (STATE(d) == CON_EXDESC)  	//log("[SA] 7s");
		{
			if (terminator != 1)
			{
				SEND_TO_Q("Создание описания прервано.\r\n", d);
			}
			SEND_TO_Q(MENU, d);
			d->connected = CON_MENU;
			//log("[SA] 7f");
		}
		else if (!d->connected && d->character && !IS_NPC(d->character))
		{
			if (terminator == 1)  	//log("[SA] 8s");
			{
				if (d->writer)
				{
					d->writer->clear();
					d->writer.reset();
				}
			}
			else  	//log("[SA] 9s");
			{
				if (d->backstr)
				{
					d->writer->set_string(d->backstr);
					free(d->backstr);
					d->backstr = nullptr;
				}
				else
				{
					d->writer->clear();
				}
				
				SEND_TO_Q("Сообщение прервано.\r\n", d);
			}
		}

		if (d->character && !IS_NPC(d->character))
		{
			PLR_FLAGS(d->character).unset(PLR_WRITING);
			
			PLR_FLAGS(d->character).unset(PLR_MAILING);
		}
		if (d->backstr)
		{
			free(d->backstr);
			d->backstr = nullptr;
		}
		
		d->writer.reset();
	}
	else if (!action)
	{
		d->writer->append_string("\r\n");
	}
}

// ***********************************************************************
// * Set of character features                                           *
// ***********************************************************************

void do_featset(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;
	char name[MAX_INPUT_LENGTH], buf2[128];
	char buf[MAX_INPUT_LENGTH], help[MAX_STRING_LENGTH];
	int feat = -1, value, i, qend;

	argument = one_argument(argument, name);

	if (!*name)  		// no arguments. print an informative text //
	{
		send_to_char("Формат: featset <игрок> '<способность>' <значение>\r\n", ch);
		strcpy(help, "Возможные способности:\r\n");
		for (qend = 0, i = 1; i < MAX_FEATS; i++)
		{
			if (feat_info[i].type == UNUSED_FTYPE)	// This is valid. //
				continue;
			sprintf(help + strlen(help), "%30s", feat_info[i].name);
			if (qend++ % 3 == 2)
			{
				strcat(help, "\r\n");
				send_to_char(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			send_to_char(help, ch);
		send_to_char("\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		send_to_char(NOPERSON, ch);
		return;
	}
	skip_spaces(&argument);

	// If there is no chars in argument //
	if (!*argument)
	{
		send_to_char("Пропущено название способности.\r\n", ch);
		return;
	}
	if (*argument != '\'')
	{
		send_to_char("Название способности надо заключить в символы : ''\r\n", ch);
		return;
	}
	// Locate the last quote and lowercase the magic words (if any) //

	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
		argument[qend] = LOWER(argument[qend]);

	if (argument[qend] != '\'')
	{
		send_to_char("Название способности должно быть заключено в символы : ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';

	if ((feat = find_feat_num(help)) <= 0)
	{
		send_to_char("Неизвестная способность.\r\n", ch);
		return;
	}

	argument += qend + 1;	// skip to next parameter //
	argument = one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Не указан числовой параметр (0 или 1).\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0 || value > 1)
	{
		send_to_char("Допустимые значения: 0 (снять), 1 (установить).\r\n", ch);
		return;
	}

	if (IS_NPC(vict))
	{
		send_to_char("Вы не можете добавить способность NPC, используйте OLC.\r\n", ch);
		return;
	}


	sprintf(buf2, "%s changed %s's %s to '%s'.", GET_NAME(ch), GET_NAME(vict),
			feat_info[feat].name, value ? "enabled" : "disabled");
	mudlog(buf2, BRF, -1, SYSLOG, TRUE);
	imm_log("%s", buf2);
	if (feat >= 0 && feat < MAX_FEATS)
	{
		if (value)
			SET_FEAT(vict, feat);
		else
			UNSET_FEAT(vict, feat);
	}
	sprintf(buf2, "Вы изменили для %s '%s' на '%s'.\r\n", GET_PAD(vict, 1),
			feat_info[feat].name, value ? "доступно" : "недоступно");
	if (!can_get_feat(vict, feat) && value == 1)
		send_to_char("Эта способность не доступна данному персонажу и будет удалена при повторном входе в игру.\r\n", ch);
	send_to_char(buf2, ch);
}

// **********************************************************************
// *  Modification of character skills                                  *
// **********************************************************************

void do_skillset(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;
	char name[MAX_INPUT_LENGTH], buf2[128];
	char buf[MAX_INPUT_LENGTH], help[MAX_STRING_LENGTH];
	int spell = -1, value, i, qend;
	ESkill skill = SKILL_INVALID;

	argument = one_argument(argument, name);

	// * No arguments. print an informative text.
	if (!*name)  		// no arguments. print an informative text
	{
		send_to_char("Формат: skillset <игрок> '<умение/заклинание>' <значение>\r\n", ch);
		strcpy(help, "Возможные умения:\r\n");
		for (qend = 0, i = 0; i <= TOP_SPELL_DEFINE; i++)
		{
			if (spell_info[i].name == unused_spellname)	// This is valid.
				continue;
			sprintf(help + strlen(help), "%18s", spell_info[i].name);
			if (qend++ % 4 == 3)
			{
				strcat(help, "\r\n");
				send_to_char(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			send_to_char(help, ch);
		send_to_char("\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD)))
	{
		send_to_char(NOPERSON, ch);
		return;
	}
	skip_spaces(&argument);

	// If there is no chars in argument
	if (!*argument)
	{
		send_to_char("Пропущено название умения.\r\n", ch);
		return;
	}
	if (*argument != '\'')
	{
		send_to_char("Умение надо заключить в символы : ''\r\n", ch);
		return;
	}
	// Locate the last quote and lowercase the magic words (if any)

	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
	{
		argument[qend] = LOWER(argument[qend]);
	}

	if (argument[qend] != '\'')
	{
		send_to_char("Умение должно быть заключено в символы : ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';

	if (SKILL_INVALID == (skill = fix_name_and_find_skill_num(help)))
	{
		spell = fix_name_and_find_spell_num(help);
	}

	if (SKILL_INVALID == skill
        && spell < 0)
	{
		send_to_char("Неизвестное умение/заклинание.\r\n", ch);
		return;
	}
	argument += qend + 1;	// skip to next parameter
	argument = one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Пропущен уровень умения.\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0)
	{
		send_to_char("Минимальное значение умения 0.\r\n", ch);
		return;
	}
	if (!is_magic_skill(skill) && value > 200)
	{
		send_to_char("Максимальное значение умения 200.\r\n", ch);
		return;
	}
	if (IS_NPC(vict))
	{
		send_to_char("Вы не можете добавить умение для мобов.\r\n", ch);
		return;
	}

	// * find_skill_num() guarantees a valid spell_info[] index, or -1, and we
	// * checked for the -1 above so we are safe here.
	sprintf(buf2, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
		spell >= 0 ? spell_info[spell].name : skill_info[skill].name, value);
	mudlog(buf2, BRF, -1, SYSLOG, TRUE);
	imm_log("%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
		spell >= 0 ? spell_info[spell].name : skill_info[skill].name, value);
	if (spell >= 0 && spell <= MAX_SPELLS)
	{
		GET_SPELL_TYPE(vict, spell) = value;
	}
	else if (SKILL_INVALID != skill
		&& skill <= MAX_SKILL_NUM)
	{
		vict->set_skill(skill, value);
	}
	sprintf(buf2, "Вы изменили для %s '%s' на %d.\r\n", GET_PAD(vict, 1),
		spell >= 0 ? spell_info[spell].name : skill_info[skill].name, value);
	send_to_char(buf2, ch);
}



/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*
*********************************************************************/

// * Traverse down the string until the begining of the next page has been
// * reached.  Return NULL if this is the last page of the string.
char *next_page(char *str, CHAR_DATA * ch)
{
	int col = 1, line = 1, spec_code = FALSE;
	const char *color;

	for (;; str++)  	// If end of string, return NULL. //
	{
		if (*str == '\0')
			return (NULL);

		// If we're at the start of the next page, return this fact. //
		else if (STRING_WIDTH(ch) && line > STRING_WIDTH(ch))
			return (str);

		// Check for the begining of an ANSI color code block. //
		else if (*str == '$' && !strncmp(str + 1, "COLOR", 5))
		{
			if (!ch)
				color = KNRM;
			else
				switch (*(str + 6))
				{
				case 'N':
					color = clr((ch), (C_NRM)) ? KIDRK : KNRM;
					break;
				case 'n':
					color = clr((ch), (C_NRM)) ? KNRM : KNRM;
					break;
				case 'R':
					color = clr((ch), (C_NRM)) ? KIRED : KNRM;
					break;
				case 'r':
					color = clr((ch), (C_NRM)) ? KRED : KNRM;
					break;
				case 'G':
					color = clr((ch), (C_NRM)) ? KIGRN : KNRM;
					break;
				case 'g':
					color = clr((ch), (C_NRM)) ? KGRN : KNRM;
					break;
				case 'Y':
					color = clr((ch), (C_NRM)) ? KIYEL : KNRM;
					break;
				case 'y':
					color = clr((ch), (C_NRM)) ? KYEL : KNRM;
					break;
				case 'B':
					color = clr((ch), (C_NRM)) ? KIBLU : KNRM;
					break;
				case 'b':
					color = clr((ch), (C_NRM)) ? KBLU : KNRM;
					break;
				case 'M':
					color = clr((ch), (C_NRM)) ? KIMAG : KNRM;
					break;
				case 'm':
					color = clr((ch), (C_NRM)) ? KMAG : KNRM;
					break;
				case 'C':
					color = clr((ch), (C_NRM)) ? KICYN : KNRM;
					break;
				case 'c':
					color = clr((ch), (C_NRM)) ? KCYN : KNRM;
					break;
				case 'W':
					color = clr((ch), (C_NRM)) ? KIDRK : KNRM;
					break;
				case 'w':
					color = clr((ch), (C_NRM)) ? KWHT : KNRM;
					break;
				default:
					color = KNRM;
				}
			strncpy(str, color, strlen(color));
			str += (strlen(color) - 1);
		}
		else if (*str == '\x1B' && !spec_code)
			spec_code = TRUE;

		// Check for the end of an ANSI color code block. //
		else if (*str == 'm' && spec_code)
			spec_code = FALSE;

		// Check for everything else. //
		else if (!spec_code)  	// Carriage return puts us in column one. //
		{
			if (*str == '\r')
				col = 1;
			// Newline puts us on the next line. //
			else if (*str == '\n')
				line++;

			// * We need to check here and see if we are over the page width,
			// * and if so, compensate by going to the begining of the next line.
			else if (STRING_LENGTH(ch) && ++col > STRING_LENGTH(ch))
			{
				col = 1;
				line++;
			}
		}
	}
}


// Function that returns the number of pages in the string.
int count_pages(char *str, CHAR_DATA * ch)
{
	int pages;

	for (pages = 1; (str = next_page(str, ch)); pages++);
	return (pages);
}


/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, DESCRIPTOR_DATA * d)
{
	int i;

	if (d->showstr_count)
	{
		*(d->showstr_vector) = str;
	}

	for (i = 1; i < d->showstr_count && str; i++)
	{
		str = d->showstr_vector[i] = next_page(str, d->character.get());
	}

	d->showstr_page = 0;
}

// The call that gets the paging ball rolling...
void page_string(DESCRIPTOR_DATA * d, char *str, int keep_internal)
{
	if (!d)
		return;

	if (!str || !*str)
	{
		send_to_char("", d->character.get());
		return;
	}
	d->showstr_count = count_pages(str, d->character.get());
	CREATE(d->showstr_vector, d->showstr_count);

	if (keep_internal)
	{
		d->showstr_head = str_dup(str);
		paginate_string(d->showstr_head, d);
	}
	else
	{
		paginate_string(str, d);
	}

	buf2[0] = '\0';
	show_string(d, buf2);
}

// TODO типа временно для стрингов
void page_string(DESCRIPTOR_DATA * d, const std::string& buf)
{
	// TODO: при keep_internal == true (а в 99% случаев так оно есть)
	// получаем дальше в page_string повторный str_dup.
	// как бы собраться с силами и переписать все это :/
	char *str = str_dup(buf.c_str());
	page_string(d, str, true);
	free(str);
}

// The call that displays the next page.
void show_string(DESCRIPTOR_DATA * d, char *input)
{
	char buffer[MAX_STRING_LENGTH];
	int diff;

	any_one_arg(input, buf);

	//* Q is for quit. :)
	if (LOWER(*buf) == 'q' || LOWER(*buf) == 'к')
	{
		free(d->showstr_vector);
		d->showstr_count = 0;
		if (d->showstr_head)
		{
			free(d->showstr_head);
			d->showstr_head = NULL;
		}
		print_con_prompt(d);
		return;
	}
	// R is for refresh, so back up one page internally so we can display
	// it again.
	else if (LOWER(*buf) == 'r' || LOWER(*buf) == 'п')
	{
		d->showstr_page = MAX(0, d->showstr_page - 1);
	}
	// B is for back, so back up two pages internally so we can display the
	// correct page here.
	else if (LOWER(*buf) == 'b' || LOWER(*buf) == 'н')
	{
		d->showstr_page = MAX(0, d->showstr_page - 2);
	}
	// Feature to 'goto' a page.  Just type the number of the page and you
	// are there!
	else if (a_isdigit(*buf))
	{
		d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));
	}

	else if (*buf)
	{
		send_to_char("Листать : <RETURN>, Q<К>онец, R<П>овтор, B<Н>азад, или номер страницы.\r\n", d->character.get());
		return;
	}
	// If we're displaying the last page, just send it to the character, and
	// then free up the space we used.
	if (d->showstr_page + 1 >= d->showstr_count)
	{
		send_to_char(d->showstr_vector[d->showstr_page], d->character.get());
		free(d->showstr_vector);
		d->showstr_vector = nullptr;
		d->showstr_count = 0;
		if (d->showstr_head)
		{
			free(d->showstr_head);
			d->showstr_head = NULL;
		}
		print_con_prompt(d);
	}
	// Or if we have more to show....
	else
	{
		diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];
		if (diff >= MAX_STRING_LENGTH)
			diff = MAX_STRING_LENGTH - 1;
		strncpy(buffer, d->showstr_vector[d->showstr_page], diff);
		buffer[diff] = '\0';
		send_to_char(buffer, d->character.get());
		d->showstr_page++;
	}
}

///
/// Печать какого-то адекватного текущему STATE() промпта
/// Нужен для многостраничной справки при генерации/перегенерации чара, чтобы
/// 1) не печатать ничего между страницами
/// 2) после последней страницы не ждать втихую нажатия от игрока, а вывести меню текущего CON_STATE
/// 3) напечатать тоже самое при выходе из пролистывания через Q/К
///
void print_con_prompt(DESCRIPTOR_DATA *d)
{
	if (d->showstr_count)
	{
		return;
	}
	if (STATE(d) == CON_RESET_STATS)
	{
		genchar_disp_menu(d->character.get());
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
