// Copyright (c) 2005 Krodo
// Part of Bylins http://www.mud.ru

#include <fstream>
#include <sstream>
#include <iomanip>
#include "conf.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "house.h"
#include "screen.h"
#include "comm.h"
#include "privilege.hpp"
#include "boards.h"
#include "char.hpp"
#include "modify.h"
#include "room.hpp"
#include "handler.h"
#include "parse.hpp"

namespace Boards
{

// общий список досок
std::vector<boost::shared_ptr<Board>> board_list;
// предметы, на которые вешаем спешиалы для старого способа работы с досками
const int GODGENERAL_BOARD_OBJ = 250;
const int GENERAL_BOARD_OBJ = 251;
const int GODCODE_BOARD_OBJ = 253;
const int GODPUNISH_BOARD_OBJ = 257;
const int GODBUILD_BOARD_OBJ = 259;
// максимальный размер сообщения
const int MAX_MESSAGE_LENGTH = 4096;
// мин.левел для поста на общих досках
const int MIN_WRITE_LEVEL = 6;
// максимальное кол-во сообщений на одной доске
const unsigned int MAX_BOARD_MESSAGES = 200;
// максимальное кол-во сообщений на спец.досках
const unsigned int MAX_REPORT_MESSAGES = 999;
const char *OVERFLOW_MESSAGE = "Да, набросали всего столько, что файл переполнен. Напомните об этом богам!\r\n";
const char *CHANGELOG_FILE = "../bin/changelog";
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
				+  board.get_alias()
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
		boost::bind(&Board::get_type, _1) == Boards::GODBUILD_BOARD);

	if (Boards::board_list.end() == board_it)
	{
		log("SYSERROR: can't find builder board (%s:%d)", __FILE__, __LINE__);
		return;
	}

	MessagePtr temp_message(new Message);
	temp_message->author = "Сервер";
	temp_message->subject = subj;
	temp_message->text = text;
	temp_message->date = time(0);
	// чтобы при релоаде не убилось
	temp_message->unique = 1;
	temp_message->level = 1;

	(*board_it)->add_message(temp_message);
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

std::string iconv_convert(const char *from, const char *to, std::string text)
{
#ifdef HAVE_ICONV
	iconv_t cnv = iconv_open(to, from);
	if (cnv == (iconv_t) - 1)
	{
		iconv_close(cnv);
		return "";
	}
	char *outbuf;
	if ((outbuf = (char *) malloc(text.length()*2 + 1)) == NULL)
	{
		iconv_close(cnv);
		return "";
	}
	char *ip = (char *) text.c_str(), *op = outbuf;
	size_t icount = text.length(), ocount = text.length()*2;

	if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t) - 1)
	{
		outbuf[text.length()*2 - ocount] = '\0';
		text = outbuf;
	}
	else
	{
		text = "";
	}

	free(outbuf);
	iconv_close(cnv);
#endif
	return text;
}

MessagePtr create_changelog_msg(std::string &author, std::string &desc,
	time_t parsed_time)
{
	MessagePtr message(new Message);
	// имя автора
	const std::size_t e_pos = author.find('<');
	const std::size_t s_pos = author.find(' ');
	if (e_pos != std::string::npos
		|| s_pos != std::string::npos)
	{
		author = author.substr(0, std::min(e_pos, s_pos));
	}
	boost::trim(author);
	message->author = author;
	boost::trim(desc);
	message->text = iconv_convert("UTF-8", "KOI8-R", desc) + "\r\n";
	// из текста первая строка в заголовок
	std::string subj(message->text.begin(),
		std::find(message->text.begin(), message->text.end(), '\n'));
	if (subj.size() > 40)
	{
		subj = subj.substr(0, 40);
	}
	boost::trim(subj);
	message->subject = subj;
	message->date = parsed_time;
	message->unique = 1;
	message->level = 1;

	return message;
}

