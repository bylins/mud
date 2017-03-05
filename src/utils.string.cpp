#include "utils.string.hpp"

#include <string.h>

namespace utils
{
	template <typename T>
	void remove_colors_template(T string, int& new_length)
	{
		int pos = new_length = 0;
		while (string[pos])
		{
			if ('&' == string[pos]
				&& string[1 + pos])
			{
				++pos;
			}
			else
			{
				string[new_length++] = string[pos];
			}
			++pos;
		}
	}

	void remove_colors(char* string)
	{
		if (string)
		{
			int new_length = 0;
			remove_colors_template<char*>(string, new_length);
			string[new_length] = '\0';
		}
	}

	void remove_colors(std::string& string)
	{
		int new_length = 0;
		remove_colors_template<std::string&>(string, new_length);
		string.resize(new_length);
	}

	shared_string_ptr get_string_without_colors(const char* string)
	{
		shared_string_ptr result;
		if (string)
		{
			result.reset(strdup(string), free);

			remove_colors(result.get());
		}

		return result;
	}

	std::string get_string_without_colors(const std::string& string)
	{
		std::string result = string;

		remove_colors(result);

		return result;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
