#include "log.hpp"

#include <time.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#	include <direct.h>
#else
#	include <sys/stat.h>
#endif

CLog LuaLog("logs/lua.txt");

CLog::CLog(const char *path)
{
	file = NULL;
	if (path == NULL)
		return;
	// Создание всех указанных в пути директорий.
	for (int i = 0; path[i] != '\0'; i++)
	{
		if (path[i] == '\\' || path[i] == '/')
		{
			char dir[256];
			strncpy(dir, path, i);
			dir[i] = '\0';
#ifdef _WIN32
			_mkdir(dir);
#else
			mkdir(dir, 0700);
#endif
		}
	}
	file = fopen(path, "a");
}

CLog::~CLog()
{
	if (file != NULL)
		fclose(file);
}

void CLog::Write(const char *format, ...)
{
	if (file == NULL)
		return;
	time_t currTime = time(NULL);
	char *timeStr = asctime(localtime(&currTime));
	timeStr[strlen(timeStr) - 1] = '\0';
	fprintf(file, "%-15.15s :: ", timeStr + 4);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
	fflush(file);
}
