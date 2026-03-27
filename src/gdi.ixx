module;
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
export module gdi;

// ─── RAII wrapper for GDI objects ────────────────────────────────────────────
export struct GdiObj {
    HGDIOBJ h;
    explicit GdiObj(HGDIOBJ h = nullptr) : h(h) {}
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
export struct DcObj {
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
export struct IconObj {
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
export struct WndResources {
    HFONT fontBig = nullptr, fontLarge = nullptr, fontSm = nullptr;
    HBRUSH brBg = nullptr, brBar = nullptr, brBtn = nullptr;
    HBRUSH brActive = nullptr, brBlink = nullptr, brFill = nullptr;
    HBRUSH brFillExp = nullptr, brHelp = nullptr;
    HPEN pnNull = nullptr, pnDivider = nullptr;
};
