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
    explicit GdiObj(HGDIOBJ h) : h(h) {}
    ~GdiObj() {
        if (h) DeleteObject(h);
    }
    GdiObj(const GdiObj&) = delete;
    GdiObj& operator=(const GdiObj&) = delete;
    operator HGDIOBJ() const { return h; }
};
