#include "boards.formatters.hpp"

#include "chars/character.h"
#include "screen.h"
#include "utils.h"
#include "conf.h"

#include <sstream>
#include <memory.h>

namespace Boards {
class CommonBoardFormatter : public Board::Formatter {
 public:
  CommonBoardFormatter(CHAR_DATA *character, const time_t &date) : m_date(date), m_character(character) {}

 protected:
  const auto &date() const { return m_date; }
  const auto character() const { return m_character; }

 private:
  const time_t m_date;
  CHAR_DATA *m_character;
};

class StreamBoardFormatter : public CommonBoardFormatter {
 public:
  StreamBoardFormatter(std::ostringstream &out, CHAR_DATA *character, const time_t &date) :
      CommonBoardFormatter(character, date), m_out(out) {}

 protected:
  void print_title(const Message::shared_ptr message);
  auto &out() { return m_out; }

 private:
  std::ostringstream &m_out;
};

void StreamBoardFormatter::print_title(const Message::shared_ptr message) {
  char timeBuf[17];
  char title[255];
  const std::tm *tm = std::localtime(&message->date);
  strftime(timeBuf, 17, "%H:%m %d-%m-%Y", tm);
  snprintf(title, 250, "%s: (%s) :: %s", timeBuf, message->author.c_str(), message->subject.c_str());
  if (message->date > date()) {
    out() << CCWHT(character(), C_NRM); // новые подсветим
  }
  out() << title << CCNRM(character(), C_NRM);
}

class Coder_BoardFormatter : public StreamBoardFormatter {
 public:
  Coder_BoardFormatter(std::ostringstream &out, CHAR_DATA *character, const time_t &date) :
      StreamBoardFormatter(out, character, date) {}

 private:
  virtual bool format(const Message::shared_ptr message) override;
};

bool Coder_BoardFormatter::format(const Message::shared_ptr message) {
  print_title(message);

  out() << " " << message->author << "\r\n";
  std::string text(message->text);
  out() << format_news_message(text);

  return true;
}

class News_And_GodNews_BoardFormatter : public StreamBoardFormatter {
 public:
  News_And_GodNews_BoardFormatter(std::ostringstream &out, CHAR_DATA *character, const time_t &date) :
      StreamBoardFormatter(out, character, date) {}

 private:
  virtual bool format(const Message::shared_ptr message) override;
};

bool News_And_GodNews_BoardFormatter::format(const Message::shared_ptr message) {
  print_title(message);

  out() << "\r\n" << message->text;

  return true;
}

class ClanNews_BoardFormatter : public StreamBoardFormatter {
 public:
  ClanNews_BoardFormatter(std::ostringstream &out, CHAR_DATA *character, const time_t &date) :
      StreamBoardFormatter(out, character, date) {}

 private:
  virtual bool format(const Message::shared_ptr message) override;
};

bool ClanNews_BoardFormatter::format(const Message::shared_ptr message) {
  if (message->date > date()) {
    out() << CCWHT(character(), C_NRM); // новые подсветим
  }
  out() << "[" << message->author << "] " << CCNRM(character(), C_NRM) << message->text;

  return true;
}

class SingleMessage_BoardFormatter : public StreamBoardFormatter {
 public:
  SingleMessage_BoardFormatter(std::ostringstream &out,
                               CHAR_DATA *character,
                               const time_t &date,
                               BoardTypes type,
                               time_t &last_date) :
      StreamBoardFormatter(out, character, date), m_last_date(last_date), m_type(type) {}

 private:
  virtual bool format(const Message::shared_ptr message) override;

  time_t &m_last_date;
  BoardTypes m_type;
};

bool SingleMessage_BoardFormatter::format(const Message::shared_ptr message) {
  if (message->date > date()) {
    special_message_format(out(), message);

    m_last_date = message->date;

    return false;
  }

  return true;
}

class ListMessages_BoardFormatter : public StreamBoardFormatter {
 public:
  ListMessages_BoardFormatter(std::ostringstream &out, CHAR_DATA *character, const time_t &date) :
      StreamBoardFormatter(out, character, date) {}

 private:
  virtual bool format(const Message::shared_ptr message) override;
};

bool ListMessages_BoardFormatter::format(const Message::shared_ptr message) {
  char timeBuf[17];
  if (message->date > date()) {
    out() << CCWHT(character(), C_NRM); // для выделения новых мессаг светло-белым
  }

  strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&message->date));
  out() << "[" << message->num + 1 << "] " << timeBuf << " "
        << "(" << message->author << ") :: "
        << message->subject << "\r\n" << CCNRM(character(), C_NRM);

  return true;
}

Boards::Board::Formatter::shared_ptr FormattersBuilder::coder(std::ostringstream &out,
                                                              CHAR_DATA *character,
                                                              const time_t &date) {
  return std::make_shared<Coder_BoardFormatter>(out, character, date);
}

Boards::Board::Formatter::shared_ptr FormattersBuilder::news_and_godnews(std::ostringstream &out,
                                                                         CHAR_DATA *character,
                                                                         const time_t &date) {
  return std::make_shared<News_And_GodNews_BoardFormatter>(out, character, date);
}

Boards::Board::Formatter::shared_ptr FormattersBuilder::clan_news(std::ostringstream &out,
                                                                  CHAR_DATA *character,
                                                                  const time_t &date) {
  return std::make_shared<ClanNews_BoardFormatter>(out, character, date);
}

Boards::Board::Formatter::shared_ptr FormattersBuilder::single_message(std::ostringstream &out,
                                                                       CHAR_DATA *character,
                                                                       const time_t &date,
                                                                       const BoardTypes type,
                                                                       time_t &last_date) {
  return std::make_shared<SingleMessage_BoardFormatter>(out, character, date, type, last_date);
}

Boards::Board::Formatter::shared_ptr FormattersBuilder::messages_list(std::ostringstream &out,
                                                                      CHAR_DATA *character,
                                                                      const time_t &date) {
  return std::make_shared<ListMessages_BoardFormatter>(out, character, date);
}

Boards::Board::Formatter::shared_ptr FormattersBuilder::create(const BoardTypes type,
                                                               std::ostringstream &out,
                                                               CHAR_DATA *character,
                                                               const time_t &date) {
  switch (type) {
    case CODER_BOARD: return coder(out, character, date);

    case NEWS_BOARD:
    case GODNEWS_BOARD: return news_and_godnews(out, character, date);

    case CLANNEWS_BOARD: return clan_news(out, character, date);

    default: return nullptr;
  }
}

void special_message_format(std::ostringstream &out, const Message::shared_ptr message) {
  char timeBuf[17];
  strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&message->date));

  out << "[" << message->num + 1 << "] "
      << timeBuf << " "
      << "(" << message->author << ") :: "
      << message->subject << "\r\n\r\n"
      << message->text << "\r\n";
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
