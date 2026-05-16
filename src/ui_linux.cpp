#include "ui_linux.hpp"
#include "actions.hpp"
#include "app.hpp"
#include "layout.hpp"
#include "ui_scene.hpp"
#include "ui_style.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>
#include <thread>

namespace ui {

namespace x11 {

struct DisplayHandle {
    Display* display = nullptr;
    explicit DisplayHandle(Display* d = nullptr) : display(d) {}
    ~DisplayHandle() {
        if (display) XCloseDisplay(display);
    }
    DisplayHandle(const DisplayHandle&) = delete;
    DisplayHandle& operator=(const DisplayHandle&) = delete;
};

static std::string current_time_text(ClockView view) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[24] = {};
    switch (view) {
    case ClockView::H24_HMS:
    case ClockView::Analog:
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
        break;
    case ClockView::H24_HM:
        std::strftime(buf, sizeof(buf), "%H:%M", &tm);
        break;
    case ClockView::H12_HMS:
        std::strftime(buf, sizeof(buf), "%I:%M:%S %p", &tm);
        break;
    case ClockView::H12_HM:
        std::strftime(buf, sizeof(buf), "%I:%M %p", &tm);
        break;
    }
    return buf;
}

static unsigned long pixel(Display* display, int screen, UiColor color) {
    Colormap cmap = DefaultColormap(display, screen);
    XColor xcolor{};
    xcolor.red = static_cast<unsigned short>(color.r * 257);
    xcolor.green = static_cast<unsigned short>(color.g * 257);
    xcolor.blue = static_cast<unsigned short>(color.b * 257);
    if (XAllocColor(display, cmap, &xcolor)) return xcolor.pixel;
    return BlackPixel(display, screen);
}

static int text_x(const ui_scene::Op& op) {
    constexpr int approx_char_w = 6;
    int text_w = (int)op.text.size() * approx_char_w;
    switch (op.align) {
    case ui_scene::Align::Center:
        return op.rect.left + std::max(0, ui_scene::width(op.rect) - text_w) / 2;
    case ui_scene::Align::Right:
        return op.rect.right - text_w;
    case ui_scene::Align::Left:
        return op.rect.left;
    }
    return op.rect.left;
}

static int text_y(const ui_scene::Op& op) {
    constexpr int approx_text_h = 12;
    return op.rect.top + std::max(approx_text_h, ui_scene::height(op.rect) + approx_text_h) / 2;
}

static void draw_scene(Display* display, Window window, GC gc, const ui_scene::Scene& scene) {
    int screen = DefaultScreen(display);
    for (const auto& op : scene.ops) {
        switch (op.kind) {
        case ui_scene::OpKind::FillRect:
        case ui_scene::OpKind::Progress:
            XSetForeground(display, gc, pixel(display, screen, op.fill));
            XFillRectangle(display, window, gc, op.rect.left, op.rect.top, (unsigned)ui_scene::width(op.rect),
                           (unsigned)ui_scene::height(op.rect));
            break;
        case ui_scene::OpKind::Divider:
            XSetForeground(display, gc, pixel(display, screen, op.stroke));
            XDrawLine(display, window, gc, op.rect.left, op.rect.top, op.rect.right, op.rect.top);
            break;
        case ui_scene::OpKind::Text:
            XSetForeground(display, gc, pixel(display, screen, op.text_color));
            XDrawString(display, window, gc, text_x(op), text_y(op), op.text.c_str(), (int)op.text.size());
            break;
        case ui_scene::OpKind::Button:
            XSetForeground(display, gc, pixel(display, screen, op.fill));
            XFillRectangle(display, window, gc, op.rect.left, op.rect.top, (unsigned)ui_scene::width(op.rect),
                           (unsigned)ui_scene::height(op.rect));
            XSetForeground(display, gc, pixel(display, screen, op.stroke));
            XDrawRectangle(display, window, gc, op.rect.left, op.rect.top, (unsigned)ui_scene::width(op.rect),
                           (unsigned)ui_scene::height(op.rect));
            XSetForeground(display, gc, pixel(display, screen, op.text_color));
            XDrawString(display, window, gc, text_x(op), text_y(op), op.text.c_str(), (int)op.text.size());
            break;
        }
    }
}

static ui_scene::MainSceneState scene_state(const App& app, std::chrono::steady_clock::time_point now) {
    return ui_scene::main_scene_state_from_app(app, now, current_time_text(app.clock_view));
}

static int scene_height(const Layout& layout, const App& app, std::chrono::steady_clock::time_point now) {
    return ui_scene::main_scene_height(layout, scene_state(app, now));
}

static ui_scene::Scene current_scene(int width, const App& app) {
    Layout layout;
    const ThemePalette& palette = palette_for(UiStyleConfig{.theme_mode = app.theme_mode, .system_dark = true});
    return ui_scene::build_main_scene(layout, width, scene_state(app, std::chrono::steady_clock::now()),
                                      make_ui(palette));
}

static void draw(Display* display, Window window, GC gc, int width, const App& app) {
    draw_scene(display, window, gc, current_scene(width, app));
    XFlush(display);
}

static int action_for_key(KeySym key, const App& app) {
    if (key >= XK_1 && key <= XK_9) {
        int idx = (int)(key - XK_1);
        if (idx < (int)app.timers.size()) return tmr_act(idx, A_TMR_START);
    }
    switch (key) {
    case XK_space:
        return app.show_sw ? A_SW_START : tmr_act(0, A_TMR_START);
    case XK_l:
    case XK_L:
        return A_SW_LAP;
    case XK_r:
    case XK_R:
        return app.show_sw ? A_SW_RESET : tmr_act(0, A_TMR_RST);
    case XK_plus:
    case XK_equal:
        return tmr_act(std::max(0, (int)app.timers.size() - 1), A_TMR_ADD);
    case XK_minus:
    case XK_underscore:
        return tmr_act(std::max(0, (int)app.timers.size() - 1), A_TMR_DEL);
    case XK_c:
    case XK_C:
        return A_CLK_CYCLE;
    case XK_t:
    case XK_T:
        return A_TOPMOST;
    case XK_d:
    case XK_D:
        return A_THEME;
    case XK_a:
    case XK_A:
        return A_SHOW_ALARMS;
    default:
        return 0;
    }
}

static void handle_action(Display* display, Window window, GC gc, int width, int& height, App& app, int act,
                          const std::filesystem::path& temp_dir) {
    if (act == 0) return;
    auto action_now = std::chrono::steady_clock::now();
    auto result = dispatch_action(app, act, action_now, temp_dir);
    if (result.resize) {
        Layout layout;
        height = scene_height(layout, app, action_now);
        XResizeWindow(display, window, (unsigned)width, (unsigned)height);
    }
    draw(display, window, gc, width, app);
}

} // namespace x11

