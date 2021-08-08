#pragma once

#include <vector>
#include <set>
#include <string>

// save/load/advise free name for new chars
class NameAdviser {
public:
	NameAdviser() = default;

	NameAdviser (const NameAdviser&) = delete;
	NameAdviser& operator= (const NameAdviser&) = delete;

public:
	// load list of free names from file
	bool load();

	// save list of free names to file
	bool save();

	// count of free names
	size_t count();

	// add free name to list
	void add(const std::string &free_name);

	// remove free name from list
	void remove(const std::string &name);

	// get list of random free names (maximum list size is limited to counter)
	std::vector<std::string> get_random_name_list(size_t counter);

private:
	bool m_initialized = false;

private:
	std::set<std::string> m_list_of_free_names;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
