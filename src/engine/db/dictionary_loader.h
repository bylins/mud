// Part of Bylins http://www.mud.ru
// Dictionary loader for YAML world data source
// Maps string names (like "kForest") to numeric enum values

#ifndef DICTIONARY_LOADER_H_
#define DICTIONARY_LOADER_H_

#ifdef HAVE_YAML

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace world_loader
{

// Single dictionary that maps string names to numeric values
class Dictionary
{
public:
	Dictionary() = default;
	explicit Dictionary(const std::string &name);

	// Load dictionary from YAML file
	bool Load(const std::string &filepath);

	// Lookup a single value by name
	// Returns default_val if not found
	long Lookup(const std::string &name, long default_val = -1) const;

	// Check if name exists in dictionary
	bool Contains(const std::string &name) const;

	// Get dictionary name (for error messages)
	const std::string &GetName() const { return m_name; }

	// Get all entries (for debugging)
	const std::unordered_map<std::string, long> &GetEntries() const { return m_entries; }

private:
	std::string m_name;
	std::unordered_map<std::string, long> m_entries;
};

// Manager for all dictionaries
class DictionaryManager
{
public:
	// Singleton access
	static DictionaryManager &Instance();

	// Load all dictionaries from directory
	// Returns true if all required dictionaries loaded successfully
	bool LoadDictionaries(const std::string &dict_dir);

	// Check if dictionaries are loaded
	bool IsLoaded() const { return m_loaded; }

	// Get a specific dictionary by name
	const Dictionary *GetDictionary(const std::string &name) const;

	// Universal lookup: "dict_name", "entry_name" -> value
	// Returns default_val if dictionary or entry not found
	long Lookup(const std::string &dict_name, const std::string &entry_name, long default_val = -1) const;

	// Lookup all names from a list and return their values
	// Useful for converting flag lists
	std::vector<long> LookupAll(const std::string &dict_name, const std::vector<std::string> &names) const;

	// Clear all loaded dictionaries
	void Clear();

private:
	DictionaryManager() = default;
	~DictionaryManager() = default;

	// Prevent copying
	DictionaryManager(const DictionaryManager &) = delete;
	DictionaryManager &operator=(const DictionaryManager &) = delete;

	bool m_loaded = false;
	std::unordered_map<std::string, std::unique_ptr<Dictionary>> m_dictionaries;
};

} // namespace world_loader

#endif // HAVE_YAML

#endif // DICTIONARY_LOADER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
