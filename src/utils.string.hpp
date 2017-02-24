#ifndef __UTILS_STRING_HPP__
#define __UTILS_STRING_HPP__

#include <memory>
#include <string>

namespace utils
{
	using shared_string_ptr = std::shared_ptr<char>;

	void remove_colors(char* string);
	void remove_colors(std::string& string);
	shared_string_ptr get_string_without_colors(const char* string);
	std::string get_string_without_colors(const std::string& string);
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
