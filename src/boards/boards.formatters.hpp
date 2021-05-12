#ifndef __BOARDS_FORMATTERS_HPP__
#define __BOARDS_FORMATTERS_HPP__

#include "boards.types.hpp"

#include <sstream>

class CHAR_DATA;    // to avoid inclusion of "char.hpp"

namespace Boards {
void special_message_format(std::ostringstream &out,
                            const Message::shared_ptr message);    // TODO: Get rid of this special case.

class FormattersBuilder {
 public:
  static Board::Formatter::shared_ptr coder(std::ostringstream &out, CHAR_DATA *character, const time_t &date);
  static Board::Formatter::shared_ptr news_and_godnews(std::ostringstream &out,
                                                       CHAR_DATA *character,
                                                       const time_t &date);
  static Board::Formatter::shared_ptr clan_news(std::ostringstream &out, CHAR_DATA *character, const time_t &date);
  static Board::Formatter::shared_ptr single_message(std::ostringstream &out,
                                                     CHAR_DATA *character,
                                                     const time_t &date,
                                                     const BoardTypes type,
                                                     time_t &last_date);
  static Board::Formatter::shared_ptr messages_list(std::ostringstream &out, CHAR_DATA *character, const time_t &date);
  static Board::Formatter::shared_ptr create(const BoardTypes type,
                                             std::ostringstream &out,
                                             CHAR_DATA *character,
                                             const time_t &date);
};
}

#endif // __BOARDS_FORMATTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
