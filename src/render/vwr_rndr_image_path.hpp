#pragma once

/**
 * Image URL → filesystem path resolver for the viewer's image renderers.
 *
 * Markdown image URLs come in three flavours:
 *   - file://...         → strip the prefix, treat as absolute filesystem path
 *   - /cms/images/foo.png → absolute web path, rewrite onto the bundled web
 *                           public root (ASE_WEB_PUBLIC_ROOT compile def)
 *   - http(s)://...      → not supported by the native loader, return as-is
 *                           so ase::imgcache renders its placeholder fallback
 *
 * Header-only so both the document-level renderer (vwr_rndr_doc.cpp) and
 * the DSL block renderers (vwr_dsl_meda.cpp's :::figure / :::gallery)
 * share one definition. ASE_WEB_PUBLIC_ROOT is provided as a compile
 * definition by the ase-viewer CMake target.
 */

#include <string>

namespace ase::viewer::render {

inline std::string resolve_image_path(const char* url) {
    if (url == nullptr || url[0] == '\0') return std::string{};
    // file:// URI — strip the prefix.
    if (url[0] == 'f' && url[1] == 'i' && url[2] == 'l' && url[3] == 'e' &&
        url[4] == ':' && url[5] == '/' && url[6] == '/') {
        return std::string{url + 7};
    }
    // Absolute web path — rewrite onto the bundled public root.
    if (url[0] == '/') {
        return std::string{ASE_WEB_PUBLIC_ROOT} + url;
    }
    // http(s):// — not supported, return as-is so the renderer falls back
    // to a placeholder.
    return std::string{url};
}

}  // namespace ase::viewer::render
