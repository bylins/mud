// Copyright (c) 2005 Krodo
// Part of Bylins http://www.mud.ru

#include "boards.h"

#include "logger.hpp"
#include "boards.types.hpp"
#include "boards.changelog.loaders.hpp"
#include "boards.constants.hpp"
#include "boards.formatters.hpp"
#include "obj.hpp"
#include "house.h"
#include "screen.h"
#include "comm.h"
#include "privilege.hpp"
#include "char.hpp"
#include "modify.h"
#include "room.hpp"
#include "handler.h"
#include "parse.hpp"
#include "utils.h"
#include "conf.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <sstream>
#include <iomanip>

namespace Boards 
{
	// общий список досок
	using boards_list_t = std::deque<Board::shared_ptr>;
	boards_list_t board_list;

	std::string dg_script_text;

	void set_last_read(CHAR_DATA *ch, BoardTypes type, time_t date)
	{
		if (ch->get_board_date(type) < date)
		{
			ch->set_board_date(type, date);
		}
	}

	bool lvl_no_write(CHAR_DATA *ch)
	{
		if (GET_LEVEL(ch) < MIN_WRITE_LEVEL && GET_REMORT(ch) <= 0)
		{
			return true;
		}
		return false;
	}

	void message_no_write(CHAR_DATA *ch)
	{
		if (lvl_no_write(ch))
		{
			send_to_char(ch,
				"Вам нужно достигнуть %d уровня, чтобы писать в этот раздел.\r\n",
				MIN_WRITE_LEVEL);
		}
		else
		{
			send_to_char("У вас нет возможности писать в этот раздел.\r\n", ch);
		}
	}

	void message_no_read(CHAR_DATA *ch, const Board &board)
	{
		std::string out("У вас нет возможности читать этот раздел.\r\n");
		if (board.is_special())
		{
			std::string name = board.get_name();
			lower_convert(name);
			out += "Для сообщения в формате обычной работы с доской используйте: " + name + " писать <заголовок>.\r\n";
			if (!board.get_alias().empty())
			{
				out += "Команда для быстрого добавления сообщения: "
					+ board.get_alias()
					+ " <текст сообщения в одну строку>.\r\n";
			}
		}
		send_to_char(out, ch);
	}

	void add_server_message(const std::string& subj, const std::string& text)
	{
		auto board_it = std::find_if(
			Boards::board_list.begin(),
			Boards::board_list.end(),
			[](const Board::shared_ptr p)
		{
			return p->get_type() == Boards::GODBUILD_BOARD;
		});

		if (Boards::board_list.end() == board_it)
		{
			log("SYSERROR: can't find builder board (%s:%d)", __FILE__, __LINE__);
			return;
		}

		const auto temp_message = std::make_shared<Message>();
		temp_message->author = "Сервер";
		temp_message->subject = subj;
		temp_message->text = text;
		temp_message->date = time(0);
		// чтобы при релоаде не убилось
		temp_message->unique = 1;
		temp_message->level = 1;

		(*board_it)->write_message(temp_message);
//		(*board_it)->renumerate_messages();
	}

	void dg_script_message()
	{
		if (!dg_script_text.empty())
		{
			const std::string subj = "отступы в скриптах";
			add_server_message(subj, dg_script_text);
			dg_script_text.clear();
		}
	}

	void changelog_message()
	{
		std::ifstream file(runtime_config.changelog_file_name());
		if (!file.is_open())
		{
			log("SYSERROR: can't open changelog file (%s:%d)", __FILE__, __LINE__);
			return;
		}

		auto coder_board = std::find_if(
			Boards::board_list.begin(),
			Boards::board_list.end(),
			[](const Board::shared_ptr p)
		{
			return p->get_type() == Boards::CODER_BOARD;
		});

		if (Boards::board_list.end() == coder_board)
		{
			log("SYSERROR: can't find coder board (%s:%d)", __FILE__, __LINE__);
			return;
		}

		const auto loader = ChangeLogLoader::create(runtime_config.changelog_format(), *coder_board);
		loader->load(file);
	}

