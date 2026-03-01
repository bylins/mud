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

#include <third_party_libs/fmt/include/fmt/format.h>

#include "modify.h"
#include "interpreter.h"
#include "engine/core/handler.h"
#include "engine/db/db.h"
#include "engine/core/comm.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/spells.h"
#include "gameplay/communication/mail.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/ui/color.h"
#include "engine/olc/olc.h"
#include "gameplay/abilities/feats.h"
#include "gameplay/clans/house.h"
#include "administration/privilege.h"
#include "engine/entities/char_data.h"
#include "gameplay/skills/skills.h"
#include "gameplay/core/genchar.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "gameplay/skills/skills_info.h"
#include "gameplay/magic/spells_info.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "engine/db/global_objects.h"
#include "table_wrapper.h"

void show_string(DescriptorData *d, char *input);

#define PARSE_FORMAT      0
#define PARSE_REPLACE     1
#define PARSE_HELP        2
#define PARSE_DELETE      3
#define PARSE_INSERT      4
#define PARSE_LIST_NORM   5
#define PARSE_LIST_NUM    6
#define PARSE_EDIT        7

// local functions
void smash_tilde(char *str);
void do_skillset(CharData *ch, char *argument, int cmd, int subcmd);
char *next_page(char *str, CharData *ch);
int count_pages(char *str, CharData *ch);
void paginate_string(char *str, DescriptorData *d);

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
void smash_tilde(char *str) {
	/*
	 * Erase any ~'s inserted by people in the editor.  This prevents anyone
	 * using online creation from causing parse errors in the world files.
	 * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
	 * his tildes thank you very much.), -gg 2/20/98
	 */
	while ((str = strchr(str, '~')) != nullptr)
		*str = ' ';
}

/*
 * Basic API function to start writing somewhere.
 *
 * 'data' isn't used in stock CircleMUD but you can use it to pass whatever
 * else you may want through it.  The improved editor patch when updated
 * could use it to pass the old text buffer, for instance.
 */
void string_write(DescriptorData *d, const utils::AbstractStringWriter::shared_ptr &writer,
				  size_t len, int mailto, void *data) {
	if (d->character && !d->character->IsNpc()) {
		d->character->SetFlag(EPlrFlag::kWriting);
	}

	if (data) {
		mudlog("SYSERR: string_write: I don't understand special data.", BRF, kLvlImmortal, SYSLOG, true);
	}

	d->writer = writer;
	d->max_str = len;
	d->mail_to = mailto;
}

