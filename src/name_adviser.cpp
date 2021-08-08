#include "name_adviser.h"

#include "logger.h"
#include "pugixml.h"
#include "boot/boot_constants.h"

#include <fstream>
#include <regex>

bool NameAdviser::load()
{
	m_initialized = true;
	m_list_of_free_names.clear();
	std::srand(static_cast<unsigned int>((std::time(0))));

	pugi::xml_document doc;
	const pugi::xml_parse_result result = doc.load_file(FNAME_FILE);
		if (!result) {
		log("NameAdviser: Could not open file to load free names");
		return false;
	}

	const std::regex name_regex ("^[А-Я][а-я]{1,15}$");
	pugi::xml_node node_free_names = doc.child("free_names");
	for (const auto& node_name : node_free_names) {
		// check name to avoid wrong format
		std::string name(node_name.value());
		if (std::regex_match(name, name_regex)) {
			m_list_of_free_names.insert(name);
		}
	}

	return true;
}

bool NameAdviser::save()
{
	if (!m_initialized) {
		log("NameAdviser: Free names file could not be saved, because it was not initialized");
		return false;
	}

	pugi::xml_document doc;
	doc.append_child().set_name("free_names");
	pugi::xml_node node_free_names = doc.child("free_names");

	for (const std::string &free_name : m_list_of_free_names) {
		node_free_names.append_attribute(free_name.c_str());
	}

	doc.save_file(FNAME_FILE);
	return true;
}

size_t NameAdviser::count()
{
	return m_list_of_free_names.size();
}

void NameAdviser::add(const std::string &free_name)
{
	m_list_of_free_names.insert(free_name);
}

void NameAdviser::remove(const std::string &name)
{
	m_list_of_free_names.erase(name);
}

std::vector<std::string> NameAdviser::get_random_name_list(size_t counter)
{
	std::vector<std::string> result;
	if (counter >= m_list_of_free_names.size()) {
		std::copy(m_list_of_free_names.begin(), m_list_of_free_names.end(), std::back_inserter(result));
	} else {
		// TODO
		std::vector<size_t> unique_numbers;
		unique_numbers.resize(m_list_of_free_names.size());
		size_t counter = 0;
		std::generate(unique_numbers.begin(), unique_numbers.end(), [&counter]() {
			return counter++;
		});

		std::random_shuffle(unique_numbers.begin(), unique_numbers.end());
		auto it_end_range = m_list_of_free_names.begin();
		std::advance(it_end_range, counter);
		std::copy(m_list_of_free_names.begin(), it_end_range, std::back_inserter(result));
	}

	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
