#pragma once

/**
 * @file        vwr_rndr_image_path.hpp
 * @brief       resolve_image_path - turns a markdown image URL into a path
 * @description Markdown image URLs arrive in three flavours, and each needs a
 *              different answer: `file://...` loses its prefix and is treated as
 *              an absolute filesystem path, `/cms/images/foo.png` is an absolute
 *              web path and is rewritten onto the bundled web public root
 *              (ASE_WEB_PUBLIC_ROOT, a compile definition of the ase-viewer
 *              target), and `http(s)://...` is returned as-is because the native
 *              loader cannot fetch it - ase::imgcache then renders its
 *              placeholder.
 *
 *              HEADER-ONLY ON PURPOSE: the document renderer
 *              (vwr_rndr_doc.cpp) and the DSL block renderers (the :::figure
 *              and :::gallery of vwr_dsl_meda.cpp) share ONE definition this
 *              way, instead of two that could drift apart.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-15
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
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
