#include "vwr_rndr_doc.hpp"
#include "vwr_rndr_text.hpp"
#include "dsl/vwr_dsl_dispatch.hpp"
#include <ase/markdown/types.hpp>
#include <cstring>
#include <sstream>
#include <vector>

namespace ase::viewer::render {

namespace {

// Heading colors (matching DocsViewer CSS vars)
constexpr double H1_R = 0.88, H1_G = 0.88, H1_B = 0.88;  // #E0E0E0
constexpr double H2_R = 0.75, H2_G = 0.75, H2_B = 0.75;  // #C0C0C0
constexpr double H3_R = 0.63, H3_G = 0.63, H3_B = 0.63;  // #A0A0A0
constexpr double H4_R = 0.54, H4_G = 0.60, H4_B = 0.60;  // #8A9A9A

// Text colors
constexpr double TEXT_R = 0.54, TEXT_G = 0.60, TEXT_B = 0.60;  // TEXT_PRIMARY
constexpr double MUTED_R = 0.35, MUTED_G = 0.35, MUTED_B = 0.35;  // TEXT_LABEL
constexpr double LINK_R = 0.35, LINK_G = 0.61, LINK_B = 0.72;  // PANEL_CYAN
constexpr double CODE_BG_R = 0.047, CODE_BG_G = 0.047, CODE_BG_B = 0.047;  // SURFACE_1_CODE

// Callout colors
constexpr double INFO_R = 0.35, INFO_G = 0.61, INFO_B = 0.72;     // cyan
constexpr double WARN_R = 0.82, WARN_G = 0.60, WARN_B = 0.13;     // amber
constexpr double TIP_R  = 0.25, TIP_G  = 0.73, TIP_B  = 0.31;     // green
constexpr double NOTE_R = 0.65, NOTE_G = 0.55, NOTE_B = 0.98;     // purple

// Spacing
constexpr double PARA_SPACING   = 10.0;
constexpr double HEAD_SPACING_T = 24.0;
constexpr double HEAD_SPACING_B = 8.0;
constexpr double CODE_PADDING   = 8.0;
constexpr double LIST_INDENT    = 20.0;
constexpr double HR_SPACING     = 16.0;
constexpr double CALLOUT_PAD    = 12.0;

void set_heading_color(const Cairo::RefPtr<Cairo::Context>& cr, uint8_t level) {
    switch (level) {
        case 1: cr->set_source_rgb(H1_R, H1_G, H1_B); break;
        case 2: cr->set_source_rgb(H2_R, H2_G, H2_B); break;
        case 3: cr->set_source_rgb(H3_R, H3_G, H3_B); break;
        default: cr->set_source_rgb(H4_R, H4_G, H4_B); break;
    }
}

struct CalloutRGB { double r, g, b; };

CalloutRGB get_callout_rgb(uint8_t type) {
    switch (type) {
        case ase::markdown::CALLOUT_INFO:    return {INFO_R, INFO_G, INFO_B};
        case ase::markdown::CALLOUT_WARNING: return {WARN_R, WARN_G, WARN_B};
        case ase::markdown::CALLOUT_TIP:     return {TIP_R, TIP_G, TIP_B};
        case ase::markdown::CALLOUT_NOTE:    return {NOTE_R, NOTE_G, NOTE_B};
        default: return {MUTED_R, MUTED_G, MUTED_B};
    }
}

void set_callout_color(const Cairo::RefPtr<Cairo::Context>& cr, uint8_t type) {
    auto c = get_callout_rgb(type);
    cr->set_source_rgb(c.r, c.g, c.b);
}

// Collect all text content from a node subtree
void collect_text(const ase::markdown::Node* node, std::string& out) {
    if (!node) return;
    if (node->text && node->text_len > 0) {
        out.append(node->text, node->text_len);
    }
    const ase::markdown::Node* child = node->first_child;
    while (child) {
        collect_text(child, out);
        child = child->next_sibling;
    }
}

}  // anonymous namespace

void render_node(RenderContext& ctx, const ase::markdown::Node* node) {
    if (!node) return;
    using namespace ase::markdown;

    int content_width = ctx.width - ctx.margin_left - ctx.margin_right;

    switch (node->type) {
        case NODE_HEADING: {
            ctx.y += HEAD_SPACING_T;
            auto layout = create_heading_layout(ctx.cr, content_width, node->heading_level);
            std::string text;
            collect_text(node, text);
            layout->set_text(text);
            set_heading_color(ctx.cr, node->heading_level);
            draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
            int w = 0, h = 0;
            layout->get_pixel_size(w, h);
            ctx.y += h + HEAD_SPACING_B;

            // H1 bottom border
            if (node->heading_level == 1) {
                ctx.cr->set_source_rgba(1, 1, 1, 0.05);
                ctx.cr->move_to(ctx.margin_left, ctx.y);
                ctx.cr->line_to(ctx.width - ctx.margin_right, ctx.y);
                ctx.cr->stroke();
                ctx.y += 8;
            }
            break;
        }

        case NODE_PARAGRAPH: {
            auto layout = create_body_layout(ctx.cr, content_width);
            std::string text;
            collect_text(node, text);
            layout->set_text(text);
            ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            draw_layout(ctx.cr, layout, ctx.margin_left, ctx.y);
            int w = 0, h = 0;
            layout->get_pixel_size(w, h);
            ctx.y += h + PARA_SPACING;
            break;
        }

        case NODE_CODE_BLOCK: {
            ctx.y += 4;
            auto layout = create_code_layout(ctx.cr, content_width - 2 * static_cast<int>(CODE_PADDING));
            std::string text;
            if (node->text && node->text_len > 0) text.assign(node->text, node->text_len);
            layout->set_text(text);
            int w = 0, h = 0;
            layout->get_pixel_size(w, h);

            ctx.cr->set_source_rgb(CODE_BG_R, CODE_BG_G, CODE_BG_B);
            ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h + 2 * CODE_PADDING);
            ctx.cr->fill();

            if (node->language) {
                ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
                ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
                ctx.cr->set_font_size(9);
                ctx.cr->move_to(ctx.margin_left + CODE_PADDING, ctx.y + 12);
                ctx.cr->show_text(node->language);
                ctx.y += 16;
            }

            ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            draw_layout(ctx.cr, layout, ctx.margin_left + CODE_PADDING, ctx.y + CODE_PADDING);
            ctx.y += h + 2 * CODE_PADDING + PARA_SPACING;
            break;
        }

        case NODE_DIFF_BLOCK: {
            ctx.y += 4;
            std::string text;
            if (node->text && node->text_len > 0) text.assign(node->text, node->text_len);

            // Background
            ctx.cr->set_source_rgb(CODE_BG_R, CODE_BG_G, CODE_BG_B);
            double line_h = 16.0;
            int line_count = 1;
            for (char c : text) if (c == '\n') line_count++;
            double block_h = line_count * line_h + 2 * CODE_PADDING;
            ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, block_h);
            ctx.cr->fill();

            // Render line by line with diff coloring
            double ly = ctx.y + CODE_PADDING;
            std::istringstream stream(text);
            std::string line;
            while (std::getline(stream, line)) {
                // Line background based on prefix
                if (!line.empty()) {
                    if (line[0] == '+') {
                        ctx.cr->set_source_rgba(0.25, 0.73, 0.31, 0.12);
                        ctx.cr->rectangle(ctx.margin_left, ly - 1, content_width, line_h);
                        ctx.cr->fill();
                        ctx.cr->set_source_rgb(0.25, 0.73, 0.31);
                    } else if (line[0] == '-') {
                        ctx.cr->set_source_rgba(0.97, 0.32, 0.29, 0.12);
                        ctx.cr->rectangle(ctx.margin_left, ly - 1, content_width, line_h);
                        ctx.cr->fill();
                        ctx.cr->set_source_rgb(0.97, 0.32, 0.29);
                    } else if (line.size() >= 2 && line[0] == '-' && line[1] == '>') {
                        ctx.cr->set_source_rgba(0.35, 0.61, 0.72, 0.12);
                        ctx.cr->rectangle(ctx.margin_left, ly - 1, content_width, line_h);
                        ctx.cr->fill();
                        ctx.cr->set_source_rgb(0.35, 0.61, 0.72);
                    } else if (line[0] == '[') {
                        ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
                    } else {
                        ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
                    }
                }

                ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
                ctx.cr->set_font_size(10);
                ctx.cr->move_to(ctx.margin_left + CODE_PADDING, ly + 11);
                ctx.cr->show_text(line);
                ly += line_h;
            }
            ctx.y += block_h + PARA_SPACING;
            break;
        }

