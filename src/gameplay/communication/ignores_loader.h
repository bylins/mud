#ifndef __IGNORES_LOADER_HPP__
#define __IGNORES_LOADER_HPP__

class CharData;    // to avoid inclusion of "char.hpp"

class IgnoresLoader {
 public:
	IgnoresLoader(CharData *character) : m_character(character) {}

	void load_from_string(const char *line);

 private:
	CharData *m_character;
};

#endif // __IGNORES_LOADER_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
