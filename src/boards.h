// Copyright (c) 2005 Krodo
// Part of Bylins http://www.mud.ru

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include <string>
#include <vector>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"

namespace Boards
{

// возможные типы доступа к доскам
enum Access
{
	ACCESS_CAN_SEE,    // видно в списке по 'доски' и попытках обратиться к доске
	ACCESS_CAN_READ,   // можно листать и читать сообщения
	ACCESS_CAN_WRITE,  // можно писать сообщения
	ACCESS_FULL,       // можно удалять чужие сообщения
	ACCESS_NUM         // кол-во флагов
};

// типы досок
enum BoardTypes: int
{
	GENERAL_BOARD,    // общая
	NEWS_BOARD,       // новости
	IDEA_BOARD,       // идеи
	ERROR_BOARD,      // баги (ошибка)
	GODNEWS_BOARD,    // новости (только для иммов)
	GODGENERAL_BOARD, // общая (только для иммов)
	GODBUILD_BOARD,   // билдеры (только для иммов)
	GODCODE_BOARD,    // кодеры (только для иммов)
	GODPUNISH_BOARD,  // наказания (только для иммов)
	PERS_BOARD,       // персональная (только для иммов)
	CLAN_BOARD,       // клановая
	CLANNEWS_BOARD,   // клановые новости
	NOTICE_BOARD,     // анонсы
	MISPRINT_BOARD,   // очепятки (опечатка)
	SUGGEST_BOARD,    // придумки (мысль)
	CODER_BOARD,      // ченж-лог из меркуриала
	TYPES_NUM         // кол-во досок
};

extern const int GODGENERAL_BOARD_OBJ;
extern const int GENERAL_BOARD_OBJ;
extern const int GODCODE_BOARD_OBJ;
extern const int GODPUNISH_BOARD_OBJ;
extern const int GODBUILD_BOARD_OBJ;
extern const unsigned MAX_BOARD_MESSAGES;
extern const unsigned MAX_REPORT_MESSAGES;
extern const char *OVERFLOW_MESSAGE;

extern std::string dg_script_text;

bool can_see(CHAR_DATA *ch, const Board &board);
bool can_read(CHAR_DATA *ch, const Board &board);
bool can_write(CHAR_DATA *ch, const Board &board);
bool full_access(CHAR_DATA *ch, const Board &board);
void clan_delete_message(const std::string &name, int vnum);
void new_message_notify(const Board &board);

std::string& format_news_message(std::string &text);

} // namespace BoardSystem

ACMD(report_on_board);

typedef boost::shared_ptr<struct Message> MessagePtr;
typedef std::vector<MessagePtr> MessageListType;

// отдельное сообщение
struct Message
{
	Message() : num(0), unique(0), level(0), rank(0), date(0) {};

	int num;             // номер на доске
	std::string author;  // имя автора
	long unique;         // уид автора
	int level;           // уровень автора (для всех, кроме клановых и персональных досок)
	int rank;            // для дрн и дрвече
	time_t date;         // дата поста
	std::string subject; // тема
	std::string text;    // текст сообщения
};

class Board
{
public:
	Board(Boards::BoardTypes in_type)
		: type(in_type), lastWrite(0), clanRent(0), persUnique(0) {};

	MessageListType messages; // список сообщений

	Boards::BoardTypes GetType() const;
	int GetClanRent() const;
	const std::string & GetName() const;

	long get_lastwrite() const;
	void set_lastwrite(long unique);

	void Save();
	void add_message(MessagePtr msg);
	void new_message_notify() const;

	std::bitset<Boards::ACCESS_NUM> get_access(CHAR_DATA *ch) const;
	void do_list(CHAR_DATA *ch) const;
	bool is_special() const;
	time_t last_message_date() const;

	const std::string & get_alias() const;

	static void BoardInit();
	static void ClanInit();
	static void clear_god_boards();
	static void init_god_board(long uid, std::string name);
	static void reload_all();
	static SPECIAL(Special);
	static void LoginInfo(CHAR_DATA * ch);

	friend ACMD(DoBoard);
	friend ACMD(DoBoardList);
	friend ACMD(report_on_board);

private:
	Boards::BoardTypes type;  // тип доски
	std::string name;         // имя доски
	std::string desc;         // описание доски
	long lastWrite;           // уид последнего писавшего (до ребута)
	int clanRent;             // номер ренты клана (для клановых досок)
	int persUnique;           // уид (для персональной доски)
	std::string persName;     // имя (для персональной доски)
	std::string file;         // имя файла для сейва/лоада
	std::string alias_;        // однострочные алиасы для спец.досок

	void Load();

	static void ShowMessage(CHAR_DATA * ch, MessagePtr message);
	static void create_board(Boards::BoardTypes type, const std::string &name,
		const std::string &desc, const std::string &file);
};

#endif
