#include "ignores.loader.hpp"

#include "logger.hpp"
#include "ignores.hpp"
#include "chars/character.h"

#include <boost/algorithm/string.hpp>
class IgnoreParser
{
public:
	IgnoreParser(const char* line, const CHAR_DATA* character) : m_pos(line), m_character(character) {}

	ignore_data::shared_ptr parse();

private:
	static ignore_data::shared_ptr parse_ignore(std::string buf);
	void skip_spaces();
	bool skip_all_spaces();

	std::string m_pos;
	const CHAR_DATA* m_character;
};

ignore_data::shared_ptr IgnoreParser::parse()
{
	boost::erase_all(this->m_pos, " ");

	const auto result = parse_ignore(m_pos);
	if (!result)
	{
		log("WARNING: could not parse ignore list [%s] of %s: invalid format.", m_pos.c_str(), m_character->get_name().c_str());
	}

	return result;
}

ignore_data::shared_ptr IgnoreParser::parse_ignore(std::string buffer)
{
	auto result = std::make_shared<ignore_data>();

	if (sscanf(buffer.c_str(), "[%ld]%ld", &result->mode, &result->id) < 2)
	{
		result.reset();
	}

	return result;
}

void IgnoresLoader::load_from_string(const char *line)
{
	IgnoreParser parser(line, m_character);
	const auto ignore = parser.parse();
	if (!player_exists(ignore->id))
	{
		ignore->id = 0;
	}
	m_character->add_ignore(ignore);
	
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
