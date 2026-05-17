#pragma once
#include <windows.h>

// ─── RAII wrapper for GDI objects ────────────────────────────────────────────
struct GdiObj {
    HGDIOBJ h;
    GdiObj() : h(nullptr) {}
    explicit GdiObj(HGDIOBJ h) : h(h) {}
    ~GdiObj() {
        if (h) DeleteObject(h);
    }
    GdiObj(const GdiObj&) = delete;
    GdiObj& operator=(const GdiObj&) = delete;
    GdiObj(GdiObj&& o) noexcept : h(o.h) { o.h = nullptr; }
    GdiObj& operator=(GdiObj&& o) noexcept {
        if (h) DeleteObject(h);
        h = o.h;
        o.h = nullptr;
        return *this;
    }
    operator HGDIOBJ() const { return h; }
};

// ─── RAII wrapper for HDC (CreateCompatibleDC / DeleteDC) ──────────────────
struct DcObj {
    HDC h = nullptr;
    explicit DcObj(HDC h = nullptr) : h(h) {}
    ~DcObj() { if (h) DeleteDC(h); }
    DcObj(const DcObj&) = delete;
    DcObj& operator=(const DcObj&) = delete;
    DcObj(DcObj&& o) noexcept : h(o.h) { o.h = nullptr; }
    DcObj& operator=(DcObj&& o) noexcept {
        if (h) DeleteDC(h);
        h = o.h;
        o.h = nullptr;
        return *this;
    }
    operator HDC() const { return h; }
};

// ─── RAII wrapper for HICON (DestroyIcon) ────────────────────────────
struct IconObj {
    HICON h = nullptr;
    explicit IconObj(HICON h = nullptr) : h(h) {}
    ~IconObj() { if (h) DestroyIcon(h); }
    IconObj(const IconObj&) = delete;
    IconObj& operator=(const IconObj&) = delete;
    IconObj(IconObj&& o) noexcept : h(o.h) { o.h = nullptr; }
    IconObj& operator=(IconObj&& o) noexcept {
        if (h) DestroyIcon(h);
        h = o.h;
        o.h = nullptr;
        return *this;
    }
    operator HICON() const { return h; }
};

// ─── Flat view of GDI handles used for painting ───────────────────────────
struct WndResources {
    HFONT fontBig = nullptr, fontLarge = nullptr, fontSm = nullptr;
    HBRUSH brBg = nullptr, brBar = nullptr, brHelp = nullptr;
};
