#ifndef __EXTERNAL_TRIGGER_HPP__
#define __EXTERNAL_TRIGGER_HPP__

#include <string>

class ExternalTriggerChecker
{
public:
	ExternalTriggerChecker(const std::string& filename);

	void init();
	bool check();

private:
	std::size_t get_mtime() const;

	std::size_t m_mtime;
	std::string m_filename;
};

#endif // __EXTERNAL_TRIGGER_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