        case NODE_MERMAID_BLOCK:
        case NODE_SVGBOB_BLOCK:
        case NODE_ASEMATH_BLOCK: {
            // Placeholder box until Graphviz/MicroTeX/muparser integration
            ctx.y += 4;
            double box_h = 80;
            ctx.cr->set_source_rgb(CODE_BG_R, CODE_BG_G, CODE_BG_B);
            ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, box_h);
            ctx.cr->fill();

            // Dashed border
            ctx.cr->set_source_rgba(LINK_R, LINK_G, LINK_B, 0.3);
            ctx.cr->set_line_width(1);
            std::vector<double> dashes = {4.0, 4.0};
            ctx.cr->set_dash(dashes, 0);
            ctx.cr->rectangle(ctx.margin_left + 1, ctx.y + 1, content_width - 2, box_h - 2);
            ctx.cr->stroke();
            ctx.cr->unset_dash();

            // Label
            const char* label = "mermaid";
            if (node->type == NODE_SVGBOB_BLOCK) label = "svgbob";
            else if (node->type == NODE_ASEMATH_BLOCK) label = "ase-math";
            ctx.cr->set_source_rgb(LINK_R, LINK_G, LINK_B);
            ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
            ctx.cr->set_font_size(11);
            ctx.cr->move_to(ctx.margin_left + content_width / 2.0 - 30, ctx.y + box_h / 2.0 + 4);
            ctx.cr->show_text(label);