// * Handle some editor commands.
void parse_action(int command, char *string, DescriptorData *d) {
	int indent = 0, rep_all = 0, flags = 0, replaced;
	int j = 0;
	int i, line_low, line_high;
	char *s, *t;
	//log("[PA] Start %d(%s)", command, string);
	switch (command) {
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
			iosystem::write_to_output(buf, d);
			break;

		case PARSE_FORMAT:
			while (isalpha(string[j]) && j < 2) {
				switch (string[j]) {
					case 'i':
						if (!indent) {
							indent = true;
							flags += kFormatIndent;
						}
						break;
					default: break;
				}
				j++;
			}

			format_text(d->writer, flags, d, d->max_str);

			sprintf(buf, "Текст отформатирован %s\r\n", (indent ? "WITH INDENT." : "."));
			iosystem::write_to_output(buf, d);
			break;

		case PARSE_REPLACE:
			while (isalpha(string[j]) && j < 2) {
				switch (string[j]) {
					case 'a':
						if (!indent)
							rep_all = 1;
						break;
					default: break;
				}
				j++;
			}
			if ((s = strtok(string, "'")) == nullptr) {
				iosystem::write_to_output("Неверный формат.\r\n", d);
				return;
			} else if ((s = strtok(nullptr, "'")) == nullptr) {
				iosystem::write_to_output("Шаблон должен быть заключен в апострофы.\r\n", d);
				return;
			} else if ((t = strtok(nullptr, "'")) == nullptr) {
				iosystem::write_to_output("No replacement string.\r\n", d);
				return;
			} else if ((t = strtok(nullptr, "'")) == nullptr) {
				iosystem::write_to_output("Замещающая строка должна быть заключена в апострофы.\r\n", d);
				return;
			} else {
				size_t total_len = strlen(t) + strlen(d->writer->get_string()) - strlen(s);
				if (total_len <= d->max_str) {
					replaced = replace_str(d->writer, s, t, rep_all, static_cast<int>(d->max_str));
					if (replaced > 0) {
						sprintf(buf, "Заменено вхождений '%s' на '%s' - %d.\r\n", s, t, replaced);
						iosystem::write_to_output(buf, d);
					} else if (replaced == 0) {
						sprintf(buf, "Шаблон '%s' не найден.\r\n", s);
						iosystem::write_to_output(buf, d);
					} else {
						iosystem::write_to_output("ОШИБКА: При попытке замены буфер переполнен - прервано.\r\n", d);
					}
				} else {
					iosystem::write_to_output("Нет свободного места для завершения команды.\r\n", d);
				}
			}
			break;

		case PARSE_DELETE:
			switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
				case 0: iosystem::write_to_output("Вы должны указать номер строки или диапазон для удаления.\r\n", d);
					return;
				case 1: line_high = line_low;
					break;
				case 2:
					if (line_high < line_low) {
						iosystem::write_to_output("Неверный диапазон.\r\n", d);
						return;
					}
					break;
			}

			i = 1;
			{
				char *buffer = str_dup(d->writer->get_string());
				std::shared_ptr<char> guard(buffer, free);
				s = buffer;
				if (s == nullptr) {
					iosystem::write_to_output("Буфер пуст.\r\n", d);
					return;
				} else if (line_low > 0) {
					unsigned int total_len = 1;
					while (s && (i < line_low)) {
						if ((s = strchr(s, '\n')) != nullptr) {
							i++;
							s++;
						}
					}
					if ((i < line_low) || (s == nullptr)) {
						iosystem::write_to_output("Строка(и) вне диапазона - проигнорировано.\r\n", d);
						return;
					}
					t = s;
					while (s && (i < line_high)) {
						if ((s = strchr(s, '\n')) != nullptr) {
							i++;
							total_len++;
							s++;
						}
					}
					if ((s) && ((s = strchr(s, '\n')) != nullptr)) {
						s++;
						while (*s != '\0') {
							*(t++) = *(s++);
						}
					} else {
						total_len--;
					}
					*t = '\0';
					d->writer->set_string(buffer);

					sprintf(buf, "%u line%sdeleted.\r\n", total_len, ((total_len != 1) ? "s " : " "));
					iosystem::write_to_output(buf, d);
				} else {
					iosystem::write_to_output("Отрицательный или нулевой номер строки для удаления.\r\n", d);
					return;
				}
			}
			break;

		case PARSE_LIST_NORM:
			// * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
			// * they are probly ok for what to do here.
			*buf = '\0';
			if (*string != '\0')
				switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
					case 0: line_low = 1;
						line_high = 999999;
						break;
					case 1: line_high = line_low;
						break;
				}
			else {
				line_low = 1;
				line_high = 999999;
			}

			if (line_low < 1) {
				iosystem::write_to_output("Начальный номер отрицательный или 0.\r\n", d);
				return;
			} else if (line_high < line_low) {
				iosystem::write_to_output("Неверный диапазон.\r\n", d);
				return;
			}
			*buf = '\0';
			if ((line_high < 999999) || (line_low > 1))
				sprintf(buf, "Текущий диапазон [%d - %d]:\r\n", line_low, line_high);
			i = 1;
			{
				const char *pos = d->writer->get_string();

				unsigned int total_len = 0;
				while (pos && (i < line_low)) {
					if ((pos = strchr(pos, '\n')) != nullptr) {
						i++;
						pos++;
					}
				}

				if ((i < line_low) || (pos == nullptr)) {
					iosystem::write_to_output("Строка(и) вне диапазона - проигнорировано.\r\n", d);
					return;
				}

				const char *beginning = pos;
				while (pos && (i <= line_high)) {
					if ((pos = strchr(pos, '\n')) != nullptr) {
						i++;
						total_len++;
						pos++;
					}
				}

				if (pos) {
					strncat(buf, beginning, pos - beginning);
				} else {
					strcat(buf, beginning);
				}

				page_string(d, buf, true);
			}

			break;

		case PARSE_LIST_NUM:
			// * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
			// * they are probly ok for what to do here.
			*buf = '\0';
			if (*string != '\0') {
				switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
					case 0: line_low = 1;
						line_high = 999999;
						break;
					case 1: line_high = line_low;
						break;
				}
			} else {
				line_low = 1;
				line_high = 999999;
			}

			if (line_low < 1) {
				iosystem::write_to_output("Номер строки должен быть больше 0.\r\n", d);
				return;
			}
			if (line_high < line_low) {
				iosystem::write_to_output("Неверный диапазон.\r\n", d);
				return;
			}
			*buf = '\0';
			i = 1;
			{
				const char *pos = d->writer->get_string();
				unsigned int total_len = 0;

				while (pos && (i < line_low)) {
					if ((pos = strchr(pos, '\n')) != nullptr) {
						i++;
						pos++;
					}
				}

				if ((i < line_low) || (pos == nullptr)) {
					iosystem::write_to_output("Строка(и) вне диапазона - проигнорировано.\r\n", d);
					return;
				}

				const char *beginning = pos;
				while (pos && (i <= line_high)) {
					if ((pos = strchr(pos, '\n')) != nullptr) {
						i++;
						total_len++;
						pos++;
						sprintf(buf1, "%s", buf);
						snprintf(buf, kMaxStringLength, "%s%4d:\r\n", buf1, (i - 1));
						strncat(buf, beginning, pos - beginning);
						beginning = pos;
					}
				}

				if (pos && beginning) {
					strncat(buf, beginning, pos - beginning);
				} else if (beginning) {
					strcat(buf, beginning);
				}
			}

			page_string(d, buf, true);
			break;

		case PARSE_INSERT: half_chop(string, buf, buf2);
			if (*buf == '\0') {
				iosystem::write_to_output("Вы должны указать номер строки, после которой вставить текст.\r\n", d);
				return;
			}
			line_low = atoi(buf);
			strcat(buf2, "\r\n");

			i = 1;
			*buf = '\0';
			{
				const char *pos = d->writer->get_string();
				const char *beginning = pos;
				if (pos == nullptr) {
					iosystem::write_to_output("Буфер пуст - ничего не вставлено.\r\n", d);
					return;
				}
				if (line_low > 0) {
					while (pos && (i < line_low)) {
						if ((pos = strchr(pos, '\n')) != nullptr) {
							i++;
							pos++;
						}
					}
					if ((i < line_low) || (pos == nullptr)) {
						iosystem::write_to_output("Номер строки вне диапазона - прервано.\r\n", d);
						return;
					}
					if ((pos - beginning + strlen(buf2) + strlen(pos) + 3) > d->max_str) {
						iosystem::write_to_output("Превышение размеров буфера - прервано.\r\n", d);
						return;
					}
					if (beginning && (*beginning != '\0')) {
						strncat(buf, beginning, pos - beginning);
					}
					strcat(buf, buf2);
					if (*pos != '\0') {
						strcat(buf, pos);
					}
					d->writer->set_string(buf);
					iosystem::write_to_output("Строка вставлена.\r\n", d);
				} else {
					iosystem::write_to_output("Номер строки должен быть больше 0.\r\n", d);
					return;
				}
			}
			break;

		case PARSE_EDIT: half_chop(string, buf, buf2);
			if (*buf == '\0') {
				iosystem::write_to_output("Вы должны указать номер строки изменяемого текста.\r\n", d);
				return;
			}
			line_low = atoi(buf);
			strcat(buf2, "\r\n");

			i = 1;
			*buf = '\0';
			{
				const char *s = d->writer->get_string();
				const char *beginning = s;
				if (s == nullptr) {
					iosystem::write_to_output("Буфер пуст - изменения не проведены.\r\n", d);
					return;
				}

				if (line_low > 0) {    // Loop through the text counting \\n characters until we get to the line/
					while (s && (i < line_low)) {
						if ((s = strchr(s, '\n')) != nullptr) {
							i++;
							s++;
						}
					}

					// * Make sure that there was a THAT line in the text.
					if ((i < line_low) || (s == nullptr)) {
						iosystem::write_to_output("Строка вне диапазона - прервано.\r\n", d);
						return;
					}

					// If s is the same as *d->str that means I'm at the beginning of the
					// message text and I don't need to put that into the changed buffer.
					if (s != beginning) {    // First things first .. we get this part into the buffer.
						// Put the first 'good' half of the text into storage.
						strncat(buf, beginning, s - beginning);
					}
					// Put the new 'good' line into place.
					strcat(buf, buf2);
					if ((s = strchr(s, '\n')) != nullptr) {
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
					if (strlen(buf) > d->max_str) {
						iosystem::write_to_output("Превышение максимального размера буфера - прервано.\r\n", d);
						return;
					}

					// * Change the size of the REAL buffer to fit the new text.
					d->writer->set_string(buf);
					iosystem::write_to_output("Строка изменена.\r\n", d);
				} else {
					iosystem::write_to_output("Номер строки должен быть больше 0.\r\n", d);
					return;
				}
			}
			break;

		default: iosystem::write_to_output("Неверная опция.\r\n", d);
			mudlog("SYSERR: invalid command passed to parse_action", BRF, kLvlImplementator, SYSLOG, true);
			return;
	}
	//log("[PA] Stop");
}