	bool is_spamer(CHAR_DATA *ch, const Board& board)
	{
		if (IS_IMMORTAL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			return false;
		}
		if (board.get_lastwrite() != GET_UNIQUE(ch))
		{
			return false;
		}
		switch (board.get_type())
		{
		case CLAN_BOARD:
		case CLANNEWS_BOARD:
		case PERS_BOARD:
		case ERROR_BOARD:
		case MISPRINT_BOARD:
		case SUGGEST_BOARD:
			return false;
		default:
			break;
		}
		return true;
	}

	void DoBoard(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
	{
		if (!ch->desc)
		{
			return;
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		{
			send_to_char("Вы ослеплены!\r\n", ch);
			return;
		}
		
		Board::shared_ptr board_ptr;
		for (const auto& board_i : board_list)
		{
			if (board_i->get_type() == subcmd
				&& Static::can_see(ch, board_i))
			{
				board_ptr = board_i;
				break;
			}
		}

		if (!board_ptr)
		{
			send_to_char("Чаво?\r\n", ch);
			return;
		}
		const Board& board = *board_ptr;

		std::string buffer, buffer2 = argument;
		GetOneParam(buffer2, buffer);
		boost::trim_if(buffer2, boost::is_any_of(std::string(" \'")));
		// первый аргумент в buffer, второй в buffer2

		if (CompareParam(buffer, "список") || CompareParam(buffer, "list"))
		{
			Static::do_list(ch, board_ptr);
		}
		else if (buffer.empty()
			|| (buffer2.empty()
				&& (CompareParam(buffer, "читать")
					|| CompareParam(buffer, "read"))))
		{
			// при пустой команде или 'читать' без цифры - показываем первое
			// непрочитанное сообщение, если такое есть
			if (!Static::can_read(ch, board_ptr))
			{
				message_no_read(ch, board);
				return;
			}
			time_t const date = ch->get_board_date(board.get_type());

			// новости мада в ленточном варианте
			if ((board.get_type() == NEWS_BOARD
				|| board.get_type() == GODNEWS_BOARD
				|| board.get_type() == CODER_BOARD)
				&& !PRF_FLAGGED(ch, PRF_NEWS_MODE))
			{
				std::ostringstream body;
				Board::Formatter::shared_ptr formatter = FormattersBuilder::create(board.get_type(), body, ch, date);
				board.format_board(formatter);
				set_last_read(ch, board.get_type(), board.last_message_date());
				page_string(ch->desc, body.str());
				return;
			}
			// дрновости в ленточном варианте
			if (board.get_type() == CLANNEWS_BOARD && !PRF_FLAGGED(ch, PRF_NEWS_MODE))
			{
				std::ostringstream body;
				Board::Formatter::shared_ptr formatter = FormattersBuilder::create(board.get_type(), body, ch, date);
				board.format_board(formatter);
				set_last_read(ch, board.get_type(), board.last_message_date());
				page_string(ch->desc, body.str());
				return;
			}

			std::ostringstream body;
			time_t last_date = date;
			const auto formatter = FormattersBuilder::single_message(body, ch, date, board.get_type(), last_date);
			board.format_board_in_reverse(formatter);
			set_last_read(ch, board.get_type(), last_date);
			page_string(ch->desc, body.str());
			if (last_date == date)
			{
				send_to_char("У вас нет непрочитанных сообщений.\r\n", ch);
			}
		}
		else if (is_number(buffer.c_str())
			|| ((CompareParam(buffer, "читать") || CompareParam(buffer, "read"))
				&& is_number(buffer2.c_str())))
		{
			// если после команды стоит цифра или 'читать цифра' - пытаемся
			// показать эту мессагу, два сравнения - чтобы не загребать 'писать число...' как чтение
			size_t num = 0;
			if (!Static::can_read(ch, board_ptr))
			{
				message_no_read(ch, board);
				return;
			}
			if (CompareParam(buffer, "читать"))
			{
				num = atoi(buffer2.c_str());
			}
			else
			{
				num = atoi(buffer.c_str());
			}

			if (num <= 0
				|| num > board.messages.size())
			{
				send_to_char("Это сообщение может вам только присниться.\r\n", ch);
				return;
			}
			std::ostringstream out;
			const auto messages_index = num - 1;
			special_message_format(out, board.get_message(messages_index));
			page_string(ch->desc, out.str());
			set_last_read(ch, board.get_type(), board.messages[messages_index]->date);
		}
		else if (CompareParam(buffer, "писать")
			|| CompareParam(buffer, "write"))
		{
			if (!Static::can_write(ch, board_ptr))
			{
				message_no_write(ch);
				return;
			}
			if (is_spamer(ch, board))
			{
				send_to_char("Да вы ведь только что писали сюда.\r\n", ch);
				return;
			}

			if (subcmd == Boards::CLAN_BOARD)
			{
				if (!CLAN(ch)->check_write_board(ch))
				{
					send_to_char("Вам запретили сюда писать!\r\n", ch);
					return;
				}
			}

			if (board.is_special() && board.messages.size() >= MAX_REPORT_MESSAGES)
			{
				send_to_char(constants::OVERFLOW_MESSAGE, ch);
				return;
			}
			/// написание новостей от другого имени
			std::string name = GET_NAME(ch);
			if (PRF_FLAGGED(ch, PRF_CODERINFO)
				&& (board.get_type() == NEWS_BOARD
					|| board.get_type() == NOTICE_BOARD))
			{
				GetOneParam(buffer2, buffer);
				if (buffer.empty())
				{
					send_to_char("Впишите хотя бы имя, от кого будет опубликовано сообщение.\r\n", ch);
					return;
				}
				name = buffer;
			}
			/// генерим мессагу
			const auto tempMessage = std::make_shared<Message>();
			tempMessage->author = name;
			tempMessage->unique = GET_UNIQUE(ch);
			// для досок кроме клановых и персональных пишем левел автора (для возможной очистки кем-то)
			PRF_FLAGGED(ch, PRF_CODERINFO) ? tempMessage->level = LVL_IMPL : tempMessage->level = GET_LEVEL(ch);

			// клановым еще ранг
			if (CLAN(ch))
			{
				tempMessage->rank = CLAN_MEMBER(ch)->rank_num;
			}
			else
			{
				tempMessage->rank = 0;
			}

			// обработка сабжа
			boost::trim(buffer2);
			if (buffer2.length() > 40)
			{
				buffer2.erase(40, std::string::npos);
				send_to_char(ch, "Тема сообщения укорочена до '%s'.\r\n", buffer2.c_str());
			}
			// для ошибок опечаток впереди пишем комнату, где стоит отправитель
			std::string subj;
			if (subcmd == ERROR_BOARD || subcmd == MISPRINT_BOARD)
			{
				subj += "[" + boost::lexical_cast<std::string>(GET_ROOM_VNUM(ch->in_room)) + "] ";
			}
			subj += buffer2;
			tempMessage->subject = subj;
			// дату номер и текст пишем уже по факту сохранения
			ch->desc->message = tempMessage;
			ch->desc->board = board_ptr;

			send_to_char(ch, "Можете писать сообщение.  (/s записать /h помощь)\r\n");
			STATE(ch->desc) = CON_WRITEBOARD;
			AbstractStringWriter::shared_ptr writer(new StdStringWriter());
			string_write(ch->desc, writer, MAX_MESSAGE_LENGTH, 0, NULL);
		}
		else if (CompareParam(buffer, "очистить") || CompareParam(buffer, "remove"))
		{
			if (!is_number(buffer2.c_str()))
			{
				send_to_char("Укажите корректный номер сообщения.\r\n", ch);
				return;
			}
			const size_t message_number = atoi(buffer2.c_str());
			const auto messages_index = message_number - 1;
			if (messages_index >= board.messages.size())
			{
				send_to_char("Это сообщение может вам только присниться.\r\n", ch);
				return;
			}
			set_last_read(ch, board.get_type(), board.messages[messages_index]->date);
			// или он может делетить любые мессаги (по левелу/рангу), или только свои
			if (!Static::full_access(ch, board_ptr))
			{
				if (board.messages[messages_index]->unique != GET_UNIQUE(ch))
				{
					send_to_char("У вас нет возможности удалить это сообщение.\r\n", ch);
					return;
				}
			}
			else if (board.get_type() != CLAN_BOARD
				&& board.get_type() != CLANNEWS_BOARD
				&& board.get_type() != PERS_BOARD
				&& !PRF_FLAGGED(ch, PRF_CODERINFO)
				&& GET_LEVEL(ch) < board.messages[messages_index]->level)
			{
				// для простых досок сверяем левела (для контроля иммов)
				// клановые ниже, у персональных смысла нет
				send_to_char("У вас нет возможности удалить это сообщение.\r\n", ch);
				return;
			}
			else if (board.get_type() == CLAN_BOARD
				|| board.get_type() == CLANNEWS_BOARD)
			{
				// у кого привилегия на новости, те могут удалять везде чужие, если ранк автора такой же или ниже
				if (CLAN_MEMBER(ch)->rank_num > board.messages[messages_index]->rank)
				{
					send_to_char("У вас нет возможности удалить это сообщение.\r\n", ch);
					return;
				}
			}
			// собственно делетим и проставляем сдвиг номеров
			board_ptr->erase_message(messages_index);
			if (board.get_lastwrite() == GET_UNIQUE(ch))
			{
				board_ptr->set_lastwrite_uid(0);
			}
			board_ptr->Save();
			send_to_char("Сообщение удалено.\r\n", ch);
		}
		else
		{
			send_to_char("Неверный формат команды, ознакомьтесь со 'справка доски'.\r\n", ch);
		}
	}

	bool act_board(CHAR_DATA *ch, int vnum, char *buf_)
	{
		switch (vnum)
		{
		case GENERAL_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GENERAL_BOARD);
			break;
		case GODGENERAL_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GODGENERAL_BOARD);
			break;
		case GODPUNISH_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GODPUNISH_BOARD);
			break;
		case GODBUILD_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GODBUILD_BOARD);
			break;
		case GODCODE_BOARD_OBJ:
			DoBoard(ch, buf_, 0, CODER_BOARD);
			break;
		default:
			return false;
		}

		return true;
	}

	std::string print_access(const std::bitset<Boards::ACCESS_NUM> &acess_flags)
	{
		std::string access;
		if (acess_flags.test(ACCESS_FULL))
		{
			access = "полный+";
		}
		else if (acess_flags.test(ACCESS_CAN_READ)
			&& acess_flags.test(ACCESS_CAN_WRITE))
		{
			access = "полный";
		}
		else if (acess_flags.test(ACCESS_CAN_READ))
		{
			access = "чтение";
		}
		else if (acess_flags.test(ACCESS_CAN_WRITE))
		{
			access = "запись";
		}
		else if (acess_flags.test(ACCESS_CAN_SEE))
		{
			access = "нет";
		}
		else
		{
			access = "";
		}
		return access;
	}
	// чтобы не травмировать народ спешиалы вешаем на старые доски с новым содержимым
	int Static::Special(CHAR_DATA* ch, void* me, int cmd, char* argument)
	{
		OBJ_DATA *board = (OBJ_DATA *)me;
		if (!ch->desc)
		{
			return 0;
		}
		// список сообщений
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		boost::trim_if(buffer, boost::is_any_of(std::string(" \'")));

		if ((CMD_IS("смотреть") || CMD_IS("осмотреть") || CMD_IS("look") || CMD_IS("examine")
			|| CMD_IS("читать") || CMD_IS("read")) && (CompareParam(buffer2, "доска") || CompareParam(buffer2, "board")))
		{
			// эта мутная запись для всяких 'см на доску' и подобных написаний в два слова
			if (buffer2.empty()
				|| (buffer.empty() && !CompareParam(buffer2, "доска") && !CompareParam(buffer2, "board"))
				|| (!buffer.empty() && !CompareParam(buffer, "доска") && !CompareParam(buffer2, "board")))
			{
				return 0;
			}

			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_), "%s", "список");

			if (act_board(ch, GET_OBJ_VNUM(board), buf_))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		// для писем
		if ((CMD_IS("читать") || CMD_IS("read")) && !buffer2.empty() && a_isdigit(buffer2[0]))
		{
			if (buffer2.find('.') != std::string::npos)
			{
				return 0;
			}
		}

		// вывод сообщения написать сообщение очистить сообщение
		if (((CMD_IS("читать") || CMD_IS("read")) && !buffer2.empty() && a_isdigit(buffer2[0]))
			|| CMD_IS("писать") || CMD_IS("write") || CMD_IS("очистить") || CMD_IS("remove"))
		{
			// перехватываем запарки вида 'писать дрв' сидя на ренте с доской от вече
			if ((CMD_IS("писать") || CMD_IS("write")) && !buffer2.empty())
			{
				for (auto i = board_list.begin(); i != board_list.end(); ++i)
				{
					if (isname(buffer2, (*i)->get_name()))
					{
						send_to_char(ch,
							"Первое слово вашего заголовка совпадает с названием одной из досок сообщений,\r\n"
							"Во избежание недоразумений воспользуйтесь форматом '<имя-доски> писать <заголовок>'.\r\n");
						return 1;
					}
				}
			}
			// общая доска
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_), "%s%s", cmd_info[cmd].command, argument);

			if (act_board(ch, GET_OBJ_VNUM(board), buf_))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		return 0;
	}

	// выводит при заходе в игру инфу о новых сообщениях на досках
	// возвращает false если ничего не было показано
	bool Static::LoginInfo(CHAR_DATA* ch)
	{
		std::ostringstream buffer, news;
		bool has_message = 0;
		buffer << "\r\nВас ожидают сообщения:\r\n";

		for (auto board = board_list.begin(); board != board_list.end(); ++board)
		{
			// доска не видна или можно только писать, опечатки тож не спамим
			if (!can_read(ch, *board)
				|| ((*board)->get_type() == MISPRINT_BOARD
					&& !PRF_FLAGGED(ch, PRF_MISPRINT))
				|| (*board)->get_type() == CODER_BOARD)
			{
				continue;
			}
			const int unread = (*board)->count_unread(ch->get_board_date((*board)->get_type()));
			if (unread > 0)
			{
				has_message = 1;
				if ((*board)->get_type() == NEWS_BOARD
					|| (*board)->get_type() == GODNEWS_BOARD
					|| (*board)->get_type() == CLANNEWS_BOARD)
				{
					news << std::setw(4) << unread << " в разделе '" << (*board)->get_description() << "' "
						<< CCWHT(ch, C_NRM) << "(" << (*board)->get_name() << ")" << CCNRM(ch, C_NRM) << ".\r\n";
				}
				else
				{
					buffer << std::setw(4) << unread << " в разделе '" << (*board)->get_description() << "' "
						<< CCWHT(ch, C_NRM) << "(" << (*board)->get_name() << ")" << CCNRM(ch, C_NRM) << ".\r\n";
				}
			}
		}

		if (has_message)
		{
			buffer << news.str();
			send_to_char(buffer.str(), ch);
			return true;
		}
		return false;
	}

	Boards::Board::shared_ptr Static::create_board(BoardTypes type, const std::string &name, const std::string &desc, const std::string &file)
	{
		const auto board = std::make_shared<Board>(type);
		board->set_name(name);
		board->set_description(desc);
		board->set_file_name(file);
		board->Load();

		if (board->get_type() == ERROR_BOARD)
		{
			board->set_alias("ошибка");
		}
		else if (board->get_type() == MISPRINT_BOARD)
		{
			board->set_alias("опечатка");
		}
		else if (board->get_type() == SUGGEST_BOARD)
		{
			board->set_alias("мысль");
		}

		switch (board->get_type())
		{
		case ERROR_BOARD:
		case MISPRINT_BOARD:
		case SUGGEST_BOARD:
		case CODER_BOARD:
			board->set_blind(true);
			break;

		default:
			break;
		}

		board_list.push_back(board);
		return board;
	}

	void Static::do_list(CHAR_DATA* ch, const Board::shared_ptr board_ptr)
	{
		if (!can_read(ch, board_ptr))
		{
			const auto& board = *board_ptr;
			message_no_read(ch, board);
			return;
		}

		std::ostringstream body;
		body << "Это доска, на которой всякие разные личности оставили свои IMHO.\r\n"
			<< "Формат: ЧИТАТЬ/ОЧИСТИТЬ <номер сообщения>, ПИСАТЬ <тема сообщения>.\r\n";
		if (board_ptr->empty())
		{
			body << "Никто ниче не накарябал, слава Богам.\r\n";
			send_to_char(body.str(), ch);
			return;
		}
		else
		{
			body << "Всего сообщений: " << board_ptr->messages_count() << "\r\n";
		}

		const auto date = ch->get_board_date(board_ptr->get_type());
		const auto formatter = FormattersBuilder::messages_list(body, ch, date);
		board_ptr->format_board(formatter);
		page_string(ch->desc, body.str());
	}

	bool Static::can_see(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_CAN_SEE);
	}

	bool Static::can_read(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_CAN_READ);
	}

	bool Static::can_write(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_CAN_WRITE);
	}

	bool Static::full_access(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_FULL);
	}

	void Static::clan_delete_message(const std::string &name, int vnum)
	{
		const std::string subj = "неактивная дружина";
		const std::string text = boost::str(boost::format(
			"Дружина %1% была автоматически удалена.\r\n"
			"Номер зоны: %2%\r\n") % name % vnum);
		add_server_message(subj, text);
	}

	void Static::new_message_notify(const Board::shared_ptr board)
	{
		if (board->get_type() != PERS_BOARD
			&& board->get_type() != CODER_BOARD
			&& !board->empty())
		{
			const Message &msg = *board->get_last_message();
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_),
				"Новое сообщение в разделе '%s' от %s, тема: %s\r\n",
				board->get_name().c_str(), msg.author.c_str(),
				msg.subject.c_str());
			// оповещаем весь мад кто с правами чтения
			for (DESCRIPTOR_DATA *f = descriptor_list; f; f = f->next)
			{
				if (f->character
					&& STATE(f) == CON_PLAYING
					&& PRF_FLAGGED(f->character, PRF_BOARD_MODE)
					&& can_read(f->character.get(), board)
					&& (board->get_type() != MISPRINT_BOARD
						|| PRF_FLAGGED(f->character, PRF_MISPRINT)))
				{
					send_to_char(buf_, f->character.get());
				}
			}
		}
	}

	// создание всех досок, кроме клановых и персональных
	void Static::BoardInit()
	{
		board_list.clear();

		create_board(GENERAL_BOARD, "Вече", "Базарная площадь", ETC_BOARD"general.board");
		create_board(NEWS_BOARD, "Новости", "Анонсы и новости Былин", ETC_BOARD"news.board");
		create_board(IDEA_BOARD, "Идеи", "Идеи и их обсуждение", ETC_BOARD"idea.board");
		create_board(ERROR_BOARD, "Баги", "Сообщения об ошибках в мире", ETC_BOARD"error.board");
		create_board(GODNEWS_BOARD, "GodNews", "Божественные новости", ETC_BOARD"god-news.board");
		create_board(GODGENERAL_BOARD, "Божества", "Божественная базарная площадь", ETC_BOARD"god-general.board");
		create_board(GODBUILD_BOARD, "Билдер", "Заметки билдеров", ETC_BOARD"god-build.board");
		create_board(GODPUNISH_BOARD, "Наказания", "Комментарии к наказаниям", ETC_BOARD"god-punish.board");
		create_board(NOTICE_BOARD, "Анонсы", "Сообщения от администрации", ETC_BOARD"notice.board");
		create_board(MISPRINT_BOARD, "Очепятки", "Опечатки в игровых локациях", ETC_BOARD"misprint.board");
		create_board(SUGGEST_BOARD, "Придумки", "Для идей в приватном режиме", ETC_BOARD"suggest.board");
		create_board(CODER_BOARD, "Кодер", "Изменения в коде Былин", "");

		dg_script_message();
		changelog_message();
	}

	// лоад/релоад клановых досок
	void Static::ClanInit()
	{
		const auto erase_predicate = [](const auto& board)
		{
			return board->get_type() == CLAN_BOARD
				|| board->get_type() == CLANNEWS_BOARD;
		};
		// чистим клан-доски для релоада
		board_list.erase(
			std::remove_if(board_list.begin(), board_list.end(), erase_predicate),
			board_list.end());

		for (const auto& clan : Clan::ClanList)
		{
			std::string name = clan->GetAbbrev();
			CreateFileName(name);

			{
				// делаем клановую доску
				const auto board = std::make_shared<Board>(CLAN_BOARD);
				board->set_name("ДрВече");
				std::string description = "Основной раздел Дружины ";
				description += clan->GetAbbrev();
				board->set_description(description);
				board->set_clan_rent(clan->GetRent());
				board->set_file_name(LIB_HOUSE + name + "/" + name + ".board");
				board->Load();
				board_list.push_back(board);
			}

			{
				// делаем клановые новости
				const auto board = std::make_shared<Board>(CLANNEWS_BOARD);
				board->set_name("ДрНовости");
				std::string description = "Новости Дружины ";
				description += clan->GetAbbrev();
				board->set_description(description);
				board->set_clan_rent(clan->GetRent());
				board->set_file_name(LIB_HOUSE + name + "/" + name + "-news.board");
				board->Load();
				board_list.push_back(board);
			}
		}
	}

	// чистим для релоада
	void Static::clear_god_boards()
	{
		const auto erase_predicate = [](const auto& board) { return board->get_type() == PERS_BOARD; };
		board_list.erase(
			std::remove_if(board_list.begin(), board_list.end(), erase_predicate),
			board_list.end());
	}

	// втыкаем блокнот имму
	void Static::init_god_board(long uid, std::string name)
	{
		const auto board = std::make_shared<Board>(PERS_BOARD);
		board->set_name("Блокнот");
		board->set_description("Ваш раздел для заметок");
		board->set_pers_unique(uid);
		board->set_pers_name(name);
		// генерим имя и лоадим по возможности
		std::string tmp_name = name;
		CreateFileName(tmp_name);
		board->set_file_name(ETC_BOARD + tmp_name + ".board");
		board->Load();
		board_list.push_back(board);
	}

	// * Релоад всех досок разом.
	void Static::reload_all()
	{
		BoardInit();
		Privilege::load_god_boards();
		ClanInit();
	}

