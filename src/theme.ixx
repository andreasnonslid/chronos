module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
export module theme;

export constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_ATTR = 20;

export bool system_prefers_dark() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD val = 0, size = sizeof(val);
        bool ok = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&val, &size) == ERROR_SUCCESS;
        RegCloseKey(key);
        if (ok) return val == 0;
    }
    return true;
}
