// Part of Bylins http://www.mud.ru
// Dictionary loader implementation

#ifdef HAVE_YAML

#include "dictionary_loader.h"
#include "utils/logger.h"

#include <yaml-cpp/yaml.h>
#include <filesystem>

namespace world_loader
{

// ============================================================================
// Dictionary implementation
// ============================================================================

Dictionary::Dictionary(const std::string &name)
	: m_name(name)
{
}

bool Dictionary::Load(const std::string &filepath)
{
	try
	{
		YAML::Node root = YAML::LoadFile(filepath);
		if (!root.IsMap())
		{
			log("SYSERR: Dictionary file '%s' is not a YAML map", filepath.c_str());
			return false;
		}

		m_entries.clear();
		for (const auto &pair : root)
		{
			std::string key = pair.first.as<std::string>();
			long value = pair.second.as<long>();
			m_entries[key] = value;
		}

		return true;
	}
	catch (const YAML::Exception &e)
	{
		log("SYSERR: Failed to load dictionary '%s': %s", filepath.c_str(), e.what());
		return false;
	}
	catch (const std::exception &e)
	{
		log("SYSERR: Failed to load dictionary '%s': %s", filepath.c_str(), e.what());
		return false;
	}
}

long Dictionary::Lookup(const std::string &name, long default_val) const
{
	auto it = m_entries.find(name);
	if (it != m_entries.end())
	{
		return it->second;
	}
	return default_val;
}

bool Dictionary::Contains(const std::string &name) const
{
	return m_entries.find(name) != m_entries.end();
}

// ============================================================================
// DictionaryManager implementation
// ============================================================================

DictionaryManager &DictionaryManager::Instance()
{
	static DictionaryManager instance;
	return instance;
}

bool DictionaryManager::LoadDictionaries(const std::string &dict_dir)
{
	namespace fs = std::filesystem;

	if (!fs::exists(dict_dir) || !fs::is_directory(dict_dir))
	{
		log("SYSERR: Dictionary directory not found: %s", dict_dir.c_str());
		return false;
	}

	log("Loading dictionaries from: %s", dict_dir.c_str());

	int loaded_count = 0;
	int error_count = 0;

	for (const auto &entry : fs::directory_iterator(dict_dir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		std::string filename = entry.path().filename().string();
		if (filename.size() < 6 || filename.substr(filename.size() - 5) != ".yaml")
		{
			continue;
		}

		// Extract dictionary name from filename (remove .yaml extension)
		std::string dict_name = filename.substr(0, filename.size() - 5);

		auto dict = std::make_unique<Dictionary>(dict_name);
		if (dict->Load(entry.path().string()))
		{
			log("   Loaded dictionary '%s' with %zu entries",
				dict_name.c_str(), dict->GetEntries().size());
			m_dictionaries[dict_name] = std::move(dict);
			loaded_count++;
		}
		else
		{
			error_count++;
		}
	}

	if (error_count > 0)
	{
		log("SYSERR: %d dictionary files failed to load", error_count);
	}

	m_loaded = (loaded_count > 0);
	log("Loaded %d dictionaries", loaded_count);

	return m_loaded;
}

const Dictionary *DictionaryManager::GetDictionary(const std::string &name) const
{
	auto it = m_dictionaries.find(name);
	if (it != m_dictionaries.end())
	{
		return it->second.get();
	}
	return nullptr;
}

long DictionaryManager::Lookup(const std::string &dict_name, const std::string &entry_name, long default_val) const
{
	const Dictionary *dict = GetDictionary(dict_name);
	if (!dict)
	{
		return default_val;
	}
	return dict->Lookup(entry_name, default_val);
}

std::vector<long> DictionaryManager::LookupAll(const std::string &dict_name, const std::vector<std::string> &names) const
{
	std::vector<long> result;
	result.reserve(names.size());

	const Dictionary *dict = GetDictionary(dict_name);
	if (!dict)
	{
		return result;
	}

	for (const auto &name : names)
	{
		long val = dict->Lookup(name, -1);
		if (val >= 0)
		{
			result.push_back(val);
		}
	}

	return result;
}

void DictionaryManager::Clear()
{
	m_dictionaries.clear();
	m_loaded = false;
}

} // namespace world_loader

#endif // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
