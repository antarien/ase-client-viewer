#pragma once

/**
 * @file        settings.hpp
 * @brief       Persistent viewer configuration - JSON at $XDG_CONFIG_HOME
 * @description Flat key/value settings for appearance, documents and
 *              rendering. Loaded once at startup and re-saved whenever
 *              the preferences dialog edits a value.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <cstdint>
#include <string>

namespace ase::viewer {

struct ViewerSettings {
    // Appearance
    uint8_t color_scheme = 0;   // 0=dark, 1=light, 2=system
    int     font_size    = 11;  // 8-24pt
    int     code_font    = 0;   // 0=FiraCode, 1=JetBrains Mono, 2=Source Code Pro

    // Documents
    std::string default_root;   // default docs directory
    uint8_t     default_mode = 0;   // 0=TECH, 1=DSGN
    uint8_t     language     = 0;   // 0=en, 1=de, 2=pt-br
    bool        live_reload  = true;

    // Viewer
    bool line_numbers   = true;
    bool math_rendering = true;
    bool mermaid_render = true;

    // Sidebar (drawer) — mirrors DocsLayout xl/lg "pinnable drawer" mode.
    // open=true + pinned=true  → inline alongside canvas, splitter resizes.
    // open=true + pinned=false → floating overlay with backdrop; ESC /
    //                            backdrop / file-click collapses.
    // open=false               → 40px rail strip on the left edge; click
    //                            or drag-right reopens.
    bool sidebar_open   = true;
    bool sidebar_pinned = true;
    int  sidebar_width  = 280;
};

/** Load settings from <config_dir>/settings.json. Missing file → defaults. */
ViewerSettings load_settings(const std::string& config_dir);

/** Save settings to <config_dir>/settings.json, creating the directory. */
void save_settings(const std::string& config_dir, const ViewerSettings& s);

}  // namespace ase::viewer
