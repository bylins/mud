#ifndef __BOOT_FATA_FILES_HPP__
#define __BOOT_FATA_FILES_HPP__

#include "boot.constants.hpp"

#include <memory>
#include <string>

class BaseDataFile
{
public:
	using shared_ptr = std::shared_ptr<BaseDataFile>;

	virtual ~BaseDataFile() {}

	virtual bool open() = 0;
	virtual bool load() = 0;
	virtual void close() = 0;
	virtual std::string full_file_name() const = 0;
};

class DataFileFactory
{
public:
    using shared_ptr = std::shared_ptr<DataFileFactory>;

    ~DataFileFactory() {}

    static shared_ptr create();

	virtual BaseDataFile::shared_ptr get_file(const EBootType mode, const std::string& file_name) = 0;
};

#endif	// __BOOT_FATA_FILES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :