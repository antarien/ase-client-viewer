#pragma once

/**
 * Image rendering — loads images via Gdk::Pixbuf, renders to Cairo surface.
 *
 * Supports: PNG, JPG, SVG (via GdkPixbuf loaders).
 * Scales to fit content width while preserving aspect ratio.
 */

#include <cairomm/cairomm.h>
#include <gdkmm/pixbuf.h>
#include <string>
#include <cstdint>

namespace ase::viewer::render {

// Render an image file at (x, y) with max_width constraint.
// Returns total height consumed (scaled image height + padding).
double render_image(const Cairo::RefPtr<Cairo::Context>& cr,
                    const std::string& image_path,
                    double x, double y, int max_width);

}  // namespace ase::viewer::render
