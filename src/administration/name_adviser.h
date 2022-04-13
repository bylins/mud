#pragma once

#include <vector>
#include <list>
#include <string>

// suggests free names to new players
class NameAdviser {
public:
	NameAdviser();

	NameAdviser (const NameAdviser&) = delete;
	NameAdviser& operator= (const NameAdviser&) = delete;

public:
	// add free name to list
	void add(const std::string &free_name);

	// remove free name from list
	void remove(const std::string &name_to_remove);

	// get list of random free names (maximum list size is hardcoded as suggestion_counter)
	std::vector<std::string> get_random_name_list();

	// load player list and list of approved names
	void init();

private:
	static bool is_names_similar(const std::string &left, const std::string &right);
	void remove_duplicates();

private:
	std::list<std::string> m_list_of_free_names;
	std::list<std::string> m_list_of_removed_names;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
