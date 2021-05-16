#include "backtrace.h"
#include <iostream>
#include <cstdlib>
#include <climits>
#ifdef _WIN32
#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4091)
#include <DbgHelp.h>
#pragma warning(pop)
#elif __CYGWIN__
// если цигвин ничего не делаем
#else
#include <execinfo.h>
#endif
namespace debug
{
	constexpr unsigned short MAX_STACK_SIZE = USHRT_MAX;
#ifdef _WIN32
	void win32_backtrace(FILE* file)
	{
		constexpr size_t MAX_NAME_LENGTH = 1024u;
		const auto process = GetCurrentProcess();
		SymInitialize(process, nullptr, TRUE);
		void* stack[MAX_STACK_SIZE];
		const auto frames = CaptureStackBackTrace(0, MAX_STACK_SIZE, stack, nullptr);
		SYMBOL_INFO* symbol = (SYMBOL_INFO *) calloc(sizeof(SYMBOL_INFO) + MAX_NAME_LENGTH*sizeof(char), 1);
		symbol->MaxNameLen = MAX_NAME_LENGTH - 1;
		symbol->SizeOfStruct = sizeof(*symbol);
		for (auto i = 0; i < frames; ++i)
		{
			SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), 0, symbol);
			fprintf(file, "%02d: %s at 0x%08llx\n",
				frames - i - 1, symbol->Name, static_cast<unsigned long long>(symbol->Address));
		}
		free(symbol);
	}
#elif __CYGWIN__
// если цигвин ничего не делаем
#else
	void linux_backtrace(FILE* file)
	{
		void* stack[MAX_STACK_SIZE];
		const auto frames = ::backtrace(stack, MAX_STACK_SIZE);
		backtrace_symbols_fd(stack, frames, fileno(file));
	}
#endif
	void backtrace(FILE* file)
	{
		if (nullptr == file)
		{
			return;
		}
#ifdef _WIN32
		win32_backtrace(file);
#elif __CYGWIN__
// если цигвин ничего не делаем
#else
		linux_backtrace(file);
#endif
	}
}
