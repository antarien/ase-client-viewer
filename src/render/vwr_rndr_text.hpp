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
 *   - Math  STIX Two Text (display math via ase::microtex)
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

constexpr int FONT_PX_H1    = 18;
constexpr int FONT_PX_H2    = 16;
constexpr int FONT_PX_H3    = 14;
constexpr int FONT_PX_H4    = 12;
constexpr int FONT_PX_BODY  = 12;
constexpr int FONT_PX_CODE  = 10;
constexpr int FONT_PX_SMALL = 9;

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
