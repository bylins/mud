#include "ignores.loader.hpp"

#include "logger.hpp"
#include "ignores.hpp"
#include "char.hpp"

class IgnoreParser
{
public:
	IgnoreParser(const char* line, const CHAR_DATA* character) : m_pos(line), m_character(character) {}

	ignore_data::shared_ptr parse();

private:
	static ignore_data::shared_ptr parse_ignore(const char *buf);
	bool skip_spaces();
	bool skip_until_spaces();

	const char* m_pos;
	const CHAR_DATA* m_character;
};

ignore_data::shared_ptr IgnoreParser::parse()
{
	if (!skip_until_spaces())
	{
		return nullptr;
	}

	if (!skip_spaces())
	{
		return nullptr;
	}

	const auto result = parse_ignore(m_pos);
	if (!result)
	{
		log("WARNING: could not parse ignore list [%s] of %s: invalid format.", m_pos, m_character->get_name().c_str());
	}

	return result;
}

ignore_data::shared_ptr IgnoreParser::parse_ignore(const char *buffer)
{
	auto result = std::make_shared<ignore_data>();

	if (sscanf(buffer, "[%ld]%ld", &result->mode, &result->id) < 2)
	{
		result.reset();
	}

	return result;
}

bool IgnoreParser::skip_spaces()
{
	do
	{
		if ('\0' == *m_pos)
		{
			return true;
		}

		++m_pos;
	} while (' ' == *m_pos || '\t' == *m_pos);

	return true;
}

bool IgnoreParser::skip_until_spaces()
{
	while (*m_pos && ' ' != *m_pos && '\t' != *m_pos)
	{
		++m_pos;
	}
	return '\0' != *m_pos;
}

void IgnoresLoader::load_from_string(const char *line)
{
	IgnoreParser parser(line, m_character);
	while (const auto ignore = parser.parse())
	{
		if (!player_exists(ignore->id))
		{
			ignore->id = 0;
		}
		m_character->add_ignore(ignore);
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
