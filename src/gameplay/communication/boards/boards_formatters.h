#ifndef __BOARDS_FORMATTERS_HPP__
#define __BOARDS_FORMATTERS_HPP__

#include "boards_types.h"

#include <sstream>
#include <string>
#include <string_view>

class CharData;    // to avoid inclusion of "char.hpp"

namespace Boards {
// Предел темы сообщения -- в ЗНАКАХ, не в байтах.
constexpr std::size_t kSubjectMaxChars = 40;

// Тема, укороченная до kSubjectMaxChars знаков, с резом по границе знака. Под UTF-8 рез по
// длине строки и укорачивал тему вдвое, и оставлял в конце один ведущий байт -- такая тема
// уходила и автору, и всем читающим доску (issue #3681).
std::string CutSubject(std::string_view subject);

void special_message_format(std::ostringstream &out,
							const Message::shared_ptr message,
							size_t max_length = 0);    // TODO: Get rid of this special case.

class FormattersBuilder {
 public:
	static Board::Formatter::shared_ptr coder(std::ostringstream &out, CharData *character, const time_t &date);
	static Board::Formatter::shared_ptr news_and_godnews(std::ostringstream &out,
														 CharData *character,
														 const time_t &date);
	static Board::Formatter::shared_ptr clan_news(std::ostringstream &out, CharData *character, const time_t &date);
	static Board::Formatter::shared_ptr single_message(std::ostringstream &out,
													   CharData *character,
													   const time_t &date,
													   const BoardTypes type,
													   time_t &last_date);
	static Board::Formatter::shared_ptr messages_list(std::ostringstream &out,
													  CharData *character,
													  const time_t &date);
	static Board::Formatter::shared_ptr create(const BoardTypes type,
											   std::ostringstream &out,
											   CharData *character,
											   const time_t &date);
};
}

#endif // __BOARDS_FORMATTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
