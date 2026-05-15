#pragma once
#include "app.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#ifdef None
#undef None
#endif

namespace ui {

using WindowHandle   = Window;
using FontHandle     = XFontStruct*;
using ThemeHandle    = const void*;

struct InstanceHandle {
    int argc = 0;
    char** argv = nullptr;
};

inline bool show_settings_dialog(WindowHandle, App&, FontHandle,
                                 ThemeHandle = nullptr, int = 0) {
    return false;
}

int run_app(InstanceHandle inst, int nShow);

} // namespace ui
