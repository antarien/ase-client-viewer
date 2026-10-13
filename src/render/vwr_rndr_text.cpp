/**
 * @file        vwr_rndr_text.cpp
 * @brief       Pango layout factories and the body-size recomputation
 * @description Builds the Pango layouts every text element draws through, with
 *              the Tailwind text class sizes of
 *              clients/ase-client-web/ase-web-docviewer applied one to one via
 *              set_absolute_size, so one unit is one device pixel whatever the
 *              DPI says.
 *
 *              set_body_font_px is the one mutator of the size table: it clamps
 *              the body size to the range a reader can still use and derives
 *              every heading, code and small size from it through the Tailwind
 *              ratios. Deriving them beats storing them, because a stored set
 *              can disagree with its own body size.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/draw/command
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

#include "vwr_rndr_text.hpp"

namespace ase::viewer::render {

namespace {

int clamp_body_px(int px) {
    if (px < 8)  return 8;
    if (px > 24) return 24;
    return px;
}

int heading_size_for_level(uint8_t level) {
    if (level == 1) return FONT_PX_H1;
    if (level == 2) return FONT_PX_H2;
    if (level == 3) return FONT_PX_H3;
    return FONT_PX_H4;
}

void apply_absolute_pixel_size(Pango::FontDescription& fd, int px) {
    fd.set_absolute_size(static_cast<double>(px) * Pango::SCALE);
}

constexpr double LINE_SPACING_FACTOR = 1.4;

void apply_line_spacing(const Glib::RefPtr<Pango::Layout>& layout) {
    layout->set_line_spacing(LINE_SPACING_FACTOR);
}

}  // namespace

int measure_text_height(const Glib::RefPtr<Pango::Layout>& layout,
                        const std::string& text, int width_px, int font_px) {
    auto fd = Pango::FontDescription();
    fd.set_family(FONT_BODY);
    apply_absolute_pixel_size(fd, font_px);
    layout->set_font_description(fd);
    layout->set_width(width_px * Pango::SCALE);
    layout->set_wrap(Pango::WrapMode::WORD_CHAR);
    apply_line_spacing(layout);
    layout->set_text(text);

    int w = 0, h = 0;
    layout->get_pixel_size(w, h);
    return h;
}

Glib::RefPtr<Pango::Layout> create_body_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px) {
    auto layout = Pango::Layout::create(cr);
    auto fd = Pango::FontDescription();
    fd.set_family(FONT_BODY);
    apply_absolute_pixel_size(fd, FONT_PX_BODY);
    layout->set_font_description(fd);
    layout->set_width(width_px * Pango::SCALE);
    layout->set_wrap(Pango::WrapMode::WORD_CHAR);
    apply_line_spacing(layout);
    return layout;
}

Glib::RefPtr<Pango::Layout> create_code_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px) {
    auto layout = Pango::Layout::create(cr);
    auto fd = Pango::FontDescription();
    fd.set_family(FONT_CODE);
    apply_absolute_pixel_size(fd, FONT_PX_CODE);
    layout->set_font_description(fd);
    layout->set_width(width_px * Pango::SCALE);
    layout->set_wrap(Pango::WrapMode::WORD_CHAR);
    apply_line_spacing(layout);
    return layout;
}

Glib::RefPtr<Pango::Layout> create_heading_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px, uint8_t level) {
    auto layout = Pango::Layout::create(cr);
    auto fd = Pango::FontDescription();
    fd.set_family(FONT_BODY);
    fd.set_weight(Pango::Weight::BOLD);
    apply_absolute_pixel_size(fd, heading_size_for_level(level));
    layout->set_font_description(fd);
    layout->set_width(width_px * Pango::SCALE);
    layout->set_wrap(Pango::WrapMode::WORD_CHAR);
    apply_line_spacing(layout);
    return layout;
}

void draw_layout(const Cairo::RefPtr<Cairo::Context>& cr,
                 const Glib::RefPtr<Pango::Layout>& layout,
                 double x, double y) {
    cr->move_to(x, y);
    layout->show_in_cairo_context(cr);
}

void set_body_font_px(int px) {
    const int body = clamp_body_px(px);
    FONT_PX_BODY  = body;
    FONT_PX_H4    = body;
    FONT_PX_H3    = (body * 7  + 3) / 6;   // ×1.167
    FONT_PX_H2    = (body * 4  + 1) / 3;   // ×1.333
    FONT_PX_H1    = (body * 3  + 1) / 2;   // ×1.5
    FONT_PX_CODE  = (body * 5  + 3) / 6;   // ×0.833
    FONT_PX_SMALL = (body * 3  + 2) / 4;   // ×0.75
}

}  // namespace ase::viewer::render
