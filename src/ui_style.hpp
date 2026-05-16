#pragma once
#include <cstdint>
#include <optional>

#include "config.hpp"

struct UiColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

constexpr UiColor rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) { return UiColor{r, g, b}; }

struct ThemePalette {
    UiColor bg;
    UiColor bar;
    UiColor btn;
    UiColor active;
    UiColor text;
    UiColor dim;
    UiColor warn;
    UiColor expire;
    UiColor blink;
    UiColor fill;
    UiColor fill_exp;
    UiColor help_bg;
    UiColor divider;
};

constexpr ThemePalette dark_palette{
    .bg = rgb(26, 26, 26),
    .bar = rgb(35, 35, 38),
    .btn = rgb(40, 40, 44),
    .active = rgb(80, 80, 88),
    .text = rgb(204, 204, 204),
    .dim = rgb(90, 90, 90),
    .warn = rgb(240, 140, 30),
    .expire = rgb(200, 50, 50),
    .blink = rgb(110, 110, 118),
    .fill = rgb(38, 38, 50),
    .fill_exp = rgb(72, 18, 18),
    .help_bg = rgb(20, 20, 20),
    .divider = rgb(50, 50, 55),
};

constexpr ThemePalette light_palette{
    .bg = rgb(243, 243, 243),
    .bar = rgb(228, 228, 232),
    .btn = rgb(214, 214, 220),
    .active = rgb(175, 175, 188),
    .text = rgb(28, 28, 28),
    .dim = rgb(138, 138, 138),
    .warn = rgb(196, 90, 0),
    .expire = rgb(196, 36, 36),
    .blink = rgb(168, 168, 178),
    .fill = rgb(208, 208, 230),
    .fill_exp = rgb(240, 196, 196),
    .help_bg = rgb(232, 232, 232),
    .divider = rgb(198, 198, 208),
};

constexpr const ThemePalette& palette_for(ThemeMode mode, bool system_dark) {
    return mode == ThemeMode::Dark || (mode == ThemeMode::Auto && system_dark) ? dark_palette : light_palette;
}

struct UiStyleConfig {
    ThemeMode theme_mode = ThemeMode::Auto;
    bool system_dark = true;
};

constexpr const ThemePalette& palette_for(const UiStyleConfig& config) {
    return palette_for(config.theme_mode, config.system_dark);
}

struct SurfaceConfig {
    bool elevated = false;
};

struct DialogConfig {
    bool active = true;
    bool scrollable = false;
};

struct ScrollViewConfig {
    bool enabled = false;
};

struct ButtonConfig {
    bool active = false;
    bool focused = false;
    bool blinking = false;
    std::optional<UiColor> fill_override = std::nullopt;
    int radius_px = 6;
};

struct DropdownConfig {
    bool open = false;
    bool focused = false;
    int radius_px = 4;
};

struct ToggleConfig {
    bool on = false;
    bool focused = false;
    int radius_px = 4;
};

struct ProgressConfig {
    bool expired = false;
};

struct AnalogClockConfig {
    bool selected = false;
};

struct IconConfig {
    bool active = false;
};

enum class TextTone { Primary, Dim, Warning, Danger };

struct TextConfig {
    TextTone tone = TextTone::Primary;
};

struct SurfacePaint {
    UiColor background;
    UiColor border;
};

struct DialogPaint {
    UiColor background;
    UiColor title_text;
    UiColor divider;
    UiColor content_background;
};

struct ScrollViewPaint {
    UiColor track;
    UiColor thumb;
    UiColor divider;
};

struct WidgetPaint {
    UiColor fill;
    UiColor text;
    UiColor border;
    int radius_px;
};

struct TextPaint {
    UiColor color;
};

struct DividerPaint {
    UiColor color;
};

struct ProgressPaint {
    UiColor fill;
};

struct AnalogClockPaint {
    UiColor background;
    UiColor face;
    UiColor outline;
    UiColor tick;
    UiColor text;
    UiColor accent;
};

struct IconPaint {
    UiColor background;
    UiColor foreground;
    UiColor accent;
};

