#include "boards.constants.hpp"

namespace Boards
{
	namespace constants
	{
		const char *OVERFLOW_MESSAGE = "Да, набросали всего столько, что файл переполнен. Напомните об этом богам!\r\n";
		const char* CHANGELOG_FILE_NAME = "../build/changelog";

		namespace loader_formats
		{
			const char* GIT = "git";
		}
	}
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
