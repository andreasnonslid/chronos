#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cstdio>
#include <string>

inline FILE*& log_file_ref() { static FILE* f = nullptr; return f; }
inline void set_log_file(FILE* f) { log_file_ref() = f; }

inline void dbg(const std::wstring& msg) {
    OutputDebugStringW(msg.c_str());
    OutputDebugStringW(L"\n");
    if (auto* f = log_file_ref()) {
        fwprintf(f, L"%ls\n", msg.c_str());
        fflush(f);
    }
}
