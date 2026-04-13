/**
 * @file        header_bar.cpp
 * @brief       HeaderBar implementation
 * @description Wires every child widget to its matching slot at construction
 *              time. Owns zero application state - the active mode, search
 *              text and read-set live in the window orchestrator.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/header_bar.hpp>

#include <cctype>

namespace ase::viewer {

HeaderBar::HeaderBar() {
    m_title_label.set_text("ASE TECH & DESIGN Viewer");
    m_title_label.add_css_class("title");
    m_header.set_title_widget(m_title_label);

    m_btn_tech.set_label("TECH");
    m_btn_dsgn.set_label("DSGN");
    m_btn_tech.set_active(true);
    m_btn_tech.add_css_class("mode-button");
    m_btn_dsgn.add_css_class("mode-button");
    m_btn_tech.set_group(m_btn_dsgn);
    m_mode_box.append(m_btn_tech);
    m_mode_box.append(m_btn_dsgn);
    m_header.pack_start(m_mode_box);

    m_btn_tech.signal_toggled().connect([this]() {
        if (m_suppress_mode_signal) return;
        const uint8_t mode = m_btn_tech.get_active() ? 0u : 1u;
        if (m_on_mode) m_on_mode(mode);
    });

    m_search.set_placeholder_text("Search docs...  (Ctrl+F)");
    m_search.add_css_class("search-entry");
    m_search.signal_search_changed().connect([this]() {
        auto raw = m_search.get_text();
        std::string lowered;
        lowered.reserve(raw.bytes());
        for (auto ch : raw) {
            lowered += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (m_on_search) m_on_search(lowered);
    });
    m_header.pack_end(m_search);

    m_btn_mark_read.set_label("\xe2\x9c\x93");
    m_btn_mark_read.set_tooltip_text("Mark all as read");
    m_btn_mark_read.add_css_class("mode-button");
    m_btn_mark_read.signal_clicked().connect([this]() {
        if (m_on_mark) m_on_mark();
    });
    m_header.pack_end(m_btn_mark_read);

    m_btn_settings.set_label("\xe2\x9a\x99");
    m_btn_settings.set_tooltip_text("Settings (Ctrl+,)");
    m_btn_settings.add_css_class("mode-button");
    m_btn_settings.signal_clicked().connect([this]() {
        if (m_on_settings) m_on_settings();
    });
    m_header.pack_end(m_btn_settings);
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

void HeaderBar::focus_search() {
    m_search.grab_focus();
}

void HeaderBar::clear_search() {
    m_search.set_text("");
}

}  // namespace ase::viewer
