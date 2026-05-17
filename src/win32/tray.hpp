#pragma once
#include <windows.h>
#include <shellapi.h>

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT TRAY_UID = 1;
constexpr int IDM_TRAY_SHOW = 1;
constexpr int IDM_TRAY_EXIT = 2;

inline NOTIFYICONDATAW make_nid(HWND hwnd, UINT flags = 0) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_UID;
    nid.uFlags = flags;
    return nid;
}

template <size_t N>
inline void safe_copy(wchar_t (&dst)[N], const wchar_t* src) {
    wcsncpy(dst, src, N - 1);
    dst[N - 1] = L'\0';
}

inline void tray_add(HWND hwnd, HICON icon) {
    auto nid = make_nid(hwnd, NIF_ICON | NIF_TIP | NIF_MESSAGE);
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = icon;
    safe_copy(nid.szTip, L"Chronos");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

inline void tray_update_tip(HWND hwnd, const wchar_t* tip) {
    auto nid = make_nid(hwnd, NIF_TIP);
    safe_copy(nid.szTip, tip);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

inline void tray_notify(HWND hwnd, const wchar_t* title, const wchar_t* msg) {
    auto nid = make_nid(hwnd, NIF_INFO);
    safe_copy(nid.szInfoTitle, title);
    safe_copy(nid.szInfo, msg);
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

inline void tray_remove(HWND hwnd) {
    auto nid = make_nid(hwnd);
    Shell_NotifyIconW(NIM_DELETE, &nid);
}
