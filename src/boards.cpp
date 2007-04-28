/* ****************************************************************************
* File: boards.cpp                                             Part of Bylins *
* Usage: Handling of multiple bulletin boards                                 *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include "conf.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "house.h"
#include "screen.h"
#include "comm.h"
#include "privilege.hpp"
#include "boards.h"

extern int isname(const char *str, const char *namelist);

BoardListType Board::BoardList;

Board::Board() : type(0), lastWrite(0), clanRent(0), persUnique(0)
{
	// тут особо ничего и не надо
}

// создание всех досок, кроме клановых и персональных
void Board::BoardInit()
{
	Board::BoardList.clear();

	// общая доска
	BoardPtr GeneralBoard(new Board);
	GeneralBoard->type = GENERAL_BOARD;
	GeneralBoard->name = "Вече";
	GeneralBoard->desc = "Базарная площадь";
	GeneralBoard->file = ETC_BOARD"general.board";
	GeneralBoard->Load();
	Board::BoardList.push_back(GeneralBoard);

	// идеи
	BoardPtr IdeaBoard(new Board);
	IdeaBoard->type = IDEA_BOARD;
	IdeaBoard->name = "Идеи";
	IdeaBoard->desc = "Идеи и их обсуждение";
	IdeaBoard->file = ETC_BOARD"idea.board";
	IdeaBoard->Load();
	Board::BoardList.push_back(IdeaBoard);

	// ошибки
	BoardPtr ErrorBoard(new Board);
	ErrorBoard->type = ERROR_BOARD;
	ErrorBoard->name = "Ошибки";
	ErrorBoard->desc = "Сообщения об ошибках в мире";
	ErrorBoard->file = ETC_BOARD"error.board";
	ErrorBoard->Load();
	Board::BoardList.push_back(ErrorBoard);

	// новости
	BoardPtr NewsBoard(new Board);
	NewsBoard->type = NEWS_BOARD;
	NewsBoard->name = "Новости";
	NewsBoard->desc = "Анонсы и новости Былин";
	NewsBoard->file = ETC_BOARD"news.board";
	NewsBoard->Load();
	Board::BoardList.push_back(NewsBoard);

	// новости БОГов
	BoardPtr GodNewsBoard(new Board);
	GodNewsBoard->type = GODNEWS_BOARD;
	GodNewsBoard->name = "GodNews";
	GodNewsBoard->desc = "Божественные новости";
	GodNewsBoard->file = ETC_BOARD"god-news.board";
	GodNewsBoard->Load();
	Board::BoardList.push_back(GodNewsBoard);

	// общая для БОГов
	BoardPtr GodGeneralBoard(new Board);
	GodGeneralBoard->type = GODGENERAL_BOARD;
	GodGeneralBoard->name = "Божества";
	GodGeneralBoard->desc = "Божественная базарная площадь";
	GodGeneralBoard->file = ETC_BOARD"god-general.board";
	GodGeneralBoard->Load();
	Board::BoardList.push_back(GodGeneralBoard);

	// для билдеров
	BoardPtr GodBuildBoard(new Board);
	GodBuildBoard->type = GODBUILD_BOARD;
	GodBuildBoard->name = "Билдер";
	GodBuildBoard->desc = "Заметки билдеров";
	GodBuildBoard->file = ETC_BOARD"god-build.board";
	GodBuildBoard->Load();
	Board::BoardList.push_back(GodBuildBoard);

	// для кодеров
	BoardPtr GodCodeBoard(new Board);
	GodCodeBoard->type = GODCODE_BOARD;
	GodCodeBoard->name = "Кодер";
	GodCodeBoard->desc = "Заметки кодеров";
	GodCodeBoard->file = ETC_BOARD"god-code.board";
	GodCodeBoard->Load();
	Board::BoardList.push_back(GodCodeBoard);

	// для комментариев наказаний
	BoardPtr GodPunishBoard(new Board);
	GodPunishBoard->type = GODPUNISH_BOARD;
	GodPunishBoard->name = "Наказания";
	GodPunishBoard->desc = "Комментарии к наказаниям";
	GodPunishBoard->file = ETC_BOARD"god-punish.board";
	GodPunishBoard->Load();
	Board::BoardList.push_back(GodPunishBoard);
}

// лоад/релоад клановых досок
void Board::ClanInit()
{
	// чистим для релоада
	for (BoardListType::iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		if ((*board)->type == CLAN_BOARD || (*board)->type == CLANNEWS_BOARD) {
			Board::BoardList.erase(board);
			board = Board::BoardList.begin();
		}
	}

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		std::string name = (*clan)->GetAbbrev();
		CreateFileName(name);
		// делаем клановую доску
		BoardPtr board(new Board);
		board->type = CLAN_BOARD;
		board->name = "ДрВече";
		board->desc = "Основной раздел Дружины ";
		board->desc += (*clan)->GetAbbrev();
		board->clanRent = (*clan)->GetRent();
		board->file = LIB_HOUSE + name + "/" + name + ".board";
		board->Load();
		Board::BoardList.push_back(board);
		// делаем клановые новости
		BoardPtr newsboard(new Board);
		newsboard->type = CLANNEWS_BOARD;
		newsboard->name = "ДрНовости";
		newsboard->desc = "Новости Дружины ";
		newsboard->desc += (*clan)->GetAbbrev();
		newsboard->clanRent = (*clan)->GetRent();
		newsboard->file = LIB_HOUSE + name + "/" + name + "-news.board";
		newsboard->Load();
		Board::BoardList.push_back(newsboard);
	}
}

// чистим для релоада
void Board::clear_god_boards()
{
	for (BoardListType::iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		if ((*board)->type == PERS_BOARD) {
			Board::BoardList.erase(board);
			board = Board::BoardList.begin();
		}
	}
}

// втыкаем блокнот имму
void Board::init_god_board(long uid, std::string name)
{
	BoardPtr board(new Board);
	board->type = PERS_BOARD;
	board->name = "Блокнот";
	board->desc = "Ваш раздел для заметок";
	board->persUnique = uid;
	board->persName = name;
	// генерим имя и лоадим по возможности
	std::string tmp_name = name;
	CreateFileName(tmp_name);
	board->file = ETC_BOARD + tmp_name + ".board";
	board->Load();
	Board::BoardList.push_back(board);
}

/**
* Релоад всех досок разом.
*/
void Board::reload_all()
{
	BoardInit();
	Privilege::load_god_boards();
	ClanInit();
}