void changelog_message()
{
	std::ifstream file(CHANGELOG_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: can't open changelog file (%s:%d)", __FILE__, __LINE__);
		return;
	}
	auto coder_board = std::find_if(
		Boards::board_list.begin(),
		Boards::board_list.end(),
		boost::bind(&Board::get_type, _1) == Boards::CODER_BOARD);

	if (Boards::board_list.end() == coder_board)
	{
		log("SYSERROR: can't find coder board (%s:%d)", __FILE__, __LINE__);
		return;
	}

	std::string buf_str, author, date, desc;
	bool description = false;
	std::vector<MessagePtr> coder_msgs;

	while (std::getline(file, buf_str))
	{
		const std::size_t pos = buf_str.find(' ');
		if (pos != std::string::npos)
		{
			std::string tmp_str = buf_str.substr(0, pos);
			if (tmp_str == "changeset:")
			{
				const time_t parsed_time = parse_asctime(date);
				if (parsed_time >= 1326121851 // переход на меркуриал
					&& !author.empty() && !date.empty() && !desc.empty())
				{
					coder_msgs.push_back(create_changelog_msg(
						author, desc, parsed_time));
				}
				description = false;
				author.clear();
				date.clear();
				desc.clear();
				continue;
			}
			if (description)
			{
				desc += buf_str + "\r\n";
				continue;
			}
			else
			{
				if (tmp_str == "user:")
				{
					author = buf_str.substr(pos);
					boost::trim(author);
				}
				else if (tmp_str == "date:")
				{
					date = buf_str.substr(pos);
					boost::trim(date);
				}
			}
		}
		else if (buf_str == "description:")
		{
			description = true;
		}
		else
		{
			desc += buf_str + "\r\n";
		}
	}

	for (auto i = coder_msgs.rbegin(); i != coder_msgs.rend(); ++i)
	{
		(*coder_board)->add_message(*i);
	}
}

} // namespace BoardSystem

using namespace Boards;

void Board::create_board(BoardTypes type, const std::string &name, const std::string &desc, const std::string &file)
{
	boost::shared_ptr<Board> board = boost::make_shared<Board>(type);
	board->name_ = name;
	board->desc_ = desc;
	board->file_ = file;
	board->Load();

	if (board->get_type() == ERROR_BOARD)
	{
		board->alias_ = "ошибка";
	}
	else if (board->get_type() == MISPRINT_BOARD)
	{
		board->alias_ = "опечатка";
	}
	else if (board->get_type() == SUGGEST_BOARD)
	{
		board->alias_ = "мысль";
	}

	switch (board->get_type())
	{
		case ERROR_BOARD:
		case MISPRINT_BOARD:
		case SUGGEST_BOARD:
		case CODER_BOARD:
			board->blind_ = true;
			break;
		default:
			break;
	}

	board_list.push_back(board);
}

// создание всех досок, кроме клановых и персональных
void Board::BoardInit()
{
	board_list.clear();

	create_board(GENERAL_BOARD, "Вече", "Базарная площадь", ETC_BOARD"general.board");
	create_board(NEWS_BOARD, "Новости", "Анонсы и новости Былин", ETC_BOARD"news.board");
	create_board(IDEA_BOARD, "Идеи", "Идеи и их обсуждение", ETC_BOARD"idea.board");
	create_board(ERROR_BOARD, "Баги", "Сообщения об ошибках в мире", ETC_BOARD"error.board");
	create_board(GODNEWS_BOARD, "GodNews", "Божественные новости", ETC_BOARD"god-news.board");
	create_board(GODGENERAL_BOARD, "Божества", "Божественная базарная площадь", ETC_BOARD"god-general.board");
	create_board(GODBUILD_BOARD, "Билдер", "Заметки билдеров", ETC_BOARD"god-build.board");
	//create_board(GODCODE_BOARD, "Кодер", "Заметки кодеров", ETC_BOARD"god-code.board");
	create_board(GODPUNISH_BOARD, "Наказания", "Комментарии к наказаниям", ETC_BOARD"god-punish.board");
	create_board(NOTICE_BOARD, "Анонсы", "Сообщения от администрации", ETC_BOARD"notice.board");
	create_board(MISPRINT_BOARD, "Очепятки", "Опечатки в игровых локациях", ETC_BOARD"misprint.board");
	create_board(SUGGEST_BOARD, "Придумки", "Для идей в приватном режиме", ETC_BOARD"suggest.board");
	create_board(CODER_BOARD, "Кодер", "Изменения в коде Былин", "");

	dg_script_message();
	changelog_message();
}

