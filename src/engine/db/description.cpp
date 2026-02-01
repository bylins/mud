// description.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "description.h"

#include "utils/logger.h"

// ========== LocalDescriptionIndex ==========

size_t LocalDescriptionIndex::add(const std::string &text)
{
	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	auto it = _desc_map.find(text);
	if (it != _desc_map.end())
	{
		// О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ -> О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
		return it->second;
	}
	else
	{
		// О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -> О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
		size_t idx = _desc_list.size();  // 0-based!
		_desc_list.push_back(text);
		_desc_map[text] = idx;
		return idx;
	}
}

const std::string &LocalDescriptionIndex::get(size_t idx) const
{
	static const std::string empty_string = "";
	try
	{
		return _desc_list.at(idx);
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
	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	auto it = _desc_map.find(text);
	if (it != _desc_map.end())
	{
		// О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ -> О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
		return it->second;
	}
	else
	{
		// О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -> О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
		size_t idx = _desc_list.size();  // 0-based!
		_desc_list.push_back(text);
		_desc_map[text] = idx;
		return idx;
	}
}

const std::string &RoomDescriptions::get(size_t idx) const
{
	static const std::string empty_string = "";
	try
	{
		return _desc_list.at(idx);  // О©╫О©╫О©╫О©╫О©╫О©╫ 0-based О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫!
	}
	catch (const std::out_of_range &)
	{
		log("SYSERR: bad room description index %zd (size=%zd)", idx, _desc_list.size());
		return empty_string;
	}
}

std::vector<size_t> RoomDescriptions::merge(const LocalDescriptionIndex &local_index)
{
	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -> О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	std::vector<size_t> local_to_global;
	local_to_global.reserve(local_index.size());

	for (size_t local_idx = 0; local_idx < local_index.size(); ++local_idx)
	{
		const std::string &desc = local_index.get(local_idx);
		size_t global_idx = add(desc);  // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫!
		local_to_global.push_back(global_idx);
	}

	return local_to_global;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

