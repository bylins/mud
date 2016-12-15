#ifndef __LEVENSHTEIN_HPP__
#define __LEVENSHTEIN_HPP__

#include <string>

int levenshtein(const std::string& string1, const std::string& string2,
	int swap_penalty, int substitution_penalty,
	int insertion_penalty, int deletion_penalty);

#endif	// __LEVENSHTEIN_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
