#include "backtrace.h"

#include <climits>
#ifdef _WIN32
#include <windows.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4091)
#endif
#include <DbgHelp.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#elif __CYGWIN__
// если цигвин ничего не делаем
#elif defined(__GLIBC__)
#include <execinfo.h>
#endif

namespace debug {
constexpr unsigned short MAX_STACK_SIZE = USHRT_MAX;

#ifdef _WIN32
void win32_backtrace(FILE* file)
{
    const auto process = GetCurrentProcess();
    SymInitialize(process, nullptr, true);
    void* stack[MAX_STACK_SIZE];
    const auto frames = CaptureStackBackTrace(0, MAX_STACK_SIZE, stack, nullptr);
    SYMBOL_INFO* symbol = (SYMBOL_INFO *) calloc(sizeof(SYMBOL_INFO) + 1024u*sizeof(char), 1);
    symbol->MaxNameLen = 1024 - 1;
    symbol->SizeOfStruct = sizeof(*symbol);
    for (auto i = 0; i < frames; ++i)
    {
        SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), 0, symbol);
        fprintf(file, "%02d: %s at 0x%08llx\n",
            frames - i - 1, symbol->Name, static_cast<unsigned long long>(symbol->Address));
        printf("%02d: %s at 0x%08llx\n",
            frames - i - 1, symbol->Name, static_cast<unsigned long long>(symbol->Address));
    }
    free(symbol);
}
#elif __CYGWIN__
void linux_backtrace(FILE *file) {}
#elif defined(__GLIBC__)
void linux_backtrace(FILE *file) {
    void *stack[MAX_STACK_SIZE];
    const auto frames = ::backtrace(stack, MAX_STACK_SIZE);
    backtrace_symbols_fd(stack, frames, fileno(file));
}
#else
// заглушка для musl или других систем без execinfo.h
void linux_backtrace(FILE *file) {}
#endif

void backtrace(FILE *file) {
    if (nullptr == file) {
        return;
    }
#ifdef _WIN32
    win32_backtrace(file);
#elif __CYGWIN__
    // если цигвин ничего не делаем
#elif defined(__GLIBC__)
    linux_backtrace(file);
#else
    linux_backtrace(file);
#endif
}
}