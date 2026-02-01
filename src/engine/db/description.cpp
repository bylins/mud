// description.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "description.h"

#include "utils/logger.h"

// ========== LocalDescriptionIndex ==========

size_t LocalDescriptionIndex::add(const std::string &text)
{
	// Check if we already have this description
	auto it = _desc_map.find(text);
	if (it != _desc_map.end())
	{
		// Duplicate found -> return existing index
		return it->second;
	}
	else
	{
		// New description -> add to index (1-based for compatibility with Legacy)
		_desc_list.push_back(text);
		size_t idx = _desc_list.size();  // 1-based! (0 = no description)
		_desc_map[text] = idx;
		return idx;
	}
}

const std::string &LocalDescriptionIndex::get(size_t idx) const
{
	static const std::string empty_string = "";
	if (idx == 0)
	{
		return empty_string;  // 0 means "no description"
	}
	try
	{
		return _desc_list.at(idx - 1);  // Convert 1-based to 0-based
	}
	catch (const std::out_of_range &)
	{
		log("SYSERR: bad local description index %zd (size=%zd)", idx, _desc_list.size());
		return empty_string;
	}
}

// ========== RoomDescriptions ==========

size_t RoomDescriptions::add(const std::string &text)
{
	// Check if we already have this description
	auto it = _desc_map.find(text);
	if (it != _desc_map.end())
	{
		// Duplicate found -> return existing index
		return it->second;
	}
	else
	{
		// New description -> add to global index (1-based for compatibility with Legacy)
		_desc_list.push_back(text);
		size_t idx = _desc_list.size();  // 1-based! (0 = no description)
		_desc_map[text] = idx;
		return idx;
	}
}

const std::string &RoomDescriptions::get(size_t idx) const
{
	static const std::string empty_string = "";
	if (idx == 0)
	{
		return empty_string;  // 0 means "no description"
	}
	try
	{
		return _desc_list.at(idx - 1);  // Convert 1-based to 0-based
	}
	catch (const std::out_of_range &)
	{
		log("SYSERR: bad room description index %zd (size=%zd)", idx, _desc_list.size());
		return empty_string;
	}
}

std::vector<size_t> RoomDescriptions::merge(const LocalDescriptionIndex &local_index)
{
	// Create mapping from local description indices to global indices
	std::vector<size_t> local_to_global;
	local_to_global.reserve(local_index.size());

	for (size_t local_idx = 0; local_idx < local_index.size(); ++local_idx)
	{
		const std::string &desc = local_index.get(local_idx + 1);  // Convert 0-based loop to 1-based index
		size_t global_idx = add(desc);  // Deduplicates automatically!
		local_to_global.push_back(global_idx);
	}

	return local_to_global;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
