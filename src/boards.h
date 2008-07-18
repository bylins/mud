/* ****************************************************************************
* File: boards.h                                               Part of Bylins *
* Usage: Header file for bulletin boards                                      *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include "conf.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"

extern const int MAX_MESSAGE_LENGTH;
extern const int MIN_WRITE_LEVEL;
extern const unsigned int MAX_BOARD_MESSAGES;

// типы досок
extern const int GENERAL_BOARD;
extern const int NEWS_BOARD;
extern const int IDEA_BOARD;
extern const int ERROR_BOARD;
extern const int GODNEWS_BOARD;
extern const int GODGENERAL_BOARD;
extern const int GODBUILD_BOARD;
extern const int GODCODE_BOARD;
extern const int GODPUNISH_BOARD;
extern const int PERS_BOARD;
extern const int CLAN_BOARD;
extern const int CLANNEWS_BOARD;
extern const int NOTICE_BOARD;
extern const int MISPRINT_BOARD;
const int BOARD_TOTAL = 14;

// предметы, на которые вешаем спешиалы для старого способа работы с досками
#define GODGENERAL_BOARD_OBJ 250
#define GENERAL_BOARD_OBJ    251
#define GODCODE_BOARD_OBJ    253
#define GODPUNISH_BOARD_OBJ  257
#define GODBUILD_BOARD_OBJ   259

ACMD(report_on_board);

typedef boost::shared_ptr<Board> BoardPtr;
typedef std::vector<BoardPtr> BoardListType;
typedef boost::shared_ptr<struct Message> MessagePtr;
typedef std::vector<MessagePtr> MessageListType;

// даты последних прочтенных мессаг на досках
struct board_data {
	time_t board_date[BOARD_TOTAL];
};

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
	static void clear_god_boards();
	static void init_god_board(long uid, std::string name);
	static void reload_all();
	static SPECIAL(Special);
	static void LoginInfo(CHAR_DATA * ch);
	bool can_write(CHAR_DATA *ch);

	friend ACMD(DoBoard);
	friend ACMD(DoBoardList);
	friend ACMD(report_on_board);

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
	static void ShowMessage(CHAR_DATA * ch, MessagePtr message);
	static void create_board(int type, const std::string &name, const std::string &desc, const std::string &file);
};

#endif
