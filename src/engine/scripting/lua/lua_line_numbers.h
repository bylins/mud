#ifndef BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_LINE_NUMBERS_H_
#define BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_LINE_NUMBERS_H_

#include <cstdio>
#include <limits>
#include <optional>
#include <string>

namespace lua_scripting {

inline std::optional<std::string> GetSourceLine(const std::string &source, int requested_line)
{
	if (requested_line < 1)
	{
		return std::nullopt;
	}

	size_t line_begin = 0;
	int line_number = 1;
	while (line_begin <= source.size())
	{
		size_t line_end = line_begin;
		while (line_end < source.size() && source[line_end] != '\r' && source[line_end] != '\n')
		{
			++line_end;
		}
		if (line_number == requested_line)
		{
			return source.substr(line_begin, line_end - line_begin);
		}
		if (line_end == source.size())
		{
			break;
		}

		const char newline = source[line_end++];
		if (line_end < source.size()
			&& (source[line_end] == '\r' || source[line_end] == '\n')
			&& source[line_end] != newline)
		{
			++line_end;
		}
		line_begin = line_end;
		++line_number;
	}
	return std::nullopt;
}

// Lua treats CR, LF, CRLF and LFCR as line boundaries; paired characters
// count as one newline. The contents are copied byte-for-byte, so this is
// independent of the source encoding.
inline std::string FormatNumberedSource(const std::string &source, int first_line, int last_line)
{
	if (first_line < 1 || last_line < first_line)
	{
		return {};
	}

	std::string result;
	size_t line_begin = 0;
	int line_number = 1;
	while (line_begin <= source.size() && line_number <= last_line)
	{
		size_t line_end = line_begin;
		while (line_end < source.size() && source[line_end] != '\r' && source[line_end] != '\n')
		{
			++line_end;
		}
		if (line_number >= first_line)
		{
			char prefix[32];
			snprintf(prefix, sizeof(prefix), "%4d:  ", line_number);
			result += prefix;
			result.append(source, line_begin, line_end - line_begin);
			result += "\r\n";
		}
		if (line_end == source.size())
		{
			break;
		}

		const char newline = source[line_end++];
		if (line_end < source.size()
			&& (source[line_end] == '\r' || source[line_end] == '\n')
			&& source[line_end] != newline)
		{
			++line_end;
		}
		line_begin = line_end;
		++line_number;
	}
	return result;
}

} // namespace lua_scripting

#endif // BYLINS_SRC_ENGINE_SCRIPTING_LUA_LUA_LINE_NUMBERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
