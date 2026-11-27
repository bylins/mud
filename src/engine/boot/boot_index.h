#ifndef __BOOT_INDEX_HPP__
#define __BOOT_INDEX_HPP__

#include "engine/db/db.h"
#include "legacy_files.h"

#include <list>
#include <unordered_map>
#include <memory>

class IndexFile : private std::list<std::string> {
 public:
	using shared_ptr = std::shared_ptr<IndexFile>;

	using base_t = std::list<std::string>;
	using base_t::begin;
	using base_t::end;

	virtual ~IndexFile() {}

	virtual bool open() = 0;
	virtual int load() = 0;

 protected:
	using base_t::clear;
	using base_t::push_back;
};

class IndexFileFactory {
 public:
	// Индекс типа mode; пути -- из files, которые должны пережить возвращённый объект.
	static IndexFile::shared_ptr get_index(const EBootType mode, const LegacyFiles &files);
};

#endif // __BOOT_INDEX_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