// подгружаем доску, если она существует
void Board::Load()
{
	std::ifstream file((this->file).c_str());
	if (!file.is_open()) {
		this->Save();
		return;
	}

	std::string buffer;
	while (file >> buffer) {
		if (buffer == "Message:") {
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
			this->messages.push_back(message);
		} else if (buffer == "Type:")
			file >> this->type;
		else if (buffer == "Clan:")
			file >> this->clanRent;
		else if (buffer == "PersUID:")
			file >> this->persUnique;
		else if (buffer == "PersName:")
			file >> this->persName;
	}
	file.close();

	this->Save();
}

// сохраняем доску
void Board::Save()
{
	std::ofstream file((this->file).c_str());
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", (this->file).c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	file << "Type: " << this->type << " "
		<< "Clan: " << this->clanRent << " "
		<< "PersUID: " << this->persUnique << " "
		<< "PersName: " << (this->persName.empty() ? "none" : this->persName) << "\n";
	for (MessageListType::const_iterator message = this->messages.begin(); message != this->messages.end(); ++message) {
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

	page_string(ch->desc, buffer.str(), 1);
}

// вынимаем дату последнего чтения конкретной доски у персонажа
long Board::LastReadDate(CHAR_DATA * ch)
{
	if (IS_NPC(ch))
		return 0;

	switch (this->type) {
	case GENERAL_BOARD:
		return GENERAL_BOARD_DATE(ch);
	case IDEA_BOARD:
		return IDEA_BOARD_DATE(ch);
	case ERROR_BOARD:
		return ERROR_BOARD_DATE(ch);
	case NEWS_BOARD:
		return NEWS_BOARD_DATE(ch);
	case GODNEWS_BOARD:
		return GODNEWS_BOARD_DATE(ch);
	case GODGENERAL_BOARD:
		return GODGENERAL_BOARD_DATE(ch);
	case GODBUILD_BOARD:
		return GODBUILD_BOARD_DATE(ch);
	case GODCODE_BOARD:
		return GODCODE_BOARD_DATE(ch);
	case GODPUNISH_BOARD:
		return GODPUNISH_BOARD_DATE(ch);
	case PERS_BOARD:
		return PERS_BOARD_DATE(ch);
	case CLAN_BOARD:
		return CLAN_BOARD_DATE(ch);
	case CLANNEWS_BOARD:
		return CLANNEWS_BOARD_DATE(ch);
	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	return 0;
}

// проставляет последнюю дату чтения конкретной доски
void Board::SetLastReadDate(CHAR_DATA * ch, long date)
{
	if (IS_NPC(ch))
		return;

	switch (this->type) {
	case GENERAL_BOARD:
		if (GENERAL_BOARD_DATE(ch) < date)
			GENERAL_BOARD_DATE(ch) = date;
		return;
	case IDEA_BOARD:
		if (IDEA_BOARD_DATE(ch) < date)
			IDEA_BOARD_DATE(ch) = date;
		return;
	case ERROR_BOARD:
		if (ERROR_BOARD_DATE(ch) < date)
			ERROR_BOARD_DATE(ch) = date;
		return;
	case NEWS_BOARD:
		if (NEWS_BOARD_DATE(ch) < date)
			NEWS_BOARD_DATE(ch) = date;
		return;
	case GODNEWS_BOARD:
		if (GODNEWS_BOARD_DATE(ch) < date)
			GODNEWS_BOARD_DATE(ch) = date;
		return;
	case GODGENERAL_BOARD:
		if (GODGENERAL_BOARD_DATE(ch) < date)
			GODGENERAL_BOARD_DATE(ch) = date;
		return;
	case GODBUILD_BOARD:
		if (GODBUILD_BOARD_DATE(ch) < date)
			GODBUILD_BOARD_DATE(ch) = date;
		return;
	case GODCODE_BOARD:
		if (GODCODE_BOARD_DATE(ch) < date)
			GODCODE_BOARD_DATE(ch) = date;
		return;
	case GODPUNISH_BOARD:
		if (GODPUNISH_BOARD_DATE(ch) < date)
			GODPUNISH_BOARD_DATE(ch) = date;
		return;
	case PERS_BOARD:
		if (PERS_BOARD_DATE(ch) < date)
			PERS_BOARD_DATE(ch) = date;
		return;
	case CLAN_BOARD:
		if (CLAN_BOARD_DATE(ch) < date)
			CLAN_BOARD_DATE(ch) = date;
		return;
	case CLANNEWS_BOARD:
		if (CLANNEWS_BOARD_DATE(ch) < date)
			CLANNEWS_BOARD_DATE(ch) = date;
		return;
	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
}

ACMD(DoBoard)
{
	if (!ch->desc)
		return;
	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("Вы ослеплены!\r\n", ch);
		return;
	}

	BoardListType::const_iterator board;
	// ну и в остальных случаях доски одни на всех и последовательно лежат, но найдем для верности
	for (board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board)
		if ((*board)->type == subcmd && (*board)->Access(ch))
			break;
	if (board == Board::BoardList.end()) {
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	std::string buffer, buffer2 = argument;
	long date = (*board)->LastReadDate(ch);
	GetOneParam(buffer2, buffer);
	boost::trim_if(buffer2, boost::is_any_of(" \'"));
	// первый аргумент в buffer, второй в buffer2

	if (CompareParam(buffer, "список") || CompareParam(buffer, "list")) {
		if ((*board)->Access(ch) == 2) {
			send_to_char("У Вас нет возможности читать этот раздел.\r\n", ch);
			return;
		}
		std::ostringstream body;
		body << "Это доска, на которой всякие разные личности оставили свои IMHO.\r\n"
			<< "Формат: ЧИТАТЬ/ОЧИСТИТЬ <номер сообщения>, ПИСАТЬ <тема сообщения>.\r\n";
		if ((*board)->messages.empty()) {
			body << "Никто ниче не накарябал, слава Богам.\r\n";
			send_to_char(body.str(), ch);
			return;
		} else
			body << "Всего сообщений: " << (*board)->messages.size() << "\r\n";
		char timeBuf[17];
		for (MessageListType::reverse_iterator message = (*board)->messages.rbegin(); message != (*board)->messages.rend(); ++message) {
			if ((*message)->date > date)
				body << CCWHT(ch, C_NRM); // для выделения новых мессаг светло-белым

			strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&(*message)->date));
			body << "[" << (*message)->num + 1 << "] "<< timeBuf << " "
				<< "(" << (*message)->author << ") :: "
				<< (*message)->subject << "\r\n" <<  CCNRM(ch, C_NRM);
		}
		page_string(ch->desc, body.str(), 1);
		return;
	}

	// при пустой команде или 'читать' без цифры - показываем первое непрочитанное сообщение, если такое есть
	if (buffer.empty() || ((CompareParam(buffer, "читать") || CompareParam(buffer, "read")) && buffer2.empty())) {
		if ((*board)->Access(ch) == 2) {
			send_to_char("У Вас нет возможности читать этот раздел.\r\n", ch);
			return;
		}
		// новости мада в ленточном варианте
		if (((*board)->type == NEWS_BOARD || (*board)->type == GODNEWS_BOARD) && !PRF_FLAGGED(ch, PRF_NEWS_MODE)) {
			std::ostringstream body;
			long date = (*board)->LastReadDate(ch);
			char timeBuf[17];

			MessageListType::reverse_iterator message;
			for (message = (*board)->messages.rbegin(); message != (*board)->messages.rend(); ++message) {
				strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&(*message)->date));
				if ((*message)->date > date)
					body << CCWHT(ch, C_NRM); // новые подсветим
				body << timeBuf << CCNRM(ch, C_NRM) << "\r\n"
						<< (*message)->text;
			}
			(*board)->SetLastReadDate(ch, time(0));
			page_string(ch->desc, body.str(), 1);
			return;
		}
		// дрновости в ленточном варианте
		if ((*board)->type == CLANNEWS_BOARD && !PRF_FLAGGED(ch, PRF_NEWS_MODE)) {
			std::ostringstream body;
			long date = (*board)->LastReadDate(ch);

			MessageListType::reverse_iterator message;
			for (message = (*board)->messages.rbegin(); message != (*board)->messages.rend(); ++message) {
				if ((*message)->date > date)
					body << CCWHT(ch, C_NRM); // новые подсветим
				body << "[" << (*message)->author << "] " << CCNRM(ch, C_NRM) << (*message)->text;
			}
			(*board)->SetLastReadDate(ch, time(0));
			page_string(ch->desc, body.str(), 1);
			return;
		}

		for (MessageListType::const_iterator message = (*board)->messages.begin(); message != (*board)->messages.end(); ++message) {
			if ((*message)->date > date) {
				Board::ShowMessage(ch, *message);
				(*board)->SetLastReadDate(ch, (*message)->date);
				return;
			}
		}
		send_to_char("У Вас нет непрочитанных сообщений.\r\n", ch);
		return;
	}

	unsigned num = 0;
	// если после команды стоит цифра или 'читать цифра' - пытаемся показать эту мессагу, два сравнения - чтобы не загребать 'писать число...' как чтение
	if (is_number(buffer.c_str()) || ((CompareParam(buffer, "читать") || CompareParam(buffer, "read")) && is_number(buffer2.c_str()))) {
		if ((*board)->Access(ch) == 2) {
			send_to_char("У Вас нет возможности читать этот раздел.\r\n", ch);
			return;
		}
		if (CompareParam(buffer, "читать"))
			num = atoi(buffer2.c_str());
		else
			num = atoi(buffer.c_str());
		if (num <= 0 || num > (*board)->messages.size()) {
			send_to_char("Это сообщение может Вам только присниться.\r\n", ch);
			return;
		}
		num = (*board)->messages.size() - num;
		Board::ShowMessage(ch, (*board)->messages[num]);
		(*board)->SetLastReadDate(ch, (*board)->messages[num]->date);
		return;
	}

	if (CompareParam(buffer, "писать") || CompareParam(buffer, "write")) {
		if ((*board)->Access(ch) < 2 || PLR_FLAGGED(ch, PLR_HELLED) || PLR_FLAGGED(ch, PLR_NAMED)) {
			send_to_char("У Вас нет возможности писать в этот раздел.\r\n", ch);
			return;
		}
		if (!IS_GOD(ch) && (*board)->lastWrite == GET_UNIQUE(ch)
			&& (*board)->type != CLAN_BOARD
			&& (*board)->type != CLANNEWS_BOARD
			&& (*board)->type != PERS_BOARD
			&& !Privilege::check_flag(ch, Privilege::NEWS_MAKER))
		{
			send_to_char("Да Вы ведь только что писали сюда.\r\n", ch);
			return;
		}

		// написание новостей от другого имени
		std::string name = GET_NAME(ch);
		if ((*board)->type == NEWS_BOARD && Privilege::check_flag(ch, Privilege::NEWS_MAKER)) {
			GetOneParam(buffer2, buffer);
			if (buffer.empty()) {
				send_to_char("Впишите хотя бы имя, от кого будет опубликовано сообщение.\r\n", ch);
				return;
			}
			name = buffer;
		}

		// генерим мессагу
		MessagePtr tempMessage(new Message);
		tempMessage->author = name;
		tempMessage->unique = GET_UNIQUE(ch);
		// для досок кроме клановых и персональных пишет левел автора (для возможной очистки кем-то)
		tempMessage->level = GET_LEVEL(ch);
		// клановым еще ранг
		if (CLAN(ch))
			tempMessage->rank = CLAN_MEMBER(ch)->rank_num;
		else
			tempMessage->rank = 0;
		boost::trim(buffer2);
		// обрежем для удобочитаемости сабж
		if (buffer2.length() > 40) {
			buffer2.erase(40, std::string::npos);
			send_to_char(ch, "Тема сообщения укорочена до '%s'.\r\n", buffer2.c_str());
		}
		if (subcmd == ERROR_BOARD)
			buffer2 += " Room: [" + boost::lexical_cast<std::string>(GET_ROOM_VNUM(ch->in_room)) + "]";
		tempMessage->subject = buffer2;
		// дату номер и текст пишем уже по факту сохранения
		ch->desc->message = tempMessage;
		ch->desc->board = *board;

		char **text;
		CREATE(text, char *, 1);
		send_to_char(ch, "Можете писать сообщение.  (/s записать /h помощь)\r\n", buffer2.c_str());
		STATE(ch->desc) = CON_WRITEBOARD;
		string_write(ch->desc, text, MAX_MESSAGE_LENGTH, 0, NULL);
	}

	if (CompareParam(buffer, "очистить") || CompareParam(buffer, "remove")) {
		if(!is_number(buffer2.c_str())) {
			send_to_char("Укажите корректный номер сообщения.\r\n", ch);
			return;
		}
		num = atoi(buffer2.c_str());
		if (num <= 0 || num > (*board)->messages.size()) {
			send_to_char("Это сообщение может Вам только присниться.\r\n", ch);
			return;
		}
		num = (*board)->messages.size() - num;
		(*board)->SetLastReadDate(ch, (*board)->messages[num]->date);
		// или он может делетить любые мессаги (по левелу/рангу), или только свои
		if ((*board)->Access(ch) < 4) {
			if ((*board)->messages[num]->unique != GET_UNIQUE(ch)) {
				send_to_char("У Вас нет возможности удалить это сообщение.\r\n", ch);
				return;
			}
		} else if ((*board)->type != CLAN_BOARD && (*board)->type != CLANNEWS_BOARD
			&& (*board)->type != PERS_BOARD && GET_LEVEL(ch) < (*board)->messages[num]->level) {
			// для простых досок сверяем левела (для контроля иммов)
			// клановые ниже, у персональных смысла нет
			send_to_char("У Вас нет возможности удалить это сообщение.\r\n", ch);
			return;
		} else if ((*board)->type == CLAN_BOARD || (*board)->type == CLANNEWS_BOARD) {
			// у кого привилегия на новости, те могут удалять везде чужие, если ранк автора такой же или ниже
			if (CLAN_MEMBER(ch)->rank_num > (*board)->messages[num]->rank) {
				send_to_char("У Вас нет возможности удалить это сообщение.\r\n", ch);
				return;
			}
		}
		// собственно делетим и проставляем сдвиг номеров
		(*board)->messages.erase((*board)->messages.begin() + num);
		int count = 0;
		for (MessageListType::reverse_iterator it = (*board)->messages.rbegin(); it != (*board)->messages.rend(); ++it)
			(*it)->num = count++;
		if ((*board)->lastWrite == GET_UNIQUE(ch))
			(*board)->lastWrite = 0;
		(*board)->Save();
		send_to_char("Сообщение удалено.\r\n", ch);
		return;
	}
}