std::string Static::print_stats(CHAR_DATA* ch, const Board::shared_ptr board, int num)
{
	const std::string access = print_access(get_access(ch, board));
	if (access.empty())
	{
		return "";
	}

	std::string out;
	if (IS_IMMORTAL(ch)
		|| PRF_FLAGGED(ch, PRF_CODERINFO)
		|| !board->get_blind())
	{
		const int unread = board->count_unread(ch->get_board_date(board->get_type()));
		out += boost::str(boost::format
			(" %2d)  %10s   [%3d|%3d]   %40s  %6s\r\n")
			% num % board->get_name() % unread % board->messages_count()
			% board->get_description() % access);
	}
	else
	{
		std::string tmp = board->get_description();
		if (!board->get_alias().empty())
		{
			tmp += " (" + board->get_alias() + ")";
		}
		out += boost::str(boost::format(" %2d)  %10s   [ - | - ]   %40s  %6s\r\n")
			% num % board->get_name() % tmp % access);
	}
	return out;
}

std::bitset<ACCESS_NUM> Static::get_access(CHAR_DATA *ch, const Board::shared_ptr board)
{
	std::bitset<ACCESS_NUM> access;

	switch (board->get_type())
	{
	case GENERAL_BOARD:
	case IDEA_BOARD:
		// все читают, пишут с мин.левела, 32 и по привилегии полный
		if (IS_GOD(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case ERROR_BOARD:
	case MISPRINT_BOARD:
		// все пишут с мин.левела, 34 и по привилегии полный
		if (IS_IMPL(ch)
			|| Privilege::check_flag(ch, Privilege::BOARDS)
			|| Privilege::check_flag(ch, Privilege::MISPRINT))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_WRITE);
			if (IS_GOD(ch)) access.set(ACCESS_CAN_READ);
		}
		break;
	case NEWS_BOARD:
		// все читают, 34 и по привилегии полный
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
		}
		break;
	case GODNEWS_BOARD:
		// 32 читают, 34 и по привилегии полный
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
		}
		break;
	case GODGENERAL_BOARD:
	case GODPUNISH_BOARD:
		// 32 читают/пишут, 34 полный
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case GODBUILD_BOARD:
	case GODCODE_BOARD:
		// 33 читают/пишут, 34 и по привилегии полный
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GRGOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case PERS_BOARD:
		if (IS_GOD(ch) && board->get_pers_uniq() == GET_UNIQUE(ch)
			&& CompareParam(board->get_pers_name(), GET_NAME(ch), 1))
		{
			access.set();
		}
		break;
	case CLAN_BOARD:
		// от клан-новостей отличается тем, что писать могут все звания
		if (CLAN(ch) && CLAN(ch)->GetRent() == board->get_clan_rent())
		{
			// воевода
			if (CLAN(ch)->CheckPrivilege(CLAN_MEMBER(ch)->rank_num, ClanSystem::MAY_CLAN_NEWS))
			{
				access.set();
			}
			else
			{
				access.set(ACCESS_CAN_SEE);
				access.set(ACCESS_CAN_READ);
				access.set(ACCESS_CAN_WRITE);
			}
		}
		break;
	case CLANNEWS_BOARD:
		// неклановые не видят, клановые могут читать все, писать могут по привилегии, воевода может стирать чужие
		if (CLAN(ch) && CLAN(ch)->GetRent() == board->get_clan_rent())
		{
			if (CLAN(ch)->CheckPrivilege(CLAN_MEMBER(ch)->rank_num, ClanSystem::MAY_CLAN_NEWS))
			{
				access.set();
			}
			else
			{
				access.set(ACCESS_CAN_SEE);
				access.set(ACCESS_CAN_READ);
			}
		}
		break;
	case NOTICE_BOARD:
		// 34+ и по привилегии полный, 32+ пишут/читают, остальные только читают
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
		}
		break;
	case SUGGEST_BOARD:
		// по привилегии boards/suggest и 34 полный, остальным только запись с мин левела/морта
		if (IS_IMPL(ch)
			|| Privilege::check_flag(ch, Privilege::BOARDS)
			|| Privilege::check_flag(ch, Privilege::SUGGEST))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case CODER_BOARD:
		access.set(ACCESS_CAN_SEE);
		access.set(ACCESS_CAN_READ);
		break;

	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
	}

	// категории граждан, которые писать могут только на клан-доски
	if (!IS_IMMORTAL(ch)
		&& (PLR_FLAGGED(ch, PLR_HELLED)
			|| PLR_FLAGGED(ch, PLR_NAMED)
			|| PLR_FLAGGED(ch, PLR_DUMB)
			|| lvl_no_write(ch))
		&& (board->get_type() != CLAN_BOARD && board->get_type() != CLANNEWS_BOARD))
	{
		access.reset(ACCESS_CAN_WRITE);
		access.reset(ACCESS_FULL);
	}

	return access;
}

