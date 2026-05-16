#pragma once
#include <windows.h>
#include <cmath>
#include <cstring>
#include "gdi.hpp"
#include "ui_style.hpp"

namespace icon_detail {

struct Pixel {
    uint8_t b, g, r, a;
};

inline void blend(Pixel& dst, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    if (alpha <= 0.f) return;
    if (alpha > 1.f) alpha = 1.f;
    uint8_t a = (uint8_t)(alpha * 255.f);
    dst.r = (uint8_t)(r * alpha);
    dst.g = (uint8_t)(g * alpha);
    dst.b = (uint8_t)(b * alpha);
    dst.a = a;
}

inline void blend_over(Pixel& dst, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    if (alpha <= 0.f) return;
    if (alpha > 1.f) alpha = 1.f;
    uint8_t sa = (uint8_t)(alpha * 255.f);
    uint8_t inv = 255 - sa;
    dst.r = (uint8_t)((r * sa + dst.r * inv) / 255);
    dst.g = (uint8_t)((g * sa + dst.g * inv) / 255);
    dst.b = (uint8_t)((b * sa + dst.b * inv) / 255);
    dst.a = (uint8_t)(sa + (dst.a * inv) / 255);
}

inline float dist(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

inline float line_dist(float px, float py, float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    float len2 = dx * dx + dy * dy;
    if (len2 < 0.0001f) return dist(px, py, x1, y1);
    float t = ((px - x1) * dx + (py - y1) * dy) / len2;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return dist(px, py, x1 + t * dx, y1 + t * dy);
}

} // namespace icon_detail

inline HICON create_app_icon(int size, bool dark = true) {
    using namespace icon_detail;

    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(bih);
    bih.biWidth = size;
    bih.biHeight = -size;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    if (!screen) return nullptr;

    HBITMAP dib = CreateDIBSection(screen, (BITMAPINFO*)&bih, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib) {
        ReleaseDC(nullptr, screen);
        return nullptr;
    }

    GdiObj mask_bmp{CreateBitmap(size, size, 1, 1, nullptr)};
    ReleaseDC(nullptr, screen);

    Pixel* px = (Pixel*)bits;
    memset(px, 0, size * size * sizeof(Pixel));

    float cx = size * 0.5f;
    float cy = size * 0.5f;
    float radius = size * 0.42f;
    float ring_w = size > 20 ? 1.8f : 1.2f;
    float hand_w = size > 20 ? 1.6f : 1.1f;
    float tick_w = size > 20 ? 1.4f : 1.0f;

    IconPaint icon = make_icon(dark ? dark_palette : light_palette, IconConfig{});
    uint8_t cr = icon.foreground.r;
    uint8_t cg = icon.foreground.g;
    uint8_t cb = icon.foreground.b;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Pixel& p = px[y * size + x];
            float fx = x + 0.5f, fy = y + 0.5f;
            float d = dist(fx, fy, cx, cy);

            float ring_d = fabsf(d - radius);
            if (ring_d < ring_w) {
                float alpha = 1.f - ring_d / ring_w;
                alpha = alpha * alpha * (3.f - 2.f * alpha);
                blend(p, cr, cg, cb, alpha);
            }
        }
    }

    const float pi = 3.14159265f;
    struct Tick {
        float angle;
        float len;
    };
    Tick ticks[] = {
        {0.f, 0.15f},
        {pi * 0.5f, 0.12f},
        {pi, 0.12f},
        {pi * 1.5f, 0.15f},
    };
    for (auto& t : ticks) {
        float inner = radius - size * t.len;
        float outer = radius - size * 0.03f;
        float dx = sinf(t.angle), dy = -cosf(t.angle);
        float x1 = cx + dx * inner, y1 = cy + dy * inner;
        float x2 = cx + dx * outer, y2 = cy + dy * outer;
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float fx = x + 0.5f, fy = y + 0.5f;
                float ld = line_dist(fx, fy, x1, y1, x2, y2);
                if (ld < tick_w) {
                    float alpha = 1.f - ld / tick_w;
                    alpha = alpha * alpha * (3.f - 2.f * alpha);
                    blend_over(px[y * size + x], cr, cg, cb, alpha);
                }
            }
        }
    }

    float hour_angle = pi * 2.f * (10.f / 12.f) - pi * 0.5f;
    float min_angle = pi * 2.f * (10.f / 60.f) - pi * 0.5f;
    float hour_len = radius * 0.55f;
    float min_len = radius * 0.78f;

    struct Hand {
        float x1, y1, x2, y2;
        float w;
    };
    Hand hands[] = {
        {cx, cy, cx + cosf(hour_angle) * hour_len, cy + sinf(hour_angle) * hour_len, hand_w * 1.2f},
        {cx, cy, cx + cosf(min_angle) * min_len, cy + sinf(min_angle) * min_len, hand_w},
    };
    for (auto& h : hands) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float fx = x + 0.5f, fy = y + 0.5f;
                float ld = line_dist(fx, fy, h.x1, h.y1, h.x2, h.y2);
                if (ld < h.w) {
                    float alpha = 1.f - ld / h.w;
                    alpha = alpha * alpha * (3.f - 2.f * alpha);
                    blend_over(px[y * size + x], cr, cg, cb, alpha);
                }
            }
        }
    }

    float dot_r = size > 20 ? 1.8f : 1.2f;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float fx = x + 0.5f, fy = y + 0.5f;
            float d = dist(fx, fy, cx, cy);
            if (d < dot_r) {
                float alpha = 1.f - d / dot_r;
                alpha = alpha * alpha * (3.f - 2.f * alpha);
                blend_over(px[y * size + x], cr, cg, cb, alpha);
            }
        }
    }

    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmMask = (HBITMAP)mask_bmp.h;
    ii.hbmColor = dib;
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(dib);
    return icon;
}
