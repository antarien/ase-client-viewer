#pragma once

/**
 * @file        canvas.hpp
 * @brief       Gtk::DrawingArea wrapper that renders markdown documents
 * @description Owns one Gtk::DrawingArea plus the parsed document, the
 *              interactive directive state and the hit region list. A
 *              GestureClick controller walks the regions on click and
 *              mutates the state before queuing a redraw. When no document
 *              is loaded the canvas paints a centred welcome title.
 *
 *              The canvas does not know about files - the orchestrator
 *              hands it parsed content via load_document(). Cleanup on
 *              load and shutdown goes through ase::markdown::free_document.
 *
 *              dump_text() walks the same AST and produces a plain text
 *              representation with bracketed directive markers so the
 *              window orchestrator can place it on the system clipboard
 *              via a Ctrl+Shift+C shortcut without losing the Cairo
 *              rich rendering.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <render/vwr_rndr_doc.hpp>

#include <ase/markdown/markdown.hpp>

#include <gtkmm/drawingarea.h>
#include <gtkmm/scrolledwindow.h>
#include <cairomm/context.h>

#include <cstdint>
#include <string>

namespace ase::viewer {

class Canvas {
public:
    Canvas();
    ~Canvas();

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    /** Return the scrolled content area for insertion into a parent. */
    Gtk::ScrolledWindow& widget() noexcept { return m_scroll; }
    Gtk::Widget* native_widget() noexcept { return &m_scroll; }

    /**
     * Parse and display the given markdown content. The mode selects
     * TECH (0) or DSGN (1) directive handling. Frontmatter is always
     * parsed. Replaces any previously loaded document and resets the
     * interactive state.
     */
    void load_document(const std::string& content, uint8_t mode);

    /** Clear the document, reverting to the welcome screen. */
    void clear_document();

    /** Return the last mode passed to load_document. */
    uint8_t current_mode() const noexcept { return m_mode; }

    /** Return the raw content of the currently loaded document. */
    const std::string& content() const noexcept { return m_content; }

    /** True if a document is loaded. */
    bool has_document() const noexcept { return m_has_doc; }

    /**
     * Re-parse the current content with a new mode. No-op if no document
     * is loaded. Resets the interactive state so fresh directive indices
     * are issued on the next draw.
     */
    void set_mode(uint8_t mode);

    /** Queue a redraw - call after mutating external state. */
    void queue_draw();

    /** Apply a new body font size (8..24 px). Rescales every heading /
     *  code / small class through render::set_body_font_px and queues a
     *  redraw. Called once on startup with settings.font_size and again
     *  every time the user edits the Preferences spin button. */
    void apply_font_size(int px);

    /**
     * Walk the current AST and produce a plain text representation with
     * bracketed directive markers so unresolved :::name{attrs} leak
     * through as raw text instead of being hidden by the Cairo renderer.
     * Empty string if no document is loaded.
     *
     * Intended to be routed to the system clipboard by the window
     * orchestrator on Ctrl+Shift+C.
     */
    std::string dump_text() const;

private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
    void handle_click(double cx, double cy);
    void free_current_document();

    Gtk::ScrolledWindow m_scroll;
    Gtk::DrawingArea    m_drawing_area;

    ase::markdown::Document m_doc{};
    bool                    m_has_doc = false;
    uint8_t                 m_mode    = 0;
    std::string             m_content;

    render::InteractiveState m_interactive_state;
    render::HitRegionList    m_hit_regions;
};

}  // namespace ase::viewer
