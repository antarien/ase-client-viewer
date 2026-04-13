/**
 * Image rendering via Gdk::Pixbuf → Cairo surface.
 */

#include "vwr_rndr_img.hpp"
#include <gdkmm/pixbuf.h>
#include <gdkmm/general.h>

namespace ase::viewer::render {

double render_image(const Cairo::RefPtr<Cairo::Context>& cr,
                    const std::string& image_path,
                    double x, double y, int max_width) {
    try {
        auto pixbuf = Gdk::Pixbuf::create_from_file(image_path);
        if (!pixbuf) return 0;

        int img_w = pixbuf->get_width();
        int img_h = pixbuf->get_height();

        // Scale to fit max_width, preserving aspect ratio
        double scale = 1.0;
        if (img_w > max_width) {
            scale = static_cast<double>(max_width) / img_w;
        }

        double draw_w = img_w * scale;
        double draw_h = img_h * scale;

        // Create Cairo surface from Pixbuf
        auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,
                                                    static_cast<int>(draw_w),
                                                    static_cast<int>(draw_h));
        auto img_cr = Cairo::Context::create(surface);

        // Scale and render pixbuf
        img_cr->scale(scale, scale);
        Gdk::Cairo::set_source_pixbuf(img_cr, pixbuf, 0, 0);
        img_cr->paint();

        // Draw surface onto main context
        cr->set_source(surface, x, y);
        cr->paint();

        return draw_h + 10; // padding below image

    } catch (...) {
        // Failed to load — show placeholder
        cr->set_source_rgba(0.353, 0.353, 0.353, 0.5);
        cr->rectangle(x, y, max_width, 40);
        cr->fill();
        cr->set_source_rgb(0.541, 0.604, 0.604);
        cr->select_font_face("Fira Code",
            Cairo::ToyFontFace::Slant::NORMAL,
            Cairo::ToyFontFace::Weight::NORMAL);
        cr->set_font_size(10);
        cr->move_to(x + 8, y + 24);
        cr->show_text("[image: " + image_path + "]");
        return 50;
    }
}

}  // namespace ase::viewer::render