// лоад/релоад клановых досок
void Board::ClanInit()
{
	// чистим клан-доски для релоада
	board_list.erase(std::remove_if(
		board_list.begin(), board_list.end(),
		boost::bind(&Board::type_, _1) == CLAN_BOARD
			|| boost::bind(&Board::type_, _1) == CLANNEWS_BOARD),
		board_list.end());

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		std::string name = (*clan)->GetAbbrev();
		CreateFileName(name);
		// делаем клановую доску
		boost::shared_ptr<Board> board = boost::make_shared<Board>(CLAN_BOARD);
		board->name_ = "ДрВече";
		board->desc_ = "Основной раздел Дружины ";
		board->desc_ += (*clan)->GetAbbrev();
		board->clan_rent_ = (*clan)->GetRent();
		board->file_ = LIB_HOUSE + name + "/" + name + ".board";
		board->Load();
		board_list.push_back(board);
		// делаем клановые новости
		board = boost::make_shared<Board>(CLANNEWS_BOARD);
		board->name_ = "ДрНовости";
		board->desc_ = "Новости Дружины ";
		board->desc_ += (*clan)->GetAbbrev();
		board->clan_rent_ = (*clan)->GetRent();
		board->file_ = LIB_HOUSE + name + "/" + name + "-news.board";
		board->Load();
		board_list.push_back(board);
	}
}

// чистим для релоада
void Board::clear_god_boards()
{
	board_list.erase(std::remove_if(
		board_list.begin(), board_list.end(),
		boost::bind(&Board::type_, _1) == PERS_BOARD),
		board_list.end());
}

// втыкаем блокнот имму
void Board::init_god_board(long uid, std::string name)
{
	boost::shared_ptr<Board> board = boost::make_shared<Board>(PERS_BOARD);
	board->name_ = "Блокнот";
	board->desc_ = "Ваш раздел для заметок";
	board->pers_unique_ = uid;
	board->pers_name_ = name;
	// генерим имя и лоадим по возможности
	std::string tmp_name = name;
	CreateFileName(tmp_name);
	board->file_ = ETC_BOARD + tmp_name + ".board";
	board->Load();
	board_list.push_back(board);
}

// * Релоад всех досок разом.
void Board::reload_all()
{
	BoardInit();
	Privilege::load_god_boards();
	ClanInit();
}

// подгружаем доску, если она существует
void Board::Load()
{
	if (file_.empty())
	{
		return;
	}
	std::ifstream file(file_.c_str());
	if (!file.is_open())
	{
		this->Save();
		return;
	}

	std::string buffer;
	while (file >> buffer)
	{
		if (buffer == "Message:")
		{
			MessagePtr message(new Message);
			file >> message->num;
			file >> message->author;
			file >> message->unique;
			file >> message->level;
			file >> message->date;
			file >> message->rank;

			ReadEndString(file);
			std::getline(file, message->subject, '~');
			ReadEndString(file);
			std::getline(file, message->text, '~');
			// не помешает глянуть че мы там залоадили
			if (message->author.empty() || !message->unique || !message->level || !message->date)
				continue;
			messages.push_back(message);
		}
		else if (buffer == "Type:")
		{
			// type у доски в любом случае уже есть, здесь остается только
			// его проверить на всякий случай, это удобно еще и тем, что
			// тип пишется в файле первым полем и дальше можно все стопнуть
			int num;
			file >> num;
			if (num != type_)
			{
				log("SYSERROR: wrong board type=%d from %s, expected=%d (%s %s %d)",
					num, file_.c_str(), type_, __FILE__, __func__, __LINE__);
				return;
			}
		}
		else if (buffer == "Clan:")
			file >> clan_rent_;
		else if (buffer == "PersUID:")
			file >> pers_unique_;
		else if (buffer == "PersName:")
			file >> pers_name_;
	}
	file.close();

	this->Save();
}

// сохраняем доску
void Board::Save()
{
	if (file_.empty())
	{
		return;
	}
	std::ofstream file(file_.c_str());
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", file_.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	file << "Type: " << type_ << " "
	<< "Clan: " << clan_rent_ << " "
	<< "PersUID: " << pers_unique_ << " "
	<< "PersName: " << (pers_name_.empty() ? "none" : pers_name_) << "\n";
	for (MessageListType::const_iterator message = this->messages.begin(); message != this->messages.end(); ++message)
	{
		file << "Message: " << (*message)->num << "\n"
		<< (*message)->author << " "
		<< (*message)->unique << " "
		<< (*message)->level << " "
		<< (*message)->date << " "
		<< (*message)->rank << "\n"
		<< (*message)->subject << "~\n"
		<< (*message)->text << "~\n";
	}
	file.close();
}

