/**
 * @file        theme.cpp
 * @brief       Dark-theme CSS built from the ase::colors SSOT
 * @description Pure string concatenation against the constants in
 *              clients/sha-client-web/sha-web-console/generated/colors.hpp
 *              (which is itself generated from sha-web-styles/src/colors.ts).
 *              No hex literals live in this file - every colour comes from
 *              the SSOT so the desktop palette tracks the web client
 *              automatically.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/theme.hpp>

#include <ase/utils/strops.hpp>

#include "colors.hpp"

#include <cstdint>
#include <string>

namespace ase::viewer::theme {

namespace {

std::string css_hex(uint32_t c) {
    char buf[8];
    ase::utils::format_hex_color(buf, sizeof(buf), c);
    return buf;
}

}  // namespace

std::string generate_css() {
    using namespace ase::colors;

    const std::string panel       = css_hex(SURFACE_0_PANEL);
    const std::string surface_1   = css_hex(SURFACE_1);
    const std::string surface_2   = css_hex(SURFACE_2);
    const std::string surface_3   = css_hex(SURFACE_3);
    const std::string border_sub  = css_hex(BORDER_SECONDARY);
    const std::string border_pri  = css_hex(BORDER_PRIMARY);
    const std::string text_main   = css_hex(TEXT_PRIMARY);
    const std::string text_label  = css_hex(TEXT_LABEL);
    const std::string text_dim    = css_hex(TEXT_DIVIDER);
    const std::string accent_cyan = css_hex(PANEL_CYAN);

    return
        "window { background-color: " + panel + "; color: " + text_main + "; }\n"

        "headerbar { background: linear-gradient(to right, " + surface_1 + ", " + surface_2 + ");"
            " border-bottom: 1px solid " + border_sub + ";"
            " color: " + accent_cyan + "; min-height: 32px; }\n"

        ".sidebar { background-color: " + panel + ";"
            " border-right: 1px solid " + border_sub + "; }\n"
        ".sidebar treeview { background-color: transparent; color: " + text_label + "; }\n"
        ".sidebar treeview:selected { background-color: " + surface_2 + "; color: " + accent_cyan + "; }\n"
        ".sidebar treeview row:hover { background-color: " + surface_2 + "; }\n"

        ".content-area { background-color: " + panel + "; }\n"

        ".mode-button { min-height: 20px; min-width: 48px; padding: 2px 8px;"
            " font-size: 10px; font-family: 'Fira Code', monospace; }\n"
        ".mode-button:checked { background-color: " + accent_cyan + "; color: " + panel + "; }\n"

        ".welcome-title { font-size: 14px; font-family: 'Fira Code', monospace;"
            " color: " + accent_cyan + "; }\n"
        ".welcome-subtitle { font-size: 11px; font-family: 'Fira Code', monospace;"
            " color: " + text_label + "; }\n"

        ".search-entry { background-color: " + surface_1 + "; color: " + text_main + ";"
            " border: 1px solid " + border_sub + ";"
            " font-family: 'Fira Code', monospace; font-size: 11px; min-height: 24px; }\n"
        ".search-entry:focus { border-color: " + accent_cyan + "; }\n"

        ".status-bar { background-color: " + surface_1 + ";"
            " border-top: 1px solid " + border_sub + ";"
            " padding: 2px 8px; font-family: 'Fira Code', monospace;"
            " font-size: 9px; color: " + text_label + "; min-height: 18px; }\n"

        "progressbar trough { background-color: " + surface_2 + "; min-height: 4px; }\n"
        "progressbar progress { background-color: " + accent_cyan + "; min-height: 4px; }\n";
}

}  // namespace ase::viewer::theme
