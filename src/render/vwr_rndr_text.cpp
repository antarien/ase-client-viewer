#include "vwr_rndr_text.hpp"

namespace ase::viewer::render {

int measure_text_height(const Glib::RefPtr<Pango::Layout>& layout,
                        const std::string& text, int width_px, int font_size) {
    auto font_desc = Pango::FontDescription();
    font_desc.set_family(FONT_BODY);
    font_desc.set_size(font_size);
    layout->set_font_description(font_desc);
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
    auto font_desc = Pango::FontDescription();
    font_desc.set_family(FONT_BODY);
    font_desc.set_size(FONT_SIZE_BODY);
    layout->set_font_description(font_desc);
    layout->set_width(width_px * Pango::SCALE);
    layout->set_wrap(Pango::WrapMode::WORD_CHAR);
    return layout;
}

Glib::RefPtr<Pango::Layout> create_code_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px) {
    auto layout = Pango::Layout::create(cr);
    auto font_desc = Pango::FontDescription();
    font_desc.set_family(FONT_CODE);
    font_desc.set_size(FONT_SIZE_CODE);
    layout->set_font_description(font_desc);
    layout->set_width(width_px * Pango::SCALE);
    layout->set_wrap(Pango::WrapMode::WORD_CHAR);
    return layout;
}

Glib::RefPtr<Pango::Layout> create_heading_layout(
    const Cairo::RefPtr<Cairo::Context>& cr, int width_px, uint8_t level) {
    auto layout = Pango::Layout::create(cr);
    auto font_desc = Pango::FontDescription();
    font_desc.set_family(FONT_BODY);
    font_desc.set_weight(Pango::Weight::BOLD);

    switch (level) {
        case 1: font_desc.set_size(FONT_SIZE_H1); break;
        case 2: font_desc.set_size(FONT_SIZE_H2); break;
        case 3: font_desc.set_size(FONT_SIZE_H3); break;
        default: font_desc.set_size(FONT_SIZE_H4); break;
    }

    layout->set_font_description(font_desc);
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
