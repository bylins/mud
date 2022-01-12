#include "name_adviser.h"

#include "db.h"
#include "logger.h"
#include "utils/utils.h"
#include "boot/boot_constants.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include "chars/char_player.h"

NameAdviser::NameAdviser() {
	std::srand(static_cast<unsigned int>((std::time(0))));
}

void NameAdviser::add(const std::string &free_name)
{
	const size_t name_counter =  std::count(m_list_of_free_names.begin(), m_list_of_free_names.end(), free_name);

	if (!name_counter) {
		// extract all known similar names
		std::list<std::string> similar_names;
		std::copy_if(m_list_of_removed_names.begin(), m_list_of_removed_names.end(), std::back_inserter(similar_names), [this, &free_name](const std::string &name) {
			return is_names_similar(free_name, name);
		});
		m_list_of_removed_names.remove_if([&free_name, this](const std::string &name) {
			return is_names_similar(free_name, name);
		});

		// just to avoid duplicates
		similar_names.remove(free_name);
		similar_names.push_back(free_name);

		m_list_of_free_names.splice(m_list_of_free_names.end(), similar_names);
	}

	remove_duplicates();
}

void NameAdviser::remove(const std::string &name_to_remove)
{
	std::list<std::string> removed_similar_names;

	m_list_of_free_names.remove(name_to_remove);

	// if name is in list of removed names - remove it finally
	m_list_of_removed_names.remove(name_to_remove);

	// search for similar names
	std::copy_if(m_list_of_free_names.begin(), m_list_of_free_names.end(), std::back_inserter(removed_similar_names), [this, &name_to_remove](const std::string &name) {
		return is_names_similar(name_to_remove, name);
	});

	// if similar names exists - remove and remember them
	if (!removed_similar_names.empty()) {
		m_list_of_free_names.remove_if([&name_to_remove, this](const std::string &name) {
			return is_names_similar(name_to_remove, name);
		});

		// if several names removed - remember them
		removed_similar_names.push_back(name_to_remove);
		m_list_of_removed_names.splice(m_list_of_removed_names.end(), removed_similar_names);
	}

	// to avoid any inconsistence - remove duplicates
	remove_duplicates();
}

std::vector<std::string> NameAdviser::get_random_name_list()
{
	static const short suggestion_counter = 4;

	std::vector<std::string> result;
	if (suggestion_counter >= m_list_of_free_names.size()) {
		std::copy(m_list_of_free_names.begin(), m_list_of_free_names.end(), std::back_inserter(result));
	} else {
		std::vector<size_t> unique_numbers;
		unique_numbers.resize(m_list_of_free_names.size());
		size_t counter = 0;
		std::generate(unique_numbers.begin(), unique_numbers.end(), [&counter]() {
			return counter++;
		});

		// extract random names
		std::random_shuffle(unique_numbers.begin(), unique_numbers.end());
		for (short i = 0; i < suggestion_counter; i++) {
			auto it_name = m_list_of_free_names.begin();
			std::advance(it_name, unique_numbers.at(i));
			result.push_back(*it_name);
		}
	}

	return result;
}

void NameAdviser::init()
{
	m_list_of_free_names.clear();
	m_list_of_removed_names.clear();

	// reading ANAME_FILE and try to load each char
	std::ifstream approved_names_file(ANAME_FILE);
	if (!approved_names_file.is_open() || !approved_names_file.is_open()) {
		log("NameAdviser: could bot build free_names list");
		return;
	}

	std::string line;
	while (std::getline(approved_names_file, line)) {
		std::istringstream iss(line);
		std::string char_name;

		// first word on each line is name
		iss >> char_name;

		Player tmp_player;
		const int char_i = load_char(char_name.c_str(), &tmp_player);
		if (char_i > -1 && !PLR_FLAGGED(&tmp_player, PLR_DELETED)) {
			// char exists and not deleted - skip name
			remove(char_name);
			continue;
		}

		add(char_name);
	}

	approved_names_file.close();
}

bool NameAdviser::is_names_similar(const std::string &left, const std::string &right)
{
	if ((left.length() < kMinNameLength) || (right.length() < kMinNameLength)) {
		return false;
	}

	std::string short_left = left.substr(0, kMinNameLength);
	for (auto &ch : short_left) {
		ch = UPPER(ch);
	}

	std::string short_rigth = right.substr(0, kMinNameLength);
	for (auto &ch : short_rigth) {
		ch = UPPER(ch);
	}

	return short_left == short_rigth;
}

void NameAdviser::remove_duplicates()
{
	m_list_of_free_names.sort();
	m_list_of_free_names.unique();

	m_list_of_removed_names.sort();
	m_list_of_removed_names.unique();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
