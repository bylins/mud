#ifndef __IGNORES_LOADER_HPP__
#define __IGNORES_LOADER_HPP__

class CharacterData;    // to avoid inclusion of "char.hpp"

class IgnoresLoader {
 public:
	IgnoresLoader(CharacterData *character) : m_character(character) {}

	void load_from_string(const char *line);

 private:
	CharacterData *m_character;
};

#endif // __IGNORES_LOADER_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
