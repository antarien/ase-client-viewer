#include "vwr_dsl_dispatch.hpp"
#include "../vwr_rndr_text.hpp"
#include <ase/markdown/types.hpp>
#include <cstring>

namespace ase::viewer::render::dsl {

namespace {

// Colors matching DSGN mode (CMS palette)
constexpr double CYAN_R = 0.35, CYAN_G = 0.61, CYAN_B = 0.72;
constexpr double GREEN_R = 0.25, GREEN_G = 0.73, GREEN_B = 0.31;
constexpr double AMBER_R = 0.82, AMBER_G = 0.60, AMBER_B = 0.13;
constexpr double PURPLE_R = 0.65, PURPLE_G = 0.55, PURPLE_B = 0.98;
constexpr double TEXT_R = 0.54, TEXT_G = 0.60, TEXT_B = 0.60;
constexpr double MUTED_R = 0.35, MUTED_G = 0.35, MUTED_B = 0.35;
constexpr double SURFACE_R = 0.047, SURFACE_G = 0.047, SURFACE_B = 0.047;

constexpr double PARA_SPACING = 10.0;
constexpr double BLOCK_PAD    = 12.0;

void collect_text(const ase::markdown::Node* node, std::string& out) {
    if (!node) return;
    if (node->text && node->text_len > 0) out.append(node->text, node->text_len);
    const ase::markdown::Node* child = node->first_child;
    while (child) { collect_text(child, out); child = child->next_sibling; }
}

bool name_is(const ase::markdown::Node* node, const char* name) {
    if (!node->directive_name) return false;
    return std::strcmp(node->directive_name, name) == 0;
}

// ── Block Directive Renderers ───────────────────────────────────

double render_hero(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 120;
    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.08);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h);
    ctx.cr->fill();
    // Border
    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.3);
    ctx.cr->set_line_width(1);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h);
    ctx.cr->stroke();
    // Title
    std::string text;
    collect_text(node, text);
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::BOLD);
    ctx.cr->set_font_size(18);
    ctx.cr->move_to(ctx.margin_left + 24, ctx.y + h / 2.0 + 6);
    ctx.cr->show_text(text);
    return h + PARA_SPACING;
}

double render_columns(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 80;
    int col_count = 2;
    int col_w = content_width / col_count;
    // Draw column placeholders
    for (int i = 0; i < col_count; i++) {
        double x = ctx.margin_left + i * col_w;
        ctx.cr->set_source_rgba(1, 1, 1, 0.02);
        ctx.cr->rectangle(x + 2, ctx.y, col_w - 4, h);
        ctx.cr->fill();
        ctx.cr->set_source_rgba(1, 1, 1, 0.05);
        ctx.cr->rectangle(x + 2, ctx.y, col_w - 4, h);
        ctx.cr->stroke();
    }
    // Label
    ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(9);
    ctx.cr->move_to(ctx.margin_left + 6, ctx.y + 14);
    ctx.cr->show_text(":::columns");
    (void)node;
    return h + PARA_SPACING;
}

double render_cards(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double card_h = 60;
    int cards = 3;
    int card_w = content_width / cards;
    for (int i = 0; i < cards; i++) {
        double x = ctx.margin_left + i * card_w;
        ctx.cr->set_source_rgb(SURFACE_R, SURFACE_G, SURFACE_B);
        ctx.cr->rectangle(x + 4, ctx.y, card_w - 8, card_h);
        ctx.cr->fill();
        ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.15);
        ctx.cr->set_line_width(1);
        ctx.cr->rectangle(x + 4, ctx.y, card_w - 8, card_h);
        ctx.cr->stroke();
    }
    ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(9);
    ctx.cr->move_to(ctx.margin_left + 10, ctx.y + 14);
    ctx.cr->show_text(":::cards");
    (void)node;
    return card_h + PARA_SPACING;
}