struct UiMakers {
    ThemePalette palette;

    constexpr SurfacePaint surface(SurfaceConfig config = {}) const {
        return SurfacePaint{
            .background = config.elevated ? palette.bar : palette.bg,
            .border = palette.divider,
        };
    }

    constexpr DialogPaint dialog(DialogConfig = {}) const {
        return DialogPaint{
            .background = palette.bg,
            .title_text = palette.text,
            .divider = palette.divider,
            .content_background = palette.bg,
        };
    }

    constexpr ScrollViewPaint scroll_view(ScrollViewConfig config = {}) const {
        return ScrollViewPaint{
            .track = palette.bar,
            .thumb = config.enabled ? palette.active : palette.dim,
            .divider = palette.divider,
        };
    }

    constexpr WidgetPaint button(ButtonConfig config = {}) const {
        return WidgetPaint{
            .fill = config.blinking        ? palette.blink
                    : config.fill_override ? *config.fill_override
                    : config.active        ? palette.active
                                           : palette.btn,
            .text = palette.text,
            .border = config.focused ? palette.text : palette.divider,
            .radius_px = config.radius_px,
        };
    }

    constexpr WidgetPaint dropdown(DropdownConfig config = {}) const {
        return WidgetPaint{
            .fill = config.open ? palette.active : palette.btn,
            .text = palette.text,
            .border = config.focused ? palette.text : palette.divider,
            .radius_px = config.radius_px,
        };
    }

    constexpr WidgetPaint toggle(ToggleConfig config = {}) const {
        return WidgetPaint{
            .fill = config.on ? palette.active : palette.btn,
            .text = palette.text,
            .border = config.focused ? palette.text : palette.divider,
            .radius_px = config.radius_px,
        };
    }

    constexpr TextPaint text(TextConfig config = {}) const {
        return TextPaint{.color = config.tone == TextTone::Dim       ? palette.dim
                                  : config.tone == TextTone::Warning ? palette.warn
                                  : config.tone == TextTone::Danger  ? palette.expire
                                                                     : palette.text};
    }

    constexpr DividerPaint divider() const { return DividerPaint{.color = palette.divider}; }

    constexpr ProgressPaint progress(ProgressConfig config = {}) const {
        return ProgressPaint{.fill = config.expired ? palette.fill_exp : palette.fill};
    }

    constexpr AnalogClockPaint analog_clock(AnalogClockConfig config = {}) const {
        return AnalogClockPaint{
            .background = palette.bg,
            .face = palette.fill,
            .outline = config.selected ? palette.active : palette.divider,
            .tick = palette.dim,
            .text = palette.text,
            .accent = palette.warn,
        };
    }

    constexpr IconPaint icon(IconConfig config = {}) const {
        return IconPaint{
            .background = config.active ? palette.active : palette.bg,
            .foreground = palette.text,
            .accent = palette.warn,
        };
    }
};

constexpr UiMakers make_ui(const ThemePalette& palette) { return UiMakers{.palette = palette}; }
constexpr UiMakers make_ui(UiStyleConfig config) { return make_ui(palette_for(config)); }

constexpr DialogPaint make_dialog(const ThemePalette& palette, const DialogConfig& config) {
    return make_ui(palette).dialog(config);
}

constexpr ScrollViewPaint make_scrollable_view(const ThemePalette& palette, const ScrollViewConfig& config) {
    return make_ui(palette).scroll_view(config);
}

constexpr AnalogClockPaint make_analog_clock(const ThemePalette& palette, const AnalogClockConfig& config) {
    return make_ui(palette).analog_clock(config);
}

constexpr IconPaint make_icon(const ThemePalette& palette, const IconConfig& config) {
    return make_ui(palette).icon(config);
}

constexpr WidgetPaint make_button(const ThemePalette& palette, const ButtonConfig& config) {
    return make_ui(palette).button(config);
}

constexpr WidgetPaint make_dropdown(const ThemePalette& palette, const DropdownConfig& config) {
    return make_ui(palette).dropdown(config);
}