// Add user input to the 'current' string (as defined by d->str) //
void string_add(DescriptorData *d, char *str) {
	int terminator = 0, action = 0;
	int i = 2, j = 0;
	char actions[kMaxInputLength];

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
	if (d->character && !d->character->IsFlagged(EPlrFlag::kMailing))
		log("[SA] <%s> adds string '%s'", GET_NAME(d->character), str);

	smash_tilde(str);
	if (!d->writer) {
		return;
	}

	if ((action = (*str == '/'))) {
		while (str[i] != '\0') {
			actions[j] = str[i];
			i++;
			j++;
		}
		actions[j] = '\0';
		*str = '\0';
		switch (str[1]) {
			case 'a': terminator = 2;    // Working on an abort message, //
				break;
			case 'c':
				if (d->writer->get_string()) {
					d->writer->clear();
					iosystem::write_to_output("Буфер очищен.\r\n", d);
				} else
					iosystem::write_to_output("Буфер пуст.\r\n", d);
				break;
			case 'd': parse_action(PARSE_DELETE, actions, d);
				break;
			case 'e': parse_action(PARSE_EDIT, actions, d);
				break;
			case 'f':
				if (d->writer->get_string()) {
					parse_action(PARSE_FORMAT, actions, d);
				} else {
					iosystem::write_to_output("Буфер пуст.\r\n", d);
				}
				break;
			case 'i':
				if (d->writer->get_string()) {
					parse_action(PARSE_INSERT, actions, d);
				} else {
					iosystem::write_to_output("Буфер пуст.\r\n", d);
				}
				break;
			case 'h': parse_action(PARSE_HELP, actions, d);
				break;
			case 'l':
				if (d->writer->get_string()) {
					parse_action(PARSE_LIST_NORM, actions, d);
				} else {
					iosystem::write_to_output("Буфер пуст.\r\n", d);
				}
				break;
			case 'n':
				if (d->writer->get_string()) {
					parse_action(PARSE_LIST_NUM, actions, d);
				} else {
					iosystem::write_to_output("Буфер пуст.\r\n", d);
				}
				break;
			case 'r': parse_action(PARSE_REPLACE, actions, d);
				break;
			case 's': terminator = 1;
				*str = '\0';
				break;
			default: iosystem::write_to_output("Интересный выбор. Я о нем подумаю...\r\n", d);
				break;
		}
	}

	if (!d->writer->get_string()) {
		if (strlen(str) + 3 > d->max_str) {
			SendMsgToChar("Слишком длинная строка - усечена.\r\n", d->character.get());
			strcpy(&str[d->max_str - 3], "\r\n");
			d->writer->set_string(str);
		} else if (EConState::kWriteMod == d->state && strlen(str) + 3 > 80) {
			SendMsgToChar("Слишком длинная строка - усечена.\r\n", d->character.get());
			str[80 - 3] = '\0';
			d->writer->set_string(str);
		} else {
			d->writer->set_string(str);
		}
	} else {
		if (EConState::kWriteMod == d->state && strlen(str) + 3 > 80) {
			SendMsgToChar("Слишком длинная строка - усечена.\r\n", d->character.get());
			str[80 - 3] = '\0';
		}

		if (strlen(str) + d->writer->length() + 3 > d->max_str)    // \r\n\0 //
		{
			SendMsgToChar(d->character.get(),
						  "Слишком длинное послание > %zu симв. Последняя строка проигнорирована.\r\n",
						  d->max_str - 3);
			action = true;
		} else {
			d->writer->append_string(str);
		}
	}

	if (terminator) {    // OLC Edits
		extern void oedit_disp_menu(DescriptorData *d);
		extern void oedit_disp_extradesc_menu(DescriptorData *d);
		extern void redit_disp_menu(DescriptorData *d);
		extern void redit_disp_extradesc_menu(DescriptorData *d);
		extern void redit_disp_exit_menu(DescriptorData *d);
		extern void medit_disp_menu(DescriptorData *d);
		extern void trigedit_disp_menu(DescriptorData *d);

#if defined(OASIS_MPROG)
		extern void medit_change_mprog(DescriptorData * d);

		if (d->connected == CON_MEDIT)
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
		if ((terminator == 2)
			&& ((d->state == EConState::kRedit) || (d->state == EConState::kMedit) || (d->state == EConState::kOedit)
				|| (d->state == EConState::kTrigedit)
				|| (d->state == EConState::kExdesc)))    //log("[SA] 2s");
		{
			if (d->backstr) {
				d->writer->set_string(d->backstr);
				free(d->backstr);
			} else {
				d->writer->clear();
			}
			d->backstr = nullptr;
			d->writer.reset();
		}
			// * This fix causes the editor to NULL out empty messages -- M. Scott
			// * Fixed to fix the fix for empty fixed messages. -- gg
		else if (d->writer
			&& d->writer->get_string()
			&& '\0' == *d->writer->get_string()) {
			d->writer->set_string("\r\n");
		}

		if (d->state == EConState::kMedit)
			medit_disp_menu(d);

		if (d->state == EConState::kTrigedit)
			trigedit_disp_menu(d);

		if (d->state == EConState::kOedit) {
			switch (OLC_MODE(d)) {
				case OEDIT_ACTDESC: oedit_disp_menu(d);
					break;
				case OEDIT_EXTRADESC_DESCRIPTION: oedit_disp_extradesc_menu(d);
					break;
			}
		} else if (d->state == EConState::kRedit) {
			switch (OLC_MODE(d)) {
				case REDIT_DESC: redit_disp_menu(d);
					break;
				case REDIT_EXIT_DESCRIPTION: redit_disp_exit_menu(d);
					break;
				case REDIT_EXTRADESC_DESCRIPTION: redit_disp_extradesc_menu(d);
					break;
			}
		} else if (d->state == EConState::kWriteNote) {
			iosystem::write_to_output("Заметка сохранена.\r\n", d);
			d->state = EConState::kPlaying;
		} else if (d->state == EConState::kWriteboard) {
			// добавление сообщения на доску
			if (terminator == 1
				&& d->writer->get_string()
				&& !d->board.expired()) {
				auto board = d->board.lock();
				d->message->date = time(nullptr);
				d->message->text = d->writer->get_string();
				// для новостных отступов ну и вообще мож все так сейвить, посмотрим
				if (board->get_type() == Boards::NEWS_BOARD
					|| board->get_type() == Boards::GODNEWS_BOARD) {
					format_news_message(d->message->text);
				}
				board->write_message(d->message);
				Boards::Static::new_message_notify(board);
				iosystem::write_to_output("Спасибо за ваши излияния души, послание сохранено.\r\n", d);
			} else {
				iosystem::write_to_output("Редактирование прервано.\r\n", d);
			}
			d->message.reset();
			d->board.reset();
			if (d->writer) {
				d->writer->clear();
				d->writer.reset();
			}
			d->state = EConState::kPlaying;
		} else if (d->state == EConState::kWriteMod) {
			// писали клановое сообщение дня
			if (terminator == 1
				&& d->writer->get_string()) {
				std::string body = d->writer->get_string();
				utils::Trim(body);
				if (body.empty()) {
					CLAN(d->character)->write_mod(body);
					SendMsgToChar("Сообщение удалено.\r\n", d->character.get());
				} else {
					// за время редактирования могли лишиться привилегии/клана
					if (d->character
						&& CLAN(d->character)
						&& CLAN(d->character)->CheckPrivilege(
							CLAN_MEMBER(d->character)->rank_num,
							ClanSystem::MAY_CLAN_MOD)) {
						std::string head = fmt::format("Сообщение дружины (автор {}):\r\n", GET_NAME(d->character));
						// отступ (копи-паст из CON_WRITEBOARD выше)
						format_news_message(body);
						head += body;

						CLAN(d->character)->write_mod(head);
						iosystem::write_to_output("Сообщение добавлено.\r\n", d);
					} else {
						iosystem::write_to_output("Ошибочка вышла...\r\n", d);
					}
				}
			} else {
				iosystem::write_to_output("Сообщение прервано.\r\n", d);
			}

			if (d->writer) {
				d->writer->clear();
				d->writer.reset();
			}
			d->state = EConState::kPlaying;
		} else if (d->state == EConState::kPlaying && (d->character->IsFlagged(EPlrFlag::kMailing))) {
			if ((terminator == 1) && d->writer->get_string()) {
				mail::add(d->mail_to, d->character->get_uid(), d->writer->get_string());
				iosystem::write_to_output("Ближайшей оказией я отправлю ваше письмо адресату!\r\n", d);
				if (DescriptorByUid(d->mail_to)) {
					mail::add_notice(d->mail_to);
				}
			} else
				iosystem::write_to_output("Письмо удалено.\r\n", d);
			//log("[SA] 5s");
			d->mail_to = 0;
			if (d->writer) {
				d->writer->clear();
				d->writer.reset();
			}
		} else if (d->state == EConState::kExdesc)    //log("[SA] 7s");
		{
			if (terminator != 1) {
				iosystem::write_to_output("Создание описания прервано.\r\n", d);
			}
			iosystem::write_to_output(MENU, d);
			d->state = EConState::kMenu;
			//log("[SA] 7f");
		} else if (d->state == EConState::kPlaying && d->character && !d->character->IsNpc()) {
			if (terminator == 1)    //log("[SA] 8s");
			{
				if (d->writer) {
					d->writer->clear();
					d->writer.reset();
				}
			} else    //log("[SA] 9s");
			{
				if (d->backstr) {
					d->writer->set_string(d->backstr);
					free(d->backstr);
					d->backstr = nullptr;
				} else {
					d->writer->clear();
				}

				iosystem::write_to_output("Сообщение прервано.\r\n", d);
			}
		}

		if (d->character && !d->character->IsNpc()) {
			d->character->UnsetFlag(EPlrFlag::kWriting);

			d->character->UnsetFlag(EPlrFlag::kMailing);
		}
		if (d->backstr) {
			free(d->backstr);
			d->backstr = nullptr;
		}

		d->writer.reset();
	} else if (!action) {
		d->writer->append_string("\r\n");
	}
}

