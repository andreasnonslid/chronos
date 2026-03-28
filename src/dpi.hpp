#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>

using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
using SetProcessDpiAwarenessContext_t = BOOL(WINAPI*)(HANDLE);

inline GetDpiForWindow_t pGetDpiForWindow = nullptr;
inline SetProcessDpiAwarenessContext_t pSetProcessDpiAwarenessContext = nullptr;
inline const HANDLE DPI_CTX_PER_MONITOR_V2 = ((HANDLE)(LONG_PTR)-4);

inline void load_dpi_apis() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        pGetDpiForWindow = (GetDpiForWindow_t)GetProcAddress(user32, "GetDpiForWindow");
        pSetProcessDpiAwarenessContext =
            (SetProcessDpiAwarenessContext_t)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    }
}
