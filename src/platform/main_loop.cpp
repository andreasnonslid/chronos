#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "actions.hpp"
#include "app.hpp"
#include "config_io.hpp"
#include "ui_style.hpp"
#include "ui_app.hpp"

// ─── Screenshot helper ────────────────────────────────────────────────────────

#ifdef CHRONOS_SCREENSHOT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#include "stb_image_write.h"
#pragma clang diagnostic pop

static bool save_screenshot(const char* path, int w, int h) {
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

// ─── Platform entry ───────────────────────────────────────────────────────────

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    const char* screenshot_path = nullptr;
    if (lpCmdLine && strstr(lpCmdLine, "--screenshot")) {
        const char* p = strstr(lpCmdLine, "--screenshot");
        p += strlen("--screenshot");
        while (*p == ' ') ++p;
        screenshot_path = p;
    }
#else
int main(int argc, char* argv[]) {
    const char* screenshot_path = nullptr;
    for (int i = 1; i < argc - 1; ++i) {
        if (strcmp(argv[i], "--screenshot") == 0)
            screenshot_path = argv[i + 1];
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

    Uint32 wflags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (screenshot_path) wflags |= SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow("Chronos", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          360, 520, wflags);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    App app;
    UiState ui;
    ui.cfg_path = config_path();
    load_config(app, ui.cfg_path);
    if (screenshot_path) ui.screenshot_path = screenshot_path;

    apply_imgui_theme(app.theme_mode, false);

    bool done = false;
    bool screenshot_done = false;
    int frame_count = 0;

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
                switch (event.key.keysym.sym) {
                case SDLK_SPACE: dispatch_action(app, A_SW_START, now, {}); break;
                case SDLK_l:     dispatch_action(app, A_SW_LAP,   now, {}); break;
                case SDLK_r:     dispatch_action(app, A_SW_RESET, now, {}); break;
                case SDLK_d:     dispatch_action(app, A_THEME,    now, {}); break;
                default: break;
                }
            }
        }

        // New frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        render_app(app, ui);

        ImGui::Render();

        int display_w, display_h;
        SDL_GetWindowSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        const auto& pal = palette_for(app.theme_mode, false);
        glClearColor(pal.bg.r / 255.f, pal.bg.g / 255.f, pal.bg.b / 255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        if (ui.dirty) {
            save_config(app, ui.cfg_path);
            ui.dirty = false;
        }

        // Screenshot mode: render 2 frames (1 to warm up, 1 to capture), then quit
        if (!ui.screenshot_path.empty()) {
            ++frame_count;
            if (frame_count >= 2 && !screenshot_done) {
#ifdef CHRONOS_SCREENSHOT
                save_screenshot(ui.screenshot_path.c_str(), display_w, display_h);
#endif
                screenshot_done = true;
                done = true;
            }
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
