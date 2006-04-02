/* ****************************************************************************
* File: boards.h                                               Part of Bylins *
* Usage: Header file for bulletin boards                                      *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"

#define MAX_MESSAGE_LENGTH 4096 // максимальный размер сообщения
#define MAX_BOARD_MESSAGES 200  // максимальное кол-во сообщений на одной доске
#define MIN_WRITE_LEVEL    6    // мин.левел для поста на общих досках
// типы досок
#define GENERAL_BOARD    0  // общая
#define IDEA_BOARD       1  // идеи
#define ERROR_BOARD      2  // ошибки
#define NEWS_BOARD       3  // новости
#define GODNEWS_BOARD    4  // новости (только для иммов)
#define GODGENERAL_BOARD 5  // общая (только для иммов)
#define GODBUILD_BOARD   6  // билдеры (только для иммов)
#define GODCODE_BOARD    7  // кодеры (только для иммов)
#define GODPUNISH_BOARD  8  // наказания (только для иммов)
#define PERS_BOARD       9  // персональная (только для иммов)
#define CLAN_BOARD       10 // клановая
#define CLANNEWS_BOARD   11 // клановые новости
// предметы, на которые вешаем спешиалы для старого способа работы с досками
#define GODGENERAL_BOARD_OBJ 250
#define GENERAL_BOARD_OBJ    251
#define GODCODE_BOARD_OBJ    253
#define GODPUNISH_BOARD_OBJ  257
#define GODBUILD_BOARD_OBJ   259

typedef boost::shared_ptr<Board> BoardPtr;
typedef std::vector<BoardPtr> BoardListType;
typedef boost::shared_ptr<struct Message> MessagePtr;
typedef std::vector<MessagePtr> MessageListType;

// отдельное сообщение
struct Message {
	int num;             // номер на доске
	std::string author;  // имя автора
	long unique;         // уид автора
	int level;           // уровень автора (для всех, кроме клановых и персональных досок)
	int rank;            // для дрн и дрвече
	time_t date;         // дата поста
	std::string subject; // тема
	std::string text;    // текст сообщения
};

// доски... наследование нафик
class Board {
public:
	static BoardListType BoardList; // список досок
	MessageListType messages; // список сообщений

	Board();
	int Access(CHAR_DATA * ch);
	int GetType() { return this->type; };
	int GetClanRent() { return this->clanRent; };
	std::string & GetName() { return this->name; };
	void SetLastRead(long unique) { this->lastWrite = unique; };
	void Save();
	static void BoardInit();
	static void ClanInit();
	static void GodInit();
	static SPECIAL(Board::Special);
	static void LoginInfo(CHAR_DATA * ch);

	friend ACMD(DoBoard);
	friend ACMD(DoBoardList);

	private:
	int type;                 // тип доски
	std::string name;         // имя доски
	std::string desc;         // описание доски
	long lastWrite;           // уид последнего писавшего (до ребута)
	int clanRent;             // номер ренты клана (для клановых досок)
	int persUnique;           // уид (для персональной доски)
	std::string persName;     // имя (для персональной доски)
	std::string file;         // имя файла для сейва/лоада

	void Load();
	void SetLastReadDate(CHAR_DATA * ch, long date);
	long LastReadDate(CHAR_DATA * ch);
	static void ShowMessage(CHAR_DATA * ch, MessagePtr message);
};

#endif