ACMD(DoBoardList)
{
	if (IS_NPC(ch))
		return;

	std::ostringstream buffer;
	buffer << " Ном         Имя  Новых|Всего                        Описание  Доступ\r\n"
		   << " ===  ==========  ===========  ==============================  ======\r\n";
	boost::format boardFormat(" %2d)  %10s  [%3d|%3d]    %30s  %6s\r\n");

	int num = 1;
	std::string access;
	for (BoardListType::const_iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		int unread = 0;
		int cansee = 1;
		switch ((*board)->Access(ch)) {
		case 0:
			cansee = 0;
			break;
		case 1:
			access = "чтение";
			break;
		case 2:
			access = "запись";
			break;
		case 3:
			access = "полный";
			break;
		case 4:
			access = "полный";
			break;
		}
		if (!cansee)
			continue;
		if ((*board)->Access(ch) == 2)
			unread = 0;
		else {
			for (MessageListType::reverse_iterator message = (*board)->messages.rbegin(); message != (*board)->messages.rend(); ++message) {
				if ((*message)->date > (*board)->LastReadDate(ch))
					++unread;
				else
					break;
			}
		}
		buffer << boardFormat % num % (*board)->name % unread % (*board)->messages.size() % (*board)->desc % access;
		++num;
	}
	send_to_char(buffer.str(), ch);
}

