/**
 * ASE-Math DSL → muparser eval → Cairo 2D plots.
 *
 * Parses ASE-Math DSL directives:
 *   plot: y = sin(x) * exp(-x/10)
 *   range: x=-10..10, y=-2..2
 *   marker: x=0, label="Origin"
 *   band: x=2..4, color=cyan
 *
 * Renders: axes, grid, curve, markers, labels.
 */

#include "vwr_rndr_amdl.hpp"
#include <muParser.h>
#include <string>
#include <sstream>
#include <cmath>

namespace ase::viewer::render {

namespace {

constexpr double AXIS_R = 0.353, AXIS_G = 0.353, AXIS_B = 0.353;
constexpr double GRID_R = 0.118, GRID_G = 0.118, GRID_B = 0.118;
constexpr double CURVE_R = 0.353, CURVE_G = 0.612, CURVE_B = 0.722;
constexpr double TEXT_R = 0.541, TEXT_G = 0.604, TEXT_B = 0.604;
constexpr double BG_R = 0.039, BG_G = 0.039, BG_B = 0.039;

constexpr double PLOT_H = 200.0;
constexpr double MARGIN = 40.0;
constexpr int SAMPLES = 200;

struct PlotConfig {
    std::string expr;
    double x_min = -10, x_max = 10;
    double y_min = -2, y_max = 2;
};

PlotConfig parse_config(const char* code, uint32_t len) {
    PlotConfig cfg;
    std::istringstream stream(std::string(code, len));
    std::string line;

    while (std::getline(stream, line)) {
        if (line.find("plot:") == 0 || line.find("y =") != std::string::npos || line.find("y=") != std::string::npos) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                cfg.expr = line.substr(eq + 1);
                // Trim
                while (!cfg.expr.empty() && cfg.expr.front() == ' ') cfg.expr.erase(0, 1);
            }
        } else if (line.find("range:") == 0) {
            // range: x=-10..10, y=-2..2
            auto xp = line.find("x=");
            if (xp != std::string::npos) {
                auto dots = line.find("..", xp);
                if (dots != std::string::npos) {
                    cfg.x_min = std::stod(line.substr(xp + 2));
                    auto comma = line.find(',', dots);
                    cfg.x_max = std::stod(line.substr(dots + 2, (comma != std::string::npos ? comma - dots - 2 : std::string::npos)));
                }
            }
            auto yp = line.find("y=");
            if (yp != std::string::npos) {
                auto dots = line.find("..", yp);
                if (dots != std::string::npos) {
                    cfg.y_min = std::stod(line.substr(yp + 2));
                    cfg.y_max = std::stod(line.substr(dots + 2));
                }
            }
        }
    }
    return cfg;
}

}  // anonymous namespace

double render_ase_math_plot(const Cairo::RefPtr<Cairo::Context>& cr,
                           const char* code, uint32_t len,
                           double x, double y, int max_width) {
    PlotConfig cfg = parse_config(code, len);
    if (cfg.expr.empty()) {
        // No expression found — render as placeholder
        cr->set_source_rgb(AXIS_R, AXIS_G, AXIS_B);
        cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
        cr->set_font_size(10);
        cr->move_to(x + 8, y + 20);
        cr->show_text("[ase-math: no expression]");
        return 30;
    }

    double plot_w = max_width - 2 * MARGIN;
    double plot_h = PLOT_H;
    double total_h = plot_h + 2 * MARGIN;

    // Background
    cr->set_source_rgb(BG_R, BG_G, BG_B);
    cr->rectangle(x, y, max_width, total_h);
    cr->fill();

    double ox = x + MARGIN;
    double oy = y + MARGIN;

    // Grid lines
    cr->set_source_rgb(GRID_R, GRID_G, GRID_B);
    cr->set_line_width(0.5);
    for (int i = 0; i <= 4; i++) {
        double gy_pos = oy + i * plot_h / 4;
        cr->move_to(ox, gy_pos);
        cr->line_to(ox + plot_w, gy_pos);
        cr->stroke();
        double gx_pos = ox + i * plot_w / 4;
        cr->move_to(gx_pos, oy);
        cr->line_to(gx_pos, oy + plot_h);
        cr->stroke();
    }

    // Axes
    cr->set_source_rgb(AXIS_R, AXIS_G, AXIS_B);
    cr->set_line_width(1);
    // X axis (at y=0 if in range)
    if (cfg.y_min <= 0 && cfg.y_max >= 0) {
        double y0 = oy + plot_h * (cfg.y_max / (cfg.y_max - cfg.y_min));
        cr->move_to(ox, y0);
        cr->line_to(ox + plot_w, y0);
        cr->stroke();
    }
    // Y axis (at x=0 if in range)
    if (cfg.x_min <= 0 && cfg.x_max >= 0) {
        double x0 = ox + plot_w * (-cfg.x_min / (cfg.x_max - cfg.x_min));
        cr->move_to(x0, oy);
        cr->line_to(x0, oy + plot_h);
        cr->stroke();
    }

    // Evaluate and plot curve
    try {
        mu::Parser parser;
        double var_x = 0;
        parser.DefineVar("x", &var_x);
        parser.SetExpr(cfg.expr);

        cr->set_source_rgb(CURVE_R, CURVE_G, CURVE_B);
        cr->set_line_width(2);

        bool first = true;
        for (int i = 0; i <= SAMPLES; i++) {
            var_x = cfg.x_min + (cfg.x_max - cfg.x_min) * i / SAMPLES;
            double val = parser.Eval();

            // Map to pixel coordinates
            double px = ox + plot_w * (var_x - cfg.x_min) / (cfg.x_max - cfg.x_min);
            double py = oy + plot_h * (1.0 - (val - cfg.y_min) / (cfg.y_max - cfg.y_min));

            // Clip to plot area
            if (py < oy - 10 || py > oy + plot_h + 10) {
                first = true;
                continue;
            }

            if (first) {
                cr->move_to(px, py);
                first = false;
            } else {
                cr->line_to(px, py);
            }
        }
        cr->stroke();
    } catch (...) {
        // muparser error — show error text
        cr->set_source_rgb(0.94, 0.32, 0.29);
        cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
        cr->set_font_size(10);
        cr->move_to(ox + 8, oy + 20);
        cr->show_text("eval error: " + cfg.expr);
    }

    // Axis labels
    cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_size(9);

    // X range labels
    cr->move_to(ox, oy + plot_h + 14);
    cr->show_text(std::to_string(static_cast<int>(cfg.x_min)));
    cr->move_to(ox + plot_w - 20, oy + plot_h + 14);
    cr->show_text(std::to_string(static_cast<int>(cfg.x_max)));

    // Y range labels
    cr->move_to(ox - 30, oy + 10);
    cr->show_text(std::to_string(static_cast<int>(cfg.y_max)));
    cr->move_to(ox - 30, oy + plot_h);
    cr->show_text(std::to_string(static_cast<int>(cfg.y_min)));

    // Expression label
    cr->set_source_rgb(CURVE_R, CURVE_G, CURVE_B);
    cr->set_font_size(10);
    cr->move_to(ox + 8, oy + 14);
    cr->show_text("y = " + cfg.expr);

    return total_h;
}

}  // namespace ase::viewer::render