int run_app(InstanceHandle, int) {
    x11::DisplayHandle display{XOpenDisplay(nullptr)};
    if (!display.display) return 1;

    std::error_code temp_ec;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path(temp_ec);
    if (temp_ec) temp_dir = std::filesystem::current_path();

    App app;
    Layout layout;
    auto now = std::chrono::steady_clock::now();
    int width = std::max(Config::MIN_WINDOW_W, layout.bar_min_client_w());
    int height = x11::scene_height(layout, app, now);
    int screen = DefaultScreen(display.display);
    Window root = RootWindow(display.display, screen);
    Window window = XCreateSimpleWindow(display.display, root, 100, 100, (unsigned)width, (unsigned)height, 1,
                                        BlackPixel(display.display, screen), WhitePixel(display.display, screen));
    XStoreName(display.display, window, "Chronos");
    XSelectInput(display.display, window, ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask);

    Atom wm_delete = XInternAtom(display.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display.display, window, &wm_delete, 1);

    GC gc = XCreateGC(display.display, window, 0, nullptr);
    XMapWindow(display.display, window);

    bool running = true;
    auto next_tick = std::chrono::steady_clock::now();
    while (running) {
        while (XPending(display.display) > 0) {
            XEvent ev{};
            XNextEvent(display.display, &ev);
            switch (ev.type) {
            case Expose:
                x11::draw(display.display, window, gc, width, app);
                break;
            case ConfigureNotify:
                width = ev.xconfigure.width;
                height = ev.xconfigure.height;
                (void)height;
                x11::draw(display.display, window, gc, width, app);
                break;
            case KeyPress: {
                KeySym key = XLookupKeysym(&ev.xkey, 0);
                if (key == XK_Escape || key == XK_q || key == XK_Q) {
                    running = false;
                    break;
                }
                x11::handle_action(display.display, window, gc, width, height, app, x11::action_for_key(key, app), temp_dir);
                break;
            }
            case ButtonPress: {
                auto scene = x11::current_scene(width, app);
                int act = ui_scene::hit_test(scene, ev.xbutton.x, ev.xbutton.y);
                x11::handle_action(display.display, window, gc, width, height, app, act, temp_dir);
                break;
            }
            case ClientMessage:
                if ((Atom)ev.xclient.data.l[0] == wm_delete) running = false;
                break;
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (now >= next_tick) {
            x11::draw(display.display, window, gc, width, app);
            next_tick = now + std::chrono::seconds{1};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }

    XFreeGC(display.display, gc);
    XDestroyWindow(display.display, window);
    return 0;
}

} // namespace ui