// 0 - нет доступа, 1 - чтение, 2 - запись, 3 - запись/чтение, 4 - полный(с удалением чужих)
int Board::Access(CHAR_DATA * ch)
{
	switch (this->type) {
	case GENERAL_BOARD:
	case IDEA_BOARD:
	// все читают, пишут с мин.левела, 32 полный
		if (IS_GOD(ch))
			return 4;
		if (GET_LEVEL(ch) < MIN_WRITE_LEVEL && !GET_REMORT(ch))
			return 1;
		else
			return 3;
	case ERROR_BOARD:
	// все пишут с мин.левела, 34 полный
		if (IS_IMPL(ch))
			return 4;
		if (GET_LEVEL(ch) < MIN_WRITE_LEVEL && !GET_REMORT(ch))
			return 0;
		else
			return 2;
	case NEWS_BOARD:
	// все читают, 34 и по привилегии полный
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::NEWS_MAKER))
			return 4;
		else
			return 1;
	case GODNEWS_BOARD:
	// 32 читают, 34 полный
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GOD(ch))
			return 1;
		else
			return 0;
	case GODGENERAL_BOARD:
	// 32 читают/пишут, 34 полный
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GOD(ch))
			return 3;
		else
			return 0;
	case GODBUILD_BOARD:
	case GODCODE_BOARD:
	// 33 читают/пишут, 34 полный
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GRGOD(ch))
			return 3;
		else
			return 0;
	case GODPUNISH_BOARD:
	// 32 читают/пишут, 34 полный
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GOD(ch))
			return 3;
		else
			return 0;
	case PERS_BOARD:
		if (IS_GOD(ch) && this->persUnique == GET_UNIQUE(ch) && CompareParam(this->persName, GET_NAME(ch), 1))
			return 4;
		else
			return 0;
	case CLAN_BOARD:
	// от клан-новостей отличается тем, что писать могут все звания
		if (CLAN(ch) && CLAN(ch)->GetRent() == this->clanRent) {
			// воевода
			if (CLAN(ch)->CheckPrivilege(CLAN_MEMBER(ch)->rank_num, MAY_CLAN_NEWS))
				return 4;
			else
				return 3;
		} else
			return 0;
	case CLANNEWS_BOARD:
	// неклановые не видят, клановые могут читать все, писать могут по привилегии, воевода может стирать чужие
		if (CLAN(ch) && CLAN(ch)->GetRent() == this->clanRent) {
			if (CLAN(ch)->CheckPrivilege(CLAN_MEMBER(ch)->rank_num, MAY_CLAN_NEWS))
				return 4;
			return 1;
		} else
			return 0;
	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	return 0;
}

