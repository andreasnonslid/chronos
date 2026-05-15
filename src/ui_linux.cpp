#include "ui_linux.hpp"
#include <chrono>
#include <ctime>
#include <string>
#include <thread>

namespace ui {

namespace x11 {

struct DisplayHandle {
    Display* display = nullptr;
    explicit DisplayHandle(Display* d = nullptr) : display(d) {}
    ~DisplayHandle() { if (display) XCloseDisplay(display); }
    DisplayHandle(const DisplayHandle&) = delete;
    DisplayHandle& operator=(const DisplayHandle&) = delete;
};

static std::string current_time_text() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[16] = {};
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return buf;
}

static void draw(Display* display, Window window, GC gc, int width, int height) {
    int screen = DefaultScreen(display);
    unsigned long bg = WhitePixel(display, screen);
    unsigned long fg = BlackPixel(display, screen);
    XSetForeground(display, gc, bg);
    XFillRectangle(display, window, gc, 0, 0, (unsigned)width, (unsigned)height);

    XSetForeground(display, gc, fg);
    std::string title = "Chronos";
    std::string time = current_time_text();
    std::string hint = "Experimental X11 backend";
    XDrawString(display, window, gc, 20, 34, title.c_str(), (int)title.size());
    XDrawString(display, window, gc, 20, height / 2, time.c_str(), (int)time.size());
    XDrawString(display, window, gc, 20, height - 24, hint.c_str(), (int)hint.size());
    XFlush(display);
}

} // namespace x11

int run_app(InstanceHandle, int) {
    x11::DisplayHandle display{XOpenDisplay(nullptr)};
    if (!display.display) return 1;

    int screen = DefaultScreen(display.display);
    int width = 360;
    int height = 160;
    Window root = RootWindow(display.display, screen);
    Window window = XCreateSimpleWindow(display.display, root, 100, 100, (unsigned)width, (unsigned)height, 1,
                                        BlackPixel(display.display, screen), WhitePixel(display.display, screen));
    XStoreName(display.display, window, "Chronos");
    XSelectInput(display.display, window, ExposureMask | StructureNotifyMask | KeyPressMask);

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
                x11::draw(display.display, window, gc, width, height);
                break;
            case ConfigureNotify:
                width = ev.xconfigure.width;
                height = ev.xconfigure.height;
                x11::draw(display.display, window, gc, width, height);
                break;
            case KeyPress: {
                KeySym key = XLookupKeysym(&ev.xkey, 0);
                if (key == XK_Escape || key == XK_q || key == XK_Q) running = false;
                break;
            }
            case ClientMessage:
                if ((Atom)ev.xclient.data.l[0] == wm_delete) running = false;
                break;
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (now >= next_tick) {
            x11::draw(display.display, window, gc, width, height);
            next_tick = now + std::chrono::seconds{1};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }

    XFreeGC(display.display, gc);
    XDestroyWindow(display.display, window);
    return 0;
}

} // namespace ui