// выводим указанное сообщение
void Board::ShowMessage(CHAR_DATA * ch, MessagePtr message)
{
	std::ostringstream buffer;
	char timeBuf[17];
	strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&message->date));

	buffer << "[" << message->num + 1 << "] "
	<< timeBuf << " "
	<< "(" << message->author << ") :: "
	<< message->subject << "\r\n\r\n"
	<< message->text << "\r\n";

	page_string(ch->desc, buffer.str());
}

void Board::do_list(CHAR_DATA *ch) const
{
	if (!can_read(ch, *this))
	{
		message_no_read(ch, *this);
		return;
	}

	std::ostringstream body;
	body << "Это доска, на которой всякие разные личности оставили свои IMHO.\r\n"
	<< "Формат: ЧИТАТЬ/ОЧИСТИТЬ <номер сообщения>, ПИСАТЬ <тема сообщения>.\r\n";
	if (messages.empty())
	{
		body << "Никто ниче не накарябал, слава Богам.\r\n";
		send_to_char(body.str(), ch);
		return;
	}
	else
	{
		body << "Всего сообщений: " << messages.size() << "\r\n";
	}

	char timeBuf[17];
	for (auto message = messages.rbegin(); message != messages.rend(); ++message)
	{
		if ((*message)->date > ch->get_board_date(type_))
			body << CCWHT(ch, C_NRM); // для выделения новых мессаг светло-белым

		strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&(*message)->date));
		body << "[" << (*message)->num + 1 << "] " << timeBuf << " "
		<< "(" << (*message)->author << ") :: "
		<< (*message)->subject << "\r\n" <<  CCNRM(ch, C_NRM);
	}
	page_string(ch->desc, body.str());
}

bool is_spamer(CHAR_DATA *ch, const Board &board)
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

