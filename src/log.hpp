#ifndef LOG_HPP_INCLUDED
#define LOG_HPP_INCLUDED

#include <stdio.h>

class CLog
{
	FILE *file;

public:
	CLog(const char *path);
	~CLog();
	void Write(const char *format, ...);
};

extern CLog LuaLog;

#endif // LOG_HPP_INCLUDED