void DoBoardList(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
		return;

	std::string out(
		" Ном         Имя  Новых|Всего                                  Описание  Доступ\r\n"
		" ===  ==========  ===========  ========================================  ======\r\n");
	int num = 1;
	for (const auto& board : board_list)
	{
		if (!board->get_blind())
		{
			out += Static::print_stats(ch, board, num++);
		}
	}
	// два цикла для сквозной нумерации без заморочек
	out += " ---  ----------  -----------  ----------------------------------------  ------\r\n";
	for (const auto& board : board_list)
	{
		if (board->get_blind())
		{
			out += Static::print_stats(ch, board, num++);
		}
	}

	send_to_char(out, ch);
}

void report_on_board(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	if (IS_NPC(ch)) return;
	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
	{
		send_to_char("Пустое сообщение пропущено.\r\n", ch);
		return;
	}

	const auto predicate = [&](const auto board) { return board->get_type() == subcmd; };
	auto board = std::find_if(board_list.begin(), board_list.end(), predicate);

	if (board == board_list.end())
	{
		send_to_char("Доска тупо не найдена... :/\r\n", ch);
		return;
	}
	if (!Static::can_write(ch, *board))
	{
		message_no_write(ch);
		return;
	}
	if ((*board)->is_special()
		&& (*board)->messages.size() >= MAX_REPORT_MESSAGES)
	{
		send_to_char(constants::OVERFLOW_MESSAGE, ch);
		return;
	}
	// генерим мессагу (TODO: копипаст с написания на доску, надо бы вынести)
	const auto temp_message = std::make_shared<Message>();
	temp_message->author = GET_NAME(ch) ? GET_NAME(ch) : "неизвестен";
	temp_message->unique = GET_UNIQUE(ch);
	// для досок кроме клановых и персональных пишет левел автора (для возможной очистки кем-то)
	temp_message->level = GET_LEVEL(ch);
	temp_message->rank = 0;
	temp_message->subject = "[" + boost::lexical_cast<std::string>(GET_ROOM_VNUM(ch->in_room)) + "]";
	temp_message->text = argument;
	temp_message->date = time(0);

	(*board)->write_message(temp_message);
//	(*board)->renumerate_messages();
//	(*board)->Save();
	send_to_char(ch,
		"Текст сообщения:\r\n"
		"%s\r\n\r\n"
		"Записали. Заранее благодарны.\r\n"
		"                        Боги.\r\n", argument);
}

} // namespace Boards

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