ACMD(DoBoard)
{
	if (!ch->desc)
		return;
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("Вы ослеплены!\r\n", ch);
		return;
	}

	std::vector<boost::shared_ptr<Board>>::const_iterator board_it;
	for (board_it = board_list.begin(); board_it != board_list.end(); ++board_it)
	{
		if ((*board_it)->type_ == subcmd && can_see(ch, **board_it))
			break;
	}
	if (board_it == board_list.end())
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}
	Board &board = **board_it;

	std::string buffer, buffer2 = argument;
	GetOneParam(buffer2, buffer);
	boost::trim_if(buffer2, boost::is_any_of(std::string(" \'")));
	// первый аргумент в buffer, второй в buffer2

	if (CompareParam(buffer, "список") || CompareParam(buffer, "list"))
	{
		board.do_list(ch);
	}
	else if (buffer.empty()
		|| ((CompareParam(buffer, "читать") || CompareParam(buffer, "read"))
			&& buffer2.empty()))
	{
		// при пустой команде или 'читать' без цифры - показываем первое
		// непрочитанное сообщение, если такое есть
		if (!can_read(ch, board))
		{
			message_no_read(ch, board);
			return;
		}
		time_t const date = ch->get_board_date(board.type_);
		// новости мада в ленточном варианте
		if ((board.type_ == NEWS_BOARD
			|| board.type_ == GODNEWS_BOARD
			|| board.type_ == CODER_BOARD)
				&& !PRF_FLAGGED(ch, PRF_NEWS_MODE))
		{
			std::ostringstream body;
			char timeBuf[17];
			MessageListType::reverse_iterator message;
			for (message = board.messages.rbegin(); message != board.messages.rend(); ++message)
			{
				strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&(*message)->date));
				if ((*message)->date > date)
				{
					body << CCWHT(ch, C_NRM); // новые подсветим
				}
				body << timeBuf << CCNRM(ch, C_NRM);
				if (board.type_ == CODER_BOARD)
				{
					body << " " << (*message)->author << "\r\n";
					std::string text((*message)->text);
					body << format_news_message(text);
				}
				else
				{
					body << "\r\n" << (*message)->text;
				}
			}
			set_last_read(ch, board.type_, board.last_message_date());
			page_string(ch->desc, body.str());
			return;
		}
		// дрновости в ленточном варианте
		if (board.type_ == CLANNEWS_BOARD && !PRF_FLAGGED(ch, PRF_NEWS_MODE))
		{
			std::ostringstream body;
			MessageListType::reverse_iterator message;
			for (message = board.messages.rbegin(); message != board.messages.rend(); ++message)
			{
				if ((*message)->date > date)
					body << CCWHT(ch, C_NRM); // новые подсветим
				body << "[" << (*message)->author << "] " << CCNRM(ch, C_NRM) << (*message)->text;
			}
			set_last_read(ch, board.type_, board.last_message_date());
			page_string(ch->desc, body.str());
			return;
		}

		for (MessageListType::const_iterator message = board.messages.begin(); message != board.messages.end(); ++message)
		{
			if ((*message)->date > date)
			{
				Board::ShowMessage(ch, *message);
				set_last_read(ch, board.type_, (*message)->date);
				return;
			}
		}
		send_to_char("У вас нет непрочитанных сообщений.\r\n", ch);
	}
	else if (is_number(buffer.c_str())
		|| ((CompareParam(buffer, "читать") || CompareParam(buffer, "read"))
			&& is_number(buffer2.c_str())))
	{
		// если после команды стоит цифра или 'читать цифра' - пытаемся
		// показать эту мессагу, два сравнения - чтобы не загребать 'писать число...' как чтение
		unsigned num = 0;
		if (!can_read(ch, board))
		{
			message_no_read(ch, board);
			return;
		}
		if (CompareParam(buffer, "читать"))
			num = atoi(buffer2.c_str());
		else
			num = atoi(buffer.c_str());
		if (num <= 0 || num > board.messages.size())
		{
			send_to_char("Это сообщение может вам только присниться.\r\n", ch);
			return;
		}
		num = board.messages.size() - num;
		Board::ShowMessage(ch, board.messages[num]);
		set_last_read(ch, board.type_, board.messages[num]->date);
	}
	else if (CompareParam(buffer, "писать") || CompareParam(buffer, "write"))
	{
		if (!can_write(ch, board))
		{
			message_no_write(ch);
			return;
		}
		if (is_spamer(ch, board))
		{
			send_to_char("Да вы ведь только что писали сюда.\r\n", ch);
			return;
		}
		if (board.is_special() && board.messages.size() >= MAX_REPORT_MESSAGES)
		{
			send_to_char(OVERFLOW_MESSAGE, ch);
			return;
		}
		/// написание новостей от другого имени
		std::string name = GET_NAME(ch);
		if ((board.type_ == NEWS_BOARD || board.type_ == NOTICE_BOARD) && PRF_FLAGGED(ch, PRF_CODERINFO))
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
		MessagePtr tempMessage(new Message);
		tempMessage->author = name;
		tempMessage->unique = GET_UNIQUE(ch);
		// для досок кроме клановых и персональных пишем левел автора (для возможной очистки кем-то)
		PRF_FLAGGED(ch, PRF_CODERINFO) ? tempMessage->level = LVL_IMPL : tempMessage->level = GET_LEVEL(ch);
		// клановым еще ранг
		if (CLAN(ch))
			tempMessage->rank = CLAN_MEMBER(ch)->rank_num;
		else
			tempMessage->rank = 0;
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
		ch->desc->board = *board_it;

		char **text;
		CREATE(text, char *, 1);
		send_to_char(ch, "Можете писать сообщение.  (/s записать /h помощь)\r\n");
		STATE(ch->desc) = CON_WRITEBOARD;
		string_write(ch->desc, text, MAX_MESSAGE_LENGTH, 0, NULL);
	}
	else if (CompareParam(buffer, "очистить") || CompareParam(buffer, "remove"))
	{
		if (!is_number(buffer2.c_str()))
		{
			send_to_char("Укажите корректный номер сообщения.\r\n", ch);
			return;
		}
		unsigned num = atoi(buffer2.c_str());
		if (num <= 0 || num > board.messages.size())
		{
			send_to_char("Это сообщение может вам только присниться.\r\n", ch);
			return;
		}
		num = board.messages.size() - num;
		set_last_read(ch, board.type_, board.messages[num]->date);
		// или он может делетить любые мессаги (по левелу/рангу), или только свои
		if (!full_access(ch, board))
		{
			if (board.messages[num]->unique != GET_UNIQUE(ch))
			{
				send_to_char("У вас нет возможности удалить это сообщение.\r\n", ch);
				return;
			}
		}
		else if (board.type_ != CLAN_BOARD
				 && board.type_ != CLANNEWS_BOARD
				 && board.type_ != PERS_BOARD
				 && !PRF_FLAGGED(ch, PRF_CODERINFO)
				 && GET_LEVEL(ch) < board.messages[num]->level)
		{
			// для простых досок сверяем левела (для контроля иммов)
			// клановые ниже, у персональных смысла нет
			send_to_char("У вас нет возможности удалить это сообщение.\r\n", ch);
			return;
		}
		else if (board.type_ == CLAN_BOARD || board.type_ == CLANNEWS_BOARD)
		{
			// у кого привилегия на новости, те могут удалять везде чужие, если ранк автора такой же или ниже
			if (CLAN_MEMBER(ch)->rank_num > board.messages[num]->rank)
			{
				send_to_char("У вас нет возможности удалить это сообщение.\r\n", ch);
				return;
			}
		}
		// собственно делетим и проставляем сдвиг номеров
		board.messages.erase(board.messages.begin() + num);
		int count = 0;
		for (auto it = board.messages.rbegin(); it != board.messages.rend(); ++it)
		{
			(*it)->num = count++;
		}
		if (board.get_lastwrite() == GET_UNIQUE(ch))
		{
			board.set_lastwrite(0);
		}
		board.Save();
		send_to_char("Сообщение удалено.\r\n", ch);
	}
	else
	{
		send_to_char("Неверный формат команды, ознакомьтесь со 'справка доски'.\r\n", ch);
	}
}