// чтобы не травмировать народ спешиалы вешаем на старые доски с новым содержимым
SPECIAL(Board::Special)
{
	OBJ_DATA *board = (OBJ_DATA *) me;
	if (!ch->desc)
		return 0;
	// список сообщений
	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	if (CMD_IS("смотреть") || CMD_IS("осмотреть") || CMD_IS("look") || CMD_IS("examine")
			|| ((CMD_IS("читать") || CMD_IS("read")) && (CompareParam(buffer2, "доска")) || CompareParam(buffer2, "board"))) {
		// эта мутная запись для всяких 'см на доску' и подобных написаний в два слова
		if (buffer2.empty()
				|| (buffer.empty() && !CompareParam(buffer2, "доска") && !CompareParam(buffer2, "board"))
				|| (!buffer.empty() && !CompareParam(buffer, "доска") && !CompareParam(buffer2, "board")))
			return 0;

		// общая доска
		if (board->item_number == real_object(GENERAL_BOARD_OBJ)) {
			char *temp_arg = str_dup("список");
			DoBoard(ch, temp_arg, 0, GENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		// общая имм-доска
		if (board->item_number == real_object(GODGENERAL_BOARD_OBJ)) {
			char *temp_arg = str_dup("список");
			DoBoard(ch, temp_arg, 0, GODGENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		// доска наказаний
		if (board->item_number == real_object(GODPUNISH_BOARD_OBJ)) {
			char *temp_arg = str_dup("список");
			DoBoard(ch, temp_arg, 0, GODPUNISH_BOARD);
			free(temp_arg);
			return 1;
		}
		// доска билдеров
		if (board->item_number == real_object(GODBUILD_BOARD_OBJ)) {
			char *temp_arg = str_dup("список");
			DoBoard(ch, temp_arg, 0, GODBUILD_BOARD);
			free(temp_arg);
			return 1;
		}
		// доска кодеров
		if (board->item_number == real_object(GODCODE_BOARD_OBJ)) {
			char *temp_arg = str_dup("список");
			DoBoard(ch, temp_arg, 0, GODCODE_BOARD);
			free(temp_arg);
			return 1;
		}
		return 0;
	}

	// для писем
	if ((CMD_IS("читать") || CMD_IS("read")) && !buffer2.empty() && isdigit(buffer2[0]))
		if (buffer2.find(".") != std::string::npos)
			return 0;

	// вывод сообщения написать сообщение очистить сообщение
	if (((CMD_IS("читать") || CMD_IS("read")) && !buffer2.empty() && isdigit(buffer2[0]))
		|| CMD_IS("писать") || CMD_IS("write") || CMD_IS("очистить") || CMD_IS("remove")) {

		// общая доска
		char *temp_arg = str_dup(cmd_info[cmd].command);
		temp_arg = str_add(temp_arg, argument);
		if (board->item_number == real_object(GENERAL_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODGENERAL_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODGENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODPUNISH_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODPUNISH_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODBUILD_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODBUILD_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODCODE_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODCODE_BOARD);
			free(temp_arg);
			return 1;
		}
		free(temp_arg);
	}
	return 0;
}

// выводит при заходе в игру инфу о новых сообщениях на досках
void Board::LoginInfo(CHAR_DATA * ch)
{
	std::ostringstream buffer, news;
	bool has_message = 0;
	buffer << "\r\nВас ожидают сообщения:\r\n";

	for (BoardListType::const_iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		int unread = 0;
		// доска не видна или можно только писать
		if (!(*board)->Access(ch) || (*board)->Access(ch) == 2)
			continue;

		for (MessageListType::reverse_iterator message = (*board)->messages.rbegin(); message != (*board)->messages.rend(); ++message) {
			if ((*message)->date > (*board)->LastReadDate(ch))
				++unread;
			else
				break;
		}

		if (unread) {
			has_message = 1;
			if ((*board)->type == NEWS_BOARD || (*board)->type == GODNEWS_BOARD || (*board)->type == CLANNEWS_BOARD)
				news << std::setw(4) << unread << " в разделе '" << (*board)->desc << "' " << CCWHT(ch, C_NRM) << "(" << (*board)->name << ")" << CCNRM(ch, C_NRM) << ".\r\n";
			else
				buffer << std::setw(4) << unread << " в разделе '" << (*board)->desc << "' " << CCWHT(ch, C_NRM) << "(" << (*board)->name << ")" << CCNRM(ch, C_NRM) << ".\r\n";
		}
	}

	if (has_message) {
		buffer << news.str();
		send_to_char(buffer.str(), ch);
	}
}
