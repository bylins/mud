// description.h
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef _DESCRIPTION_H_INCLUDED
#define _DESCRIPTION_H_INCLUDED

#include "engine/core/conf.h"
#include <string>
#include <vector>
#include <map>

/**
 * LocalDescriptionIndex - Thread-local description index for parallel room loading.
 * Used by worker threads to collect room descriptions without race conditions.
 */
class LocalDescriptionIndex {
public:
	LocalDescriptionIndex() = default;
	~LocalDescriptionIndex() = default;

	// Add description to local index. Returns 0-based index.
	size_t add(const std::string &text);

	// Get description by 0-based index.
	const std::string &get(size_t idx) const;

	// Number of descriptions in index.
	size_t size() const { return _desc_list.size(); }

	// Direct access to descriptions (for merge).
	const std::vector<std::string> &descriptions() const { return _desc_list; }

private:
	std::vector<std::string> _desc_list;
	std::map<std::string, size_t> _desc_map;
};

/**
 * RoomDescriptions - Global room description storage.
 * Maintains unique descriptions in GlobalObjects to save memory.
 *
 * Saves ~50% memory on room descriptions (>70K rooms in full MUD).
 * Each room stores only description index (0-based) instead of full text.
 */
class RoomDescriptions {
public:
	RoomDescriptions() = default;
	~RoomDescriptions() = default;

	// Add description to global index. Returns 0-based index.
	// If description already exists, returns existing index.
	size_t add(const std::string &text);

	// Get description by 0-based index.
	const std::string &get(size_t idx) const;

	// Number of descriptions in index.
	size_t size() const { return _desc_list.size(); }

	// Merge thread-local descriptions into global index.
	// Returns mapping from local indices to global indices.
	std::vector<size_t> merge(const LocalDescriptionIndex &local_index);

private:
	std::vector<std::string> _desc_list;
	std::map<std::string, size_t> _desc_map;
};

#endif // _DESCRIPTION_H_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
