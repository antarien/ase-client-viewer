/**
 * @file        theme.cpp
 * @brief       Dark-theme CSS built from the ase::colors + ase::theme SSOT
 * @description Ported 1:1 from clients/ase-client-explorer/src/theme.cpp
 *              (dashboard aesthetic, NerdFont glyph buttons, dense cells)
 *              and extended with the viewer-specific status bar + content
 *              area styles. Every colour comes from the generated
 *              sha-web-console/colors.hpp and every spacing constant from
 *              design_tokens.hpp so the desktop palette tracks the web
 *              client automatically.
 *
 *              Shared alias chain mirrored from sha-web-styles/src/dash/
 *              tokens.css:
 *                --dash-surface-0       = SURFACE_0          (pure black)
 *                --dash-surface-2       = SURFACE_3          (header bg)
 *                --dash-text-primary    = TEXT_DEFAULT       (#9A9A9A)
 *                --dash-text-muted      = TEXT_LABEL         (#5A5A5A)
 *                --dash-border-primary  = BORDER_PRIMARY     (#2A2A2A)
 *                --dash-border-secondary= BORDER_SECONDARY   (#1F1F1F)
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/theme.hpp>

#include <ase/utils/strops.hpp>

#include "colors.hpp"
#include "design_tokens.hpp"

#include <cstdint>
#include <string>

namespace ase::viewer::theme {

namespace {

std::string css_hex(uint32_t c) {
    char buf[8];
    ase::utils::format_hex_color(buf, sizeof(buf), c);
    return buf;
}

std::string px(int v) { return std::to_string(v) + "px"; }

}  // namespace

std::string generate_css() {
    using namespace ase::colors;
    namespace t = ase::theme;

    // ── Color tokens — mirror dash/tokens.css aliases ───────────────
    const std::string surface_0     = css_hex(SURFACE_0);          // pure black bg
    const std::string surface_1     = css_hex(SURFACE_1);          // elevated card
    const std::string surface_2     = css_hex(SURFACE_2);          // popover, selection
    const std::string surface_2_h   = css_hex(SURFACE_2_HOVER);
    const std::string surface_3     = css_hex(SURFACE_3);          // dashboard header bg
    const std::string surface_4     = css_hex(SURFACE_4);
    const std::string surface_5     = css_hex(SURFACE_5);
    const std::string text_primary  = css_hex(TEXT_DEFAULT);       // dash-text-primary
    const std::string text_muted    = css_hex(TEXT_LABEL);         // dash-text-muted
    const std::string text_hover    = css_hex(TEXT_HOVER);
    const std::string text_light    = css_hex(TEXT_LIGHT);
    const std::string text_divider  = css_hex(TEXT_DIVIDER);
    const std::string border_primary= css_hex(BORDER_PRIMARY);
    const std::string border_sec    = css_hex(BORDER_SECONDARY);
    const std::string accent        = css_hex(PANEL_CYAN);         // mode-button checked, selection

    // ── Design tokens (cached as CSS strings for readability) ────────
    const std::string font_mono     = t::font_mono;
    const std::string fs_xs         = px(t::font_xs);     // 10
    const std::string fs_sm         = px(t::font_sm);     // 11
    const std::string fs_md         = px(t::font_md);     // 12
    const std::string fs_lg         = px(t::font_lg);     // 14
    const std::string sp_xs         = px(t::space_xs);    // 4
    const std::string sp_sm         = px(t::space_sm);    // 8
    const std::string sp_md         = px(t::space_md);    // 16
    const std::string sp_lg         = px(t::space_lg);    // 24
    const std::string sp_xl         = px(t::space_xl);    // 32
    const std::string sp_2          = px(t::space_xs / 2);             // 2
    const std::string sp_6          = px(t::space_sm - t::space_xs/2); // 6
    const std::string sp_12         = px(t::space_sm + t::space_xs);   // 12

    return
        "* { border-radius: 0; }\n"

        // ── Window + header bar ──────────────────────────────────────
        "window { background-color: " + surface_0 + "; color: " + text_primary + ";"
            " font-family: " + font_mono + "; }\n"
        "headerbar { background-color: " + surface_3 + ";"
            " border-bottom: 1px solid " + border_sec + "; color: " + text_primary + ";"
            " min-height: " + sp_xl + "; padding: 0 " + sp_sm + "; }\n"
        "headerbar > .title, headerbar .title {"
            " color: " + text_light + "; font-family: " + font_mono + ";"
            " font-size: " + fs_md + "; font-weight: bold; }\n"

        // ── Flat glyph buttons in the header (NerdFont markup child) ─
        "headerbar button.flat { background: transparent; border: none;"
            " min-height: " + sp_lg + "; min-width: " + sp_lg + ";"
            " padding: 0 " + sp_sm + "; box-shadow: none; }\n"
        "headerbar button.flat:hover { background-color: " + surface_4 + "; }\n"
        "headerbar button.flat:active { background-color: " + surface_2 + "; }\n"

        // ── TECH / DSGN toggle pair — uppercase mini buttons ─────────
        ".mode-button { min-height: " + sp_md + "; min-width: " + sp_xl + ";"
            " padding: " + sp_2 + " " + sp_sm + ";"
            " font-family: " + font_mono + "; font-size: " + fs_xs + ";"
            " background-color: transparent; border: 1px solid " + border_sec + ";"
            " color: " + text_muted + "; }\n"
        ".mode-button:hover { background-color: " + surface_4 + "; color: " + text_primary + "; }\n"
        ".mode-button:checked { background-color: " + accent + "; color: " + surface_0 + ";"
            " border-color: " + accent + "; }\n"

        // ── Header search entry — small font like dashboard filter ───
        "headerbar searchentry, headerbar searchentry > text,"
        " headerbar entry, headerbar entry > text {"
            " background-color: " + surface_1 + "; color: " + text_primary + ";"
            " border: 1px solid " + border_primary + ";"
            " font-size: " + fs_xs + "; min-height: " + sp_md + "; padding: 0 " + sp_sm + "; }\n"
        "headerbar searchentry:focus-within, headerbar entry:focus-within {"
            " border-color: " + accent + "; }\n"

        // ── Sidebar tree view ────────────────────────────────────────
        ".sidebar { background-color: " + surface_0 + ";"
            " border-right: 1px solid " + border_sec + "; }\n"
        ".sidebar treeview { background-color: transparent; color: " + text_muted + ";"
            " font-family: " + font_mono + "; font-size: " + fs_md + "; }\n"
        ".sidebar treeview:selected { background-color: " + surface_2 + "; color: " + accent + "; }\n"
        ".sidebar treeview row:hover { background-color: " + surface_2_h + "; }\n"

        // GtkListView variant (in case any viewer piece uses GtkListView).
        "listview { background-color: transparent; color: " + text_primary + ";"
            " font-family: " + font_mono + "; font-size: " + fs_md + "; }\n"
        "listview > row { padding: 0 " + sp_sm + "; min-height: " + sp_lg + "; }\n"
        "listview > row:hover { background-color: " + surface_2_h + "; }\n"
        "listview > row:selected { background-color: " + surface_2 + "; color: " + accent + "; }\n"
        "treeexpander { min-width: " + sp_md + "; }\n"

        // ── Popover / menu / scrollbar — dashboard surfaces ──────────
        "popover, popover > contents, menu, menuitem {"
            " background-color: " + surface_1 + "; color: " + text_primary + "; }\n"
        "button { background-color: " + surface_3 + "; color: " + text_primary + "; }\n"
        "button:hover { background-color: " + surface_4 + "; }\n"
        "entry, searchbar, search > entry { background-color: " + surface_1 + ";"
            " color: " + text_primary + "; border: 1px solid " + border_primary + "; }\n"
        "scrollbar slider { background-color: " + surface_4 + "; }\n"
        "scrollbar slider:hover { background-color: " + surface_5 + "; }\n"

        // ── Canvas content area ──────────────────────────────────────
        ".content-area { background-color: " + surface_0 + "; }\n"

        // ── Welcome placeholder (no document loaded) ─────────────────
        ".welcome-title { font-size: " + fs_lg + "; font-family: " + font_mono + ";"
            " color: " + accent + "; }\n"
        ".welcome-subtitle { font-size: " + fs_sm + "; font-family: " + font_mono + ";"
            " color: " + text_muted + "; }\n"

        // ── Status bar ───────────────────────────────────────────────
        ".status-bar { background-color: " + surface_1 + ";"
            " border-top: 1px solid " + border_sec + ";"
            " padding: " + sp_2 + " " + sp_sm + ";"
            " font-family: " + font_mono + "; font-size: " + fs_xs + ";"
            " color: " + text_muted + "; min-height: " + sp_md + "; }\n"

        "progressbar trough { background-color: " + surface_2 + "; min-height: " + sp_xs + "; }\n"
        "progressbar progress { background-color: " + accent + "; min-height: " + sp_xs + "; }\n"

        // ── Adwaita preference window surfaces (fallback until the  ──
        //    custom dashboard-style dialog ships) — matches explorer. ──
        "preferencespage { background-color: " + surface_0 + "; }\n"
        "preferencespage row.activatable,"
        " preferencespage row.entry,"
        " preferencespage row.combo,"
        " preferencespage row.switch {"
            " background-color: " + surface_1 + "; color: " + text_primary + ";"
            " border: 1px solid " + border_sec + "; }\n"
        "preferencespage row.activatable:hover { background-color: " + surface_2_h + "; }\n"

        // ── Dashboard-style settings dialog classes (reserved) ───────
        ".ase-cfg-window { background-color: " + surface_0 + "; }\n"
        ".ase-cfg-section-title { color: " + text_muted + ";"
            " background-color: " + surface_3 + ";"
            " font-size: " + fs_sm + "; font-weight: 600;"
            " padding: " + sp_sm + " " + sp_md + ";"
            " border-bottom: 1px solid " + border_sec + ";"
            " text-transform: uppercase; }\n"
        ".ase-cfg-cell { padding: " + sp_sm + " " + sp_md + ";"
            " border-right: 1px solid " + border_sec + ";"
            " border-bottom: 1px solid " + border_sec + ";"
            " background-color: " + surface_0 + "; }\n"
        ".ase-cfg-cell:hover { background-color: " + surface_2_h + "; }\n"
        ".ase-cfg-cell-label { color: " + text_muted + "; font-size: " + fs_xs + ";"
            " text-transform: uppercase; }\n"
        ".ase-cfg-cell-value { color: " + text_light + "; font-size: " + fs_md + "; }\n";
}

}  // namespace ase::viewer::theme
