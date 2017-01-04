#ifndef __BOOT_INDEX_HPP__
#define __BOOT_INDEX_HPP__

#include "db.h"

#include <list>
#include <unordered_map>
#include <memory>

class IndexFile : private std::list<std::string>
{
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

class IndexFileFactory
{
public:
	static IndexFile::shared_ptr get_index(const EBootType mode);
};

#endif // __BOOT_INDEX_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
