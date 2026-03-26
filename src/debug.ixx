module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cstdio>
#include <string>
export module debug;

static FILE* g_log_file = nullptr;
export void set_log_file(FILE* f) { g_log_file = f; }

export void dbg(const std::wstring& msg) {
    OutputDebugStringW(msg.c_str());
    OutputDebugStringW(L"\n");
    if (g_log_file) {
        fwprintf(g_log_file, L"%ls\n", msg.c_str());
        fflush(g_log_file);
    }
}