int count_unread(const MessageListType &messages, time_t last_read)
{
	int unread = 0;
	for (auto i = messages.rbegin(); i != messages.rend(); ++i)
	{
		if ((*i)->date > last_read)
		{
			++unread;
		}
		else
		{
			break;
		}
	}
	return unread;
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

std::string Board::print_stats(CHAR_DATA *ch, int num)
{
	const std::string access = print_access(get_access(ch));
	if (access.empty()) return "";

	std::string out;
	if (IS_IMMORTAL(ch) || PRF_FLAGGED(ch, PRF_CODERINFO) || !blind_)
	{
		const int unread = count_unread(
			messages, ch->get_board_date(type_));
		out += boost::str(boost::format
			(" %2d)  %10s   [%3d|%3d]   %40s  %6s\r\n")
			% num % name_ % unread % messages.size()
			% desc_ % access);
	}
	else
	{
		std::string tmp = alias_.empty() ? desc_
			: desc_ + " (" + alias_ + ")";
		out += boost::str(boost::format
			(" %2d)  %10s   [ - | - ]   %40s  %6s\r\n")
			% num % name_ % tmp % access);
	}
	return out;
}

ACMD(DoBoardList)
{
	if (IS_NPC(ch))
		return;

	std::string out(
		" Ном         Имя  Новых|Всего                                  Описание  Доступ\r\n"
		" ===  ==========  ===========  ========================================  ======\r\n");
	int num = 1;
	for (auto board = board_list.cbegin(); board != board_list.cend(); ++board)
	{
		if (!(*board)->blind())
		{
			out += (*board)->print_stats(ch, num++);
		}
	}
	// два цикла для сквозной нумерации без заморочек
	out +=
		" ---  ----------  -----------  ----------------------------------------  ------\r\n";
	for (auto board = board_list.cbegin(); board != board_list.cend(); ++board)
	{
		if ((*board)->blind())
		{
			out += (*board)->print_stats(ch, num++);
		}
	}

	send_to_char(out, ch);
}

std::bitset<ACCESS_NUM> Board::get_access(CHAR_DATA *ch) const
{
	std::bitset<ACCESS_NUM> access;

	switch (this->type_)
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
		if (IS_GOD(ch) && this->pers_unique_ == GET_UNIQUE(ch)
			&& CompareParam(this->pers_name_, GET_NAME(ch), 1))
		{
			access.set();
		}
		break;
	case CLAN_BOARD:
		// от клан-новостей отличается тем, что писать могут все звания
		if (CLAN(ch) && CLAN(ch)->GetRent() == this->clan_rent_)
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
		if (CLAN(ch) && CLAN(ch)->GetRent() == this->clan_rent_)
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
		&& (this->type_ != CLAN_BOARD && this->type_ != CLANNEWS_BOARD))
	{
		access.reset(ACCESS_CAN_WRITE);
		access.reset(ACCESS_FULL);
	}

	return access;
}

