#pragma once
#include <windows.h>

using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
using SetProcessDpiAwarenessContext_t = BOOL(WINAPI*)(HANDLE);

// Lazily-loaded DPI API function pointers (Win8.1+ / Win10+).
// Call g_dpi.load() once at startup before using any function pointer.
struct Dpi {
    GetDpiForWindow_t GetDpiForWindow = nullptr;
    SetProcessDpiAwarenessContext_t SetProcessDpiAwarenessContext = nullptr;

    void load() {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            GetDpiForWindow = reinterpret_cast<GetDpiForWindow_t>(
                reinterpret_cast<void*>(GetProcAddress(user32, "GetDpiForWindow")));
            SetProcessDpiAwarenessContext = reinterpret_cast<SetProcessDpiAwarenessContext_t>(
                reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")));
        }
    }
};

inline Dpi g_dpi;
inline const HANDLE DPI_CTX_PER_MONITOR_V2 = ((HANDLE)(LONG_PTR)-4);