double render_tabs(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 90;
    // Tab bar
    ctx.cr->set_source_rgba(1, 1, 1, 0.03);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, 24);
    ctx.cr->fill();
    // Active tab indicator
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y + 22, 80, 2);
    ctx.cr->fill();
    // Tab labels
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(10);
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    ctx.cr->move_to(ctx.margin_left + 8, ctx.y + 16);
    ctx.cr->show_text("Tab 1");
    ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
    ctx.cr->move_to(ctx.margin_left + 88, ctx.y + 16);
    ctx.cr->show_text("Tab 2");
    // Content area
    ctx.cr->set_source_rgba(1, 1, 1, 0.02);
    ctx.cr->rectangle(ctx.margin_left, ctx.y + 24, content_width, h - 24);
    ctx.cr->fill();
    (void)node;
    return h + PARA_SPACING;
}

double render_timeline(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 100;
    // Vertical line
    double cx = ctx.margin_left + 12;
    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.3);
    ctx.cr->set_line_width(2);
    ctx.cr->move_to(cx, ctx.y);
    ctx.cr->line_to(cx, ctx.y + h);
    ctx.cr->stroke();
    // Timeline dots
    for (int i = 0; i < 3; i++) {
        double dy = ctx.y + 10 + i * 35;
        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        ctx.cr->arc(cx, dy, 4, 0, 2 * 3.14159);
        ctx.cr->fill();
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
        ctx.cr->set_font_size(10);
        ctx.cr->move_to(cx + 16, dy + 4);
        ctx.cr->show_text("Event " + std::to_string(i + 1));
    }
    (void)node; (void)content_width;
    return h + PARA_SPACING;
}

double render_stats(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 60;
    int cols = 3;
    int cw = content_width / cols;
    for (int i = 0; i < cols; i++) {
        double x = ctx.margin_left + i * cw + cw / 2.0;
        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::BOLD);
        ctx.cr->set_font_size(22);
        ctx.cr->move_to(x - 20, ctx.y + 30);
        ctx.cr->show_text("42");
        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
        ctx.cr->set_font_size(9);
        ctx.cr->move_to(x - 20, ctx.y + 48);
        ctx.cr->show_text("metric");
    }
    (void)node;
    return h + PARA_SPACING;
}

double render_steps(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double step_h = 28;
    int count = 4;
    for (int i = 0; i < count; i++) {
        double sy = ctx.y + i * step_h;
        // Number circle
        ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
        ctx.cr->arc(ctx.margin_left + 12, sy + 12, 10, 0, 2 * 3.14159);
        ctx.cr->fill();
        ctx.cr->set_source_rgb(0.016, 0.016, 0.016);
        ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::BOLD);
        ctx.cr->set_font_size(10);
        ctx.cr->move_to(ctx.margin_left + 8, sy + 16);
        ctx.cr->show_text(std::to_string(i + 1));
        // Label
        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
        ctx.cr->set_font_size(11);
        ctx.cr->move_to(ctx.margin_left + 30, sy + 16);
        ctx.cr->show_text("Step " + std::to_string(i + 1));
    }
    (void)node; (void)content_width;
    return count * step_h + PARA_SPACING;
}

double render_accordion(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 30;
    ctx.cr->set_source_rgba(1, 1, 1, 0.03);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h);
    ctx.cr->fill();
    ctx.cr->set_source_rgba(1, 1, 1, 0.06);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h);
    ctx.cr->stroke();
    // Arrow + label
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(11);
    ctx.cr->move_to(ctx.margin_left + 8, ctx.y + 20);
    ctx.cr->show_text("\xe2\x96\xb6 Accordion Section"); // ▶
    (void)node;
    return h + PARA_SPACING;
}

// ── Leaf Directive Renderers ────────────────────────────────────

double render_badge(RenderContext& ctx, int content_width) {
    double h = 20;
    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.15);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, 60, h);
    ctx.cr->fill();
    ctx.cr->set_source_rgb(CYAN_R, CYAN_G, CYAN_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(9);
    ctx.cr->move_to(ctx.margin_left + 6, ctx.y + 14);
    ctx.cr->show_text("badge");
    (void)content_width;
    return h + 4;
}

double render_divider(RenderContext& ctx, int content_width) {
    ctx.y += 8;
    ctx.cr->set_source_rgba(CYAN_R, CYAN_G, CYAN_B, 0.2);
    ctx.cr->set_line_width(1);
    ctx.cr->move_to(ctx.margin_left, ctx.y);
    ctx.cr->line_to(ctx.margin_left + content_width, ctx.y);
    ctx.cr->stroke();
    return 16;
}