// ***********************************************************************
// * Set of character features                                           *
// ***********************************************************************

void do_featset(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	char name[kMaxInputLength], buf2[128];
	char buf[kMaxInputLength], help[kMaxStringLength];
	int value, qend;

	argument = one_argument(argument, name);

	if (!*name) {
		std::ostringstream out;
		out << "Формат: featset <игрок> '<способность>' <значение>" << std:: endl
			<< "Возможные способности:" << std:: endl;

		table_wrapper::Table table;
		for (const auto &feat : MUD::Feats()) {
			if (feat.IsInvalid()) {
				continue;
			}
			table << feat.GetName();
			if (table.cur_col() % 3 == 0) {
				table << table_wrapper::kEndRow;
			}
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(out, table);
		out << std:: endl;

		SendMsgToChar(out.str().c_str(), ch);
		return;
	}

	if (!(vict = get_char_vis(ch, name, EFind::kCharInWorld))) {
		SendMsgToChar(NOPERSON, ch);
		return;
	}
	skip_spaces(&argument);

	// If there is no entities in argument //
	if (!*argument) {
		SendMsgToChar("Пропущено название способности.\r\n", ch);
		return;
	}
	if (*argument != '\'') {
		SendMsgToChar("Название способности надо заключить в символы : ''\r\n", ch);
		return;
	}
	// Locate the last quote and lowercase the magic words (if any) //

	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
		argument[qend] = LOWER(argument[qend]);

	if (argument[qend] != '\'') {
		SendMsgToChar("Название способности должно быть заключено в символы : ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';

	auto feat_id =  FixNameAndFindFeatId(help);
	if (feat_id == EFeat::kUndefined) {
		SendMsgToChar("Неизвестная способность.\r\n", ch);
		return;
	}
	if (MUD::Feat(feat_id).IsInvalid()) {
		SendMsgToChar("Отключенное или служебное значение.\r\n", ch);
		return;
	}

	argument += qend + 1;    // skip to next parameter //
	argument = one_argument(argument, buf);

	if (!*buf) {
		SendMsgToChar("Не указан числовой параметр (0 или 1).\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0 || value > 1) {
		SendMsgToChar("Допустимые значения: 0 (снять), 1 (установить).\r\n", ch);
		return;
	}

	if (vict->IsNpc()) {
		SendMsgToChar("Вы не можете добавить способность NPC, используйте OLC.\r\n", ch);
		return;
	}

	sprintf(buf2, "%s changed %s's %s to '%s'.", GET_NAME(ch), GET_NAME(vict),
			MUD::Feat(feat_id).GetCName(), value ? "enabled" : "disabled");
	mudlog(buf2, BRF, -1, SYSLOG, true);
	imm_log("%s", buf2);
	if (value) {
		vict->SetFeat(feat_id);
	} else {
		vict->UnsetFeat(feat_id);
	}

	sprintf(buf2, "Вы изменили для %s '%s' на '%s'.\r\n", GET_PAD(vict, 1),
			MUD::Feat(feat_id).GetCName(), value ? "доступно" : "недоступно");
	if (!CanGetFeat(vict, feat_id) && value == 1) {
		SendMsgToChar("Эта способность недоступна персонажу и будет удалена при повторном входе в игру.\r\n",
					 ch);
	}
	SendMsgToChar(buf2, ch);
}

// **********************************************************************
// *  Modification of character skills                                  *
// **********************************************************************

void do_skillset(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	char name[kMaxInputLength], buf2[128];
	char buf[kMaxInputLength], help[kMaxStringLength];
	int value;

	argument = one_argument(argument, name);

	auto qend{0};
	if (!*name) {
		SendMsgToChar("Формат: skillset <игрок> '<умение/заклинание>' <значение>\r\n", ch);
		strcpy(help, "Возможные умения:\r\n");

		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (MUD::Spell(spell_id).IsInvalid()) {    // This is valid.
				continue;
			}
			sprintf(help + strlen(help), "%30s", MUD::Spell(spell_id).GetCName());
			if (qend++ % 4 == 3) {
				strcat(help, "\r\n");
				SendMsgToChar(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			SendMsgToChar(help, ch);
		SendMsgToChar("\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, name, EFind::kCharInWorld))) {
		SendMsgToChar(NOPERSON, ch);
		return;
	}
	skip_spaces(&argument);

	// If there is no entities in argument
	if (!*argument) {
		SendMsgToChar("Пропущено название умения.\r\n", ch);
		return;
	}
	if (*argument != '\'') {
		SendMsgToChar("Умение надо заключить в символы : ''\r\n", ch);
		return;
	}
	// Locate the last quote and lowercase the magic words (if any)

	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++) {
		argument[qend] = LOWER(argument[qend]);
	}

	if (argument[qend] != '\'') {
		SendMsgToChar("Умение должно быть заключено в символы : ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';

	auto skill_id{ESkill::kUndefined};
	auto spell_id{ESpell::kUndefined};
	if (ESkill::kUndefined == (skill_id = FixNameAndFindSkillId(help))) {
		spell_id = FixNameAndFindSpellId(help);
	}

	if (skill_id == ESkill::kUndefined && spell_id == ESpell::kFirst) {
		SendMsgToChar("Неизвестное умение/заклинание.\r\n", ch);
		return;
	}
	argument += qend + 1;    // skip to next parameter
	argument = one_argument(argument, buf);

	if (!*buf) {
		SendMsgToChar("Пропущен уровень умения.\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0) {
		SendMsgToChar("Минимальное значение умения 0.\r\n", ch);
		return;
	}
	if (vict->IsNpc()) {
		SendMsgToChar("Вы не можете добавить умение для мобов.\r\n", ch);
		return;
	}
	if (value > MUD::Skill(skill_id).cap && spell_id < ESpell::kFirst) {
		SendMsgToChar("Превышено максимально возможное значение умения.\r\n", ch);
		value = MUD::Skill(skill_id).cap;
	}

	sprintf(buf2, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
			spell_id >= ESpell::kFirst ? MUD::Spell(spell_id).GetCName() : MUD::Skill(skill_id).GetName(), value);
	mudlog(buf2, BRF, kLvlImmortal, SYSLOG, true);
	if (spell_id >= ESpell::kFirst && spell_id <= ESpell::kLast) {
		if (value == 0 && IS_SET(GET_SPELL_TYPE(vict, spell_id), ESpellType::kTemp)) {
			for (auto it = vict->temp_spells.begin(); it != vict->temp_spells.end();) {
				if (it->second.spell == spell_id) {
					it = vict->temp_spells.erase(it);
				} else
					++it;
			}
		}
		if (IS_SET(value, ESpellType::kTemp)) {
			temporary_spells::AddSpell(vict, spell_id, time(nullptr), 3600);
		}
		GET_SPELL_TYPE(vict, spell_id) = value;
	} else if (ESkill::kUndefined != skill_id && skill_id <= ESkill::kLast) {
		vict->set_skill(skill_id, value);
	}
	sprintf(buf2, "Вы изменили для %s '%s' на %d.\r\n", GET_PAD(vict, 1),
			spell_id > ESpell::kUndefined ? MUD::Spell(spell_id).GetCName() : MUD::Skill(skill_id).GetName(), value);
	SendMsgToChar(buf2, ch);
}



/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*
*********************************************************************/

// * Traverse down the string until the begining of the next page has been
// * reached.  Return NULL if this is the last page of the string.
char *next_page(char *str, CharData *ch) {
	int col = 1, line = 1, spec_code = false;
	const char *color;

	for (;; str++)    // If end of string, return NULL. //
	{
		if (*str == '\0')
			return (nullptr);

			// If we're at the start of the next page, return this fact. //
		else if (STRING_WIDTH(ch) && line > STRING_WIDTH(ch))
			return (str);

			// Check for the begining of an ANSI color code block. //
		else if (*str == '$' && !strncmp(str + 1, "COLOR", 5)) {
			if (!ch)
				color = kColorNrm;
			else
				switch (*(str + 6)) {
					case 'N': color = kColorBoldBlk;
						break;
					case 'n': color = kColorNrm;
						break;
					case 'R': color = kColorBoldRed;
						break;
					case 'r': color = kColorRed;
						break;
					case 'G': color = kColorBoldGrn;
						break;
					case 'g': color = kColorGrn;
						break;
					case 'Y': color = kColorBoldYel;
						break;
					case 'y': color = kColorYel;
						break;
					case 'B': color = kColorBoldBlu;
						break;
					case 'b': color = kColorBlu;
						break;
					case 'M': color = kColorBoldMag;
						break;
					case 'm': color = kColorMag;
						break;
					case 'C': color = kColorBoldCyn;
						break;
					case 'c': color = kColorCyn;
						break;
					case 'W': color = kColorBoldBlk;
						break;
					case 'w': color = kColorWht;
						break;
					default: color = kColorNrm;
				}
			memcpy(str, color, strlen(color));
			str += (strlen(color) - 1);
		} else if (*str == '\x1B' && !spec_code)
			spec_code = true;

			// Check for the end of an ANSI color code block. //
		else if (*str == 'm' && spec_code)
			spec_code = false;

			// Check for everything else. //
		else if (!spec_code)    // Carriage return puts us in column one. //
		{
			if (*str == '\r')
				col = 1;
				// Newline puts us on the next line. //
			else if (*str == '\n')
				line++;

				// * We need to check here and see if we are over the page width,
				// * and if so, compensate by going to the begining of the next line.
			else if (STRING_LENGTH(ch) && ++col > STRING_LENGTH(ch)) {
				col = 1;
				line++;
			}
		}
	}
}

// Function that returns the number of pages in the string.
int count_pages(char *str, CharData *ch) {
	int pages;

	for (pages = 1; (str = next_page(str, ch)); pages++);
	return (pages);
}

/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, DescriptorData *d) {
	int i;

	if (d->showstr_count) {
		*(d->showstr_vector) = str;
	}

	for (i = 1; i < d->showstr_count && str; i++) {
		str = d->showstr_vector[i] = next_page(str, d->character.get());
	}

	d->showstr_page = 0;
}

// The call that gets the paging ball rolling...
void page_string(DescriptorData *d, char *str, int keep_internal) {
	if (!d)
		return;

	if (!str || !*str) {
		SendMsgToChar("", d->character.get());
		return;
	}
	d->showstr_count = count_pages(str, d->character.get());
	CREATE(d->showstr_vector, d->showstr_count);

	if (keep_internal) {
		d->showstr_head = str_dup(str);
		paginate_string(d->showstr_head, d);
	} else {
		paginate_string(str, d);
	}

	buf2[0] = '\0';
	show_string(d, buf2);
}

// TODO типа временно для стрингов
void page_string(DescriptorData *d, const std::string &buf) {
	// TODO: при keep_internal == true (а в 99% случаев так оно есть)
	// получаем дальше в page_string повторный str_dup.
	// как бы собраться с силами и переписать все это :/
	char *str = str_dup(buf.c_str());
	page_string(d, str, true);
	free(str);
}

// The call that displays the next page.
void show_string(DescriptorData *d, char *input) {
	char buffer[kMaxStringLength];
	int diff;

	any_one_arg(input, buf);

	//* Q is for quit. :)
	if (LOWER(*buf) == 'q' || LOWER(*buf) == 'к') {
		free(d->showstr_vector);
		d->showstr_count = 0;
		if (d->showstr_head) {
			free(d->showstr_head);
			d->showstr_head = nullptr;
		}
		print_con_prompt(d);
		return;
	}
		// R is for refresh, so back up one page internally so we can display
		// it again.
	else if (LOWER(*buf) == 'r' || LOWER(*buf) == 'п') {
		d->showstr_page = MAX(0, d->showstr_page - 1);
	}
		// B is for back, so back up two pages internally so we can display the
		// correct page here.
	else if (LOWER(*buf) == 'b' || LOWER(*buf) == 'н') {
		d->showstr_page = MAX(0, d->showstr_page - 2);
	}
		// Feature to 'goto' a page.  Just type the number of the page and you
		// are there!
	else if (a_isdigit(*buf)) {
		d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));
	} else if (*buf) {
		SendMsgToChar("Листать : <RETURN>, Q<К>онец, R<П>овтор, B<Н>азад, или номер страницы.\r\n", d->character.get());
		return;
	}
	// If we're displaying the last page, just send it to the character, and
	// then free up the space we used.
	if (d->showstr_page + 1 >= d->showstr_count) {
		SendMsgToChar(d->showstr_vector[d->showstr_page], d->character.get());
		free(d->showstr_vector);
		d->showstr_vector = nullptr;
		d->showstr_count = 0;
		if (d->showstr_head) {
			free(d->showstr_head);
			d->showstr_head = nullptr;
		}
		print_con_prompt(d);
	}
		// Or if we have more to show....
	else {
		diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];
		if (diff >= kMaxStringLength)
			diff = kMaxStringLength - 1;
		strncpy(buffer, d->showstr_vector[d->showstr_page], diff);
		buffer[diff] = '\0';
		SendMsgToChar(buffer, d->character.get());
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
void print_con_prompt(DescriptorData *d) {
	if (d->showstr_count) {
		return;
	}
	if (d->state == EConState::kResetStats) {
		genchar_disp_menu(d->character.get());
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
