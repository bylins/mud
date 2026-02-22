#ifndef __BOARDS_TYPES_HPP__
#define __BOARDS_TYPES_HPP__

#include "boards_message.h"

#include <string>
#include <memory>
#include <deque>
#include <bitset>

namespace Boards {
// возможные типы доступа к доскам
enum Access {
	ACCESS_CAN_SEE,    // видно в списке по 'доски' и попытках обратиться к доске
	ACCESS_CAN_READ,   // можно листать и читать сообщения
	ACCESS_CAN_WRITE,  // можно писать сообщения
	ACCESS_FULL,       // можно удалять чужие сообщения
	ACCESS_NUM         // кол-во флагов
};

// типы досок
enum BoardTypes : int {
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
	CODER_BOARD,      // выборка из ченж-лога, затрагивающая игроков
	TYPES_NUM         // кол-во досок
};

class Board {
 public:
	using shared_ptr = std::shared_ptr<Board>;

	class Formatter {
	 public:
		using shared_ptr = std::shared_ptr<Formatter>;

		virtual ~Formatter() {}

		virtual bool format(const Message::shared_ptr message) = 0;
	};

	Board(BoardTypes in_type);

	MessageListType messages; // список сообщений

	auto get_type() const { return type_; }

	const auto &get_name() const { return name_; }
	void set_name(const std::string &new_name) { name_ = new_name; }

	auto get_lastwrite() const { return last_write_; }
	void set_lastwrite_uid(long unique) { last_write_ = unique; }

	void Save();
	void renumerate_messages();
	void write_message(Message::shared_ptr message);
	void add_message(Message::shared_ptr message);

	bool is_special() const;
	time_t last_message_date() const;

	const auto &get_alias() const { return alias_; }
	void set_alias(const std::string &new_alias) { alias_ = new_alias; }

	auto get_blind() const { return blind_; }
	void set_blind(const bool value) { blind_ = value; }

	const auto &get_description() const { return desc_; }
	void set_description(const std::string &new_description) { desc_ = new_description; }

	void set_file_name(const std::string &file_name) { file_ = file_name; }

	void set_clan_rent(const int value) { clan_rent_ = value; }
	auto get_clan_rent() const { return clan_rent_; }

	void set_pers_unique(const int uid) { pers_unique_ = uid; }
	const auto &get_pers_uniq() const { return pers_unique_; }

	void set_pers_name(const std::string &name) { pers_name_ = name; }
	const auto &get_pers_name() const { return pers_name_; }

	bool empty() const { return messages.empty(); }
	auto messages_count() const { return messages.size(); }
	void erase_message(const size_t index);
	int count_unread(time_t last_read) const;
	auto get_last_message() const { return *messages.begin(); }
	auto get_message(const size_t index) const { return messages[index]; }

	void Load();

	void format_board(Formatter::shared_ptr formatter) const {
		format_board_implementation(formatter,
									messages.begin(),
									messages.end());
	}
	void format_board_in_reverse(Formatter::shared_ptr formatter) const {
		format_board_implementation(formatter,
									messages.rbegin(),
									messages.rend());
	}

 private:
	template<typename ForwardIterator>
	void format_board_implementation(Formatter::shared_ptr formatter,
									 ForwardIterator begin,
									 const ForwardIterator &end) const;

	BoardTypes type_;  // тип доски
	std::string name_;         // имя доски
	std::string desc_;         // описание доски
	long last_write_;          // уид последнего писавшего (до ребута)
	int clan_rent_;            // номер ренты клана (для клановых досок)
	int pers_unique_;          // уид (для персональной доски)
	std::string pers_name_;    // имя (для персональной доски)
	std::string file_;         // имя файла для сейва/лоада
	std::string alias_;        // однострочные алиасы для спец.досок
	bool blind_;               // игроки не видят счетчики сообщений
};

template<typename ForwardIterator>
void Boards::Board::format_board_implementation(Formatter::shared_ptr formatter,
												ForwardIterator pos,
												const ForwardIterator &end) const {
	while (pos != end) {
		if (!formatter->format(*pos)) {
			break;
		}
		++pos;
	}
}
}

#endif // __BOARDS_TYPES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
