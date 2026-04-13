/**
 * Pango layout factories — match clients/ase-client-web/ase-web-docviewer
 * Tailwind text class sizes 1:1 via set_absolute_size (device pixels).
 *
 * @module      ase-client-viewer
 * @layer       5 (Clients)
 */

#include "vwr_rndr_text.hpp"

namespace ase::viewer::render {

namespace {

int heading_size_for_level(uint8_t level) {
    if (level == 1) return FONT_PX_H1;
    if (level == 2) return FONT_PX_H2;
    if (level == 3) return FONT_PX_H3;
    return FONT_PX_H4;
}

void apply_absolute_pixel_size(Pango::FontDescription& fd, int px) {
    fd.set_absolute_size(static_cast<double>(px) * Pango::SCALE);
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
    return layout;
}

void draw_layout(const Cairo::RefPtr<Cairo::Context>& cr,
                 const Glib::RefPtr<Pango::Layout>& layout,
                 double x, double y) {
    cr->move_to(x, y);
    layout->show_in_cairo_context(cr);
}

}  // namespace ase::viewer::render