double render_spacer(RenderContext& ctx) {
    (void)ctx;
    return 32; // just vertical space
}

double render_progress(RenderContext& ctx, int content_width) {
    double h = 8;
    ctx.cr->set_source_rgba(1, 1, 1, 0.05);
    ctx.cr->rectangle(ctx.margin_left, ctx.y + 4, content_width, h);
    ctx.cr->fill();
    ctx.cr->set_source_rgb(GREEN_R, GREEN_G, GREEN_B);
    ctx.cr->rectangle(ctx.margin_left, ctx.y + 4, content_width * 0.65, h);
    ctx.cr->fill();
    return h + 12;
}

double render_kbd(RenderContext& ctx, const ase::markdown::Node* node) {
    std::string text;
    collect_text(node, text);
    if (text.empty()) text = "Ctrl+K";
    double h = 22;
    ctx.cr->set_source_rgba(1, 1, 1, 0.08);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, text.size() * 8 + 16, h);
    ctx.cr->fill();
    ctx.cr->set_source_rgba(1, 1, 1, 0.12);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, text.size() * 8 + 16, h);
    ctx.cr->stroke();
    ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(10);
    ctx.cr->move_to(ctx.margin_left + 8, ctx.y + 15);
    ctx.cr->show_text(text);
    return h + 4;
}

// ── Generic placeholder for unimplemented directives ────────────

double render_generic(RenderContext& ctx, const ase::markdown::Node* node, int content_width) {
    double h = 40;
    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.06);
    ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h);
    ctx.cr->fill();
    // Dashed border
    ctx.cr->set_source_rgba(PURPLE_R, PURPLE_G, PURPLE_B, 0.2);
    ctx.cr->set_line_width(1);
    std::vector<double> dashes = {3.0, 3.0};
    ctx.cr->set_dash(dashes, 0);
    ctx.cr->rectangle(ctx.margin_left + 1, ctx.y + 1, content_width - 2, h - 2);
    ctx.cr->stroke();
    ctx.cr->unset_dash();
    // Label
    const char* name = node->directive_name ? node->directive_name : "directive";
    ctx.cr->set_source_rgb(PURPLE_R, PURPLE_G, PURPLE_B);
    ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx.cr->set_font_size(10);
    ctx.cr->move_to(ctx.margin_left + content_width / 2.0 - 30, ctx.y + h / 2.0 + 4);
    ctx.cr->show_text(name);
    return h + PARA_SPACING;
}

}  // anonymous namespace

double render_directive(RenderContext& ctx, const ase::markdown::Node* node) {
    using namespace ase::markdown;
    int content_width = ctx.width - ctx.margin_left - ctx.margin_right;
    double h = 0;

    // Block directives
    if (name_is(node, "hero"))        h = render_hero(ctx, node, content_width);
    else if (name_is(node, "columns")) h = render_columns(ctx, node, content_width);
    else if (name_is(node, "cards"))   h = render_cards(ctx, node, content_width);
    else if (name_is(node, "tabs"))    h = render_tabs(ctx, node, content_width);
    else if (name_is(node, "timeline")) h = render_timeline(ctx, node, content_width);
    else if (name_is(node, "stats"))   h = render_stats(ctx, node, content_width);
    else if (name_is(node, "steps"))   h = render_steps(ctx, node, content_width);
    else if (name_is(node, "accordion")) h = render_accordion(ctx, node, content_width);
    // Leaf directives
    else if (name_is(node, "badge"))    h = render_badge(ctx, content_width);
    else if (name_is(node, "divider"))  h = render_divider(ctx, content_width);
    else if (name_is(node, "spacer"))   h = render_spacer(ctx);
    else if (name_is(node, "progress")) h = render_progress(ctx, content_width);
    else if (name_is(node, "kbd"))      h = render_kbd(ctx, node);
    // Generic fallback
    else h = render_generic(ctx, node, content_width);

    ctx.y += h;
    return h;
}

}  // namespace ase::viewer::render::dsl
