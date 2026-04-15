/**
 * @file        header_bar.cpp
 * @brief       HeaderBar implementation — glyph buttons + search-in-title
 * @description Mirrors clients/ase-client-explorer window header mechanics
 *              1:1. Every glyph traces back to the generated ui_icons.hpp
 *              SSOT (icon-definitions.ts). set_show_title_buttons(false)
 *              removes the OS minimise/maximise/close triple — the window
 *              is closed via the Escape fallthrough in ViewerWindow.
 *
 *              TECH / DSGN mode toggles live at pack_start and keep the
 *              sidebar's subfolder swap working. Everything else (search,
 *              mark-all-read, refresh, settings) is a flat glyph button.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/header_bar.hpp>

#include <ase/utils/strops.hpp>

#include "colors.hpp"
#include "ui_icons.hpp"

#include <gtk/gtk.h>

#include <cctype>
#include <string>

namespace ase::viewer {

namespace {

// NerdFont family fallback chain — matches ase-client-explorer icons.cpp
// so every glyph renders with the same metrics.
constexpr const char* ICON_FONT      = "FiraCode Nerd Font, JetBrainsMono Nerd Font Mono";
constexpr int         ICON_FONT_SIZE = 14;

std::string to_utf8(char32_t cp) {
    std::string s;
    if (cp < 0x80) {
        s += static_cast<char>(cp);
    } else if (cp < 0x800) {
        s += static_cast<char>(0xC0 | (cp >> 6));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return s;
}

std::string glyph_markup(char32_t glyph, uint32_t rgb_color, int point_size) {
    // Hex colour through the ase::utils SSOT helper — the validator
    // blocks printf/snprintf, and every other glyph renderer in the
    // codebase already goes through format_hex_rgb.
    char color_hex[8];
    ase::utils::format_hex_rgb(color_hex, sizeof(color_hex),
        (rgb_color >> 16) & 0xFFu,
        (rgb_color >>  8) & 0xFFu,
         rgb_color        & 0xFFu);
    std::string out;
    out.append("<span font_family='");
    out.append(ICON_FONT);
    out.append("' font_size='");
    out.append(std::to_string(point_size * 1024));
    out.append("' foreground='");
    out.append(color_hex);
    out.append("'>");
    out.append(to_utf8(glyph));
    out.append("</span>");
    return out;
}

void set_glyph_child(Gtk::Button& btn, char32_t glyph) {
    constexpr uint32_t header_glyph_color = ase::colors::TEXT_LIGHT & 0xFFFFFFu;
    const std::string markup = glyph_markup(glyph, header_glyph_color, ICON_FONT_SIZE);

    GtkWidget* lbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(lbl), markup.c_str());
    gtk_label_set_use_markup(GTK_LABEL(lbl), TRUE);
    gtk_button_set_child(GTK_BUTTON(btn.gobj()), lbl);
    btn.add_css_class("flat");
}

}  // namespace

HeaderBar::HeaderBar() {
    // No OS window buttons — explorer parity + matches the ESC-to-close
    // convention set by ViewerWindow::build_ui.
    m_header.set_show_title_buttons(false);

    m_title_label.set_text("ASE Viewer");
    m_title_label.add_css_class("title");
    m_header.set_title_widget(m_title_label);

    // ── TECH / DSGN toggle pair at pack_start ──────────────────────────
    // Kept because the mode still drives the sidebar subfolder swap
    // (tech/ vs cms/) in ViewerWindow::switch_mode_directory. Styled as
    // small flat toggle buttons with uppercase labels via .mode-button.
    m_btn_tech.set_label("TECH");
    m_btn_dsgn.set_label("DSGN");
    m_btn_tech.add_css_class("mode-button");
    m_btn_dsgn.add_css_class("mode-button");
    m_mode_box.append(m_btn_tech);
    m_mode_box.append(m_btn_dsgn);
    m_btn_tech.set_group(m_btn_dsgn);
    m_btn_tech.set_active(true);
    m_header.pack_start(m_mode_box);

    m_btn_tech.signal_toggled().connect([this]() {
        if (m_suppress_mode_signal) return;
        const uint8_t mode = m_btn_tech.get_active() ? 0u : 1u;
        if (m_on_mode) m_on_mode(mode);
    });

    // ── Search entry: created now but NOT inserted yet ─────────────────
    // Explorer-style: the entry is the centre title widget when search
    // is active and the plain title label takes that slot otherwise.
    m_search.set_placeholder_text("Filter files... (Ctrl+F)");
    m_search.set_hexpand(true);
    m_search.set_visible(false);
    m_search.signal_search_changed().connect([this]() {
        auto raw = m_search.get_text();
        std::string lowered;
        lowered.reserve(raw.bytes());
        for (auto ch : raw) {
            lowered += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (m_on_search) m_on_search(lowered);
    });
    // Pressing Escape in the entry fires `stop-search` — route that back
    // to the caller's search-toggle handler so the title label restores.
    m_search.signal_stop_search().connect([this]() {
        if (m_on_search_toggle) m_on_search_toggle();
    });

    // ── Right-aligned glyph button cluster (pack_end reverses order) ───
    //
    //   [search] [mark-read] [refresh] [settings]
    //
    // pack_end prepends each new button to the right edge, so we add them
    // in REVERSE visual order: settings first ends up rightmost.

    set_glyph_child(m_btn_settings, ase::ui_icons::ICON_SETTINGS);
    m_btn_settings.set_tooltip_text("Settings (Ctrl+,)");
    m_btn_settings.signal_clicked().connect([this]() {
        if (m_on_settings) m_on_settings();
    });
    m_header.pack_end(m_btn_settings);

    set_glyph_child(m_btn_refresh, ase::ui_icons::ICON_REFRESH);
    m_btn_refresh.set_tooltip_text("Refresh (F5)");
    m_btn_refresh.signal_clicked().connect([this]() {
        if (m_on_refresh) m_on_refresh();
    });
    m_header.pack_end(m_btn_refresh);

    set_glyph_child(m_btn_mark_read, ase::ui_icons::ICON_CHECK);
    m_btn_mark_read.set_tooltip_text("Mark all as read");
    m_btn_mark_read.signal_clicked().connect([this]() {
        if (m_on_mark) m_on_mark();
    });
    m_header.pack_end(m_btn_mark_read);

    set_glyph_child(m_btn_search, ase::ui_icons::ICON_SEARCH);
    m_btn_search.set_tooltip_text("Search (Ctrl+F)");
    m_btn_search.signal_clicked().connect([this]() {
        if (m_on_search_toggle) m_on_search_toggle();
    });
    m_header.pack_end(m_btn_search);
}

void HeaderBar::set_title(const std::string& title) {
    m_title_label.set_text(title);
}

void HeaderBar::set_mode(uint8_t mode) {
    m_suppress_mode_signal = true;
    if (mode == 0u) {
        m_btn_tech.set_active(true);
    } else {
        m_btn_dsgn.set_active(true);
    }
    m_suppress_mode_signal = false;
}

void HeaderBar::toggle_search(bool visible) {
    m_search_visible = visible;
    if (visible) {
        // Swap the centre title slot to the search entry and hand focus
        // to it so the user can type immediately.
        m_header.set_title_widget(m_search);
        m_search.set_visible(true);
        m_search.grab_focus();
    } else {
        // Swap back to the title label. Clear the entry so subsequent
        // searches start fresh and the filter state resets in lockstep.
        m_search.set_text("");
        m_search.set_visible(false);
        m_header.set_title_widget(m_title_label);
    }
}

void HeaderBar::seed_search_with(const std::string& utf8_char) {
    if (!m_search_visible) return;
    m_search.set_text(utf8_char);
    // set_text leaves the cursor at position 0 — move past the seed so
    // subsequent keystrokes append instead of prepending (typing c, p, p
    // would otherwise produce "ppc" instead of "cpp").
    gtk_editable_set_position(GTK_EDITABLE(m_search.gobj()), -1);
}

void HeaderBar::focus_search() {
    if (!m_search_visible) toggle_search(true);
    else                   m_search.grab_focus();
}

void HeaderBar::clear_search() {
    m_search.set_text("");
}

}  // namespace ase::viewer
