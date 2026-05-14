#pragma once
#include <windows.h>
// Forward-declare DWM to avoid MinGW header chain issues (uxtheme.h missing commctrl.h).
extern "C"
    __declspec(dllimport) HRESULT __stdcall DwmSetWindowAttribute(HWND hwnd, DWORD attr, LPCVOID data, DWORD size);
extern "C"
    __declspec(dllimport) HRESULT __stdcall SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
