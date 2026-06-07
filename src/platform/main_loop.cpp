#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include "config_io.hpp"
#include "ui_render.hpp"

// ─── Screenshot helper ────────────────────────────────────────────────────────

#ifdef CHRONOS_SCREENSHOT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#include "stb_image_write.h"
#pragma clang diagnostic pop

static bool save_screenshot(const char* path, int w, int h) {
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // prevent row padding when w*3 isn't 4-byte aligned
    std::vector<uint8_t> pixels(w * h * 3);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    // Flip vertically (OpenGL origin is bottom-left)
    for (int row = 0; row < h / 2; ++row) {
        uint8_t* a = pixels.data() + row * w * 3;
        uint8_t* b = pixels.data() + (h - 1 - row) * w * 3;
        for (int col = 0; col < w * 3; ++col) std::swap(a[col], b[col]);
    }
    return stbi_write_png(path, w, h, 3, pixels.data(), w * 3) != 0;
}
#endif

static void render_frame(SDL_Window* window, SDL_GLContext gl_ctx, App& app, UiState& ui) {
    SDL_GL_MakeCurrent(window, gl_ctx);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    render_app(app, ui);

    ImGui::Render();

    int display_w = 0;
    int display_h = 0;
    SDL_GL_GetDrawableSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    const auto& pal = palette_for(app.theme_mode, false);
    glClearColor(pal.bg.r / 255.f, pal.bg.g / 255.f, pal.bg.b / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

static bool process_due_notifications(SDL_Window* window, App& app) {
    bool dirty = false;
    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < (int)app.timers.size(); ++i) {
        auto& ts = app.timers[i];
        if (!ts.t.touched() || !ts.t.expired(now) || ts.notified) continue;

        ts.notified = true;
        std::string title = app.timers.size() <= 1
            ? "Timer expired"
            : std::format("Timer {} expired", i + 1);
        std::string body = ts.label.empty() ? "Time is up." : wide_to_utf8(ts.label);
        platform_notify(window, title.c_str(), body.c_str());
        if (app.sound_on_expiry) platform_beep();

        if (ts.pomodoro) {
            advance_pomodoro_phase(ts, app, now);
            dirty = true;
        }
    }

    tm lt = local_tm(std::time(nullptr));
    int h = lt.tm_hour, m = lt.tm_min;
    int cur_min = h * 60 + m;
    if (cur_min != app.alarm_notified_minute) {
        app.alarm_notified_minute = cur_min;
        for (auto& a : app.alarms) a.notified = false;
    }
    for (auto& a : app.alarms) {
        if (a.notified || !a.enabled) continue;
        if (!alarm_matches(a, h, m, lt.tm_wday, lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday)) continue;
        a.notified = true;
        std::string body = a.name.empty()
            ? std::format("{:02}:{:02}", a.hour, a.minute)
            : std::format("{} ({:02}:{:02})", a.name, a.hour, a.minute);
        platform_notify(window, "Alarm", body.c_str());
        if (app.sound_on_expiry) platform_beep();
    }

    return dirty;
}

// ─── Platform entry ───────────────────────────────────────────────────────────

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    const char* screenshot_path = nullptr;
    const char* config_override = nullptr;
    std::vector<int> replay_actions;
    int forced_settings_tab = -1;
    bool strip_open_on_start = false;
    if (lpCmdLine && strstr(lpCmdLine, "--screenshot")) {
        const char* p = strstr(lpCmdLine, "--screenshot");
        p += strlen("--screenshot");
        while (*p == ' ') ++p;
        screenshot_path = p;
    }
#else
int main(int argc, char* argv[]) {
    const char* screenshot_path = nullptr;
    const char* config_override = nullptr;
    std::vector<int> replay_actions;
    int forced_settings_tab = -1;
    bool strip_open_on_start = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc)
            screenshot_path = argv[i + 1];
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
            config_override = argv[i + 1];
        if (strcmp(argv[i], "--settings-tab") == 0 && i + 1 < argc)
            forced_settings_tab = (int)strtol(argv[i + 1], nullptr, 10);
        if (strcmp(argv[i], "--strip-open") == 0)
            strip_open_on_start = true;
        if (strcmp(argv[i], "--actions") == 0 && i + 1 < argc) {
            const char* s = argv[i + 1];
            while (*s) {
                char* end;
                int code = (int)strtol(s, &end, 10);
                if (end != s) { replay_actions.push_back(code); s = end; }
                else ++s;  // no digit found — skip the bad char to avoid infinite loop
                if (*s == ',') ++s;
            }
        }
    }
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // SDL_WINDOW_HIDDEN prevents the GL surface from fully initialising on some
    // software renderers (Mesa/Xvfb), leaving the framebuffer uncleared. Always
    // show the window; Xvfb makes it invisible anyway.
    // BORDERLESS: removes OS title bar; we draw our own strip in ImGui.
    Uint32 wflags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;
