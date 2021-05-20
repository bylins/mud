#ifndef __EXTERNAL_TRIGGER_HPP__
#define __EXTERNAL_TRIGGER_HPP__

#include <string>
#include <filesystem>

class ExternalTriggerChecker
{
public:
	ExternalTriggerChecker(const std::string& filename);

	void init();
	bool check();

private:
    std::filesystem::file_time_type get_mtime() const;

    std::filesystem::file_time_type m_mtime;
	std::string m_filename;
};

#endif // __EXTERNAL_TRIGGER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
