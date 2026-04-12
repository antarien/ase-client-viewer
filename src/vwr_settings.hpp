#pragma once

/**
 * Viewer settings — persistent configuration via JSON file.
 *
 * Stored at ~/.config/ase-viewer/settings.json
 * Provides Adw::PreferencesWindow for GTK4 UI.
 */

#include <string>
#include <cstdint>

namespace ase::viewer {

struct ViewerSettings {
    // Appearance
    uint8_t color_scheme = 0;   // 0=dark, 1=light, 2=system
    int font_size        = 11;  // 8-24pt
    int code_font        = 0;   // 0=FiraCode, 1=JetBrains Mono, 2=Source Code Pro

    // Documents
    std::string default_root;   // default docs directory
    uint8_t default_mode = 0;   // 0=TECH, 1=DSGN
    uint8_t language     = 0;   // 0=en, 1=de, 2=pt-br
    bool live_reload     = true;

    // Viewer
    bool line_numbers    = true;
    bool math_rendering  = true;
    bool mermaid_render  = true;
};

// Load from ~/.config/ase-viewer/settings.json
ViewerSettings load_settings(const std::string& config_dir);

// Save to ~/.config/ase-viewer/settings.json
void save_settings(const std::string& config_dir, const ViewerSettings& s);

}  // namespace ase::viewer
