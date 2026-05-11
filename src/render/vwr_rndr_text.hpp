#pragma once

/**
 * Text rendering utilities for the ASE Markdown Viewer.
 *
 * Provides Pango-based text measurement and drawing for all markdown
 * text elements (headings, paragraphs, code, etc.). Sizes mirror the
 * web docviewer's Tailwind classes 1:1 (text-xs, text-sm, text-base,
 * text-lg, text-2xs, text-3xs) so the native viewer renders at the
 * same pixel sizes as ase-client-web.
 *
 * Fonts:
 *   - Body  Fira Code (matches web font-mono on the docs container)
 *   - Code  Fira Code (text-3xs / 10 px)
 *   - Icons Symbols Nerd Font Mono (NerdFont glyph fallback)
 *   - Math  STIX Two Text (display math via ase::adp::microtex)
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include <pangomm.h>
#include <cairomm/cairomm.h>
#include <string>
#include <cstdint>

namespace ase::viewer::render {

constexpr const char* FONT_CODE  = "Fira Code";
constexpr const char* FONT_ICONS = "Symbols Nerd Font Mono";
constexpr const char* FONT_MATH  = "STIX Two Text";
constexpr const char* FONT_BODY  = "Fira Code";

// Web SSOT: Tailwind text classes from clients/ase-client-web/ase-web-docviewer/src/DocsViewer.tsx
//   md:text-lg   = 18 px  → H1
//   md:text-base = 16 px  → H2
//   md:text-sm   = 14 px  → H3
//   md:text-xs   = 12 px  → H4 + body container
//   text-3xs     = 10 px  → inline code, code blocks
//   text-2xs     =  9 px  → small / micro labels
//
// Stored as integer pixels; rendering uses Pango::FontDescription::set_absolute_size
// with `value * Pango::SCALE` so 1 unit == 1 device pixel regardless of DPI.

// Globals (not constexpr) so the runtime can rescale them when the user
// edits the font size in Preferences. set_body_font_px() is the SSOT
// mutator: it derives every heading / code / small size from the body
// size via Tailwind ratios (H1=1.5×, H2=1.333×, H3=1.167×, H4=1.0×,
// CODE=0.833×, SMALL=0.75×).

inline int FONT_PX_H1    = 18;
inline int FONT_PX_H2    = 16;
inline int FONT_PX_H3    = 14;
inline int FONT_PX_H4    = 12;
inline int FONT_PX_BODY  = 12;
inline int FONT_PX_CODE  = 10;
inline int FONT_PX_SMALL = 9;

/** Set the body pixel size and recompute every dependent class
 *  (H1..H4, CODE, SMALL) using the Tailwind ratio table. Caller is
 *  responsible for triggering a redraw of any already-painted widgets. */
void set_body_font_px(int px);

int measure_text_height(const Glib::RefPtr<Pango::Layout>& layout,
                        const std::string& text, int width_px, int font_px);

Glib::RefPtr<Pango::Layout> create_body_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px);

Glib::RefPtr<Pango::Layout> create_code_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px);

Glib::RefPtr<Pango::Layout> create_heading_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px, uint8_t level);

void draw_layout(const Cairo::RefPtr<Cairo::Context>& cr,
                 const Glib::RefPtr<Pango::Layout>& layout,
                 double x, double y);

}  // namespace ase::viewer::render
