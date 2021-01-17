#include "external.trigger.hpp"

#include <filesystem>

ExternalTriggerChecker::ExternalTriggerChecker(const std::string& filename) : m_mtime(), m_filename(filename)
{
	init();
}

void ExternalTriggerChecker::init()
{
	m_mtime = get_mtime();
}

bool ExternalTriggerChecker::check()
{
	bool result = false;

	try
	{
		const auto mtime = get_mtime();
		if (m_mtime < mtime)
		{
			result = true;
			m_mtime = mtime;
		}
	}
	catch (const std::filesystem::filesystem_error&)
	{
	}

	return result;
}

std::filesystem::file_time_type ExternalTriggerChecker::get_mtime() const
{
    std::filesystem::file_time_type result;

	if (!m_filename.empty())
	{
		try
		{
			result = std::filesystem::last_write_time(m_filename.c_str());
		}
		catch (const std::error_code&)
		{
		}
	}

	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
