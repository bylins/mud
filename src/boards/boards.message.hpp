#ifndef __BOARDS_MESSAGE_HPP__
#define __BOARDS_MESSAGE_HPP__

#include <string>
#include <memory>
#include <deque>

// отдельное сообщение
struct Message {
  using shared_ptr = std::shared_ptr<Message>;

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

using MessageListType = std::deque<Message::shared_ptr>;

#endif // __BOARDS_MESSAGE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
