#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define UNICODE
#define _UNICODE
#endif
#include <cstdint>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

// ─── UTF-8 / wide string conversion ──────────────────────────────────────────
// On Windows: delegates to WideCharToMultiByte / MultiByteToWideChar (UTF-16).
// On other platforms: manual UTF-32 ↔ UTF-8 (wchar_t is 4 bytes / UTF-32).

#ifdef _WIN32

inline std::string wide_to_utf8(const std::wstring& w) {
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), len, nullptr, nullptr);
    return s;
}

inline std::wstring utf8_to_wide(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    return w;
}

#else

inline std::string wide_to_utf8(const std::wstring& w) {
    std::string out;
    out.reserve(w.size());
    for (wchar_t wc : w) {
        auto cp = static_cast<uint32_t>(wc);
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

inline std::wstring utf8_to_wide(const std::string& s) {
    std::wstring out;
    out.reserve(s.size());
    const auto* us = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = us[i++];
        uint32_t cp;
        int extra;
        if (c < 0x80)      { cp = c;        extra = 0; }
        else if (c < 0xC0) continue; // invalid continuation byte, skip
        else if (c < 0xE0) { cp = c & 0x1F; extra = 1; }
        else if (c < 0xF0) { cp = c & 0x0F; extra = 2; }
        else               { cp = c & 0x07; extra = 3; }
        int got = 0;
        while (got < extra && i < s.size()) {
            cp = (cp << 6) | (us[i++] & 0x3F);
            ++got;
        }
        if (got == extra)
            out += static_cast<wchar_t>(cp);
    }
    return out;
}

#endif