#ifdef CHRONOS_DEBUG_UI_OVERLAY
    constexpr float debug_default_scale = 1.65f;
    constexpr int initial_window_w = (int)(360 * debug_default_scale + 0.5f);
    constexpr int initial_window_h = (int)(520 * debug_default_scale + 0.5f);
    constexpr int min_window_w = (int)(240 * debug_default_scale + 0.5f);
    constexpr int min_window_h = (int)(180 * debug_default_scale + 0.5f);
#else
    constexpr int initial_window_w = 360;
    constexpr int initial_window_h = 520;
    constexpr int min_window_w = 240;
    constexpr int min_window_h = 180;
#endif
    SDL_Window* window = SDL_CreateWindow("Chronos", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          initial_window_w, initial_window_h, wflags);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1);  // vsync

    SDL_SetWindowMinimumSize(window, min_window_w, min_window_h);
    platform_window_init(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Load ProggyClean as the base font (explicit size so codicons can merge into it)
    io.Fonts->AddFontFromFileTTF(CHRONOS_PROGGYCLEAN_TTF, 13.f);
    {
        ImFontConfig cfg;
        cfg.MergeMode   = true;
        cfg.GlyphOffset = {0.f, 1.f};  // slight drop to optically centre icons
        static const ImWchar icon_ranges[] = { 0xEA00, 0xECFF, 0 };
        io.Fonts->AddFontFromFileTTF(CHRONOS_CODICON_TTF, 13.f, &cfg, icon_ranges);
    }

    ImGui_ImplSDL2_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    App app;
    UiState ui;
    ui.cfg_path    = config_override ? std::filesystem::path{config_override} : config_path();
    ui.sdl_window  = window;
    load_config(app, ui.cfg_path);
    platform_set_always_on_top(window, app.topmost);
    if (screenshot_path) ui.screenshot_path = screenshot_path;

    apply_imgui_theme(app.theme_mode, false);

    // Pre-dispatch actions (for headless UI testing — simulates button-click sequences)
    if (!replay_actions.empty() || forced_settings_tab >= 0) {
        auto now = std::chrono::steady_clock::now();
        for (int act : replay_actions) {
            auto r = dispatch_action(app, act, now, {});
            if (r.open_settings)     ui.show_settings = true;
            if (r.apply_theme)       apply_imgui_theme(app.theme_mode, false);
            if (r.open_alarm_dialog) {
                memset(ui.alarm_name, 0, sizeof(ui.alarm_name));
                ui.alarm_hour = 8; ui.alarm_minute = 0;
                ui.alarm_days_mode = true;
                for (int d = 0; d < 7; ++d) ui.alarm_days[d] = true;
                tm lt = local_tm(std::time(nullptr));
                ui.alarm_year  = lt.tm_year + 1900;
                ui.alarm_month = lt.tm_mon + 1;
                ui.alarm_day   = lt.tm_mday;
                ui.show_add_alarm = true;
            }
        }
        if (forced_settings_tab >= 0) {
            ui.show_settings  = true;
            ui.settings_tab   = forced_settings_tab;
        }
    }
    if (strip_open_on_start) {
        // Force the strip open and pin it so click_outside cannot dismiss it.
        // Used by --strip-open screenshot mode to verify the strip renders correctly.
        // The toolbar_strip_btn_update / toolbar_strip_click_outside logic is
        // covered by the unit tests in tests/test_toolbar_strip.cpp.
        ui.toolbar_strip_open   = true;
        ui.toolbar_strip_pinned = true;
    }

    bool done = false;
    bool screenshot_done = false;
    int frame_count = 0;
    bool rendering = false;
    bool topmost_reapplied_after_first_frame = false;

    struct RenderCtx { SDL_Window* win; SDL_GLContext gl; App* app; UiState* ui; bool* rendering; };
    RenderCtx rctx{window, gl_ctx, &app, &ui, &rendering};
    auto resize_event_watch = [](void* ud, SDL_Event* ev) -> int {
        if (ev->type != SDL_WINDOWEVENT) return 0;
        if (ev->window.event != SDL_WINDOWEVENT_SIZE_CHANGED &&
            ev->window.event != SDL_WINDOWEVENT_RESIZED &&
            ev->window.event != SDL_WINDOWEVENT_EXPOSED) {
            return 0;
        }
        auto* c = static_cast<RenderCtx*>(ud);
        if (*c->rendering) return 0;
        *c->rendering = true;
        render_frame(c->win, c->gl, *c->app, *c->ui);
        *c->rendering = false;
        return 0;
    };
    SDL_AddEventWatch(resize_event_watch, &rctx);

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
            // Keyboard shortcuts
            if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
                auto now = std::chrono::steady_clock::now();
                auto kb_action = [&](int action) {
                    auto r = dispatch_action(app, action, now, {});
                    if (r.save_config)  ui.dirty = true;
                    if (r.apply_theme)  apply_imgui_theme(app.theme_mode, false);
                    if (r.set_topmost)  platform_set_always_on_top(window, app.topmost);
                };
                switch (event.key.keysym.sym) {
                case SDLK_SPACE: kb_action(A_SW_START); break;
                case SDLK_l:     kb_action(A_SW_LAP);   break;
                case SDLK_r:     kb_action(A_SW_RESET); break;
                case SDLK_d:     kb_action(A_THEME);    break;
                case SDLK_EQUALS: case SDLK_KP_PLUS:
                    if (event.key.keysym.mod & KMOD_CTRL) adjust_ui_scale(ui, +0.1f);
                    break;
                case SDLK_MINUS: case SDLK_KP_MINUS:
                    if (event.key.keysym.mod & KMOD_CTRL) adjust_ui_scale(ui, -0.1f);
                    break;
                case SDLK_0: case SDLK_KP_0:
                    if (event.key.keysym.mod & KMOD_CTRL) adjust_ui_scale(ui, 1.f - ui.ui_scale);
                    break;
                default: break;
                }
            }
        }

        if (process_due_notifications(window, app)) ui.dirty = true;

        if (ui.minimize_to_tray_requested) {
            platform_minimize_to_tray(window);
            ui.minimize_to_tray_requested = false;
        }

        Uint32 window_flags = SDL_GetWindowFlags(window);
        bool window_visible = (window_flags & SDL_WINDOW_SHOWN) != 0 &&
                              (window_flags & SDL_WINDOW_MINIMIZED) == 0;
        if (!window_visible) {
            if (ui.dirty) {
                save_config(app, ui.cfg_path);
                ui.dirty = false;
            }
            SDL_Delay(250);
            continue;
        }

        rendering = true;

        // New frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        render_app(app, ui);

        ImGui::Render();

        int display_w, display_h;
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        const auto& pal = palette_for(app.theme_mode, false);
        glClearColor(pal.bg.r / 255.f, pal.bg.g / 255.f, pal.bg.b / 255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Screenshot: capture before SwapWindow while back buffer holds current frame.
        // glReadPixels reads the back buffer; after swap it becomes undefined on many
        // software renderers (Mesa zeroes it), so capture must happen here.
        if (!ui.screenshot_path.empty()) {
            ++frame_count;
            if (frame_count >= 2 && !screenshot_done) {
#ifdef CHRONOS_SCREENSHOT
                save_screenshot(ui.screenshot_path.c_str(), display_w, display_h);
                screenshot_done = true;
                done = true;
#else
                fprintf(stderr, "--screenshot requires a CHRONOS_SCREENSHOT build\n");
                done = true;
#endif
            }
        }

        SDL_GL_SwapWindow(window);
        rendering = false;

        if (!topmost_reapplied_after_first_frame) {
            platform_set_always_on_top(window, app.topmost);
            topmost_reapplied_after_first_frame = true;
        }

        if (ui.close_requested) done = true;

        if (ui.dirty) {
            save_config(app, ui.cfg_path);
            ui.dirty = false;
        }
    }

    SDL_DelEventWatch(resize_event_watch, &rctx);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    platform_window_shutdown(window);
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