            ctx.y += box_h + PARA_SPACING;
            break;
        }

        case NODE_CALLOUT: {
            ctx.y += 4;
            // Collect callout content
            std::string text;
            collect_text(node, text);
            auto layout = create_body_layout(ctx.cr, content_width - 28);
            layout->set_text(text);
            int w = 0, h = 0;
            layout->get_pixel_size(w, h);

            // Background tint
            auto callout_c = get_callout_rgb(node->callout_type);
            ctx.cr->set_source_rgba(callout_c.r, callout_c.g, callout_c.b, 0.08);
            ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h + 2 * CALLOUT_PAD);
            ctx.cr->fill();

            // Left border
            set_callout_color(ctx.cr, node->callout_type);
            ctx.cr->set_line_width(3);
            ctx.cr->move_to(ctx.margin_left, ctx.y);
            ctx.cr->line_to(ctx.margin_left, ctx.y + h + 2 * CALLOUT_PAD);
            ctx.cr->stroke();

            // Text
            ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
            draw_layout(ctx.cr, layout, ctx.margin_left + 16, ctx.y + CALLOUT_PAD);
            ctx.y += h + 2 * CALLOUT_PAD + PARA_SPACING;
            break;
        }

        case NODE_LIST: {
            const ase::markdown::Node* item = node->first_child;
            int index = node->list_start;
            while (item) {
                std::string text;
                collect_text(item, text);
                auto layout = create_body_layout(ctx.cr, content_width - static_cast<int>(LIST_INDENT));
                layout->set_text(text);
                ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
                ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
                ctx.cr->set_font_size(11);
                ctx.cr->move_to(ctx.margin_left, ctx.y + 2);
                if (node->list_ordered) {
                    ctx.cr->show_text(std::to_string(index++) + ".");
                } else {
                    ctx.cr->show_text("\xe2\x86\x92"); // →
                }
                ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);
                draw_layout(ctx.cr, layout, ctx.margin_left + LIST_INDENT, ctx.y);
                int w = 0, h = 0;
                layout->get_pixel_size(w, h);
                ctx.y += h + 2;
                item = item->next_sibling;
            }
            ctx.y += PARA_SPACING;
            break;
        }

        case NODE_BLOCKQUOTE: {
            ctx.y += 4;
            std::string text;
            collect_text(node, text);
            auto layout = create_body_layout(ctx.cr, content_width - 20);
            layout->set_text(text);
            int w = 0, h = 0;
            layout->get_pixel_size(w, h);

            // Background
            ctx.cr->set_source_rgba(1, 1, 1, 0.02);
            ctx.cr->rectangle(ctx.margin_left, ctx.y, content_width, h + 16);
            ctx.cr->fill();

            // Left border
            ctx.cr->set_source_rgba(1, 1, 1, 0.08);
            ctx.cr->set_line_width(2);
            ctx.cr->move_to(ctx.margin_left, ctx.y);
            ctx.cr->line_to(ctx.margin_left, ctx.y + h + 16);
            ctx.cr->stroke();

            ctx.cr->set_source_rgb(MUTED_R, MUTED_G, MUTED_B);
            draw_layout(ctx.cr, layout, ctx.margin_left + 12, ctx.y + 8);
            ctx.y += h + 16 + PARA_SPACING;
            break;
        }

        case NODE_THEMATIC_BREAK: {
            ctx.y += HR_SPACING;
            ctx.cr->set_source_rgba(1, 1, 1, 0.05);
            ctx.cr->move_to(ctx.margin_left, ctx.y);
            ctx.cr->line_to(ctx.width - ctx.margin_right, ctx.y);
            ctx.cr->stroke();
            ctx.y += HR_SPACING;
            break;
        }

        case NODE_TABLE: {
            // Simple table rendering: iterate rows and cells
            const ase::markdown::Node* row = node->first_child;
            bool is_header = true;
            while (row) {
                const ase::markdown::Node* cell = row->first_child;
                double x = ctx.margin_left;
                int cell_count = 0;
                const ase::markdown::Node* c = cell;
                while (c) { cell_count++; c = c->next_sibling; }
                int cell_width = cell_count > 0 ? content_width / cell_count : content_width;

                while (cell) {
                    std::string text;
                    collect_text(cell, text);
                    auto layout = create_body_layout(ctx.cr, cell_width - 8);
                    if (is_header) {
                        auto fd = layout->get_font_description();
                        fd.set_weight(Pango::Weight::BOLD);
                        layout->set_font_description(fd);
                    }
                    layout->set_text(text);

                    if (is_header) ctx.cr->set_source_rgb(H3_R, H3_G, H3_B);
                    else ctx.cr->set_source_rgb(TEXT_R, TEXT_G, TEXT_B);

                    draw_layout(ctx.cr, layout, x + 4, ctx.y + 4);

                    // Cell border
                    ctx.cr->set_source_rgba(1, 1, 1, 0.05);
                    ctx.cr->rectangle(x, ctx.y, cell_width, 24);
                    ctx.cr->stroke();

                    x += cell_width;
                    cell = cell->next_sibling;
                }
                ctx.y += 24;
                is_header = false;
                row = row->next_sibling;
            }
            ctx.y += PARA_SPACING;
            break;
        }

        case NODE_BLOCK_DIRECTIVE:
        case NODE_LEAF_DIRECTIVE:
        case NODE_TEXT_DIRECTIVE: {
            dsl::render_directive(ctx, node);
            break;
        }

        case NODE_WIKI_LINK: {
            // [[Wiki Link]] — rendered as clickable cyan text
            std::string text;
            collect_text(node, text);
            ctx.cr->set_source_rgb(LINK_R, LINK_G, LINK_B);
            ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
            ctx.cr->set_font_size(11);
            ctx.cr->move_to(ctx.margin_left, ctx.y + 14);
            ctx.cr->show_text("[[" + text + "]]");
            ctx.y += 20;
            break;
        }

        case NODE_GLOSSARY_TERM: {
            // {{Glossary}} — rendered with dotted underline
            std::string text;
            collect_text(node, text);
            ctx.cr->set_source_rgb(0.65, 0.55, 0.98);
            ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
            ctx.cr->set_font_size(11);
            ctx.cr->move_to(ctx.margin_left, ctx.y + 14);
            ctx.cr->show_text(text);
            // Dotted underline
            std::vector<double> dots = {2.0, 2.0};
            ctx.cr->set_dash(dots, 0);
            ctx.cr->set_line_width(1);
            ctx.cr->move_to(ctx.margin_left, ctx.y + 17);
            ctx.cr->line_to(ctx.margin_left + text.size() * 7, ctx.y + 17);
            ctx.cr->stroke();
            ctx.cr->unset_dash();
            ctx.y += 20;
            break;
        }

        case NODE_CROSS_REF: {
            // {ref:path#anchor} — cyan link
            std::string text;
            if (node->url) text = node->url;
            else collect_text(node, text);
            ctx.cr->set_source_rgb(LINK_R, LINK_G, LINK_B);
            ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
            ctx.cr->set_font_size(10);
            ctx.cr->move_to(ctx.margin_left, ctx.y + 14);
            ctx.cr->show_text("\xe2\x86\x92 " + text); // → ref
            ctx.y += 20;
            break;
        }

        case NODE_VERSION_INSERT: {
            // {version} — inline version placeholder
            ctx.cr->set_source_rgba(LINK_R, LINK_G, LINK_B, 0.6);
            ctx.cr->select_font_face("Fira Code", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
            ctx.cr->set_font_size(9);
            ctx.cr->move_to(ctx.margin_left, ctx.y + 12);
            ctx.cr->show_text("{version}");
            ctx.y += 18;
            break;
        }

        default: {
            // For unhandled container nodes, recurse into children
            const ase::markdown::Node* child = node->first_child;
            while (child) {
                render_node(ctx, child);
                child = child->next_sibling;
            }
            break;
        }
    }
}

double render_document(RenderContext& ctx, const ase::markdown::Document& doc) {
    ctx.y = ctx.margin_top;
    if (doc.root) {
        const ase::markdown::Node* child = doc.root->first_child;
        while (child) {
            render_node(ctx, child);
            child = child->next_sibling;
        }
    }
    return ctx.y + ctx.margin_top;
}

}  // namespace ase::viewer::render
