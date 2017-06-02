#include "backtrace.hpp"

#include <iostream>
#include <cstdlib>
#include <climits>

#ifdef _WIN32

#include <windows.h>

#pragma warning(push)
#pragma warning(disable:4091)
#include <DbgHelp.h>
#pragma warning(pop)

#else	// _WIN32

//#include <execinfo.h>

#endif

namespace debug
{
	constexpr unsigned short MAX_STACK_SIZE = USHRT_MAX;

	void backtrace(FILE* file)
	{
		return;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