bool act_board(CHAR_DATA *ch, int vnum, char *buf_)
{
	switch(vnum)
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

// чтобы не травмировать народ спешиалы вешаем на старые доски с новым содержимым
SPECIAL(Board::Special)
{
	OBJ_DATA *board = (OBJ_DATA *) me;
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
	if ((CMD_IS("читать") || CMD_IS("read")) && !buffer2.empty() && isdigit(buffer2[0]))
	{
		if (buffer2.find(".") != std::string::npos)
		{
			return 0;
		}
	}

	// вывод сообщения написать сообщение очистить сообщение
	if (((CMD_IS("читать") || CMD_IS("read")) && !buffer2.empty() && isdigit(buffer2[0]))
			|| CMD_IS("писать") || CMD_IS("write") || CMD_IS("очистить") || CMD_IS("remove"))
	{
		// перехватываем запарки вида 'писать дрв' сидя на ренте с доской от вече
		if ((CMD_IS("писать") || CMD_IS("write")) && !buffer2.empty())
		{
			for (auto i = board_list.begin(); i != board_list.end(); ++i)
			{
				if (isname(buffer2, (*i)->get_name().c_str()))
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
void Board::LoginInfo(CHAR_DATA * ch)
{
	std::ostringstream buffer, news;
	bool has_message = 0;
	buffer << "\r\nВас ожидают сообщения:\r\n";

	for (auto board = board_list.begin(); board != board_list.end(); ++board)
	{
		// доска не видна или можно только писать, опечатки тож не спамим
		if (!can_read(ch, **board)
			|| ((*board)->type_ == MISPRINT_BOARD && !PRF_FLAGGED(ch, PRF_MISPRINT))
			|| (*board)->type_ == CODER_BOARD)
		{
			continue;
		}
		const int unread = count_unread((*board)->messages, ch->get_board_date((*board)->type_));
		if (unread > 0)
		{
			has_message = 1;
			if ((*board)->type_ == NEWS_BOARD || (*board)->type_ == GODNEWS_BOARD || (*board)->type_ == CLANNEWS_BOARD)
				news << std::setw(4) << unread << " в разделе '" << (*board)->desc_ << "' " << CCWHT(ch, C_NRM) << "(" << (*board)->name_ << ")" << CCNRM(ch, C_NRM) << ".\r\n";
			else
				buffer << std::setw(4) << unread << " в разделе '" << (*board)->desc_ << "' " << CCWHT(ch, C_NRM) << "(" << (*board)->name_ << ")" << CCNRM(ch, C_NRM) << ".\r\n";
		}
	}

	if (has_message)
	{
		buffer << news.str();
		send_to_char(buffer.str(), ch);
	}
}

ACMD(report_on_board)
{
	if (IS_NPC(ch)) return;
	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
	{
		send_to_char("Пустое сообщение пропущено.\r\n", ch);
		return;
	}

	auto board = std::find_if(
		board_list.begin(), board_list.end(),
		boost::bind(&Board::type_, _1) == subcmd);

	if (board == board_list.end())
	{
		send_to_char("Доска тупо не найдена... :/\r\n", ch);
		return;
	}
	if (!can_write(ch, **board))
	{
		message_no_write(ch);
		return;
	}
	if ((*board)->is_special()
		&& (*board)->messages.size() >= MAX_REPORT_MESSAGES)
	{
		send_to_char(OVERFLOW_MESSAGE, ch);
		return;
	}
	// генерим мессагу (TODO: копипаст с написания на доску, надо бы вынести)
	MessagePtr temp_message(new Message);
	temp_message->author = GET_NAME(ch) ? GET_NAME(ch) : "неизвестен";
	temp_message->unique = GET_UNIQUE(ch);
	// для досок кроме клановых и персональных пишет левел автора (для возможной очистки кем-то)
	temp_message->level = GET_LEVEL(ch);
	temp_message->rank = 0;
	temp_message->subject = "[" + boost::lexical_cast<std::string>(GET_ROOM_VNUM(ch->in_room)) + "]";
	temp_message->text = argument;
	temp_message->date = time(0);

	(*board)->add_message(temp_message);
	send_to_char(ch,
		"Текст сообщения:\r\n"
		"%s\r\n\r\n"
		"Записали. Заранее благодарны.\r\n"
		"                        Боги.\r\n", argument);
}

BoardTypes Board::get_type() const
{
	return type_;
}

long Board::get_lastwrite() const
{
	return last_write_;
}

void Board::set_lastwrite(long unique)
{
	last_write_ = unique;
}

const std::string & Board::get_name() const
{
	return name_;
}

bool Board::is_special() const
{
	if (get_type() == ERROR_BOARD
		|| get_type() == MISPRINT_BOARD
		|| get_type() == SUGGEST_BOARD)
	{
		return true;
	}
	return false;
}

void Board::new_message_notify() const
{
	if (get_type() != PERS_BOARD
		&& get_type() != CODER_BOARD
		&& !messages.empty())
	{
		const Message &msg = **(messages.rbegin());
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_),
			"Новое сообщение в разделе '%s' от %s, тема: %s\r\n",
			get_name().c_str(), msg.author.c_str(),
			msg.subject.c_str());
		// оповещаем весь мад кто с правами чтения
		for (DESCRIPTOR_DATA *f = descriptor_list; f; f = f->next)
		{
			if (f->character
				&& STATE(f) == CON_PLAYING
				&& PRF_FLAGGED(f->character, PRF_BOARD_MODE)
				&& can_read(f->character, *this)
				&& (get_type() != MISPRINT_BOARD
					|| PRF_FLAGGED(f->character, PRF_MISPRINT)))
			{
				send_to_char(buf_, f->character);
			}
		}
	}
}

void Board::add_message(MessagePtr msg)
{
	if ((get_type() == CODER_BOARD)
		&& messages.size() >= MAX_REPORT_MESSAGES)
	{
		messages.erase(messages.begin());
	}
	if (!is_special()
		&& get_type() != NEWS_BOARD
		&& get_type() != GODNEWS_BOARD
		&& get_type() != CODER_BOARD
		&& messages.size() >= MAX_BOARD_MESSAGES)
	{
		messages.erase(messages.begin());
	}

	// чтобы время написания разделялось
	if (last_message_date() >= msg->date)
	{
		msg->date = last_message_date() + 1;
	}

	messages.push_back(msg);
	int count = 0;
	for (auto it = messages.rbegin(); it != messages.rend(); ++it)
	{
		(*it)->num = count++;
	}
	set_lastwrite(msg->unique);
	Save();
	new_message_notify();
}

time_t Board::last_message_date() const
{
	if (!messages.empty())
	{
		return (*messages.rbegin())->date;
	}
	return 0;
}

const std::string & Board::get_alias() const
{
	return alias_;
}

bool Board::blind() const
{
	return blind_;
}

namespace Boards
{

bool can_see(CHAR_DATA *ch, const Board &board)
{
	auto access_ = board.get_access(ch);
	return access_.test(ACCESS_CAN_SEE);
}

bool can_read(CHAR_DATA *ch, const Board &board)
{
	auto access_ = board.get_access(ch);
	return access_.test(ACCESS_CAN_READ);
}

bool can_write(CHAR_DATA *ch, const Board &board)
{
	auto access_ = board.get_access(ch);
	return access_.test(ACCESS_CAN_WRITE);
}

bool full_access(CHAR_DATA *ch, const Board &board)
{
	auto access_ = board.get_access(ch);
	return access_.test(ACCESS_FULL);
}

void clan_delete_message(const std::string &name, int vnum)
{
	const std::string subj = "неактивная дружина";
	const std::string text = boost::str(boost::format(
		"Дружина %1% была автоматически удалена.\r\n"
		"Номер зоны: %2%\r\n") % name % vnum);
	add_server_message(subj, text);
}

std::string& format_news_message(std::string &text)
{
	StringReplace(text, '\n', "\n   ");
	boost::trim(text);
	text.insert(0, "   ");
	text += '\n';
	return text;
}

} // namespace Boards

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
