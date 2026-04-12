#pragma once

/**
 * Text rendering utilities for the ASE Markdown Viewer.
 *
 * Provides Pango-based text measurement and drawing for
 * all markdown text elements (headings, paragraphs, code, etc.)
 * Uses FiraCode for monospace, NerdFont for icons, STIX for math body.
 */

#include <pangomm.h>
#include <cairomm/cairomm.h>
#include <string>
#include <cstdint>

namespace ase::viewer::render {

// Font families (system-installed via AUR packages)
constexpr const char* FONT_CODE  = "Fira Code";
constexpr const char* FONT_ICONS = "Symbols Nerd Font Mono";
constexpr const char* FONT_MATH  = "STIX Two Text";
constexpr const char* FONT_BODY  = "Fira Code";

// Font sizes in Pango units (1pt = 1024 Pango units)
constexpr int FONT_SIZE_H1    = 15 * 1024;
constexpr int FONT_SIZE_H2    = 13 * 1024;
constexpr int FONT_SIZE_H3    = 12 * 1024;
constexpr int FONT_SIZE_H4    = 11 * 1024;
constexpr int FONT_SIZE_BODY  = 11 * 1024;
constexpr int FONT_SIZE_CODE  = 10 * 1024;
constexpr int FONT_SIZE_SMALL = 9  * 1024;

// Measure text height using Pango layout
int measure_text_height(const Glib::RefPtr<Pango::Layout>& layout,
                        const std::string& text, int width_px, int font_size);

// Create a Pango layout configured for body text
Glib::RefPtr<Pango::Layout> create_body_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px);

// Create a Pango layout configured for code text
Glib::RefPtr<Pango::Layout> create_code_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px);

// Create a Pango layout for a heading
Glib::RefPtr<Pango::Layout> create_heading_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px, uint8_t level);

// Draw a Pango layout at position (x, y)
void draw_layout(const Cairo::RefPtr<Cairo::Context>& cr,
                 const Glib::RefPtr<Pango::Layout>& layout,
                 double x, double y);

}  // namespace ase::viewer::render
