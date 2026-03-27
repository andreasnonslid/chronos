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
