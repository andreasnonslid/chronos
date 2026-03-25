module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
export module tray;

export constexpr UINT WM_TRAYICON = WM_APP + 1;
export constexpr UINT TRAY_UID = 1;
export constexpr int IDM_TRAY_SHOW = 1;
export constexpr int IDM_TRAY_EXIT = 2;

export void tray_add(HWND hwnd, HICON icon) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_UID;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = icon;
    wcsncpy(nid.szTip, L"Chronos", 127);
    nid.szTip[127] = L'\0';
    Shell_NotifyIconW(NIM_ADD, &nid);
}

export void tray_update_tip(HWND hwnd, const wchar_t* tip) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_UID;
    nid.uFlags = NIF_TIP;
    wcsncpy(nid.szTip, tip, 127);
    nid.szTip[127] = L'\0';
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

export void tray_notify(HWND hwnd, const wchar_t* title, const wchar_t* msg) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_UID;
    nid.uFlags = NIF_INFO;
    wcsncpy(nid.szInfoTitle, title, 63);
    nid.szInfoTitle[63] = L'\0';
    wcsncpy(nid.szInfo, msg, 255);
    nid.szInfo[255] = L'\0';
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

export void tray_remove(HWND hwnd) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_UID;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}
