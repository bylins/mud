#include "external.trigger.hpp"

#include <boost/filesystem/operations.hpp>

ExternalTriggerChecker::ExternalTriggerChecker(const std::string& filename) : m_mtime(0), m_filename(filename)
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
		const std::size_t mtime = get_mtime();
		if (m_mtime < mtime)
		{
			result = true;
			m_mtime = mtime;
		}
	}
	catch (const boost::filesystem::filesystem_error&)
	{
	}

	return result;
}

std::size_t ExternalTriggerChecker::get_mtime() const
{
	std::size_t result = 0;

	if (!m_filename.empty())
	{
		try
		{
			result = boost::filesystem::last_write_time(m_filename.c_str());
		}
		catch (const boost::system::error_code&)
		{
		}
	}

	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
