#include "ui_windows.hpp"
#include "debug.hpp"
#include "dpi.hpp"
#include "icon.hpp"
#include "layout.hpp"
#include "window.hpp"
#include "wndstate.hpp"
#include <cstdio>
#include <filesystem>
#include <cwchar>

namespace ui {

namespace {

struct FileCloser {
    FILE* f = nullptr;
    ~FileCloser() {
        if (f) {
            fprintf(f, "=== chronos exit ===\n");
            fclose(f);
        }
    }
};

struct MutexGuard {
    HANDLE h = nullptr;
    ~MutexGuard() { if (h) CloseHandle(h); }
};

} // namespace

int run_app(InstanceHandle hInst, int nShow) {
    MutexGuard mutex{CreateMutexW(nullptr, TRUE, L"ChronosAppMutex")};
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"ChronoApp", nullptr);
        if (existing) {
            ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
        }
        return 0;
    }

    FileCloser log_guard;
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            if (wcscmp(argv[i], L"--debug") == 0) {
                wchar_t exe[MAX_PATH] = {};
                std::filesystem::path log_path;
                if (GetModuleFileNameW(nullptr, exe, MAX_PATH))
                    log_path = std::filesystem::path{exe}.parent_path() / "debug.log";
                else
                    log_path = std::filesystem::path{L"debug.log"};
                FILE* log_f = _wfopen(log_path.c_str(), L"a");
                log_guard.f = log_f;
                set_log_file(log_f);
                if (log_f) fprintf(log_f, "\n=== chronos start ===\n");
            }
        }
        LocalFree(argv);
    }

    g_dpi.load();
    if (g_dpi.SetProcessDpiAwarenessContext) g_dpi.SetProcessDpiAwarenessContext(DPI_CTX_PER_MONITOR_V2);

    Layout init_layout;
    HDC dc = GetDC(nullptr);
    init_layout.update_for_dpi(dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96);
    if (dc) ReleaseDC(nullptr, dc);

    IconObj icon_sm{create_app_icon(16)};
    IconObj icon_lg{create_app_icon(32)};

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = icon_lg;
    wc.hIconSm = icon_sm;
    wc.lpszClassName = L"ChronoApp";
    if (!RegisterClassExW(&wc)) return 1;

    constexpr DWORD ws = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
    int init_h = init_layout.bar_h + init_layout.clk_h + init_layout.sw_h + init_layout.tmr_h;
    RECT wr{0, 0, init_layout.bar_min_client_w(), init_h};
    AdjustWindowRect(&wr, ws, FALSE);

    HWND hwnd = CreateWindowExW(0, L"ChronoApp", L"Chronos", ws, CW_USEDEFAULT, CW_USEDEFAULT,
                                wr.right - wr.left, wr.bottom - wr.top,
                                nullptr, nullptr, hInst, &init_layout);
    if (!hwnd) return 1;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg{};
    int msg_ret = 0;
    while ((msg_ret = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (msg_ret < 0) {
        dbg(L"GetMessageW failed in main message loop");
        return 1;
    }
    return (int)msg.wParam;
}

} // namespace ui
