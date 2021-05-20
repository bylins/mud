#ifndef __COREDUMP_HPP__
#define __COREDUMP_HPP__

#ifndef WIN32
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#endif

namespace debug
{
	inline void coredump()
	{
#ifndef WIN32
		pid_t pid = fork();
		if (0 == pid)
		{
			abort();
		}
		int result = 0;
		wait(&result);
#endif
	}
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
